// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "devtool/lynx_devtool/lynx_devtool_ng.h"

#include "core/shell/lynx_shell.h"
#include "devtool/base_devtool/native/public/abstract_devtool.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_agent.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_component_agent.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_css_agent_ng.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_debugger_agent.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_dom_agent_ng.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_heap_profiler_agent.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_input_agent.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_io_agent.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_layer_tree_agent_ng.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_log_agent.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_lynx_agent_ng.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_memory_agent.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_overlay_agent_ng.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_page_agent_ng.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_performance_agent.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_profiler_agent.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_runtime_agent.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_template_agent.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_testbench_recorder_agent.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_testbench_replay_agent.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_tracing_agent.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_ui_tree_agent.h"
#include "devtool/lynx_devtool/agent/domain_agent/system_info_agent.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace devtool {
static constexpr char kDomainKeyPrefix[] = "enable_cdp_domain_";
LynxDevToolNG::LynxDevToolNG()
    : devtool_mediator_(std::make_shared<LynxDevToolMediator>()) {
  static std::once_flag flag;
  std::call_once(flag, [] {
    auto& global_dispatcher =
        lynx::devtool::AbstractDevTool::GetGlobalMessageDispatcherInstance();

    if (lynx::tasm::LynxEnv::GetInstance().IsDevToolEnabled() ||
        lynx::tasm::LynxEnv::GetInstance().IsDebugModeEnabled()) {
      RegisterGlobalDomainAgents(global_dispatcher);
    } else if (lynx::tasm::LynxEnv::GetInstance()
                   .IsDevToolEnabledForDebuggableView()) {
      std::unordered_set<std::string> activated_domains =
          lynx::tasm::LynxEnv::GetInstance().GetActivatedCDPDomains();
      for (const auto& domain : activated_domains) {
        RegisterGlobalDomainAgents(global_dispatcher, domain);
      }
    }
  });
  if (lynx::tasm::LynxEnv::GetInstance().IsDevToolEnabled() ||
      lynx::tasm::LynxEnv::GetInstance().IsDebugModeEnabled()) {
    RegisterInstanceDomainAgents();
  } else if (lynx::tasm::LynxEnv::GetInstance()
                 .IsDevToolEnabledForDebuggableView()) {
    std::unordered_set<std::string> activated_domains =
        lynx::tasm::LynxEnv::GetInstance().GetActivatedCDPDomains();
    for (const auto& domain : activated_domains) {
      RegisterInstanceDomainAgents(domain);
    }
  }
}

LynxDevToolNG::~LynxDevToolNG() { devtool_mediator_->Destroy(); }

int32_t LynxDevToolNG::Attach(const std::string& url) {
  int32_t session_id = AbstractDevTool::Attach(url);
  devtool_mediator_->OnAttached();
  return session_id;
}

void LynxDevToolNG::RegisterGlobalDomainAgents(
    DevToolMessageDispatcher& global_dispatcher) {
  global_dispatcher.RegisterAgent("Tracing",
                                  std::make_unique<InspectorTracingAgent>());
  global_dispatcher.RegisterAgent(
      "Recording", std::make_unique<InspectorTestBenchRecorderAgent>());
  global_dispatcher.RegisterAgent(
      "Replay", std::make_unique<InspectorTestBenchReplayAgent>());
  global_dispatcher.RegisterAgent("IO", std::make_unique<InspectorIOAgent>());
  global_dispatcher.RegisterAgent("Memory",
                                  std::make_unique<InspectorMemoryAgent>());
  global_dispatcher.RegisterAgent("SystemInfo",
                                  std::make_unique<SystemInfoAgent>());
  global_dispatcher.RegisterAgent("Component",
                                  std::make_unique<InspectorComponentAgent>());
}

