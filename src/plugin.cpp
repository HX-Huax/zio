#include "audio_recorder.h"
#include "zio_includes.h"

#include <cstring>
#include <iostream>

static struct TS3Functions ts3Functions;
static std::unique_ptr<zio::AudioRecorder> audio_recorder;

// 插件命令处理
static void handle_command(const char *command);

// TeamSpeak插件必需的回调函数
extern "C" {
const char *ts3plugin_name() { return "ZIO Voice Recorder"; }
const char *ts3plugin_version() { return "1.0"; }
int ts3plugin_apiVersion() { return 26; }
const char *ts3plugin_author() { return "HX"; }
const char *ts3plugin_description() {
  return "Records voice in current channel";
}

int ts3plugin_init() {
  std::cout << "ZIO Voice Recorder plugin initializing..." << std::endl;
  audio_recorder = std::make_unique<zio::AudioRecorder>();
  return 0;
}

void ts3plugin_shutdown() {
  std::cout << "ZIO Voice Recorder plugin shutting down..." << std::endl;
  audio_recorder.reset();
}

void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
  ts3Functions = funcs;
}

int ts3plugin_offersConfigure() { return PLUGIN_OFFERS_CONFIGURE_QT_THREAD; }

void ts3plugin_configure(void *handle, void *qParentWidget) {
  // 配置对话框实现
  std::cout << "Configure called" << std::endl;
}

int ts3plugin_requestAutoload() {
  return 0; // 1表示自动加载，0表示不自动加载
}

// 插件命令处理
int ts3plugin_processCommand(uint64 serverConnectionHandlerID,
                             const char *command) {
  handle_command(command);
  return 0; // 0: 命令已处理，1: 命令未处理
}

// 频道切换事件
void ts3plugin_currentChannelChanged(uint64 serverConnectionHandlerID,
                                     uint64 channelID) {
  if (audio_recorder) {
    audio_recorder->set_current_channel(serverConnectionHandlerID, channelID);
    char msg[256];
    snprintf(msg, sizeof(msg), "Now recording channel: %llu",
             (unsigned long long)channelID);
    ts3Functions.printMessageToCurrentTab(msg);
  }
}

// 语音数据处理回调
void ts3plugin_onEditPlaybackVoiceDataEvent(uint64 serverConnectionHandlerID,
                                            anyID clientID, short *samples,
                                            int sampleCount, int channels) {
  if (audio_recorder) {
    audio_recorder->on_edit_playback_voice_data_event(
        serverConnectionHandlerID, clientID, samples, sampleCount, channels);
  }
}

// 后处理语音数据回调
void ts3plugin_onEditPostProcessVoiceDataEvent(
    uint64 serverConnectionHandlerID, anyID clientID, short *samples,
    int sampleCount, int channels, const unsigned int *channelSpeakerArray,
    unsigned int *channelFillMask) {
  if (audio_recorder) {
    audio_recorder->on_edit_post_process_voice_data_event(
        serverConnectionHandlerID, clientID, samples, sampleCount, channels,
        channelSpeakerArray, channelFillMask);
  }
}
}

// 命令处理实现
static void handle_command(const char *command) {
  if (std::strncmp(command, "!ziorecord", 10) == 0) {
    if (audio_recorder) {
      audio_recorder->trigger_save("/home/hx/Recordings");
      ts3Functions.printMessageToCurrentTab("Recording saved!");
    }
  } else if (std::strncmp(command, "!ziostart", 9) == 0) {
    if (audio_recorder) {
      audio_recorder->start_recording();
      ts3Functions.printMessageToCurrentTab("Recording started!");
    }
  } else if (std::strncmp(command, "!ziostop", 8) == 0) {
    if (audio_recorder) {
      audio_recorder->stop_recording();
      ts3Functions.printMessageToCurrentTab("Recording stopped!");
    }
  } else if (std::strncmp(command, "!ziostatus", 10) == 0) {
    if (audio_recorder) {
      char msg[256];
      snprintf(msg, sizeof(msg), "Recording status: %s, Buffer size: %zu ms",
               audio_recorder->is_recording() ? "ON" : "OFF",
               audio_recorder->get_buffer_size_ms());
      ts3Functions.printMessageToCurrentTab(msg);
    }
  }
}
