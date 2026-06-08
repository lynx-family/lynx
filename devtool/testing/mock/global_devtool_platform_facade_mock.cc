// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/testing/mock/global_devtool_platform_facade_mock.h"

#include "gtest/gtest.h"

namespace lynx {
namespace testing {}  // namespace testing

namespace devtool {
GlobalDevToolPlatformFacade& GlobalDevToolPlatformFacade::GetInstance() {
  // Keep the GetInstance definition in one .cc file. Defining this non-inline
  // function in the mock header would give every test translation unit its own
  // copy and can produce duplicate symbols once multiple DevTool tests include
  // the mock facade.
  static lynx::testing::GlobalDevToolPlatformFacadeMock instance;
  return instance;
}
}  // namespace devtool
}  // namespace lynx
