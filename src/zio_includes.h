#pragma once

// Standard library includes
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

// TeamSpeak SDK includes
#include <plugin_definitions.h>
#include <teamspeak/public_definitions.h>
#include <teamspeak/public_errors.h>
#include <teamspeak/public_rare_definitions.h>
#include <ts3_functions.h>

// C++20/23 features
#include <format>
#include <span>
// 定义TeamSpeak常量（如果SDK中没有提供）
#ifndef CLIENT_CHANNEL_ID
#define CLIENT_CHANNEL_ID                                                      \
  0x0D // 这是TeamSpeak SDK中常见的值，但请根据实际SDK调整
#endif
namespace zio {
// Common types
using ClientID = anyID;
using ServerConnectionHandlerID = uint64_t;
using Timestamp = std::chrono::steady_clock::time_point;

// Constants
constexpr uint32_t DEFAULT_SAMPLE_RATE = 48000;
constexpr size_t DEFAULT_BUFFER_CAPACITY_MS = 300000; // 5 minutes
constexpr size_t DEFAULT_PRE_SAVE_TIME_MS = 30000;    // 30 seconds

// Utility functions
inline uint64_t timestamp_to_ms(Timestamp ts) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             ts.time_since_epoch())
      .count();
}

inline Timestamp ms_to_timestamp(uint64_t ms) {
  return Timestamp(std::chrono::milliseconds(ms));
}
} // namespace zio
