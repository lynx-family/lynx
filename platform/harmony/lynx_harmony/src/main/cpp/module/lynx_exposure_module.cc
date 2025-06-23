// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/module/lynx_exposure_module.h"

#include <string>
#include <utility>

#include "base/include/log/logging.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace harmony {

const std::string LynxExposureModule::name_ = "LynxExposureModule";

LynxExposureModule::LynxExposureModule(
    const std::shared_ptr<tasm::harmony::LynxContext>& context)
    : NativeModuleCAPI(), context_(context) {
  RegisterMethod(
      piper::NativeModuleMethod("stopExposure", 1),
      reinterpret_cast<piper::LynxNativeModule::NativeModuleInvocation>(
          &LynxExposureModule::StopExposure));
  RegisterMethod(
      piper::NativeModuleMethod("resumeExposure", 0),
      reinterpret_cast<piper::LynxNativeModule::NativeModuleInvocation>(
          &LynxExposureModule::ResumeExposure));
  RegisterMethod(
      piper::NativeModuleMethod("setObserverFrameRate", 1),
      reinterpret_cast<piper::LynxNativeModule::NativeModuleInvocation>(
          &LynxExposureModule::SetObserverFrameRate));
}

void LynxExposureModule::Destroy() { LOGI("LynxExposureModule Destroy"); }

std::unique_ptr<pub::Value> LynxExposureModule::StopExposure(
    std::unique_ptr<pub::Value> args, const piper::CallbackMap& callbacks) {
  auto lepus_args = pub::ValueUtils::ConvertValueToLepusValue(*(args.get()));
  auto ctx = context_.lock();
  if (!ctx) {
    return std::unique_ptr<pub::Value>(nullptr);
  }
  ctx->StopExposure(lepus_args.Array()->get(0));
  return std::unique_ptr<pub::Value>(nullptr);
}

std::unique_ptr<pub::Value> LynxExposureModule::ResumeExposure(
    std::unique_ptr<pub::Value> args, const piper::CallbackMap& callbacks) {
  auto ctx = context_.lock();
  if (!ctx) {
    return std::unique_ptr<pub::Value>(nullptr);
  }
  ctx->ResumeExposure();
  return std::unique_ptr<pub::Value>(nullptr);
}

std::unique_ptr<pub::Value> LynxExposureModule::SetObserverFrameRate(
    std::unique_ptr<pub::Value> args, const piper::CallbackMap& callbacks) {
  auto lepus_args = pub::ValueUtils::ConvertValueToLepusValue(*(args.get()));
  auto ctx = context_.lock();
  if (!ctx) {
    return std::unique_ptr<pub::Value>(nullptr);
  }
  ctx->SetObserverFrameRate(lepus_args.Array()->get(0));
  return std::unique_ptr<pub::Value>(nullptr);
}

}  // namespace harmony
}  // namespace lynx
