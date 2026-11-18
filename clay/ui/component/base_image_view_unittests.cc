// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <array>
#include <memory>
#include <string>
#include <utility>

#include "clay/ui/component/image_view.h"
#include "clay/ui/lynx_module/lynx_ui_method_types.h"
#include "clay/ui/testing/ui_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace {

class BaseImageViewTest : public UITest {
 protected:
  class TestImageView : public ImageView {
   public:
    using ImageView::ImageView;

    RenderImage* render_image() { return GetRenderImage(); }
  };

  void UISetUp() override {
    image_view_ = std::make_unique<TestImageView>(1, page_.get());
  }

  void UITearDown() override { image_view_.reset(); }

  std::unique_ptr<TestImageView> image_view_;
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

TEST_F_UI(BaseImageViewTest, SupportsAdditionalImageModes) {
  constexpr std::array<std::pair<const char*, FillMode>, 13> kModes = {{
      {"scaleToFill", FillMode::kScaleToFill},
      {"aspectFit", FillMode::kAspectFit},
      {"aspectFill", FillMode::kAspectFill},
      {"center", FillMode::kCenter},
      {"widthFix", FillMode::kWidthFix},
      {"heightFix", FillMode::kHeightFix},
      {"top", FillMode::kTop},
      {"bottom", FillMode::kBottom},
      {"left", FillMode::kLeft},
      {"right", FillMode::kRight},
      {"top left", FillMode::kTopLeft},
      {"top right", FillMode::kTopRight},
      {"bottom left", FillMode::kBottomLeft},
  }};

  for (const auto& [mode, expected] : kModes) {
    image_view_->SetMode(mode);
    EXPECT_EQ(image_view_->render_image()->GetMode(), expected) << mode;
  }

  image_view_->SetMode("bottom right");
  EXPECT_EQ(image_view_->render_image()->GetMode(), FillMode::kBottomRight);

  image_view_->SetAutoSize(false);
  image_view_->SetMode("widthFix");
  EXPECT_TRUE(image_view_->render_image()->UsesAutoSize());
  image_view_->SetMode("aspectFit");
  EXPECT_FALSE(image_view_->render_image()->UsesAutoSize());
}

}  // namespace clay
