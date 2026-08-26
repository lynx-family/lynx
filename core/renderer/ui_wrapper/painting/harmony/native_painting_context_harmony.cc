// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/harmony/native_painting_context_harmony.h"

#include <memory>
#include <utility>

#include "core/renderer/ui_wrapper/layout/harmony/text_layout_harmony.h"
#include "core/renderer/ui_wrapper/layout/harmony/text_measurer_harmony.h"
#include "core/renderer/ui_wrapper/painting/harmony/native_painting_context_platform_harmony_ref.h"
#include "core/renderer/ui_wrapper/painting/harmony/platform_renderer_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/shadow_node_owner.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/paragraph_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"

namespace lynx {
namespace tasm {

NativePaintingCtxHarmony::NativePaintingCtxHarmony(
    harmony::UIOwner* ui_owner,
    const std::shared_ptr<harmony::LynxContext>& lynx_context)
    : ui_owner_(ui_owner),
      text_measurer_(
          std::make_unique<TextMeasurerHarmony>(ui_owner_->Context())),
      renderer_context_(
          std::make_shared<harmony::LynxRendererContext>(lynx_context)) {
  text_layout_impl_ = std::make_unique<TextLayoutHarmony>(text_measurer_.get());
  platform_ref_ = std::make_shared<NativePaintingCtxPlatformHarmonyRef>(
      std::make_unique<PlatformRendererHarmonyFactory>(renderer_context_));
}

NativePaintingCtxHarmony::~NativePaintingCtxHarmony() {
  text_layout_impl_.reset();
  if (auto ref = std::static_pointer_cast<NativePaintingCtxPlatformRef>(
          platform_ref_)) {
    ref->Destroy();
  }
  platform_ref_.reset();
  renderer_context_.reset();
}

void NativePaintingCtxHarmony::Flush() {
  if (queue_) {
    queue_->Flush();
  }
}

void NativePaintingCtxHarmony::FinishTasmOperation(
    const std::shared_ptr<PipelineOptions>& options) {
  if (queue_ != nullptr && options != nullptr &&
      options->native_update_data_order_ ==
          queue_->GetNativeUpdateDataOrder()) {
    queue_->UpdateStatus(shell::UIOperationStatus::TASM_FINISH);
  }
}

void NativePaintingCtxHarmony::FinishLayoutOperation(
    const std::shared_ptr<PipelineOptions>& options) {
  if (options == nullptr || !has_first_screen_) {
    return;
  }
  Enqueue([renderer_context = renderer_context_, options]() {
    if (renderer_context == nullptr) {
      return;
    }
    auto lynx_context = renderer_context->GetLynxContext();
    auto* ui_owner =
        lynx_context != nullptr ? lynx_context->GetUIOwner() : nullptr;
    if (ui_owner == nullptr || ui_owner->Destroyed()) {
      return;
    }
    ui_owner->OnLayoutFinish(options->list_comp_id_, options->operation_id);
    if (options->need_timestamps && !options->pipeline_id.empty()) {
      ui_owner->PostDrawEndTimingFrameCallback(options->pipeline_id);
    }
  });
  if (queue_ == nullptr) {
    return;
  }
  Enqueue([weak_queue = std::weak_ptr<shell::DynamicUIOperationQueue>(queue_),
           native_update_data_order = options->native_update_data_order_]() {
    if (auto queue = weak_queue.lock();
        queue != nullptr &&
        native_update_data_order == queue->GetNativeUpdateDataOrder()) {
      queue->UpdateStatus(shell::UIOperationStatus::ALL_FINISH);
    }
  });
  if (options->native_update_data_order_ ==
      queue_->GetNativeUpdateDataOrder()) {
    queue_->UpdateStatus(shell::UIOperationStatus::LAYOUT_FINISH);
  }
}

void NativePaintingCtxHarmony::SetUIOperationQueue(
    const std::shared_ptr<shell::UIOperationQueueInterface>& queue) {
  queue_ = std::static_pointer_cast<shell::DynamicUIOperationQueue>(queue);
}

void NativePaintingCtxHarmony::OnFirstScreen() { has_first_screen_ = true; }

int32_t NativePaintingCtxHarmony::GetTagInfo(const std::string& tag_name) {
  return ui_owner_ != nullptr ? ui_owner_->GetTagInfo(tag_name) : 0;
}

void NativePaintingCtxHarmony::CreatePlatformRenderer(
    int id, PlatformRendererType type, const fml::RefPtr<PropBundle>& init_data,
    const PlatformRendererInitConfig& init_config) {
  Enqueue([platform_ref = platform_ref_, id, type, init_data, init_config]() {
    auto ref = std::static_pointer_cast<NativePaintingCtxPlatformHarmonyRef>(
        platform_ref);
    ref->CreatePlatformRenderer(id, type, init_data, init_config);
  });
}

void NativePaintingCtxHarmony::UpdateDisplayList(int id,
                                                 DisplayList display_list) {
  Enqueue([platform_ref = platform_ref_, id,
           display_list = std::move(display_list)]() mutable {
    auto ref = std::static_pointer_cast<NativePaintingCtxPlatformHarmonyRef>(
        platform_ref);
    ref->UpdateDisplayList(id, std::move(display_list));
  });
}

void NativePaintingCtxHarmony::UpdateTextBundle(int id, intptr_t bundle) {
  auto* paragraph = reinterpret_cast<harmony::ParagraphHarmony*>(bundle);
  if (paragraph == nullptr) {
    return;
  }
  fml::RefPtr<fml::RefCountedThreadSafeStorage> text_bundle(paragraph);
  Enqueue([renderer_context = renderer_context_, id,
           text_bundle = std::move(text_bundle)]() mutable {
    if (renderer_context != nullptr) {
      renderer_context->UpdateTextBundle(id, std::move(text_bundle));
    }
  });
}

void NativePaintingCtxHarmony::DestroyTextBundle(int id) {
  Enqueue([renderer_context = renderer_context_, id]() {
    if (renderer_context != nullptr) {
      renderer_context->DestroyTextBundle(id);
    }
  });
}

void NativePaintingCtxHarmony::Enqueue(shell::UIOperation&& operation) {
  if (queue_) {
    queue_->EnqueueUIOperation(std::move(operation));
  }
}

}  // namespace tasm
}  // namespace lynx
