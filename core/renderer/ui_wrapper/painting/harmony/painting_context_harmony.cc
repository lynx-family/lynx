// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/harmony/painting_context_harmony.h"

#include <cstring>
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

void PaintingContextHarmonyRef::ListReusePaintingNode(
    int sign, const std::string& item_key) {
  ui_owner_->ListReusePaintingNode(sign, item_key);
}

void PaintingContextHarmonyRef::ListCellWillAppear(
    int sign, const std::string& item_key) {
  ui_owner_->ListCellWillAppear(sign, item_key);
}

void PaintingContextHarmonyRef::ListCellDisappear(int sign, bool isExist,
                                                  const std::string& item_key) {
  ui_owner_->ListCellDisappear(sign, isExist, item_key);
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
  ui_owner_->PostDrawEndTimingFrameCallback(pipeline_id);
}

void PaintingContextHarmonyRef::CreateUI(int id, const std::string& tag,
                                         PropBundleHarmony* props,
                                         uint32_t node_index) {
  ui_owner_->CreateUI(id, tag, props, node_index);
}

void PaintingContextHarmonyRef::UpdateUI(int id, PropBundleHarmony* props) {
  ui_owner_->UpdateUI(id, props);
}

void PaintingContextHarmonyRef::ConsumeGesture(int64_t id, int32_t gesture_id,
                                               const lepus::Value& map) {
  ui_owner_->ConsumeGesture(id, gesture_id, map);
}

void PaintingContextHarmonyRef::UpdateLayout(
    int tag, float x, float y, float width, float height, const float* paddings,
    const float* margins, const float* borders, const float* sticky,
    float max_height, uint32_t node_index) {
  ui_owner_->UpdateLayout(tag, x, y, width, height, paddings, margins, sticky,
                          max_height, node_index);
}

void PaintingContextHarmonyRef::OnLayoutFinish(int32_t list_comp_id,
                                               int64_t operation_id) {
  ui_owner_->OnLayoutFinish(list_comp_id, operation_id);
}

void PaintingContextHarmonyRef::StopExposure(const lepus::Value& options) {
  ui_owner_->StopExposure(options);
}

void PaintingContextHarmonyRef::ResumeExposure() {
  ui_owner_->ResumeExposure();
}

void PaintingContextHarmonyRef::UpdateExtraData(
    int32_t id,
    const fml::RefPtr<fml::RefCountedThreadSafeStorage>& platform_bundle) {
  ui_owner_->UpdateExtraData(id, platform_bundle);
}

void PaintingContextHarmonyRef::InvokeUIMethod(int32_t id,
                                               const std::string& method,
                                               tasm::PropBundleHarmony* args,
                                               int32_t callback_id) {
  ui_owner_->InvokeUIMethod(id, method, args, callback_id);
}

void PaintingContextHarmonyRef::InvokeUIMethod(
    int32_t id, const std::string& method, const lepus::Value& params,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  ui_owner_->InvokeUIMethod(id, method, params, std::move(callback));
}

void PaintingContextHarmonyRef::SetKeyframes(PropBundleHarmony* prop_bundle) {
  ui_owner_->SetKeyframes(prop_bundle);
}

PaintingContextHarmony::PaintingContextHarmony(harmony::UIOwner* ui_owner) {
  platform_ref_ = std::make_shared<PaintingContextHarmonyRef>(ui_owner);
}

PaintingContextHarmony::~PaintingContextHarmony() = default;

void PaintingContextHarmony::CreatePaintingNode(
    int id, const std::string& tag,
    const fml::RefPtr<PropBundle>& painting_data, bool flatten,
    bool create_node_async, uint32_t node_index) {
  Enqueue([platform_ref = platform_ref_, id, tag, props = painting_data,
           node_index] {
    auto harmony_ref =
        std::static_pointer_cast<PaintingContextHarmonyRef>(platform_ref);
    harmony_ref->CreateUI(
        id, tag, reinterpret_cast<PropBundleHarmony*>(props.get()), node_index);
  });
}

