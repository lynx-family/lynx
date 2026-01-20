// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/embedder/core/inspector_owner_embedder.h"

#include <memory>
#include <unordered_map>
#include <utility>

#include "core/renderer/utils/lynx_env.h"
#include "core/services/recorder/recorder_controller.h"
#include "devtool/embedder/core/debug_bridge_embedder.h"
#include "devtool/embedder/core/devtool_platform_embedder.h"
#include "devtool/embedder/core/invoke_cdp_from_sdk_sender_embedder.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"

namespace lynx {
namespace devtool {

InspectorOwnerEmbedder::InspectorOwnerEmbedder()
    : connection_id_(0),
      embedder_proxy_(nullptr),
      devtoolng_delegate_(nullptr),
      platform_embedder_(nullptr) {}

void InspectorOwnerEmbedder::Init(
    devtool::LynxDevToolProxy* proxy,
    const std::shared_ptr<devtool::LynxInspectorOwner>& shared_self) {
  weak_self_ = std::static_pointer_cast<InspectorOwnerEmbedder>(shared_self);
  embedder_proxy_ = proxy;
  embedder_proxy_->SetInspectorOwner(this);

  platform_embedder_ = std::make_shared<DevtoolPlatformEmbedder>();
  platform_embedder_->Init(proxy, weak_self_.lock());
  InitDevToolNGDelegate();
}

void InspectorOwnerEmbedder::InitDevToolNGDelegate() {
  devtoolng_delegate_ = std::make_shared<DevToolNGDelegateEmbedder>();
}

void InspectorOwnerEmbedder::OnLoadTemplate(
    const std::string& url, const std::vector<uint8_t>& tem,
    const std::shared_ptr<tasm::TemplateData>& init_data) {
  if (platform_embedder_) {
    platform_embedder_->OnLoadTemplate(url, tem, init_data);
  }
}

void InspectorOwnerEmbedder::OnTemplateAssemblerCreated(intptr_t ptr) {
  if (devtoolng_delegate_) {
    devtoolng_delegate_->OnTasmCreated(ptr);
    if (platform_embedder_ && platform_embedder_->GetDevtoolPlatformFacade()) {
      devtoolng_delegate_->SetDevtoolPlatformAbility(
          platform_embedder_->GetDevtoolPlatformFacade());
    }
  }
  record_id_ = ptr;
}

std::shared_ptr<lynx::runtime::js::InspectorRuntimeObserverNG>
InspectorOwnerEmbedder::OnBackgroundRuntimeCreated(
    const std::string& group_thread_name) {
  if (devtoolng_delegate_) {
    return devtoolng_delegate_->OnBackgroundRuntimeCreated(group_thread_name);
  }
  return nullptr;
}

void InspectorOwnerEmbedder::InitRecord() {
  CHECK_NULL_AND_LOG_RETURN(devtoolng_delegate_, "devtoolng_delegate_ is null");
  std::string dir_path = tasm::LynxEnv::GetInstance().GetStorageDirectory();
  // TODO(zhaosong.lmm):Use the actual device screen width and height.
  float default_screen_width = 0;
  float default_screen_height = 0;
  tasm::recorder::RecorderController::InitConfig(
      dir_path, devtoolng_delegate_->getSessionId(), default_screen_width,
      default_screen_height, record_id_);
}

void InspectorOwnerEmbedder::OnLoaded(const std::string& url) {
  if (devtoolng_delegate_->getSessionId() == 0) {
    AttachDebugBridge(url);
  }
}

void InspectorOwnerEmbedder::AttachDebugBridge(const std::string& url) {
  if (devtoolng_delegate_ && (!devtoolng_delegate_->isAttachToDebugRouter())) {
    devtoolng_delegate_->attachToDebug(url);
    InitRecord();
  }
}

void InspectorOwnerEmbedder::StopCasting() {
  CHECK_NULL_AND_LOG_RETURN(platform_embedder_, "platform_embedder_ is null");
  platform_embedder_->StopCasting();
}

void InspectorOwnerEmbedder::ContinueCasting() {
  if (platform_embedder_ && devtoolng_delegate_ &&
      devtoolng_delegate_->isAttachToDebugRouter()) {
    platform_embedder_->ContinueCasting();
  }
}

void InspectorOwnerEmbedder::PauseCasting() {
  if (platform_embedder_ && devtoolng_delegate_ &&
      devtoolng_delegate_->isAttachToDebugRouter()) {
    platform_embedder_->PauseCasting();
  }
}

void InspectorOwnerEmbedder::SetConnectionID(int32_t connection_id) {
  connection_id_ = connection_id;
}

std::string InspectorOwnerEmbedder::GetTemplateUrl() {
  CHECK_NULL_AND_LOG_RETURN_VALUE(
      platform_embedder_, "platform_embedder_ is null", "___UNKNOWN___");
  return platform_embedder_->GetTemplateUrl();
}

int32_t InspectorOwnerEmbedder::GetSessionId() {
  CHECK_NULL_AND_LOG_RETURN_VALUE(devtoolng_delegate_,
                                  "devtoolng_delegate_ is null", -1);
  return devtoolng_delegate_->getSessionId();
}

void InspectorOwnerEmbedder::SendResponse(const std::string& response) {
  CHECK_NULL_AND_LOG_RETURN(devtoolng_delegate_, "devtoolng_delegate_ is null");
  devtoolng_delegate_->sendMessageToDebugPlatform("CDP", response);
}

void InspectorOwnerEmbedder::DispatchConsoleMessage(const std::string& message,
                                                    int32_t level,
                                                    int64_t time_stamp) {
  CHECK_NULL_AND_LOG_RETURN(platform_embedder_, "platform_embedder_ is null");
  platform_embedder_->SendConsoleEvent(message, level, time_stamp);
}

void InspectorOwnerEmbedder::SubscribeMessage(
    const std::string& type, const std::shared_ptr<MessageHandler>& handler) {
  CHECK_NULL_AND_LOG_RETURN(devtoolng_delegate_, "devtoolng_delegate_ is null");
  devtoolng_delegate_->subscribeMessage(type, handler);
}

void InspectorOwnerEmbedder::UnsubscribeMessage(const std::string& type) {
  CHECK_NULL_AND_LOG_RETURN(devtoolng_delegate_, "devtoolng_delegate_ is null");
  devtoolng_delegate_->UnSubscribeMessage(type);
}

std::shared_ptr<tasm::TemplateData> InspectorOwnerEmbedder::getTemplateDate() {
  CHECK_NULL_AND_LOG_RETURN_VALUE(platform_embedder_,
                                  "platform_embedder_ is null", nullptr);
  return platform_embedder_->getTemplateDate();
}

InspectorOwnerEmbedder::~InspectorOwnerEmbedder() {
  if (devtoolng_delegate_) {
    devtoolng_delegate_->detachFromDebug();
  }
}

void InspectorOwnerEmbedder::InvokeCDPFromSDK(
    const std::string& cdp_msg,
    std::function<void(const std::string&)>&& callback) {
  if (devtoolng_delegate_) {
    devtoolng_delegate_->DispatchMessage(
        std::make_shared<InvokeCDPFromSDKSenderEmbedder>(std::move(callback)),
        "CDP", cdp_msg);
  }
}

void InspectorOwnerEmbedder::OnShow() { ContinueCasting(); }

void InspectorOwnerEmbedder::OnHide() { PauseCasting(); }

void InspectorOwnerEmbedder::FlushConsoleMessages() {
  CHECK_NULL_AND_LOG_RETURN(platform_embedder_, "platform_embedder_ is null");
  platform_embedder_->FlushConsoleMessages();
}

void InspectorOwnerEmbedder::GetConsoleObject(const std::string& object_id,
                                              bool need_stringify,
                                              int callback_id) {
  CHECK_NULL_AND_LOG_RETURN(platform_embedder_, "platform_embedder_ is null");
  platform_embedder_->GetConsoleObject(object_id, need_stringify, callback_id);
}

}  // namespace devtool
}  // namespace lynx
