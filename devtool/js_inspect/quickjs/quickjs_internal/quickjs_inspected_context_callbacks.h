// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_JS_INSPECT_QUICKJS_QUICKJS_INTERNAL_QUICKJS_INSPECTED_CONTEXT_CALLBACKS_H_
#define DEVTOOL_JS_INSPECT_QUICKJS_QUICKJS_INTERNAL_QUICKJS_INSPECTED_CONTEXT_CALLBACKS_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "quickjs/include/quickjs.h"
#ifdef __cplusplus
}
#endif
#include <vector>

namespace quickjs_inspector {

// get all the qjs debugger callbacks
std::vector<void *> &GetQJSCallbackFuncs();

}  // namespace quickjs_inspector

#endif  // DEVTOOL_JS_INSPECT_QUICKJS_QUICKJS_INTERNAL_QUICKJS_INSPECTED_CONTEXT_CALLBACKS_H_
