// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/harmony/native_painting_context_harmony.h"

#include <utility>

#include "core/renderer/ui_wrapper/layout/harmony/text_layout_harmony.h"
#include "core/renderer/ui_wrapper/layout/harmony/text_measurer_harmony.h"
#include "core/renderer/ui_wrapper/painting/harmony/native_painting_context_platform_harmony_ref.h"
#include "core/renderer/ui_wrapper/painting/harmony/platform_renderer_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_renderer_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/paragraph_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"

namespace lynx {
namespace tasm {

NativePaintingCtxHarmony::NativePaintingCtxHarmony(
    const std::shared_ptr<harmony::LynxContext>& context) {
  text_measurer_ = std::make_unique<TextMeasurerHarmony>(context.get());
  text_layout_impl_ = std::make_unique<TextLayoutHarmony>(text_measurer_.get());
  renderer_context_ = std::make_shared<harmony::LynxRendererContext>(context);
  platform_ref_ = std::make_shared<NativePaintingCtxPlatformHarmonyRef>(
      std::make_unique<PlatformRendererHarmonyFactory>(renderer_context_));
}

NativePaintingCtxHarmony::~NativePaintingCtxHarmony() {
  if (auto ref = std::static_pointer_cast<NativePaintingCtxPlatformRef>(
          platform_ref_)) {
    ref->Destroy();
  }
  platform_ref_.reset();
  renderer_context_.reset();
}

void NativePaintingCtxHarmony::SetUIOperationQueue(
    const std::shared_ptr<shell::UIOperationQueueInterface>& queue) {
  queue_ = std::static_pointer_cast<shell::DynamicUIOperationQueue>(queue);
}

void NativePaintingCtxHarmony::Flush() {
  if (queue_ != nullptr) {
    queue_->Flush();
  }
}

void NativePaintingCtxHarmony::FinishTasmOperation(
    const std::shared_ptr<PipelineOptions>& options) {
  if (queue_ != nullptr && options->native_update_data_order_ ==
                               queue_->GetNativeUpdateDataOrder()) {
    queue_->UpdateStatus(shell::UIOperationStatus::TASM_FINISH);
  }
}

void NativePaintingCtxHarmony::FinishLayoutOperation(
    const std::shared_ptr<PipelineOptions>& options) {
  if (queue_ == nullptr) {
    return;
  }
  Enqueue([renderer_context = renderer_context_,
           weak_queue = std::weak_ptr<shell::DynamicUIOperationQueue>(queue_),
           options]() {
    auto* ui_owner = renderer_context->GetUIOwner();
    if (ui_owner != nullptr && !ui_owner->Destroyed()) {
      ui_owner->OnLayoutFinish(options->list_comp_id_, options->operation_id);
    }
    if (auto queue = weak_queue.lock();
        queue != nullptr && options->native_update_data_order_ ==
                                queue->GetNativeUpdateDataOrder()) {
      queue->UpdateStatus(shell::UIOperationStatus::ALL_FINISH);
    }
  });
  if (options->native_update_data_order_ ==
      queue_->GetNativeUpdateDataOrder()) {
    queue_->UpdateStatus(shell::UIOperationStatus::LAYOUT_FINISH);
  }
}

std::unique_ptr<pub::Value> NativePaintingCtxHarmony::GetTextInfo(
    const std::string& content, const pub::Value& info) {
  return nullptr;
}

std::vector<float> NativePaintingCtxHarmony::getBoundingClientOrigin(int id) {
  return {};
}

std::vector<float> NativePaintingCtxHarmony::getWindowSize(int id) {
  return {};
}

std::vector<float> NativePaintingCtxHarmony::GetRectToWindow(int id) {
  return {};
}

std::vector<float> NativePaintingCtxHarmony::GetRectToLynxView(int64_t id) {
  return {};
}

std::vector<float> NativePaintingCtxHarmony::ScrollBy(int64_t id, float width,
                                                      float height) {
  return {};
}

void NativePaintingCtxHarmony::Invoke(
    int64_t id, const std::string& method, const pub::Value& params,
    const std::function<void(int32_t, const pub::Value&)>& callback) {}

void NativePaintingCtxHarmony::EnqueueInvoke(
    int64_t id, const std::string& method, const pub::Value& params,
    const std::function<void(int32_t, const pub::Value&)>& callback) {}

int32_t NativePaintingCtxHarmony::GetTagInfo(const std::string& tag_name) {
  auto* ui_owner = renderer_context_->GetUIOwner();
  return ui_owner != nullptr ? ui_owner->GetTagInfo(tag_name) : 0;
}

bool NativePaintingCtxHarmony::IsFlatten(
    base::MoveOnlyClosure<bool, bool> func) {
  return false;
}

bool NativePaintingCtxHarmony::NeedAnimationProps() { return false; }

void NativePaintingCtxHarmony::CreatePlatformRenderer(
    int id, PlatformRendererType type, const fml::RefPtr<PropBundle>& init_data,
    const PlatformRendererInitConfig& init_config) {
  Enqueue([platform_ref = platform_ref_, id, type, init_data, init_config]() {
    auto ref = std::static_pointer_cast<NativePaintingCtxPlatformHarmonyRef>(
        platform_ref);
    ref->CreatePlatformRenderer(id, type, init_data, init_config);
  });
}

void NativePaintingCtxHarmony::CreatePlatformExtendedRenderer(
    int id, const base::String& tag_name,
    const fml::RefPtr<PropBundle>& init_data,
    const PlatformRendererInitConfig& init_config) {}

void NativePaintingCtxHarmony::EnqueueDisplayList(int id, DisplayList list) {
  Enqueue([platform_ref = platform_ref_, id, list = std::move(list)]() mutable {
    auto ref = std::static_pointer_cast<NativePaintingCtxPlatformHarmonyRef>(
        platform_ref);
    ref->UpdateDisplayList(id, std::move(list));
  });
}

void NativePaintingCtxHarmony::EnqueueDisplayLists(
    DisplayListUpdateBatch batch) {
  Enqueue([platform_ref = platform_ref_, batch = std::move(batch)]() mutable {
    auto ref = std::static_pointer_cast<NativePaintingCtxPlatformHarmonyRef>(
        platform_ref);
    ref->UpdateDisplayLists(std::move(batch));
  });
}

void NativePaintingCtxHarmony::UpdateTextBundle(int id, intptr_t bundle) {
  auto* paragraph = reinterpret_cast<harmony::ParagraphHarmony*>(bundle);
  if (paragraph == nullptr) {
    return;
  }
  fml::RefPtr<harmony::ParagraphHarmony> text_bundle(paragraph);
  Enqueue([renderer_context = renderer_context_, id,
           text_bundle = std::move(text_bundle)]() mutable {
    renderer_context->UpdateTextBundle(id, std::move(text_bundle));
  });
}

void NativePaintingCtxHarmony::DestroyTextBundle(int id) {
  Enqueue([renderer_context = renderer_context_, id]() {
    renderer_context->DestroyTextBundle(id);
  });
}

fml::RefPtr<PaintImage> NativePaintingCtxHarmony::CreateImage(
    int id, base::String src, const ImagePaintInfo& paint_info, float width,
    float height, int32_t event_mask, bool disable_default_resize) {
  return nullptr;
}

void NativePaintingCtxHarmony::UpdatePlatformEventBundle(
    int id, PlatformEventBundle bundle) {}

void NativePaintingCtxHarmony::Enqueue(shell::UIOperation operation) {
  if (queue_ != nullptr) {
    queue_->EnqueueUIOperation(std::move(operation));
  }
}

}  // namespace tasm
}  // namespace lynx
