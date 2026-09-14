// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/harmony/native_painting_context_platform_harmony_ref.h"

#include <utility>

#include "core/value_wrapper/value_impl_lepus.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_renderer_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_root.h"

namespace lynx::tasm {

NativePaintingCtxPlatformHarmonyRef::NativePaintingCtxPlatformHarmonyRef(
    std::unique_ptr<PlatformRendererFactory> renderer_factory,
    std::weak_ptr<harmony::LynxRendererContext> renderer_context)
    : NativePaintingCtxPlatformRef(std::move(renderer_factory)),
      renderer_context_(std::move(renderer_context)) {}

NativePaintingCtxPlatformHarmonyRef::~NativePaintingCtxPlatformHarmonyRef() {
  Destroy();
}

void NativePaintingCtxPlatformHarmonyRef::SetNeedMarkPaintEndTiming(
    const tasm::PipelineID& pipeline_id) {
  if (auto renderer_context = renderer_context_.lock()) {
    auto* ui_owner = renderer_context->GetUIOwner();
    if (ui_owner != nullptr) {
      ui_owner->PostDrawEndTimingFrameCallback(pipeline_id);
    }
  }
}

std::vector<float> NativePaintingCtxPlatformHarmonyRef::GetTransformValue(
    int32_t sign, const std::vector<float>& offsets) {
  return GetTransformValueForEventTarget(sign, offsets);
}

void NativePaintingCtxPlatformHarmonyRef::GetRootViewLocationOnScreen(
    float location[2]) {
  location[0] = 0.f;
  location[1] = 0.f;
  auto renderer_context = renderer_context_.lock();
  auto* ui_owner = renderer_context ? renderer_context->GetUIOwner() : nullptr;
  if (ui_owner == nullptr || ui_owner->Destroyed()) {
    return;
  }
  if (auto* root = ui_owner->Root()) {
    root->GetOffsetToScreen(location);
  }
}

void NativePaintingCtxPlatformHarmonyRef::GetScreenSize(float size[2]) {
  if (size == nullptr) {
    return;
  }
  size[0] = 0.f;
  size[1] = 0.f;
  auto renderer_context = renderer_context_.lock();
  auto lynx_context =
      renderer_context ? renderer_context->GetLynxContext() : nullptr;
  if (lynx_context != nullptr) {
    lynx_context->ScreenSize(size);
  }
}

void NativePaintingCtxPlatformHarmonyRef::GetPlatformRendererScrollOffset(
    int32_t sign, float offset[2]) {
  offset[0] = 0.f;
  offset[1] = 0.f;
  auto renderer_context = renderer_context_.lock();
  auto* ui_owner = renderer_context ? renderer_context->GetUIOwner() : nullptr;
  if (ui_owner == nullptr || ui_owner->Destroyed()) {
    return;
  }
  if (auto* ui = ui_owner->FindUIBySign(sign)) {
    offset[0] = ui->ScrollX();
    offset[1] = ui->ScrollY();
  }
}

bool NativePaintingCtxPlatformHarmonyRef::IsPlatformRendererScrollable(
    int32_t sign) {
  auto renderer_context = renderer_context_.lock();
  auto* ui_owner = renderer_context ? renderer_context->GetUIOwner() : nullptr;
  if (ui_owner == nullptr || ui_owner->Destroyed()) {
    return false;
  }
  auto* ui = ui_owner->FindUIBySign(sign);
  return ui != nullptr && ui->IsScrollable();
}

void NativePaintingCtxPlatformHarmonyRef::InvokePlatformViewUIMethod(
    int32_t id, const std::string& method, const lepus::Value& params,
    base::MoveOnlyClosure<void, int32_t, const pub::Value&> callback) {
  auto renderer_context = renderer_context_.lock();
  auto* ui_owner = renderer_context ? renderer_context->GetUIOwner() : nullptr;
  if (ui_owner == nullptr || ui_owner->Destroyed()) {
    NativePaintingCtxPlatformRef::InvokePlatformViewUIMethod(
        id, method, params, std::move(callback));
    return;
  }
  base::MoveOnlyClosure<void, int32_t, const lepus::Value&> cb =
      [callback = std::move(callback)](int32_t code,
                                       const lepus::Value& data) mutable {
        if (!callback) {
          return;
        }
        auto invoke_callback = std::move(callback);
        invoke_callback(code, PubLepusValue(data));
      };
  ui_owner->InvokeUIMethod(id, method, params, std::move(cb));
}

void NativePaintingCtxPlatformHarmonyRef::NotifyNodeReady(
    const std::vector<int32_t>& signs) {
  auto renderer_context = renderer_context_.lock();
  auto* ui_owner = renderer_context ? renderer_context->GetUIOwner() : nullptr;
  if (ui_owner == nullptr || ui_owner->Destroyed()) {
    return;
  }
  for (const auto sign : signs) {
    ui_owner->OnNodeReady(sign);
  }
}

void NativePaintingCtxPlatformHarmonyRef::DestroyImageOnPlatformThread(
    int32_t image_key) {
  if (auto renderer_context = renderer_context_.lock()) {
    renderer_context->DestroyImageManager(image_key);
  }
}

}  // namespace lynx::tasm
