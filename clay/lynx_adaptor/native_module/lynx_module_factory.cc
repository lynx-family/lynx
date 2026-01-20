// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/native_module/lynx_module_factory.h"

#include "clay/lynx_adaptor/native_module/lynx_config_module.h"
#include "clay/lynx_adaptor/native_module/lynx_exposure_module.h"
#include "clay/lynx_adaptor/native_module/lynx_focus_module.h"
#include "clay/lynx_adaptor/native_module/lynx_intersection_observer_module.h"
#include "clay/lynx_adaptor/native_module/lynx_text_info_module.h"
#include "clay/lynx_adaptor/native_module/lynx_ui_method_module.h"

#if !defined(OS_IOS) && !defined(OS_TVOS) && !defined(OS_ANDROID)
#include "clay/lynx_adaptor/native_module/lynx_websocket_module.h"
#endif
#include "clay/ui/component/view_context.h"

namespace lynx {

lynx::runtime::NativeModuleFactory* LynxModuleFactory::CreateModuleFactory(
    clay::ViewContext* view_context) {
  auto module_factory = new LynxModuleFactory(view_context);
  module_factory->RegisterCreator(LynxConfigModule::GetName(),
                                  LynxConfigModule::Create);
  module_factory->RegisterCreator(LynxExposureModule::GetName(),
                                  LynxExposureModule::Create);
  module_factory->RegisterCreator(LynxFocusModule::GetName(),
                                  LynxFocusModule::Create);
  module_factory->RegisterCreator(LynxUIMethodModule::GetName(),
                                  LynxUIMethodModule::Create);
  module_factory->RegisterCreator(LynxTextInfoModule::GetName(),
                                  LynxTextInfoModule::Create);
  module_factory->RegisterCreator(LynxIntersectionObserverModule::GetName(),
                                  LynxIntersectionObserverModule::Create);
#if !defined(OS_ANDROID) && !defined(OS_IOS) && !defined(OS_TVOS)
  module_factory->RegisterCreator(LynxWebSocketModule::GetName(),
                                  LynxWebSocketModule::Create);
#endif
  return module_factory;
}

LynxModuleFactory::LynxModuleFactory(clay::ViewContext* view_context)
    : view_context_id_(view_context->unique_id()),
      task_runner_(view_context->GetUITaskRunner()) {}

std::shared_ptr<lynx::runtime::LynxNativeModule>
LynxModuleFactory::CreateModule(const std::string& name) {
  std::lock_guard<std::mutex> lock(clay_mutex_);
  auto itr = clay_creators_.find(name);
  if (itr == clay_creators_.end()) {
    return nullptr;
  }
  return itr->second(view_context_id_, task_runner_);
}
}  // namespace lynx
