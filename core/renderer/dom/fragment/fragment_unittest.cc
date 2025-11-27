// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/dom/fragment/fragment.h"

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/lynx_env_config.h"
#include "core/renderer/starlight/types/layout_result.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {

static constexpr int32_t kConfigWidth = 1080;
static constexpr int32_t kConfigHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

class FragmentTest : public ::testing::Test {
 public:
  FragmentTest() {}
  ~FragmentTest() override {}

  void SetUp() override {
    LynxEnvConfig lynx_env_config(kConfigWidth, kConfigHeight,
                                  kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    manager = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<MockPaintingContext>(), tasm_mediator.get(),
        lynx_env_config);
    auto config = std::make_shared<PageConfig>();
    manager->enable_fragment_layer_render_ = true;
    manager->enable_layout_in_element_mode_ = true;
    config->SetEnableZIndex(true);
    config->SetEnableFiberArch(true);
    manager->SetConfig(config);
  }

  std::unique_ptr<lynx::tasm::ElementManager> manager;
  std::shared_ptr<::testing::NiceMock<test::MockTasmDelegate>> tasm_mediator;
};

TEST_F(FragmentTest, PlainRectGeneratesClipRectOp) {
  auto element = manager->CreateFiberText("text");
  Fragment fragment(element.get());

  starlight::LayoutResultForRendering layout;
  layout.border_ = starlight::DirectionValue<float>({1.f, 2.f, 3.f, 4.f});
  layout.size_ = FloatSize(100.f, 60.f);
  fragment.UpdateLayout(layout);

  element->computed_css_style()->origin_overflow_ = 0;

  DisplayListBuilder builder;
  fragment.DrawClip(builder);

  DisplayList list = builder.Build();
  const int32_t* ops = list.GetContentOpTypesData();
  const int32_t* ints = list.GetContentIntData();
  const float* floats = list.GetContentFloatData();

  ASSERT_NE(ops, nullptr);
  ASSERT_NE(ints, nullptr);
  ASSERT_NE(floats, nullptr);

  EXPECT_EQ(ops[0], static_cast<int32_t>(DisplayListOpType::kClipRect));
  EXPECT_EQ(ints[0], 0);
  EXPECT_EQ(ints[1], 4);

  EXPECT_FLOAT_EQ(floats[0], 1.f);
  EXPECT_FLOAT_EQ(floats[1], 2.f);
  EXPECT_FLOAT_EQ(floats[2], 100.f - 1.f - 3.f);
  EXPECT_FLOAT_EQ(floats[3], 60.f - 2.f - 4.f);
}

TEST_F(FragmentTest, RoundedRectGeneratesClipPathOpParams) {
  auto element = manager->CreateFiberText("text");
  Fragment fragment(element.get());

  starlight::LayoutResultForRendering layout;
  layout.border_ = starlight::DirectionValue<float>({1.f, 2.f, 3.f, 4.f});
  layout.size_ = FloatSize(100.f, 60.f);
  fragment.UpdateLayout(layout);

  element->computed_css_style()->origin_overflow_ =
      starlight::ComputedCSSStyle::OVERFLOW_HIDDEN;

  auto* lcs = element->computed_css_style()->GetLayoutComputedStyle();
  lcs->surround_data_.border_data_ = starlight::BordersData();
  auto& bd = *lcs->surround_data_.border_data_;
  bd.radius_x_top_left = starlight::NLength::MakeUnitNLength(10.f);
  bd.radius_y_top_left = starlight::NLength::MakeUnitNLength(12.f);
  bd.radius_x_top_right = starlight::NLength::MakeUnitNLength(14.f);
  bd.radius_y_top_right = starlight::NLength::MakeUnitNLength(16.f);
  bd.radius_x_bottom_right = starlight::NLength::MakeUnitNLength(18.f);
  bd.radius_y_bottom_right = starlight::NLength::MakeUnitNLength(20.f);
  bd.radius_x_bottom_left = starlight::NLength::MakeUnitNLength(22.f);
  bd.radius_y_bottom_left = starlight::NLength::MakeUnitNLength(24.f);

  DisplayListBuilder builder;
  fragment.DrawClip(builder);

  DisplayList list = builder.Build();
  const int32_t* ops = list.GetContentOpTypesData();
  const int32_t* ints = list.GetContentIntData();
  const float* floats = list.GetContentFloatData();

  ASSERT_NE(ops, nullptr);
  ASSERT_NE(ints, nullptr);
  ASSERT_NE(floats, nullptr);

  EXPECT_EQ(ops[0], static_cast<int32_t>(DisplayListOpType::kClipRect));
  EXPECT_EQ(ints[0], 0);
  EXPECT_EQ(ints[1], 12);

  EXPECT_FLOAT_EQ(floats[0], 1.f);
  EXPECT_FLOAT_EQ(floats[1], 2.f);
  EXPECT_FLOAT_EQ(floats[2], 100.f - 1.f - 3.f);
  EXPECT_FLOAT_EQ(floats[3], 60.f - 2.f - 4.f);

  EXPECT_FLOAT_EQ(floats[4], 10.f - 1.f);
  EXPECT_FLOAT_EQ(floats[5], 12.f - 2.f);
  EXPECT_FLOAT_EQ(floats[6], 14.f - 3.f);
  EXPECT_FLOAT_EQ(floats[7], 16.f - 2.f);
  EXPECT_FLOAT_EQ(floats[8], 18.f - 3.f);
  EXPECT_FLOAT_EQ(floats[9], 20.f - 4.f);
  EXPECT_FLOAT_EQ(floats[10], 22.f - 1.f);
  EXPECT_FLOAT_EQ(floats[11], 24.f - 4.f);
}

}  // namespace tasm
}  // namespace lynx
