// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/auto_scroller.h"

#include "core/renderer/dom/lynx_get_ui_result.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_list.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"

namespace lynx {
namespace tasm {
namespace harmony {
AutoScroller::AutoScroller(BaseScrollContainer* scroll_container)
    : scroll_container_(scroll_container) {}

void OnVSync(const std::weak_ptr<AutoScroller>& weak_this,
             int64_t frame_start_time_ns, int64_t frame_target_time_ns) {
  auto auto_scroller = weak_this.lock();
  if (!auto_scroller || auto_scroller->GetScrollContainer() == nullptr) {
    return;
  }
  auto_scroller->AutoScrollInternal(frame_start_time_ns, frame_target_time_ns);
}

void AutoScroller::AutoScrollInternal(int64_t frame_start_time_ns,
                                      int64_t frame_target_time_ns) {
  auto result = scroll_container_->GetScrollOffset();
  if (auto_stop_) {
    if (scroll_container_->IsAtEnd() && rate_ >= 0) {
      block_auto_scroll_ = false;
    } else if (scroll_container_->GetScrollDistance() <= 0 && rate_ <= 0) {
      block_auto_scroll_ = false;
    }
  }
  // reach the edge and the auto_stop is true, should intercept the
  // frameCallback
  if (!block_auto_scroll_) {
    if (scroll_container_) {
      scroll_container_->AutoScrollStopped();
    }
    return;
  }
  float offset_x{result.first}, offset_y{result.second};
  if (last_frame_start_time_ns_ == 0) {
    last_frame_start_time_ns_ = frame_target_time_ns;
  }
  double dt =
      static_cast<double>(frame_start_time_ns - last_frame_start_time_ns_) /
      1e9;
  if (dt <= 0.0) {
    dt = 1.0 / 60.0;
  }
  last_frame_start_time_ns_ = frame_start_time_ns;
  if (scroll_container_->IsHorizontal()) {
    offset_x = result.first + rate_ * static_cast<float>(dt);
  } else {
    offset_y = result.second + rate_ * static_cast<float>(dt);
  }
  // scroll
  NodeManager::Instance().SetAttributeWithNumberValue(
      scroll_container_->Node(), NODE_SCROLL_OFFSET, offset_x, offset_y, 0,
      static_cast<int>(ArkUI_AnimationCurve::ARKUI_CURVE_SMOOTH), 0);

  // request next frame
  auto vsync_monitor = scroll_container_->GetContext()->VSyncMonitor();
  if (vsync_monitor) {
    vsync_monitor->ScheduleVSyncSecondaryCallback(
        reinterpret_cast<uintptr_t>(this),
        [weak_this = weak_from_this()](int64_t start_ts, int64_t target_ts) {
          OnVSync(weak_this, start_ts, target_ts);
        });
  }
}

void AutoScroller::AutoScroll(
    float rate, bool start, bool auto_stop,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  auto_stop_ = auto_stop;
  rate_ = rate;
  last_frame_start_time_ns_ = 0;
  if (start) {
    if (rate == 0) {
      callback(
          LynxGetUIResult::PARAM_INVALID,
          lepus_value("the rate of speed is not right, current value is 0 !"));
      return;
    }
    if (block_auto_scroll_) {
      callback(LynxGetUIResult::PARAM_INVALID,
               lepus_value("the scroll container is scrolling!"));
      return;
    }
    block_auto_scroll_ = true;
    auto vsync_monitor = scroll_container_->GetContext()->VSyncMonitor();
    if (vsync_monitor) {
      vsync_monitor->ScheduleVSyncSecondaryCallback(
          reinterpret_cast<uintptr_t>(this),
          [weak_this = weak_from_this()](int64_t start_ts, int64_t target_ts) {
            OnVSync(weak_this, start_ts, target_ts);
          });
    }
  } else {
    block_auto_scroll_ = false;
  }
}

AutoScroller::~AutoScroller() {}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