void LynxDevToolNG::RegisterGlobalDomainAgents(
    DevToolMessageDispatcher& global_dispatcher,
    const std::string& domain_key) {
  std::string domain_key_prefix(kDomainKeyPrefix);
  if (!domain_key.compare(domain_key_prefix + "tracing")) {
    global_dispatcher.RegisterAgent("Tracing",
                                    std::make_unique<InspectorTracingAgent>());
  } else if (!domain_key.compare(domain_key_prefix + "recording")) {
    global_dispatcher.RegisterAgent(
        "Recording", std::make_unique<InspectorTestBenchRecorderAgent>());
  } else if (!domain_key.compare(domain_key_prefix + "replay")) {
    global_dispatcher.RegisterAgent(
        "Replay", std::make_unique<InspectorTestBenchReplayAgent>());
  } else if (!domain_key.compare(domain_key_prefix + "io")) {
    global_dispatcher.RegisterAgent("IO", std::make_unique<InspectorIOAgent>());
  } else if (!domain_key.compare(domain_key_prefix + "systeminfo")) {
    global_dispatcher.RegisterAgent("SystemInfo",
                                    std::make_unique<SystemInfoAgent>());
  } else if (!domain_key.compare(domain_key_prefix + "component")) {
    global_dispatcher.RegisterAgent(
        "Component", std::make_unique<InspectorComponentAgent>());
  }
}

void LynxDevToolNG::RegisterInstanceDomainAgents() {
  RegisterAgent("Inspector",
                std::make_unique<InspectorAgent>(devtool_mediator_));
  RegisterAgent("CSS",
                std::make_unique<InspectorCSSAgentNG>(devtool_mediator_));
  RegisterAgent("Debugger",
                std::make_unique<InspectorDebuggerAgent>(devtool_mediator_));
  RegisterAgent("DOM",
                std::make_unique<InspectorDOMAgentNG>(devtool_mediator_));
  RegisterAgent("Overlay",
                std::make_unique<InspectorOverlayAgentNG>(devtool_mediator_));
  RegisterAgent("Input",
                std::make_unique<InspectorInputAgent>(devtool_mediator_));
  RegisterAgent("Log", std::make_unique<InspectorLogAgent>(devtool_mediator_));
  RegisterAgent("Page",
                std::make_unique<InspectorPageAgentNG>(devtool_mediator_));
  RegisterAgent("Runtime",
                std::make_unique<InspectorRuntimeAgent>(devtool_mediator_));
  RegisterAgent("Tracing", std::make_unique<InspectorTracingAgent>());
  RegisterAgent("Recording",
                std::make_unique<InspectorTestBenchRecorderAgent>());
  RegisterAgent("Replay", std::make_unique<InspectorTestBenchReplayAgent>());
  RegisterAgent("IO", std::make_unique<InspectorIOAgent>());
  RegisterAgent("HeapProfiler", std::make_unique<InspectorHeapProfilerAgent>(
                                    devtool_mediator_));
  RegisterAgent("Performance",
                std::make_unique<InspectorPerformanceAgent>(devtool_mediator_));
  RegisterAgent("Memory", std::make_unique<InspectorMemoryAgent>());
  RegisterAgent("SystemInfo", std::make_unique<SystemInfoAgent>());
  RegisterAgent("Lynx",
                std::make_unique<InspectorLynxAgentNG>(devtool_mediator_));
  RegisterAgent("Template",
                std::make_unique<InspectorTemplateAgent>(devtool_mediator_));
  RegisterAgent("Profiler",
                std::make_unique<InspectorProfilerAgent>(devtool_mediator_));
  RegisterAgent("Component", std::make_unique<InspectorComponentAgent>());
  RegisterAgent("LayerTree",
                std::make_unique<InspectorLayerTreeAgentNG>(devtool_mediator_));
  RegisterAgent("UITree",
                std::make_unique<InspectorUITreeAgent>(devtool_mediator_));
}

