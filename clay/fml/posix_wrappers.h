// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FML_POSIX_WRAPPERS_H_
#define CLAY_FML_POSIX_WRAPPERS_H_

#include "build/build_config.h"

// Provides wrappers for POSIX functions that have been renamed on Windows.
// See
// https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-3-c4996?view=vs-2019#posix-function-names
// for context.
namespace fml {

char* strdup(const char* str1);

}  // namespace fml

#endif  // CLAY_FML_POSIX_WRAPPERS_H_
