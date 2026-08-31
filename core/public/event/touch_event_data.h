// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_EVENT_TOUCH_EVENT_DATA_H_
#define CORE_PUBLIC_EVENT_TOUCH_EVENT_DATA_H_

#include <cstdint>

#include "base/include/vector.h"

namespace lynx {
namespace event {

struct TouchEventTargetPoint {
  int32_t element_id;
  float x;
  float y;
};

using TouchEventTargetPoints = base::InlineVector<TouchEventTargetPoint, 2>;

}  // namespace event
}  // namespace lynx

#endif  // CORE_PUBLIC_EVENT_TOUCH_EVENT_DATA_H_