void LynxDevToolNG::RegisterInstanceDomainAgents(
    const std::string& domain_key) {
  std::string domain_key_prefix(kDomainKeyPrefix);
  if (!domain_key.compare(domain_key_prefix + "dom")) {
    RegisterAgent("DOM",
                  std::make_unique<InspectorDOMAgentNG>(devtool_mediator_));
  } else if (!domain_key.compare(domain_key_prefix + "css")) {
    RegisterAgent("CSS",
                  std::make_unique<InspectorCSSAgentNG>(devtool_mediator_));
  } else if (!domain_key.compare(domain_key_prefix + "page")) {
    RegisterAgent("Page",
                  std::make_unique<InspectorPageAgentNG>(devtool_mediator_));
  } else if (!domain_key.compare(domain_key_prefix + "inspector")) {
    RegisterAgent("Inspector",
                  std::make_unique<InspectorAgent>(devtool_mediator_));
  } else if (!domain_key.compare(domain_key_prefix + "debugger")) {
    RegisterAgent("Debugger",
                  std::make_unique<InspectorDebuggerAgent>(devtool_mediator_));
  } else if (!domain_key.compare(domain_key_prefix + "overlay")) {
    RegisterAgent("Overlay",
                  std::make_unique<InspectorOverlayAgentNG>(devtool_mediator_));
  } else if (!domain_key.compare(domain_key_prefix + "input")) {
    RegisterAgent("Input",
                  std::make_unique<InspectorInputAgent>(devtool_mediator_));
  } else if (!domain_key.compare(domain_key_prefix + "log")) {
    RegisterAgent("Log",
                  std::make_unique<InspectorLogAgent>(devtool_mediator_));
  } else if (!domain_key.compare(domain_key_prefix + "runtime")) {
    RegisterAgent("Runtime",
                  std::make_unique<InspectorRuntimeAgent>(devtool_mediator_));
  } else if (!domain_key.compare(domain_key_prefix + "tracing")) {
    RegisterAgent("Tracing", std::make_unique<InspectorTracingAgent>());
  } else if (!domain_key.compare(domain_key_prefix + "recording")) {
    RegisterAgent("Recording",
                  std::make_unique<InspectorTestBenchRecorderAgent>());
  } else if (!domain_key.compare(domain_key_prefix + "replay")) {
    RegisterAgent("Replay", std::make_unique<InspectorTestBenchReplayAgent>());
  } else if (!domain_key.compare(domain_key_prefix + "io")) {
    RegisterAgent("IO", std::make_unique<InspectorIOAgent>());
  } else if (!domain_key.compare(domain_key_prefix + "heapprofiler")) {
    RegisterAgent("HeapProfiler", std::make_unique<InspectorHeapProfilerAgent>(
                                      devtool_mediator_));
  } else if (!domain_key.compare(domain_key_prefix + "performance")) {
    RegisterAgent("Performance", std::make_unique<InspectorPerformanceAgent>(
                                     devtool_mediator_));
  } else if (!domain_key.compare(domain_key_prefix + "systeminfo")) {
    RegisterAgent("SystemInfo", std::make_unique<SystemInfoAgent>());
  } else if (!domain_key.compare(domain_key_prefix + "lynx")) {
    RegisterAgent("Lynx",
                  std::make_unique<InspectorLynxAgentNG>(devtool_mediator_));
  } else if (!domain_key.compare(domain_key_prefix + "template")) {
    RegisterAgent("Template",
                  std::make_unique<InspectorTemplateAgent>(devtool_mediator_));
  } else if (!domain_key.compare(domain_key_prefix + "profiler")) {
    RegisterAgent("Profiler",
                  std::make_unique<InspectorProfilerAgent>(devtool_mediator_));
  } else if (!domain_key.compare(domain_key_prefix + "component")) {
    RegisterAgent("Component", std::make_unique<InspectorComponentAgent>());
  } else if (!domain_key.compare(domain_key_prefix + "layertree")) {
    RegisterAgent("LayerTree", std::make_unique<InspectorLayerTreeAgentNG>(
                                   devtool_mediator_));
  }
}

void LynxDevToolNG::OnTasmCreated(intptr_t shell_ptr) {
  auto* shell = reinterpret_cast<lynx::shell::LynxShell*>(shell_ptr);
  devtool_mediator_->Init(shell, shared_from_this());
}

void LynxDevToolNG::SendMessageToDebugPlatform(const std::string& type,
                                               const std::string& message) {
  GetCurrentSender()->SendMessage(type, message);
}

void LynxDevToolNG::SetDevToolPlatformFacade(
    const std::shared_ptr<DevToolPlatformFacade>& platform_facade) {
  devtool_mediator_->SetDevToolPlatformFacade(platform_facade);
  platform_facade->InitWithDevToolMediator(devtool_mediator_);
}

std::shared_ptr<MessageSender> LynxDevToolNG::GetMessageSender() const {
  return GetCurrentSender();
}

std::shared_ptr<lynx::piper::InspectorRuntimeObserverNG>
LynxDevToolNG::OnBackgroundRuntimeCreated(
    const std::string& group_thread_name) {
  return devtool_mediator_->InitWhenBackgroundRuntimeCreated(
      group_thread_name, shared_from_this());
}

}  // namespace devtool

}  // namespace lynx
