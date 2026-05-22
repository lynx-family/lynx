// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_LYNX_VIEW_PRIV_H_
#define PLATFORM_EMBEDDER_LYNX_VIEW_PRIV_H_

#include <memory>
#include <string>

#include "platform/embedder/core/lynx_template_renderer.h"
#include "platform/embedder/fetcher/lynx_resource_fetcher_holder.h"
#include "platform/embedder/lynx_ui_renderer.h"
#include "platform/embedder/lynx_view_clients.h"
#include "platform/embedder/module/extension_module_factory_impl.h"
#include "platform/embedder/module/lynx_module_manager_napi.h"
#include "platform/embedder/public/capi/lynx_view_capi.h"
#include "platform/embedder/public/capi/lynx_vsync_monitor_capi.h"
#include "platform/embedder/public/lynx_event_simulation_proxy.h"

struct lynx_view_t {
  std::unique_ptr<lynx::pub::LynxEventSimulationProxy> event_simulation_proxy =
      nullptr;
  std::unique_ptr<lynx::embedder::LynxTemplateRenderer> lynx_template_renderer =
      nullptr;
  std::unique_ptr<lynx::embedder::LynxViewClients> lynx_view_clients = nullptr;
#if ENABLE_NAPI_BINDING
  std::shared_ptr<lynx::embedder::LynxModuleManagerNAPI> lynx_module_manager =
      nullptr;
  std::shared_ptr<lynx::embedder::ExtensionModuleFactoryImpl>
      extension_factory_ = nullptr;
#endif
  std::unique_ptr<lynx::embedder::LynxUIRenderer> lynx_ui_renderer = nullptr;
  std::shared_ptr<lynx::embedder::LynxResourceFetcherHolder>
      resource_fetcher_holder = nullptr;
  lynx_vsync_monitor_t* custom_vsync_monitor = nullptr;
  std::shared_ptr<lynx::tasm::TemplateData> global_props = nullptr;
  void* user_data = nullptr;
  std::string webview2_fixed_runtime_path;
};

#endif  // PLATFORM_EMBEDDER_LYNX_VIEW_PRIV_H_
