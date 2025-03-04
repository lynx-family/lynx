// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_BINDINGS_NAPI_TEST_TEST_ELEMENT_H_
#define CORE_RUNTIME_BINDINGS_NAPI_TEST_TEST_ELEMENT_H_

#include <memory>
#include <string>

#include "core/runtime/bindings/napi/test/napi_test_context.h"
#include "core/runtime/bindings/napi/test/test_context.h"
#include "third_party/binding/common/base.h"

namespace lynx {
namespace test {

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
    js_context_ = Napi::Persistent(NapiTestContext::Wrap(
        std::unique_ptr<TestContext>(context_), NapiEnv()));
    return context_;
  }

 private:
  Napi::ObjectReference js_context_;
  TestContext* context_ = nullptr;
};

}  // namespace test
}  // namespace lynx
#endif  // CORE_RUNTIME_BINDINGS_NAPI_TEST_TEST_ELEMENT_H_
