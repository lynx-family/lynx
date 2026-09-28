// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bytecode/js_cache_tracker.h"
#include "core/runtime/js/bytecode/js_cache_tracker_unittest.h"
#include "core/runtime/js/jsi/jsi_unittest.h"
#include "third_party/modp_b64/modp_b64.h"

namespace lynx {
namespace runtime {
namespace js {
namespace test {
using namespace lynx::tasm::report::test;
using namespace cache::testing;
class QuickjsRuntimeTest : public JSITestBase {};

TEST_P(QuickjsRuntimeTest, PreservesEmbeddedNullInStringsAndPropertyReads) {
  Scope scope(rt);
  auto result = eval(R"(({key:'prefix', 'key\u0000suffix':'a\u0000b'}))");
  ASSERT_TRUE(result);
  auto object = result->getObject(rt);
  const std::string name("key\0suffix", 10);
  auto string_name = String::createFromUtf8(rt, name);
  auto prop_name = PropNameID::forUtf8(rt, name);
  EXPECT_EQ(string_name.utf8(rt), name);
  EXPECT_EQ(prop_name.utf8(rt), name);
  for (const auto& value : {object.getProperty(rt, string_name),
                            object.getProperty(rt, prop_name)}) {
    ASSERT_TRUE(value);
    ASSERT_TRUE(value->isString());
    EXPECT_EQ(value->getString(rt).utf8(rt), std::string("a\0b", 3));
  }
  EXPECT_EQ(object.getProperty(rt, "key")->getString(rt).utf8(rt), "prefix");
}

TEST_P(QuickjsRuntimeTest, ReportsPropertyGetterFailure) {
  Scope scope(rt);
  auto result = eval(R"(({get value() { throw Error('getter failed'); }}))");
  ASSERT_TRUE(result);
  auto object = result->getObject(rt);
  EXPECT_CALL(*exception_handler_, OnJSIException).Times(2);
  EXPECT_FALSE(object.getProperty(rt, String::createFromUtf8(rt, "value")));
  EXPECT_FALSE(object.getProperty(rt, PropNameID::forAscii(rt, "value")));
  EXPECT_EQ(eval("1 + 1")->getNumber(), 2);
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

INSTANTIATE_TEST_SUITE_P(Runtimes, QuickjsRuntimeTest,
                         ::testing::Values(MakeRuntimeFactory<QuickjsRuntime>));
}  // namespace test
}  // namespace js
}  // namespace runtime
}  // namespace lynx
