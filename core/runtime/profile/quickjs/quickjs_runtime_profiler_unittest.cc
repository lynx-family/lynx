// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/profile/quickjs/quickjs_runtime_profiler.h"

#include <memory>

#include "core/runtime/jsi/jsi.h"
#include "core/runtime/jsi/quickjs/quickjs_api.h"
#include "core/runtime/jsi/quickjs/quickjs_context_wrapper.h"
#include "testing/utils/make_js_runtime.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace profile {
namespace testing {

class MockHandler : public piper::JSIExceptionHandler {
 public:
  void beginFailingTest() {
    did_failed_ = false;
    expect_fail_ = true;
  }
  void endFailingTest() {
    EXPECT_TRUE(did_failed_);
    did_failed_ = false;
    expect_fail_ = false;
  }

  void onJSIException(const piper::JSIException &exception) override {
    EXPECT_TRUE(expect_fail_);
    did_failed_ = true;
  };

 private:
  bool did_failed_{false};
  bool expect_fail_{false};
};

TEST(QuickjsRuntimeProfilerTest, QuickjsRuntimeProfilerTotalTest) {
  auto handler = std::make_shared<MockHandler>();
  auto rt = ::testing::utils::makeJSRuntime(handler);
  auto ctx = rt->getSharedContext();
  auto quickjs_context =
      std::static_pointer_cast<piper::QuickjsContextWrapper>(ctx);
  auto quickjs_profiler =
      std::make_unique<QuickjsRuntimeProfiler>(quickjs_context);
  quickjs_profiler->SetupProfiling(100);
  quickjs_profiler->StartProfiling(true);
  auto buffer = std::make_unique<piper::StringBuffer>("var b = 1");
  auto ret = rt->evaluateJavaScript(std::move(buffer), "");
  auto runtime_profile = quickjs_profiler->StopProfiling(true);
  EXPECT_TRUE(ret.has_value());
  ASSERT_NE(runtime_profile->runtime_profile_, "");
}

}  // namespace testing
}  // namespace profile
}  // namespace lynx
