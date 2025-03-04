// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/jscache/js_cache_tracker.h"
#include "core/runtime/jscache/js_cache_tracker_unittest.h"
#include "core/runtime/jsi/jsi_unittest.h"
#include "third_party/modp_b64/modp_b64.h"

namespace lynx {
namespace piper {
namespace test {
using namespace lynx::tasm::report::test;
using namespace lynx::piper::cache::testing;
class QuickjsRuntimeTest : public JSITestBase {};

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
        "F07F2AEAAAAYAAAAUAAAAAIAAAAKAAAAAQICcR4vYXBwLXNlcnZpY2UuanMNAAYAngEAAQ"
        "ACAAANAaABAAAAOMsAAACROcsAAADKKJgDARgAAICAgJCAgICAgAEAC4CAgDAAAYCAgBA"
        "=";
    modp_b64_decode(bytecode);
    auto prep = rt.prepareJavaScript(
        std::make_shared<StringBuffer>(std::move(bytecode)), "bytecode.js");
    auto res = rt.evaluatePreparedJavaScript(prep);
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(rt.global().getProperty(rt, "q")->getNumber(), 1);
    CheckPrepareJSEvent("bytecode.js", true, cache::JsScriptType::BINARY, 0ul,
                        cache::JsCacheErrorCode::NO_ERROR, 1);
  }

  {
    // "q++;" with target sdk version "1000.1000"
    std::string bytecode =
        "F07F2AEAAAAYAAAAUAAAAOgDAADoAwAAAQICcR4vYXBwLXNlcnZpY2UuanMNAAYAngEAAQ"
        "ACAAANAaABAAAAOMsAAACROcsAAADKKJgDARgAAICAgJCAgICAgAEAC4CAgDAAAYCAgBA"
        "=";
    modp_b64_decode(bytecode);
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
}  // namespace piper
}  // namespace lynx
