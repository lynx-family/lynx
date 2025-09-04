// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_SCROLL_BASE_LYNX_BASE_SCROLL_VIEW_NESTED_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_SCROLL_BASE_LYNX_BASE_SCROLL_VIEW_NESTED_H_

#include <arkui/native_type.h>

#include <string>
#include <utility>
#include <vector>

namespace lynx {
namespace tasm {
namespace harmony {

enum class NestedScrollMode {
  NestedScrollModeSelfOnly = 0,
  NestedScrollModeSelfFirst,
  NestedScrollModeParentFirst,
  NestedScrollModeParallel,
};

class LynxBaseScrollViewNested {
 public:
  virtual NestedScrollMode ForwardsNestedScrollMode() = 0;

  virtual NestedScrollMode BackwardsNestedScrollMode() = 0;

  virtual void SetForwardsNestedScrollMode(NestedScrollMode mode) = 0;

  virtual void SetBackwardsNestedScrollMode(NestedScrollMode mode) = 0;

  virtual ~LynxBaseScrollViewNested() = default;
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_SCROLL_BASE_LYNX_BASE_SCROLL_VIEW_NESTED_H_
