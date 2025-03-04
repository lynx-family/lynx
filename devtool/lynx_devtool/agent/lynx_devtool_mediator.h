// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_LYNX_DEVTOOL_MEDIATOR_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_LYNX_DEVTOOL_MEDIATOR_H_

#include "base/include/fml/task_runner.h"
#include "core/shell/lynx_shell.h"
#include "devtool/base_devtool/native/public/message_sender.h"
#include "devtool/lynx_devtool/agent/inspector_default_executor.h"
#include "devtool/lynx_devtool/agent/inspector_tasm_executor.h"
#include "devtool/lynx_devtool/agent/inspector_ui_executor.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator_base.h"

namespace lynx {
namespace devtool {

class LynxDevToolNG;
class InspectorJavaScriptDebuggerImpl;
class InspectorLepusDebuggerImpl;

// Why LynxDevToolMediator and InspectorXXExecutor?
//
// 1. Thread Safety and Efficiency: In a multithreaded environment, access to
// shared resources needs to be synchronized to prevent data races and other
// thread safety issues. When Lynx enables multithreading strategies,
// LynxDevToolMediator is responsible for managing all thread dispatches,
// ensuring operations are performed on the correct thread, thus avoiding thread
// safety issues.
//
// 2. Code Readability and Stability: Without a unified thread scheduling,
// thread switching operations might be scattered throughout the code, reducing
// code readability and stability, and making future maintenance and iterations
// challenging. LynxDevToolMediator provides a unified thread scheduling, making
// it more intuitive on which thread the code runs, improving code readability
// and stability.
//
// 3. Decoupling and Modularity: The design of LynxDevToolMediator and
// InspectorXXXExecutor reduces the coupling between components and improves
// code modularity. For example, InspectorXXXAgent, which handles various domain
// CDP messages, only needs to parse CDP method json parameters and then
// dispatch them to the corresponding functions of LynxDevToolMediator for
// execution, without concerning the specific thread scheduling and execution
// details. This makes the code structure clearer and easier to understand and
// maintain.
//
// 4. Flexibility and Scalability: The design of LynxDevToolMediator and
// InspectorXXXExecutor makes the code more flexible and scalable. When new
// features need to be added or existing features need to be modified,
// modifications can be made in the corresponding executor without changing
// other parts of the code.

class LynxDevToolMediator
    : public std::enable_shared_from_this<LynxDevToolMediator>,
      public LynxDevToolMediatorBase {
 public:
  LynxDevToolMediator();
  ~LynxDevToolMediator() = default;

 public:
  void Init(lynx::shell::LynxShell* shell,
            const std::shared_ptr<LynxDevToolNG>& lynx_devtool_ng);
  void Destroy();

  void SetDevToolPlatformFacade(
      const std::shared_ptr<DevToolPlatformFacade>& platform_facade);
  std::shared_ptr<lynx::piper::InspectorRuntimeObserverNG>
  InitWhenBackgroundRuntimeCreated(
      const std::string& group_thread_name,
      const std::shared_ptr<LynxDevToolNG>& lynx_devtool_ng);
  void SetRuntimeEnableNeeded(bool enable);
  void OnAttached();

 public:
  // DOM domain -> tasm executor
  DECLARE_DEVTOOL_METHOD(QuerySelector);
  DECLARE_DEVTOOL_METHOD(GetAttributes);
  DECLARE_DEVTOOL_METHOD(InnerText);
  DECLARE_DEVTOOL_METHOD(QuerySelectorAll);
  DECLARE_DEVTOOL_METHOD(DOM_Enable);
  DECLARE_DEVTOOL_METHOD(DOM_Disable);
  DECLARE_DEVTOOL_METHOD(GetDocument);
  DECLARE_DEVTOOL_METHOD(GetDocumentWithBoxModel);
  DECLARE_DEVTOOL_METHOD(RequestChildNodes);
  DECLARE_DEVTOOL_METHOD(DOM_GetBoxModel);
  DECLARE_DEVTOOL_METHOD(SetAttributesAsText);
  DECLARE_DEVTOOL_METHOD(MarkUndoableState);
  DECLARE_DEVTOOL_METHOD(PushNodesByBackendIdsToFrontend);
  DECLARE_DEVTOOL_METHOD(RemoveNode);
  DECLARE_DEVTOOL_METHOD(MoveTo);
  DECLARE_DEVTOOL_METHOD(CopyTo);
  DECLARE_DEVTOOL_METHOD(GetOuterHTML);
  DECLARE_DEVTOOL_METHOD(SetOuterHTML);
  DECLARE_DEVTOOL_METHOD(SetInspectedNode);
  DECLARE_DEVTOOL_METHOD(PerformSearch);
  DECLARE_DEVTOOL_METHOD(GetSearchResults);
  DECLARE_DEVTOOL_METHOD(DiscardSearchResults);
  DECLARE_DEVTOOL_METHOD(GetOriginalNodeIndex);
  DECLARE_DEVTOOL_METHOD(ScrollIntoViewIfNeeded);
  DECLARE_DEVTOOL_METHOD(DOMEnableDomTree);
  DECLARE_DEVTOOL_METHOD(DOMDisableDomTree);
  DECLARE_DEVTOOL_METHOD(GetNodeForLocation);

  // CSS domain -> tasm executor
  DECLARE_DEVTOOL_METHOD(CSS_Enable);
  DECLARE_DEVTOOL_METHOD(CSS_Disable);
  DECLARE_DEVTOOL_METHOD(GetMatchedStylesForNode);
  DECLARE_DEVTOOL_METHOD(GetComputedStyleForNode);
  DECLARE_DEVTOOL_METHOD(GetInlineStylesForNode);
  DECLARE_DEVTOOL_METHOD(SetStyleTexts);
  DECLARE_DEVTOOL_METHOD(GetStyleSheetText);
  DECLARE_DEVTOOL_METHOD(GetBackgroundColors);
  DECLARE_DEVTOOL_METHOD(SetStyleSheetText);
  DECLARE_DEVTOOL_METHOD(CreateStyleSheet);
  DECLARE_DEVTOOL_METHOD(AddRule);
  DECLARE_DEVTOOL_METHOD(StartRuleUsageTracking);
  DECLARE_DEVTOOL_METHOD(UpdateRuleUsageTracking);
  DECLARE_DEVTOOL_METHOD(StopRuleUsageTracking);

  // Performance domain -> ui executor
  DECLARE_DEVTOOL_METHOD(PerformanceEnable)
  DECLARE_DEVTOOL_METHOD(PerformanceDisable)
  DECLARE_DEVTOOL_METHOD(getAllTimingInfo)

  // Input domain -> ui executor
  DECLARE_DEVTOOL_METHOD(EmulateTouchFromMouseEvent)

  // Inspector domain -> devtools executor
  DECLARE_DEVTOOL_METHOD(InspectorEnable)
  DECLARE_DEVTOOL_METHOD(InspectorDetached)

  // methods of Log domain -> devtool executor
  DECLARE_DEVTOOL_METHOD(LogEnable)
  DECLARE_DEVTOOL_METHOD(LogDisable)
  DECLARE_DEVTOOL_METHOD(LogClear)

  // events of Log domain -> devtool executor
  virtual void SendLogEntryAddedEvent(
      const lynx::piper::ConsoleMessage& message);
  // Lynx domain
  DECLARE_DEVTOOL_METHOD(LynxGetProperties)
  DECLARE_DEVTOOL_METHOD(LynxGetData)
  DECLARE_DEVTOOL_METHOD(LynxGetComponentId)
  DECLARE_DEVTOOL_METHOD(LynxSetTraceMode)
  DECLARE_DEVTOOL_METHOD(LynxGetRectToWindow)
  DECLARE_DEVTOOL_METHOD(LynxGetVersion)
  DECLARE_DEVTOOL_METHOD(LynxTransferData)
  DECLARE_DEVTOOL_METHOD(LynxGetViewLocationOnScreen)
  DECLARE_DEVTOOL_METHOD(LynxSendEventToVM)
  DECLARE_DEVTOOL_METHOD(GetScreenshot)

  // Template domain
  DECLARE_DEVTOOL_METHOD(TemplateGetTemplateData)
  DECLARE_DEVTOOL_METHOD(TemplateGetTemplateJsInfo)
  DECLARE_DEVTOOL_METHOD(TemplateGetTemplateApiInfo)

  // Overlay domain -> tasm executor
  DECLARE_DEVTOOL_METHOD(HighlightNode)
  DECLARE_DEVTOOL_METHOD(HideHighlight)

  // Layer Tree domain -> ui executor
  DECLARE_DEVTOOL_METHOD(LayerTreeEnable)
  DECLARE_DEVTOOL_METHOD(LayerTreeDisable)
  DECLARE_DEVTOOL_METHOD(LayerPainted)
  DECLARE_DEVTOOL_METHOD(CompositingReasons)

  // Page domain - > ui executor
  DECLARE_DEVTOOL_METHOD(StartScreencast)
  DECLARE_DEVTOOL_METHOD(StopScreencast)
  DECLARE_DEVTOOL_METHOD(ScreencastFrameAck)
  DECLARE_DEVTOOL_METHOD(PageEnable)
  DECLARE_DEVTOOL_METHOD(PageCanEmulate)
  DECLARE_DEVTOOL_METHOD(PageCanScreencast)
  DECLARE_DEVTOOL_METHOD(PageGetResourceContent)
  DECLARE_DEVTOOL_METHOD(PageGetResourceTree)
  DECLARE_DEVTOOL_METHOD(PageReload)
  DECLARE_DEVTOOL_METHOD(PageNavigate)

  // UITree domain - > ui executor
  DECLARE_DEVTOOL_METHOD(UITree_Enable)
  DECLARE_DEVTOOL_METHOD(UITree_Disable)
  DECLARE_DEVTOOL_METHOD(GetLynxUITree)
  DECLARE_DEVTOOL_METHOD(GetUIInfoForNode)
  DECLARE_DEVTOOL_METHOD(SetUIStyle)

 public:
  std::shared_ptr<InspectorUIExecutor> GetUIExecutor() { return ui_executor_; }
  std::shared_ptr<InspectorTasmExecutor> GetTasmExecutor() {
    return element_executor_;
  }
  std::shared_ptr<InspectorDefaultExecutor> GetDevToolExecutor() {
    return devtool_executor_;
  }
  const std::shared_ptr<lynx::devtool::InspectorJavaScriptDebuggerImpl>&
  GetJSDebugger() {
    return js_debugger_;
  }
  const std::shared_ptr<InspectorLepusDebuggerImpl>& GetLepusDebugger() {
    return lepus_debugger_;
  }
  const std::shared_ptr<lynx::devtool::MessageSender> GetMessageSender();

 public:
  void RunOnJSThread(lynx::base::closure&& closure, bool run_now = true);
  bool RunOnTASMThread(lynx::base::closure&& closure, bool run_now = true);
  bool RunOnDevToolThread(lynx::base::closure&& closure, bool run_now = true);

 public:
  std::vector<double> GetBoxModel(tasm::Element* element);
  lynx::tasm::LayoutNode* GetLayoutNodeForElement(lynx::tasm::Element* element);
  void SendLayoutTree();
  void SendCDPEvent(const Json::Value& msg);
  void DispatchJSMessage(const Json::Value& message);

  void LayerTreeDidChange();

  // implemented by ui executor
  void ScrollIntoView(int node_id);
  void PageReload(bool ignore_cache);

 private:
  bool RunOnUIThread(lynx::base::closure&& closure, bool run_now = true);

  lynx::fml::RefPtr<lynx::fml::TaskRunner> tasm_task_runner_;
  lynx::fml::RefPtr<lynx::fml::TaskRunner> ui_task_runner_;
  lynx::fml::RefPtr<lynx::fml::TaskRunner> js_task_runner_;

  std::shared_ptr<InspectorTasmExecutor> element_executor_;
  std::shared_ptr<InspectorUIExecutor> ui_executor_;
  std::shared_ptr<InspectorDefaultExecutor> devtool_executor_;
  std::shared_ptr<InspectorJavaScriptDebuggerImpl> js_debugger_;
  std::shared_ptr<InspectorLepusDebuggerImpl> lepus_debugger_;

  std::weak_ptr<LynxDevToolNG> devtool_wp_;
  bool fully_initialized_{false};
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_LYNX_DEVTOOL_MEDIATOR_H_
