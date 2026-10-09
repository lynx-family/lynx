// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_EVENT_BUNDLE_H_
#define CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_EVENT_BUNDLE_H_

#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

#include "base/include/value/base_value.h"

namespace lynx {
namespace tasm {

inline bool ParseIntStrict(std::string_view s, int32_t* out) {
  if (out == nullptr || s.empty()) {
    return false;
  }

  bool negative = false;
  size_t i = 0;
  if (s[0] == '-') {
    negative = true;
    i = 1;
  } else if (s[0] == '+') {
    i = 1;
  }

  if (i >= s.size()) {
    return false;
  }

  int64_t acc = 0;
  for (; i < s.size(); ++i) {
    const char c = s[i];
    if (c < '0' || c > '9') {
      return false;
    }
    acc = acc * 10 + (c - '0');

    const int64_t limit =
        negative ? static_cast<int64_t>(std::numeric_limits<int32_t>::max()) + 1
                 : static_cast<int64_t>(std::numeric_limits<int32_t>::max());
    if (acc > limit) {
      return false;
    }
  }

  if (negative) {
    acc = -acc;
  }
  *out = static_cast<int32_t>(acc);
  return true;
}

inline float EventPropValueToFloat(const lepus::Value& value) {
  if (value.IsBool()) {
    return value.Bool() ? 1.0f : 0.0f;
  }

  if (value.IsNumber()) {
    return static_cast<float>(static_cast<int32_t>(value.Number()));
  }

  if (value.IsString()) {
    const std::string str = value.StdString();
    const std::string_view s = str;
    int32_t number = 0;

    if (s.size() >= 2 && s.rfind("px") == s.size() - 2) {
      if (ParseIntStrict(s.substr(0, s.size() - 2), &number)) {
        return static_cast<float>(number);
      }
      return 0.0f;
    }

    if (!s.empty() && s.rfind("%") == s.size() - 1) {
      if (ParseIntStrict(s.substr(0, s.size() - 1), &number)) {
        return static_cast<float>(number) / 100.0f;
      }
      return 0.0f;
    }

    if (ParseIntStrict(s, &number)) {
      return static_cast<float>(number);
    }
  }

  return 0.0f;
}

enum class PlatformEventName : int32_t {
  kUnknown = -1,
  kTouchStart = 0,
  kTouchMove = 1,
  kTouchEnd = 2,
  kTouchCancel = 3,
  kTap = 4,
  kClick = 5,
  kLongPress = 6,
  kUIAppear = 7,
  kUIDisappear = 8,
};

inline PlatformEventName PlatformEventNameFromString(std::string_view name) {
  if (name == "touchstart") {
    return PlatformEventName::kTouchStart;
  }
  if (name == "touchmove") {
    return PlatformEventName::kTouchMove;
  }
  if (name == "touchend") {
    return PlatformEventName::kTouchEnd;
  }
  if (name == "touchcancel") {
    return PlatformEventName::kTouchCancel;
  }
  if (name == "tap") {
    return PlatformEventName::kTap;
  }
  if (name == "click") {
    return PlatformEventName::kClick;
  }
  if (name == "longpress") {
    return PlatformEventName::kLongPress;
  }
  if (name == "uiappear") {
    return PlatformEventName::kUIAppear;
  }
  if (name == "uidisappear") {
    return PlatformEventName::kUIDisappear;
  }
  return PlatformEventName::kUnknown;
}

enum class PlatformEventPropName : int32_t {
  kUnknown = -1,
  kIDSelector = 0,
  kDataset = 1,
  kExposureId = 2,
  kExposureScene = 3,
  kExposureArea = 4,
  kPointerEvents = 5,
  kUserInteractionEnabled = 6,
  kNativeInteractionEnabled = 7,
  kEventThrough = 8,
  kBlockNativeEvent = 9,
  kConsumeSlideEvent = 10,
  kIgnoreFocus = 11,
  kHitSlop = 12,
  kEventsPassThrough = 13,
  kEnableSimultaneousTouch = 14,
  kEnableTouchPseudoPropagation = 15,
  kBlockNativeEventAreas = 16,
  kExposureScreenMarginLeft = 17,
  kExposureScreenMarginRight = 18,
  kExposureScreenMarginTop = 19,
  kExposureScreenMarginBottom = 20,
  kExposureUIMarginLeft = 21,
  kExposureUIMarginRight = 22,
  kExposureUIMarginTop = 23,
  kExposureUIMarginBottom = 24,
  kEnableExposureUIClip = 25,
  kEventThroughActiveRegions = 26,
};

inline PlatformEventPropName PlatformEventPropNameFromString(
    std::string_view name) {
  if (name == "idSelector") {
    return PlatformEventPropName::kIDSelector;
  }
  if (name == "dataset") {
    return PlatformEventPropName::kDataset;
  }
  if (name == "exposure-id") {
    return PlatformEventPropName::kExposureId;
  }
  if (name == "exposure-scene") {
    return PlatformEventPropName::kExposureScene;
  }
  if (name == "exposure-area") {
    return PlatformEventPropName::kExposureArea;
  }
  if (name == "pointer-events") {
    return PlatformEventPropName::kPointerEvents;
  }
  if (name == "user-interaction-enabled") {
    return PlatformEventPropName::kUserInteractionEnabled;
  }
  if (name == "native-interaction-enabled") {
    return PlatformEventPropName::kNativeInteractionEnabled;
  }
  if (name == "event-through") {
    return PlatformEventPropName::kEventThrough;
  }
  if (name == "block-native-event") {
    return PlatformEventPropName::kBlockNativeEvent;
  }
  if (name == "consume-slide-event") {
    return PlatformEventPropName::kConsumeSlideEvent;
  }
  if (name == "ignore-focus") {
    return PlatformEventPropName::kIgnoreFocus;
  }
  if (name == "hit-slop") {
    return PlatformEventPropName::kHitSlop;
  }
  if (name == "events-pass-through") {
    return PlatformEventPropName::kEventsPassThrough;
  }
  if (name == "ios-enable-simultaneous-touch") {
    return PlatformEventPropName::kEnableSimultaneousTouch;
  }
  if (name == "enable-touch-pseudo-propagation") {
    return PlatformEventPropName::kEnableTouchPseudoPropagation;
  }
  if (name == "block-native-event-areas") {
    return PlatformEventPropName::kBlockNativeEventAreas;
  }
  if (name == "exposure-screen-margin-left") {
    return PlatformEventPropName::kExposureScreenMarginLeft;
  }
  if (name == "exposure-screen-margin-right") {
    return PlatformEventPropName::kExposureScreenMarginRight;
  }
  if (name == "exposure-screen-margin-top") {
    return PlatformEventPropName::kExposureScreenMarginTop;
  }
  if (name == "exposure-screen-margin-bottom") {
    return PlatformEventPropName::kExposureScreenMarginBottom;
  }
  if (name == "exposure-ui-margin-left") {
    return PlatformEventPropName::kExposureUIMarginLeft;
  }
  if (name == "exposure-ui-margin-right") {
    return PlatformEventPropName::kExposureUIMarginRight;
  }
  if (name == "exposure-ui-margin-top") {
    return PlatformEventPropName::kExposureUIMarginTop;
  }
  if (name == "exposure-ui-margin-bottom") {
    return PlatformEventPropName::kExposureUIMarginBottom;
  }
  if (name == "enable-exposure-ui-clip") {
    return PlatformEventPropName::kEnableExposureUIClip;
  }
  if (name == "event-through-active-regions") {
    return PlatformEventPropName::kEventThroughActiveRegions;
  }
  return PlatformEventPropName::kUnknown;
}

using PlatformEventPropMap =
    base::InlineOrderedFlatMap<PlatformEventPropName, lepus::Value, 16>;

class PlatformEventBundle {
 public:
  PlatformEventBundle() = default;
  PlatformEventBundle(PlatformEventPropMap event_props,
                      base::Vector<PlatformEventName> event_names)
      : event_props_(std::move(event_props)),
        event_names_(std::move(event_names)) {}

  bool Empty() const { return event_props_.empty() && event_names_.empty(); }

  const PlatformEventPropMap& EventProps() const { return event_props_; }
  const base::Vector<PlatformEventName>& EventNames() const {
    return event_names_;
  }

  void SetEventProps(PlatformEventPropMap event_props) {
    event_props_ = std::move(event_props);
  }
  void SetEventNames(base::Vector<PlatformEventName> event_names) {
    event_names_ = std::move(event_names);
  }

 private:
  PlatformEventPropMap event_props_;
  base::Vector<PlatformEventName> event_names_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_EVENT_BUNDLE_H_
