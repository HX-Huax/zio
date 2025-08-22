#pragma once

#include "audio_buffer.h"
#include <fstream>

namespace zio {

class FileWriter {
public:
  FileWriter();
  ~FileWriter();

  void start();
  void stop();
  void enqueue_save_task(std::vector<AudioChunk> chunks,
                         const std::filesystem::path &base_path);

private:
  void writer_thread();

  struct SaveTask {
    std::vector<AudioChunk> chunks;
    std::filesystem::path base_path;
  };

  // WAV file format helpers
  struct WAVHeader {
    char chunk_id[4] = {'R', 'I', 'F', 'F'};
    uint32_t chunk_size;
    char format[4] = {'W', 'A', 'V', 'E'};
    char subchunk1_id[4] = {'f', 'm', 't', ' '};
    uint32_t subchunk1_size = 16;
    uint16_t audio_format = 1; // PCM
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample = 16;
    char subchunk2_id[4] = {'d', 'a', 't', 'a'};
    uint32_t subchunk2_size;
  };

  void write_wav_file(const SaveTask &task);
  void write_multitrack_wav(const SaveTask &task);

  std::atomic<bool> running_{false};
  std::thread writer_thread_;
  std::queue<SaveTask> task_queue_;
  std::mutex queue_mutex_;
  std::condition_variable queue_cv_;
};

} // namespace zio
