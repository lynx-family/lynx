// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_INTERSECTION_OBSERVER_H_
#define CLAY_UI_COMPONENT_INTERSECTION_OBSERVER_H_

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "clay/gfx/geometry/float_rect.h"
#include "clay/public/value.h"
#include "clay/ui/component/css_property.h"

namespace clay {
class IntersectionObserverManager;

class IntersectionObserverEntry {
 public:
  IntersectionObserverEntry() = default;
  ~IntersectionObserverEntry() = default;

  void ComputeIntersectionRatio();
  void ComputeIntersectionRect();

  clay::Value::Map ToMap();
  clay::Value::Map RectToMap(FloatRect rect);

  BaseView* target_view_;
  BaseView* root_;
  FloatRect bounding_client_rect_;
  FloatRect relative_rect_;
  FloatRect intersection_rect_;
  bool has_intersection_ = false;
  float intersection_ratio_;
  float time_;
  std::string relative_to_id_;
};

class IntersectionObserver {
 public:
  IntersectionObserver(IntersectionObserverManager* manager,
                       const clay::Value::Map& map, BaseView* view);
  virtual ~IntersectionObserver();

  BaseView* GetAttachedView() const { return attached_view_; }

  virtual void CheckForIntersectionWithTarget();

  void OnAttach();
  void OnDetach();

  enum ObserverType {
    kIntersectionObserver,
    kExposeObserver,
  };

  virtual bool IsOfType(ObserverType type) const {
    return type == kIntersectionObserver;
  }

 protected:
  bool HasCrossedThreshold();

  virtual void NotifyTarget();

  bool is_custom_observer_ = false;
  int custom_callback_id_ = 0;
  int custom_observer_id_ = 0;

  IntersectionObserverManager* manager_;
  BaseView* root_;
  BaseView* attached_view_;
  float margin_left_;
  float margin_right_;
  float margin_top_;
  float margin_bottom_;
  float initial_ratio_;
  std::vector<float> thresholds_;
  bool observer_all_ = false;  // not support now;
  bool is_initial_ = true;
  bool is_detaching_ = false;
  bool available_ = true;

  std::unique_ptr<IntersectionObserverEntry> old_entry_;
  std::unique_ptr<IntersectionObserverEntry> now_entry_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_INTERSECTION_OBSERVER_H_
