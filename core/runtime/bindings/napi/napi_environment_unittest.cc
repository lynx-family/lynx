// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/napi/napi_environment.h"

#include "core/runtime/bindings/napi/napi_runtime_proxy.h"
#include "core/runtime/bindings/napi/napi_runtime_proxy_quickjs.h"
#include "core/runtime/bindings/napi/shim/shim_napi_env_quickjs.h"
#include "core/runtime/bindings/napi/test/napi_test_context.h"
#include "core/runtime/bindings/napi/test/napi_test_element.h"
#include "core/runtime/bindings/napi/test/test_module.h"
#include "third_party/binding/napi/shim/shim_napi.h"
#include "third_party/binding/napi/shim/shim_napi_env.h"
#include "third_party/binding/napi/shim/shim_napi_runtime.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus
#include "quickjs/include/quickjs.h"
#ifdef __cplusplus
}
#endif  // __cplusplus

namespace lynx {
namespace piper {

class NapiEnvironmentTest : public ::testing::Test {
 public:
  NapiEnvironmentTest()
      : rt_(LEPUS_NewRuntime()),
        ctx_(LEPUS_NewContext(rt_)),
        runtime_proxy_(static_cast<NapiRuntimeProxy*>(
            NapiRuntimeProxyQuickjs::Create(ctx_).release())),
        env_(runtime_proxy_->Env()) {}
  ~NapiEnvironmentTest() override {
    LEPUS_FreeContext(ctx_);
    LEPUS_FreeRuntime(rt_);
  }

 private:
  LEPUSRuntime* rt_;
  LEPUSContext* ctx_;

 public:
  std::unique_ptr<NapiRuntimeProxy> runtime_proxy_;
  Napi::Env env_;
};

#define AssertExp(exp) EXPECT_EQ(env_.RunScript(exp).ToBoolean().Value(), true);

TEST_F(NapiEnvironmentTest, BasicScriptingTest) {
  NapiEnvironment e(std::make_unique<NapiEnvironment::Delegate>());
  e.SetRuntimeProxy(std::move(runtime_proxy_));
  e.Attach();

  Napi::HandleScope hscope(env_);

  auto v = env_.RunScript("321");
  EXPECT_EQ(v.ToNumber().Int32Value(), 321);

  env_.RunScript("x = 42");
  EXPECT_EQ(env_.Global().Get("x").ToNumber().Int32Value(), 42);

  v = env_.RunScript(
      "msg = 'hello world!';\n"
      "function check(value) { return value == 'hello world!'; }\n"
      "check(msg)");
  EXPECT_EQ(v.ToBoolean().Value(), true);
}

TEST_F(NapiEnvironmentTest, LoadModuleTest) {
  class TestDelegate : public NapiEnvironment::Delegate {
    void OnAttach(Napi::Env env) override {
      test::TestModule m;
      Napi::Object global = env.Global();
      m.OnLoad(global);
    }
  };
  NapiEnvironment e(std::make_unique<TestDelegate>());
  e.SetRuntimeProxy(std::move(runtime_proxy_));
  e.Attach();

  Napi::HandleScope hscope(env_);

  // Expect the constructors injected.
  AssertExp("TestElement != undefined");
  AssertExp("TestContext != undefined");

  // Expect creating objects to work.
  env_.RunScript("let elem = new TestElement('test');");
  AssertExp("elem != undefined");

  // Test module logic.
  env_.RunScript(
      "let ctx = elem.getContext('test');\n"
      "let ctx1 = elem.getContext('test');");
  AssertExp("ctx != undefined");
  AssertExp("ctx == ctx1");
  env_.RunScript("let foo = ctx.testPlusOne(41);");
  AssertExp("foo == 42");
}

}  // namespace piper
}  // namespace lynx
