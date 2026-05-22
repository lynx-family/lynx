// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FML_PLATFORM_DARWIN_PLATFORM_VERSION_H_
#define CLAY_FML_PLATFORM_DARWIN_PLATFORM_VERSION_H_

#include <sys/types.h>

#include "base/include/fml/macros.h"

namespace fml {

bool IsPlatformVersionAtLeast(size_t major, size_t minor = 0, size_t patch = 0);

}  // namespace fml

#endif  // CLAY_FML_PLATFORM_DARWIN_PLATFORM_VERSION_H_
