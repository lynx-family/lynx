// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef TESTING_DEBUGGER_DETECTION_H_
#define TESTING_DEBUGGER_DETECTION_H_

#include "base/include/fml/macros.h"

namespace clay {
namespace testing {

enum class DebuggerStatus {
  kDontKnow,
  kAttached,
};

DebuggerStatus GetDebuggerStatus();

}  // namespace testing
}  // namespace clay

#endif  // TESTING_DEBUGGER_DETECTION_H_
