// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef TESTING_UTILS_MAKE_JS_RUNTIME_H_
#define TESTING_UTILS_MAKE_JS_RUNTIME_H_

#include <memory>

#include "core/runtime/js/jsi/jsi.h"
#include "core/runtime/js/jsi/quickjs/quickjs_api.h"

namespace testing {
namespace utils {

std::unique_ptr<lynx::runtime::js::Runtime> makeJSRuntime(
    std::shared_ptr<lynx::runtime::js::JSIExceptionHandler> handler = nullptr);

}
}  // namespace testing

#endif  // TESTING_UTILS_MAKE_JS_RUNTIME_H_
