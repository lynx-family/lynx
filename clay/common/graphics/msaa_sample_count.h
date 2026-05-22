// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_COMMON_GRAPHICS_MSAA_SAMPLE_COUNT_H_
#define CLAY_COMMON_GRAPHICS_MSAA_SAMPLE_COUNT_H_

// Supported MSAA sample count values.
enum class MsaaSampleCount {
  kNone = 1,
  kTwo = 2,
  kFour = 4,
  kEight = 8,
  kSixteen = 16,
};

#endif  // CLAY_COMMON_GRAPHICS_MSAA_SAMPLE_COUNT_H_
