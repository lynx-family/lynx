/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the current directory.
 */
// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSI_JSI_UNITTEST_H_
#define CORE_RUNTIME_JSI_JSI_UNITTEST_H_

#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "core/runtime/jsi/jsi.h"
#include "core/runtime/jsi/quickjs/quickjs_runtime.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

#ifdef OS_OSX
#include "core/runtime/jsi/jsc/jsc_runtime.h"
#endif

namespace lynx::piper {
class Runtime;
namespace test {

using RuntimeFactory = std::function<std::unique_ptr<Runtime>(
    std::shared_ptr<JSIExceptionHandler>)>;

template <typename T,
          typename = std::enable_if_t<std::is_base_of_v<Runtime, T>>>
std::unique_ptr<Runtime> MakeRuntimeFactory(
    std::shared_ptr<JSIExceptionHandler> exception_handler) {
  auto rt = std::make_unique<T>();

  auto vm = rt->createVM(nullptr);
  auto context = rt->createContext(vm);

  rt->InitRuntime(context, std::move(exception_handler));
  return rt;
}

inline std::vector<RuntimeFactory> runtimeGenerators() {
  std::vector<RuntimeFactory> runtime_factories{};

  runtime_factories.emplace_back(MakeRuntimeFactory<QuickjsRuntime>);
#ifdef OS_OSX
  runtime_factories.emplace_back(MakeRuntimeFactory<JSCRuntime>);
#endif
  // TODO(wangqingyu): add V8 runtime

  return runtime_factories;
}

// A gmock matcher that match the message of an error
MATCHER_P(HasMessage, message, "") {
  // Should only match JSIException
  static_assert(std::is_same_v<std::decay_t<arg_type>, JSIException>);
  return message == arg.message();
}

// A mocked JSIExceptionHandler that using gmock
// An example usage:
//   EXPECT_CALL(*exception_handler_, onJSIException).Times(1)
// An example usage with HasMessage matcher:
//   EXPECT_CALL(*exception_handler_,
//     onJSIException(HasMessage("foo"))).Times(1);
class MockExceptionHandler : public piper::JSIExceptionHandler {
 public:
  MOCK_METHOD(void, onJSIException, (const JSIException&), (override));
};

class JSITestBase : public ::testing::TestWithParam<RuntimeFactory> {
 public:
  JSITestBase()
      : exception_handler_(std::make_shared<MockExceptionHandler>()),
        factory(GetParam()),
        runtime(factory(exception_handler_)),
        rt(*runtime) {}

  std::optional<Value> eval(const char* code) {
    return rt.global().getPropertyAsFunction(rt, "eval")->call(rt, code);
  }

  Function function(const std::string& code) {
    return eval(("(" + code + ")").c_str())->getObject(rt).getFunction(rt);
  }

  bool checkValue(const Value& value, const std::string& jsValue) {
    return function("function(value) { return value == " + jsValue + "; }")
        .call(rt, std::move(value))
        ->getBool();
  }

  std::shared_ptr<MockExceptionHandler> exception_handler_;
  RuntimeFactory factory;
  std::shared_ptr<Runtime> runtime;
  Runtime& rt;
};

}  // namespace test
}  // namespace lynx::piper

#endif  // CORE_RUNTIME_JSI_JSI_UNITTEST_H_
