// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "base/include/log/logging.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/services/replay/replay_controller.h"
#include "devtool/lynx_devtool/agent/hierarchy_observer_impl.h"
#include "devtool/lynx_devtool/agent/inspector_common_observer_impl.h"
#include "devtool/lynx_devtool/agent/inspector_element_observer_impl.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"
#include "devtool/lynx_devtool/js_debug/js/inspector_java_script_debugger_impl.h"
#include "devtool/lynx_devtool/js_debug/lepus/inspector_lepus_debugger_impl.h"
#include "devtool/lynx_devtool/lynx_devtool_ng.h"
#include "devtool/lynx_devtool/tracing/devtool_trace_event_def.h"

namespace lynx {
namespace devtool {

namespace {

int GenerateViewId() {
  static base::NoDestructor<std::atomic<int>> id{1};
  return (*id)++;
}

}  // namespace

LynxDevToolMediator::LynxDevToolMediator() { view_id_ = GenerateViewId(); }

void LynxDevToolMediator::Init(
    lynx::shell::LynxShell* shell,
    const std::shared_ptr<LynxDevToolNG>& lynx_devtool_ng) {
  devtool_wp_ = lynx_devtool_ng;
  auto* runners = shell->GetRunners();
  tasm::TemplateAssembler* tasm = shell->GetTasm();
  tasm_task_runner_ = runners->GetTASMTaskRunner();
  js_task_runner_ = runners->GetJSTaskRunner();

  ui_task_runner_ = runners->GetUITaskRunner();

  // Get the enabled state of the WhiteBoard and reset it after reloading.
  bool white_board_enabled = false;
  if (element_executor_ != nullptr) {
    white_board_enabled = element_executor_->IsWhiteBoardEnabled();
  }

  element_executor_ = std::make_shared<InspectorTasmExecutor>(
      shared_from_this(), tasm, view_id_);
  ui_executor_ = std::make_shared<InspectorUIExecutor>(shared_from_this());
  ui_executor_->SetShell(shell);
  if (!devtool_executor_) {
    devtool_executor_ =
        std::make_shared<InspectorDefaultExecutor>(shared_from_this());
  }
  if (fully_initialized_) {
    RunOnDevToolThread([executor = devtool_executor_]() { executor->Reset(); });
  }
  if (js_debugger_ == nullptr) {
    js_debugger_ = std::make_shared<InspectorJavaScriptDebuggerImpl>(
        shared_from_this(), view_id_);
  }
  if (lepus_debugger_ == nullptr) {
    lepus_debugger_ =
        std::make_shared<InspectorLepusDebuggerImpl>(shared_from_this());
    int64_t record_id = reinterpret_cast<int64_t>(shell);
    lepus_debugger_->SetRecordID(record_id);
  }

  // shell set element observer in tasm thread;
  shell->SetInspectorElementObserver(
      std::make_shared<InspectorElementObserverImpl>(element_executor_));
  shell->SetHierarchyObserver(
      std::make_shared<lynx::devtool::HierarchyObserverImpl>(ui_executor_));
  auto runtime_observer = js_debugger_->GetInspectorRuntimeObserver();
  runtime_observer->SetDevToolMediator(shared_from_this());
  shell->SetInspectorRuntimeObserver(runtime_observer);
  auto lepus_observer = lepus_debugger_->GetInspectorLepusObserver();
  lepus_observer->SetConsolePostNeeded(!shell->IsRuntimeEnabled());
  lepus_observer->SetDevToolMediator(shared_from_this());
  tasm->SetLepusObserver(lepus_observer);
  tasm::replay::ReplayController::SetDevToolObserver(
      std::make_shared<lynx::devtool::InspectorCommonObserverImpl>(
          lynx_devtool_ng->GetCurrentSender(), shared_from_this()));

  auto white_board_delegate = tasm->GetWhiteBoardDelegate();
  if (white_board_delegate != nullptr) {
    const auto& inspector_delegate =
        element_executor_->GetWhiteBoardInspectorDelegate();
    InitWhiteBoardInspector(white_board_delegate, inspector_delegate);
    element_executor_->SetWhiteBoardEnabled(white_board_enabled);
  }

  // When using background runtime, `OnAttached()` is called before `Init()`, so
  // `lepus_debugger_` is not initialized at this time. Therefore, we need to
  // call `OnAttached()` again.
  if (attached_) {
    OnAttached();
  }

  fully_initialized_ = true;
}

void LynxDevToolMediator::SetDevToolPlatformFacade(
    const std::shared_ptr<DevToolPlatformFacade>& platform_facade) {
  if (ui_executor_) {
    ui_executor_->SetDevToolPlatformFacade(platform_facade);
  }
  if (js_debugger_) {
    js_debugger_->SetDevToolPlatformFacade(platform_facade);
  }
  if (devtool_executor_) {
    devtool_executor_->SetDevToolPlatformFacade(platform_facade);
  }
  if (lepus_debugger_) {
    lepus_debugger_->SetDevToolPlatformFacade(platform_facade);
  }
  if (element_executor_) {
    element_executor_->SetDevToolPlatformFacade(platform_facade);
  }
}

std::shared_ptr<lynx::runtime::js::InspectorRuntimeObserverNG>
LynxDevToolMediator::InitWhenBackgroundRuntimeCreated(
    const std::string& group_thread_name,
    const std::shared_ptr<LynxDevToolNG>& lynx_devtool_ng) {
  devtool_wp_ = lynx_devtool_ng;
  js_task_runner_ =
      lynx::base::TaskRunnerManufactor::GetJSRunner(group_thread_name);
  if (js_debugger_ == nullptr) {
    js_debugger_ =
        std::make_shared<lynx::devtool::InspectorJavaScriptDebuggerImpl>(
            shared_from_this(), view_id_);
  }
  if (!devtool_executor_) {
    devtool_executor_ =
        std::make_shared<InspectorDefaultExecutor>(shared_from_this());
  }
  auto runtime_observer = js_debugger_->GetInspectorRuntimeObserver();

  runtime_observer->SetDevToolMediator(shared_from_this());
  return runtime_observer;
}

void LynxDevToolMediator::OnAttached() {
  attached_ = true;
  if (js_debugger_ != nullptr) {
    js_debugger_->OnAttached();
  }
  if (lepus_debugger_ != nullptr) {
    lepus_debugger_->OnAttached();
  }
}

void LynxDevToolMediator::SetTag(const std::string& tag) {
  if (js_debugger_ != nullptr) {
    auto runtime_observer = js_debugger_->GetInspectorRuntimeObserver();
    runtime_observer->SetTag(tag);
  }
}

// DOM protocol
void LynxDevToolMediator::QuerySelector(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->QuerySelector(sender, message);
                    });
  }
}

