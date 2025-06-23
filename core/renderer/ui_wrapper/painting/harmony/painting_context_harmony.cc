// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/harmony/painting_context_harmony.h"

#include <string>
#include <utility>
#include <vector>

#include "core/shell/dynamic_ui_operation_queue.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/utils/text_utils.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_root.h"

namespace lynx {
namespace tasm {

void PaintingContextHarmonyRef::InsertPaintingNode(int parent, int child,
                                                   int index) {
  ui_owner_->InsertUI(parent, child, index);
}

void PaintingContextHarmonyRef::RemovePaintingNode(int parent, int child,
                                                   int index, bool is_move) {
  ui_owner_->RemoveUI(parent, child, index, is_move);
}

void PaintingContextHarmonyRef::DestroyPaintingNode(int parent, int child,
                                                    int index) {
  ui_owner_->DestroyUI(parent, child, index);
}

void PaintingContextHarmonyRef::SetGestureDetectorState(int64_t id,
                                                        int32_t gesture_id,
                                                        int32_t state) {
  ui_owner_->SetGestureDetectorState(id, gesture_id, state);
}

void PaintingContextHarmonyRef::UpdateScrollInfo(int32_t container_id,
                                                 bool smooth,
                                                 float estimated_offset,
                                                 bool scrolling) {
  ui_owner_->UpdateScrollInfo(container_id, smooth, estimated_offset,
                              scrolling);
}

void PaintingContextHarmonyRef::UpdateEventInfo(bool has_touch_pseudo) {
  ui_owner_->SetHasTouchPseudo(has_touch_pseudo);
}

void PaintingContextHarmonyRef::InsertListItemPaintingNode(int list_sign,
                                                           int child_sign) {
  ui_owner_->InsertListItemPaintingNode(list_sign, child_sign);
}

void PaintingContextHarmonyRef::RemoveListItemPaintingNode(int list_sign,
                                                           int child_sign) {
  ui_owner_->RemoveListItemPaintingNode(list_sign, child_sign);
}

void PaintingContextHarmonyRef::UpdateContentOffsetForListContainer(
    int32_t container_id, float content_size, float delta_x, float delta_y,
    bool is_init_scroll_offset, bool from_layout) {
  ui_owner_->UpdateContentOffsetForListContainer(
      container_id, content_size, delta_x, delta_y, is_init_scroll_offset,
      from_layout);
}

void PaintingContextHarmonyRef::UpdateNodeReadyPatching(
    std::vector<int32_t> ready_ids, std::vector<int32_t> remove_ids) {
  for (int node_ready_id : ready_ids) {
    ui_owner_->OnNodeReady(node_ready_id);
  }
}

void PaintingContextHarmonyRef::SetNeedMarkPaintEndTiming(
    const tasm::PipelineID& pipeline_id) {
  // For Harmony, we mock the draw_end timing by monitoring FrameCallback by
  // now.
  // TODO(kechenglong): move this logic to platform if Harmony supports
  // draw_end monitoring.
  ui_owner_->PostDrawEndTimingFrameCallback();
}

PaintingContextHarmony::~PaintingContextHarmony() = default;

#pragma region NodeOperations
void PaintingContextHarmony::CreatePaintingNode(
    int id, const std::string& tag,
    const fml::RefPtr<PropBundle>& painting_data, bool flatten,
    bool create_node_async, uint32_t node_index) {
  Enqueue([ui_owner = ui_owner_.get(), id, tag, props = painting_data,
           node_index] {
    ui_owner->CreateUI(
        id, tag, reinterpret_cast<PropBundleHarmony*>(props.get()), node_index);
  });
}

void PaintingContextHarmony::ConsumeGesture(int64_t id, int32_t gesture_id,
                                            const pub::Value& params) {
  auto lepus_map = pub::ValueUtils::ConvertValueToLepusValue(params);

  Enqueue(
      [ui_owner = ui_owner_.get(), id, gesture_id, map = std::move(lepus_map)] {
        ui_owner->ConsumeGesture(id, gesture_id, map);
      });
}

void PaintingContextHarmony::UpdatePaintingNode(
    int id, bool tend_to_flatten,
    const fml::RefPtr<PropBundle>& painting_data) {
  Enqueue([ui_owner = ui_owner_.get(), id, props = painting_data] {
    ui_owner->UpdateUI(id, reinterpret_cast<PropBundleHarmony*>(props.get()));
  });
}

void PaintingContextHarmony::UpdateLayout(
    int tag, float x, float y, float width, float height, const float* paddings,
    const float* margins, const float* borders, const float* bounds,
    const float* sticky, float max_height, uint32_t node_index) {
#define MAKE_UNIQUE_COPY(src, size)                      \
  std::unique_ptr<float[]> src##_copy{nullptr};          \
  if (src) {                                             \
    src##_copy = std::make_unique<float[]>(size);        \
    memcpy(src##_copy.get(), src, sizeof(float) * size); \
  }

  MAKE_UNIQUE_COPY(paddings, 4)
  MAKE_UNIQUE_COPY(margins, 4)
  MAKE_UNIQUE_COPY(borders, 4)
  MAKE_UNIQUE_COPY(sticky, 4)
#undef MAKE_UNIQUE_COPY
  Enqueue([ui_owner = ui_owner_.get(), tag, x, y, width, height,
           paddings = std::move(paddings_copy),
           margins = std::move(margins_copy), borders = std::move(borders_copy),
           sticky = std::move(sticky_copy), max_height, node_index] {
    ui_owner->UpdateLayout(tag, x, y, width, height, paddings.get(),
                           margins.get(), sticky.get(), max_height, node_index);
  });
}

void PaintingContextHarmony::SetKeyframes(
    fml::RefPtr<PropBundle> keyframes_data) {}

#pragma endregion

#pragma region Lifecycles
void PaintingContextHarmony::Flush() { queue_->Flush(); }
void PaintingContextHarmony::HandleValidate(int tag) {}
void PaintingContextHarmony::FinishTasmOperation(
    const std::shared_ptr<PipelineOptions>& options) {}
void PaintingContextHarmony::FinishLayoutOperation(
    const std::shared_ptr<PipelineOptions>& options) {
  Enqueue([ui_owner = ui_owner_.get(), options]() {
    ui_owner->OnLayoutFinish(options->list_comp_id_, options->operation_id);
  });
}
#pragma endregion

#pragma region For Devtool
// todo: to be implemented.

std::vector<float> PaintingContextHarmony::getBoundingClientOrigin(int id) {
  return {};
}
std::unique_ptr<pub::Value> PaintingContextHarmony::GetTextInfo(
    const std::string& content, const pub::Value& info) {
  auto ret =
      harmony::TextUtils::GetTextInfo(content, info, ui_owner_->Context());
  return std::make_unique<PubLepusValue>(std::move(ret));
}
std::vector<float> PaintingContextHarmony::getWindowSize(int id) { return {}; }

std::vector<float> PaintingContextHarmony::GetRectToWindow(int id) {
  return {};
}
#pragma endregion

#pragma region UIMethods

std::vector<float> PaintingContextHarmony::GetRectToLynxView(int64_t id) {
  float result[4] = {0, 0, 0, 0};
  auto task = base::MoveOnlyClosure<void>(
      [ui_owner = ui_owner_.get(), &result, id]() mutable {
        auto ui = ui_owner->FindUIBySign(id);
        if (ui) {
          ui->GetBoundingClientRect(result, false);
        }
      });
  ui_owner_->GetUITaskRunner()->PostSyncTask(std::move(task));
  result[2] = result[2] - result[0];
  result[3] = result[3] - result[1];
  return std::vector<float>(result, result + 4);
}

std::vector<float> PaintingContextHarmony::ScrollBy(int64_t id, float width,
                                                    float height) {
  auto* ui = ui_owner_->FindUIBySign(id);
  if (ui) {
    return ui->ScrollBy(width, height);
  }
  return std::vector<float>{0, 0, width, height};
}

void PaintingContextHarmony::Invoke(
    int64_t id, const std::string& method, const pub::Value& params,
    const std::function<void(int32_t code, const pub::Value& data)>& callback) {
  base::MoveOnlyClosure<void, int32_t, const pub::Value&> cb =
      [callback](int32_t code, const pub::Value& data) {
        callback(code, data);
      };
  ui_owner_->InvokeUIMethod(id, method, params, std::move(cb));
}
#pragma endregion

int32_t PaintingContextHarmony::GetTagInfo(const std::string& tag_name) {
  return ui_owner_->GetTagInfo(tag_name);
}

bool PaintingContextHarmony::IsFlatten(base::MoveOnlyClosure<bool, bool> func) {
  if (func != nullptr) {
    return func(false);
  }
  return false;
}

void PaintingContextHarmony::UpdatePlatformExtraBundle(
    int32_t id, PlatformExtraBundle* bundle) {
  Enqueue(
      [ui_owner = ui_owner_.get(), id,
       platform_bundle =
           reinterpret_cast<PlatformExtraBundleHarmony*>(bundle)->GetBundle()] {
        ui_owner->UpdateExtraData(id, platform_bundle);
      });
}

bool PaintingContextHarmony::NeedAnimationProps() { return false; }

void PaintingContextHarmony::InvokeUIMethod(int32_t id,
                                            const std::string& method,
                                            fml::RefPtr<tasm::PropBundle> value,
                                            int32_t callback_id) {
  Enqueue([ui_owner = ui_owner_.get(), value = std::move(value), id,
           method_name = method, callback_id]() {
    auto* prop_bundle = reinterpret_cast<tasm::PropBundleHarmony*>(value.get());
    ui_owner->InvokeUIMethod(id, method_name, prop_bundle, callback_id);
  });
}

void PaintingContextHarmony::SetUIOperationQueue(
    const std::shared_ptr<shell::DynamicUIOperationQueue>& queue) {
  queue_ = queue;
  queue_->SetEnableFlush(true);
}

void PaintingContextHarmony::Enqueue(shell::UIOperation&& op) {
  queue_->EnqueueUIOperation(std::move(op));
}
}  // namespace tasm
}  // namespace lynx
