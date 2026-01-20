// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/text_codec_helper.h"

#include "testing/utils/make_js_runtime.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace runtime {
namespace js {
namespace test {

class MockHandler : public JSIExceptionHandler {
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

  void onJSIException(const JSIException& exception) override {
    EXPECT_TRUE(expect_fail_);
    did_failed_ = true;
  };

 private:
  bool did_failed_{false};
  bool expect_fail_{false};
};

TEST(TextCodecHelper, TestTextCodecHelper) {
  auto handler = std::make_shared<MockHandler>();
  auto rt = testing::utils::makeJSRuntime(handler);

  auto global = rt->global();
  Object helper_obj =
      Object::createFromHostObject(*rt, std::make_shared<TextCodecHelper>());
  global.setProperty(*rt, "TextCodecHelper", helper_obj);

  auto ret = rt->evaluateJavaScript(std::make_unique<StringBuffer>(R"""(
        const str = "HelloWorld△❎";
        TextCodecHelper.decode(TextCodecHelper.encode(str)) === str
        )"""),
                                    "");

  ASSERT_TRUE((ret.has_value() && ret->getBool()));
}

}  // namespace test
}  // namespace js
}  // namespace runtime
}  // namespace lynx