void LynxDevToolMediator::GetAttributes(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->GetAttributes(sender, message);
                    });
  }
}

void LynxDevToolMediator::InnerText(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->InnerText(sender, message);
                    });
  }
}

void LynxDevToolMediator::QuerySelectorAll(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->QuerySelectorAll(sender, message);
                    });
  }
}

void LynxDevToolMediator::DOM_Enable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->DOM_Enable(sender, message);
                    });
  }
}

void LynxDevToolMediator::DOM_Disable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->DOM_Disable(sender, message);
                    });
  }
}

void LynxDevToolMediator::GetDocument(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->GetDocument(sender, message);
                    });
  }
}

void LynxDevToolMediator::GetDocumentWithBoxModel(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (ui_task_runner_) {
    RunOnTaskRunner(ui_task_runner_, [element_executor = element_executor_,
                                      sender, message]() {
      element_executor->GetDocumentWithBoxModel(sender, message);
    });
  }
}

void LynxDevToolMediator::RequestChildNodes(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->RequestChildNodes(sender, message);
                    });
  }
}

void LynxDevToolMediator::DOM_GetBoxModel(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (ui_task_runner_) {
    RunOnTaskRunner(ui_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->DOM_GetBoxModel(sender, message);
                    });
  }
}

void LynxDevToolMediator::SetAttributesAsText(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->SetAttributesAsText(sender, message);
                    });
  }
}

void LynxDevToolMediator::MarkUndoableState(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->MarkUndoableState(sender, message);
                    });
  }
}

void LynxDevToolMediator::PushNodesByBackendIdsToFrontend(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_, [element_executor = element_executor_,
                                        sender, message]() {
      element_executor->PushNodesByBackendIdsToFrontend(sender, message);
    });
  }
}

void LynxDevToolMediator::RemoveNode(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->RemoveNode(sender, message);
                    });
  }
}

void LynxDevToolMediator::MoveTo(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->MoveTo(sender, message);
                    });
  }
}

void LynxDevToolMediator::CopyTo(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->CopyTo(sender, message);
                    });
  }
}

