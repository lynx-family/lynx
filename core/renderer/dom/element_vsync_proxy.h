// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_ELEMENT_VSYNC_PROXY_H_
#define CORE_RENDERER_DOM_ELEMENT_VSYNC_PROXY_H_

#include <memory>
#include <set>
#include <string>

#include "base/include/fml/time/time_point.h"
constexpr char kPreferredFpsHigh[] = "high";
constexpr char kPreferredFpsAuto[] = "auto";
constexpr char kPreferredFpsLow[] = "low";

static fml::TimeDelta LOW_FRAME_DURATION =
    fml::TimeDelta::FromSecondsF(1 / 30.0);

namespace lynx {
namespace shell {
class VSyncMonitor;
}
namespace tasm {
class ElementManager;

class ElementVsyncProxy
    : public std::enable_shared_from_this<ElementVsyncProxy> {
 public:
  ElementVsyncProxy(
      ElementManager *element_manager,
      const std::shared_ptr<shell::VSyncMonitor> &vsync_monitor = nullptr);
  ~ElementVsyncProxy() = default;

  // Tick all element of set.
  void TickAllElement(fml::TimePoint &time);

  void RequestNextFrame();

  void MarkNextFrameHasArrived() { has_requested_next_frame_ = false; }

  bool HasRequestedNextFrame() { return has_requested_next_frame_; }

  void SetPreferredFps(const std::string &preferred_fps);

  inline void set_preferred_fps(const std::string &preferred_fps) {
    preferred_fps_ = preferred_fps;
  }

  inline std::string preferred_fps() { return preferred_fps_; }

  inline void set_last_tick_time(const fml::TimePoint &t) {
    last_tick_time_ = t;
  }

  inline fml::TimePoint last_tick_time() { return last_tick_time_; }

 private:
  // It marks whether has requested next frame time.
  bool has_requested_next_frame_ = false;
  ElementManager *element_manager_;
  // TODO(wujintian): move the member variable to element manager. Then some
  // member function such as `RegistToSet` and `NotifyElementDestroy` and
  // `GetAnimationElements` can be removed.
  std::shared_ptr<shell::VSyncMonitor> vsync_monitor_{nullptr};

  // NewAnimator preferred Fps
  std::string preferred_fps_ = kPreferredFpsAuto;
  // Record last animation tick time.
  fml::TimePoint last_tick_time_ = fml::TimePoint();
};

}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_DOM_ELEMENT_VSYNC_PROXY_H_
