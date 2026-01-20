// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/native_module/lynx_config_module.h"

#include <utility>

#include "clay/common/sys_info.h"
#include "clay/gfx/image/image_data_cache.h"
#include "clay/lynx_adaptor/clay_value.h"
#include "clay/ui/common/isolate.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/view_context.h"

namespace lynx {

const std::string LynxConfigModule::name_ = "LynxConfigModule";

LynxConfigModule::LynxConfigModule(uint32_t view_context_id,
                                   fml::RefPtr<fml::TaskRunner> task_runner)
    : LynxModuleBase(view_context_id, task_runner) {
  lynx::runtime::NativeModuleMethod set_dump_info("setDumpInfoToDevtoolEnabled",
                                                  1);
  RegisterMethod(set_dump_info, &LynxConfigModule::setDumpInfoToDevtoolEnabled);

  lynx::runtime::NativeModuleMethod set_render_options("setRenderOptions", 1);
  RegisterMethod(set_render_options, &LynxConfigModule::setRenderOptions);
}

LynxConfigModule::~LynxConfigModule() = default;

std::unique_ptr<lynx::pub::Value> LynxConfigModule::setDumpInfoToDevtoolEnabled(
    std::unique_ptr<lynx::pub::Value> args_array,
    const lynx::runtime::CallbackMap& callback_map) {
  fml::TaskRunner::RunNowOrPostTask(task_runner_, [weak_this = weak_from_this(),
                                                   args_array =
                                                       std::move(args_array),
                                                   callback_map]() {
    auto strong_this = weak_this.lock();
    if (!strong_this) {
      return;
    }
    auto view_context = clay::Isolate::Instance().GetViewContextById(
        strong_this->view_context_id_);
    if (!view_context) {
      FML_DLOG(ERROR) << "setDumpInfoToDevtoolEnabled failed, view context has "
                         "been destroyed.";
      return;
    }
    static_cast<clay::PageView*>(view_context->GetPageView())
        ->DumpInfoToDevtoolEnabled(args_array->GetValueAtIndex(0)->Bool());
  });
  return std::make_unique<lynx::ClayValue>(clay::Value(true));
}

std::unique_ptr<lynx::pub::Value> LynxConfigModule::setRenderOptions(
    std::unique_ptr<lynx::pub::Value> args_array,
    const lynx::runtime::CallbackMap& callback_map) {
  fml::TaskRunner::RunNowOrPostTask(task_runner_, [weak_this = weak_from_this(),
                                                   args_array =
                                                       std::move(args_array),
                                                   callback_map]() {
    auto strong_this = weak_this.lock();
    if (!strong_this) {
      return;
    }
    auto view_context = clay::Isolate::Instance().GetViewContextById(
        strong_this->view_context_id_);
    if (!view_context) {
      FML_DLOG(ERROR) << "setRenderOptions failed, view context has "
                         "been destroyed.";
      return;
    }

    auto args = args_array->GetValueAtIndex(0);
    if (args->Contains("imageRawDataCache")) {
      auto raw_data_cache = args->GetValueForKey("imageRawDataCache")->Number();
      FML_DLOG(WARNING) << "setRenderOptions raw_data_cache=" << raw_data_cache;
      if (raw_data_cache > 0) {
        clay::ImageDataCache::GetInstance().SetMaxCachedBytes(raw_data_cache);
      }
    }
    if (args->Contains("isLowDevice")) {
      auto is_low_device = args->GetValueForKey("isLowDevice")->Bool();
      FML_DLOG(WARNING) << "setRenderOptions is_low_device=" << is_low_device;
      clay::SysInfo::SetCustomIsLowEndDevice(is_low_device);
    }
    if (args->Contains("ignoreRasterCache")) {
      auto ignore_raster_cache =
          args->GetValueForKey("ignoreRasterCache")->Bool();
      if (view_context) {
        auto render_settings =
            static_cast<clay::PageView*>(view_context->GetPageView())
                ->GetRenderSettings();
        if (render_settings) {
          FML_DLOG(WARNING)
              << "setRenderOptions ignore_raster_cache=" << ignore_raster_cache;
          render_settings->SetIgnoreRasterCache(ignore_raster_cache);
        }
      }
    }
  });
  return std::make_unique<lynx::ClayValue>(clay::Value(true));
}

}  // namespace lynx
