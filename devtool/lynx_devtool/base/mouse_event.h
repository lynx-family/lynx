// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_BASE_MOUSE_EVENT_H_
#define DEVTOOL_LYNX_DEVTOOL_BASE_MOUSE_EVENT_H_

#include <string>

namespace lynx {
namespace devtool {
struct MouseEvent {
  std::string type_;
  int x_;
  int y_;
  std::string button_;
  float delta_x_;
  float delta_y_;
  int modifiers_;
  int click_count_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_BASE_MOUSE_EVENT_H_