void PaintingContextHarmony::ConsumeGesture(int64_t id, int32_t gesture_id,
                                            const pub::Value& params) {
  auto lepus_map = pub::ValueUtils::ConvertValueToLepusValue(params);

  Enqueue([platform_ref = platform_ref_, id, gesture_id,
           gesture_map = std::move(lepus_map)] {
    auto harmony_ref =
        std::static_pointer_cast<PaintingContextHarmonyRef>(platform_ref);
    harmony_ref->ConsumeGesture(id, gesture_id, gesture_map);
  });
}

void PaintingContextHarmony::UpdatePaintingNode(
    int id, bool tend_to_flatten,
    const fml::RefPtr<PropBundle>& painting_data) {
  Enqueue([platform_ref = platform_ref_, id, props = painting_data] {
    auto harmony_ref =
        std::static_pointer_cast<PaintingContextHarmonyRef>(platform_ref);
    harmony_ref->UpdateUI(id,
                          reinterpret_cast<PropBundleHarmony*>(props.get()));
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
  Enqueue([platform_ref = platform_ref_, tag, x, y, width, height,
           paddings = std::move(paddings_copy),
           margins = std::move(margins_copy), borders = std::move(borders_copy),
           sticky = std::move(sticky_copy), max_height, node_index] {
    auto harmony_ref =
        std::static_pointer_cast<PaintingContextHarmonyRef>(platform_ref);
    harmony_ref->UpdateLayout(tag, x, y, width, height, paddings.get(),
                              margins.get(), borders.get(), sticky.get(),
                              max_height, node_index);
  });
}

void PaintingContextHarmony::SetKeyframes(
    fml::RefPtr<PropBundle> keyframes_data) {
  Enqueue([platform_ref = platform_ref_, data = std::move(keyframes_data)]() {
    auto harmony_ref =
        std::static_pointer_cast<PaintingContextHarmonyRef>(platform_ref);
    auto* prop_bundle = reinterpret_cast<PropBundleHarmony*>(data.get());
    harmony_ref->SetKeyframes(prop_bundle);
  });
}

void PaintingContextHarmony::Flush() { queue_->Flush(); }
void PaintingContextHarmony::HandleValidate(int tag) {}
void PaintingContextHarmony::FinishTasmOperation(
    const std::shared_ptr<PipelineOptions>& options) {}
void PaintingContextHarmony::FinishLayoutOperation(
    const std::shared_ptr<PipelineOptions>& options) {
  Enqueue([platform_ref = platform_ref_, options]() {
    auto harmony_ref =
        std::static_pointer_cast<PaintingContextHarmonyRef>(platform_ref);
    harmony_ref->OnLayoutFinish(options->list_comp_id_, options->operation_id);
  });
}

std::vector<float> PaintingContextHarmony::getBoundingClientOrigin(int id) {
  return {};
}

std::unique_ptr<pub::Value> PaintingContextHarmony::GetTextInfo(
    const std::string& content, const pub::Value& info) {
  auto ret =
      harmony::TextUtils::GetTextInfo(content, info, GetUIOwner()->Context());
  return std::make_unique<PubLepusValue>(std::move(ret));
}

void PaintingContextHarmony::StopExposure(const pub::Value& options) {
  auto lepus_options = pub::ValueUtils::ConvertValueToLepusValue(options);
  Enqueue(
      [platform_ref = platform_ref_, lepus_opts = std::move(lepus_options)]() {
        auto harmony_ref =
            std::static_pointer_cast<PaintingContextHarmonyRef>(platform_ref);
        harmony_ref->StopExposure(lepus_opts);
      });
}

void PaintingContextHarmony::ResumeExposure() {
  Enqueue([platform_ref = platform_ref_]() {
    auto harmony_ref =
        std::static_pointer_cast<PaintingContextHarmonyRef>(platform_ref);
    harmony_ref->ResumeExposure();
  });
}

std::vector<float> PaintingContextHarmony::getWindowSize(int id) { return {}; }

std::vector<float> PaintingContextHarmony::GetRectToWindow(int id) {
  return {};
}

std::vector<float> PaintingContextHarmony::GetRectToLynxView(int64_t id) {
  float result[4] = {0, 0, 0, 0};
  auto task = base::MoveOnlyClosure<void>(
      [platform_ref = platform_ref_, &result, id]() mutable {
        auto harmony_ref =
            std::static_pointer_cast<PaintingContextHarmonyRef>(platform_ref);
        auto ui = harmony_ref->GetUIOwner()->FindUIBySign(id);
        if (ui) {
          ui->GetBoundingClientRect(result, false);
        }
      });
  GetUIOwner()->GetUITaskRunner()->PostSyncTask(std::move(task));
  result[2] = result[2] - result[0];
  result[3] = result[3] - result[1];
  return std::vector<float>(result, result + 4);
}

std::vector<float> PaintingContextHarmony::ScrollBy(int64_t id, float width,
                                                    float height) {
  auto* ui = GetUIOwner()->FindUIBySign(id);
  if (ui) {
    return ui->ScrollBy(width, height);
  }
  return std::vector<float>{0, 0, width, height};
}

void PaintingContextHarmony::Invoke(
    int64_t id, const std::string& method, const pub::Value& params,
    const std::function<void(int32_t code, const pub::Value& data)>& callback) {
  base::MoveOnlyClosure<void, int32_t, const lepus::Value&> cb =
      [platform_ref = platform_ref_, callback](int32_t code,
                                               const lepus::Value& data) {
        auto harmony_ref =
            std::static_pointer_cast<PaintingContextHarmonyRef>(platform_ref);
        harmony_ref->GetUIOwner()->RunTaskOnTASMThread(
            [callback, code, data]() { callback(code, PubLepusValue(data)); });
      };
  auto lepus_params = pub::ValueUtils::ConvertValueToLepusValue(params);

  Enqueue([platform_ref = platform_ref_, id, method,
           lepus_params = std::move(lepus_params),
           cb = std::move(cb)]() mutable {
    auto harmony_ref =
        std::static_pointer_cast<PaintingContextHarmonyRef>(platform_ref);
    harmony_ref->InvokeUIMethod(id, method, lepus_params, std::move(cb));
  });
}

void PaintingContextHarmony::EnqueueInvoke(
    int64_t id, const std::string& method, const pub::Value& params,
    const std::function<void(int32_t code, const pub::Value& data)>& callback) {
  Invoke(id, method, params, callback);
}

int32_t PaintingContextHarmony::GetTagInfo(const std::string& tag_name) {
  return GetUIOwner()->GetTagInfo(tag_name);
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
      [platform_ref = platform_ref_, id,
       platform_bundle =
           reinterpret_cast<PlatformExtraBundleHarmony*>(bundle)->GetBundle()] {
        auto harmony_ref =
            std::static_pointer_cast<PaintingContextHarmonyRef>(platform_ref);
        harmony_ref->UpdateExtraData(id, platform_bundle);
      });
}

bool PaintingContextHarmony::NeedAnimationProps() { return false; }

void PaintingContextHarmony::InvokeUIMethod(int32_t id,
                                            const std::string& method,
                                            fml::RefPtr<tasm::PropBundle> value,
                                            int32_t callback_id) {
  Enqueue([platform_ref = platform_ref_, value = std::move(value), id,
           method_name = method, callback_id]() {
    auto* prop_bundle = reinterpret_cast<tasm::PropBundleHarmony*>(value.get());
    auto harmony_ref =
        std::static_pointer_cast<PaintingContextHarmonyRef>(platform_ref);
    harmony_ref->InvokeUIMethod(id, method_name, prop_bundle, callback_id);
  });
}

void PaintingContextHarmony::SetUIOperationQueue(
    const std::shared_ptr<shell::UIOperationQueueInterface>& queue) {
  queue_ = std::static_pointer_cast<shell::DynamicUIOperationQueue>(queue);
  queue_->SetEnableFlush(true);
}

void PaintingContextHarmony::Enqueue(shell::UIOperation&& op) {
  queue_->EnqueueUIOperation(std::move(op));
}
}  // namespace tasm
}  // namespace lynx
