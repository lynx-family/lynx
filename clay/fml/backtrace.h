// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FML_BACKTRACE_H_
#define CLAY_FML_BACKTRACE_H_

#include <string>

#include "base/include/fml/macros.h"

namespace fml {

// Retrieve the backtrace, for debugging.
//
// If the |offset| is 0, the backtrace is included caller function.
std::string BacktraceHere(size_t offset = 0);

void InstallCrashHandler();

bool IsCrashHandlingSupported();

}  // namespace fml

#endif  // CLAY_FML_BACKTRACE_H_
