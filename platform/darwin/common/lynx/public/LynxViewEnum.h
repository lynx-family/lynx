// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/**
 * This enum is used to define size measure mode of LynxView.
 * If mode is Undefined, the size will be determined by the content.
 * If mode is Exact, the size will be the size set by outside.
 * If mode is Max, the size will be determined by the content, but not exceed
 * the maximum size.
 */
typedef NS_ENUM(NSInteger, LynxViewSizeMode) {
  LynxViewSizeModeUndefined = 0,
  LynxViewSizeModeExact,
  LynxViewSizeModeMax
};

// now only support LynxThreadStrategyForRenderAllOnUI!
typedef NS_ENUM(NSInteger, LynxThreadStrategyForRender) {
  LynxThreadStrategyForRenderAllOnUI = 0,
  LynxThreadStrategyForRenderMostOnTASM = 1,
  LynxThreadStrategyForRenderPartOnLayout = 2,
  LynxThreadStrategyForRenderMultiThreads = 3,
};
