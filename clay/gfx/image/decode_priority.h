// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_DECODE_PRIORITY_H_
#define CLAY_GFX_IMAGE_DECODE_PRIORITY_H_

enum class DecodePriority {
  kPending,    // Do not trigger decode
  kDeferred,   // Trigger decode later
  kImmediate,  // Trigger decode immediately
};

#endif  // CLAY_GFX_IMAGE_DECODE_PRIORITY_H_
