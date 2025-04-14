// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_SIGNAL_COMPUTATION_UNITTEST_H_
#define CORE_RENDERER_SIGNAL_COMPUTATION_UNITTEST_H_

#include <tuple>

#include "core/renderer/signal/computation.h"
#include "core/renderer/signal/signal_context_unittest.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

class ComputationTest : public BaseSignalTest {
 protected:
  ComputationTest() : BaseSignalTest() {}
  ~ComputationTest() override {}
};

}  // namespace test
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_SIGNAL_COMPUTATION_UNITTEST_H_
