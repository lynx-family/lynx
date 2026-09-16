// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/harmony/native_painting_context_harmony.h"

#include <algorithm>
#include <array>
#include <utility>

#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/fml/task_runner.h"
#include "core/renderer/ui_wrapper/layout/harmony/text_layout_harmony.h"
#include "core/renderer/ui_wrapper/layout/harmony/text_measurer_harmony.h"
#include "core/renderer/ui_wrapper/painting/harmony/native_painting_context_platform_harmony_ref.h"
#include "core/renderer/ui_wrapper/painting/harmony/paint_image_harmony.h"
#include "core/renderer/ui_wrapper/painting/harmony/platform_renderer_harmony.h"
#include "core/value_wrapper/value_wrapper_utils.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_renderer_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/paragraph_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/utils/text_utils.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"

namespace lynx {
namespace tasm {
namespace {

std::array<float, 4> CopyMetrics(const float* source) {
  std::array<float, 4> result = {0.f, 0.f, 0.f, 0.f};
  if (source != nullptr) {
    std::copy_n(source, result.size(), result.data());
  }
  return result;
}

}  // namespace

NativePaintingCtxHarmony::NativePaintingCtxHarmony(
    const std::shared_ptr<harmony::LynxContext>& context) {
  text_measurer_ = std::make_unique<TextMeasurerHarmony>(context.get());
  text_layout_impl_ = std::make_unique<TextLayoutHarmony>(text_measurer_.get());
  renderer_context_ = std::make_shared<harmony::LynxRendererContext>(context);
  platform_ref_ = std::make_shared<NativePaintingCtxPlatformHarmonyRef>(
      std::make_unique<PlatformRendererHarmonyFactory>(renderer_context_),
      renderer_context_);
  context->SetNativePaintingContext(
      std::static_pointer_cast<NativePaintingCtxPlatformRef>(platform_ref_));
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

void NativePaintingCtxHarmony::UpdatePaintingNode(
    int id, bool, const fml::RefPtr<PropBundle>& painting_data) {
  if (!painting_data) {
    return;
  }
  Enqueue([ref = platform_ref_, id, painting_data]() {
    std::static_pointer_cast<NativePaintingCtxPlatformHarmonyRef>(ref)
        ->UpdateAttributes(id, painting_data);
  });
}

void NativePaintingCtxHarmony::UpdateLayout(
    int tag, float x, float y, float width, float height, const float* paddings,
    const float* margins, const float* borders, const float* bounds,
    const float* sticky, float max_height, uint32_t node_index,
    bool display_none) {
  (void)bounds;
  (void)sticky;
  (void)max_height;
  (void)node_index;
  (void)display_none;
  auto padding_values = CopyMetrics(paddings);
  auto margin_values = CopyMetrics(margins);
  auto border_values = CopyMetrics(borders);
  Enqueue([ref = platform_ref_, tag, x, y, width, height, padding_values,
           margin_values, border_values]() {
    std::static_pointer_cast<NativePaintingCtxPlatformHarmonyRef>(ref)
        ->UpdateLayoutMetrics(tag, x, y, width, height, padding_values.data(),
                              margin_values.data(), border_values.data());
  });
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
  auto context = renderer_context_->GetLynxContext();
  auto result = harmony::TextUtils::GetTextInfo(content, info, context.get());
  return std::make_unique<PubLepusValue>(std::move(result));
}

void NativePaintingCtxHarmony::StopExposure(const pub::Value& options) {
  auto lynx_context = renderer_context_->GetLynxContext();
  auto runner = lynx_context ? lynx_context->GetUITaskRunner() : nullptr;
  if (runner == nullptr) {
    return;
  }
  auto lepus_options =
      pub::ValueUtils::ConvertValueToLepusValue(options).ToLepusValue();
  runner->PostTask([ref = platform_ref_, options = std::move(lepus_options)]() {
    std::static_pointer_cast<NativePaintingCtxPlatformHarmonyRef>(ref)
        ->StopExposure(options);
  });
}

void NativePaintingCtxHarmony::ResumeExposure() {
  auto lynx_context = renderer_context_->GetLynxContext();
  auto runner = lynx_context ? lynx_context->GetUITaskRunner() : nullptr;
  if (runner == nullptr) {
    return;
  }
  runner->PostTask([ref = platform_ref_]() {
    std::static_pointer_cast<NativePaintingCtxPlatformHarmonyRef>(ref)
        ->ResumeExposure();
  });
}

std::vector<float> NativePaintingCtxHarmony::getBoundingClientOrigin(int id) {
  return {};
}

std::vector<float> NativePaintingCtxHarmony::getWindowSize(int id) {
  float size[2] = {0.f, 0.f};
  std::static_pointer_cast<NativePaintingCtxPlatformHarmonyRef>(platform_ref_)
      ->GetScreenSize(size);
  return {size[0], size[1]};
}

std::vector<float> NativePaintingCtxHarmony::GetRectToWindow(int id) {
  return {};
}

std::vector<float> NativePaintingCtxHarmony::GetRectToLynxView(int64_t id) {
  auto lynx_context = renderer_context_->GetLynxContext();
  auto runner = lynx_context ? lynx_context->GetUITaskRunner() : nullptr;
  if (runner == nullptr) {
    return {};
  }
  auto ref = std::static_pointer_cast<NativePaintingCtxPlatformHarmonyRef>(
      platform_ref_);
  if (runner->RunsTasksOnCurrentThread()) {
    return ref->GetRectToLynxView(static_cast<int32_t>(id));
  }

  struct RectQueryResult {
    fml::AutoResetWaitableEvent event;
    std::vector<float> value;
  };
  auto result = std::make_shared<RectQueryResult>();
  runner->PostTask([ref, id, result]() {
    result->value = ref->GetRectToLynxView(static_cast<int32_t>(id));
    result->event.Signal();
  });
  if (result->event.WaitWithTimeout(fml::TimeDelta::FromSeconds(1))) {
    return {};
  }
  return std::move(result->value);
}

std::vector<float> NativePaintingCtxHarmony::ScrollBy(int64_t id, float width,
                                                      float height) {
  return {};
}

void NativePaintingCtxHarmony::Invoke(
    int64_t id, const std::string& method, const pub::Value& params,
    const std::function<void(int32_t, const pub::Value&)>& callback) {
  auto context = renderer_context_->GetLynxContext();
  if (!context) {
    return;
  }
  auto runner = context->GetUITaskRunner();
  if (!runner) {
    return;
  }
  auto lepus_params =
      pub::ValueUtils::ConvertValueToLepusValue(params).ToLepusValue();
  base::MoveOnlyClosure<void, int32_t, const pub::Value&> cb =
      [callback](int32_t code, const pub::Value& data) {
        callback(code, data);
      };
  // A UI method may not trigger a pipeline, so post directly to the UI thread.
  runner->PostTask([ref = platform_ref_, id, method,
                    params = std::move(lepus_params),
                    cb = std::move(cb)]() mutable {
    auto harmony_ref =
        std::static_pointer_cast<NativePaintingCtxPlatformHarmonyRef>(ref);
    if (harmony_ref) {
      harmony_ref->InvokeUIMethod(id, method, params, std::move(cb));
    }
  });
}

void NativePaintingCtxHarmony::EnqueueInvoke(
    int64_t id, const std::string& method, const pub::Value& params,
    const std::function<void(int32_t, const pub::Value&)>& callback) {
  auto lepus_params =
      pub::ValueUtils::ConvertValueToLepusValue(params).ToLepusValue();
  base::MoveOnlyClosure<void, int32_t, const pub::Value&> cb =
      [callback](int32_t code, const pub::Value& data) {
        callback(code, data);
      };
  Enqueue([ref = platform_ref_, id, method, params = std::move(lepus_params),
           cb = std::move(cb)]() mutable {
    auto harmony_ref =
        std::static_pointer_cast<NativePaintingCtxPlatformHarmonyRef>(ref);
    if (harmony_ref) {
      harmony_ref->InvokeUIMethod(id, method, params, std::move(cb));
    }
  });
}

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
    const PlatformRendererInitConfig& init_config) {
  Enqueue([platform_ref = platform_ref_, id, tag_name, init_data,
           init_config]() {
    auto ref = std::static_pointer_cast<NativePaintingCtxPlatformHarmonyRef>(
        platform_ref);
    ref->CreatePlatformExtendedRenderer(id, tag_name, init_data, init_config);
  });
}

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

void NativePaintingCtxHarmony::EnqueueReconstructEventTargetTreeRecursively() {
  if (queue_ == nullptr) {
    return;
  }
  auto ref = std::static_pointer_cast<NativePaintingCtxPlatformHarmonyRef>(
      platform_ref_);
  if (ref->HasScheduledEventTargetTreeUpdate()) {
    return;
  }
  bool expected = false;
  if (!event_target_tree_update_enqueued_->compare_exchange_strong(expected,
                                                                   true)) {
    return;
  }
  Enqueue([enqueued = event_target_tree_update_enqueued_, ref]() {
    ref->ScheduleEnsureEventTargetTree(kRootId);
    enqueued->store(false);
  });
}

fml::RefPtr<PaintImage> NativePaintingCtxHarmony::CreateImage(
    int id, base::String src, const ImagePaintInfo& paint_info, float width,
    float height, int32_t event_mask, bool disable_default_resize) {
  const int32_t image_key = GenerateUniqueImageKey();
  Enqueue([renderer_context = renderer_context_, id, src = src.str(),
           paint_info, width, height, event_mask, image_key]() {
    renderer_context->CreateImageManager(id, src, paint_info, width, height,
                                         event_mask, image_key);
  });
  return fml::MakeRefCounted<PaintImageHarmony>(
      image_key,
      std::static_pointer_cast<NativePaintingCtxPlatformRef>(platform_ref_));
}

void NativePaintingCtxHarmony::UpdatePlatformEventBundle(
    int id, PlatformEventBundle bundle) {
  Enqueue([ref = platform_ref_, id, bundle = std::move(bundle)]() mutable {
    std::static_pointer_cast<NativePaintingCtxPlatformHarmonyRef>(ref)
        ->UpdatePlatformEventBundle(id, std::move(bundle));
  });
}

void NativePaintingCtxHarmony::Enqueue(shell::UIOperation operation) {
  if (queue_ != nullptr) {
    queue_->EnqueueUIOperation(std::move(operation));
  }
}

}  // namespace tasm
}  // namespace lynx
