// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/log/logging.h"

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <ostream>
#include <string>
#include <utility>

#if !defined(_WIN32)
#include <unistd.h>
#endif

namespace lynx {
namespace base {
namespace logging {
namespace {

const char* const kLogSeverityNames[LOG_NUM_SEVERITIES] = {
    "VERBOSE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL"};

#ifdef NDEBUG
int32_t g_min_log_level = LOG_INFO;
#else
int32_t g_min_log_level = LOG_DEBUG;
#endif

static bool isLogOutputByPlatform = false;
static bool kHasInitedLynxLog = false;
static bool kHasInitedLynxLogWriteFunction = false;
// TODO(tangyongjie): Use NDEBUG macro determine kIsPrintAllLogToAllChannels
// value instead of external input.
static bool kIsPrintAllLogToAllChannels = false;

static PlatformLogCallBack kPlatformLogFunc = nullptr;
static InitAlogCallBack kInitAlogCallBack = nullptr;

// init alog write function
bool InitAlogNative() {
  if (!kHasInitedLynxLogWriteFunction) {
    alog_write_func_ptr alog_write_func =
        kInitAlogCallBack ? kInitAlogCallBack() : nullptr;
    if (lynx::base::InitAlog(alog_write_func)) {
      kHasInitedLynxLogWriteFunction = true;
    }
  }
  return kHasInitedLynxLogWriteFunction;
}

void PrintLogMessageByAlog(int level, const char* tag, const char* message) {
  if (InitAlogNative()) {
    if (level < ALOG_LEVEL_VERBOSE || level > ALOG_LEVEL_FATAL) {
      return;
    }
#if defined(OS_ANDROID)
    // for logging FATAL level on android platform
    if (level == ALOG_LEVEL_FATAL) {
      ALogWrite(ALOG_LEVEL_ERROR, tag, message);
      return;
    }
#endif
    // use ALog to print native logMessage
    ALogWrite(level, tag, message);
  }
}

void PrintLogMessageByPlatformLog(LogMessage* msg, const char* tag) {
  if (kPlatformLogFunc) {
    kPlatformLogFunc(msg, tag);
  }
}

bool IsLogOutputByPlatform() { return isLogOutputByPlatform; }

void Log(LogMessage* msg) {
  constexpr const char* kTag = "lynx";
  constexpr const char* kConsoleTag = "lynx-console";
  const char* tag = (msg->source() == logging::LOG_SOURCE_JS_EXT ||
                     msg->source() == logging::LOG_SOURCE_JS)
                        ? kConsoleTag
                        : kTag;
  // 0. all logs are consumed at the platform layer.
  if (IsLogOutputByPlatform()) {
    PrintLogMessageByPlatformLog(msg, tag);
    return;
  }
  // 1. all logs are logged to the delegate and ALog for debug.
  if (kIsPrintAllLogToAllChannels) {
    PrintLogMessageByAlog(msg->severity(), tag, msg->stream().str().c_str());
    PrintLogMessageByPlatformLog(msg, tag);
    return;
  }
  // 2. only native logs output to ALog for release.
  if (msg->source() == logging::LOG_SOURCE_NATIVE) {
    PrintLogMessageByAlog(msg->severity(), kTag, msg->stream().str().c_str());
    return;
  }
  // 3. console.alog output to Alog and console.report output to delegate for
  // release.
  if (msg->source() == logging::LOG_SOURCE_JS_EXT) {
    if (msg->severity() == logging::LOG_INFO) {
      // console.alog output to Alog
      PrintLogMessageByAlog(logging::LOG_ERROR, kConsoleTag,
                            msg->stream().str().c_str());
    } else {
      // console.report output to delegate
      PrintLogMessageByPlatformLog(msg, kConsoleTag);
    }
  }
}

const char* LogSeverityName(int32_t severity) {
  if (severity >= 0 && severity < LOG_NUM_SEVERITIES)
    return kLogSeverityNames[severity];
  return "UNKNOWN";
}

}  // namespace

[[maybe_unused]] bool HasInitedLynxLogWriteFunction() {
  return kHasInitedLynxLogWriteFunction;
}

void InitLynxLogging(InitAlogCallBack initAlogCallback,
                     PlatformLogCallBack PlatformLogCallBack,
                     bool isPrintAllLogToAllChannels) {
  if (initAlogCallback || PlatformLogCallBack) {
    kInitAlogCallBack = initAlogCallback;
    kPlatformLogFunc = PlatformLogCallBack;
    kIsPrintAllLogToAllChannels = isPrintAllLogToAllChannels;
    kHasInitedLynxLog = true;
  }
}

void SetMinLogLevel(int level) {
  if (g_min_log_level >= level) {
    return;
  }
  g_min_log_level = std::min(LOG_FATAL, level);
}

int GetMinLogLevel() { return g_min_log_level; }

void PrintLogToLynxLogging(int level, const char* tag, const char* message) {
  PrintLogMessageByAlog(level, tag, message);
}

[[maybe_unused]] void EnableLogOutputByPlatform() {
  isLogOutputByPlatform = true;
}

LogMessage::LogMessage(const char* file, int line, LogSeverity severity,
                       LogSource source, int64_t rt_id, LogChannel channel_type)
    : severity_(severity),
      file_(file),
      line_(line),
      source_(source),
      runtime_id_(rt_id),
      channel_type_(channel_type) {
  // FIXME(shouqun): Suppress unused warning.
  (void)line_;
  (void)file_;
  Init(file, line);
}

LogMessage::LogMessage(const char* file, int line, LogSeverity severity)
    : severity_(severity),
      file_(file),
      line_(line),
      source_(LOG_SOURCE_NATIVE),
      runtime_id_(-1),
      channel_type_(LOG_CHANNEL_LYNX_INTERNAL) {
  Init(file, line);
}

LogMessage::~LogMessage() {
  stream_ << std::endl;

  if (kHasInitedLynxLog) {
    Log(this);
  } else {
    std::string str_newline(stream_.str());
    printf("lynx/%s [%s:%d]: %s\n", LogSeverityName(severity_), file_, line_,
           str_newline.c_str());
  }

  if (severity_ == LOG_FATAL) {
    abort();
  }
}

// writes the common header info to the stream
void LogMessage::Init(const char* file, int line) {
  std::string filename(file);

  stream_ << '[';
  thread_local std::thread::id tid = std::this_thread::get_id();
  stream_ << tid << ':';

  // function localtime_r will call getenv which system function, but getenv is
  // not thread-safe although, it does not make crash directly, but when setenv
  // will be called in other thread it possibly will crash in lynx log. Then,
  // the time tag is useless, and other user, like ALog, has owner time tag so
  // disable it here.
  //  time_t t = time(NULL);
  // #if defined(OS_WIN)
  //  struct tm local_time = {0};
  //
  //  localtime_s(&local_time, &t);
  // #else
  //  struct tm local_time = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, nullptr};
  //  localtime_r(&t, &local_time);
  // #endif
  //
  //  struct tm* tm_time = &local_time;
  //  stream_ << std::setfill('0') << std::setw(2) << 1 + tm_time->tm_mon
  //          << std::setw(2) << tm_time->tm_mday << '/' << std::setw(2)
  //          << tm_time->tm_hour << std::setw(2) << tm_time->tm_min <<
  //          std::setw(2)
  //          << tm_time->tm_sec << ':';

  stream_ << LogSeverityName(severity_);
  stream_ << ":" << filename << "(" << line << ")] ";

  message_start_ = stream_.str().length();
}

}  // namespace logging
}  // namespace base
}  // namespace lynx
