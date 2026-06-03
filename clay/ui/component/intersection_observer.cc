// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/intersection_observer.h"

#include <cstddef>
#include <memory>
#include <utility>

#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/css_property.h"
#include "clay/ui/component/intersection_observer_manager.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/scroll_view.h"

#ifndef LYNX_ENABLE_CLAY_NATIVE_LIST
#include "clay/ui/component/list/base_list_view.h"
#endif

namespace clay {

namespace utils = attribute_utils;

namespace {

float TryGetFloatFromMap(const std::string& key, const clay::Value::Map& map,
                         float default_value) {
  const auto& iter = map.find(key);
  if (iter == map.end()) {
    return default_value;
  } else {
    return static_cast<float>(utils::GetDouble(iter->second));
  }
}

clay::Value::Map IntersectionRectToMap(const FloatRect& rect) {
  clay::Value::Map map;
  map.emplace("left", clay::Value(rect.x()));
  map.emplace("top", clay::Value(rect.y()));
  map.emplace("right", clay::Value(rect.width() + rect.x()));
  map.emplace("bottom", clay::Value(rect.height() + rect.y()));
  return map;
}

BaseView* FindAncestorViewByIdSelector(std::string_view id_selector,
                                       BaseView* child) {
  FML_DCHECK(child);

  if (child->GetIdSelector() == id_selector) {
    return child;
  }

  BaseView* parent = child->Parent();
  while (parent) {
    if (parent->GetIdSelector() == id_selector) {
      return parent;
    }
    parent = parent->Parent();
  }

  return nullptr;
}

FloatRect BoundingRectWithScroll(BaseView* view) {
  auto location = view->AbsoluteLocationWithScroll();
  return FloatRect(location.x(), location.y(), view->Width(), view->Height());
}

FloatRect ComputeIntersection(BaseView* target_view, BaseView* root,
                              FloatRect bounding_client_rect,
                              FloatRect relative_rect, bool ui_clip_enabled) {
  if (!target_view || !root || relative_rect.IsEmpty() ||
      bounding_client_rect.IsEmpty()) {
    return FloatRect();
  }

  FloatRect intersection_rect = bounding_client_rect;
  bool at_root = false;
  BaseView* parent = target_view->Parent();
  while (!at_root && parent) {
    if (!parent->Visible()) {
      return FloatRect();
    }

    FloatRect parent_rect;
    if (parent == root) {
      at_root = true;
      parent_rect = relative_rect;
    } else {
      if (parent->GetOverflow() == CSSProperty::OVERFLOW_HIDDEN ||
          parent->Is<ScrollView>() ||
#ifndef LYNX_ENABLE_CLAY_NATIVE_LIST
          parent->Is<BaseListView>() ||
#endif
          ui_clip_enabled) {
        parent_rect = BoundingRectWithScroll(parent);
      }
    }

    if (!parent_rect.IsEmpty()) {
      if (intersection_rect.Intersects(parent_rect)) {
        intersection_rect.Intersect(parent_rect);
      } else {
        intersection_rect = FloatRect();
      }
    }

    parent = parent->Parent();
  }

  return intersection_rect;
}

}  // namespace

void IntersectionObserverEntry::ComputeIntersectionRect(bool ui_clip_enabled) {
  intersection_rect_ =
      ComputeIntersection(target_view_, root_, bounding_client_rect_,
                          relative_rect_, ui_clip_enabled);
}

bool IsViewIntersecting(BaseView* view, BaseView* root, bool ui_clip_enabled) {
  return view && root &&
         !ComputeIntersection(view, root, BoundingRectWithScroll(view),
                              BoundingRectWithScroll(root), ui_clip_enabled)
              .IsEmpty();
}

clay::Value::Map IntersectionObserverEntry::ToMap() {
  clay::Value::Map map;
  map["observerId"] = clay::Value(relative_to_id_);
  map["relativeRect"] = clay::Value(RectToMap(relative_rect_));
  map["boundingClientRect"] = clay::Value(RectToMap(bounding_client_rect_));
  map["intersectionRect"] = clay::Value(RectToMap(intersection_rect_));
  map["intersectionRatio"] = clay::Value(intersection_ratio_);
  map["time"] = clay::Value(time_);
  return map;
}

clay::Value::Map IntersectionObserverEntry::RectToMap(FloatRect rect) {
  clay::Value::Map map;
  map.emplace("left", clay::Value(static_cast<int>(rect.x())));
  map.emplace("right", clay::Value(static_cast<int>(rect.MaxX())));
  map.emplace("top", clay::Value(static_cast<int>(rect.y())));
  map.emplace("bottom", clay::Value(static_cast<int>(rect.MaxY())));
  return map;
}

void IntersectionObserverEntry::ComputeIntersectionRatio() {
  if (intersection_rect_.IsEmpty()) {
    intersection_ratio_ = 0;
    return;
  }

  float target_area =
      bounding_client_rect_.width() * bounding_client_rect_.height();
  float intersection_area =
      intersection_rect_.width() * intersection_rect_.height();

  if (target_area > 0) {
    intersection_ratio_ = intersection_area / target_area;
  } else {
    intersection_ratio_ = 0;
  }
}

IntersectionObserver::IntersectionObserver(IntersectionObserverManager* manager,
                                           const clay::Value::Map& map,
                                           BaseView* view)
    : manager_(manager), attached_view_(view) {
  if (map.find("relativeToIdSelector") == map.end()) {
    root_ = manager->page_view();
  } else {
    auto relative_to_id_selector =
        utils::GetCString(map.at("relativeToIdSelector"));
    if (relative_to_id_selector.length() > 0 &&
        relative_to_id_selector[0] == '#') {
      root_ = FindAncestorViewByIdSelector(relative_to_id_selector.substr(1),
                                           attached_view_);
    } else {
      root_ = manager->page_view();
    }
  }

  margin_left_ = TryGetFloatFromMap("marginLeft", map, 0.f);
  margin_right_ = TryGetFloatFromMap("marginRight", map, 0.f);
  margin_top_ = TryGetFloatFromMap("marginTop", map, 0.f);
  margin_bottom_ = TryGetFloatFromMap("marginBottom", map, 0.f);
  initial_ratio_ = TryGetFloatFromMap("initialRatio", map, 0.f);

  if (map.find("thresholds") != map.end()) {
    const auto& array = utils::GetArray(map.at("thresholds"));
    for (size_t i = 0; i < array.size(); ++i) {
      thresholds_.emplace_back(static_cast<float>(utils::GetDouble(array[i])));
    }
  } else {
    thresholds_.emplace_back(0.f);
  }

  if (map.find("observerAll") == map.end()) {
    observer_all_ = false;
  } else {
    observer_all_ = utils::GetBool(map.at("observerAll"));
  }

  if (map.find("customObserverId") != map.end()) {
    custom_observer_id_ = utils::GetInt(map.at("customObserverId"));
  }
  if (map.find("customCallbackId") != map.end()) {
    custom_callback_id_ = utils::GetInt(map.at("customCallbackId"));
    is_custom_observer_ = true;
  }

  CheckForIntersectionWithTarget();
}

void IntersectionObserver::OnAttach() {
  available_ = true;
  is_initial_ = true;
  CheckForIntersectionWithTarget();
}

void IntersectionObserver::OnDetach() {
  if (!available_) {
    return;
  }
  is_detaching_ = true;
  CheckForIntersectionWithTarget();
  available_ = false;
  is_detaching_ = false;
}

void IntersectionObserver::CheckForIntersectionWithTarget() {
  FML_DCHECK(manager_ && attached_view_);
  if (!available_ || !root_) {
    return;
  }
  old_entry_ = std::move(now_entry_);
  now_entry_ = std::make_unique<IntersectionObserverEntry>();

  FloatRect target_rect =
      is_detaching_ ? FloatRect() : BoundingRectWithScroll(attached_view_);
  FloatRect root_rect = BoundingRectWithScroll(root_);
  root_rect.Expand(margin_left_, margin_right_, margin_top_, margin_bottom_);
  now_entry_->bounding_client_rect_ = target_rect;
  now_entry_->relative_rect_ = root_rect;
  now_entry_->target_view_ = attached_view_;
  now_entry_->root_ = root_;
  now_entry_->ComputeIntersectionRect(exposure_ui_clip_enabled_);
  now_entry_->time_ = 0;  // not support time
  now_entry_->relative_to_id_ = attached_view_->GetIdSelector();
  now_entry_->ComputeIntersectionRatio();

  bool need_notify;
  if (is_initial_) {
    need_notify = initial_ratio_ < now_entry_->intersection_ratio_;
    is_initial_ = false;
  } else {
    need_notify = HasCrossedThreshold();
  }
  if (need_notify) {
    NotifyTarget();
  }
}

void IntersectionObserver::NotifyTarget() {
  if (attached_view_ && attached_view_->page_view()) {
    if (is_custom_observer_) {
      clay::Value::Map map = {};
      map.emplace("isIntersecting", clay::Value(true));
      map.emplace("intersectionRatio",
                  clay::Value(now_entry_->intersection_ratio_));
      map.emplace(
          "intersectionRect",
          clay::Value(IntersectionRectToMap(now_entry_->intersection_rect_)));
      map.emplace("boundingClientRect",
                  clay::Value(IntersectionRectToMap(
                      now_entry_->bounding_client_rect_)));
      map.emplace(
          "relativeRect",
          clay::Value(IntersectionRectToMap(now_entry_->relative_rect_)));
      map.emplace("observerId",
                  clay::Value(now_entry_->target_view_->GetIdSelector()));
      map.emplace("time", clay::Value(now_entry_->time_));
      attached_view_->page_view()->CallJSIntersectionObserver(
          custom_observer_id_, custom_callback_id_,
          clay::Value(std::move(map)));
    } else if (auto* delegate =
                   attached_view_->page_view()->GetEventDelegate()) {
      delegate->OnIntersectionEvent(attached_view_->GetCallbackId(),
                                    now_entry_->ToMap());
    }
  }
}

bool IntersectionObserver::HasCrossedThreshold() {
  float old_ratio = old_entry_ && !old_entry_->intersection_rect_.IsEmpty()
                        ? old_entry_->intersection_ratio_
                        : -1;
  float now_ratio = now_entry_ && !now_entry_->intersection_rect_.IsEmpty()
                        ? now_entry_->intersection_ratio_
                        : -1;
  if (old_ratio == now_ratio) {
    return false;
  }

  for (const auto& threshold : thresholds_) {
    // Return true if an entry matches a threshold or if the new ratio
    // and the old ratio are on the opposite sides of a threshold.
    if (threshold == old_ratio || threshold == now_ratio ||
        ((threshold < old_ratio) != (threshold < now_ratio))) {
      return true;
    }
  }
  return false;
}

IntersectionObserver::~IntersectionObserver() {}

}  // namespace clay
