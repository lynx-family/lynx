// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "testing/utils/make_js_runtime.h"

namespace testing {
namespace utils {

std::unique_ptr<lynx::piper::Runtime> makeJSRuntime(
    std::shared_ptr<lynx::piper::JSIExceptionHandler> handler) {
  std::unique_ptr<lynx::piper::Runtime> rt = lynx::piper::makeQuickJsRuntime();
  lynx::piper::StartupData data{};
  auto vm = rt->createVM(&data);
  auto ctx = rt->createContext(vm);
  rt->InitRuntime(ctx, handler);
  return rt;
}

}  // namespace utils
}  // namespace testing