void LynxDevToolMediator::GetOuterHTML(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->GetOuterHTML(sender, message);
                    });
  }
}

void LynxDevToolMediator::SetOuterHTML(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->SetOuterHTML(sender, message);
                    });
  }
}

void LynxDevToolMediator::SetInspectedNode(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->SetInspectedNode(sender, message);
                    });
  }
}

void LynxDevToolMediator::PerformSearch(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->PerformSearch(sender, message);
                    });
  }
}

void LynxDevToolMediator::GetSearchResults(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->GetSearchResults(sender, message);
                    });
  }
}

void LynxDevToolMediator::DiscardSearchResults(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->DiscardSearchResults(sender, message);
                    });
  }
}

void LynxDevToolMediator::GetOriginalNodeIndex(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->GetOriginalNodeIndex(sender, message);
                    });
  }
}

void LynxDevToolMediator::GetNodeForLocation(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (ui_task_runner_) {
    RunOnTaskRunner(ui_task_runner_,
                    [ui_executor = ui_executor_, sender, message]() {
                      ui_executor->GetNodeForLocation(sender, message);
                    });
  }
}

void LynxDevToolMediator::ScrollIntoViewIfNeeded(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  RunOnTASMThread([sender, message, executor = element_executor_] {
    executor->ScrollIntoViewIfNeeded(sender, message);
  });
}

void LynxDevToolMediator::DOMEnableDomTree(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  RunOnTASMThread([sender, message, executor = element_executor_] {
    executor->DOMEnableDomTree(sender, message);
  });
}

void LynxDevToolMediator::DOMDisableDomTree(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  RunOnTASMThread([sender, message, executor = element_executor_] {
    executor->DOMDisableDomTree(sender, message);
  });
}

void LynxDevToolMediator::ScrollIntoView(int node_id) {
  RunOnUIThread([executor = ui_executor_, node_id] {
    executor->ScrollIntoView(node_id);
  });
}

void LynxDevToolMediator::PageReload(bool ignore_cache) {
  RunOnUIThread([executor = ui_executor_, ignore_cache] {
    executor->PageReload(ignore_cache);
  });
}

// CSS protocol
void LynxDevToolMediator::CSS_Enable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->CSS_Enable(sender, message);
                    });
  }
}

void LynxDevToolMediator::CSS_Disable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->CSS_Disable(sender, message);
                    });
  }
}

void LynxDevToolMediator::GetMatchedStylesForNode(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_, [element_executor = element_executor_,
                                        sender, message]() {
      element_executor->GetMatchedStylesForNode(sender, message);
    });
  }
}

void LynxDevToolMediator::GetComputedStyleForNode(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (ui_task_runner_) {
    RunOnTaskRunner(ui_task_runner_, [element_executor = element_executor_,
                                      sender, message]() {
      element_executor->GetComputedStyleForNode(sender, message);
    });
  }
}

void LynxDevToolMediator::GetInlineStylesForNode(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->GetInlineStylesForNode(sender, message);
                    });
  }
}

void LynxDevToolMediator::SetStyleTexts(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->SetStyleTexts(sender, message);
                    });
  }
}

void LynxDevToolMediator::GetStyleSheetText(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->GetStyleSheetText(sender, message);
                    });
  }
}

void LynxDevToolMediator::GetBackgroundColors(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->GetBackgroundColors(sender, message);
                    });
  }
}

void LynxDevToolMediator::SetStyleSheetText(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->SetStyleSheetText(sender, message);
                    });
  }
}

void LynxDevToolMediator::CreateStyleSheet(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->CreateStyleSheet(sender, message);
                    });
  }
}

void LynxDevToolMediator::AddRule(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->AddRule(sender, message);
                    });
  }
}

void LynxDevToolMediator::StartRuleUsageTracking(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->StartRuleUsageTracking(sender, message);
                    });
  }
}
void LynxDevToolMediator::UpdateRuleUsageTracking(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_, [element_executor = element_executor_,
                                        sender, message]() {
      element_executor->UpdateRuleUsageTracking(sender, message);
    });
  }
}

void LynxDevToolMediator::StopRuleUsageTracking(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->StopRuleUsageTracking(sender, message);
                    });
  }
}

void LynxDevToolMediator::HighlightNode(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->HighlightNode(sender, message);
                    });
  }
}

