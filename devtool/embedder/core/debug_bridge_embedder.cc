// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/embedder/core/debug_bridge_embedder.h"

#include <utility>

#include "base/include/log/logging.h"
#include "devtool/embedder/core/env_embedder.h"
#include "devtool/lynx_devtool/config/devtool_config.h"
#include "devtool/lynx_devtool/js_debug/inspector_const_extend.h"
#include "third_party/debug_router/src/debug_router/common/debug_router.h"
#include "third_party/jsoncpp/include/json/json.h"

namespace lynx {
namespace devtool {

using DebugRouter = debugrouter::common::DebugRouter;

DebugBridgeEmbedder& DebugBridgeEmbedder::GetInstance() {
  static lynx::base::NoDestructor<DebugBridgeEmbedder> instance;
  return *instance;
}

DebugBridgeEmbedder::DebugBridgeEmbedder()
    : open_card_callback_(nullptr), close_card_callback_(nullptr) {
  debug_state_listener_ = std::make_shared<DebugStateListenerEmbedder>();
  agent_dispatcher_ = std::make_shared<DevtoolAgentDispatcher>();
  Init();
}

void DebugBridgeEmbedder::Init() {
  agent_dispatcher_->InitDevToolNGDelegate();
  DebugRouter::GetInstance().AddGlobalHandler(this);
  DebugRouter::GetInstance().AddStateListener(debug_state_listener_);
}

bool DebugBridgeEmbedder::Enable(
    const std::string& url,
    const std::unordered_map<std::string, std::string>& app_infos) {
  if (IsEnabled()) {
    LOGW("Devtools already connected!");
    return true;
  }

  if (DebugRouter::GetInstance().IsValidSchema(url)) {
    SetAppInfo(app_infos);
    return DebugRouter::GetInstance().HandleSchema(url);
  }
  return false;
}

// only be used by automatic test
void DebugBridgeEmbedder::EnableDebugging(const std::string& schema) {
  DebugRouter::GetInstance().HandleSchema(schema);
}

bool DebugBridgeEmbedder::IsEnabled() {
  return DebugRouter::GetInstance().IsConnected();
}

void DebugBridgeEmbedder::SetOpenCardCallback(
    DevtoolsOpenCardCallback callback) {
  std::lock_guard<std::mutex> lock(callback_mutex_);
  open_card_callback_ = std::move(callback);
}

void DebugBridgeEmbedder::SetCloseCardCallback(
    DevtoolsCloseCardCallback callback) {
  std::lock_guard<std::mutex> lock(callback_mutex_);
  close_card_callback_ = std::move(callback);
}

void DebugBridgeEmbedder::OpenCard(const std::string& url) {
  DevtoolsOpenCardCallback callback;
  {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    callback = open_card_callback_;
  }
  if (callback) {
    callback(url);
  }
}

void DebugBridgeEmbedder::OnMessage(const std::string& message,
                                    const std::string& type) {
  if (!type.compare("CloseCard")) {
    DevtoolsCloseCardCallback callback;
    {
      std::lock_guard<std::mutex> lock(callback_mutex_);
      callback = close_card_callback_;
    }
    if (callback) {
      callback();
    }
    return;
  }

  Json::Value root;
  Json::Reader reader;
  if (!reader.parse(message, root, false)) {
    return;
  }
  if (!type.compare("SetGlobalSwitch")) {
    bool valid = root["global_key"].isString() && root["global_value"].isBool();
    if (!valid) {
      return;
    }

    std::string key = root["global_key"].asString();
    bool value = root["global_value"].asBool();

    EnvEmbedder::SetSwitch(key, value);

    DebugRouter::GetInstance().SendDataAsync(message, "SetGlobalSwitch", -1);
  } else if (!type.compare("GetGlobalSwitch")) {
    bool valid = root["global_key"].isString();
    if (!valid) {
      return;
    }
    std::string key = root["global_key"].asString();
    bool value = EnvEmbedder::GetSwitch(key);
    DebugRouter::GetInstance().SendDataAsync(value ? "true" : "false",
                                             "GetGlobalSwitch", -1);
  }
}

void DebugBridgeEmbedder::SetAppInfo(
    const std::unordered_map<std::string, std::string>& app_infos) {
  for (auto it = app_infos.begin(); it != app_infos.end(); it++) {
    DebugRouter::GetInstance().SetAppInfo(it->first, it->second);
  }
}

}  // namespace devtool
}  // namespace lynx
