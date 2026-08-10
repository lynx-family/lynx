// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "clay/lynx_adaptor/clay_value.h"
#include "clay/lynx_adaptor/native_module/lynx_module_factory.h"
#include "clay/lynx_adaptor/native_module/lynx_text_info_module.h"
#include "clay/lynx_adaptor/painting_context_clay.h"
#include "clay/public/value.h"
#include "clay/ui/component/view_context.h"
#include "clay/ui/shadow/shadow_node_owner.h"
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

clay::Value::Map CreateTextInfoParams(
    std::optional<double> pixel_ratio = std::nullopt) {
  clay::Value::Map params;
  params.emplace("fontSize", clay::Value("20px"));
  params.emplace("maxWidth", clay::Value("80px"));
  params.emplace("maxLine", clay::Value(20));
  if (pixel_ratio) {
    params.emplace("pixelRatio", clay::Value(*pixel_ratio));
  }
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
  auto get_text_info = [&](double device_pixel_ratio,
                           double params_pixel_ratio) {
    SetDevicePixelRatio(device_pixel_ratio);
    lynx::ClayValue info(clay::Value(CreateTextInfoParams(params_pixel_ratio)));
    return ReadTextInfoResult(api->GetTextInfo(kLongText, info));
  };

  const auto logical_result = get_text_info(1.0, 1.0);
  const auto high_dpr_result = get_text_info(2.5, 2.5);
  const auto misleading_params_result = get_text_info(2.5, 7.0);
  EXPECT_GT(logical_result.width, 0);
  EXPECT_LE(logical_result.width, 80);
  EXPECT_GT(logical_result.content.size(), 1u);
  EXPECT_DOUBLE_EQ(logical_result.width, high_dpr_result.width);
  EXPECT_EQ(logical_result.content, high_dpr_result.content);
  EXPECT_DOUBLE_EQ(high_dpr_result.width, misleading_params_result.width);
  EXPECT_EQ(high_dpr_result.content, misleading_params_result.content);
}

TEST_F_UI(GetTextInfoEntryPointTest, NativeModuleCoversPublicApi) {
  std::unique_ptr<lynx::runtime::NativeModuleFactory> factory(
      lynx::LynxModuleFactory::CreateModuleFactory(view_context_.get()));
  auto module = factory->CreateModule(lynx::LynxTextInfoModule::GetName());
  ASSERT_NE(module, nullptr);
  lynx::runtime::CallbackMap callbacks;

  auto get_text_info = [&](double device_pixel_ratio,
                           std::optional<double> params_pixel_ratio) {
    SetDevicePixelRatio(device_pixel_ratio);
    clay::Value::Array args;
    args.emplace_back(kLongText);
    args.emplace_back(clay::Value(CreateTextInfoParams(params_pixel_ratio)));
    auto invocation = module->InvokeMethod(
        "getTextInfo",
        std::make_unique<lynx::ClayValue>(clay::Value(std::move(args))), 2,
        callbacks);
    if (!invocation.has_value()) {
      ADD_FAILURE() << invocation.error();
      return TextInfoResult{};
    }
    return ReadTextInfoResult(std::move(invocation.value()));
  };

  const auto logical_result = get_text_info(1.0, std::nullopt);
  const auto high_dpr_result = get_text_info(2.5, std::nullopt);
  const auto misleading_params_result = get_text_info(2.5, 7.0);
  EXPECT_GT(logical_result.width, 0);
  EXPECT_LE(logical_result.width, 80);
  EXPECT_GT(logical_result.content.size(), 1u);
  EXPECT_DOUBLE_EQ(logical_result.width, high_dpr_result.width);
  EXPECT_EQ(logical_result.content, high_dpr_result.content);
  EXPECT_DOUBLE_EQ(high_dpr_result.width, misleading_params_result.width);
  EXPECT_EQ(high_dpr_result.content, misleading_params_result.content);

  auto invalid_args = module->InvokeMethod(
      "getTextInfo",
      std::make_unique<lynx::ClayValue>(clay::Value(clay::Value::Map{})), 1,
      callbacks);
  ASSERT_FALSE(invalid_args.has_value());
  EXPECT_EQ(invalid_args.error(), "Invalid argument count");

  auto unknown_method = module->InvokeMethod(
      "unknown",
      std::make_unique<lynx::ClayValue>(clay::Value(clay::Value::Array{})), 0,
      callbacks);
  ASSERT_FALSE(unknown_method.has_value());
  EXPECT_EQ(unknown_method.error(), "Unknown method");
}

}  // namespace
}  // namespace clay
