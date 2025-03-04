// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <fstream>
#include <string>

#include "core/runtime/bindings/napi/napi_environment.h"
#include "core/runtime/bindings/napi/napi_runtime_proxy.h"
#include "core/runtime/bindings/napi/napi_runtime_proxy_quickjs.h"
#include "core/runtime/bindings/napi/shim/shim_napi_env_quickjs.h"
#include "jsbridge/bindings/gen_test/napi_test_context.h"
#include "jsbridge/bindings/gen_test/napi_test_element.h"
#include "base/include/string/string_utils.h"
#include "third_party/binding/gen_test/test_async_object.h"
#include "third_party/binding/gen_test/test_context.h"
#include "third_party/binding/gen_test/test_module.h"
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

using binding::ArrayBufferView;

#define AssertExp(exp) EXPECT_EQ(env_.RunScript(exp).ToBoolean().Value(), true);

class BindingGeneratorTest : public ::testing::Test {
 public:
  BindingGeneratorTest()
      : rt_(LEPUS_NewRuntime()),
        ctx_(LEPUS_NewContext(rt_)),
        runtime_proxy_(static_cast<NapiRuntimeProxy*>(
            NapiRuntimeProxyQuickjs::Create(ctx_).release())),
        e(std::make_unique<TestDelegate>()),
        env_(runtime_proxy_->Env()) {
    e.SetRuntimeProxy(std::move(runtime_proxy_));
    e.Attach();
  }

  ~BindingGeneratorTest() override {
    e.Detach();
    LEPUS_FreeContext(ctx_);
    LEPUS_FreeRuntime(rt_);
  }

  void SetUp() override {
    // Expect the constructors injected.
    AssertExp("TestElement != undefined");
    AssertExp("TestContext != undefined");

    // Expect creating objects to work.
    env_.RunScript("let elem = new TestElement('test');");
    AssertExp("elem != undefined");

    // Context creation.
    env_.RunScript(
        "let ctx = elem.getContext('test');\n"
        "let ctx1 = elem.getContext('test');");
    AssertExp("ctx != undefined");
    AssertExp("ctx == ctx1");
    LOGI("Context created");

    std::ifstream ts(
        "third_party/binding/gen_test/jsbridge/bindings/gen_test/"
        "testContext.ts");
    if (ts.bad() || ts.fail()) {
      LOGE("Reading JS hook failed!");
    }
    std::stringstream buffer;
    buffer << ts.rdbuf();
    std::string js_code = buffer.str();

    // Remove the trailing 'export' as this environment does not support it.
    js_code.erase(js_code.find("export {"));

    env_.RunScript(js_code.c_str());
    env_.RunScript("hookTestContext(globalThis, ctx);");
    EXPECT_FALSE(env_.GetAndClearPendingException().IsObject());
    LOGI("Setup complete");
  }

 private:
  class TestDelegate : public NapiEnvironment::Delegate {
    void OnAttach(Napi::Env env) override {
      gen_test::TestModule m;
      Napi::Object global = env.Global();
      m.OnLoad(global);
    }
  };

  LEPUSRuntime* rt_;
  LEPUSContext* ctx_;
  std::unique_ptr<NapiRuntimeProxy> runtime_proxy_;
  NapiEnvironment e;

 protected:
  Napi::Env env_;
};

TEST_F(BindingGeneratorTest, CommandBufferBasicTest) {
  Napi::HandleScope hscope(env_);

  // Buffered call should be asynchronous.
  env_.RunScript("ctx.voidFromVoid();");
  auto log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 0u);

  // But should get flushed by finish().
  env_.RunScript("ctx.finish();");
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 2u);
  EXPECT_EQ(log[0], "VoidFromVoid()");
  EXPECT_EQ(log[1], "Finish()");

  // Redundant arguments should be ok.
  env_.RunScript("ctx.voidFromVoid(123, ctx);");
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 0u);

  // Sync call should also flush.
  env_.RunScript("ctx.stringFromVoid();");
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 2u);
  EXPECT_EQ(log[0], "VoidFromVoid()");
  EXPECT_EQ(log[1], "StringFromVoid()");
  EXPECT_FALSE(env_.GetAndClearPendingException().IsObject());
}

