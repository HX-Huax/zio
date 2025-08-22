#include "audio_buffer.h"
#include <algorithm>

namespace zio {

AudioBuffer::AudioBuffer(size_t capacity_ms, uint32_t sample_rate)
    : capacity_ms_(capacity_ms), sample_rate_(sample_rate) {}

size_t AudioBuffer::samples_to_ms(size_t samples) const {
  return (samples * 1000) / sample_rate_;
}

size_t AudioBuffer::ms_to_samples(size_t ms) const {
  return (ms * sample_rate_) / 1000;
}

void AudioBuffer::push(AudioChunk &&chunk) {
  std::lock_guard lock(mutex_);

  // Add new chunk
  buffer_.push_back(std::move(chunk));
  total_samples_ += buffer_.back().data.size();

  // Remove old chunks if exceeding capacity
  while (!buffer_.empty() && samples_to_ms(total_samples_) > capacity_ms_) {
    total_samples_ -= buffer_.front().data.size();
    buffer_.pop_front();
  }
}

std::vector<AudioChunk>
AudioBuffer::extract_last_n_ms(uint64_t ms, uint64_t until_timestamp_ms) {
  std::lock_guard lock(mutex_);
  std::vector<AudioChunk> result;

  if (buffer_.empty())
    return result;

  // If no until timestamp specified, use the latest timestamp
  if (until_timestamp_ms == 0) {
    until_timestamp_ms = buffer_.back().timestamp_ms;
  }

  // Calculate the minimum timestamp to include
  uint64_t min_timestamp =
      (until_timestamp_ms > ms) ? until_timestamp_ms - ms : 0;

  // Find chunks within the time range
  for (const auto &chunk : buffer_) {
    if (chunk.timestamp_ms >= min_timestamp &&
        chunk.timestamp_ms <= until_timestamp_ms) {
      result.push_back(chunk);
    }
  }

  return result;
}

size_t AudioBuffer::get_size_ms() const {
  std::lock_guard lock(mutex_);
  return samples_to_ms(total_samples_);
}

} // namespace zio
