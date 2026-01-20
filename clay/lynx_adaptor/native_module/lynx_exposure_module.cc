
// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/native_module/lynx_exposure_module.h"

#include <utility>

#include "clay/lynx_adaptor/clay_value.h"
#include "clay/ui/common/isolate.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/view_context.h"

namespace lynx {

const std::string LynxExposureModule::name_ = "LynxExposureModule";

LynxExposureModule::LynxExposureModule(uint32_t view_context_id,
                                       fml::RefPtr<fml::TaskRunner> task_runner)
    : LynxModuleBase(view_context_id, task_runner) {
  lynx::runtime::NativeModuleMethod stop_exposure("stopExposure", 1);
  RegisterMethod(stop_exposure, &LynxExposureModule::stopExposure);

  lynx::runtime::NativeModuleMethod resume_exposure("resumeExposure", 0);
  RegisterMethod(resume_exposure, &LynxExposureModule::resumeExposure);
}
LynxExposureModule::~LynxExposureModule() = default;

std::unique_ptr<lynx::pub::Value> LynxExposureModule::stopExposure(
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
      FML_DLOG(ERROR) << "stopExposure failed, view context has "
                         "been destroyed.";
      return;
    }
    bool send_event = args_array->GetValueAtIndex(0)->Bool();
    auto intersection_manager =
        view_context->GetPageView()->intersection_observer_manager();
    if (!intersection_manager) {
      FML_DLOG(ERROR)
          << "stopExposure failed, intersection_observer_manager is nullptr";
      return;
    }
    intersection_manager->StopExposure(send_event);
  });

  return std::make_unique<lynx::ClayValue>(clay::Value(true));
}
std::unique_ptr<lynx::pub::Value> LynxExposureModule::resumeExposure(
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
      FML_DLOG(ERROR) << "resumeExposure failed, view context has "
                         "been destroyed.";
      return;
    }
    auto intersection_manager =
        view_context->GetPageView()->intersection_observer_manager();
    if (!intersection_manager) {
      FML_DLOG(ERROR)
          << "resumeExposure failed, intersection_observer_manager is nullptr";
      return;
    }
    intersection_manager->ResumeExposure();
  });

  return std::make_unique<lynx::ClayValue>(clay::Value(true));
}

}  // namespace lynx
