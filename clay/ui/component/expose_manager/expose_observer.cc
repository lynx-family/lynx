// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/expose_manager/expose_observer.h"

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/string/string_utils.h"
#include "base/trace/native/trace_event.h"
#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/common/value_utils.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/intersection_observer.h"
#include "clay/ui/component/page_view.h"

namespace clay {

namespace {
namespace utils = attribute_utils;
FloatRect BoundingRectWithScroll(BaseView* view) {
  auto location = view->AbsoluteLocationWithScroll();
  return FloatRect(location.x(), location.y(), view->Width(), view->Height());
}
float ParesLengthParam(const clay::Value& value, PageView* page_view) {
  std::string length_str = utils::GetCString(value, "");
  if (length_str.empty()) {
    return 0;
  }
  return utils::ToPxWithDisplayMetrics(length_str, page_view);
}

float ParseLengthWithPercent(const attr::Length& value, PageView* page_view,
                             float view_length) {
  if (value.unit == attr::Unit::kNone || view_length <= 0) {
    return 0;
  }
  return utils::ToPxWithDisplayMetrics(value, page_view, view_length);
}

}  // namespace

namespace detail_attr {
const char* kExposureScene = "exposure-scene";
const char* kExposureId = "exposure-id";
const char* kSign = "sign";
const char* kDataSet = "dataset";
const char* kUniqueId = "unique-id";
// in order to compatable with old lynx versions
const char* kExposureSceneCompat = "exposureScene";
const char* kExposureIdCompat = "exposureID";
const char* kDataSetCompat = "dataSet";

}  // namespace detail_attr

/**
 * Detail data structure :
 *  {
 *      "exposure-scene": xxx,
 *      "exposure-id": xxx,
 *      "dataset": {...},
 *      "unique-id":xxx
 *      "extra-data": always null in lynx android, ignore here
 *      "sign": xxx,
 *      //.......
 *    }
 *  refer to:
 *  https://lynxjs.org/api/lynx-api/event/global-event#exposure
 */
ExposeObserver::ExposeObserver(IntersectionObserverManager* manager,
                               const clay::Value::Map& map, BaseView* view)
    : IntersectionObserver(manager, map, view) {}

void ExposeObserver::StopExposure(bool send_event) {
  const bool should_notify_disappear =
      send_event && expose_attrs_.expose_state == kExposed;
  expose_attrs_.exposure_stoped = true;
  if (should_notify_disappear) {
    NotifyExposureEvent(false);
  }
}

void ExposeObserver::ResumeExposure() { expose_attrs_.exposure_stoped = false; }

void ExposeObserver::SetExposureHostVisible(bool visible) {
  exposure_host_visible_ = visible;
}

bool ExposeObserver::TryNotifyAppearWithoutGeometry() {
  if (!available_ || !root_ || expose_attrs_.exposure_stoped ||
      !exposure_host_visible_ || expose_attrs_.exposure_area > 0 ||
      !expose_attrs_.exposure_should_notify_appear_ || IsExposed()) {
    return false;
  }
  NotifyExposureEvent(true);
  return true;
}

void ExposeObserver::NotifyAppearEvent(bool appear) {
  if (attached_view_ && attached_view_->page_view()) {
    const char* event_name = appear ? "uiappear" : "uidisappear";
    clay::Value::Map params;
    params[detail_attr::kSign] = clay::Value(attached_view_->GetCallbackId());
    params[detail_attr::kExposureId] = clay::Value(expose_attrs_.exposure_id);
    params[detail_attr::kExposureIdCompat] =
        clay::Value(expose_attrs_.exposure_id);
    params[detail_attr::kExposureScene] =
        clay::Value(expose_attrs_.exposure_scene);
    params[detail_attr::kExposureSceneCompat] =
        clay::Value(expose_attrs_.exposure_scene);
    params[detail_attr::kUniqueId] =
        clay::Value(attached_view_->GetCallbackId());
    params[detail_attr::kDataSet] =
        CloneClayValue(attached_view_->GetDataSet());
    params[detail_attr::kDataSetCompat] =
        CloneClayValue(attached_view_->GetDataSet());
    auto* page_view = attached_view_->page_view();
    const auto& page_children = page_view->GetChildren();
    const bool is_page_first_child = attached_view_->Parent() == page_view &&
                                     !page_children.empty() &&
                                     page_children.front() == attached_view_;
    EventDispatchOptions dispatch_options{.emergency =
                                              appear && is_page_first_child};
    page_view->SendCustomEvent(attached_view_->GetCallbackId(), event_name,
                               std::move(params), dispatch_options);
  }
}

void ExposeObserver::NotifyGlobalEvent(bool appear) {
  if (!attached_view_ || !attached_view_->page_view()) {
    return;
  }
  auto params = std::make_unique<clay::Value::Map>();
  params->emplace(detail_attr::kExposureId,
                  clay::Value(expose_attrs_.exposure_id));
  params->emplace(detail_attr::kExposureIdCompat,
                  clay::Value(expose_attrs_.exposure_id));
  params->emplace(detail_attr::kExposureScene,
                  clay::Value(expose_attrs_.exposure_scene));
  params->emplace(detail_attr::kExposureSceneCompat,
                  clay::Value(expose_attrs_.exposure_scene));
  params->emplace(detail_attr::kSign,
                  clay::Value(attached_view_->GetCallbackId()));
  params->emplace(detail_attr::kUniqueId,
                  clay::Value(attached_view_->GetCallbackId()));
  attached_view_->page_view()->AddGlobalExposureEvent(appear, std::move(params),
                                                      attached_view_);
}

void ExposeObserver::NotifyExposureEvent(bool appear) {
  TRACE_EVENT("clay", CLAY_EXPOSE_OBSERVER_NOTIFY_EXPOSURE_EVENT, "appear",
              appear, "view_id", attached_view_ ? attached_view_->id() : -1);
  if (expose_attrs_.exposure_should_notify_appear_ ||
      expose_attrs_.exposure_should_notify_disappear_) {
    NotifyAppearEvent(appear);
  }
  if (!expose_attrs_.exposure_id.empty()) {
    NotifyGlobalEvent(appear);
  } else {
    FML_DLOG(ERROR) << " fail notify by exposure id null, view "
                    << attached_view_
                    << "view id: " << attached_view_->GetIdSelector();
  }
  expose_attrs_.expose_state =
      appear ? ExposureState::kExposed : ExposureState::kDisExposed;
}

bool ExposeObserver::IsExposed(double intersection_ratio,
                               double minimum_clipping_ratio) const {
  return expose_attrs_.exposure_area <= 0
             ? intersection_ratio > 0
             : minimum_clipping_ratio >= expose_attrs_.exposure_area;
}

void ExposeObserver::NotifyTarget() {
  NotifyExposureEvent(IsExposed(now_entry_->intersection_ratio_,
                                now_entry_->minimum_clipping_ratio_));
}

void ExposeObserver::CheckForIntersectionWithTarget() {
  FML_DCHECK(manager_ && attached_view_);
  if (!available_ || !root_ || expose_attrs_.exposure_stoped ||
      (expose_attrs_.exposure_id.empty() &&
       !expose_attrs_.exposure_should_notify_appear_ &&
       !expose_attrs_.exposure_should_notify_disappear_)) {
    return;
  }
  // Window detachment only suspends exposure detection and preserves the
  // current state. A real node detachment must still close an active exposure.
  if (!exposure_host_visible_ && !is_detaching_) {
    return;
  }
  TRACE_EVENT("clay", CLAY_EXPOSE_OBSERVER_CHECK_FOR_INTERSECTION_WITH_TARGET,
              "view_id", attached_view_->id());
  now_entry_ = std::make_unique<IntersectionObserverEntry>();

  FloatRect target_rect =
      is_detaching_ ? FloatRect() : BoundingRectWithScroll(attached_view_);
  FloatRect root_rect = BoundingRectWithScroll(root_);
  if (!is_detaching_ &&
      (expose_attrs_.exposure_ui_margin_enabled == kUndefined
           ? manager_->GetExposureUIMarginEnabled()
           : expose_attrs_.exposure_ui_margin_enabled == kEnable)) {
    auto page_view = this->attached_view_->page_view();
    float top = ParseLengthWithPercent(expose_attrs_.exposure_ui_margin_top,
                                       page_view, target_rect.height());
    float right = ParseLengthWithPercent(expose_attrs_.exposure_ui_margin_right,
                                         page_view, target_rect.width());
    float bottom =
        ParseLengthWithPercent(expose_attrs_.exposure_ui_margin_bottom,
                               page_view, target_rect.height());
    float left = ParseLengthWithPercent(expose_attrs_.exposure_ui_margin_left,
                                        page_view, target_rect.width());
    target_rect.Expand(top, right, bottom, left);
  }
  root_rect.Expand(expose_attrs_.exposure_screen_margin_top,
                   expose_attrs_.exposure_screen_margin_right,
                   expose_attrs_.exposure_screen_margin_bottom,
                   expose_attrs_.exposure_screen_margin_left);
  now_entry_->bounding_client_rect_ = target_rect;
  now_entry_->relative_rect_ = root_rect;
  now_entry_->target_view_ = attached_view_;
  now_entry_->root_ = root_;
  now_entry_->ComputeIntersectionRect(exposure_ui_clip_enabled_, true);
  now_entry_->time_ = 0;  // not support time
  now_entry_->relative_to_id_ = attached_view_->GetIdSelector();
  now_entry_->ComputeIntersectionRatio();

  bool need_notify = false;
  const bool exposed = IsExposed(now_entry_->intersection_ratio_,
                                 now_entry_->minimum_clipping_ratio_);
  switch (expose_attrs_.expose_state) {
    case kInit:
    case kDisExposed:
      need_notify = exposed;
      break;
    case kExposed:
      need_notify = !exposed;
  }
  if (need_notify) {
    NotifyTarget();
  }
}

bool ExposeObserver::UpdateExposeData(const char* attr,
                                      const clay::Value& value) {
  if (!this->attached_view_ || !this->attached_view_->page_view()) {
    return false;
  }
  auto page_view = this->attached_view_->page_view();
  auto kw = GetKeywordID(attr);
  if (KeywordID::kExposureId == kw) {
    auto id = utils::GetCString(value, "");
    if (!id.empty() && id != expose_attrs_.exposure_id) {
      // send old disexposure first when changed
      if (!expose_attrs_.exposure_id.empty()) {
        NotifyGlobalEvent(false);
      }
      // treat as new added on next frame
      if (attached_view_ && attached_view_->page_view()) {
        expose_attrs_.expose_state = kInit;
        attached_view_->page_view()->RequestPaint();
      }
    }
    expose_attrs_.exposure_id = id;
    return true;
  } else if (KeywordID::kExposureScene == kw) {
    auto exposure_scene = utils::GetCString(value, "");
    if (exposure_scene != expose_attrs_.exposure_scene) {
      // send old disexposure first when changed
      if (!expose_attrs_.exposure_id.empty()) {
        NotifyGlobalEvent(false);
      }
      // treat as new added on next frame
      if (attached_view_ && attached_view_->page_view()) {
        expose_attrs_.expose_state = kInit;
        attached_view_->page_view()->RequestPaint();
      }
    }
    expose_attrs_.exposure_scene = exposure_scene;
    return true;
  } else if (KeywordID::kExposureArea == kw) {
    std::string area_str = utils::GetCString(value, "");
    std::string input_copy = lynx::base::TrimString(area_str);
    expose_attrs_.exposure_area = 0;
    if (!input_copy.empty()) {
      char* endptr = nullptr;
      errno = 0;
      double area_double = strtod(input_copy.c_str(), &endptr);
      if (errno == 0 && std::isfinite(area_double) &&
          endptr != input_copy.c_str()) {
        std::string_view str_endptr(endptr, strlen(endptr));
        if (str_endptr == "%") {
          expose_attrs_.exposure_area = static_cast<float>(area_double / 100);
        }
      }
    }
    return true;
  } else if (KeywordID::kEnableExposureUiMargin == kw) {
    if (utils::GetBool(value, false)) {
      expose_attrs_.exposure_ui_margin_enabled =
          NodeExposureUIMarginEnabled::kEnable;
    } else {
      expose_attrs_.exposure_ui_margin_enabled =
          NodeExposureUIMarginEnabled::kDisable;
    }
    return true;
  } else if (KeywordID::kEnableExposureUiClip == kw) {
    if (utils::GetBool(value, false)) {
      exposure_ui_clip_enabled_ = true;
    } else {
      exposure_ui_clip_enabled_ = false;
    }
    return true;
  } else if (KeywordID::kExposureScreenMarginLeft == kw) {
    expose_attrs_.exposure_screen_margin_left =
        ParesLengthParam(value, page_view);
    return true;
  } else if (KeywordID::kExposureScreenMarginRight == kw) {
    expose_attrs_.exposure_screen_margin_right =
        ParesLengthParam(value, page_view);
    return true;
  } else if (KeywordID::kExposureScreenMarginTop == kw) {
    expose_attrs_.exposure_screen_margin_top =
        ParesLengthParam(value, page_view);
    return true;
  } else if (KeywordID::kExposureScreenMarginBottom == kw) {
    expose_attrs_.exposure_screen_margin_bottom =
        ParesLengthParam(value, page_view);
    return true;
  } else if (KeywordID::kExposureUiMarginLeft == kw) {
    if (!attr::TryGetLength(value, expose_attrs_.exposure_ui_margin_left)) {
      expose_attrs_.exposure_ui_margin_left = {0, attribute_utils::kNone};
    }
    return true;
  } else if (KeywordID::kExposureUiMarginRight == kw) {
    if (!attr::TryGetLength(value, expose_attrs_.exposure_ui_margin_right)) {
      expose_attrs_.exposure_ui_margin_right = {0, attribute_utils::kNone};
    }
    return true;
  } else if (KeywordID::kExposureUiMarginTop == kw) {
    if (!attr::TryGetLength(value, expose_attrs_.exposure_ui_margin_top)) {
      expose_attrs_.exposure_ui_margin_top = {0, attribute_utils::kNone};
    }
    return true;
  } else if (KeywordID::kExposureUiMarginBottom == kw) {
    if (!attr::TryGetLength(value, expose_attrs_.exposure_ui_margin_bottom)) {
      expose_attrs_.exposure_ui_margin_bottom = {0, attribute_utils::kNone};
    }
    return true;
  } else if (KeywordID::kUiappear == kw) {
    expose_attrs_.exposure_should_notify_appear_ = true;
    return true;
  } else if (KeywordID::kUidisappear == kw) {
    expose_attrs_.exposure_should_notify_disappear_ = true;
    return true;
  } else {
    return false;
  }
}

}  // namespace clay