// clang-format off
static std::vector<std::string> string_cases = {
    "Lynx is a high-performance cross-platform rendering framework.",  // English
    "Lynx 是高性能的跨平台渲染框架。",  // Chinese
    "Lynxは高性能なクロスプラットフォームのレンダリングフレームワークです。",  // Japanese
    "Lynx — это высокопроизводительный кроссплатформенный фреймворк для рендеринга.",  // Russian
    "Lynx هو إطار عمل عالي الأداء متعدد المنصات لتقديم الرسومات.",  // Arabic
    "Lynx 湜滈悻能哋跨岼珆渲媣框泇。",  // Martian
    "🦊👉🏻🚀📈🌐⌨️🎨🖼️📐🎊",  // Emoji
    "𐤀𐤁𐤂",  // UTF-16 multiple charcode
};
// clang-format on

TEST_F(BindingGeneratorTest, BindingCallStringTest) {
  Napi::HandleScope hscope(env_);

  // Binding call should be synchronous.
  env_.RunScript("ctx.voidFromString_('');");
  auto log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 1u);
  EXPECT_EQ(log[0], "VoidFromString('')");

  EXPECT_FALSE(env_.GetAndClearPendingException().IsObject());

  // Arguments should be passed correctly.
  for (const auto& s : string_cases) {
    env_.RunScript(("ctx.voidFromString_('" + s + "');").c_str());
    log = gen_test::TestContext::RetrieveLog();
    EXPECT_EQ(log.size(), 1u);
    EXPECT_EQ(log[0], "VoidFromString('" + s + "')");
  }

  EXPECT_FALSE(env_.GetAndClearPendingException().IsObject());

  // Insufficient argument should throw.
  env_.RunScript("ctx.voidFromString_();");
  EXPECT_TRUE(env_.GetAndClearPendingException().IsObject());

  // Returning string values should work.
  env_.RunScript("let str;");
  for (const auto& s : string_cases) {
    gen_test::TestContext::SetReturnString(s);
    env_.RunScript("str = ctx.stringFromVoid();");
    log = gen_test::TestContext::RetrieveLog();
    EXPECT_EQ(log.size(), 1u);
    EXPECT_EQ(log[0], "StringFromVoid()");
    AssertExp(("'" + s + "' == str").c_str());
  }

  EXPECT_FALSE(env_.GetAndClearPendingException().IsObject());
}

TEST_F(BindingGeneratorTest, CommandBufferStringTest) {
  Napi::HandleScope hscope(env_);

  // Buffered call should be asynchronous.
  env_.RunScript("ctx.voidFromString$('');");
  auto log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 0u);

  EXPECT_FALSE(env_.GetAndClearPendingException().IsObject());

  // But should get flushed by finish().
  env_.RunScript("ctx.finish();");
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 2u);
  EXPECT_EQ(log[0], "VoidFromString('')");
  EXPECT_EQ(log[1], "Finish()");

  EXPECT_FALSE(env_.GetAndClearPendingException().IsObject());

  // Arguments should be passed correctly.
  for (const auto& s : string_cases) {
    env_.RunScript(("ctx.voidFromString$('" + s + "');").c_str());
  }
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 0u);
  env_.RunScript("ctx.finish();");
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), string_cases.size() + 1u);
  for (size_t i = 0; i < string_cases.size(); ++i) {
    EXPECT_EQ(log[i], "VoidFromString('" + string_cases[i] + "')");
  }

  EXPECT_FALSE(env_.GetAndClearPendingException().IsObject());

  // Insufficient argument should throw.
  env_.RunScript("ctx.voidFromString$();");
  EXPECT_TRUE(env_.GetAndClearPendingException().IsObject());
}

TEST_F(BindingGeneratorTest, CommandBufferStringArrayTest) {
  Napi::HandleScope hscope(env_);

  // Simple arrays.
  env_.RunScript("ctx.voidFromStringArray$([]);");
  env_.RunScript("ctx.voidFromStringArray$(['42']);");
  env_.RunScript("ctx.voidFromStringArray$(['42', 'abc']);");
  auto log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 0u);
  env_.RunScript("ctx.finish();");
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 4u);
  EXPECT_EQ(log[0], "VoidFromStringArray([])");
  EXPECT_EQ(log[1], "VoidFromStringArray(['42'])");
  EXPECT_EQ(log[2], "VoidFromStringArray(['42', 'abc'])");

  EXPECT_FALSE(env_.GetAndClearPendingException().IsObject());

  // Complex one.
  std::string joined_array = "['" + base::Join(string_cases, "', '") + "']";
  env_.RunScript(("ctx.voidFromStringArray$(" + joined_array + ");").c_str());
  env_.RunScript("ctx.finish();");
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 2u);
  EXPECT_EQ(log[0], "VoidFromStringArray(" + joined_array + ")");

  EXPECT_FALSE(env_.GetAndClearPendingException().IsObject());

  // Insufficient argument should throw.
  env_.RunScript("ctx.voidFromStringArray$();");
  EXPECT_TRUE(env_.GetAndClearPendingException().IsObject());

  // Wrong argument should throw.
  env_.RunScript("ctx.voidFromStringArray$(42);");
  EXPECT_TRUE(env_.GetAndClearPendingException().IsObject());
}

