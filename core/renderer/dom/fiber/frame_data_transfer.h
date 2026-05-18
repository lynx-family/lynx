// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_FRAME_DATA_TRANSFER_H_
#define CORE_RENDERER_DOM_FIBER_FRAME_DATA_TRANSFER_H_

#include <cstdint>
#include <memory>

#include "base/include/value/base_value.h"

namespace lynx {
namespace tasm {

int64_t RegisterFrameDataTransferValue(std::unique_ptr<lepus::Value> value);
std::unique_ptr<lepus::Value> ConsumeFrameDataTransferValue(int64_t handle);

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_FRAME_DATA_TRANSFER_H_
