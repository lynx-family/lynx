// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/dom/fragment/fragment.h"

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/image_fragment_behavior.h"
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
    manager->page_options_.embedded_mode_ = static_cast<EmbeddedMode>(
        static_cast<int32_t>(manager->page_options_.embedded_mode_) |
        static_cast<int32_t>(EmbeddedMode::FRAGMENT_LAYER_RENDER));
    manager->page_options_.embedded_mode_ = static_cast<EmbeddedMode>(
        static_cast<int32_t>(manager->page_options_.embedded_mode_) |
        static_cast<int32_t>(EmbeddedMode::LAYOUT_IN_ELEMENT));
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
  EXPECT_FLOAT_EQ(floats[1], 3.f);
  EXPECT_FLOAT_EQ(floats[2], 100.f - 1.f - 2.f);
  EXPECT_FLOAT_EQ(floats[3], 60.f - 3.f - 4.f);
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
  EXPECT_FLOAT_EQ(floats[1], 3.f);
  EXPECT_FLOAT_EQ(floats[2], 100.f - 1.f - 2.f);
  EXPECT_FLOAT_EQ(floats[3], 60.f - 3.f - 4.f);

  EXPECT_FLOAT_EQ(floats[4], 10.f - 1.f);
  EXPECT_FLOAT_EQ(floats[5], 12.f - 3.f);
  EXPECT_FLOAT_EQ(floats[6], 14.f - 2.f);
  EXPECT_FLOAT_EQ(floats[7], 16.f - 3.f);
  EXPECT_FLOAT_EQ(floats[8], 18.f - 2.f);
  EXPECT_FLOAT_EQ(floats[9], 20.f - 4.f);
  EXPECT_FLOAT_EQ(floats[10], 22.f - 1.f);
  EXPECT_FLOAT_EQ(floats[11], 24.f - 4.f);
}

TEST_F(FragmentTest, TestUpdateLayoutAndDefineBoxAndDrawImage) {
  auto element = manager->CreateFiberImage("image");
  Fragment fragment(element.get());
  fragment.SetBehavior(std::make_unique<ImageFragmentBehavior>(&fragment));

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

  fragment.UpdateLayout(layout);

  EXPECT_EQ(fragment.LayoutResult().layout_result.border_,
            starlight::DirectionValue<float>({1.f, 2.f, 3.f, 4.f}));
  EXPECT_EQ(fragment.LayoutResult().layout_result.padding_,
            starlight::DirectionValue<float>({0.f, 0.f, 0.f, 0.f}));
  EXPECT_EQ(fragment.LayoutResult().layout_result.margin_,
            starlight::DirectionValue<float>({0.f, 0.f, 0.f, 0.f}));

  EXPECT_EQ(fragment.LayoutResult().border_radius_info->x_top_left, 10.f);
  EXPECT_EQ(fragment.LayoutResult().border_radius_info->y_top_left, 12.f);
  EXPECT_EQ(fragment.LayoutResult().border_radius_info->x_top_right, 14.f);
  EXPECT_EQ(fragment.LayoutResult().border_radius_info->y_top_right, 16.f);
  EXPECT_EQ(fragment.LayoutResult().border_radius_info->x_bottom_right, 18.f);
  EXPECT_EQ(fragment.LayoutResult().border_radius_info->y_bottom_right, 20.f);
  EXPECT_EQ(fragment.LayoutResult().border_radius_info->x_bottom_left, 22.f);
  EXPECT_EQ(fragment.LayoutResult().border_radius_info->y_bottom_left, 24.f);

  DisplayListBuilder builder;
  EXPECT_EQ(fragment.DefineBorderBox(builder), 0);
  EXPECT_EQ(fragment.DefinePaddingBox(builder), 1);
  EXPECT_EQ(fragment.DefineContentBox(builder), 2);

  fragment.behavior_->OnDraw(builder);

  DisplayList list = builder.Build();
  const int32_t* ops = list.GetContentOpTypesData();
  const int32_t* ints = list.GetContentIntData();
  const float* floats = list.GetContentFloatData();

  ASSERT_NE(ops, nullptr);
  ASSERT_NE(ints, nullptr);
  ASSERT_NE(floats, nullptr);

  EXPECT_EQ(ops[0], static_cast<int32_t>(DisplayListOpType::kRecordBox));
  EXPECT_EQ(ints[0], 0);
  EXPECT_EQ(ints[1], 12);

  EXPECT_FLOAT_EQ(floats[0], 0.f);
  EXPECT_FLOAT_EQ(floats[1], 0.f);
  EXPECT_FLOAT_EQ(floats[2], 100.f);
  EXPECT_FLOAT_EQ(floats[3], 60.f);

  EXPECT_FLOAT_EQ(floats[4], 10.f);
  EXPECT_FLOAT_EQ(floats[5], 12.f);
  EXPECT_FLOAT_EQ(floats[6], 14.f);
  EXPECT_FLOAT_EQ(floats[7], 16.f);
  EXPECT_FLOAT_EQ(floats[8], 18.f);
  EXPECT_FLOAT_EQ(floats[9], 20.f);
  EXPECT_FLOAT_EQ(floats[10], 22.f);
  EXPECT_FLOAT_EQ(floats[11], 24.f);

  EXPECT_EQ(ops[1], static_cast<int32_t>(DisplayListOpType::kRecordBox));
  EXPECT_EQ(ints[2], 0);
  EXPECT_EQ(ints[3], 12);

  EXPECT_FLOAT_EQ(floats[12], 1.f);
  EXPECT_FLOAT_EQ(floats[13], 3.f);
  EXPECT_FLOAT_EQ(floats[14], 97.f);
  EXPECT_FLOAT_EQ(floats[15], 53.f);

  EXPECT_FLOAT_EQ(floats[16], 10.f - 1.f);
  EXPECT_FLOAT_EQ(floats[17], 12.f - 3.f);
  EXPECT_FLOAT_EQ(floats[18], 14.f - 2.f);
  EXPECT_FLOAT_EQ(floats[19], 16.f - 3.f);
  EXPECT_FLOAT_EQ(floats[20], 18.f - 2.f);
  EXPECT_FLOAT_EQ(floats[21], 20.f - 4.f);
  EXPECT_FLOAT_EQ(floats[22], 22.f - 1.f);
  EXPECT_FLOAT_EQ(floats[23], 24.f - 4.f);

  EXPECT_EQ(ops[2], static_cast<int32_t>(DisplayListOpType::kRecordBox));
  EXPECT_EQ(ints[4], 0);
  EXPECT_EQ(ints[5], 12);

  EXPECT_FLOAT_EQ(floats[24], 1.f);
  EXPECT_FLOAT_EQ(floats[25], 3.f);
  EXPECT_FLOAT_EQ(floats[26], 100.f - 1.f - 2.f);
  EXPECT_FLOAT_EQ(floats[27], 60.f - 3.f - 4.f);

  EXPECT_FLOAT_EQ(floats[28], 10.f - 1.f);
  EXPECT_FLOAT_EQ(floats[29], 12.f - 3.f);
  EXPECT_FLOAT_EQ(floats[30], 14.f - 2.f);
  EXPECT_FLOAT_EQ(floats[31], 16.f - 3.f);
  EXPECT_FLOAT_EQ(floats[32], 18.f - 2.f);
  EXPECT_FLOAT_EQ(floats[33], 20.f - 4.f);
  EXPECT_FLOAT_EQ(floats[34], 22.f - 1.f);
  EXPECT_FLOAT_EQ(floats[35], 24.f - 4.f);

  EXPECT_EQ(ops[3], static_cast<int32_t>(DisplayListOpType::kImage));
  EXPECT_EQ(ints[6], 2);
  EXPECT_EQ(ints[7], 0);
}