TEST_F(BindingGeneratorTest, TypedArrayTest) {
  Napi::HandleScope hscope(env_);

  // Short arrays.
  env_.RunScript("const float32array = new Float32Array(2);");
  env_.RunScript("float32array[0] = -1.2");
  env_.RunScript("float32array[1] = 3.14");
  env_.RunScript("ctx.voidFromTypedArray(float32array);");
  auto log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 0u);
  env_.RunScript("ctx.finish();");
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 2u);
  EXPECT_EQ(log[0], "VoidFromTypedArray_CommandBuffer([-1.2, 3.14])");
  env_.RunScript("ctx.voidFromTypedArray_(float32array);");
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 1u);
  EXPECT_EQ(log[0], "VoidFromTypedArray_Binding([-1.2, 3.14])");

  EXPECT_FALSE(env_.GetAndClearPendingException().IsObject());

  // Long arrays should call binding directly. Also test the 32KB threshold.
  std::string long_array;
  long_array.reserve(50 * 1024);
  for (int i = 0; i < 4 * 1024; ++i) {
    if (i > 0) {
      long_array += ", ";
    }
    long_array += "-1.2, 3.14";
  }
  env_.RunScript(("ctx.voidFromTypedArray(new Float32Array([" + long_array + "]));").c_str());
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 0u);
  env_.RunScript(("ctx.voidFromTypedArray(new Float32Array([" + long_array + ", 1.1]));").c_str());
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 2u);
  EXPECT_EQ(log[0], "VoidFromTypedArray_CommandBuffer([" + long_array + "])");
  EXPECT_EQ(log[1], "VoidFromTypedArray_Binding([" + long_array + ", 1.1])");

  EXPECT_FALSE(env_.GetAndClearPendingException().IsObject());

  // Arrays can work implicitly.
  env_.RunScript("ctx.voidFromTypedArray([5.7, 6.99, 10]);");
  env_.RunScript("ctx.finish();");
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 2u);
  EXPECT_EQ(log[0], "VoidFromTypedArray_CommandBuffer([5.7, 6.99, 10])");

  EXPECT_FALSE(env_.GetAndClearPendingException().IsObject());

  // Insufficient argument should throw.
  env_.RunScript("ctx.voidFromTypedArray();");
  EXPECT_TRUE(env_.GetAndClearPendingException().IsObject());

  // Other types are converted to typed arrays.
  env_.RunScript("ctx.voidFromTypedArray(12);");
  env_.RunScript("ctx.voidFromTypedArray('12');");
  env_.RunScript("ctx.voidFromTypedArray(ctx);");
  env_.RunScript("ctx.voidFromTypedArray(new Int8Array([5, 6, 10]));");
  env_.RunScript("ctx.voidFromTypedArray(['aaa', '42', 3.14]);");
  env_.RunScript("ctx.finish();");
  EXPECT_FALSE(env_.GetAndClearPendingException().IsObject());
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 6u);
  EXPECT_EQ(log[0], "VoidFromTypedArray_CommandBuffer([0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0])");
  EXPECT_EQ(log[1], "VoidFromTypedArray_CommandBuffer([0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0])");
  EXPECT_EQ(log[2], "VoidFromTypedArray_CommandBuffer([])");
  EXPECT_EQ(log[3], "VoidFromTypedArray_CommandBuffer([5, 6, 10])");
  EXPECT_EQ(log[4], "VoidFromTypedArray_CommandBuffer([nan, 42, 3.14])");
}

