// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_SIGNAL_LYNX_SIGNAL_UNITTEST_H_
#define CORE_RENDERER_SIGNAL_LYNX_SIGNAL_UNITTEST_H_

#include <tuple>

#include "core/renderer/signal/lynx_signal.h"
#include "core/renderer/signal/signal_context.h"
#include "core/renderer/signal/signal_context_unittest.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

class SignalTest : public BaseSignalTest {
 public:
  SignalTest() = default;
  ~SignalTest() = default;

 private:
  SignalContext signal_context_;
};

}  // namespace test
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_SIGNAL_LYNX_SIGNAL_UNITTEST_H_
