// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_EVENT_PSEUDO_STATUS_H_
#define CORE_PUBLIC_EVENT_PSEUDO_STATUS_H_

#include <cstdint>

namespace lynx {
namespace event {

struct PseudoStatusChange {
  int32_t id;
  uint32_t pre_status;
  uint32_t current_status;
};

}  // namespace event
}  // namespace lynx

#endif  // CORE_PUBLIC_EVENT_PSEUDO_STATUS_H_