TEST_F(BindingGeneratorTest, ArrayBufferTest) {
  Napi::HandleScope hscope(env_);

  // Short buffers.
  env_.RunScript("let buffer = new ArrayBuffer(8);");
  env_.RunScript("let int8array = new Int8Array(buffer);");
  env_.RunScript("int8array[0] = 7;");
  env_.RunScript("int8array[1] = 6;");
  env_.RunScript("int8array[7] = 1;");
  env_.RunScript("ctx.voidFromArrayBuffer(buffer);");
  auto log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 0u);
  env_.RunScript("ctx.finish();");
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 2u);
  EXPECT_EQ(log[0], "VoidFromArrayBuffer_CommandBuffer(8, [7, 6, 0, 0, 0, 0, 0, 1])");
  env_.RunScript("ctx.voidFromArrayBuffer_(buffer);");
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 1u);
  EXPECT_EQ(log[0], "VoidFromArrayBuffer_Binding(8, [7, 6, 0, 0, 0, 0, 0, 1])");

  EXPECT_FALSE(env_.GetAndClearPendingException().IsObject());

  // Long buffers should call binding directly. Also test the 32KB threshold.
  std::string long_array;
  long_array.reserve(50 * 1024);
  constexpr uint32_t long_array_threshold = 32 * 1024;
  for (int i = 0; i < long_array_threshold / 2; ++i) {
    if (i > 0) {
      long_array += ", ";
    }
    long_array += "127, 255";
  }
  env_.RunScript(("buffer = new ArrayBuffer(" + std::to_string(long_array_threshold) + ");").c_str());
  env_.RunScript("int8array = new Int8Array(buffer);");
  env_.RunScript(("int8array.set([" + long_array + "]);").c_str());
  env_.RunScript("ctx.voidFromArrayBuffer(buffer);");
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 0u);
  env_.RunScript(("buffer = new ArrayBuffer(" + std::to_string(long_array_threshold + 1) + ");").c_str());
  env_.RunScript("int8array = new Int8Array(buffer);");
  env_.RunScript(("int8array.set([" + long_array + ", 1]);").c_str());
  env_.RunScript("ctx.voidFromArrayBuffer(buffer);");
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 2u);
  EXPECT_EQ(log[0], "VoidFromArrayBuffer_CommandBuffer(" + std::to_string(long_array_threshold) + ", [" + long_array + "])");
  EXPECT_EQ(log[1], "VoidFromArrayBuffer_Binding(" + std::to_string(long_array_threshold + 1) + ", [" + long_array + ", 1])");

  EXPECT_FALSE(env_.GetAndClearPendingException().IsObject());

  // Insufficient argument should throw.
  env_.RunScript("ctx.voidFromArrayBuffer();");
  EXPECT_TRUE(env_.GetAndClearPendingException().IsObject());

  // Other types are converted to ArrayBuffer.
  env_.RunScript("ctx.voidFromArrayBuffer(12);");
  env_.RunScript("ctx.voidFromArrayBuffer('12');");
  env_.RunScript("ctx.voidFromArrayBuffer(ctx);");
  env_.RunScript("ctx.voidFromArrayBuffer(new Int8Array([5, 6, 10]));");
  env_.RunScript("ctx.voidFromArrayBuffer(['aaa', '42', 3.14]);");
  env_.RunScript("ctx.finish();");
  EXPECT_FALSE(env_.GetAndClearPendingException().IsObject());
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 6u);
  EXPECT_EQ(log[0], "VoidFromArrayBuffer_CommandBuffer(12, [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0])");
  EXPECT_EQ(log[1], "VoidFromArrayBuffer_CommandBuffer(12, [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0])");
  EXPECT_EQ(log[2], "VoidFromArrayBuffer_CommandBuffer(0, [])");
  EXPECT_EQ(log[3], "VoidFromArrayBuffer_CommandBuffer(3, [5, 6, 10])");
  EXPECT_EQ(log[4], "VoidFromArrayBuffer_CommandBuffer(3, [0, 42, 3])");
}

