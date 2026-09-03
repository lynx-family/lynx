// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <array>
#include <memory>
#include <string>

#include "clay/ui/component/image_view.h"
#include "clay/ui/lynx_module/lynx_ui_method_types.h"
#include "clay/ui/testing/ui_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace {

class BaseImageViewTest : public UITest {
 protected:
  void UISetUp() override {
    image_view_ = std::make_unique<ImageView>(1, page_.get());
  }

  void UITearDown() override { image_view_.reset(); }

  std::unique_ptr<ImageView> image_view_;
};

}  // namespace

TEST_F_UI(BaseImageViewTest, AnimationUIMethodsReturnEmptySuccessData) {
  constexpr std::array<const char*, 4> kMethods = {
      "startAnimate", "resumeAnimation", "pauseAnimation", "stopAnimation"};

  for (const char* method : kMethods) {
    bool callback_invoked = false;
    InvokeUIMethod(image_view_.get(), method, {},
                   [&callback_invoked, method](LynxUIMethodResult code,
                                               const clay::Value& data) {
                     callback_invoked = true;
                     EXPECT_EQ(code, LynxUIMethodResult::kSuccess) << method;
                     EXPECT_TRUE(data.IsNone()) << method;
                   });
    EXPECT_TRUE(callback_invoked) << method;
  }
}

}  // namespace clay
