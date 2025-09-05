// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/log/logging_base.h"

#include <iostream>
#include <map>
#include <string>

#include "base/include/log/logging.h"

namespace lynx {

void PrintLogMessageForDebug(int level, const char* message);
bool IsExternalChannel(lynx::base::logging::LogChannel channel_type);

namespace base {
namespace logging {

void PrintLogMessageByDelegate(LogMessage* msg, const char* tag) {
  int level = msg->severity();
  std::string message = msg->stream().str();
  // print native's log to hybrid devtool for debug
  PrintLogMessageForDebug(level, message.c_str());

  std::vector<LynxLogDelegate*> delegates = GetLoggingDelegates();
  for (LynxLogDelegate* delegate : delegates) {
    LynxLogFunction log_function = delegate->log_function;
    if (log_function == nullptr || level < delegate->min_log_level ||
        (delegate->accept_runtime_id >= 0 &&
         delegate->accept_runtime_id != msg->runtimeId())) {
      continue;
    }

    // only upload external JS logs and console.report to logging delegate
    switch (msg->source()) {
      case LOG_SOURCE_JS:
        if (IsExternalChannel(msg->ChannelType()) &&
            (delegate->accept_source & LynxLogSourceJS)) {
          log_function(level, message.c_str());
        }
        break;
      case LOG_SOURCE_JS_EXT:
        log_function(level, message.c_str());
        break;
      case LOG_SOURCE_NATIVE:
        // output the native log of lynx when alog is not supported on the
        // PC（Windows & Mac)
        log_function(level, message.c_str());
        break;
      default:
        break;
    }
  }
}
}  // namespace logging
}  // namespace base

void InitLynxBaseLog(bool print_logs_to_all_channels) {
  base::logging::InitLynxLogging(nullptr,
                                 base::logging::PrintLogMessageByDelegate,
                                 print_logs_to_all_channels);
}

static LynxLogDelegate* lynx_log_debug_delegate_ = nullptr;
static std::map<int, LynxLogDelegate*> lynx_log_delegates_;

static int lynx_default_delegate_id_ = -1;
static int lynx_current_id_ = 0;
#ifdef DEBUG
static int lynx_alog_min_level_ = lynx::base::logging::LOG_DEBUG;
#else
static int lynx_alog_min_level_ = lynx::base::logging::LOG_INFO;
#endif
static bool is_jS_logs_from_external_channels_open_ = false;

bool IsExternalChannel(lynx::base::logging::LogChannel channel_type) {
  return is_jS_logs_from_external_channels_open_ &&
         channel_type == lynx::base::logging::LOG_CHANNEL_LYNX_EXTERNAL;
}

void PrintLogMessageForDebug(int level, const char* message) {
  if (lynx_log_debug_delegate_ == nullptr ||
      level < lynx_log_debug_delegate_->min_log_level) {
    return;
  }
  LynxLogFunction log_function = lynx_log_debug_delegate_->log_function;
  if (log_function) {
    log_function(level, message);
  }
}

void SetDebugLoggingDelegate(LynxLogDelegate* delegate) {
  lynx_log_debug_delegate_ = delegate;
}

int AddLoggingDelegate(LynxLogDelegate* delegate) {
  int delegate_id = ++lynx_current_id_;
  lynx_log_delegates_[delegate_id] = delegate;
  return delegate_id;
}

LynxLogDelegate* GetLoggingDelegate(int delegate_id) {
  if (lynx_log_delegates_.find(delegate_id) != lynx_log_delegates_.end()) {
    return lynx_log_delegates_[delegate_id];
  }
  return nullptr;
}

std::vector<LynxLogDelegate*> GetLoggingDelegates() {
  std::vector<LynxLogDelegate*> result;
  for (auto& delegate : lynx_log_delegates_) {
    result.push_back(delegate.second);
  }
  return result;
}

void RemoveLoggingDelegate(int delegate_id) {
  if (lynx_log_delegates_.find(delegate_id) != lynx_log_delegates_.end()) {
    delete lynx_log_delegates_[delegate_id];
    lynx_log_delegates_.erase(delegate_id);
  }
}

void SetMinimumLoggingLevel(int min_log_level) {
  if (lynx_alog_min_level_ < min_log_level) {
    lynx_alog_min_level_ = min_log_level;
    lynx::base::logging::SetMinLogLevel(min_log_level);
  }
}

void SetJSLogsFromExternalChannels(bool is_open) {
  is_jS_logs_from_external_channels_open_ = is_open;
}

int GetMinimumLoggingLevel() { return lynx_alog_min_level_; }

int LynxSetLogFunction(LynxLogFunction log_function) {
  LynxLogDelegate* delegate = new LynxLogDelegate();
  delegate->log_function = log_function;
  lynx_default_delegate_id_ = AddLoggingDelegate(delegate);
  return lynx_default_delegate_id_;
}

void DefaultLogFunction(int level, const char* message) {
  std::cerr << message;
}

LynxLogFunction LynxGetLogFunction() {
  LynxLogDelegate* delegate = GetLoggingDelegate(lynx_default_delegate_id_);
  if (!delegate) {
    return DefaultLogFunction;
  }
  return delegate->log_function;
}

}  // namespace lynx
