// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "clay/lynx_adaptor/clay_value.h"
#include "clay/lynx_adaptor/native_module/lynx_text_info_module.h"
#include "clay/lynx_adaptor/painting_context_clay.h"
#include "clay/public/value.h"
#include "clay/ui/component/view_context.h"
#include "clay/ui/shadow/shadow_node_owner.h"
#include "clay/ui/shadow/text_render.h"
#include "clay/ui/testing/ui_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace {

constexpr char kLongText[] =
    "one two three four five six seven eight nine ten eleven twelve";

struct TextInfoResult {
  double width = 0;
  std::vector<std::string> content;
};

TextInfoResult ReadTextInfoResult(std::unique_ptr<lynx::pub::Value> result) {
  TextInfoResult text_info;
  if (!result || !result->IsMap()) {
    ADD_FAILURE() << "getTextInfo must return a map";
    return text_info;
  }
  auto width = result->GetValueForKey("width");
  auto content = result->GetValueForKey("content");
  if (!width || !width->IsNumber() || !content || !content->IsArray()) {
    ADD_FAILURE() << "getTextInfo returned an invalid result";
    return text_info;
  }
  text_info.width = width->Number();
  content->ForeachArray([&text_info](int64_t, const lynx::pub::Value& line) {
    text_info.content.emplace_back(line.str());
  });
  return text_info;
}

clay::Value::Map CreateTextInfoParams(const char* font_size,
                                      double pixel_ratio) {
  clay::Value::Map params;
  if (font_size) {
    params.emplace("fontSize", clay::Value(font_size));
  }
  params.emplace("maxWidth", clay::Value("80px"));
  params.emplace("maxLine", clay::Value(20));
  params.emplace("pixelRatio", clay::Value(pixel_ratio));
  return params;
}

class GetTextInfoEntryPointTest : public UITest {
 protected:
  void UISetUp() override {
    owner_ = std::make_unique<ShadowNodeOwner>(ui_task_runner());
    view_context_ = std::make_unique<ViewContext>(page_.get(), owner_.get());
    owner_->SetViewContext(view_context_.get());
  }

  void UITearDown() override {
    owner_->SetViewContext(nullptr);
    view_context_.reset();
    owner_.reset();
  }

  void SetDevicePixelRatio(double device_pixel_ratio) {
    auto metrics = page_->GetViewportMetrics();
    metrics.device_pixel_ratio = device_pixel_ratio;
    page_->SetViewportMetrics(metrics);
  }

  std::unique_ptr<ShadowNodeOwner> owner_;
  std::unique_ptr<ViewContext> view_context_;
};

TEST_F_UI(GetTextInfoEntryPointTest, PaintingContextIgnoresParamsPixelRatio) {
  lynx::tasm::PaintingContextClay painting_context(view_context_.get());
  lynx::tasm::PaintingCtxPlatformImpl* api = &painting_context;
  SetDevicePixelRatio(2.5);
  auto get_text_info = [&](double params_pixel_ratio) {
    lynx::ClayValue info(
        clay::Value(CreateTextInfoParams("20px", params_pixel_ratio)));
    return ReadTextInfoResult(api->GetTextInfo(kLongText, info));
  };

  const auto baseline = get_text_info(1.0);
  const auto misleading_params = get_text_info(7.0);
  EXPECT_GT(baseline.width, 0);
  EXPECT_LE(baseline.width, 80);
  EXPECT_GT(baseline.content.size(), 1u);
  EXPECT_DOUBLE_EQ(baseline.width, misleading_params.width);
  EXPECT_EQ(baseline.content, misleading_params.content);
}

TEST_F_UI(GetTextInfoEntryPointTest, NativeModuleCoversPublicApi) {
  lynx::LynxTextInfoModule module(view_context_->unique_id());
  lynx::runtime::CallbackMap callbacks;
  SetDevicePixelRatio(2.5);
  auto get_text_info = [&](double params_pixel_ratio) {
    clay::Value::Array args;
    args.emplace_back(kLongText);
    args.emplace_back(
        clay::Value(CreateTextInfoParams(nullptr, params_pixel_ratio)));
    auto invocation = module.InvokeMethod(
        "getTextInfo",
        std::make_unique<lynx::ClayValue>(clay::Value(std::move(args))), 2,
        callbacks);
    if (!invocation.has_value()) {
      ADD_FAILURE() << invocation.error();
      return TextInfoResult{};
    }
    return ReadTextInfoResult(std::move(invocation.value()));
  };

  const auto baseline = get_text_info(1.0);
  const auto misleading_params = get_text_info(7.0);
  EXPECT_GT(baseline.width, 0);
  EXPECT_LE(baseline.width, 80);
  EXPECT_GT(baseline.content.size(), 1u);
  EXPECT_DOUBLE_EQ(baseline.width, misleading_params.width);
  EXPECT_EQ(baseline.content, misleading_params.content);
}

TEST_F_UI(GetTextInfoEntryPointTest,
          TextRenderFallsBackToParamsPixelRatioWithoutPageView) {
  auto expect_fallback = [&](const char* font_size) {
    auto get_text_info = [&](double params_pixel_ratio) {
      return ReadTextInfoResult(
          std::make_unique<lynx::ClayValue>(TextRender::GetTextInfo(
              kLongText,
              clay::Value(CreateTextInfoParams(font_size, params_pixel_ratio)),
              nullptr)));
    };

    const auto baseline = get_text_info(1.0);
    const auto fractional_ratio = get_text_info(2.5);
    EXPECT_GT(baseline.width, 0);
    EXPECT_LE(baseline.width, 80);
    EXPECT_GT(baseline.content.size(), 1u);
    EXPECT_NEAR(baseline.width, fractional_ratio.width, 0.001);
    EXPECT_EQ(baseline.content, fractional_ratio.content);

    for (double invalid_ratio :
         {0.0, -1.0, std::numeric_limits<double>::quiet_NaN(),
          std::numeric_limits<double>::infinity()}) {
      const auto invalid_result = get_text_info(invalid_ratio);
      EXPECT_DOUBLE_EQ(baseline.width, invalid_result.width);
      EXPECT_EQ(baseline.content, invalid_result.content);
    }
  };

  expect_fallback("20px");
  expect_fallback(nullptr);
}

}  // namespace
}  // namespace clay
