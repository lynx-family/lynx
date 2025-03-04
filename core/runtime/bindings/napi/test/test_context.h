// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_BINDINGS_NAPI_TEST_TEST_CONTEXT_H_
#define CORE_RUNTIME_BINDINGS_NAPI_TEST_TEST_CONTEXT_H_

#include "third_party/binding/common/base.h"

namespace lynx {
namespace test {

class TestContext : public binding::ImplBase {
 public:
  TestContext() = default;
  uint32_t TestPlusOne(uint32_t num) { return num + 1; }
};

}  // namespace test
}  // namespace lynx
#endif  // CORE_RUNTIME_BINDINGS_NAPI_TEST_TEST_CONTEXT_H_
