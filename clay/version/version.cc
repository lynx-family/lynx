// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// FLUTTER_NOLINT: https://github.com/flutter/flutter/issues/68332

#include "clay/version/version.h"

#if !(OS_IOS && ENABLE_SKITY)
#include "clay/version/version.inc"
#endif

namespace clay {

const char* GetEngineVersion() { return ENGINE_VERSION; }

const char* GetSkiaVersion() { return SKIA_VERSION; }

const char* GetBuildVersion() { return CLAY_BUILD_VERSION; }

int GetBuildNumber() { return CLAY_BUILD_NUMBER; }

}  // namespace clay
