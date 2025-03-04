// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/common/platform_call_back.h"

namespace lynx {
namespace shell {

namespace {
int32_t GetNextTimerIndex() {
  static std::atomic<int> next_timer_index = 0;
  return ++next_timer_index;
}
}  // namespace

PlatformCallBack::PlatformCallBack(DataCallBackType func)
    : func_(std::move(func)), id_(GetNextTimerIndex()) {}

}  // namespace shell
}  // namespace lynx
