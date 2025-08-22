#include "file_writer.h"
#include <algorithm>
#include <iostream>
#include <ranges>

namespace zio {

FileWriter::FileWriter() : running_(false) {}

FileWriter::~FileWriter() { stop(); }

void FileWriter::start() {
  if (running_)
    return;

  running_ = true;
  writer_thread_ = std::thread(&FileWriter::writer_thread, this);
}

void FileWriter::stop() {
  if (!running_)
    return;

  running_ = false;
  queue_cv_.notify_all();

  if (writer_thread_.joinable()) {
    writer_thread_.join();
  }
}

void FileWriter::enqueue_save_task(std::vector<AudioChunk> chunks,
                                   const std::filesystem::path &base_path) {
  if (chunks.empty())
    return;

  std::lock_guard lock(queue_mutex_);
  task_queue_.push({std::move(chunks), base_path});
  queue_cv_.notify_one();
}

void FileWriter::writer_thread() {
  while (running_) {
    std::unique_lock lock(queue_mutex_);
    queue_cv_.wait(lock,
                   [this]() { return !task_queue_.empty() || !running_; });

    if (!running_ && task_queue_.empty())
      break;

    if (!task_queue_.empty()) {
      SaveTask task = std::move(task_queue_.front());
      task_queue_.pop();
      lock.unlock();

      try {
        write_multitrack_wav(task);
      } catch (const std::exception &e) {
        std::cerr << "Error writing audio file: " << e.what() << std::endl;
      }
    }
  }
}

void FileWriter::write_multitrack_wav(const SaveTask &task) {
  if (task.chunks.empty())
    return;

  // Group chunks by client ID
  std::map<ClientID, std::vector<AudioChunk>> chunks_by_client;
  for (const auto &chunk : task.chunks) {
    chunks_by_client[chunk.client_id].push_back(chunk);
  }

  // Get the start and end timestamps
  auto [min_ts, max_ts] = std::ranges::minmax_element(
      task.chunks, [](const AudioChunk &a, const AudioChunk &b) {
        return a.timestamp_ms < b.timestamp_ms;
      });

  uint64_t start_time = min_ts->timestamp_ms;
  uint64_t end_time = max_ts->timestamp_ms;
  uint64_t duration_ms = end_time - start_time;

  // Create output directory if needed
  std::filesystem::create_directories(task.base_path.parent_path());

  // Generate filename with timestamp
  auto zoned_time = std::chrono::zoned_time{std::chrono::current_zone(),
                                            std::chrono::system_clock::now()};

  // auto now = std::chrono::system_clock::now();
  // auto time_t = std::chrono::system_clock::to_time_t(now);
  std::string timestamp_str = std::format("{:%Y-%m-%d_%H-%M-%S}", zoned_time);

  std::filesystem::path file_path =
      task.base_path / std::format("ts_record_{}.wav", timestamp_str);

  // Write a separate WAV file for each client
  for (const auto &[client_id, chunks] : chunks_by_client) {
    std::filesystem::path client_file_path =
        task.base_path /
        std::format("ts_record_{}_client_{}.wav", timestamp_str, client_id);

    // Prepare WAV header
    WAVHeader header;
    header.num_channels = 1; // Mono
    header.sample_rate = chunks.front().sample_rate;
    header.byte_rate =
        header.sample_rate * header.num_channels * (header.bits_per_sample / 8);
    header.block_align = header.num_channels * (header.bits_per_sample / 8);

    // Calculate total data size
    size_t total_samples = 0;
    for (const auto &chunk : chunks) {
      total_samples += chunk.data.size();
    }

    header.subchunk2_size = total_samples * (header.bits_per_sample / 8);
    header.chunk_size = 36 + header.subchunk2_size;

    // Write WAV file
    std::ofstream file(client_file_path, std::ios::binary);
    if (!file) {
      throw std::runtime_error("Cannot open file for writing: " +
                               client_file_path.string());
    }

    // Write header
    file.write(reinterpret_cast<const char *>(&header), sizeof(header));

    // Write audio data
    for (const auto &chunk : chunks) {
      file.write(reinterpret_cast<const char *>(chunk.data.data()),
                 chunk.data.size() * sizeof(int16_t));
    }

    // Update header with correct sizes (in case of write errors)
    if (file.tellp() != sizeof(header) + header.subchunk2_size) {
      std::cerr << "Warning: File size doesn't match expected size"
                << std::endl;
    }
  }

  // Also write a metadata file with timestamps
  std::filesystem::path meta_file_path =
      task.base_path / std::format("ts_record_{}_meta.txt", timestamp_str);

  std::ofstream meta_file(meta_file_path);
  meta_file << "Recording started at: " << start_time << " ms\n";
  meta_file << "Recording ended at: " << end_time << " ms\n";
  meta_file << "Duration: " << duration_ms << " ms\n";
  meta_file << "Clients recorded: " << chunks_by_client.size() << "\n";

  for (const auto &[client_id, chunks] : chunks_by_client) {
    meta_file << "Client " << client_id << ": " << chunks.size() << " chunks\n";
  }
}

} // namespace zio