void LynxDevToolMediator::HideHighlight(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message]() {
                      element_executor->HideHighlight(sender, message);
                    });
  }
}

void LynxDevToolMediator::Destroy() {
  // Must be called before destructing, because in the destructor of
  // InspectorJavaScriptDebuggerImpl, we will post a task to the JS thread by
  // using the weak_ptr of LynxDevToolMediator saved in it.
  if (js_debugger_ != nullptr) {
    js_debugger_->StopDebug();
    js_debugger_.reset();
  }
}

void LynxDevToolMediator::DispatchJSMessage(const Json::Value& message) {
  if (message.isMember(kKeySessionId) && lepus_debugger_ != nullptr) {
    std::string msg = message.toStyledString();
    TRACE_EVENT(LYNX_TRACE_CATEGORY_DEVTOOL,
                LYNX_DEVTOOL_MEDIATOR_DISPATCH_MTS_MESSAGE, "msg", msg);
    lepus_debugger_->DispatchMessage(msg, message[kKeySessionId].asString());
  } else if (js_debugger_ != nullptr) {
    std::string msg = message.toStyledString();
    TRACE_EVENT(LYNX_TRACE_CATEGORY_DEVTOOL,
                LYNX_DEVTOOL_MEDIATOR_DISPATCH_BTS_MESSAGE, "msg", msg);
    js_debugger_->DispatchMessage(msg);
  }
}

void LynxDevToolMediator::UpdateTarget() {
  if (lepus_debugger_ != nullptr) {
    lepus_debugger_->UpdateTarget();
  }
}

void LynxDevToolMediator::SetRuntimeEnableNeeded(bool enable) {
  CHECK_NULL_AND_LOG_RETURN(js_debugger_, "js_debugger_ is null");
  js_debugger_->SetRuntimeEnableNeeded(enable);
}

void LynxDevToolMediator::RunOnJSThread(lynx::base::closure&& closure,
                                        bool run_now) {
  RunOnTaskRunner(js_task_runner_, std::move(closure), run_now);
}

bool LynxDevToolMediator::RunOnTASMThread(lynx::base::closure&& closure,
                                          bool run_now) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_, std::move(closure), run_now);
    return true;
  }
  return false;
}

bool LynxDevToolMediator::RunOnUIThread(lynx::base::closure&& closure,
                                        bool run_now) {
  if (ui_task_runner_) {
    RunOnTaskRunner(ui_task_runner_, std::move(closure), run_now);
    return true;
  }
  return false;
}

bool LynxDevToolMediator::RunOnDevToolThread(lynx::base::closure&& closure,
                                             bool run_now) {
  if (default_task_runner_) {
    RunOnTaskRunner(default_task_runner_, std::move(closure), run_now);
    return true;
  }
  return false;
}

void LynxDevToolMediator::StartScreencast(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (ui_task_runner_) {
    RunOnTaskRunner(ui_task_runner_,
                    [ui_executor = ui_executor_, sender, message]() {
                      ui_executor->StartScreencast(sender, message);
                    });
  }
}

void LynxDevToolMediator::StopScreencast(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (ui_task_runner_) {
    RunOnTaskRunner(ui_task_runner_,
                    [ui_executor = ui_executor_, sender, message]() {
                      ui_executor->StopScreencast(sender, message);
                    });
  }
}

void LynxDevToolMediator::ScreencastFrameAck(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (ui_task_runner_) {
    RunOnTaskRunner(ui_task_runner_,
                    [ui_executor = ui_executor_, sender, message] {
                      ui_executor->ScreencastFrameAck(sender, message);
                    });
  }
}

void LynxDevToolMediator::PageEnable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (ui_task_runner_) {
    RunOnTaskRunner(ui_task_runner_,
                    [ui_executor = ui_executor_, sender, message] {
                      ui_executor->PageEnable(sender, message);
                    });
  }
}

void LynxDevToolMediator::PageCanEmulate(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (ui_task_runner_) {
    RunOnTaskRunner(ui_task_runner_,
                    [ui_executor = ui_executor_, sender, message] {
                      ui_executor->PageCanEmulate(sender, message);
                    });
  }
}

void LynxDevToolMediator::PageCanScreencast(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (ui_task_runner_) {
    RunOnTaskRunner(ui_task_runner_,
                    [ui_executor = ui_executor_, sender, message] {
                      ui_executor->PageCanScreencast(sender, message);
                    });
  }
}

