#pragma once

#include "audio_buffer.h"
#include "file_writer.h"

namespace zio {

class AudioRecorder {
public:
  AudioRecorder(uint32_t sample_rate = DEFAULT_SAMPLE_RATE,
                size_t buffer_capacity_ms = DEFAULT_BUFFER_CAPACITY_MS);
  ~AudioRecorder();

  // TeamSpeak回调处理
  void on_edit_playback_voice_data_event(ServerConnectionHandlerID server_id,
                                         ClientID client_id, short *samples,
                                         int sample_count, int channels);

  void on_edit_post_process_voice_data_event(
      ServerConnectionHandlerID server_id, ClientID client_id, short *samples,
      int sample_count, int channels, const unsigned int *channel_speaker_array,
      unsigned int *channel_fill_mask);

  // 触发保存
  void trigger_save(const std::filesystem::path &base_path,
                    uint64_t pre_time_ms = DEFAULT_PRE_SAVE_TIME_MS);

  // 频道管理
  void set_current_channel(ServerConnectionHandlerID server_id,
                           uint64_t channel_id);
  bool is_client_in_current_channel(ServerConnectionHandlerID server_id,
                                    ClientID client_id) const;

  // 状态查询
  bool is_recording() const { return is_recording_; }
  size_t get_buffer_size_ms() const;

  // 开始/停止记录
  void start_recording();
  void stop_recording();

private:
  mutable std::mutex buffers_mutex_;
  std::map<ClientID, std::unique_ptr<AudioBuffer>> client_buffers_;
  std::unique_ptr<FileWriter> file_writer_;

  ServerConnectionHandlerID current_server_id_{0};
  uint64_t current_channel_id_{0};
  uint32_t sample_rate_;

  std::atomic<bool> is_recording_{false};

  // 获取或创建客户端缓冲区
  AudioBuffer *get_or_create_client_buffer(ClientID client_id);

  // 时间戳生成
  uint64_t get_current_timestamp_ms() const;
};

} // namespace zio
