// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef TESTING_UTILS_MAKE_JS_RUNTIME_H_
#define TESTING_UTILS_MAKE_JS_RUNTIME_H_

#include <memory>

#include "core/runtime/jsi/jsi.h"
#include "core/runtime/jsi/quickjs/quickjs_api.h"

namespace testing {
namespace utils {

std::unique_ptr<lynx::piper::Runtime> makeJSRuntime(
    std::shared_ptr<lynx::piper::JSIExceptionHandler> handler = nullptr);

}
}  // namespace testing

#endif  // TESTING_UTILS_MAKE_JS_RUNTIME_H_
