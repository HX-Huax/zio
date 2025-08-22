#pragma once

#include "zio_includes.h"

namespace zio {

struct AudioChunk {
  std::vector<int16_t> data;
  uint64_t timestamp_ms;
  ClientID client_id;
  ServerConnectionHandlerID server_id;
  uint32_t sample_rate;
  uint16_t channels;

  AudioChunk() = default;
  AudioChunk(const int16_t *samples, size_t count, uint64_t ts_ms, ClientID cid,
             ServerConnectionHandlerID sid, uint32_t sr = DEFAULT_SAMPLE_RATE,
             uint16_t ch = 1)
      : data(samples, samples + count), timestamp_ms(ts_ms), client_id(cid),
        server_id(sid), sample_rate(sr), channels(ch) {}
};

class AudioBuffer {
public:
  AudioBuffer(size_t capacity_ms, uint32_t sample_rate);
  ~AudioBuffer() = default;

  void push(AudioChunk &&chunk);
  std::vector<AudioChunk> extract_last_n_ms(uint64_t ms,
                                            uint64_t until_timestamp_ms = 0);

  size_t get_capacity_ms() const { return capacity_ms_; }
  size_t get_size_ms() const;

private:
  const size_t capacity_ms_;
  const uint32_t sample_rate_;

  std::mutex mutex_;
  std::deque<AudioChunk> buffer_;
  size_t total_samples_ = 0;

  size_t samples_to_ms(size_t samples) const;
  size_t ms_to_samples(size_t ms) const;
};

} // namespace zio
