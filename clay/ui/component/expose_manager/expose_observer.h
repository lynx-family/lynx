// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_EXPOSE_MANAGER_EXPOSE_OBSERVER_H_
#define CLAY_UI_COMPONENT_EXPOSE_MANAGER_EXPOSE_OBSERVER_H_

#include <functional>
#include <string>

#include "clay/public/value.h"
#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/component/css_property.h"
#include "clay/ui/component/intersection_observer.h"

namespace clay {

enum NodeExposureUIMarginEnabled { kUndefined = 0, kEnable = 1, kDisable = 2 };
enum ExposureState { kInit = 0, kExposed = 1, kDisExposed = 2 };
namespace attr = attribute_utils;

struct ExposeAttrs {
  ExposureState expose_state = ExposureState::kInit;

  bool exposure_stoped = false;
  bool exposure_should_notify_appear_ = false;
  bool exposure_should_notify_disappear_ = false;
  // higher priority than pageConfig.exposure_ui_margin_enabled
  NodeExposureUIMarginEnabled exposure_ui_margin_enabled = kUndefined;
  float exposure_screen_margin_left = 0;
  float exposure_screen_margin_right = 0;
  float exposure_screen_margin_top = 0;
  float exposure_screen_margin_bottom = 0;

  attr::Length exposure_ui_margin_left = {0.0, attr::Unit::kNone};
  attr::Length exposure_ui_margin_right = {0.0, attr::Unit::kNone};
  attr::Length exposure_ui_margin_top = {0.0, attr::Unit::kNone};
  attr::Length exposure_ui_margin_bottom = {0.0, attr::Unit::kNone};
  float exposure_area = 0;
  std::string exposure_scene = "";
  std::string exposure_id = "";
};

/**
 *
 *   PageConfig.enableCheckExposureOptimize not implemented Since it is only for
 *iOS optimize.
 *  PageConfig.enableExposureWhenLayout not implemented Since it used to fix
 *android platform issues.
 **/
class ExposeObserver : public IntersectionObserver {
 public:
  ExposeObserver(IntersectionObserverManager* manager,
                 const clay::Value::Map& map, BaseView* view);

  ~ExposeObserver() override = default;

  bool UpdateExposeData(const char* attr, const clay::Value& value);

  void StopExposure(bool send_event);

  void ResumeExposure();

  void CheckForIntersectionWithTarget() override;

 protected:
  bool IsOfType(ObserverType type) const override {
    return type == IntersectionObserver::kExposeObserver;
  };

 private:
  ExposeAttrs expose_attrs_ = {};
  void NotifyAppearEvent(bool appear);
  void NotifyGlobalEvent(bool appear);
  void AssembleDetailData();
  void NotifyTarget() override;
  std::function<void(clay::Value::Map)> custom_event_callback_ = nullptr;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_EXPOSE_MANAGER_EXPOSE_OBSERVER_H_
