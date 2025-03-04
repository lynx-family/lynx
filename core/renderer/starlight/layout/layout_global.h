// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_LAYOUT_LAYOUT_GLOBAL_H_
#define CORE_RENDERER_STARLIGHT_LAYOUT_LAYOUT_GLOBAL_H_

typedef enum {
  SLMeasureModeIndefinite = 0,
  SLMeasureModeDefinite = 1,
  SLMeasureModeAtMost = 2
} SLMeasureMode;

bool IsSLDefiniteMode(SLMeasureMode mode);

bool IsSLIndefiniteMode(SLMeasureMode mode);

bool IsSLAtMostMode(SLMeasureMode mode);

struct FloatSize {
  FloatSize() : width_(0.f), height_(0.f), baseline_(0.f) {}
  FloatSize(float width, float height)
      : width_(width), height_(height), baseline_(0.f) {}
  FloatSize(float width, float height, float baseline)
      : width_(width), height_(height), baseline_(baseline) {}
  float width_;
  float height_;
  float baseline_;
};

typedef void (*SLRequestLayoutFunc)(void* context);

typedef enum { SLLayoutModeWeb = 0 } SLLayoutMode;

typedef struct {
  SLLayoutMode layout_mode_;
} SLConfig;

static constexpr SLConfig SL_CONFIG = {
    .layout_mode_ = SLLayoutModeWeb,
};
#endif  // CORE_RENDERER_STARLIGHT_LAYOUT_LAYOUT_GLOBAL_H_