void LynxDevToolMediator::PageGetResourceContent(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [element_executor = element_executor_, sender, message] {
                      element_executor->PageGetResourceContent(sender, message);
                    });
  }
}

void LynxDevToolMediator::PageGetResourceTree(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (ui_task_runner_) {
    RunOnTaskRunner(ui_task_runner_,
                    [ui_executor = ui_executor_, sender, message] {
                      ui_executor->PageGetResourceTree(sender, message);
                    });
  }
}

void LynxDevToolMediator::PageReload(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (ui_task_runner_) {
    RunOnTaskRunner(ui_task_runner_,
                    [ui_executor = ui_executor_, sender, message] {
                      ui_executor->PageReload(sender, message);
                    });
  }
}

void LynxDevToolMediator::PageNavigate(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (ui_task_runner_) {
    RunOnTaskRunner(ui_task_runner_,
                    [ui_executor = ui_executor_, sender, message] {
                      ui_executor->PageNavigate(sender, message);
                    });
  }
}

void LynxDevToolMediator::GetScreenshot(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (ui_task_runner_) {
    RunOnTaskRunner(ui_task_runner_,
                    [ui_executor = ui_executor_, sender, message] {
                      ui_executor->GetScreenshot(sender, message);
                    });
  }
}

void LynxDevToolMediator::UITree_Enable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (ui_task_runner_) {
    RunOnTaskRunner(ui_task_runner_,
                    [ui_executor = ui_executor_, sender, message] {
                      ui_executor->UITree_Enable(sender, message);
                    });
  }
}

void LynxDevToolMediator::UITree_Disable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (ui_task_runner_) {
    RunOnTaskRunner(ui_task_runner_,
                    [ui_executor = ui_executor_, sender, message] {
                      ui_executor->UITree_Disable(sender, message);
                    });
  }
}

void LynxDevToolMediator::GetLynxUITree(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (ui_task_runner_) {
    RunOnTaskRunner(ui_task_runner_,
                    [ui_executor = ui_executor_, sender, message] {
                      ui_executor->GetLynxUITree(sender, message);
                    });
  }
}

void LynxDevToolMediator::GetUIInfoForNode(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (ui_task_runner_) {
    RunOnTaskRunner(ui_task_runner_,
                    [ui_executor = ui_executor_, sender, message] {
                      ui_executor->GetUIInfoForNode(sender, message);
                    });
  }
}

void LynxDevToolMediator::SetUIStyle(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (ui_task_runner_) {
    RunOnTaskRunner(ui_task_runner_,
                    [ui_executor = ui_executor_, sender, message] {
                      ui_executor->SetUIStyle(sender, message);
                    });
  }
}

void LynxDevToolMediator::WhiteBoardEnable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [executor = element_executor_, sender, message] {
                      executor->WhiteBoardEnable(sender, message);
                    });
  } else if (js_task_runner_) {
    RunOnJSThread([js_debugger = js_debugger_, sender, message] {
      js_debugger->WhiteBoardEnable(sender, message);
    });
  }
}

void LynxDevToolMediator::WhiteBoardDisable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [executor = element_executor_, sender, message] {
                      executor->WhiteBoardDisable(sender, message);
                    });
  } else if (js_task_runner_) {
    RunOnJSThread([js_debugger = js_debugger_, sender, message] {
      js_debugger->WhiteBoardDisable(sender, message);
    });
  }
}

void LynxDevToolMediator::WhiteBoardSetSharedData(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [executor = element_executor_, sender, message] {
                      executor->WhiteBoardSetSharedData(sender, message);
                    });
  } else if (js_task_runner_) {
    RunOnJSThread([js_debugger = js_debugger_, sender, message] {
      js_debugger->WhiteBoardSetSharedData(sender, message);
    });
  }
}

void LynxDevToolMediator::WhiteBoardGetSharedData(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [executor = element_executor_, sender, message] {
                      executor->WhiteBoardGetSharedData(sender, message);
                    });
  } else if (js_task_runner_) {
    RunOnJSThread([js_debugger = js_debugger_, sender, message] {
      js_debugger->WhiteBoardGetSharedData(sender, message);
    });
  }
}

void LynxDevToolMediator::WhiteBoardRemoveSharedData(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [executor = element_executor_, sender, message] {
                      executor->WhiteBoardRemoveSharedData(sender, message);
                    });
  } else if (js_task_runner_) {
    RunOnJSThread([js_debugger = js_debugger_, sender, message] {
      js_debugger->WhiteBoardRemoveSharedData(sender, message);
    });
  }
}

