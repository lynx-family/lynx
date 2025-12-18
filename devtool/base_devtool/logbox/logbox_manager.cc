// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/base_devtool/logbox/logbox_manager.h"

#include <algorithm>
#include <cstdint>
#include <regex>
#include <utility>

#include "base/include/debug/lynx_error.h"
#include "devtool/base_devtool/logbox/logbox_dialog_base.h"
#include "devtool/base_devtool/logbox/logbox_notification.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace devtool {
void LogBoxManager::OnNewLog(LogBoxLogMsg&& log,
                             const std::shared_ptr<LogBoxProxy>& proxy) {
  CHECK_NULL_RETURN(proxy);
  bool is_new_proxy = false;
  int32_t level = log.error_level_;
  auto target = std::find(proxys_.begin(), proxys_.end(), proxy);
  if (target == proxys_.end()) {
    target = proxys_.emplace(proxys_.end(), proxy);
    is_new_proxy = true;
  }
  current_proxy_[level] = proxy;
  if (!notification_) {
    InitNotification(proxy);
  }
  std::string brief_message = ExtractBriefMessage(log.error_message_);
  last_messages_[level] = brief_message;
  log_counts_[level]++;
  notification_->UpdateInfo(level, brief_message, log_counts_[level]);

  if (dialog_ && dialog_->IsShowing() &&
      (level == static_cast<int32_t>(dialog_->GetLevel()))) {
    if (current_proxy_[level].lock() == proxy) {
      dialog_->ShowErrorMessage(log);
    } else if (is_new_proxy) {
      auto provider = proxy->GetProvider();
      CHECK_NULL_RETURN(provider);
      dialog_->UpdateViewInfo(std::distance(proxys_.begin(), target),
                              proxys_.size(), log.error_level_,
                              proxy->GetProvider()->GetTemplateUrl());
    }
  } else {
    notification_->Show();
  }
}

// Prepare resources and logs of current view
// initializes the dialog if needed
void LogBoxManager::RequestLogsOfCurrentView(int32_t level) {
  if (!dialog_) {
    return;
  }
  auto proxy = current_proxy_[level].lock();
  if (!proxy) {
    return;
  }
  int32_t index = GetIndexOfProxy(proxy);
  auto provider = proxy->GetProvider();
  if (current_index_ != index && provider) {
    dialog_->UpdateViewInfo(index, proxys_.size(), level,
                            provider->GetTemplateUrl());
    dialog_->SetJsSources(provider->GetAllJsSource());
    current_index_ = index;
  }
  dialog_->ShowErrorMessages(proxy->GetLogs());
}

int32_t LogBoxManager::GetIndexOfProxy(
    const std::shared_ptr<LogBoxProxy>& proxy) {
  auto it = std::find(proxys_.begin(), proxys_.end(), proxy);
  if (it != proxys_.end()) {
    return std::distance(proxys_.begin(), it);
  }
  return -1;
}

void LogBoxManager::OnNotificationClick(int32_t level) {
  notification_->HideNotification(level);
  if (!dialog_) {
    dialog_ = LogBoxDialogBase::Create(shared_from_this());
    dialog_->Init(
        [level, weak_self = weak_from_this()]() {
          if (auto sp = weak_self.lock()) {
            sp->RequestLogsOfCurrentView(level);
          }
        },
        native_window_);
  } else {
    RequestLogsOfCurrentView(level);
    dialog_->Show();
  }
}

void LogBoxManager::OnNotificationClose(int32_t level) {
  if (notification_) {
    notification_->HideNotification(level);
  }
}

void LogBoxManager::OnRemoveBtnClick(int32_t level) {
  auto iter = proxys_.begin();
  while (current_index_ > 0 && iter != proxys_.end()) {
    ++iter;
    --current_index_;
  }
  current_index_ = -1;  // current proxy is either non-exists or be cleared
  if (iter == proxys_.end()) return;

  log_counts_[level] -= (*iter)->ClearLogs(level);
  current_proxy_[level].reset();
  last_messages_[level].clear();
}

std::string LogBoxManager::ExtractBriefMessage(std::string log_str) {
  if (log_str.empty()) {
    return "";
  }

  rapidjson::Document error_doc;
  error_doc.Parse(log_str.c_str());
  if (!error_doc.HasParseError() && error_doc.IsObject() &&
      error_doc.HasMember("rawError") && error_doc["rawError"].IsObject()) {
    const auto& raw_error = error_doc["rawError"];
    if (raw_error.HasMember("message") && raw_error["message"].IsString()) {
      log_str = raw_error["message"].GetString();
    }
  }
  // Replace newlines with spaces
  std::replace(log_str.begin(), log_str.end(), '\n', ' ');
  return log_str;
}

void LogBoxManager::OnNativeWindowChange() {
  if (notification_) {
    notification_->OnNativeWindowChange();
  }
}

void LogBoxManager::InitNotification(
    const std::shared_ptr<LogBoxProxy>& proxy) {
  auto provider = proxy->GetProvider();
  if (provider) {
    notification_ = LogBoxNotification::Create(weak_from_this());
    notification_->SetNativeWindow(provider->GetNativeWindow());
  }
}
}  // namespace devtool
}  // namespace lynx
