// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/painting_context_clay.h"

#include <array>
#include <cmath>
#include <unordered_map>
#include <utility>

#include "clay/lynx_adaptor/base_def.h"
#include "clay/lynx_adaptor/clay_value.h"
#include "clay/lynx_adaptor/platform_extra_bundle_clay.h"
#include "clay/lynx_adaptor/prop_bundle_impl.h"
#include "clay/lynx_adaptor/value_converter.h"
#include "clay/public/value.h"
#include "clay/ui/common/value_utils.h"
#include "clay/ui/component/list/lynx_list_data.h"
#include "clay/ui/lynx_module/type_utils.h"
#include "clay/ui/shadow/text_render.h"
#include "core/renderer/css/css_property_id.h"

namespace lynx {

namespace {

float MaybeRoundLayoutMetric(float value) {
#if defined(OS_MAC) || defined(OS_WIN)
  return std::roundf(value);
#else
  return value;
#endif
}

[[maybe_unused]] void ConvertListData(const lynx::tasm::ListData& origin,
                                      clay::LynxListData* res) {
  FML_DCHECK(res);
  for (const std::string& name : origin.GetViewTypeNames()) {
    res->PushViewType(name.c_str());
  }

  res->SetNewArch(origin.GetNewArch());
  res->SetDiffable(origin.GetDiffable());
  res->SetFullSpan(origin.GetFullSpan());
  res->SetStickyTop(origin.GetStickyTop());
  res->SetStickyBottom(origin.GetStickyBottom());
  res->SetInsertions(origin.GetInsertions());
  res->SetRemovals(origin.GetRemovals());
  res->SetUpdateFrom(origin.GetUpdateFrom());
  res->SetUpdateTo(origin.GetUpdateTo());
  res->SetMoveFrom(origin.GetMoveFrom());
  res->SetMoveTo(origin.GetMoveTo());
}

}  // namespace

namespace tasm {

void PaintingContextClayRef::InsertPaintingNode(int parent, int child,
                                                int index) {
  view_context_->AddView(child, parent, index);
}

void PaintingContextClayRef::RemovePaintingNode(int parent, int child,
                                                int index, bool is_move) {
  view_context_->RemoveView(child, parent, is_move);
}

void PaintingContextClayRef::DestroyPaintingNode(int parent, int child,
                                                 int index) {
  view_context_->RemoveView(child, parent);
  view_context_->DestroyView(child);
}

void PaintingContextClayRef::UpdateScrollInfo(int32_t container_id, bool smooth,
                                              float estimated_offset,
                                              bool scrolling) {
  view_context_->UpdateScrollInfo(container_id, smooth, estimated_offset,
                                  scrolling);
}

void PaintingContextClayRef::UpdateNodeReadyPatching(
    std::vector<int32_t> ready_ids, std::vector<int32_t> remove_ids) {
  auto task = [view_context = view_context_, ready_ids = std::move(ready_ids),
               remove_ids = std::move(remove_ids)]() mutable {
    view_context->UpdateNodeReadyPatching(std::move(ready_ids),
                                          std::move(remove_ids));
  };
  auto queue = queue_.lock();
  if (queue) {
    queue->Enqueue(std::move(task));
  } else {
    task();
  }
}

void PaintingContextClayRef::InsertListItemPaintingNode(int list_sign,
                                                        int child_sign) {
  view_context_->InsertListItemPaintingNode(list_sign, child_sign);
}

void PaintingContextClayRef::RemoveListItemPaintingNode(int list_sign,
                                                        int child_sign) {
  view_context_->RemoveListItemPaintingNode(list_sign, child_sign);
}

void PaintingContextClayRef::UpdateContentOffsetForListContainer(
    int32_t container_id, float content_size, float delta_x, float delta_y,
    bool is_init_scroll_offset, bool from_layout) {
  view_context_->UpdateContentOffsetForListContainer(container_id, content_size,
                                                     delta_x, delta_y);
}

void PaintingContextClayRef::SetPerfController(
    const std::shared_ptr<PerfControllerClay>& controller) {
  perf_controller_ = controller;
  view_context_->SetPipelineTimingDelegate(perf_controller_);
  view_context_->SetScrollFluencyMonitorDelegate(perf_controller_);
  if (perf_controller_) {
    perf_controller_->SetUITaskRunner(view_context_->GetUITaskRunner());
  }
}

void PaintingContextClayRef::SetNeedMarkPaintEndTiming(
    const tasm::PipelineID& pipeline_id) {
  if (perf_controller_) {
    perf_controller_->SetNeedMarkPaintEndTiming(pipeline_id);
  }
}

void PaintingContextClayRef::SetGestureDetectorState(int64_t id,
                                                     int32_t gesture_id,
                                                     int32_t state) {
  view_context_->SetGestureDetectorState(id, gesture_id, state);
}

PaintingContextClay::PaintingContextClay(clay::ViewContext* view_context)
    : view_context_(view_context) {
  FML_DCHECK(view_context);
  platform_ref_ = std::make_shared<PaintingContextClayRef>(view_context);
  view_context_->SetUIComponentDelegate(this);
}

PaintingContextClay::~PaintingContextClay() {
  view_context_->SetUIComponentDelegate(nullptr);
  view_context_->ResetPageView();
}

void PaintingContextClay::Flush() { ui_operation_queue_ref_->Flush(); }

void PaintingContextClay::HandleValidate(int tag) {
  // TODO(zhangxiao): this function use to reload image if image download
  // failed
}

void PaintingContextClay::FinishLayoutOperation(
    const std::shared_ptr<PipelineOptions>& options) {
  const int32_t child_view_id = options ? options->list_comp_id_ : 0;
  const int32_t parent_view_id = options ? options->list_id_ : 0;
  auto task = [view_context = view_context_, child_view_id, parent_view_id]() {
    view_context->FinishLayoutOperation(child_view_id, parent_view_id);
  };
  if (ui_operation_queue_ref_) {
    Enqueue(std::move(task));
  } else {
    task();
  }
}

void PaintingContextClay::FinishTasmOperation(
    const std::shared_ptr<PipelineOptions>& options) {}

// Invoked by BTS
void PaintingContextClay::InvokeUIMethod(int32_t view_id,
                                         const std::string& method,
                                         fml::RefPtr<tasm::PropBundle> args,
                                         int32_t callback_id) {
  Enqueue([view_context = view_context_, runtime_proxy = runtime_proxy_,
           args = std::move(args), view_id, method = method, callback_id]() {
    clay::LynxModuleValues values;
    auto prob = static_cast<lynx::PropBundleImpl*>(args.get());
    for (auto& it : prob->mutable_map()) {
      values.names.push_back(it.first);
      values.values.push_back(std::move(it.second));
    }
    view_context->InvokeUIMethod(
        view_id, method, values,
        [runtime_proxy, callback_id](clay::LynxUIMethodResult code,
                                     clay::Value data) {
          clay::Value::Map map;
          map.emplace("code", clay::Value(static_cast<int>(code)));
          map.emplace("data", std::move(data));
          runtime_proxy->CallJSApiCallbackWithValue(
              callback_id,
              std::make_unique<ClayValue>(clay::Value(std::move(map))));
        });
  });
}

void PaintingContextClay::CreatePaintingNode(
    int id, const std::string& tag,
    const fml::RefPtr<PropBundle>& painting_data, bool flatten,
    bool create_node_async, uint32_t node_index) {
  auto task = [view_context = view_context_, id, tag, painting_data,
               flatten]() mutable {
    std::string tag_name = tag;
    auto* pda = painting_data.get();

    // Terminology:
    // flatten: In lynx, the view does not have a real platform view, and its
    //          content will be drawn on the parent node that owns the platform
    //          view.
    // repaint_boundary: In Clay, whether this view paints separately from its
    //                   parent.
    //
    // So, if flatten is true, need to paint this view and its parent together,
    // that means repaint_boundary is false in Clay; and when flatten is false,
    // need to draw this view and its parent separately, that means
    // repaint_boundary is true in Clay.
    //
    // In Clay, we force the following components to have a repaint boundary
    // for better performance, regardless of the value of flatten: PageView,
    // ScrollView, ListView, Swiper, EditableView[Input].
    view_context->CreateView(id, tag_name);
    // we just apply value to repaint boundary if flatten false
    if (!flatten) {
      view_context->SetRepaintBoundary(id, true);
    }
    SetAttribute(view_context, id, pda, true);
  };
  if (ui_operation_queue_ref_) {
    Enqueue(std::move(task));
  } else {
    task();
  }
}

int32_t PaintingContextClay::GetTagInfo(const std::string& tag_name) {
  return view_context_->GetTagInfo(tag_name);
}

void PaintingContextClay::SetKeyframes(fml::RefPtr<PropBundle> keyframes_data) {
  auto task = [view_context = view_context_,
               keyframes_data = std::move(keyframes_data)]() mutable {
    auto pdr = static_cast<PropBundleImpl*>(keyframes_data.get());
    auto keyframes_iter = pdr->map().find("keyframes");
    if (keyframes_iter == pdr->map().end()) {
      FML_DLOG(ERROR) << "SetKeyframes 'keyframes' not found";
      return;
    }

    const auto& prop_keyframes_value = keyframes_iter->second;
    view_context->SetKeyframes(prop_keyframes_value);
  };
  if (ui_operation_queue_ref_) {
    Enqueue(std::move(task));
  } else {
    task();
  }
}

void PaintingContextClay::UpdatePaintingNode(
    int id, bool tend_to_flatten,
    const fml::RefPtr<PropBundle>& painting_data) {
  auto task = [view_context = view_context_, id, tend_to_flatten,
               painting_data]() {
    auto* pda = painting_data.get();
    SetAttribute(view_context, id, pda, false);
    view_context->SetRepaintBoundary(id, !tend_to_flatten);
  };
  if (ui_operation_queue_ref_) {
    Enqueue(std::move(task));
  } else {
    task();
  }
}

void PaintingContextClay::UpdateLayout(int tag, float x, float y, float width,
                                       float height, const float* paddings,
                                       const float* margins,
                                       const float* borders,
                                       const float* bounds, const float* sticky,
                                       float max_height, uint32_t node_index) {
  const std::array<float, 4> paddings_copy = {paddings[0], paddings[1],
                                              paddings[2], paddings[3]};
  const std::array<float, 4> margins_copy = {margins[0], margins[1], margins[2],
                                             margins[3]};
  const bool has_sticky = sticky != nullptr;
  const std::array<float, 4> sticky_copy =
      has_sticky
          ? std::array<float, 4>{sticky[0], sticky[1], sticky[2], sticky[3]}
          : std::array<float, 4>{0, 0, 0, 0};
  auto task = [view_context = view_context_, tag, x, y, width, height,
               paddings_copy, margins_copy, sticky_copy, has_sticky]() {
    // Set margins, bounds, paddings.
    // Margins should be earlier then bounds because of it may be used during
    // bounds setting.
    view_context->SetMargins(tag, margins_copy[0], margins_copy[1],
                             margins_copy[2], margins_copy[3]);
    view_context->SetBounds(
        tag, MaybeRoundLayoutMetric(x), MaybeRoundLayoutMetric(y),
        MaybeRoundLayoutMetric(width), MaybeRoundLayoutMetric(height));
    view_context->SetPaddings(tag, MaybeRoundLayoutMetric(paddings_copy[0]),
                              MaybeRoundLayoutMetric(paddings_copy[1]),
                              MaybeRoundLayoutMetric(paddings_copy[2]),
                              MaybeRoundLayoutMetric(paddings_copy[3]));
    view_context->UpdateSticky(tag, has_sticky ? sticky_copy.data() : nullptr);
  };
  if (ui_operation_queue_ref_) {
    Enqueue(std::move(task));
  } else {
    task();
  }
}

// Invoked by MTS/worklet
void PaintingContextClay::Invoke(
    int64_t id, const std::string& method, const pub::Value& params,
    const std::function<void(int32_t code, const pub::Value& data)>& callback) {
  clay::LynxModuleValues values;
  auto map = ValueConverter::CreateClayValue(params);
  if (map.IsMap()) {
    for (auto& [key, val] : map.GetMap()) {
      values.names.push_back(key);
      values.values.push_back(std::move(val));
    }
  }
  view_context_->InvokeUIMethod(
      id, method, values,
      [callback](clay::LynxUIMethodResult code, clay::Value data) {
        callback(static_cast<int>(code), ClayValue(std::move(data)));
      });
}

void PaintingContextClay::EnqueueInvoke(
    int64_t id, const std::string& method, const pub::Value& params,
    const std::function<void(int32_t code, const pub::Value& data)>& callback) {
  clay::LynxModuleValues values;
  auto map = ValueConverter::CreateClayValue(params);
  if (map.IsMap()) {
    for (auto& [key, val] : map.GetMap()) {
      values.names.push_back(key);
      values.values.push_back(std::move(val));
    }
  }
  Enqueue([view_context = view_context_, id, method, values = std::move(values),
           callback]() mutable {
    view_context->InvokeUIMethod(
        id, method, values,
        [callback](clay::LynxUIMethodResult code, clay::Value data) {
          callback(static_cast<int>(code), ClayValue(std::move(data)));
        });
  });
}

void PaintingContextClay::getAbsolutePosition(int id, float* position) {
  view_context_->GetAbsolutePosition(id, position[0], position[1]);
}

void PaintingContextClay::OnFirstMeaningfulLayout() {
  view_context_->OnFirstMeaningfulLayout();
}

std::vector<float> PaintingContextClay::getBoundingClientOrigin(int id) {
  float to_root_x = 0;
  float to_root_y = 0;
  float position[2] = {0.0, 0.0};
  getAbsolutePosition(id, position);

  to_root_x = position[1];
  to_root_y = position[0];
  return std::vector<float>{to_root_x, to_root_y};
}

bool PaintingContextClay::IsFlatten(base::MoveOnlyClosure<bool, bool> func) {
  if (func != nullptr) {
    return func(true);
  }
  return false;
}

void PaintingContextClay::UpdatePlatformExtraBundle(
    int32_t id, PlatformExtraBundle* bundle) {
  Enqueue(
      [view_context = view_context_, id,
       platform_bundle =
           reinterpret_cast<PlatformExtraBundleClay*>(bundle)->GetBundle()] {
        view_context->UpdateExtraData(id, platform_bundle.get());
      });
}

// private
void PaintingContextClay::SetAttribute(clay::ViewContext* view_context,
                                       int sign, PropBundle* attributes,
                                       bool init) {
  auto pda = static_cast<PropBundleImpl*>(attributes);

  if (init && pda) {  // Add event props when create view
    for (auto& event : pda->event_handlers()) {
      view_context->AddEventProp(sign, event.c_str());
    }
  }

  if (!pda || pda->map().empty()) {
    view_context->DidUpdateAttributes(sign);
    return;
  }

  auto& map = pda->mutable_map();

  clay::Value trans{};
  auto iter = map.find(kPropertyNameTransition);
  if (iter != map.end()) {  // Has transition
    if (init) {
      trans = std::move(iter->second);  // Save transition value
    } else {                            // Set transition firstly and destroy it
      view_context->SetAttribute(sign, iter->first.c_str(), iter->second);
    }
    // Erase the transition prop
    map.erase(iter);
  }
  // remove animation and execute in the end
  iter = map.find(kPropertyNameAnimation);
  clay::Value animation_property{};
  if (iter != map.end()) {
    animation_property = std::move(iter->second);
    map.erase(iter);
  }

  iter = map.begin();
  for (; iter != map.end(); iter++) {
    view_context->SetAttribute(sign, iter->first.c_str(), iter->second);
  }

  // Update transition finally when create view
  if (init && trans.IsArray()) {
    view_context->SetAttribute(sign, kPropertyNameTransition, trans);
  }

  if (animation_property.IsArray()) {
    view_context->SetAttribute(sign, kPropertyNameAnimation,
                               animation_property);
  }

  const auto& gesture_detector_map = pda->GestureDetectorMap();
  if (gesture_detector_map && gesture_detector_map->size() > 0) {
    view_context->SetGestureDetectorMap(sign, gesture_detector_map.value());
  }
  view_context->DidUpdateAttributes(sign);
}

clay::LynxListData* PaintingContextClay::OnListGetData(int view_id) {
#ifndef ENABLE_CLAY_LITE
  lynx::tasm::ListData list_data = engine_proxy_->GetListData(view_id);
  clay::LynxListData* res = new clay::LynxListData(
      static_cast<int>(list_data.GetViewTypeNames().size()));
  ConvertListData(list_data, res);
  return res;
#else
  return nullptr;
#endif
}

int PaintingContextClay::OnObtainChild(int view_id, int index,
                                       int operation_id) {
  return engine_proxy_->ObtainListChild(view_id, static_cast<uint32_t>(index),
                                        operation_id, false);
}

void PaintingContextClay::OnRecycleChild(int view_id, int child_id) {
  engine_proxy_->RecycleListChild(view_id, child_id);
}

void PaintingContextClay::OnCreateAddChild(int view_id, int index,
                                           int operation_id) {
  engine_proxy_->RenderListChild(view_id, static_cast<uint32_t>(index),
                                 operation_id);
}

void PaintingContextClay::OnUpdateChild(int parent_id, int to_update_id,
                                        int target_index,
                                        int64_t operation_id) {
  engine_proxy_->UpdateListChild(parent_id, static_cast<uint32_t>(to_update_id),
                                 static_cast<uint32_t>(target_index),
                                 operation_id);
}

void PaintingContextClay::OnScrollByListContainer(int view_id, float offset_x,
                                                  float offset_y,
                                                  float original_x,
                                                  float original_y) {
  engine_proxy_->ScrollByListContainer(view_id, offset_x, offset_y, original_x,
                                       original_y);
}

void PaintingContextClay::OnScrollToPosition(int view_id, int position,
                                             float offset, int align,
                                             bool smooth) {
  engine_proxy_->ScrollToPosition(view_id, position, offset, align, smooth);
}

void PaintingContextClay::OnScrollStopped(int view_id) {
  engine_proxy_->ScrollStopped(view_id);
}

bool PaintingContextClay::OnEnableRasterAnimation() {
  return engine_proxy_->EnableRasterAnimation();
}

std::unique_ptr<lynx::pub::Value> PaintingContextClay::GetTextInfo(
    const std::string& content, const pub::Value& info) {
  auto rk_info = ValueConverter::CreateClayValue(info);
  auto result = clay::TextRender::GetTextInfo(content.c_str(), rk_info);
  return std::make_unique<ClayValue>(std::move(result));
}

void PaintingContextClay::StopExposure(const pub::Value& options) {
  auto opts = ValueConverter::CreateClayValue(options);
  bool send_event = true;
  if (opts.IsMap()) {
    send_event = SafeGetBool(FindMapItem(opts.GetMap(), "sendEvent"), true);
  }
  Enqueue([view_context = view_context_, send_event] {
    view_context->StopExposure(send_event);
  });
}

void PaintingContextClay::ResumeExposure() {
  Enqueue([view_context = view_context_] { view_context->ResumeExposure(); });
}

std::vector<float> PaintingContextClay::GetRectToLynxView(int64_t id) {
  return view_context_->GetRectToLynxView(id);
}

std::list<int32_t> PaintingContextClay::GetAncestorElements(int32_t tag) {
  return engine_proxy_->GetAncestorElements(tag);
}

void PaintingContextClay::ConsumeGesture(int64_t id, int32_t gesture_id,
                                         const pub::Value& params) {
  auto clay_params = ValueConverter::CreateClayValue(params);
  view_context_->ConsumeGesture(id, gesture_id, clay_params);
}

void PaintingContextClay::Enqueue(base::closure&& op) {
  if (!EnableUIOperationQueue() || !ui_operation_queue_ref_) {
    op();
    return;
  }
  ui_operation_queue_ref_->Enqueue(std::move(op));
}

}  // namespace tasm
}  // namespace lynx