TEST_F(FragmentTest, TestCheckRootIfNeedClipBounds) {
  auto element = manager->CreateFiberImage("image");
  Fragment fragment(element.get());
  fragment.SetBehavior(std::make_unique<ImageFragmentBehavior>(&fragment));

  element->computed_css_style()->origin_overflow_ =
      starlight::ComputedCSSStyle::OVERFLOW_HIDDEN;

  DisplayListBuilder builder;
  fragment.CheckRootIfNeedClipBounds(builder);

  DisplayList list = builder.Build();
  EXPECT_TRUE(list.RootNeedClipBounds());
}

TEST_F(FragmentTest, TestCheckRootIfNeedClipBounds1) {
  auto element = manager->CreateFiberImage("image");
  Fragment fragment(element.get());
  fragment.SetBehavior(std::make_unique<ImageFragmentBehavior>(&fragment));

  element->computed_css_style()->origin_overflow_ =
      starlight::ComputedCSSStyle::OVERFLOW_Y;

  DisplayListBuilder builder;
  fragment.CheckRootIfNeedClipBounds(builder);

  DisplayList list = builder.Build();
  EXPECT_FALSE(list.RootNeedClipBounds());
}

TEST_F(FragmentTest, TestDrawNodeCapacity) {
  auto root = manager->CreateFiberPage("0", 0);

  auto root_child_0 = manager->CreateFiberView();
  root->InsertNode(root_child_0);

  auto root_child_1 = manager->CreateFiberView();
  root->InsertNode(root_child_1);

  auto root_child_0_child_0 = manager->CreateFiberView();
  root_child_0->InsertNode(root_child_0_child_0);

  auto root_child_0_child_1 = manager->CreateFiberView();
  root_child_0->InsertNode(root_child_0_child_1);

  root->FlushActionsAsRoot();
  EXPECT_TRUE(root->HasElementContainer());
  EXPECT_TRUE(root_child_0->HasElementContainer());
  EXPECT_TRUE(root_child_1->HasElementContainer());

  EXPECT_TRUE(root->element_container()->is_fragment());
  static_cast<Fragment*>(root->element_container())->UpdateLayout(0, 0);
  EXPECT_EQ(
      static_cast<Fragment*>(root->element_container())->draw_node_capacity_,
      5);

  static_cast<Fragment*>(root_child_0->element_container())
      ->has_platform_renderer_ = true;
  static_cast<Fragment*>(root->element_container())->UpdateLayout(0, 0);
  EXPECT_EQ(
      static_cast<Fragment*>(root->element_container())->draw_node_capacity_,
      2);
  EXPECT_EQ(static_cast<Fragment*>(root_child_0->element_container())
                ->draw_node_capacity_,
            3);
}

}  // namespace tasm
}  // namespace lynx
