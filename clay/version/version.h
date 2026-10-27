// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_VERSION_VERSION_H_
#define CLAY_VERSION_VERSION_H_

#include "build/build_config.h"

namespace clay {

const char* GetEngineVersion();

const char* GetSkiaVersion();

const char* GetBuildVersion();

int GetBuildNumber();

}  // namespace clay

#endif  // CLAY_VERSION_VERSION_H_
