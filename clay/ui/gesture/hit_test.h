// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_HIT_TEST_H_
#define CLAY_UI_GESTURE_HIT_TEST_H_

#include <functional>
#include <list>
#include <map>
#include <utility>

#include "base/include/fml/memory/weak_ptr.h"
#include "clay/ui/gesture/scrollable_direction.h"

namespace clay {

enum class SlideDirection : uint8_t;
enum class ScrollDirection;
struct PointerEvent;

class HitTestTarget {
 public:
  HitTestTarget() : hit_test_weak_factory_(this) {}

  virtual void HandleEvent(const PointerEvent& event) = 0;

  virtual bool HasDragGestureRecognizer(ScrollDirection direction) const = 0;

  // TODO(zuojinglong.9): Implement a generic method to handle additional
  // gesture recognizers.
  virtual bool HasTapGestureRecognizer() const = 0;
  virtual bool HasLongPressGestureRecognizer() const = 0;

  // TODO(zuojinglong.9): Implement a generic method to handle additional event
  // types.
  virtual bool HasTapEvent() const = 0;
  virtual bool HasLongPressEvent() const = 0;

  virtual bool ShouldBlockNativeEvent() const = 0;

  virtual bool ShouldPassEventToNative() const { return false; }

  virtual bool HasConsumeSlideEventAngles() const { return false; }
  virtual bool ConsumeSlideEvent(float angle) { return false; }
  virtual ScrollableDirection GetScrollableDirection() const {
    return ScrollableDirection::kNone;
  }

  fml::WeakPtr<HitTestTarget> GetHitTestTargetWeakPtr() const {
    return hit_test_weak_factory_.GetWeakPtr();
  }

 protected:
  fml::WeakPtrFactory<HitTestTarget> hit_test_weak_factory_;
};

using HitTestResult = std::list<fml::WeakPtr<HitTestTarget>>;

class HitTestable {
 public:
  virtual bool HitTest(const PointerEvent& event, HitTestResult& result) = 0;
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_HIT_TEST_H_
