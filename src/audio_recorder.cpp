#include "audio_recorder.h"
#include <cstring>
#include <iostream>

// 假设有访问TeamSpeak客户端信息的函数
extern struct TS3Functions ts3Functions;

namespace zio {

AudioRecorder::AudioRecorder(uint32_t sample_rate, size_t buffer_capacity_ms)
    : sample_rate_(sample_rate), file_writer_(std::make_unique<FileWriter>()) {
  file_writer_->start();
  is_recording_ = true; // 默认开始记录
}

AudioRecorder::~AudioRecorder() { file_writer_->stop(); }

AudioBuffer *AudioRecorder::get_or_create_client_buffer(ClientID client_id) {
  std::lock_guard lock(buffers_mutex_);

  auto it = client_buffers_.find(client_id);
  if (it != client_buffers_.end()) {
    return it->second.get();
  }

  // 创建新缓冲区
  auto buffer =
      std::make_unique<AudioBuffer>(DEFAULT_BUFFER_CAPACITY_MS, sample_rate_);
  auto result = client_buffers_.emplace(client_id, std::move(buffer));
  return result.first->second.get();
}

uint64_t AudioRecorder::get_current_timestamp_ms() const {
  return timestamp_to_ms(std::chrono::steady_clock::now());
}

void AudioRecorder::on_edit_playback_voice_data_event(
    ServerConnectionHandlerID server_id, ClientID client_id, short *samples,
    int sample_count, int channels) {
  if (!is_recording_ || !is_client_in_current_channel(server_id, client_id)) {
    return;
  }

  AudioBuffer *buffer = get_or_create_client_buffer(client_id);
  if (buffer) {
    AudioChunk chunk(samples, sample_count, get_current_timestamp_ms(),
                     client_id, server_id, sample_rate_, channels);
    buffer->push(std::move(chunk));
  }
}

void AudioRecorder::on_edit_post_process_voice_data_event(
    ServerConnectionHandlerID server_id, ClientID client_id, short *samples,
    int sample_count, int channels, const unsigned int *channel_speaker_array,
    unsigned int *channel_fill_mask) {
  // 与on_edit_playback_voice_data_event类似处理
  on_edit_playback_voice_data_event(server_id, client_id, samples, sample_count,
                                    channels);
}

void AudioRecorder::trigger_save(const std::filesystem::path &base_path,
                                 uint64_t pre_time_ms) {
  std::lock_guard lock(buffers_mutex_);

  uint64_t until_timestamp = get_current_timestamp_ms();
  std::vector<AudioChunk> all_chunks;

  // 从所有客户端缓冲区提取指定时间范围的音频数据
  for (const auto &[client_id, buffer] : client_buffers_) {
    auto client_chunks =
        buffer->extract_last_n_ms(pre_time_ms, until_timestamp);
    all_chunks.insert(all_chunks.end(),
                      std::make_move_iterator(client_chunks.begin()),
                      std::make_move_iterator(client_chunks.end()));
  }

  // 按时间戳排序
  std::sort(all_chunks.begin(), all_chunks.end(),
            [](const AudioChunk &a, const AudioChunk &b) {
              return a.timestamp_ms < b.timestamp_ms;
            });

  // 提交保存任务
  file_writer_->enqueue_save_task(std::move(all_chunks), base_path);
}

void AudioRecorder::set_current_channel(ServerConnectionHandlerID server_id,
                                        uint64_t channel_id) {
  current_server_id_ = server_id;
  current_channel_id_ = channel_id;
}

bool AudioRecorder::is_client_in_current_channel(
    ServerConnectionHandlerID server_id, ClientID client_id) const {
  // 如果未设置当前频道，则记录所有客户端
  if (current_server_id_ == 0 || current_channel_id_ == 0) {
    return true;
  }

  // 检查服务器连接是否匹配
  if (server_id != current_server_id_) {
    return false;
  }

  // 使用TeamSpeak API获取客户端信息并检查频道
  // 使用SDK版本26的API
  uint64_t client_channel_id = 0;
  int error = 0;

  // 获取客户端频道ID - 使用正确的常量名称
  error = ts3Functions.getClientVariableAsUInt64(
      server_id, client_id, CLIENT_CHANNEL_ID, &client_channel_id);
  if (error != ERROR_ok) {
    // 如果获取失败，尝试通过客户端列表获取
    anyID *client_list = nullptr;
    error = ts3Functions.getClientList(server_id, &client_list);
    if (error == ERROR_ok && client_list) {
      // 遍历客户端列表
      for (int i = 0; client_list[i] != 0; i++) {
        if (client_list[i] == client_id) {
          // 再次尝试获取频道ID
          error = ts3Functions.getClientVariableAsUInt64(
              server_id, client_id, CLIENT_CHANNEL_ID, &client_channel_id);
          if (error == ERROR_ok) {
            ts3Functions.freeMemory(client_list);
            return client_channel_id == current_channel_id_;
          }
          break;
        }
      }
      ts3Functions.freeMemory(client_list);
    }

    // 如果所有方法都失败，默认返回true（记录该客户端）
    return true;
  }

  return client_channel_id == current_channel_id_;
}

size_t AudioRecorder::get_buffer_size_ms() const {
  std::lock_guard lock(buffers_mutex_);
  size_t total_ms = 0;

  for (const auto &[client_id, buffer] : client_buffers_) {
    total_ms += buffer->get_size_ms();
  }

  return client_buffers_.empty() ? 0 : total_ms / client_buffers_.size();
}

// 添加开始/停止记录的方法
void AudioRecorder::start_recording() { is_recording_ = true; }

void AudioRecorder::stop_recording() { is_recording_ = false; }

} // namespace zio
