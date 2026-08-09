// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_LYNX_VIEW_EVENT_SIMULATION_PROXY_H_
#define PLATFORM_EMBEDDER_LYNX_VIEW_EVENT_SIMULATION_PROXY_H_

#include <memory>
#include <string>

#include "platform/embedder/public/lynx_event_simulation_proxy.h"

namespace lynx {
namespace embedder {

class LynxViewEventSimulationTarget {
 public:
  virtual ~LynxViewEventSimulationTarget() = default;

  virtual int GetNodeForLocation(int x, int y) = 0;
  virtual void SendTouchEvent(const std::string& name, int tag, int x,
                              int y) = 0;
  virtual void EmulateMouseEvent(const std::string& event_name, float x,
                                 float y, float delta_x, float delta_y) = 0;
  virtual void Focus(int node_id) = 0;
  virtual void InsertText(const std::string& text) = 0;
};

class LynxViewEventSimulationProxy final
    : public pub::LynxEventSimulationProxy {
 public:
  explicit LynxViewEventSimulationProxy(
      std::unique_ptr<LynxViewEventSimulationTarget> target);

  void EmulateTouch(const std::string& event_type, int x, int y,
                    const std::string& button, float delta_x, float delta_y,
                    int modifiers, int click_count) override;
  void Focus(int node_id) override;
  void InsertText(const std::string& text) override;

 private:
  static constexpr int kTapSlopPx = 5;

  void EmulateRightMouseEvent(const std::string& event_type, int x, int y,
                              float delta_x, float delta_y);
  void UpdateTapState(int x, int y);
  void ResetTouchState();

  std::unique_ptr<LynxViewEventSimulationTarget> target_;
  bool right_mouse_active_ = false;
  bool touch_active_ = false;
  bool moved_beyond_tap_slop_ = false;
  int active_tag_ = 0;
  int down_x_ = 0;
  int down_y_ = 0;
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_LYNX_VIEW_EVENT_SIMULATION_PROXY_H_
