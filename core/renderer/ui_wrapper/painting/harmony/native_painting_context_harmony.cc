// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/harmony/native_painting_context_harmony.h"

#include <memory>
#include <utility>

#include "core/renderer/ui_wrapper/common/harmony/platform_extra_bundle_harmony.h"
#include "core/renderer/ui_wrapper/layout/harmony/text_layout_harmony.h"
#include "core/renderer/ui_wrapper/layout/harmony/text_layout_manager_harmony.h"
#include "core/renderer/ui_wrapper/painting/harmony/native_painting_context_platform_harmony_ref.h"
#include "core/renderer/ui_wrapper/painting/harmony/platform_renderer_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/shadow_node_owner.h"

namespace lynx {
namespace tasm {
NativePaintingCtxHarmony::NativePaintingCtxHarmony(
    harmony::UIOwner* ui_owner, harmony::ShadowNodeOwner* node_owner)
    : ui_owner_(std::unique_ptr<harmony::UIOwner>(ui_owner)),
      text_layout_manager_(
          std::make_unique<TextLayoutManagerHarmony>(node_owner)),
      renderer_context_(
          std::make_shared<harmony::LynxRendererContext>(ui_owner_.get())) {
  text_layout_impl_ =
      std::make_unique<TextLayoutHarmony>(text_layout_manager_.get());
  platform_ref_ = std::make_shared<NativePaintingCtxPlatformHarmonyRef>(
      std::make_unique<PlatformRendererHarmonyFactory>(renderer_context_),
      ui_owner_.get());
}

NativePaintingCtxHarmony::~NativePaintingCtxHarmony() = default;

void NativePaintingCtxHarmony::Flush() { queue_->Flush(); }

void NativePaintingCtxHarmony::FinishTasmOperation(
    const std::shared_ptr<PipelineOptions>& options) {
  if (!queue_ || !options) {
    return;
  }
  if (options->native_update_data_order_ ==
      queue_->GetNativeUpdateDataOrder()) {
    queue_->UpdateStatus(shell::UIOperationStatus::TASM_FINISH);
  }
}

void NativePaintingCtxHarmony::FinishLayoutOperation(
    const std::shared_ptr<PipelineOptions>& options) {
  if (!options) {
    return;
  }
  Enqueue([ui_owner = ui_owner_.get(), options]() {
    if (!ui_owner) {
      return;
    }
    ui_owner->OnLayoutFinish(options->list_comp_id_, options->operation_id);
    // TODO: Remove this fallback once the fragment layer reports paint end
    // after the actual draw completes.
    ui_owner->PostDrawEndTimingFrameCallback(options->pipeline_id);
  });
  if (!queue_ || !has_first_screen_) {
    return;
  }
  Enqueue([weak_queue = std::weak_ptr<shell::DynamicUIOperationQueue>(queue_),
           native_update_data_order = options->native_update_data_order_]() {
    if (auto queue = weak_queue.lock()) {
      if (native_update_data_order == queue->GetNativeUpdateDataOrder()) {
        queue->UpdateStatus(shell::UIOperationStatus::ALL_FINISH);
      }
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

void NativePaintingCtxHarmony::Enqueue(shell::UIOperation&& op) {
  queue_->EnqueueUIOperation(std::move(op));
}

int32_t NativePaintingCtxHarmony::GetTagInfo(const std::string& tag_name) {
  return ui_owner_ != nullptr ? ui_owner_->GetTagInfo(tag_name) : 0;
}

void NativePaintingCtxHarmony::UpdatePlatformExtraBundle(
    int32_t id, PlatformExtraBundle* bundle) {
  if (bundle == nullptr) {
    return;
  }
  auto platform_bundle =
      static_cast<PlatformExtraBundleHarmony*>(bundle)->GetBundle();
  Enqueue([renderer_context = renderer_context_, id,
           platform_bundle = std::move(platform_bundle)]() mutable {
    if (renderer_context != nullptr) {
      renderer_context->UpdatePlatformExtraBundle(id,
                                                  std::move(platform_bundle));
    }
  });
}

void NativePaintingCtxHarmony::CreatePlatformRenderer(
    int id, PlatformRendererType type, const fml::RefPtr<PropBundle>& init_data,
    const PlatformRendererInitConfig& init_config) {
  Enqueue(
      [platform_ref = platform_ref_, id, type, props = init_data, init_config] {
        auto harmony_ref =
            std::static_pointer_cast<NativePaintingCtxPlatformHarmonyRef>(
                platform_ref);
        harmony_ref->CreatePlatformRenderer(id, type, props, init_config);
      });
}

void NativePaintingCtxHarmony::CreatePlatformExtendedRenderer(
    int id, const base::String& tag_name,
    const fml::RefPtr<PropBundle>& init_data,
    const PlatformRendererInitConfig& init_config) {
  Enqueue([platform_ref = platform_ref_, id, tag_name, props = init_data,
           init_config] {
    auto harmony_ref =
        std::static_pointer_cast<NativePaintingCtxPlatformHarmonyRef>(
            platform_ref);
    harmony_ref->CreatePlatformExtendedRenderer(id, tag_name, props,
                                                init_config);
  });
}

void NativePaintingCtxHarmony::UpdateDisplayList(int id,
                                                 DisplayList display_list) {
  Enqueue([platform_ref = platform_ref_, id,
           list = std::move(display_list)]() mutable {
    auto harmony_ref =
        std::static_pointer_cast<NativePaintingCtxPlatformHarmonyRef>(
            platform_ref);
    harmony_ref->UpdateDisplayList(id, std::move(list));
  });
}

fml::RefPtr<PaintImage> NativePaintingCtxHarmony::CreateImage(
    int id, base::String src, const ImagePaintInfo& paint_info, float width,
    float height, int32_t event_mask, bool disable_default_resize) {
  Enqueue([renderer_context = renderer_context_, id, src = src.str(),
           paint_info, width, height, event_mask, disable_default_resize]() {
    if (renderer_context) {
      renderer_context->CreateImageManager(id, src, paint_info, width, height,
                                           event_mask, disable_default_resize);
    }
  });
  return fml::MakeRefCounted<PaintImage>(id);
}

void NativePaintingCtxHarmony::UpdateTextBundle(int id, intptr_t bundle) {
  if (text_layout_manager_ == nullptr) {
    return;
  }
  auto platform_bundle = text_layout_manager_->GetTextBundle(id);
  if (platform_bundle == nullptr) {
    return;
  }
  Enqueue([renderer_context = renderer_context_, id,
           platform_bundle = std::move(platform_bundle)]() mutable {
    if (renderer_context != nullptr) {
      renderer_context->UpdateTextBundle(id, std::move(platform_bundle));
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

void NativePaintingCtxHarmony::ReconstructEventTargetTreeRecursively() {
  Enqueue([platform_ref = platform_ref_]() {
    auto harmony_ref =
        std::static_pointer_cast<NativePaintingCtxPlatformHarmonyRef>(
            platform_ref);
    harmony_ref->ReconstructEventTargetTreeRecursively();
  });
}

void NativePaintingCtxHarmony::UpdatePlatformEventBundle(
    int id, PlatformEventBundle bundle) {
  Enqueue(
      [platform_ref = platform_ref_, id, bundle = std::move(bundle)]() mutable {
        auto harmony_ref =
            std::static_pointer_cast<NativePaintingCtxPlatformHarmonyRef>(
                platform_ref);
        harmony_ref->UpdatePlatformEventBundle(id, std::move(bundle));
      });
}

}  // namespace tasm
}  // namespace lynx
