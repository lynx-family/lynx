// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_COMMON_VSYNC_CALLBACK_H_
#define CLAY_COMMON_VSYNC_CALLBACK_H_

#include <cstdint>
#include <functional>

namespace clay {

// Both timestamps are in nanoseconds and use fml::TimePoint::Now()'s clock
// origin.
using VsyncSignalCallback = std::function<void(int64_t frame_start_time_ns,
                                               int64_t frame_target_time_ns)>;
// Requests arrive on the raster thread. The host may reply on any thread.
using VsyncRequestCallback = std::function<void(VsyncSignalCallback)>;

}  // namespace clay

#endif  // CLAY_COMMON_VSYNC_CALLBACK_H_
