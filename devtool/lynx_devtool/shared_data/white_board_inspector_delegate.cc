// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/shared_data/white_board_inspector_delegate.h"

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

void WhiteBoardInspectorDelegate::GetSharedData(
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
  std::vector<std::pair<std::string, std::string>> data;
  std::string error_msg;
  auto error_code = sp->GetSharedData(data, error_msg);

  if (error_code.has_value()) {
    responder->SendError(*error_code, error_msg);
    return;
  }

  Json::Value result(Json::ValueType::objectValue);
  Json::Value entries(Json::ValueType::arrayValue);
  for (const auto& item : data) {
    Json::Value entry(Json::ValueType::objectValue);
    entry["key"] = item.first;
    entry["value"] = item.second;
    entries.append(entry);
  }
  result["entries"] = entries;
  responder->SendSuccess(std::move(result));
}

void WhiteBoardInspectorDelegate::RemoveSharedData(
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
  std::string error_msg;
  auto error_code = sp->RemoveSharedData(key, error_msg);

  if (error_code.has_value()) {
    responder->SendError(*error_code, error_msg);
    return;
  }

  responder->SendSuccess();
}

void WhiteBoardInspectorDelegate::Clear(
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
  std::string error_msg;
  auto error_code = sp->ClearSharedData(error_msg);

  if (error_code.has_value()) {
    responder->SendError(*error_code, error_msg);
    return;
  }

  responder->SendSuccess();
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

Json::Value WhiteBoardInspectorDelegate::GenEventMessage(
    const std::string& method, const Json::Value& params) {
  Json::Value msg(Json::ValueType::objectValue);
  msg["method"] = method;
  msg["params"] = params;
  return msg;
}

}  // namespace devtool
}  // namespace lynx
