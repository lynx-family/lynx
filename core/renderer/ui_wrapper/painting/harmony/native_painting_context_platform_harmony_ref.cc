// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/harmony/native_painting_context_platform_harmony_ref.h"

#include <utility>

#include "core/value_wrapper/value_impl_lepus.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_renderer_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/paragraph_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_list.h"
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

bool NativePaintingCtxPlatformHarmonyRef::RequestRootFocus(int32_t root_id,
                                                           bool focused) {
  auto renderer_context = renderer_context_.lock();
  auto* owner = renderer_context ? renderer_context->GetUIOwner() : nullptr;
  return owner && owner->RequestRootFocus(root_id, focused);
}

void NativePaintingCtxPlatformHarmonyRef::ApplyListScrollState(
    int32_t sign, float scroll_x, float scroll_y, float content_width,
    float content_height, bool cmp_managed) {
  auto context = renderer_context_.lock();
  auto* owner = context ? context->GetUIOwner() : nullptr;
  if (!owner || owner->Destroyed()) return;
  auto* ui = owner->FindUIBySign(sign);
  if (ui && ui->IsList()) {
    static_cast<harmony::UIList*>(ui)->ApplyListScrollState(
        scroll_x, scroll_y, content_width, content_height, cmp_managed);
  }
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

void NativePaintingCtxPlatformHarmonyRef::InsertListItemPaintingNode(
    int32_t list_id, int32_t child_id) {
  auto renderer_context = renderer_context_.lock();
  auto* ui_owner = renderer_context ? renderer_context->GetUIOwner() : nullptr;
  if (ui_owner == nullptr || ui_owner->Destroyed()) {
    return;
  }
  ui_owner->InsertListItemPaintingNode(list_id, child_id);
}

void NativePaintingCtxPlatformHarmonyRef::RemoveListItemPaintingNode(
    int32_t list_id, int32_t child_id) {
  auto renderer_context = renderer_context_.lock();
  auto* ui_owner = renderer_context ? renderer_context->GetUIOwner() : nullptr;
  if (ui_owner == nullptr || ui_owner->Destroyed()) {
    return;
  }
  ui_owner->RemoveListItemPaintingNode(list_id, child_id);
}

void NativePaintingCtxPlatformHarmonyRef::UpdateContentOffsetForListContainer(
    int32_t container_id, float content_size, float delta_x, float delta_y,
    bool is_init_scroll_offset, bool from_layout) {
  auto renderer_context = renderer_context_.lock();
  auto* ui_owner = renderer_context ? renderer_context->GetUIOwner() : nullptr;
  if (ui_owner == nullptr || ui_owner->Destroyed()) {
    return;
  }
  ui_owner->UpdateContentOffsetForListContainer(
      container_id, content_size, delta_x, delta_y, is_init_scroll_offset,
      from_layout);
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

bool NativePaintingCtxPlatformHarmonyRef::GetPlatformRendererPosition(
    int32_t sign, float position[2]) {
  auto renderer_context = renderer_context_.lock();
  auto* ui_owner = renderer_context ? renderer_context->GetUIOwner() : nullptr;
  if (ui_owner == nullptr || ui_owner->Destroyed()) {
    return false;
  }
  auto* ui = ui_owner->FindUIBySign(sign);
  if (ui == nullptr) {
    return false;
  }
  auto* parent = ui->Parent();
  if (parent == nullptr) {
    return false;
  }
  if (parent->Tag() == "refresh" || parent->Tag() == "x-refresh-view") {
    const float density = ui->GetContext()->ScaledDensity();
    ArkUI_IntOffset child_offset{0, 0};
    ArkUI_IntOffset parent_offset{0, 0};
    if (density <= 0.f ||
        OH_ArkUI_NodeUtils_GetPositionWithTranslateInScreen(
            ui->Node(), &child_offset) != ARKUI_ERROR_CODE_NO_ERROR ||
        OH_ArkUI_NodeUtils_GetPositionWithTranslateInScreen(
            parent->Node(), &parent_offset) != ARKUI_ERROR_CODE_NO_ERROR) {
      return false;
    }
    // Refresh places the header in a native slot and the content in an inner
    // stack. Include those native ancestors, then restore content coordinates
    // because CUI applies the parent's pull-down offset separately.
    float transformed_origin[2] = {0.f, 0.f};
    if (auto* transform = ui->GetTransform()) {
      transform->GetTransformMatrix(ui->width_, ui->height_, 1.f, true)
          .mapPoint(transformed_origin, transformed_origin);
    }
    position[0] = (child_offset.x - parent_offset.x) / density +
                  parent->ScrollX() - transformed_origin[0] +
                  ui->OffsetXForCalcPosition();
    position[1] = (child_offset.y - parent_offset.y) / density +
                  parent->ScrollY() - transformed_origin[1] +
                  ui->OffsetYForCalcPosition();
    return true;
  }
  if (parent->Tag() == "scroll-coordinator" ||
      parent->Tag() == "x-foldview-ng") {
    // The coordinator adjusts header and slot positions after Lynx layout.
    // Reuse the component's native hit-test geometry for content coordinates.
    float rect[4];
    ui->GetOriginRect(rect);
    position[0] = rect[0];
    position[1] = rect[1];
    if (ui->Tag() == "scroll-coordinator-toolbar" ||
        ui->Tag() == "x-foldview-toolbar-ng") {
      // The toolbar is fixed outside the scrolling content. Cancel the event
      // tree's parent scroll compensation to keep its hit region fixed.
      position[0] += parent->ScrollX();
      position[1] += parent->ScrollY();
    }
    return true;
  }
  const bool is_pager_item =
      (ui->Tag() == "viewpager-item" || ui->Tag() == "x-viewpager-item-ng") &&
      (parent->Tag() == "viewpager" || parent->Tag() == "x-viewpager-ng");
  const bool is_swiper_item =
      (ui->Tag() == "swiper-item" || ui->Tag() == "x-swiper-item") &&
      parent->Tag() == "x-swiper";
  if (!is_pager_item && !is_swiper_item) {
    return false;
  }
  position[0] = ui->ViewLeft();
  position[1] = ui->ViewTop();
  return true;
}

PlatformTextEventTargetRegions
NativePaintingCtxPlatformHarmonyRef::GetTextEventTargetRegions(
    int32_t text_id) {
  PlatformTextEventTargetRegions regions;
  const auto* ranges = GetTextEventTargetRanges(text_id);
  auto renderer_context = renderer_context_.lock();
  auto context =
      renderer_context ? renderer_context->GetLynxContext() : nullptr;
  auto paragraph =
      renderer_context ? renderer_context->GetTextBundle(text_id) : nullptr;
  if (ranges == nullptr || context == nullptr || paragraph == nullptr) {
    return regions;
  }
  const float density = context->ScaledDensity();
  if (density <= 0.f) {
    return regions;
  }
  for (const auto& range : *ranges) {
    if (range.start < 0 || range.end <= range.start) {
      continue;
    }
    auto rects = paragraph->GetRectsForRange(range.start, range.end);
    for (uint32_t i = 0; i < rects.GetCount(); ++i) {
      const float width = rects.GetRight(i) - rects.GetLeft(i);
      const float height = rects.GetBottom(i) - rects.GetTop(i);
      if (width <= 0.f || height <= 0.f) {
        continue;
      }
      // Typography uses physical pixels; event targets use logical units.
      // The text target already accounts for the text content's origin.
      regions.push_back(PlatformTextEventTargetRegion{
          range.sign,
          (rects.GetLeft(i) + paragraph->GetTranslateLeftOffset()) / density,
          rects.GetTop(i) / density, width / density, height / density});
    }
  }
  return regions;
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
