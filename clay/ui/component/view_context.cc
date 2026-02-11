// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/view_context.h"

#include <cstring>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <utility>

#include "base/include/string/string_number_convert.h"
#include "clay/gfx/geometry/float_rect.h"
#include "clay/gfx/geometry/sticky_info.h"
#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/common/isolate.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/component.h"
#include "clay/ui/component/editable/editable_view.h"
#include "clay/ui/component/intersection_observer_manager.h"
#include "clay/ui/component/list/list_container/list_container_wrapper.h"
#include "clay/ui/component/list/lynx_list_data.h"
#include "clay/ui/component/native_view.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/view.h"
#include "clay/ui/lynx_module/lynx_ui_method_registrar.h"
#include "clay/ui/shadow/bundle.h"
#ifdef ENABLE_NET_LOADER
#include "clay/net/net_loader_manager.h"
#endif
#include <cstdlib>

#include "clay/public/value.h"
#include "clay/ui/component/view_registry.h"
#include "clay/ui/rendering/render_object.h"
#include "clay/ui/resource/font_collection.h"
#include "clay/ui/shadow/editable_shadow_node.h"
#include "clay/ui/shadow/inline_view_shadow_node.h"
#include "clay/ui/shadow/native_view_shadow_node.h"
#include "clay/ui/shadow/shadow_node.h"
#include "clay/ui/shadow/text_shadow_node.h"
#define DEBUG_KEYFRAMES 0

#define DEBUG_CLAY_CTX 0
#define CTX_DEBUG_TAG "[CLAY]-[CTX] "
#if (DEBUG_CLAY_CTX)
#define CTX_LOG FML_LOG(ERROR) << CTX_DEBUG_TAG
#else
#define CTX_LOG FML_EAT_STREAM_PARAMETERS(true)
#endif

#ifndef FIND_VIEW_WITH_ID_OR_RET
#define FIND_VIEW_WITH_ID_OR_RET                             \
  auto target = view_map_.find(id);                          \
  if (target == view_map_.end()) {                           \
    FML_LOG(ERROR) << "target view: " << id << " not found"; \
    return;                                                  \
  }                                                          \
  auto view = target->second
#endif

