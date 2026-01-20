// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/resource/response_handler_in_js.h"

#include <memory>

#include "core/runtime/common/bindings/resource/response_promise.h"
#include "core/runtime/js/bindings/js_app.h"
#include "core/runtime/js/bindings/resource/response_handler_in_js.h"
#include "core/runtime/js/jsi/jsi.h"
#include "core/runtime/js/jsi/jsi_unittest.h"
#include "core/runtime/js/mock_template_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace runtime {
namespace js {
namespace test {

class ResponseHandlerInJSTest : public test::JSITestBase {
 protected:
  void SetUp() override {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    base::UIThread::Init();

    // Lazily create adapter to ensure fml is initialized
    adapter =
        std::make_shared<JsTaskAdapter>(runtime, "-1", tasm::PageOptions());
  }
  std::shared_ptr<JsTaskAdapter> adapter;
};

class TestDelegate : public runtime::test::MockTemplateDelegate {
 public:
  void InvokeResponsePromiseCallback(base::closure closure) override {
    called_ = true;
    if (closure) {
      closure();
    }
  }

  bool called() const { return called_; }

 private:
  bool called_ = false;
};

TEST_P(ResponseHandlerInJSTest, ThenCallbackFiresWithoutHoldingHandler) {
  TestDelegate delegate;
  auto promise =
      std::make_shared<runtime::ResponsePromise<tasm::BundleResourceInfo>>();

  Object nativeModuleProxy = Object::createFromHostObject(*runtime, nullptr);

  auto app =
      App::Create(0, runtime, &delegate, nullptr, std::move(nativeModuleProxy),
                  nullptr, "-1", tasm::PageOptions());
  bool resource_callback_called = false;
  {
    auto handler =
        std::make_shared<ResponseHandlerInJS>(delegate,
                                              /*url=*/"test-url", promise, app);

    handler->AddResourceListener(
        [&resource_callback_called](tasm::BundleResourceInfo info) {
          resource_callback_called = true;
        });
  }

  tasm::BundleResourceInfo info;
  info.url = "test-url";
  info.code = 200;
  promise->SetValue(info);

  EXPECT_TRUE(resource_callback_called);
}

INSTANTIATE_TEST_SUITE_P(
    Runtimes, ResponseHandlerInJSTest, ::testing::ValuesIn(runtimeGenerators()),
    [](const ::testing::TestParamInfo<ResponseHandlerInJSTest::ParamType>&
           info) {
      auto rt = info.param(nullptr);
      switch (rt->type()) {
        case JSRuntimeType::v8:
          return "v8";
        case JSRuntimeType::jsc:
          return "jsc";
        case JSRuntimeType::quickjs:
          return "quickjs";
        case JSRuntimeType::jsvm:
          return "jsvm";
      }
    });
}  // namespace test
}  // namespace js
}  // namespace runtime
}  // namespace lynx