void LynxDevToolMediator::WhiteBoardClear(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (tasm_task_runner_) {
    RunOnTaskRunner(tasm_task_runner_,
                    [executor = element_executor_, sender, message] {
                      executor->WhiteBoardClear(sender, message);
                    });
  } else if (js_task_runner_) {
    RunOnJSThread([js_debugger = js_debugger_, sender, message] {
      js_debugger->WhiteBoardClear(sender, message);
    });
  }
}

void LynxDevToolMediator::SendCDPEvent(const Json::Value& msg) {
  SendCDPEventImpl(msg);
}

void LynxDevToolMediator::SendCDPEvent(const std::string& msg) {
  SendCDPEventImpl(msg);
}

template <typename MsgType>
void LynxDevToolMediator::SendCDPEventImpl(const MsgType& msg) {
  auto devtool = devtool_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool, "devtool is null");
  devtool->GetMessageSender()->SendMessage("CDP", msg);

  // send cdp event message to the SDK-side listener
  std::lock_guard<std::mutex> lock(cdp_event_listener_mutex_);
  for (auto& listener : cdp_event_listener_map_) {
    listener.second->SendMessage("CDP", msg);
  }
}

void LynxDevToolMediator::AddCDPEventListener(
    const std::string& name, const std::shared_ptr<MessageSender>& listener) {
  std::lock_guard<std::mutex> lock(cdp_event_listener_mutex_);
  auto it = cdp_event_listener_map_.find(name);
  if (it != cdp_event_listener_map_.end()) {
    LOGI("CDPEventListener has exists:" << it->first);
  }
  cdp_event_listener_map_[name] = listener;
}

void LynxDevToolMediator::RemoveCDPEventListener(const std::string& name) {
  LOGI("RemoveCDPEventListener: " << name);
  std::lock_guard<std::mutex> lock(cdp_event_listener_mutex_);
  cdp_event_listener_map_.erase(name);
}

void LynxDevToolMediator::EmulateTouchFromMouseEvent(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  RunOnUIThread([sender, message, executor = ui_executor_] {
    executor->EmulateTouchFromMouseEvent(sender, message);
  });
}

void LynxDevToolMediator::InspectorEnable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  RunOnDevToolThread([sender, message, executor = devtool_executor_] {
    executor->InspectorEnable(sender, message);
  });
}

void LynxDevToolMediator::InspectorDetached(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  RunOnDevToolThread([sender, message, executor = devtool_executor_] {
    executor->InspectorDetached(sender, message);
  });
}

void LynxDevToolMediator::PerformanceEnable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  RunOnUIThread([sender, message, executor = ui_executor_] {
    executor->PerformanceEnable(sender, message);
  });
}

void LynxDevToolMediator::PerformanceDisable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  RunOnUIThread([sender, message, executor = ui_executor_] {
    executor->PerformanceDisable(sender, message);
  });
}

void LynxDevToolMediator::getAllTimingInfo(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  RunOnUIThread([sender, message, executor = ui_executor_] {
    executor->getAllTimingInfo(sender, message);
  });
}

void LynxDevToolMediator::LogEnable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  RunOnDevToolThread([sender, message, executor = devtool_executor_] {
    executor->LogEnable(sender, message);
  });
}

void LynxDevToolMediator::LogDisable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  RunOnDevToolThread([sender, message, executor = devtool_executor_] {
    executor->LogDisable(sender, message);
  });
}

void LynxDevToolMediator::LogClear(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  RunOnDevToolThread([sender, message, executor = devtool_executor_] {
    executor->LogClear(sender, message);
  });
}

void LynxDevToolMediator::SendLogEntryAddedEvent(
    const lynx::runtime::js::ConsoleMessage& message) {
  RunOnDevToolThread([message, executor = devtool_executor_] {
    executor->SendLogEntryAddedEvent(message);
  });
}

void LynxDevToolMediator::LayerTreeEnable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  RunOnUIThread([sender, message, executor = element_executor_] {
    executor->LayerTreeEnable(sender, message);
  });
}

void LynxDevToolMediator::LayerTreeDisable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  RunOnUIThread([sender, message, executor = element_executor_] {
    executor->LayerTreeDisable(sender, message);
  });
}

void LynxDevToolMediator::SendLayerTreeDidChangeEvent() {
  RunOnUIThread([executor = element_executor_] {
    executor->SendLayerTreeDidChangeEvent();
  });
}

