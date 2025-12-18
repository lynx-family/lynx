// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/base_devtool/logbox/logbox_base.h"

#include <utility>

#include "devtool/base_devtool/logbox/logbox_manager.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"

namespace lynx {
namespace devtool {
void LogBoxProxy::ShowErrorMessage(lynx::base::LynxError&& log) {
  auto sp_provider = provider_.lock();
  if (!sp_provider) {
    return;
  }
  auto owner = LogBoxOwner::GetInstance();
  CHECK_NULL_RETURN(owner);

  LogBoxLogMsg msg(log);
  logs_.emplace_back(std::move(log));
  owner->Dispatch(msg, sp_provider->GetNativeWindow(), shared_from_this());
}

std::list<LogBoxLogMsg> LogBoxProxy::GetLogs(int32_t level) {
  std::list<LogBoxLogMsg> filtered_logs;
  for (const auto& log : logs_) {
    // there is 1 gap between errors in LynxError and LogBoxLogMsg
    if (level < 0 || log.error_level_ == LogBoxLevel2LynxErrorLevel(level)) {
      filtered_logs.emplace_back(log);
    }
  }
  return filtered_logs;
}

int32_t LogBoxProxy::ClearLogs(int32_t level) {
  int32_t cnt = 0;
  for (auto iter = logs_.begin(); iter != logs_.end();) {
    // there is 1 gap between errors in LynxError and LogBoxLogMsg
    if (iter->error_level_ == LogBoxLevel2LynxErrorLevel(level)) {
      iter = logs_.erase(iter);
      ++cnt;
    } else {
      ++iter;
    }
  }
  return cnt;
}

bool LogBoxOwner::Dispatch(LogBoxLogMsg& msg, void* native_context,
                           const std::shared_ptr<LogBoxProxy>& proxy) {
  auto manager = GetOrCreateLogBoxManager(native_context);
  if (!manager) {
    return false;
  }
  manager->OnNewLog(std::move(msg), proxy);
  return true;
}

__attribute__((weak)) std::shared_ptr<LogBoxOwner> LogBoxOwner::GetInstance() {
  return nullptr;
}

int32_t LynxErrorLevel2LogBoxLevel(lynx::base::LynxErrorLevel lynxErrorlevel) {
  return static_cast<int32_t>(lynxErrorlevel) - 1;
}

lynx::base::LynxErrorLevel LogBoxLevel2LynxErrorLevel(int32_t logboxLevel) {
  return static_cast<lynx::base::LynxErrorLevel>(logboxLevel + 1);
}
}  // namespace devtool
}  // namespace lynx
