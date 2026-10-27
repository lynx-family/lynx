// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/expose_manager/expose_observer.h"

#include <initializer_list>
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
#include "core/base/trace/trace_event_def.h"

namespace clay {

namespace {
namespace utils = attribute_utils;

std::string GetTraceDataSetValue(
    const clay::Value& data_set,
    std::initializer_list<const char*> candidate_keys) {
  const auto& map = utils::GetMap(data_set);
  for (const auto* key : candidate_keys) {
    auto value = utils::GetCString(utils::GetMapItem(map, key), "");
    if (!value.empty()) {
      return value;
    }
  }
  return "";
}

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
    NotifyExposureEvent(false, "stop_exposure", 0);
  }
}

void ExposeObserver::ResumeExposure() { expose_attrs_.exposure_stoped = false; }

void ExposeObserver::SetExposureHostVisible(bool visible) {
  TRACE_EVENT("clay", CLAY_EXPOSE_OBSERVER_SET_HOST_VISIBLE, "visible", visible,
              "previous_visible", exposure_host_visible_, "view_id",
              attached_view_ ? attached_view_->id() : -1);
  exposure_host_visible_ = visible;
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
    const auto& data_set = attached_view_->GetDataSet();
    auto page_root_id = GetTraceDataSetValue(
        data_set, {"pageRootId", "page-root-id", "page_root_id"});
    if (!page_root_id.empty()) {
      auto page_view_id = GetTraceDataSetValue(
          data_set, {"pageViewId", "page-view-id", "page_view_id"});
      auto route_id =
          GetTraceDataSetValue(data_set, {"routeId", "route-id", "route_id"});
      auto square_root_id = GetTraceDataSetValue(
          data_set, {"squareRootId", "square-root-id", "square_root_id"});
      auto run_id =
          GetTraceDataSetValue(data_set, {"runId", "run-id", "run_id"});
      TRACE_EVENT_INSTANT(
          "lynx", "LynxExposure.dispatchCustomEvent", "renderer", "clay",
          "event_name", event_name, "sign", attached_view_->GetCallbackId(),
          "exposure_id", expose_attrs_.exposure_id.c_str(), "page_view_id",
          page_view_id.c_str(), "route_id", route_id.c_str(), "page_root_id",
          page_root_id.c_str(), "square_root_id", square_root_id.c_str(),
          "run_id", run_id.c_str());
    }
    attached_view_->page_view()->SendCustomEvent(
        attached_view_->GetCallbackId(), event_name, std::move(params));
  }
}

void ExposeObserver::NotifyGlobalEvent(bool appear, uint64_t trace_flow_id) {
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
  attached_view_->page_view()->AddGlobalExposureEvent(
      appear, std::move(params), attached_view_, trace_flow_id);
}

void ExposeObserver::NotifyExposureEvent(bool appear, const char* reason,
                                         double intersection_ratio) {
  const auto previous_state = expose_attrs_.expose_state;
  const bool has_custom_event = expose_attrs_.exposure_should_notify_appear_ ||
                                expose_attrs_.exposure_should_notify_disappear_;
  const bool has_global_event = !expose_attrs_.exposure_id.empty();
  const uint64_t global_event_flow_id = has_global_event ? TRACE_FLOW_ID() : 0;
  if (appear) {
    TRACE_EVENT_INSTANT(
        "clay", "ClayExposure", "view_id",
        attached_view_ ? attached_view_->id() : -1, "reason", reason,
        "previous_state", previous_state, "intersection_ratio",
        intersection_ratio, "is_detaching", is_detaching_, "host_visible",
        exposure_host_visible_, "custom_event", has_custom_event,
        "global_event", has_global_event,
        [global_event_flow_id](lynx::perfetto::EventContext ctx) {
          if (global_event_flow_id != 0) {
            ctx.event()->add_flow_ids(global_event_flow_id);
          }
        });
  } else {
    TRACE_EVENT_INSTANT(
        "clay", "ClayDisexposure", "view_id",
        attached_view_ ? attached_view_->id() : -1, "reason", reason,
        "previous_state", previous_state, "intersection_ratio",
        intersection_ratio, "is_detaching", is_detaching_, "host_visible",
        exposure_host_visible_, "custom_event", has_custom_event,
        "global_event", has_global_event,
        [global_event_flow_id](lynx::perfetto::EventContext ctx) {
          if (global_event_flow_id != 0) {
            ctx.event()->add_flow_ids(global_event_flow_id);
          }
        });
  }
  if (has_custom_event) {
    NotifyAppearEvent(appear);
  }
  if (has_global_event) {
    NotifyGlobalEvent(appear, global_event_flow_id);
  } else {
    FML_DLOG(ERROR) << " fail notify by exposure id null, view "
                    << attached_view_
                    << "view id: " << attached_view_->GetIdSelector();
  }
  expose_attrs_.expose_state =
      appear ? ExposureState::kExposed : ExposureState::kDisExposed;
}

void ExposeObserver::NotifyTarget() {
  double ratio = 0;
  auto map = now_entry_->ToMap();
  const auto it = map.find("intersectionRatio");
  if (it != map.end()) {
    attribute_utils::TryGetNum(it->second, ratio, 0);
  }
  NotifyExposureEvent(ratio > 0, is_detaching_ ? "detach" : "intersection",
                      ratio);
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
  TRACE_EVENT("clay", CLAY_EXPOSE_OBSERVER_CHECK_INTERSECTION, "view_id",
              attached_view_->id());
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
                   expose_attrs_.exposure_screen_margin_right);
  now_entry_->bounding_client_rect_ = target_rect;
  now_entry_->relative_rect_ = root_rect;
  now_entry_->target_view_ = attached_view_;
  now_entry_->root_ = root_;
  now_entry_->ComputeIntersectionRect(exposure_ui_clip_enabled_);
  now_entry_->time_ = 0;  // not support time
  now_entry_->relative_to_id_ = attached_view_->GetIdSelector();
  now_entry_->ComputeIntersectionRatio();

  bool need_notify = false;
  switch (expose_attrs_.expose_state) {
    case kInit:
    case kDisExposed:
      need_notify = now_entry_->intersection_ratio_ > 0;
      break;
    case kExposed:
      need_notify = now_entry_->intersection_ratio_ <= 0;
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
    if (!input_copy.empty()) {
      char* endptr = nullptr;
      errno = 0;
      [[maybe_unused]] double area_double = strtod(input_copy.c_str(), &endptr);
      if (endptr) {
        std::string_view str_endptr(endptr, strlen(endptr));
        if ((input_copy.c_str() + input_copy.size() != endptr) &&
            (str_endptr == "%")) {
          expose_attrs_.exposure_area = area_double / 100;
          thresholds_.push_back(expose_attrs_.exposure_area);
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
