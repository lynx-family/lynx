// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bytecode/js_cache_tracker.h"
#include "core/runtime/js/bytecode/js_cache_tracker_unittest.h"
#include "core/runtime/js/bytecode/quickjs/quickjs_cache_generator.h"
#include "core/runtime/js/js_bundle.h"
#include "core/runtime/js/jsi/jsi_unittest.h"
#include "core/runtime/js/jsi/jslib.h"
#include "third_party/modp_b64/modp_b64.h"

namespace lynx {
namespace runtime {
namespace js {
namespace test {
using namespace lynx::tasm::report::test;
using namespace cache::testing;
class QuickjsRuntimeTest : public JSITestBase {};

TEST_P(QuickjsRuntimeTest, WrappedSourceStackPositions) {
  const char* source_url = "/app-service.js";
  const char* read_position =
      "capturedStack.match(/app-service\\.js:\\d+:\\d+/)[0]";
  for (const auto& source :
       {"try { throw new Error('first line'); } catch (e) { "
        "globalThis.capturedStack = e.stack; }",
        "\nglobalThis.capture = function capture() {\n"
        "  try { throw new Error('callback'); } catch (e) {\n"
        "    globalThis.capturedStack = e.stack;\n"
        "  }\n};\ncapture();"}) {
    SCOPED_TRACE(source);
    ASSERT_TRUE(rt.evaluateJavaScript(std::make_shared<StringBuffer>(source),
                                      source_url)
                    .has_value());
    const auto expected = eval(read_position)->getString(rt).utf8(rt);
    const bool has_callback =
        std::string(source).find("capture()") != std::string::npos;

    std::string wrapped = source;
    const int offset = AddAppServiceWrapForJsContent(wrapped);
    auto buffer = std::make_shared<StringBuffer>(std::move(wrapped));
    for (bool use_bytecode : {false, true}) {
      SCOPED_TRACE(use_bytecode);
      std::unique_ptr<const PreparedJavaScript> prepared;
      if (use_bytecode) {
        cache::QuickjsCacheGenerator generator(source_url, buffer, offset);
        auto bytecode = generator.GenerateCache();
        ASSERT_NE(bytecode, nullptr);
        prepared = std::make_unique<QuickjsJavaScriptPreparation>(
            buffer, bytecode, source_url, offset);
      } else {
        prepared = rt.prepareJavaScript(buffer, source_url, offset);
      }
      ASSERT_NE(prepared, nullptr);
      auto result = rt.evaluatePreparedJavaScript(*prepared);
      ASSERT_TRUE(result.has_value());
      rt.global().setProperty(rt, "wrappedModule", std::move(*result));
      ASSERT_TRUE(eval("capturedStack = '';"
                       "wrappedModule.init({tt: {"
                       "define: function(name, body) { this.body = body; },"
                       "require: function() { this.body(); }"
                       "}});")
                      .has_value());
      EXPECT_EQ(eval(read_position)->getString(rt).utf8(rt), expected);
      if (has_callback) {
        // The function retains its original positions after initialization.
        ASSERT_TRUE(eval("capturedStack = ''; capture();").has_value());
        EXPECT_EQ(eval(read_position)->getString(rt).utf8(rt), expected);
      }
    }
  }
}

TEST_P(QuickjsRuntimeTest, SourceLineOffsetSeparatesBytecodeCache) {
  std::string wrapped = "throw new Error('test');";
  const int offset = AddAppServiceWrapForJsContent(wrapped);
  auto buffer = std::make_shared<StringBuffer>(std::move(wrapped));
  cache::QuickjsCacheGenerator original("/app-service.js", buffer);
  cache::QuickjsCacheGenerator adjusted("/app-service.js", buffer, offset);
  cache::QuickjsCacheGenerator same("/app-service.js", buffer, offset);
  cache::QuickjsCacheGenerator different("/app-service.js", buffer, offset - 1);
  EXPECT_FALSE(original.Md5().has_value());
  ASSERT_TRUE(adjusted.Md5().has_value());
  EXPECT_EQ(adjusted.Md5(), same.Md5());
  EXPECT_NE(adjusted.Md5(), different.Md5());
}

TEST_P(QuickjsRuntimeTest, PrepareJavaScriptTest) {
  rt.prepareJavaScript(std::make_unique<StringBuffer>("var foo = 0;"),
                       "/foo.js");
  CheckPrepareJSEvent("/foo.js", false, cache::JsScriptType::SOURCE, 0ul,
                      cache::JsCacheErrorCode::NO_ERROR, 1);

  auto res =
      rt.evaluateJavaScript(std::make_unique<StringBuffer>("var q = 0;"), "");
  EXPECT_TRUE(res.has_value());
  EXPECT_EQ(rt.global().getProperty(rt, "q")->getNumber(), 0);

  {
    // "q++;"
    std::string bytecode =
        "F07F2AEAAAAYAAAAUAAAAAIAAAAKAAAAAQICcR4vYXBwLXNlcnZpY2UuanMNAAYAngEA"
        "AQ"
        "ACAAANAaABAAAAOMsAAACROcsAAADKKJgDARgAAICAgJCAgICAgAEAC4CAgDAAAYCAgB"
        "A"
        "=";
    lynx_modp_b64_decode(bytecode);
    auto prep = rt.prepareJavaScript(
        std::make_shared<StringBuffer>(std::move(bytecode)), "bytecode.js");
    auto res = rt.evaluatePreparedJavaScript(*prep);
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(rt.global().getProperty(rt, "q")->getNumber(), 1);
    CheckPrepareJSEvent("bytecode.js", true, cache::JsScriptType::BINARY, 0ul,
                        cache::JsCacheErrorCode::NO_ERROR, 1);
  }

  {
    // "q++;" with target sdk version "1000.1000"
    std::string bytecode =
        "F07F2AEAAAAYAAAAUAAAAOgDAADoAwAAAQICcR4vYXBwLXNlcnZpY2UuanMNAAYAngEA"
        "AQ"
        "ACAAANAaABAAAAOMsAAACROcsAAADKKJgDARgAAICAgJCAgICAgAEAC4CAgDAAAYCAgB"
        "A"
        "=";
    lynx_modp_b64_decode(bytecode);
    auto prep = rt.prepareJavaScript(
        std::make_shared<StringBuffer>(std::move(bytecode)), "bytecode.js");
    EXPECT_EQ(prep, nullptr);
    CheckPrepareJSEvent("bytecode.js", false, cache::JsScriptType::BINARY, 0ul,
                        cache::JsCacheErrorCode::TARGET_SDK_MISMATCH, 1);
  }
}

// Apps in one group hold suppression scopes on different runtimes of a shared
// VM and release them in any order.
TEST_P(QuickjsRuntimeTest, GCPauseSuppressionEndsWithLastScopeOnSharedVM) {
  if (!rt.getGCFlag()) {
    GTEST_SKIP() << "GC pause suppression only applies to tracing GC.";
  }
  QuickjsRuntime other;
  other.InitRuntime(rt.getSharedContext());
  LEPUSRuntime* vm =
      std::static_pointer_cast<QuickjsRuntimeInstance>(rt.getSharedVM())
          ->Runtime();

  auto first = std::make_unique<GCPauseSuppressionMode>(&rt);
  auto second = std::make_unique<GCPauseSuppressionMode>(&other);
  first.reset();
  EXPECT_TRUE(LEPUS_GetGCPauseSuppressionMode(vm));
  second.reset();
  EXPECT_FALSE(LEPUS_GetGCPauseSuppressionMode(vm));
}

INSTANTIATE_TEST_SUITE_P(Runtimes, QuickjsRuntimeTest,
                         ::testing::Values(MakeRuntimeFactory<QuickjsRuntime>));
}  // namespace test
}  // namespace js
}  // namespace runtime
}  // namespace lynx