void LynxDevToolMediator::CompositingReasons(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  RunOnUIThread([sender, message, executor = element_executor_] {
    executor->CompositingReasons(sender, message);
  });
}

std::vector<double> LynxDevToolMediator::GetBoxModel(tasm::Element* element) {
  if (!ui_task_runner_->RunsTasksOnCurrentThread()) {
    LOGE("LynxDevToolMediator::GetBoxModel must be called on the UI thread");
    return std::vector<double>();
  }
  return ui_executor_->GetBoxModel(element);
}

SLNode* LynxDevToolMediator::GetLayoutObjectForElement(
    lynx::tasm::Element* element) {
  if (!ui_task_runner_->RunsTasksOnCurrentThread()) {
    LOGE(
        "LynxDevToolMediator::GetLayoutObjectForElement must be called on the "
        "UI "
        "thread");
    return nullptr;
  }
  return ui_executor_->GetLayoutObjectForElement(element);
}

void LynxDevToolMediator::SendLayoutTree() {
  // Execute in UI thread since GetLayoutObjectForElement has assertion for
  // UI thread
  RunOnUIThread([executor = element_executor_] { executor->SendLayoutTree(); });
}

void LynxDevToolMediator::LynxGetProperties(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  RunOnTASMThread([sender, message, executor = element_executor_] {
    executor->LynxGetProperties(sender, message);
  });
}

void LynxDevToolMediator::LynxGetData(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  RunOnTASMThread([sender, message, executor = element_executor_] {
    executor->LynxGetData(sender, message);
  });
}

void LynxDevToolMediator::LynxGetComponentId(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  RunOnTASMThread([sender, message, executor = element_executor_] {
    executor->LynxGetComponentId(sender, message);
  });
}

void LynxDevToolMediator::LynxSetTraceMode(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  RunOnDevToolThread([sender, message, executor = devtool_executor_] {
    executor->LynxSetTraceMode(sender, message);
  });
}

void LynxDevToolMediator::LynxGetRectToWindow(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  RunOnUIThread([sender, message, executor = ui_executor_] {
    executor->LynxGetRectToWindow(sender, message);
  });
}

void LynxDevToolMediator::LynxGetVersion(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  RunOnDevToolThread([sender, message, executor = devtool_executor_] {
    executor->LynxGetVersion(sender, message);
  });
}

void LynxDevToolMediator::LynxTransferData(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  RunOnUIThread([sender, message, executor = ui_executor_] {
    executor->LynxTransferData(sender, message);
  });
}

void LynxDevToolMediator::LynxGetViewLocationOnScreen(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  RunOnUIThread([sender, message, executor = ui_executor_] {
    executor->LynxGetViewLocationOnScreen(sender, message);
  });
}

void LynxDevToolMediator::LynxSendEventToVM(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  RunOnUIThread([sender, message, executor = ui_executor_] {
    executor->LynxSendEventToVM(sender, message);
  });
}

void LynxDevToolMediator::TemplateGetTemplateData(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  RunOnUIThread([sender, message, executor = ui_executor_] {
    executor->TemplateGetTemplateData(sender, message);
  });
}

void LynxDevToolMediator::TemplateGetTemplateJsInfo(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  RunOnUIThread([sender, message, executor = ui_executor_] {
    executor->TemplateGetTemplateJsInfo(sender, message);
  });
}

void LynxDevToolMediator::TemplateGetTemplateApiInfo(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  RunOnTASMThread([sender, message, executor = element_executor_] {
    executor->TemplateGetTemplateApiInfo(sender, message);
  });
}

void LynxDevToolMediator::InitWhiteBoardInspector(
    const std::shared_ptr<tasm::WhiteBoardDelegate>& delegate,
    const std::shared_ptr<WhiteBoardInspectorDelegate>& inspector_delegate) {
  if (delegate == nullptr || inspector_delegate == nullptr) {
    return;
  }
  auto inspector = std::static_pointer_cast<WhiteBoardInspectorImpl>(
      delegate->GetInspector());
  if (inspector == nullptr) {
    inspector = std::make_shared<WhiteBoardInspectorImpl>();
    delegate->SetInspector(inspector);
  }
  inspector->InsertDelegate(inspector_delegate, view_id_);
  inspector_delegate->SetInspector(inspector);
}

}  // namespace devtool
}  // namespace lynx
