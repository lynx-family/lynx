// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_EVENT_CONSUME_SLIDE_EVENT_UTILS_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_EVENT_CONSUME_SLIDE_EVENT_UTILS_H_

#include <cstddef>
#include <vector>

#include "base/include/float_comparison.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/event_target.h"

namespace lynx {
namespace tasm {
namespace harmony {

// Convert inclusive angle ranges, stored as [begin, end, ...], to the ArkUI
// gesture direction used by the existing Harmony consume-slide-event path.
template <typename T>
int ConsumeSlideDirectionMaskFromAngleRanges(const std::vector<T>& ranges) {
  bool octants[8] = {};
  for (std::size_t i = 0; i + 1 < ranges.size(); i += 2) {
    const float begin = (static_cast<double>(ranges[i]) + 180.0) / 45.0;
    const float end = (static_cast<double>(ranges[i + 1]) + 180.0) / 45.0;
    if (!base::FloatsLargerOrEqual(end, begin)) {
      continue;
    }

    // Keep the same octant boundary rules as UIBase's original parser.
    octants[0] |= base::FloatsLargerOrEqual(begin, 0.f) &&
                  base::FloatsLarger(1.f, begin) &&
                  base::FloatsLargerOrEqual(8.f, end);
    for (int octant = 1; octant <= 6; ++octant) {
      const float lower = static_cast<float>(octant);
      const float upper = lower + 1.f;
      octants[octant] |= (base::FloatsLargerOrEqual(begin, lower) &&
                          base::FloatsLarger(upper, begin) &&
                          base::FloatsLargerOrEqual(8.f, end)) ||
                         (base::FloatsLargerOrEqual(begin, 0.f) &&
                          base::FloatsLarger(end, lower) &&
                          base::FloatsLargerOrEqual(upper, end)) ||
                         (base::FloatsLargerOrEqual(lower, begin) &&
                          base::FloatsLargerOrEqual(end, upper));
    }
    octants[7] |= base::FloatsLargerOrEqual(begin, 0.f) &&
                  base::FloatsLarger(end, 7.f) &&
                  base::FloatsLargerOrEqual(8.f, end);
  }

  return (octants[5] || octants[6] ? 1 : 0) |
         (octants[3] || octants[4] ? 2 : 0) |
         (octants[1] || octants[2] ? 4 : 0) |
         (octants[0] || octants[7] ? 8 : 0);
}

inline int ConsumeSlideDirectionMask(ConsumeSlideDirection direction) {
  switch (direction) {
    case ConsumeSlideDirection::kHorizontal:
      return 2 | 8;
    case ConsumeSlideDirection::kVertical:
      return 1 | 4;
    case ConsumeSlideDirection::kUp:
      return 1;
    case ConsumeSlideDirection::kRight:
      return 2;
    case ConsumeSlideDirection::kDown:
      return 4;
    case ConsumeSlideDirection::kLeft:
      return 8;
    case ConsumeSlideDirection::kAll:
      return 15;
    default:
      return 0;
  }
}

inline ConsumeSlideDirection ConsumeSlideDirectionFromMask(int mask) {
  if (mask == 15) {
    return ConsumeSlideDirection::kAll;
  }
  if ((mask & (2 | 8)) == (2 | 8)) {
    return ConsumeSlideDirection::kHorizontal;
  }
  if ((mask & (1 | 4)) == (1 | 4)) {
    return ConsumeSlideDirection::kVertical;
  }
  if (mask & 1) {
    return ConsumeSlideDirection::kUp;
  }
  if (mask & 2) {
    return ConsumeSlideDirection::kRight;
  }
  if (mask & 4) {
    return ConsumeSlideDirection::kDown;
  }
  if (mask & 8) {
    return ConsumeSlideDirection::kLeft;
  }
  return ConsumeSlideDirection::kNone;
}

template <typename T>
ConsumeSlideDirection ConsumeSlideDirectionFromAngleRanges(
    const std::vector<T>& ranges) {
  return ConsumeSlideDirectionFromMask(
      ConsumeSlideDirectionMaskFromAngleRanges(ranges));
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_EVENT_CONSUME_SLIDE_EVENT_UTILS_H_
