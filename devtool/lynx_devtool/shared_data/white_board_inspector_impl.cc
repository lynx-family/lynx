// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/shared_data/white_board_inspector_impl.h"

#include "base/include/log/logging.h"
#include "core/runtime/lepus_context/json_parser.h"
#include "core/shared_data/lynx_white_board.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"
#include "devtool/lynx_devtool/shared_data/white_board_inspector_delegate.h"

namespace lynx {
namespace devtool {

#define NOTIFY_DELEGATES(method, ...)                           \
  for (auto it = delegates_.begin(); it != delegates_.end();) { \
    auto sp = it->second.lock();                                \
    if (sp == nullptr) {                                        \
      it = delegates_.erase(it);                                \
    } else {                                                    \
      if (sp->IsEnabled()) {                                    \
        sp->method(__VA_ARGS__);                                \
      }                                                         \
      it++;                                                     \
    }                                                           \
  }

void WhiteBoardInspectorImpl::InsertDelegate(
    const std::shared_ptr<WhiteBoardInspectorDelegate>& delegate, int view_id) {
  delegates_[view_id] = delegate;
}

void WhiteBoardInspectorImpl::RemoveDelegate(int view_id) {
  delegates_.erase(view_id);
}

void WhiteBoardInspectorImpl::SetSharedData(const std::string& key,
                                            const std::string& value,
                                            int& error_code,
                                            std::string& error_message) {
  auto sp = white_board_.lock();
  if (sp == nullptr) {
    error_code = CDPErrorCode::kServerError;
    error_message = "Failed to set shared data!";
    return;
  }
  rapidjson::Document document;
  if (document.Parse(value).HasParseError()) {
    error_code = CDPErrorCode::kInvalidParams;
    error_message = "The value must be a valid JSON string!";
    return;
  }
  lepus::Value lepus_value = lepus::jsonValueTolepusValue(document);
  auto data = std::make_shared<pub::ValueImplLepus>(lepus_value);
  sp->SetGlobalSharedData(key, data);
}

void WhiteBoardInspectorImpl::GetSharedData(
    std::vector<std::pair<std::string, std::string>>& shared_data,
    int& error_code, std::string& error_message) {
  auto sp = white_board_.lock();
  if (sp == nullptr) {
    error_code = CDPErrorCode::kServerError;
    error_message = "Failed to get shared data!";
    return;
  }
  const auto& data = sp->GetAllGlobalSharedData();
  for (const auto& item : data) {
    auto lepus_value =
        pub::ValueUtils::ConvertValueToLepusValue(*(item.second));
    auto str_value = lepus::lepusValueToString(lepus_value, false, true);
    shared_data.emplace_back(std::make_pair(item.first, str_value));
  }
}

void WhiteBoardInspectorImpl::RemoveSharedData(const std::string& key,
                                               int& error_code,
                                               std::string& error_message) {
  auto sp = white_board_.lock();
  if (sp == nullptr) {
    error_code = CDPErrorCode::kServerError;
    error_message = "Failed to remove shared data!";
    return;
  }
  if (sp->GetGlobalSharedData(key) == nullptr) {
    error_code = CDPErrorCode::kInvalidParams;
    error_message = "The key does not exist!";
    return;
  }
  sp->RemoveGlobalSharedData(key);
}

void WhiteBoardInspectorImpl::ClearSharedData(int& error_code,
                                              std::string& error_message) {
  auto sp = white_board_.lock();
  if (sp == nullptr) {
    error_code = CDPErrorCode::kServerError;
    error_message = "Failed to clear shared data!";
    return;
  }
  sp->ClearGlobalSharedData();
}

void WhiteBoardInspectorImpl::OnSharedDataAdded(const std::string& key,
                                                const pub::Value& value) {
  auto lepus_value = pub::ValueUtils::ConvertValueToLepusValue(value);
  auto str_value = lepus::lepusValueToString(lepus_value);
  NOTIFY_DELEGATES(OnSharedDataAdded, key, str_value);
}

void WhiteBoardInspectorImpl::OnSharedDataUpdated(const std::string& key,
                                                  const pub::Value& value) {
  auto lepus_value = pub::ValueUtils::ConvertValueToLepusValue(value);
  auto str_value = lepus::lepusValueToString(lepus_value);
  NOTIFY_DELEGATES(OnSharedDataUpdated, key, str_value);
}

void WhiteBoardInspectorImpl::OnSharedDataRemoved(const std::string& key) {
  NOTIFY_DELEGATES(OnSharedDataRemoved, key);
}

void WhiteBoardInspectorImpl::OnSharedDataCleared() {
  NOTIFY_DELEGATES(OnSharedDataCleared);
}

}  // namespace devtool
}  // namespace lynx
