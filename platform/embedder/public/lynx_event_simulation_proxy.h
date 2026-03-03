// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_PUBLIC_LYNX_EVENT_SIMULATION_PROXY_H_
#define PLATFORM_EMBEDDER_PUBLIC_LYNX_EVENT_SIMULATION_PROXY_H_

#include <string>

namespace lynx {
namespace pub {

class LynxEventSimulationProxy {
 public:
  static constexpr const char* kMousePressed = "mousePressed";
  static constexpr const char* kMouseMoved = "mouseMoved";
  static constexpr const char* kMouseReleased = "mouseReleased";
  static constexpr const char* kMouseWheel = "mouseWheel";
  static constexpr const char* kMouseLeftButton = "left";
  static constexpr const char* kMouseRightButton = "right";

  virtual ~LynxEventSimulationProxy() = default;
  virtual void EmulateTouch(const std::string& event_type, int x, int y,
                            const std::string& button, float delta_x,
                            float delta_y, int modifiers, int click_count) = 0;
};

}  // namespace pub
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_PUBLIC_LYNX_EVENT_SIMULATION_PROXY_H_
