// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_INSPECTOR_UI_EXECUTOR_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_INSPECTOR_UI_EXECUTOR_H_
#include <memory>
#include <unordered_map>

#include "core/shell/lynx_shell.h"
#include "devtool/base_devtool/native/public/message_sender.h"
#include "devtool/lynx_devtool/agent/agent_defines.h"
#include "devtool/lynx_devtool/agent/devtool_platform_facade.h"

namespace lynx {
namespace devtool {

class LynxDevToolMediator;

class InspectorUIExecutor
    : public std::enable_shared_from_this<InspectorUIExecutor> {
 public:
  explicit InspectorUIExecutor(
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator);
  ~InspectorUIExecutor();

  void SetDevToolPlatformFacade(
      const std::shared_ptr<DevToolPlatformFacade>& devtool_platform_facade);
  void SetShell(lynx::shell::LynxShell* shell);
  bool ShellIsDestroyed() { return shell_ == nullptr; }

  // dom domain
  DECLARE_DEVTOOL_METHOD(GetNodeForLocation)

  // page domain
  DECLARE_DEVTOOL_METHOD(StartScreencast)
  DECLARE_DEVTOOL_METHOD(StopScreencast)
  DECLARE_DEVTOOL_METHOD(ScreencastFrameAck)
  DECLARE_DEVTOOL_METHOD(PageEnable)
  DECLARE_DEVTOOL_METHOD(PageCanEmulate)
  DECLARE_DEVTOOL_METHOD(PageCanScreencast)
  DECLARE_DEVTOOL_METHOD(PageGetResourceTree)
  DECLARE_DEVTOOL_METHOD(PageReload)
  DECLARE_DEVTOOL_METHOD(PageNavigate)

  // uitree domain
  DECLARE_DEVTOOL_METHOD(UITree_Enable)
  DECLARE_DEVTOOL_METHOD(UITree_Disable)
  DECLARE_DEVTOOL_METHOD(GetLynxUITree)
  DECLARE_DEVTOOL_METHOD(GetUIInfoForNode)
  DECLARE_DEVTOOL_METHOD(SetUIStyle)

  // lynx domain
  DECLARE_DEVTOOL_METHOD(LynxGetRectToWindow)
  DECLARE_DEVTOOL_METHOD(LynxTransferData)
  DECLARE_DEVTOOL_METHOD(LynxGetViewLocationOnScreen)
  DECLARE_DEVTOOL_METHOD(LynxSendEventToVM)
  DECLARE_DEVTOOL_METHOD(GetScreenshot)
  DECLARE_DEVTOOL_METHOD(TemplateGetTemplateData)
  DECLARE_DEVTOOL_METHOD(TemplateGetTemplateJsInfo)

  // Performance domain
  DECLARE_DEVTOOL_METHOD(PerformanceEnable)
  DECLARE_DEVTOOL_METHOD(PerformanceDisable)
  DECLARE_DEVTOOL_METHOD(getAllTimingInfo)

  // Input domain
  DECLARE_DEVTOOL_METHOD(EmulateTouchFromMouseEvent)

  // event
 public:
  void SendPageScreencastFrameEvent(
      const std::string& data,
      std::shared_ptr<lynx::devtool::ScreenMetadata> metadata);
  void SendPageScreencastVisibilityChangedEvent(bool status);
  void SendPageFrameNavigatedEvent(const std::string& url);
  void SendLynxScreenshotCapturedEvent(const std::string& data);

 public:
  std::vector<double> GetBoxModel(tasm::Element* element);

  // task run on ui thread
  void ScrollIntoView(int node_id);
  void PageReload(bool ignore_cache, std::string template_binary = "",
                  bool from_template_fragments = false,
                  int32_t template_size = 0);

 public:
  void OnLayoutNodeCreated(int32_t id, tasm::LayoutNode* ptr);
  void OnLayoutNodeDestroy(int32_t id);
  void OnComponentUselessUpdate(const std::string& component_name,
                                const lepus::Value& properties);
  tasm::LayoutNode* GetLayoutNodeForElement(lynx::tasm::Element* element);
  tasm::LayoutNode* GetLayoutNodeById(int32_t id);

 protected:
  lynx::shell::LynxShell* shell_;
  bool performance_ready_;
  std::shared_ptr<DevToolPlatformFacade> devtool_platform_facade_;
  std::weak_ptr<LynxDevToolMediator> devtool_mediator_wp_;

 private:
  bool uitree_enabled_;
  bool uitree_use_compression_;
  int uitree_compression_threshold_;
  std::unordered_map<int32_t, lynx::tasm::LayoutNode*> layout_nodes;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_INSPECTOR_UI_EXECUTOR_H_
