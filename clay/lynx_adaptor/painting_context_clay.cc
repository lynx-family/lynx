// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/painting_context_clay.h"

#include <unordered_map>
#include <utility>

#include "clay/lynx_adaptor/base_def.h"
#include "clay/lynx_adaptor/clay_value.h"
#include "clay/lynx_adaptor/platform_extra_bundle_clay.h"
#include "clay/lynx_adaptor/prop_bundle_impl.h"
#include "clay/lynx_adaptor/value_converter.h"
#include "clay/public/value.h"
#include "clay/ui/component/list/lynx_list_data.h"
#include "clay/ui/lynx_module/type_utils.h"
#include "clay/ui/shadow/text_render.h"
#include "core/renderer/css/css_property_id.h"

namespace lynx {

namespace {

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
  view_context_->UpdateNodeReadyPatching(std::move(ready_ids),
                                         std::move(remove_ids));
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

void PaintingContextClay::Flush() { queue_->Flush(); }

void PaintingContextClay::HandleValidate(int tag) {
  // TODO(zhangxiao): this function use to reload image if image download
  // failed
}

void PaintingContextClay::FinishLayoutOperation(
    const std::shared_ptr<PipelineOptions>& options) {
  view_context_->FinishLayoutOperation(options->list_comp_id_,
                                       options->list_id_);
}

void PaintingContextClay::FinishTasmOperation(
    const std::shared_ptr<PipelineOptions>& options) {}

// Invoked by BTS
void PaintingContextClay::InvokeUIMethod(int32_t view_id,
                                         const std::string& method,
                                         fml::RefPtr<tasm::PropBundle> args,
                                         int32_t callback_id) {
  clay::LynxModuleValues values;
  auto prob = static_cast<lynx::PropBundleImpl*>(args.get());
  for (auto& it : prob->mutable_map()) {
    values.names.push_back(it.first);
    values.values.push_back(std::move(it.second));
  }
  view_context_->InvokeUIMethod(
      view_id, method, values,
      [this, callback_id](clay::LynxUIMethodResult code, clay::Value data) {
        clay::Value::Map map;
        map.emplace("code", clay::Value(static_cast<int>(code)));
        map.emplace("data", std::move(data));
        runtime_proxy_->CallJSApiCallbackWithValue(
            callback_id,
            std::make_unique<ClayValue>(clay::Value(std::move(map))));
      });
}

void PaintingContextClay::CreatePaintingNode(
    int id, const std::string& tag,
    const fml::RefPtr<PropBundle>& painting_data, bool flatten,
    bool create_node_async, uint32_t node_index) {
  const char* tag_name = tag.c_str();
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
  // In Clay, we force the following components to have a repaint boundary for
  // better performance, regardless of the value of flatten: PageView,
  // ScrollView, ListView, Swiper, EditableView[Input].
  view_context_->CreateView(id, tag_name);
  // we just apply value to repaint boundary if flatten false
  if (!flatten) {
    view_context_->SetRepaintBoundary(id, true);
  }
  SetAttribute(id, pda, true);
}

int32_t PaintingContextClay::GetTagInfo(const std::string& tag_name) {
  return view_context_->GetTagInfo(tag_name);
}

void PaintingContextClay::SetKeyframes(fml::RefPtr<PropBundle> keyframes_data) {
  auto pdr = static_cast<PropBundleImpl*>(keyframes_data.get());
  auto keyframes_iter = pdr->map().find("keyframes");
  if (keyframes_iter == pdr->map().end()) {
    FML_DLOG(ERROR) << "SetKeyframes 'keyframes' not found";
    return;
  }

  const auto& prop_keyframes_value = keyframes_iter->second;
  view_context_->SetKeyframes(prop_keyframes_value);
}

void PaintingContextClay::UpdatePaintingNode(
    int id, bool tend_to_flatten,
    const fml::RefPtr<PropBundle>& painting_data) {
  auto* pda = painting_data.get();
  SetAttribute(id, pda, false);
  view_context_->SetRepaintBoundary(id, !tend_to_flatten);
}

void PaintingContextClay::UpdateLayout(int tag, float x, float y, float width,
                                       float height, const float* paddings,
                                       const float* margins,
                                       const float* borders,
                                       const float* bounds, const float* sticky,
                                       float max_height, uint32_t node_index) {
  // Set margins, bounds, paddings.
  // Margins should be earlier then bounds because of it may be used during
  // bounds setting.
  view_context_->SetMargins(tag, margins[0], margins[1], margins[2],
                            margins[3]);
  view_context_->SetBounds(tag, std::roundf(x), std::roundf(y),
                           std::roundf(width), std::roundf(height));
  view_context_->SetPaddings(tag, std::roundf(paddings[0]),
                             std::roundf(paddings[1]), std::roundf(paddings[2]),
                             std::roundf(paddings[3]));
  view_context_->UpdateSticky(tag, sticky);
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
  auto task =
      [view_context = view_context_, id,
       platform_bundle =
           reinterpret_cast<PlatformExtraBundleClay*>(bundle)->GetBundle()] {
        view_context->UpdateExtraData(id, platform_bundle.get());
      };
  if (queue_) {
    Enqueue(std::move(task));
  } else {
    task();
  }
}

// private
void PaintingContextClay::SetAttribute(int sign, PropBundle* attributes,
                                       bool init) {
  auto pda = static_cast<PropBundleImpl*>(attributes);

  if (init && pda) {  // Add event props when create view
    for (auto& event : pda->event_handlers()) {
      view_context_->AddEventProp(sign, event.c_str());
    }
  }

  if (!pda || pda->map().empty()) {
    view_context_->DidUpdateAttributes(sign);
    return;
  }

  auto& map = pda->mutable_map();

  clay::Value trans{};
  auto iter = map.find(kPropertyNameTransition);
  if (iter != map.end()) {  // Has transition
    if (init) {
      trans = std::move(iter->second);  // Save transition value
    } else {                            // Set transition firstly and destroy it
      view_context_->SetAttribute(sign, iter->first.c_str(), iter->second);
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
    view_context_->SetAttribute(sign, iter->first.c_str(), iter->second);
  }

  // Update transition finally when create view
  if (init && trans.IsArray()) {
    view_context_->SetAttribute(sign, kPropertyNameTransition, trans);
  }

  if (animation_property.IsArray()) {
    view_context_->SetAttribute(sign, kPropertyNameAnimation,
                                animation_property);
  }
  view_context_->DidUpdateAttributes(sign);
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

void PaintingContextClay::StopExposure(const pub::Value& options) {}

void PaintingContextClay::ResumeExposure() {}

std::list<int32_t> PaintingContextClay::GetAncestorElements(int32_t tag) {
  return engine_proxy_->GetAncestorElements(tag);
}

void PaintingContextClay::Enqueue(base::closure&& op) {
  queue_->Enqueue(std::move(op));
}

}  // namespace tasm
}  // namespace lynx
