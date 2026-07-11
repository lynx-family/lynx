// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_INCLUDE_STARLIGHT_STANDALONE_STARLIGHT_VALUE_H_
#define CORE_INCLUDE_STARLIGHT_STANDALONE_STARLIGHT_VALUE_H_

#include <stdint.h>

#include "starlight_enums.h"

#ifdef __cplusplus
extern "C" {
#endif

// return type of measure function
typedef struct StarlightSize {
  float width_;
  float height_;
} StarlightSize;

// used to get the value of a style getter
typedef struct StarlightValue {
  float value_;
  SLUnit unit_;
  float percentage_;
  int32_t flags_;
} StarlightValue;

#define SLValueFlagHasValue (1 << 0)
#define SLValueFlagHasPercentage (1 << 1)

#define SLUndefined (10E20f)
#define kDefaultPhysicalPixelsPerLayoutUnit (1.f)
#define kStarlightDefaultTargetSDKVersion "3.2"

#ifdef __cplusplus
}
#endif

#endif  // CORE_INCLUDE_STARLIGHT_STANDALONE_STARLIGHT_VALUE_H_