TEST_F(BindingGeneratorTest, ArrayBufferViewTest) {
  Napi::HandleScope hscope(env_);

  // Short arrays.
  env_.RunScript("const float32array = new Float32Array(2);");
  env_.RunScript("float32array[0] = -1.2");
  env_.RunScript("float32array[1] = 3.14");
  env_.RunScript("ctx.voidFromArrayBufferView(float32array);");
  auto log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 0u);
  env_.RunScript("ctx.finish();");
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 2u);
  EXPECT_EQ(log[0], "VoidFromArrayBufferView(" + std::to_string(ArrayBufferView::kTypeFloat32) + "|2, [-1.2, 3.14])");
  env_.RunScript("ctx.voidFromArrayBufferView_(float32array);");
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 1u);
  EXPECT_EQ(log[0], "VoidFromArrayBufferView(" + std::to_string(ArrayBufferView::kTypeFloat32) + "|2, [-1.2, 3.14])");

  EXPECT_FALSE(env_.GetAndClearPendingException().IsObject());

  // Long arrays should call binding directly. Also test the 32KB threshold.
  std::string long_array;
  long_array.reserve(50 * 1024);
  constexpr uint32_t long_array_threshold = 8 * 1024;
  for (int i = 0; i < long_array_threshold / 2; ++i) {
    if (i > 0) {
      long_array += ", ";
    }
    long_array += "-1.2, 3.14";
  }
  env_.RunScript(("ctx.voidFromArrayBufferView(new Float32Array([" + long_array + "]));").c_str());
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 0u);
  env_.RunScript(("ctx.voidFromArrayBufferView(new Float32Array([" + long_array + ", 1.1]));").c_str());
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 2u);
  EXPECT_EQ(log[0], "VoidFromArrayBufferView(" + std::to_string(ArrayBufferView::kTypeFloat32) + "|" + std::to_string(long_array_threshold) + ", [" + long_array + "])");
  EXPECT_EQ(log[1], "VoidFromArrayBufferView(" + std::to_string(ArrayBufferView::kTypeFloat32) + "|" + std::to_string(long_array_threshold + 1) + ", [" + long_array + ", 1.1])");

  EXPECT_FALSE(env_.GetAndClearPendingException().IsObject());

  // Insufficient argument should throw.
  env_.RunScript("ctx.voidFromArrayBufferView();");
  EXPECT_TRUE(env_.GetAndClearPendingException().IsObject());

  // Other types are passed as empty.
  env_.RunScript("ctx.voidFromArrayBufferView(12);");
  env_.RunScript("ctx.voidFromArrayBufferView('12');");
  env_.RunScript("ctx.voidFromArrayBufferView(ctx);");
  env_.RunScript("ctx.voidFromArrayBufferView(['aaa', '42', 3.14]);");
  env_.RunScript("ctx.finish();");
  EXPECT_FALSE(env_.GetAndClearPendingException().IsObject());
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 5u);
  EXPECT_EQ(log[0], "VoidFromArrayBufferView(" + std::to_string(ArrayBufferView::kTypeEmpty) + "|0, [])");
  EXPECT_EQ(log[1], "VoidFromArrayBufferView(" + std::to_string(ArrayBufferView::kTypeEmpty) + "|0, [])");
  EXPECT_EQ(log[2], "VoidFromArrayBufferView(" + std::to_string(ArrayBufferView::kTypeEmpty) + "|0, [])");
  EXPECT_EQ(log[3], "VoidFromArrayBufferView(" + std::to_string(ArrayBufferView::kTypeEmpty) + "|0, [])");

  EXPECT_FALSE(env_.GetAndClearPendingException().IsObject());

  // View types should all work.
  env_.RunScript("ctx.voidFromArrayBufferView(new Int8Array([-5, 6, -10]));");
  env_.RunScript("ctx.voidFromArrayBufferView(new Uint8Array([5, 6, 10]));");
  env_.RunScript("ctx.voidFromArrayBufferView(new Uint8ClampedArray([256, 257, 25]));");
  env_.RunScript("ctx.voidFromArrayBufferView(new Int16Array([-5, 6, -32768]));");
  env_.RunScript("ctx.voidFromArrayBufferView(new Uint16Array([5, 6, 65535]));");
  env_.RunScript("ctx.voidFromArrayBufferView(new Int32Array([-5, 6, -65536]));");
  env_.RunScript("ctx.voidFromArrayBufferView(new Uint32Array([5, 6, 65536]));");
  env_.RunScript("ctx.voidFromArrayBufferView(new Float32Array([-1.2, 3.14, 65536]));");
  env_.RunScript("ctx.voidFromArrayBufferView(new Float64Array([-1.2, 3.14, 65536]));");
  env_.RunScript("let uint8array = new Uint8Array([5, 6, 10]);");
  env_.RunScript("ctx.voidFromArrayBufferView(new DataView(uint8array.buffer));");
  env_.RunScript("ctx.finish();");
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log[0], "VoidFromArrayBufferView(" + std::to_string(ArrayBufferView::kTypeInt8) + "|3, [-5, 6, -10])");
  EXPECT_EQ(log[1], "VoidFromArrayBufferView(" + std::to_string(ArrayBufferView::kTypeUint8) + "|3, [5, 6, 10])");
  EXPECT_EQ(log[2], "VoidFromArrayBufferView(" + std::to_string(ArrayBufferView::kTypeUint8Clamped) + "|3, [255, 255, 25])");
  EXPECT_EQ(log[3], "VoidFromArrayBufferView(" + std::to_string(ArrayBufferView::kTypeInt16) + "|3, [-5, 6, -32768])");
  EXPECT_EQ(log[4], "VoidFromArrayBufferView(" + std::to_string(ArrayBufferView::kTypeUint16) + "|3, [5, 6, 65535])");
  EXPECT_EQ(log[5], "VoidFromArrayBufferView(" + std::to_string(ArrayBufferView::kTypeInt32) + "|3, [-5, 6, -65536])");
  EXPECT_EQ(log[6], "VoidFromArrayBufferView(" + std::to_string(ArrayBufferView::kTypeUint32) + "|3, [5, 6, 65536])");
  EXPECT_EQ(log[7], "VoidFromArrayBufferView(" + std::to_string(ArrayBufferView::kTypeFloat32) + "|3, [-1.2, 3.14, 65536])");
  EXPECT_EQ(log[8], "VoidFromArrayBufferView(" + std::to_string(ArrayBufferView::kTypeFloat64) + "|3, [-1.2, 3.14, 65536])");
  EXPECT_EQ(log[9], "VoidFromArrayBufferView(" + std::to_string(ArrayBufferView::kTypeDataView) + "|3, [5, 6, 10])");

  EXPECT_FALSE(env_.GetAndClearPendingException().IsObject());

  // Nullable should accept null.
  env_.RunScript("ctx.voidFromArrayBufferView(null);");
  EXPECT_TRUE(env_.GetAndClearPendingException().IsObject());
  env_.RunScript("ctx.voidFromNullableArrayBufferView(null);");
  EXPECT_FALSE(env_.GetAndClearPendingException().IsObject());
  env_.RunScript("ctx.finish();");
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log[0], "VoidFromNullableArrayBufferView(" + std::to_string(ArrayBufferView::kTypeEmpty) + "|0, [])");
}

