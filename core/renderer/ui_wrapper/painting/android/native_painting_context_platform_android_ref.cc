// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/android/native_painting_context_platform_android_ref.h"

#include <string>
#include <utility>

namespace lynx {
namespace tasm {

NativePaintingCtxAndroidRef::NativePaintingCtxAndroidRef(
    std::unique_ptr<PlatformRendererFactory> view_factory)
    : NativePaintingCtxPlatformRef(std::move(view_factory)) {}

void NativePaintingCtxAndroidRef::GetRootViewLocationOnScreen(
    float location[2]) {
  if (location == nullptr) {
    return;
  }
  location[0] = 0.f;
  location[1] = 0.f;

  auto* factory =
      static_cast<PlatformRendererAndroidFactory*>(view_factory_.get());
  if (factory == nullptr) {
    return;
  }
  auto* context = factory->GetContext();
  if (context == nullptr) {
    return;
  }
  const auto res = context->GetRootViewLocationOnScreen();
  if (res.size() >= 2) {
    location[0] = res[0];
    location[1] = res[1];
  }
}

void NativePaintingCtxAndroidRef::GetScreenSize(float size[2]) {
  if (size == nullptr) {
    return;
  }
  size[0] = 0.f;
  size[1] = 0.f;

  auto* factory =
      static_cast<PlatformRendererAndroidFactory*>(view_factory_.get());
  if (factory == nullptr) {
    return;
  }
  auto* context = factory->GetContext();
  if (context == nullptr) {
    return;
  }
  const auto res = context->GetScreenSize();
  if (res.size() >= 2) {
    size[0] = res[0];
    size[1] = res[1];
  }
}

void NativePaintingCtxAndroidRef::GetPlatformRendererScrollOffset(
    int32_t sign, float offset[2]) {
  if (offset == nullptr) {
    return;
  }
  offset[0] = 0.f;
  offset[1] = 0.f;

  auto* factory =
      static_cast<PlatformRendererAndroidFactory*>(view_factory_.get());
  if (factory == nullptr) {
    return;
  }
  auto* context = factory->GetContext();
  if (context == nullptr) {
    return;
  }
  const auto res = context->GetRendererHostScrollOffset(sign);
  if (res.size() >= 2) {
    offset[0] = res[0];
    offset[1] = res[1];
  }
}

bool NativePaintingCtxAndroidRef::IsPlatformRendererScrollable(int32_t sign) {
  auto* factory =
      static_cast<PlatformRendererAndroidFactory*>(view_factory_.get());
  if (factory == nullptr) {
    return false;
  }
  auto* context = factory->GetContext();
  if (context == nullptr) {
    return false;
  }
  return context->IsRendererHostScrollable(sign);
}

void NativePaintingCtxAndroidRef::InvokePlatformRendererUIMethod(
    int32_t id, const std::string& method, const lepus::Value& params,
    base::MoveOnlyClosure<void, int32_t, const pub::Value&> callback) {
  auto* factory =
      static_cast<PlatformRendererAndroidFactory*>(view_factory_.get());
  if (factory == nullptr) {
    NativePaintingCtxPlatformRef::InvokePlatformRendererUIMethod(
        id, method, params, std::move(callback));
    return;
  }
  auto* context = factory->GetContext();
  if (context == nullptr) {
    NativePaintingCtxPlatformRef::InvokePlatformRendererUIMethod(
        id, method, params, std::move(callback));
    return;
  }
  context->InvokeUIMethod(id, method, params, std::move(callback));
}

}  // namespace tasm
}  // namespace lynx
