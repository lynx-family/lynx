// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_VIEW_CALLBACK_SCROLL_EVENT_CALLBACK_MANAGER_H_
#define CLAY_UI_COMPONENT_VIEW_CALLBACK_SCROLL_EVENT_CALLBACK_MANAGER_H_

#include <string>

#include "clay/gfx/scroll_direction.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/page_view.h"

namespace clay {

class ScrollEventCallbackManager {
 public:
  // Because of the wrapper mechanism, the wrapper's id is used when sending
  // callbacks so we cannot use view->id() directly.
  ScrollEventCallbackManager(BaseView* view, int32_t callback_id);
  virtual ~ScrollEventCallbackManager();

  class BorderStatus {
   public:
    BorderStatus() : state_(State::kBorderNone) {}
    bool IsUpper() const {
      return (state_ & State::kBorderUpper) == State::kBorderUpper;
    }
    bool IsLower() const {
      return (state_ & State::kBorderLower) == State::kBorderLower;
    }
    void SetUpper(bool enable) {
      if (enable) {
        state_ = static_cast<BorderStatus::State>(state_ | kBorderUpper);
      } else {
        state_ = static_cast<BorderStatus::State>(state_ & kBorderLower);
      }
    }
    void SetLower(bool enable) {
      if (enable) {
        state_ = static_cast<BorderStatus::State>(state_ | kBorderLower);
      } else {
        state_ = static_cast<BorderStatus::State>(state_ & kBorderUpper);
      }
    }

   private:
    enum State {
      kBorderNone = 0,
      kBorderUpper = 1 << 0,
      kBorderLower = 1 << 1,
      kBorderBoth = kBorderUpper | kBorderLower,
    };
    State state_;
  };

  enum class ScrollState {
    kIdle = 1,   // Not scrolling
    kDragging,   // Dragged by outside input
    kAnimating,  // Animating to a final position while not under outside
                 // control
  };

  virtual bool AddEventCallback(const std::string& event) {
    if (event.compare(event_attr::kEventScroll) == 0) {
      EnableSendEvent(ScrollEvents::kScrollEvent);
      return true;
    } else if (event.compare(event_attr::kEventScrollToUpper) == 0) {
      EnableSendEvent(ScrollEvents::kScrollToUpper);
      return true;
    } else if (event.compare(event_attr::kEventScrollToLower) == 0) {
      EnableSendEvent(ScrollEvents::kScrollToLower);
      return true;
    } else if (event.compare(event_attr::kEventScrollEnd) == 0) {
      EnableSendEvent(ScrollEvents::kScrollEnd);
      return true;
    } else if (event.compare(event_attr::kEventScrollStateChange) == 0) {
      EnableSendEvent(ScrollEvents::kScrollStateChange);
      return true;
    }
    return false;
  }

  void SetScrollEventThrottle(fml::TimeDelta throttle) {
    scroll_event_throttle_ = throttle;
  }

  virtual void NotifyScrolled(const FloatSize& scrolled,
                              const FloatSize& offset,
                              const BorderStatus& current_status,
                              const bool is_dragging = false);
  void NotifyScrollEnd(const FloatSize& offset) const;

  void NotifyScrollStateChange(ScrollState old_state, ScrollState current_state,
                               float velocity, bool is_dragging) const;

 protected:
  void HandleScrolled(const FloatSize& scrolled, const FloatSize& offset,
                      const BorderStatus& current_status,
                      bool const is_dragging = false);

  virtual void SendScrollEvent(const char* event_name,
                               const FloatSize& scrolled,
                               const FloatSize& offset,
                               const FloatSize& content,
                               const bool is_dragging = false) const;

  PageView* page_view_;
  BaseView* view_;
  int32_t callback_id_;

 private:
  enum ScrollEvents {
    kNone = 0,
    kScrollEvent = 1 << 0,
    kScrollToUpper = 1 << 1,
    kScrollToLower = 1 << 2,
    kScrollEnd = 1 << 3,
    kScrollStateChange = 1 << 4,
  };

  bool ShouldSendEvent(ScrollEvents event) const { return event_flag_ & event; }

  void EnableSendEvent(ScrollEvents event) {
    event_flag_ = static_cast<ScrollEvents>(event_flag_ | event);
  }

  fml::TimeDelta scroll_event_throttle_;
  fml::TimePoint last_scroll_event_;
  ScrollEvents event_flag_ = ScrollEvents::kNone;
  BorderStatus last_status_;
};

}  // namespace clay
#endif  // CLAY_UI_COMPONENT_VIEW_CALLBACK_SCROLL_EVENT_CALLBACK_MANAGER_H_
