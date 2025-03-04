// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef THIRD_PARTY_BINDINGS_GEN_TEST_TEST_ELEMENT_H_
#define THIRD_PARTY_BINDINGS_GEN_TEST_TEST_ELEMENT_H_

#include <memory>
#include <string>

#include "jsbridge/bindings/gen_test/napi_test_context.h"
#include "test_context.h"
#include "third_party/binding/napi/napi_bridge.h"

namespace lynx {
namespace gen_test {

class TestElement : public binding::ImplBase {
 public:
  static std::unique_ptr<TestElement> Create() {
    return std::unique_ptr<TestElement>(new TestElement());
  }
  TestElement() = default;
  TestContext* GetContext(const std::string& id) {
    if (context_) {
      return context_;
    }
    context_ = new TestContext();
    return context_;
  }

 private:
  TestContext* context_ = nullptr;
};

}  // namespace gen_test
}  // namespace lynx

#endif  // THIRD_PARTY_BINDINGS_GEN_TEST_TEST_ELEMENT_H_
