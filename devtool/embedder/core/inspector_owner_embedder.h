// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_EMBEDDER_CORE_INSPECTOR_OWNER_EMBEDDER_H_
#define DEVTOOL_EMBEDDER_CORE_INSPECTOR_OWNER_EMBEDDER_H_

#include <memory>
#include <string>
#include <vector>

#include "core/public/devtool/lynx_devtool_proxy.h"
#include "core/public/devtool/lynx_inspector_owner.h"
#include "devtool/embedder/core/devtoolng_delegate_embedder.h"

namespace debugrouter {
class DebugRouterSlot;
}  // namespace debugrouter

namespace lynx {

namespace tasm {
class TemplateData;
}  // namespace tasm

namespace devtool {
class MessageHandler;
class DevtoolPlatformEmbedder;
class DevToolNGDelegateEmbedder;
class ScreenCastHelperEmbedder;
struct ScreenMetadata;

class InspectorOwnerEmbedder
    : public std::enable_shared_from_this<InspectorOwnerEmbedder>,
      public devtool::LynxInspectorOwner {
 public:
  InspectorOwnerEmbedder();
  ~InspectorOwnerEmbedder() override;

  void SetUITaskRunner(
      const fml::RefPtr<fml::TaskRunner>& task_runner) override {
    ui_task_runner_ = task_runner;
  }
  const fml::RefPtr<fml::TaskRunner>& GetUITaskRunner() {
    return ui_task_runner_;
  }

  void Init(devtool::LynxDevToolProxy* proxy,
            const std::shared_ptr<LynxInspectorOwner>& shared_self) override;
  void InitDevToolNGDelegate();
  void DispatchDocumentUpdated();
  void DispatchScreencastVisibilityChanged(bool status);

  void Reload(bool ignore_cache);

  void SetConnectionID(int32_t connection_id);

  void AttachDebugBridge(const std::string& url);

  void SendResponse(const std::string& response);
  void DispatchConsoleMessage(const std::string& message, int32_t level,
                              int64_t time_stamp);
  void StopCasting();
  void ContinueCasting();
  void PauseCasting();
  void SubscribeMessage(const std::string& type,
                        const std::shared_ptr<MessageHandler>& handler);
  void UnsubscribeMessage(const std::string& type);

  std::shared_ptr<tasm::TemplateData> getTemplateDate();

  std::string GetTemplateUrl();

  int32_t GetSessionId();

  // LynxInspectorOwner
  void OnTemplateAssemblerCreated(intptr_t prt) override;
  std::shared_ptr<lynx::runtime::js::InspectorRuntimeObserverNG>
  OnBackgroundRuntimeCreated(const std::string& group_thread_name) override;
  void OnLoaded(const std::string& url) override;
  void OnLoadTemplate(const std::string& url, const std::vector<uint8_t>& tem,
                      const std::shared_ptr<tasm::TemplateData>& data) override;
  void OnShow() override;
  void OnHide() override;
  void InvokeCDPFromSDK(
      const std::string& cdp_msg,
      std::function<void(const std::string&)>&& callback) override;

  void FlushConsoleMessages();
  void GetConsoleObject(const std::string& object_id, bool need_stringify,
                        int callback_id);
  virtual void OnConsoleMessage(const std::string& message) {}
  virtual void OnConsoleObject(const std::string& detail, int callback_id) {}

 private:
  void InitRecord();

  int32_t connection_id_;
  int64_t record_id_;
  devtool::LynxDevToolProxy* embedder_proxy_;

  std::shared_ptr<DevToolNGDelegateEmbedder> devtoolng_delegate_;

  std::shared_ptr<DevtoolPlatformEmbedder> platform_embedder_;

  std::weak_ptr<InspectorOwnerEmbedder> weak_self_;

  fml::RefPtr<fml::TaskRunner> ui_task_runner_ = nullptr;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_EMBEDDER_CORE_INSPECTOR_OWNER_EMBEDDER_H_
