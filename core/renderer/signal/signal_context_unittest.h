// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_SIGNAL_SIGNAL_CONTEXT_UNITTEST_H_
#define CORE_RENDERER_SIGNAL_SIGNAL_CONTEXT_UNITTEST_H_

#include <tuple>

#include "core/renderer/signal/signal_context.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

class SignalContextTest : public ::testing::TestWithParam<std::tuple<bool>> {
  // TODO(songshourui.null): impl this later.
};

}  // namespace testing
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_SIGNAL_SIGNAL_CONTEXT_UNITTEST_H_
