// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/js_debug/lepus/inspector_lepus_observer_impl.h"

#include "core/runtime/common/lynx_console_helper.h"
#include "devtool/lynx_devtool/js_debug/lepus/inspector_lepus_debugger_impl.h"
#if defined(OS_ANDROID) || defined(OS_IOS)
#include "devtool/lynx_devtool/js_debug/lepus/manager/lepus_inspector_manager_impl.h"
#endif

namespace lynx {
namespace devtool {

static int32_t GetLevelNumByStr(const std::string &level) {
  static base::NoDestructor<std::unordered_map<std::string, int>> level_map(
      {{piper::LepusConsoleAlog, piper::CONSOLE_LOG_ALOG},
       {piper::LepusConsoleDebug, piper::CONSOLE_LOG_INFO},
       {piper::LepusConsoleError, piper::CONSOLE_LOG_ERROR},
       {piper::LepusConsoleInfo, piper::CONSOLE_LOG_INFO},
       {piper::LepusConsoleLog, piper::CONSOLE_LOG_LOG},
       {piper::LepusConsoleReport, piper::CONSOLE_LOG_REPORT},
       {piper::LepusConsoleWarn, piper::CONSOLE_LOG_WARNING}});
  return (*level_map)[level];
}

InspectorLepusObserverImpl::InspectorLepusObserverImpl(
    const std::shared_ptr<InspectorLepusDebuggerImpl> &debugger)
    : debugger_wp_(debugger) {}

std::unique_ptr<lepus::LepusInspectorManager>
InspectorLepusObserverImpl::CreateLepusInspectorManager() {
#if defined(OS_ANDROID) || defined(OS_IOS)
  return std::make_unique<lepus::LepusInspectorManagerImpl>();
#endif
  return nullptr;
}

std::string InspectorLepusObserverImpl::GetDebugInfo(const std::string &url) {
  auto sp = debugger_wp_.lock();
  if (sp != nullptr) {
    return sp->GetDebugInfo(url);
  }
  return "";
}

void InspectorLepusObserverImpl::SetDebugInfoUrl(const std::string &url) {
  auto sp = debugger_wp_.lock();
  if (sp != nullptr) {
    sp->SetDebugInfoUrl(url);
  }
}

void InspectorLepusObserverImpl::OnInspectorInited(
    const std::string &vm_type, const std::string &name,
    const std::shared_ptr<devtool::InspectorClientNG> &client) {
  auto sp = debugger_wp_.lock();
  if (sp != nullptr) {
    sp->OnInspectorInited(vm_type, name, client);
  }
}

void InspectorLepusObserverImpl::OnContextDestroyed(const std::string &name) {
  auto sp = debugger_wp_.lock();
  if (sp != nullptr) {
    sp->OnContextDestroyed(name);
  }
}

void InspectorLepusObserverImpl::OnConsoleMessage(const std::string &level,
                                                  const std::string &msg) {
  if (need_post_console_) {
    auto sp = mediator_ptr_.lock();
    if (sp != nullptr) {
      int32_t level_num = GetLevelNumByStr(level);
      auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
      sp->SendLogEntryAddedEvent({msg, level_num, static_cast<int64_t>(ts)});
    }
  }
}

}  // namespace devtool
}  // namespace lynx
