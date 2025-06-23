// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/module/lynx_intersection_observer_module.h"

#include <string>
#include <utility>

#include "base/include/log/logging.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace harmony {

const std::string LynxIntersectionObserverModule::name_ =
    "IntersectionObserverModule";

LynxIntersectionObserverModule::LynxIntersectionObserverModule(
    const std::shared_ptr<tasm::harmony::LynxContext>& context)
    : NativeModuleCAPI(), context_(context) {
  RegisterMethod(
      piper::NativeModuleMethod("createIntersectionObserver", 3),
      reinterpret_cast<piper::LynxNativeModule::NativeModuleInvocation>(
          &LynxIntersectionObserverModule::CreateIntersectionObserver));
  RegisterMethod(
      piper::NativeModuleMethod("relativeTo", 3),
      reinterpret_cast<piper::LynxNativeModule::NativeModuleInvocation>(
          &LynxIntersectionObserverModule::RelativeTo));
  RegisterMethod(
      piper::NativeModuleMethod("relativeToViewport", 2),
      reinterpret_cast<piper::LynxNativeModule::NativeModuleInvocation>(
          &LynxIntersectionObserverModule::RelativeToViewport));
  RegisterMethod(
      piper::NativeModuleMethod("relativeToScreen", 2),
      reinterpret_cast<piper::LynxNativeModule::NativeModuleInvocation>(
          &LynxIntersectionObserverModule::RelativeToScreen));
  RegisterMethod(
      piper::NativeModuleMethod("observe", 3),
      reinterpret_cast<piper::LynxNativeModule::NativeModuleInvocation>(
          &LynxIntersectionObserverModule::Observe));
  RegisterMethod(
      piper::NativeModuleMethod("disconnect", 1),
      reinterpret_cast<piper::LynxNativeModule::NativeModuleInvocation>(
          &LynxIntersectionObserverModule::Disconnect));
}

void LynxIntersectionObserverModule::Destroy() {
  LOGI("LynxIntersectionObserverModule Destroy");
}

std::unique_ptr<pub::Value>
LynxIntersectionObserverModule::CreateIntersectionObserver(
    std::unique_ptr<pub::Value> args, const piper::CallbackMap& callbacks) {
  auto lepus_args = pub::ValueUtils::ConvertValueToLepusValue(*(args.get()));
  auto ctx = context_.lock();
  if (!ctx) {
    return std::unique_ptr<pub::Value>(nullptr);
  }
  auto lepus_array = lepus_args.Array();
  ctx->CreateUIIntersectionObserver(lepus_array->get(0).Number(),
                                    lepus_array->get(1).StdString(),
                                    lepus_array->get(2));
  return std::unique_ptr<pub::Value>(nullptr);
}

std::unique_ptr<pub::Value> LynxIntersectionObserverModule::RelativeTo(
    std::unique_ptr<pub::Value> args, const piper::CallbackMap& callbacks) {
  auto lepus_args = pub::ValueUtils::ConvertValueToLepusValue(*(args.get()));
  auto ctx = context_.lock();
  if (!ctx) {
    return std::unique_ptr<pub::Value>(nullptr);
  }
  auto lepus_array = lepus_args.Array();
  auto intersection_observer =
      ctx->GetUIIntersectionObserver(lepus_array->get(0).Number());
  if (intersection_observer) {
    intersection_observer->RelativeTo(lepus_array->get(1).StdString(),
                                      lepus_array->get(2));
  }
  return std::unique_ptr<pub::Value>(nullptr);
}

std::unique_ptr<pub::Value> LynxIntersectionObserverModule::RelativeToViewport(
    std::unique_ptr<pub::Value> args, const piper::CallbackMap& callbacks) {
  auto lepus_args = pub::ValueUtils::ConvertValueToLepusValue(*(args.get()));
  auto ctx = context_.lock();
  if (!ctx) {
    return std::unique_ptr<pub::Value>(nullptr);
  }
  auto lepus_array = lepus_args.Array();
  auto intersection_observer =
      ctx->GetUIIntersectionObserver(lepus_array->get(0).Number());
  if (intersection_observer) {
    intersection_observer->RelativeToViewport(lepus_array->get(1));
  }
  return std::unique_ptr<pub::Value>(nullptr);
}

std::unique_ptr<pub::Value> LynxIntersectionObserverModule::RelativeToScreen(
    std::unique_ptr<pub::Value> args, const piper::CallbackMap& callbacks) {
  auto lepus_args = pub::ValueUtils::ConvertValueToLepusValue(*(args.get()));
  auto ctx = context_.lock();
  if (!ctx) {
    return std::unique_ptr<pub::Value>(nullptr);
  }
  auto lepus_array = lepus_args.Array();
  auto intersection_observer =
      ctx->GetUIIntersectionObserver(lepus_array->get(0).Number());
  if (intersection_observer) {
    intersection_observer->RelativeToScreen(lepus_array->get(1));
  }
  return std::unique_ptr<pub::Value>(nullptr);
}

std::unique_ptr<pub::Value> LynxIntersectionObserverModule::Observe(
    std::unique_ptr<pub::Value> args, const piper::CallbackMap& callbacks) {
  auto lepus_args = pub::ValueUtils::ConvertValueToLepusValue(*(args.get()));
  auto ctx = context_.lock();
  if (!ctx) {
    return std::unique_ptr<pub::Value>(nullptr);
  }
  auto lepus_array = lepus_args.Array();
  auto intersection_observer =
      ctx->GetUIIntersectionObserver(lepus_array->get(0).Number());
  if (intersection_observer) {
    intersection_observer->Observe(lepus_array->get(1).StdString(),
                                   lepus_array->get(2).Number());
  }
  return std::unique_ptr<pub::Value>(nullptr);
}

std::unique_ptr<pub::Value> LynxIntersectionObserverModule::Disconnect(
    std::unique_ptr<pub::Value> args, const piper::CallbackMap& callbacks) {
  auto lepus_args = pub::ValueUtils::ConvertValueToLepusValue(*(args.get()));
  auto ctx = context_.lock();
  if (!ctx) {
    return std::unique_ptr<pub::Value>(nullptr);
  }
  auto lepus_array = lepus_args.Array();
  auto intersection_observer =
      ctx->GetUIIntersectionObserver(lepus_array->get(0).Number());
  if (intersection_observer) {
    intersection_observer->Disconnect();
  }
  return std::unique_ptr<pub::Value>(nullptr);
}

}  // namespace harmony
}  // namespace lynx
