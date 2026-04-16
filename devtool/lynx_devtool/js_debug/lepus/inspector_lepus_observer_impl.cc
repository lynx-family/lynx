// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/js_debug/lepus/inspector_lepus_observer_impl.h"

#include "core/runtime/common/lynx_console_helper.h"
#include "devtool/lynx_devtool/js_debug/helper/js_debug_helper.h"
#include "devtool/lynx_devtool/js_debug/lepus/inspector_lepus_debugger_impl.h"

namespace lynx {
namespace devtool {

static int32_t GetFuncNameByStr(const std::string& func_name) {
  static base::NoDestructor<std::unordered_map<std::string, int>> names_map(
      {{runtime::ConsoleAlog, runtime::CONSOLE_LOG_ALOG},
       {runtime::ConsoleDebug, runtime::CONSOLE_LOG_INFO},
       {runtime::ConsoleError, runtime::CONSOLE_LOG_ERROR},
       {runtime::ConsoleInfo, runtime::CONSOLE_LOG_INFO},
       {runtime::ConsoleLog, runtime::CONSOLE_LOG_LOG},
       {runtime::ConsoleReport, runtime::CONSOLE_LOG_REPORT},
       {runtime::ConsoleWarn, runtime::CONSOLE_LOG_WARNING}});
  auto maybe_name = names_map->find(func_name);
  if (maybe_name == names_map->end()) {
    return runtime::CONSOLE_UNKNOWN;
  }
  return maybe_name->second;
}

InspectorLepusObserverImpl::InspectorLepusObserverImpl(
    const std::shared_ptr<InspectorLepusDebuggerImpl>& debugger)
    : debugger_wp_(debugger) {}

std::unique_ptr<lepus::LepusInspectorManager>
InspectorLepusObserverImpl::CreateLepusInspectorManager(
    runtime::ContextType context_type) {
  return JSDebugHelper::GetInstance()->CreateLepusInspectorManager(
      context_type);
}

std::string InspectorLepusObserverImpl::GetDebugInfo(const std::string& url) {
  auto sp = debugger_wp_.lock();
  if (sp != nullptr) {
    return sp->GetDebugInfo(url);
  }
  return "";
}

void InspectorLepusObserverImpl::SetDebugInfoUrl(const std::string& url,
                                                 const std::string& file_name) {
  auto sp = debugger_wp_.lock();
  if (sp != nullptr) {
    sp->SetDebugInfoUrl(url, file_name);
  }
}

void InspectorLepusObserverImpl::OnInspectorInited(
    const std::string& vm_type, const std::string& name,
    const std::shared_ptr<devtool::InspectorClientNG>& client) {
  auto sp = debugger_wp_.lock();
  if (sp != nullptr) {
    sp->OnInspectorInited(vm_type, name, client);
  }
}

void InspectorLepusObserverImpl::OnContextDestroyed(const std::string& name) {
  auto sp = debugger_wp_.lock();
  if (sp != nullptr) {
    sp->OnContextDestroyed(name);
  }
}

void InspectorLepusObserverImpl::TakeOver(
    const std::shared_ptr<lepus::InspectorLepusObserver>& other) {
  if (other == nullptr) {
    return;
  }

  auto other_impl = std::static_pointer_cast<InspectorLepusObserverImpl>(other);
  if (this == other_impl.get()) {
    return;
  }

  CopyMembersFrom(other_impl);
}

void InspectorLepusObserverImpl::OnConsoleEvent(const std::string& level,
                                                const std::string& args) {
  if (need_post_console_) {
    auto sp = mediator_ptr_.lock();
    if (sp != nullptr) {
      int32_t level_num = GetFuncNameByStr(level);
      // TODO: support other console event.
      if (level_num == runtime::CONSOLE_UNKNOWN) {
        return;
      }
      auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
      sp->SendLogEntryAddedEvent({args, level_num, static_cast<int64_t>(ts)});
    }
  }
}

void InspectorLepusObserverImpl::PrepareForScriptEval(const std::string& name) {
  auto sp = debugger_wp_.lock();
  if (sp != nullptr) {
    sp->PrepareForScriptEval(name);
  }
}

void InspectorLepusObserverImpl::CopyMembersFrom(
    const std::shared_ptr<InspectorLepusObserverImpl>& other) {
  auto debugger = debugger_wp_.lock();
  if (debugger == nullptr) {
    return;
  }
  auto other_debugger = other->debugger_wp_.lock();
  debugger->TakeOver(other_debugger);
  need_post_console_ = other->need_post_console_;
  mediator_ptr_ = other->mediator_ptr_;

  auto mediator = mediator_ptr_.lock();
  if (mediator != nullptr) {
    mediator->UpdateLepusDebugger(debugger);
  }
}

}  // namespace devtool
}  // namespace lynx
