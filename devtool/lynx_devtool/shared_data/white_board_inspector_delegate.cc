// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/shared_data/white_board_inspector_delegate.h"

#include "base/include/log/logging.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"

namespace lynx {
namespace devtool {

WhiteBoardInspectorDelegate::WhiteBoardInspectorDelegate(int view_id)
    : view_id_(view_id) {}

WhiteBoardInspectorDelegate::~WhiteBoardInspectorDelegate() {
  auto sp = inspector_.lock();
  if (sp != nullptr) {
    sp->RemoveDelegate(view_id_);
  }
}

void WhiteBoardInspectorDelegate::Enable(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value&) {
  enabled_ = true;
  responder->SendSuccess();
}

void WhiteBoardInspectorDelegate::Disable(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value&) {
  enabled_ = false;
  responder->SendSuccess();
}

void WhiteBoardInspectorDelegate::SetSharedData(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  if (!enabled_) {
    responder->SendError(CDPErrorCode::ServerError,
                         "WhiteBoard is not enabled");
    return;
  }
  auto sp = inspector_.lock();
  if (sp == nullptr) {
    responder->SendError(CDPErrorCode::ServerError,
                         "WhiteBoard inspector is unavailable");
    return;
  }
  std::string key = params["key"].asString();
  std::string value = params["value"].asString();
  std::string error_msg;
  auto error_code = sp->SetSharedData(key, value, error_msg);

  if (error_code.has_value()) {
    responder->SendError(*error_code, error_msg);
    return;
  }

  responder->SendSuccess();
}

std::string WhiteBoardInspectorDelegate::GetSharedData(
    const Json::Value& message) {
  if (!enabled_) {
    return "";
  }
  auto sp = inspector_.lock();
  CHECK_NULL_AND_LOG_RETURN_VALUE(
      sp, "WhiteBoardInspectorDelegate::GetSharedData, inspector_ is null", "");
  int message_id = message["id"].asInt64();
  std::vector<std::pair<std::string, std::string>> data;
  int error_code = 0;
  std::string error_msg;
  sp->GetSharedData(data, error_code, error_msg);

  if (error_code != 0) {
    return GenErrorMessage(message_id, error_code, error_msg);
  }

  Json::Value content(Json::ValueType::objectValue);
  Json::Value entries(Json::ValueType::arrayValue);
  for (const auto& item : data) {
    Json::Value entry(Json::ValueType::objectValue);
    entry["key"] = item.first;
    entry["value"] = item.second;
    entries.append(entry);
  }
  content["entries"] = entries;
  return GenResponseMessage(message_id, content);
}

std::string WhiteBoardInspectorDelegate::RemoveSharedData(
    const Json::Value& message) {
  if (!enabled_) {
    return "";
  }
  auto sp = inspector_.lock();
  CHECK_NULL_AND_LOG_RETURN_VALUE(
      sp, "WhiteBoardInspectorDelegate::RemoveSharedData, inspector_ is null",
      "");
  Json::Value params = message["params"];
  int message_id = message["id"].asInt64();
  std::string key = params["key"].asString();
  int error_code = 0;
  std::string error_msg;
  sp->RemoveSharedData(key, error_code, error_msg);

  if (error_code != 0) {
    return GenErrorMessage(message_id, error_code, error_msg);
  }

  return GenResponseMessage(message_id,
                            Json::Value(Json::ValueType::objectValue));
}

std::string WhiteBoardInspectorDelegate::Clear(const Json::Value& message) {
  if (!enabled_) {
    return "";
  }
  auto sp = inspector_.lock();
  CHECK_NULL_AND_LOG_RETURN_VALUE(
      sp, "WhiteBoardInspectorDelegate::Clear, inspector_ is null", "");
  int message_id = message["id"].asInt64();
  int error_code = 0;
  std::string error_msg;
  sp->ClearSharedData(error_code, error_msg);

  if (error_code != 0) {
    return GenErrorMessage(message_id, error_code, error_msg);
  }

  return GenResponseMessage(message_id,
                            Json::Value(Json::ValueType::objectValue));
}

void WhiteBoardInspectorDelegate::OnSharedDataAdded(const std::string& key,
                                                    const std::string& value) {
  Json::Value params(Json::ValueType::objectValue);
  params["key"] = key;
  params["value"] = value;
  SendEvent(GenEventMessage("WhiteBoard.onSharedDataAdded", params));
}

void WhiteBoardInspectorDelegate::OnSharedDataUpdated(
    const std::string& key, const std::string& value) {
  Json::Value params(Json::ValueType::objectValue);
  params["key"] = key;
  params["newValue"] = value;
  SendEvent(GenEventMessage("WhiteBoard.onSharedDataUpdated", params));
}

void WhiteBoardInspectorDelegate::OnSharedDataRemoved(const std::string& key) {
  Json::Value params(Json::ValueType::objectValue);
  params["key"] = key;
  SendEvent(GenEventMessage("WhiteBoard.onSharedDataRemoved", params));
}

void WhiteBoardInspectorDelegate::OnSharedDataCleared() {
  Json::Value msg(Json::ValueType::objectValue);
  msg["method"] = "WhiteBoard.onSharedDataCleared";
  SendEvent(msg);
}

std::string WhiteBoardInspectorDelegate::GenResponseMessage(
    int message_id, const Json::Value& result) {
  Json::Value response(Json::ValueType::objectValue);
  response["id"] = message_id;
  response["result"] = result;
  return response.toStyledString();
}

Json::Value WhiteBoardInspectorDelegate::GenEventMessage(
    const std::string& method, const Json::Value& params) {
  Json::Value msg(Json::ValueType::objectValue);
  msg["method"] = method;
  msg["params"] = params;
  return msg;
}

std::string WhiteBoardInspectorDelegate::GenErrorMessage(
    int message_id, int code, const std::string& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value error(Json::ValueType::objectValue);
  error["code"] = code;
  error["message"] = message;
  response["id"] = message_id;
  response["error"] = error;
  return response.toStyledString();
}

}  // namespace devtool
}  // namespace lynx
