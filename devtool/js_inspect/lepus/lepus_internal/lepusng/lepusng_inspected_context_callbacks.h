// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INTERNAL_LEPUSNG_LEPUSNG_INSPECTED_CONTEXT_CALLBACKS_H_
#define DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INTERNAL_LEPUSNG_LEPUSNG_INSPECTED_CONTEXT_CALLBACKS_H_

#include <vector>

namespace lepus_inspector {

std::vector<void *> &GetDebuggerCallbackFuncs();

}  // namespace lepus_inspector

#endif  // DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INTERNAL_LEPUSNG_LEPUSNG_INSPECTED_CONTEXT_CALLBACKS_H_