namespace clay {

ViewContext::ViewContext(PageView* root, ShadowNodeOwner* shadow_node_owner)
    : page_view_(root),
      shadow_node_owner_(shadow_node_owner),
      weak_factory_(this) {
  static uint32_t id = 0;
  unique_id_ = id++;
  Isolate::Instance().RegisterViewContext(unique_id_,
                                          weak_factory_.GetWeakPtr());
  page_view_->view_context_ = this;
  FML_DLOG(INFO) << "ViewContext construction; id = " << unique_id_;
}

ViewContext::~ViewContext() {
  Isolate::Instance().UnregisterViewContext(unique_id_);
  FML_DLOG(INFO) << "ViewContext destruction; id = " << unique_id_;
}

void ViewContext::CleanLeakedViews() {
  std::list<int> leaked_view_ids_;
  for (auto& pair : view_map_) {
    if (pair.second != page_view_) {
      leaked_view_ids_.emplace_back(pair.first);
    }
  }
  for (int id : leaked_view_ids_) {
    RemoveView(id, -1);
    DestroyView(id);
  }
}

void ViewContext::OnOutputSurfaceDestroyed() {
  for (auto& pair : view_map_) {
    if (!pair.second->attach_to_tree()) {
      pair.second->render_object()->MarkSubtreeDirty();
    }
  }
}

int ViewContext::IndexOf(int id) const {
  auto it = view_map_.find(id);
  if (it == view_map_.end()) {
    return -1;
  }
  auto child = it->second;
  auto parent = it->second->Parent();
  if (!parent) {
    return -1;
  }
  return parent->GetChildIndex(child);
}

bool ViewContext::CreateView(int id, const std::string& tag_name) {
  CTX_LOG << "CreateView id:" << id << " tag:" << tag_name;
  FML_DCHECK(id != -1);

  if (tag_name == "page") {
    view_map_[id] = page_view_;
    page_view_->SetID(id);
    return true;
  }
  BaseView* view = nullptr;
  view = ViewRegistry::GetInstance()->CreateView(id, tag_name, page_view_);

  if (!view) {
    FML_DLOG(ERROR) << "unsupported view type: " << tag_name
                    << " , fallback to view.";
    view = new View(id, page_view_);
  }

  view->SetDestructListener(
      [this](BaseView* view) { view_map_.erase(view->id()); });
  view_map_[id] = view;
  ConsumeInitialAttributes(view_map_[id]);
  return true;
}

void ViewContext::AddView(int id, int parent_id, int index) {
  CTX_LOG << "AddView id:" << id << " parent id:" << parent_id
          << " index: " << index;

  auto target = view_map_.find(id);
  auto parent = view_map_.find(parent_id);

  if (parent == view_map_.end()) {
    FML_LOG(ERROR) << "target view parent: " << id << " not found";
    return;
  }

  if (target == view_map_.end()) {
    FML_LOG(ERROR) << "target view: " << id << " not found";
    return;
  }

  if (index < 0) {
    parent->second->AddChild(target->second);
  } else {
    parent->second->AddChild(target->second, index);
  }
}

void ViewContext::InsertListItemPaintingNode(int list_id, int child_id) {
  auto parent = view_map_.find(list_id);
  auto child = view_map_.find(child_id);
  if (parent == view_map_.end()) {
    FML_LOG(ERROR) << "parent view: " << list_id << " not found";
    return;
  }
  if (child == view_map_.end()) {
    FML_LOG(ERROR) << "child view: " << child_id << " not found";
    return;
  }
  if (parent->second->Is<ListContainerWrapper>()) {
    static_cast<ListContainerWrapper*>(parent->second)
        ->InsertListItemPaintingNode(child->second);
  }
}

void ViewContext::RemoveListItemPaintingNode(int list_id, int child_id) {
  auto parent = view_map_.find(list_id);
  auto child = view_map_.find(child_id);
  if (parent == view_map_.end()) {
    FML_LOG(ERROR) << "parent view: " << list_id << " not found";
    return;
  }
  if (child == view_map_.end()) {
    FML_LOG(ERROR) << "child view: " << child_id << " not found";
    return;
  }
  if (parent->second->Is<ListContainerWrapper>()) {
    static_cast<ListContainerWrapper*>(parent->second)
        ->RemoveListItemPaintingNode(child->second);
  }
}

void ViewContext::RemoveView(int id, int parent_id,
                             bool is_temporarily_removed) {
  CTX_LOG << "RemoveView id:" << id;

  FIND_VIEW_WITH_ID_OR_RET;

  BaseView* parent;
  if (parent_id < 0) {
    parent = view->Parent();
  } else {
    auto it = view_map_.find(parent_id);
    if (it == view_map_.end()) {
      FML_LOG(ERROR) << "target view parent: " << id << " not found";
      return;
    }
    parent = it->second;
  }

  if (parent) {
    if (is_temporarily_removed) {
      parent->RemoveChildTemporarily(view);
    } else {
      parent->RemoveChild(view);
    }
  }
}

void ViewContext::DestroyAnonymousView(BaseView* view) {
  if (!view->IsAnonymousView()) {
    FML_DCHECK(false);
    // Anonymous view id is -1, and named view may be destroyed previously.
    return;
  }
  view->Destroy();
  auto& children = view->GetChildren();
  auto iter = children.begin();
  while (iter != children.end()) {
    if (!DestroyView((*iter)->id())) {
      DestroyAnonymousView(*iter);
    }
    // Remove child dangling pointer in parent.
    iter = children.erase(iter);
  }
  delete view;
}

bool ViewContext::DestroyView(int id) {
  CTX_LOG << "DestroyView id:" << id;

  auto target = view_map_.find(id);
  if (target == view_map_.end()) {
    FML_DLOG(ERROR) << "target view: " << id << " not found";
    return false;
  }

  BaseView* view = target->second;
  if (view->IsDelayDestroy()) {
    view_map_.erase(id);
    return true;
  }

  view->Destroy();
  auto& children = view->GetChildren();
  auto iter = children.begin();
  while (iter != children.end()) {
    if (!DestroyView((*iter)->id())) {
      DestroyAnonymousView(*iter);
    }
    // Remove child dangling pointer in parent.
    iter = children.erase(iter);
  }

  delete view;
  view_map_.erase(id);
  return true;
}

void ViewContext::ResetPageView() {
  CTX_LOG << "ResetPageView";

  auto& children = page_view_->GetChildren();
  auto iter = children.begin();
  while (iter != children.end()) {
    if (!DestroyView((*iter)->id())) {
      DestroyAnonymousView(*iter);
    }
    // Remove child dangling pointer in parent.
    iter = children.erase(iter);
  }

  page_view_->ResetPageView();
  component_id_to_ui_id_map_.clear();
  CleanLeakedViews();
}

ShadowNode* ViewContext::CreateShadowNode(int id, const std::string& tag_name,
                                          bool allow_inline) {
  CTX_LOG << "CreateLayoutNode id:" << id << " tag:" << tag_name;

  auto node = ViewRegistry::GetInstance()->CreateShadowNode(
      id, shadow_node_owner_, tag_name);
  if (node) {
    shadow_node_owner_->AddNode(id, node);
  } else if (allow_inline) {
    node = new InlineViewShadowNode(shadow_node_owner_, tag_name, id);
    shadow_node_owner_->AddNode(id, node);
    // NOTE: Here we must return nullptr instead of the node to pass the proper
    // flags to Lynx. This requires refactoring.
    return nullptr;
  }
  return node;
}

int32_t ViewContext::GetTagInfo(const std::string& tag_name) {
  return ViewRegistry::GetInstance()->GetTagInfo(tag_name);
}

void ViewContext::AddShadowNode(int id, int parent_id, int index) {
  CTX_LOG << "AddLayoutNode id:" << id << " parent id:" << parent_id
          << " index: " << index;

  auto target = shadow_node_owner_->GetNode(id);
  auto parent = shadow_node_owner_->GetNode(parent_id);

  if (!parent || !target) {
    return;
  }

  if (index < 0) {
    parent->AddChild(target);
  } else {
    parent->AddChild(target, index);
  }
}

void ViewContext::NotifyLowMemory() {
  for (auto& pair : view_map_) {
    pair.second->NotifyLowMemory();
  }
}

void ViewContext::RemoveShadowNode(int id) {
  CTX_LOG << "RemoveLayout id:" << id;

  auto child = shadow_node_owner_->GetNode(id);
  if (!child) {
    return;
  }

  ShadowNode* parent = child->Parent();
  if (parent) {
    parent->RemoveChild(child);
  }
}

bool ViewContext::DestroyShadowNode(int id) {
  CTX_LOG << "Destroy shadow node id:" << id;

  auto node = shadow_node_owner_->GetNode(id);
  if (!node) {
    return false;
  }

  node->Destroy();
  shadow_node_owner_->RemoveNode(id);
  return true;
}

void ViewContext::DestroyAllShadowNode() { shadow_node_owner_->ClearNodes(); }

void ViewContext::TextMeasure(int id, float width, TextMeasureMode width_mode,
                              float height, TextMeasureMode height_mode,
                              float& out_width, float& out_height) {
  auto node = shadow_node_owner_->GetNode(id);
  if (!node) {
    return;
  }

  Measurable* measurable = node->GetMeasurable();
  CustomMeasurable* custom_measurable = node->GetCustomMeasurable();
  if (measurable) {
    MeasureResult result;
    measurable->Measure({width, width_mode, height, height_mode}, result);
    out_width = result.width;
    out_height = result.height;
  } else if (custom_measurable) {
    auto result =
        custom_measurable->Measure({width, width_mode, height, height_mode});
    out_width = result.width;
    out_height = result.height;
  } else {
    FML_LOG(ERROR) << "target shadow node: " << id << " is not measurable";
  }
}

void ViewContext::OnLayout(int id, float width, TextMeasureMode width_mode,
                           float height, TextMeasureMode height_mode,
                           const std::array<float, 4>& paddings,
                           const std::array<float, 4>& borders) {
  auto node = shadow_node_owner_->GetNode(id);
  if (node) {
    node->OnLayout(width, width_mode, height, height_mode, paddings, borders);
  }
}

void ViewContext::UpdateRootSize(int32_t width, int32_t height) {
  page_view_->UpdateRootSize(width, height);
}

void ViewContext::SetShadowNodeAttribute(int id, const char* attr,
                                         const clay::Value& value) {
  auto node = shadow_node_owner_->GetNode(id);
  if (node) {
    node->SetAttribute(attr, value);
  }
}

void ViewContext::ScheduleLayout() { shadow_node_owner_->ScheduleLayout(); }

void ViewContext::Alignment(int id) {
  auto node = shadow_node_owner_->GetNode(id);
  if (node) {
    auto custom_measurable = node->GetCustomMeasurable();
    if (custom_measurable) {
      custom_measurable->Align();
    }
  }
}

void ViewContext::Invalidate() { page_view_->RequestPaint(); }

void ViewContext::SetBounds(int id, float left, float top, float width,
                            float height) {
  CTX_LOG << "SetBounds id:" << id << " " << left << "," << top << " " << width
          << "," << height;

  FIND_VIEW_WITH_ID_OR_RET;
  view->SetBound(
      GetPageView()->RoundPixels(left), GetPageView()->RoundPixels(top),
      GetPageView()->RoundPixels(width), GetPageView()->RoundPixels(height));
}

void ViewContext::SetPaddings(int id, float padding_left, float padding_top,
                              float padding_right, float padding_bottom) {
  FIND_VIEW_WITH_ID_OR_RET;
  view->SetPaddings(GetPageView()->RoundPixels(padding_left),
                    GetPageView()->RoundPixels(padding_top),
                    GetPageView()->RoundPixels(padding_right),
                    GetPageView()->RoundPixels(padding_bottom));
}

void ViewContext::SetMargins(int id, float margin_left, float margin_top,
                             float margin_right, float margin_bottom) {
  FIND_VIEW_WITH_ID_OR_RET;
  view->SetMargins(GetPageView()->RoundPixels(margin_left),
                   GetPageView()->RoundPixels(margin_top),
                   GetPageView()->RoundPixels(margin_right),
                   GetPageView()->RoundPixels(margin_bottom));
}

void ViewContext::SetOpacity(int id, float opacity) {
  FIND_VIEW_WITH_ID_OR_RET;
  view->SetOpacity(opacity);
}

void ViewContext::SetOverflow(int id, int overflow) {
  FIND_VIEW_WITH_ID_OR_RET;
  view->SetOverflow(overflow);
}

void ViewContext::SetBorderStyle(int id, BorderStyleType left,
                                 BorderStyleType top, BorderStyleType right,
                                 BorderStyleType bottom) {
  FIND_VIEW_WITH_ID_OR_RET;
  view->SetBorderStyle({Side::kLeft, Side::kTop, Side::kRight, Side::kBottom},
                       {left, top, right, bottom});
}

void ViewContext::SetBorderWidth(int id, int left_width, int top_width,
                                 int right_width, int bottom_width) {
  FIND_VIEW_WITH_ID_OR_RET;
  view->SetBorderWidth({Side::kLeft, Side::kTop, Side::kRight, Side::kBottom},
                       {GetPageView()->RoundPixels(left_width),
                        GetPageView()->RoundPixels(top_width),
                        GetPageView()->RoundPixels(right_width),
                        GetPageView()->RoundPixels(bottom_width)});
}

void ViewContext::SetBorderColor(int id, unsigned int left_color,
                                 unsigned int top_color,
                                 unsigned int right_color,
                                 unsigned int bottom_color) {
  FIND_VIEW_WITH_ID_OR_RET;
  view->SetBorderColor({Side::kLeft, Side::kTop, Side::kRight, Side::kBottom},
                       {left_color, top_color, right_color, bottom_color});
}

void ViewContext::SetBorderRadius(int id, const FloatSize& left_top,
                                  const FloatSize& right_top,
                                  const FloatSize& right_bottom,
                                  const FloatSize& left_bottom) {
  FIND_VIEW_WITH_ID_OR_RET;
  FloatSize scale_left_top = left_top;
  FloatSize scale_right_top = right_top;
  FloatSize scale_right_bottom = right_bottom;
  FloatSize scale_left_bottom = left_bottom;
  view->SetBorderRadius(scale_left_top, scale_right_top, scale_right_bottom,
                        scale_left_bottom);
}

void ViewContext::SetOutlineStyle(int id, BorderStyleType style) {
  FIND_VIEW_WITH_ID_OR_RET;
  view->SetOutlineStyle(style);
}

void ViewContext::SetOutlineWidth(int id, int width) {
  FIND_VIEW_WITH_ID_OR_RET;
  view->SetOutlineWidth(width);
}

void ViewContext::SetOutlineColor(int id, unsigned int color) {
  FIND_VIEW_WITH_ID_OR_RET;
  view->SetOutlineColor(color);
}

void ViewContext::SetCursor(int id, const char* src[], int size) {
  auto view = GetViewById(id);
  if (view) {
    std::vector<std::string> vec;
    for (int i = 0; i < size; ++i) {
      vec.emplace_back(src[i]);
    }

    view->SetCursor(vec);
  }
}

void ViewContext::SetBackgroundColor(int id, unsigned int color) {
  FIND_VIEW_WITH_ID_OR_RET;
  CTX_LOG << "SetBackgroundColor. id:" << id << " color:" << color;
  view->SetBackgroundColor(Color(color));
}

void ViewContext::SetBackground(int id, const BackgroundData& background) {
  FIND_VIEW_WITH_ID_OR_RET;
  CTX_LOG << "SetBackground. id:" << id;
  view->SetBackground(background);
}

void ViewContext::AppendShadow(int id, const Shadow& shadow) {
  FIND_VIEW_WITH_ID_OR_RET;
  view->AppendShadow(shadow);
}

void ViewContext::SetTransform(int id, const TransformOperations& ops,
                               const FloatPoint& origin) {
  FIND_VIEW_WITH_ID_OR_RET;
  view->SetTransform(ops, origin);
}

void ViewContext::SetTransition(
    int id, const std::vector<TransitionData>& transition_data) {
  FIND_VIEW_WITH_ID_OR_RET;
  view->SetTransition(transition_data);
}

void ViewContext::SetAnimation(
    int id, const std::vector<AnimationData>& animation_data) {
  FIND_VIEW_WITH_ID_OR_RET;
  view->SetAnimation(animation_data);
}

void ViewContext::SetKeyframes(const clay::Value& keyframes_value) {
  page_view_->SetKeyframesData(keyframes_value);
}

void ViewContext::SetAttribute(int id, const char* attr,
                               const clay::Value& value) {
  FIND_VIEW_WITH_ID_OR_RET;
  view->SetAttribute(attr, value);

  if (view->Is<Component>() && strcmp(attr, "ComponentID") == 0) {
    std::string component_id;
    if (value.IsString()) {
      component_id = value.GetString();
    } else if (value.IsInt()) {
      component_id = std::to_string(value.GetInt());
    } else {
      component_id = "-1";
    }
    component_id_to_ui_id_map_[component_id] = id;
  }
}

const clay::ViewportMetrics& ViewContext::GetViewportMetrics() const {
  return page_view_->GetViewportMetrics();
}

void ViewContext::DidUpdateAttributes(int id) {
  FIND_VIEW_WITH_ID_OR_RET;
  CTX_LOG << "id:" << id << " " << view->GetName() << " DidUpdateAttributes.";
  view->DidUpdateAttributes();
}

void ViewContext::AddEventProp(int id, const char* event) {
  FIND_VIEW_WITH_ID_OR_RET;
  view->AddEventCallback(event);
}

void ViewContext::SetGestureDetectorMap(
    int id,
    const std::unordered_map<uint32_t, std::shared_ptr<GestureDetector>>&
        gesture_detector_map) {
  FIND_VIEW_WITH_ID_OR_RET;
  view->SetGestureDetectorMap(gesture_detector_map);
}

void ViewContext::SetGestureDetectorState(int id, int32_t gesture_id,
                                          int32_t state) {
  FIND_VIEW_WITH_ID_OR_RET;
  view->SetGestureDetectorState(gesture_id, state);
}

void ViewContext::ConsumeGesture(int id, int32_t gesture_id,
                                 const Value& params) {
  FIND_VIEW_WITH_ID_OR_RET;
  view->ConsumeGesture(gesture_id, params);
}

void ViewContext::AddShadowNodeEventProp(int id, const char* event) {
  auto node = shadow_node_owner_->GetNode(id);
  if (node && node->IsTextShadowNode()) {
    node->AddEventCallback(event);
  }
}

void ViewContext::SetRepaintBoundary(int id, bool repaint_boundary) {
  FIND_VIEW_WITH_ID_OR_RET;
  CTX_LOG << "id:" << id << " " << view->GetName() << " SetRepaintBoundary.";
  view->SetRepaintBoundary(repaint_boundary);
}

void HostNetLoadFinish(size_t request_seq, bool success, const uint8_t* data,
                       size_t length) {
#ifdef ENABLE_NET_LOADER
  NetLoaderManager::Instance().OnFinished(request_seq, success, data, length);
#endif
}

void ViewContext::SetNetLoadCallback(NetLoadCallback net_load_callback) {
#ifdef ENABLE_NET_LOADER
  if (net_load_callback) {
    NetLoaderManager::Instance().SetHostNetLoader(
        [net_load_callback](const std::string& url, const std::string& method,
                            const std::string& body,
                            const std::map<std::string, std::string>& headers,
                            size_t request_seq) {
          size_t headers_size = headers.size();
          const char** headers_ptr = nullptr;
          if (headers_size > 0) {
            headers_ptr = new const char*[headers_size];
            size_t index = 0;
            for (const auto& header : headers) {
#ifdef _MSC_VER
              headers_ptr[index++] =
                  _strdup((header.first + ":" + header.second).c_str());
#else
              headers_ptr[index++] =
                  strdup((header.first + ":" + header.second).c_str());
#endif
            }
          }

          net_load_callback(url.c_str(), method.c_str(), body.c_str(),
                            headers_ptr, headers_size, request_seq,
                            HostNetLoadFinish);
          for (size_t i = 0; i < headers_size; ++i) {
            free(const_cast<char*>(headers_ptr[i]));
          }

          if (headers_ptr) {
            delete[] headers_ptr;
          }
        });
  }
#endif
}

void ViewContext::OnFirstMeaningfulLayout() {
  page_view_->OnFirstMeaningfulLayout();
}

void ViewContext::UpdateNodeReadyPatching(std::vector<int32_t> ready_ids,
                                          std::vector<int32_t> remove_ids) {
  for (auto id : ready_ids) {
    auto view = FindViewByViewId(id);
    if (view) {
      view->OnNodeReady();
    }
  }
}

void ViewContext::ConsumeInitialAttributes(BaseView* view) {
  view->DidUpdateAttributes();
}

BaseView* ViewContext::FindViewByViewId(int view_id) {
  BaseView* view = nullptr;
  if (view_id == -1) {
    view = page_view_;
  }
  if (view == nullptr) {
    auto itr = view_map_.find(view_id);
    if (itr != view_map_.end()) {
      view = itr->second;
    }
  }
  return view;
}

ShadowNode* ViewContext::FindShadowNodeByNodeId(int node_id) {
  return shadow_node_owner_->GetNode(node_id);
}

BaseView* ViewContext::FindViewByComponentId(const std::string& component_id) {
  if (component_id == "-1") {
    return page_view_;
  }
  auto itr = component_id_to_ui_id_map_.find(component_id);
  int sign = 0;
  if (itr != component_id_to_ui_id_map_.end()) {
    sign = itr->second;
  } else {
    if (!lynx::base::StringToInt(component_id, &sign)) {
      sign = -1;
    }
  }

  return FindViewByViewId(sign);
}

BaseView* ViewContext::FindViewByIdSelector(std::string_view id_selector,
                                            BaseView* from) {
  FML_DCHECK(from);

  if (from->GetIdSelector() == id_selector) {
    return from;
  }

  // DFS
  BaseView* res = nullptr;
  for (BaseView* child : from->GetChildren()) {
    res = FindViewByIdSelector(id_selector, child);
    if (res) {
      break;
    }
  }
  return res;
}

BaseView* ViewContext::FindViewByRefIdSelector(std::string_view ref_id_selector,
                                               BaseView* from) {
  FML_DCHECK(from);

  if (from->GetRefIdSelector() == ref_id_selector) {
    return from;
  }

  // DFS
  BaseView* res = nullptr;
  for (BaseView* child : from->GetChildren()) {
    res = FindViewByRefIdSelector(ref_id_selector, child);
    if (res) {
      break;
    }
  }
  return res;
}

void ViewContext::SetFontFace(const char* font_family, const char* src[],
                              int size) {
  if (size < 1) {
    return;
  }

  std::vector<std::string> src_vec;
  for (int i = 0; i < size; ++i) {
    src_vec.emplace_back(src[i]);
  }

  auto font_collection = Isolate::Instance().GetFontCollection();
  font_collection->PreLoadFontOnMem(
      page_view_->GetTaskRunner(), page_view_->GetResourceLoaderIntercept(),
      page_view_->GetServiceManager(), std::string(font_family),
      std::move(src_vec));
}

void ViewContext::GetAbsolutePosition(int id, float& top, float& left) {
  FIND_VIEW_WITH_ID_OR_RET;
  auto position = view->AbsoluteLocationWithScroll();
  left = position.x();
  top = position.y();
  return;
}

BaseView* ViewContext::GetViewById(int id) {
  auto target = view_map_.find(id);
  if (target == view_map_.end()) {
    FML_LOG(ERROR) << "target view: " << id << " not found";
    return nullptr;
  }

  return target->second;
}

int ViewContext::GetViewIdForLocation(int x, int y) {
  return page_view_->GetViewIdForLocation(x, y);
}

void ViewContext::InvokeUIMethod(int view_id, const std::string& method,
                                 const LynxModuleValues& params,
                                 const LynxUIMethodCallback& callback) {
  auto view = FindViewByViewId(view_id);
  if (view != nullptr) {
    LynxUIMethodRegistrar::Instance().Invoke(method, view, params, callback);
  } else {
    callback(LynxUIMethodResult::kNodeNotFound, clay::Value("node not found"));
  }
}

void ViewContext::UpdateContentOffsetForListContainer(
    int id, float content_size, float target_content_offset_x,
    float target_content_offset_y) {
  auto it = view_map_.find(id);
  if (it != view_map_.end()) {
    auto list_container_view = static_cast<ListContainerWrapper*>(it->second);
    list_container_view->UpdateContentOffsetForListContainer(
        content_size, target_content_offset_x, target_content_offset_y);
  }
}

void ViewContext::UpdateScrollInfo(int id, bool smooth, float estimated_offset,
                                   bool scrolling) {
  auto it = view_map_.find(id);
  if (it != view_map_.end()) {
    auto list_container_view = static_cast<ListContainerWrapper*>(it->second);
    list_container_view->UpdateScrollInfo(smooth, estimated_offset, scrolling);
  }
}

void ViewContext::FinishLayoutOperation(int child_view_id, int parent_view_id) {
  auto parent = view_map_.find(parent_view_id);
  auto child = view_map_.find(child_view_id);
  if (parent != view_map_.end()) {
    parent->second->OnLayoutFinish(child != view_map_.end() ? child->second
                                                            : nullptr);
  }
}

void ViewContext::GetTransformValue(int id,
                                    const float* pad_border_margin_layout,
                                    int size, float* res) {
  // res is std::vector<float>(32 ,0) passed by lynx
  FIND_VIEW_WITH_ID_OR_RET;
  std::vector<float> vec;
  for (int i = 0; i < size; i++) {
    vec.push_back(pad_border_margin_layout[i]);
  }
  TransOffset arr;

  // Returns the coordinates of the four types of boxes
  // - border-box:
  // That is, the width and height of the view set by the front-end, including
  // content, padding, border, corresponding to the real rendering view
  // - content-box: border-box with border and padding width and height removed
  // - padding-box: border-box without the padding width and height of the box
  // - margin-box: border-box plus margin box
  for (int i = 0; i < 4; i++) {
    if (i == 0) {
      view->GetTransformValue(
          pad_border_margin_layout[BoxModelOffset::PAD_LEFT] +
              pad_border_margin_layout[BoxModelOffset::BORDER_LEFT] +
              pad_border_margin_layout[BoxModelOffset::LAYOUT_LEFT],
          -pad_border_margin_layout[BoxModelOffset::PAD_RIGHT] -
              pad_border_margin_layout[BoxModelOffset::BORDER_RIGHT] -
              pad_border_margin_layout[BoxModelOffset::LAYOUT_RIGHT],
          pad_border_margin_layout[BoxModelOffset::PAD_TOP] +
              pad_border_margin_layout[BoxModelOffset::BORDER_TOP] +
              pad_border_margin_layout[BoxModelOffset::LAYOUT_TOP],
          -pad_border_margin_layout[BoxModelOffset::PAD_BOTTOM] -
              pad_border_margin_layout[BoxModelOffset::BORDER_BOTTOM] -
              pad_border_margin_layout[BoxModelOffset::LAYOUT_BOTTOM],
          arr);
    } else if (i == 1) {
      view->GetTransformValue(
          pad_border_margin_layout[BoxModelOffset::BORDER_LEFT] +
              pad_border_margin_layout[BoxModelOffset::LAYOUT_LEFT],
          -pad_border_margin_layout[BoxModelOffset::BORDER_RIGHT] -
              pad_border_margin_layout[BoxModelOffset::LAYOUT_RIGHT],
          pad_border_margin_layout[BoxModelOffset::BORDER_TOP] +
              pad_border_margin_layout[BoxModelOffset::LAYOUT_TOP],
          -pad_border_margin_layout[BoxModelOffset::BORDER_BOTTOM] -
              pad_border_margin_layout[BoxModelOffset::LAYOUT_BOTTOM],
          arr);
    } else if (i == 2) {
      view->GetTransformValue(
          pad_border_margin_layout[BoxModelOffset::LAYOUT_LEFT],
          -pad_border_margin_layout[BoxModelOffset::LAYOUT_RIGHT],
          pad_border_margin_layout[BoxModelOffset::LAYOUT_TOP],
          -pad_border_margin_layout[BoxModelOffset::LAYOUT_BOTTOM], arr);
    } else {
      view->GetTransformValue(
          -pad_border_margin_layout[BoxModelOffset::MARGIN_LEFT] +
              pad_border_margin_layout[BoxModelOffset::LAYOUT_LEFT],
          pad_border_margin_layout[BoxModelOffset::MARGIN_RIGHT] -
              pad_border_margin_layout[BoxModelOffset::LAYOUT_RIGHT],
          -pad_border_margin_layout[BoxModelOffset::MARGIN_TOP] +
              pad_border_margin_layout[BoxModelOffset::LAYOUT_TOP],
          pad_border_margin_layout[BoxModelOffset::MARGIN_BOTTOM] -
              pad_border_margin_layout[BoxModelOffset::LAYOUT_BOTTOM],
          arr);
    }
    res[i * 8] = arr.left_top[0];
    res[i * 8 + 1] = arr.left_top[1];
    res[i * 8 + 2] = arr.right_top[0];
    res[i * 8 + 3] = arr.right_top[1];
    res[i * 8 + 4] = arr.right_bottom[0];
    res[i * 8 + 5] = arr.right_bottom[1];
    res[i * 8 + 6] = arr.left_bottom[0];
    res[i * 8 + 7] = arr.left_bottom[1];
  }
}

void ViewContext::UpdateSticky(int id, const float* sticky) {
  FIND_VIEW_WITH_ID_OR_RET;
  if (sticky == nullptr) {
    view->UpdateSticky(std::nullopt);
    return;
  }
  StickyInfo sticky_info;
  sticky_info.left = sticky[0];
  sticky_info.top = sticky[1];
  sticky_info.right = sticky[2];
  sticky_info.bottom = sticky[3];
  sticky_info.offset_x = 0;
  sticky_info.offset_y = 0;
  view->UpdateSticky(sticky_info);
}

Bundle* ViewContext::GetTextBundle(int32_t id) {
  auto node = shadow_node_owner_->GetNode(id);
  if (node) {
    return node->MoveBundle();
  } else {
    return nullptr;
  }
}

void ViewContext::UpdateExtraData(int id, Bundle* bundle) {
  auto view = FindViewByViewId(id);
  if (view && bundle) {
    bundle->UpdateExtraData(view);
  }
}

float ViewContext::GetBaseline(int id) const {
  auto node = shadow_node_owner_->GetNode(id);
  if (node && node->IsTextShadowNode()) {
    return static_cast<TextShadowNode*>(node)->GetBaseline();
  }
  return 0.f;
}

void ViewContext::SetDefaultOverflowVisible(bool value) {
  page_view_->SetDefaultOverflow(value ? CSSProperty::OVERFLOW_XY
                                       : CSSProperty::OVERFLOW_HIDDEN);
}

void ViewContext::SetExternalScreenshotCallback(
    ExternalScreenshotCallback callback) {
  page_view_->SetExternalScreenshotCallback(std::move(callback));
}

fml::RefPtr<fml::TaskRunner> ViewContext::GetUITaskRunner() const {
  return page_view_->GetTaskRunner();
}

const clay::TaskRunners& ViewContext::GetTaskRunners() const {
  return page_view_->GetTaskRunners();
}

const std::shared_ptr<ServiceManager>& ViewContext::GetServiceManager() const {
  return page_view_->GetServiceManager();
}

void ViewContext::SetEventDelegate(EventDelegate* delegate) {
  if (page_view_) {
    page_view_->SetEventDelegate(delegate);
  }
}

void ViewContext::SetUIComponentDelegate(UIComponentDelegate* delegate) {
  if (page_view_) {
    page_view_->ui_component_delegate_ = delegate;
  }
}

void ViewContext::SetLayoutDelegate(LayoutDelegate* delegate) {
  shadow_node_owner_->SetLayoutDelegate(delegate);
}

void ViewContext::SetPipelineTimingDelegate(
    std::shared_ptr<PipelineTimingDelegate> delegate) {
  if (page_view_) {
    page_view_->SetPipelineTimingDelegate(delegate);
  }
}

void ViewContext::SetScrollFluencyMonitorDelegate(
    std::shared_ptr<ScrollFluencyMonitorDelegate> delegate) {
  if (page_view_) {
    page_view_->SetScrollFluencyMonitorDelegate(delegate);
  }
}

void ViewContext::SyncNativeViewTags(
    std::unordered_set<std::string> tags,
    std::unordered_set<std::string> bootstrap_tags) {
  for (const auto& tag : tags) {
    if (ViewRegistry::GetInstance()->HasView(tag) &&
        bootstrap_tags.find(tag) == bootstrap_tags.end()) {
      continue;
    }
    ViewRegistry::GetInstance()->RegisterView(
        tag,
        [tag](int id, PageView* page_view) {
          return new NativeView(id, tag, page_view);
        },
        GetShadowNodeCreator<NativeViewShadowNode>(), true);
  }
}

std::vector<float> ViewContext::GetRectToLynxView(int64_t id) {
  auto view = FindViewByViewId(id);
  FloatRect rect;
  if (view) {
    rect = view->BoundsRelativeTo(nullptr);
  }
  return std::vector<float>{rect.left(), rect.top(), rect.width(),
                            rect.height()};
}

void ViewContext::StopExposure(bool send_event) {
  if (page_view_) {
    auto intersection_manager = page_view_->intersection_observer_manager();
    if (intersection_manager) {
      intersection_manager->StopExposure(send_event);
    }
  }
}

void ViewContext::ResumeExposure() {
  if (page_view_) {
    auto intersection_manager = page_view_->intersection_observer_manager();
    if (intersection_manager) {
      intersection_manager->ResumeExposure();
    }
  }
}

void ViewContext::SyncNativeViewCompositionPreferences(
    std::unordered_map<std::string, NativeViewCompositionPreference>
        composition_preferences) {
  native_view_composition_preferences_ = std::move(composition_preferences);
}

}  // namespace clay
