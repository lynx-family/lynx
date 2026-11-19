// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_VSYNC_FRAME_TICK_WORK_H_
#define BASE_INCLUDE_VSYNC_FRAME_TICK_WORK_H_

namespace lynx {
namespace base {

class VSyncFrameTickWork {
 public:
  virtual ~VSyncFrameTickWork() = default;
  virtual void Complete() = 0;
};

}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_VSYNC_FRAME_TICK_WORK_H_
