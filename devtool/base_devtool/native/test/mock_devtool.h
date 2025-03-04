// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_TEST_MOCK_DEVTOOL_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_TEST_MOCK_DEVTOOL_H_

#include "devtool/base_devtool/native/public/abstract_devtool.h"

namespace lynx {
namespace devtool {

class MockDevTool : public lynx::devtool::AbstractDevTool {
 public:
  MockDevTool() = default;
  ~MockDevTool() override = default;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_TEST_MOCK_DEVTOOL_H_