TEST_F(BindingGeneratorTest, AsyncObjectTest) {
  Napi::HandleScope hscope(env_);

  // Creating an async object should be async.
  EXPECT_EQ(gen_test::TestContext::async_object, nullptr);
  env_.RunScript("const async = ctx.createAsyncObject();");
  auto log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 0u);
  env_.RunScript("ctx.finish();");
  gen_test::TestAsyncObject* obj = gen_test::TestContext::async_object;
  std::stringstream ss;
  ss << (uintptr_t)obj;
  EXPECT_NE(obj, nullptr);
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 2u);
  EXPECT_EQ(log[0], "CreateAsyncObject(" + ss.str() + ")");

  EXPECT_FALSE(env_.GetAndClearPendingException().IsObject());

  // Following calls should be directed to the same object.
  env_.RunScript("ctx.asyncForAsyncObject(async);");
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 0u);
  env_.RunScript("ctx.syncForAsyncObject(async);");
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log[0], "AsyncForAsyncObject(" + ss.str() + ")");
  EXPECT_EQ(log[1], "SyncForAsyncObject(" + ss.str() + ")");

  EXPECT_FALSE(env_.GetAndClearPendingException().IsObject());

  // Returning value should work.
  std::string s = "Hello, world!";
  gen_test::TestContext::SetReturnString(s);
  env_.RunScript(("str = ctx.syncForAsyncObject(async);"));
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 1u);
  AssertExp(("'" + s + "' == str").c_str());

  // Passing null to sync call should throw.
  env_.RunScript("ctx.syncForAsyncObject(null);");
  EXPECT_TRUE(env_.GetAndClearPendingException().IsObject());
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 0u);

  // Passing null to async call should throw.
  env_.RunScript("ctx.asyncForAsyncObject(null);");
  EXPECT_TRUE(env_.GetAndClearPendingException().IsObject());
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 0u);

  // Passing null to nullable parameter should be ok.
  env_.RunScript("ctx.asyncForNullableAsyncObject(null);");
  EXPECT_FALSE(env_.GetAndClearPendingException().IsObject());
  env_.RunScript("ctx.finish();");
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 2u);
  EXPECT_EQ(log[0], "AsyncForNullableAsyncObject(0)");

  // Passing wrong type should throw.
  env_.RunScript("ctx.asyncForAsyncObject(ctx);");
  EXPECT_TRUE(env_.GetAndClearPendingException().IsObject());
  log = gen_test::TestContext::RetrieveLog();
  EXPECT_EQ(log.size(), 0u);
}

}  // namespace piper
}  // namespace lynx
