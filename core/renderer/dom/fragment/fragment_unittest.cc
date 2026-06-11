// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/dom/fragment/fragment.h"

#include <array>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/fragment_behavior.h"
#include "core/renderer/dom/fragment/image_fragment_behavior.h"
#include "core/renderer/lynx_env_config.h"
#include "core/renderer/starlight/types/layout_result.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/renderer/ui_wrapper/common/testing/prop_bundle_mock.h"
#include "core/renderer/ui_wrapper/painting/native_painting_context_platform_ref.h"
#include "core/renderer/ui_wrapper/painting/platform_renderer_impl.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {

// Forward-declared from fragment.cc: computes outset-adjusted border radius
// per W3C CSS Backgrounds and Borders Module Level 3.
float ComputeOutsetAdjustedRadius(float radius, float spread, float coverage);

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

class RecordingFragmentBehavior : public FragmentBehavior {
 public:
  explicit RecordingFragmentBehavior(Fragment* fragment)
      : FragmentBehavior(fragment) {}

  void CreatePlatformRenderer(
      const fml::RefPtr<PropBundle>& attributes) override {
    attributes_ = attributes;
  }

  PlatformRendererType GetType() const override {
    return PlatformRendererType::kText;
  }

  fml::RefPtr<PropBundle> attributes_;
};

class TestPlatformRenderer : public PlatformRendererImpl {
 public:
  TestPlatformRenderer(int id, PlatformRendererType type)
      : PlatformRendererImpl(id, type, base::String()) {}

 protected:
  void OnUpdateDisplayList(DisplayList display_list) override {
    if (display_list.HasContent()) {
      display_list_ = std::move(display_list);
    }
  }
  void OnUpdateAttributes(const fml::RefPtr<PropBundle>&, bool) override {}
  void OnAddChild(PlatformRenderer*) override {}
  void OnRemoveFromParent() override {}
  void OnUpdateSubtreeProperties(const DisplayList&) override {}
};

class TestPlatformRendererFactory : public PlatformRendererFactory {
 public:
  fml::RefPtr<PlatformRenderer> CreateRenderer(
      int id, PlatformRendererType type,
      const fml::RefPtr<PropBundle>&) override {
    return fml::MakeRefCounted<TestPlatformRenderer>(id, type);
  }

  fml::RefPtr<PlatformRenderer> CreateExtendedRenderer(
      int id, const base::String&, const fml::RefPtr<PropBundle>&) override {
    return fml::MakeRefCounted<TestPlatformRenderer>(
        id, PlatformRendererType::kExtended);
  }
};

class TestNativePaintingCtxPlatformRef : public NativePaintingCtxPlatformRef {
 public:
  TestNativePaintingCtxPlatformRef()
      : NativePaintingCtxPlatformRef(
            std::make_unique<TestPlatformRendererFactory>()) {}

  void GetPlatformRendererScrollOffset(int32_t sign, float offset[2]) override {
    auto it = scroll_offsets.find(sign);
    if (it == scroll_offsets.end()) {
      return;
    }
    offset[0] = it->second[0];
    offset[1] = it->second[1];
  }

  bool IsPlatformRendererScrollable(int32_t sign) override {
    return scrollable_signs.count(sign) > 0;
  }

  std::unordered_map<int32_t, std::array<float, 2>> scroll_offsets;
  std::unordered_set<int32_t> scrollable_signs;
};

TEST_F(FragmentTest, CreateLayerIfNeededWritesFlattenInitData) {
  auto element = manager->CreateFiberText("text");
  element->MarkAsDirectChildOfCompatibleComponent(true);
  Fragment fragment(element.get());
  auto behavior = std::make_unique<RecordingFragmentBehavior>(&fragment);
  auto* behavior_ptr = behavior.get();
  fragment.SetBehavior(std::move(behavior));

  ASSERT_TRUE(element->TendToFlatten());
  fragment.CreateLayerIfNeeded(nullptr);

  ASSERT_TRUE(behavior_ptr->attributes_);
  auto* props = static_cast<PropBundleMock*>(behavior_ptr->attributes_.get());
  ASSERT_TRUE(props->Contains(kTendsToFlattenInitDataKey));
  EXPECT_TRUE(props->GetPropsMap().at(kTendsToFlattenInitDataKey).Bool());
  EXPECT_TRUE(props->Contains(kDirectChildOfCompatibleComponentInitDataKey));
}

TEST_F(FragmentTest, ReusedEventTargetTreeRefreshesScrollOffsetForHitTest) {
  auto root_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      kRootId, PlatformRendererType::kPage);
  DisplayListBuilder root_builder;
  root_builder
      .Begin(kRootId, PlatformRendererType::kPage, 0.f, 0.f, 100.f, 100.f)
      .DrawView(1)
      .End();
  root_renderer->UpdateDisplayList(root_builder.Build());

  auto scroll_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      1, PlatformRendererType::kScroll);
  DisplayListBuilder scroll_builder;
  scroll_builder.Begin(1, PlatformRendererType::kScroll, 0.f, 0.f, 100.f, 50.f)
      .Begin(2, PlatformRendererType::kView, 0.f, 60.f, 20.f, 20.f)
      .End()
      .End();
  scroll_renderer->UpdateDisplayList(scroll_builder.Build());
  root_renderer->AddChild(scroll_renderer);

  TestNativePaintingCtxPlatformRef platform_ref;
  platform_ref.renderers_.insert_or_assign(kRootId, root_renderer);
  platform_ref.renderers_.insert_or_assign(1, scroll_renderer);
  platform_ref.scrollable_signs.insert(1);
  platform_ref.scroll_offsets[1] = {0.f, 0.f};
  platform_ref.need_reconstruct_event_target_tree_ = true;

  auto root_target = platform_ref.ReconstructEventTargetTreeRecursively();
  ASSERT_NE(root_target, nullptr);

  float point[2] = {10.f, 40.f};
  auto hit_target = root_target->HitTest(point);
  ASSERT_NE(hit_target, nullptr);
  EXPECT_EQ(hit_target->Sign(), 1);

  platform_ref.scroll_offsets[1] = {0.f, 30.f};
  bool did_reconstruct = true;
  auto reused_root =
      platform_ref.ReconstructEventTargetTreeRecursively(&did_reconstruct);

  EXPECT_FALSE(did_reconstruct);
  EXPECT_EQ(root_target.get(), reused_root.get());
  hit_target = reused_root->HitTest(point);
  ASSERT_NE(hit_target, nullptr);
  EXPECT_EQ(hit_target->Sign(), 2);
}

TEST_F(FragmentTest, ValidExposureEventPropsBypassEqualCheck) {
  auto element = manager->CreateFiberText("text");
  Fragment fragment(element.get());

  EXPECT_FALSE(fragment.event_bundle_dirty_);

  fragment.SetEventProp(PlatformEventPropName::kUnknown, lepus::Value("id"));
  EXPECT_FALSE(fragment.event_bundle_dirty_);

  fragment.SetEventProp(PlatformEventPropName::kIDSelector, lepus::Value("id"));
  EXPECT_TRUE(fragment.event_bundle_dirty_);

  fragment.event_bundle_dirty_ = false;
  fragment.SetEventProp(PlatformEventPropName::kIDSelector, lepus::Value("id"));
  EXPECT_FALSE(fragment.event_bundle_dirty_);

  fragment.SetEventProp(PlatformEventPropName::kIDSelector,
                        lepus::Value("next-id"));
  EXPECT_TRUE(fragment.event_bundle_dirty_);

  fragment.event_bundle_dirty_ = false;
  fragment.SetEventProp(PlatformEventPropName::kExposureId,
                        lepus::Value("exposure-id"));
  EXPECT_TRUE(fragment.event_bundle_dirty_);

  fragment.event_bundle_dirty_ = false;
  fragment.SetEventProp(PlatformEventPropName::kExposureId,
                        lepus::Value("exposure-id"));
  EXPECT_TRUE(fragment.event_bundle_dirty_);

  fragment.event_bundle_dirty_ = false;
  fragment.SetEventProp(PlatformEventPropName::kExposureScene,
                        lepus::Value("scene"));
  EXPECT_TRUE(fragment.event_bundle_dirty_);

  fragment.event_bundle_dirty_ = false;
  fragment.SetEventProp(PlatformEventPropName::kExposureScene,
                        lepus::Value("scene"));
  EXPECT_TRUE(fragment.event_bundle_dirty_);

  fragment.event_bundle_dirty_ = false;
  fragment.SetEventProp(PlatformEventPropName::kExposureScene, lepus::Value());
  EXPECT_TRUE(fragment.event_bundle_dirty_);

  fragment.event_bundle_dirty_ = false;
  fragment.SetEventProp(PlatformEventPropName::kExposureScene, lepus::Value());
  EXPECT_FALSE(fragment.event_bundle_dirty_);

  fragment.event_bundle_dirty_ = false;
  fragment.ClearEventProps();
  EXPECT_TRUE(fragment.event_bundle_dirty_);

  fragment.event_bundle_dirty_ = false;
  fragment.ClearEventProps();
  EXPECT_FALSE(fragment.event_bundle_dirty_);

  fragment.AddEventName(PlatformEventName::kUnknown);
  EXPECT_FALSE(fragment.event_bundle_dirty_);

  fragment.AddEventName(PlatformEventName::kUIAppear);
  EXPECT_TRUE(fragment.event_bundle_dirty_);

  fragment.event_bundle_dirty_ = false;
  fragment.AddEventName(PlatformEventName::kUIAppear);
  EXPECT_FALSE(fragment.event_bundle_dirty_);

  fragment.AddEventName(PlatformEventName::kUIDisappear);
  EXPECT_TRUE(fragment.event_bundle_dirty_);

  fragment.event_bundle_dirty_ = false;
  fragment.ClearEventNames();
  EXPECT_TRUE(fragment.event_bundle_dirty_);

  fragment.event_bundle_dirty_ = false;
  fragment.ClearEventNames();
  EXPECT_FALSE(fragment.event_bundle_dirty_);
}

TEST_F(FragmentTest, DrawBoxShadowWithOutsetShadow) {
  auto element = manager->CreateFiberView();
  Fragment fragment(element.get());

  starlight::LayoutResultForRendering layout;
  layout.border_ = starlight::DirectionValue<float>({1.f, 2.f, 3.f, 4.f});
  layout.padding_ = starlight::DirectionValue<float>({5.f, 6.f, 7.f, 8.f});
  layout.size_ = FloatSize(100.f, 60.f);
  fragment.UpdateLayout(layout);

  // Set up a single outset box shadow
  element->computed_css_style()->box_shadow_ =
      base::InlineVector<starlight::ShadowData, 1>();
  starlight::ShadowData shadow;
  shadow.h_offset = 3.0f;
  shadow.v_offset = 4.0f;
  shadow.blur = 5.0f;
  shadow.spread = 2.0f;
  shadow.color = 0xFF000000;
  shadow.option = starlight::ShadowOption::kNone;
  element->computed_css_style()->box_shadow_->push_back(shadow);

  DisplayListBuilder builder;
  fragment.DrawBoxShadow(builder);

  DisplayList list = builder.Build();
  const int32_t* ops = list.GetContentOpTypesData();
  const int32_t* ints = list.GetContentIntData();
  const float* floats = list.GetContentFloatData();

  ASSERT_NE(ops, nullptr);
  ASSERT_NE(ints, nullptr);
  ASSERT_NE(floats, nullptr);

  ASSERT_GE(list.GetContentOpTypesSize(), 3u);
  ASSERT_GE(list.GetContentIntDataSize(), 10u);
  ASSERT_GE(list.GetContentFloatDataSize(), 9u);

  // Op 0: RecordBox for border box (DefineBorderBox)
  // Op 1: RecordBox for shadow box
  // Op 2: BoxShadow
  EXPECT_EQ(ops[0], static_cast<int32_t>(DisplayListOpType::kRecordBox));
  EXPECT_EQ(ops[1], static_cast<int32_t>(DisplayListOpType::kRecordBox));
  EXPECT_EQ(ops[2], static_cast<int32_t>(DisplayListOpType::kBoxShadow));

  // Verify BoxShadow params:
  // Each plain RecordBox: int_count=0, float_count=4 (2 ints + 4 floats)
  // Op 0: ints[0]=0, ints[1]=4, floats[0..3]=border box
  // Op 1: ints[2]=0, ints[3]=4, floats[4..7]=shadow box
  // Op 2: ints[4]=4, ints[5]=1, ints[6..9]=params, floats[8]=blur
  EXPECT_EQ(ints[4], 4);  // int_count for BoxShadow
  EXPECT_EQ(ints[5], 1);  // float_count for BoxShadow
  EXPECT_EQ(ints[6], 1);  // shadow_box_index
  EXPECT_EQ(ints[7], 0);  // clip_box_index
  EXPECT_EQ(static_cast<uint32_t>(ints[8]), 0xFF000000);  // color
  EXPECT_EQ(ints[9], 0);                                  // clip_mode (outset)

  EXPECT_FLOAT_EQ(floats[8], 5.0f);  // blur
}

TEST_F(FragmentTest, DrawBoxShadowWithInsetShadow) {
  auto element = manager->CreateFiberView();
  Fragment fragment(element.get());

  starlight::LayoutResultForRendering layout;
  layout.border_ = starlight::DirectionValue<float>({1.f, 2.f, 3.f, 4.f});
  layout.padding_ = starlight::DirectionValue<float>({5.f, 6.f, 7.f, 8.f});
  layout.size_ = FloatSize(100.f, 60.f);
  fragment.UpdateLayout(layout);

  // Set up a single inset box shadow
  element->computed_css_style()->box_shadow_ =
      base::InlineVector<starlight::ShadowData, 1>();
  starlight::ShadowData shadow;
  shadow.h_offset = 2.0f;
  shadow.v_offset = 3.0f;
  shadow.blur = 4.0f;
  shadow.spread = 1.0f;
  shadow.color = 0x80FF0000;
  shadow.option = starlight::ShadowOption::kInset;
  element->computed_css_style()->box_shadow_->push_back(shadow);

  DisplayListBuilder builder;
  fragment.DrawBoxShadow(builder);

  DisplayList list = builder.Build();
  const int32_t* ops = list.GetContentOpTypesData();
  const int32_t* ints = list.GetContentIntData();
  const float* floats = list.GetContentFloatData();

  ASSERT_NE(ops, nullptr);
  ASSERT_NE(ints, nullptr);
  ASSERT_NE(floats, nullptr);

  ASSERT_GE(list.GetContentOpTypesSize(), 3u);
  ASSERT_GE(list.GetContentIntDataSize(), 10u);
  ASSERT_GE(list.GetContentFloatDataSize(), 9u);

  // Op 0: RecordBox for padding box (DefinePaddingBox)
  // Op 1: RecordBox for shadow box
  // Op 2: BoxShadow
  EXPECT_EQ(ops[0], static_cast<int32_t>(DisplayListOpType::kRecordBox));
  EXPECT_EQ(ops[1], static_cast<int32_t>(DisplayListOpType::kRecordBox));
  EXPECT_EQ(ops[2], static_cast<int32_t>(DisplayListOpType::kBoxShadow));

  // Verify BoxShadow params:
  // Each plain RecordBox: int_count=0, float_count=4 (2 ints + 4 floats)
  // Op 0: ints[0]=0, ints[1]=4, floats[0..3]=padding box
  // Op 1: ints[2]=0, ints[3]=4, floats[4..7]=shadow box
  // Op 2: ints[4]=4, ints[5]=1, ints[6..9]=params, floats[8]=blur
  EXPECT_EQ(ints[4], 4);  // int_count for BoxShadow
  EXPECT_EQ(ints[5], 1);  // float_count for BoxShadow
  EXPECT_EQ(ints[6], 1);  // shadow_box_index
  EXPECT_EQ(ints[7], 0);  // clip_box_index
  EXPECT_EQ(static_cast<uint32_t>(ints[8]), 0x80FF0000);  // color
  EXPECT_EQ(ints[9], 1);                                  // clip_mode (inset)

  EXPECT_FLOAT_EQ(floats[8], 4.0f);  // blur
}

TEST_F(FragmentTest, DrawBoxShadowMultipleShadows) {
  auto element = manager->CreateFiberView();
  Fragment fragment(element.get());

  starlight::LayoutResultForRendering layout;
  layout.border_ = starlight::DirectionValue<float>({0.f, 0.f, 0.f, 0.f});
  layout.padding_ = starlight::DirectionValue<float>({5.f, 5.f, 5.f, 5.f});
  layout.size_ = FloatSize(100.f, 60.f);
  fragment.UpdateLayout(layout);

  // Set up two shadows: first outset, then inset
  element->computed_css_style()->box_shadow_ =
      base::InlineVector<starlight::ShadowData, 1>();

  starlight::ShadowData shadow1;
  shadow1.h_offset = 1.0f;
  shadow1.v_offset = 2.0f;
  shadow1.blur = 3.0f;
  shadow1.spread = 0.0f;
  shadow1.color = 0xFFFF0000;
  shadow1.option = starlight::ShadowOption::kNone;
  element->computed_css_style()->box_shadow_->push_back(shadow1);

  starlight::ShadowData shadow2;
  shadow2.h_offset = -1.0f;
  shadow2.v_offset = -2.0f;
  shadow2.blur = 4.0f;
  shadow2.spread = 0.0f;
  shadow2.color = 0xFF00FF00;
  shadow2.option = starlight::ShadowOption::kInset;
  element->computed_css_style()->box_shadow_->push_back(shadow2);

  DisplayListBuilder builder;
  fragment.DrawBoxShadow(builder);

  DisplayList list = builder.Build();
  const int32_t* ops = list.GetContentOpTypesData();

  ASSERT_NE(ops, nullptr);
  ASSERT_GE(list.GetContentOpTypesSize(), 6u);

  // Shadows are drawn in reverse order (painter's algorithm):
  // shadow2 (inset) first, then shadow1 (outset)
  // For each shadow: RecordBox (clip), RecordBox (shadow), BoxShadow
  EXPECT_EQ(ops[0], static_cast<int32_t>(DisplayListOpType::kRecordBox));
  EXPECT_EQ(ops[1], static_cast<int32_t>(DisplayListOpType::kRecordBox));
  EXPECT_EQ(ops[2], static_cast<int32_t>(DisplayListOpType::kBoxShadow));
  EXPECT_EQ(ops[3], static_cast<int32_t>(DisplayListOpType::kRecordBox));
  EXPECT_EQ(ops[4], static_cast<int32_t>(DisplayListOpType::kRecordBox));
  EXPECT_EQ(ops[5], static_cast<int32_t>(DisplayListOpType::kBoxShadow));
}

TEST_F(FragmentTest, DrawBoxShadowNoShadowData) {
  auto element = manager->CreateFiberView();
  Fragment fragment(element.get());

  DisplayListBuilder builder;
  fragment.DrawBoxShadow(builder);

  DisplayList list = builder.Build();

  EXPECT_EQ(list.GetContentOpTypesSize(), 0u);
  EXPECT_EQ(list.GetContentIntDataSize(), 0u);
  EXPECT_EQ(list.GetContentFloatDataSize(), 0u);
}

TEST_F(FragmentTest, ComputeOutsetAdjustedRadiusFollowsW3CSpec) {
  // W3C formula: radius + spread * (1 - (1 - ratio)^3 * (1 - coverage^3))
  //
  // With radius=10, spread=20, coverage=0.5:
  //   ratio = 10/20 = 0.5
  //   (1 - ratio)^3 = 0.5^3 = 0.125
  //   coverage^3 = 0.5^3 = 0.125
  //   (1 - coverage^3) = 0.875
  //   result = 10 + 20 * (1 - 0.125 * 0.875)
  //          = 10 + 20 * (1 - 0.109375)
  //          = 10 + 20 * 0.890625
  //          = 10 + 17.8125
  //          = 27.8125
  EXPECT_FLOAT_EQ(ComputeOutsetAdjustedRadius(10.f, 20.f, 0.5f), 27.8125f);

  EXPECT_FLOAT_EQ(ComputeOutsetAdjustedRadius(10.f, 0.f, 0.5f), 10.f);
  EXPECT_FLOAT_EQ(ComputeOutsetAdjustedRadius(10.f, -5.f, 0.5f), 5.0f);
  EXPECT_FLOAT_EQ(ComputeOutsetAdjustedRadius(10.f, 5.f, 0.5f), 15.f);
  EXPECT_FLOAT_EQ(ComputeOutsetAdjustedRadius(5.f, 10.f, 1.5f), 15.f);
  // When both radius and coverage are zero, formula reduces to:
  //   0 + spread * (1 - (1 - 0)^3 * (1 - 0)) = 0 + spread * (1 - 1) = 0
  EXPECT_FLOAT_EQ(ComputeOutsetAdjustedRadius(0.f, 20.f, 0.f), 0.f);
  EXPECT_FLOAT_EQ(ComputeOutsetAdjustedRadius(0.f, 10.f, 0.f), 0.f);
}

TEST_F(FragmentTest, ComputeOutsetAdjustedRadiusNegativeSpread) {
  // With negative spread, the function should return max(radius + spread, 0)
  // spread < 0 takes the fast path: std::max(radius + spread, 0.f)
  EXPECT_FLOAT_EQ(ComputeOutsetAdjustedRadius(10.f, -5.f, 0.5f), 5.0f);
  // Larger negative spread than radius: clamped to zero
  EXPECT_FLOAT_EQ(ComputeOutsetAdjustedRadius(5.f, -10.f, 0.5f), 0.0f);
  // Zero radius with negative spread: clamped to zero
  EXPECT_FLOAT_EQ(ComputeOutsetAdjustedRadius(0.f, -5.f, 0.5f), 0.0f);
}

TEST_F(FragmentTest, DrawBoxShadowInsetWithLargeSpreadSkipsInvertedRect) {
  auto element = manager->CreateFiberView();
  Fragment fragment(element.get());

  starlight::LayoutResultForRendering layout;
  layout.border_ = starlight::DirectionValue<float>({0.f, 0.f, 0.f, 0.f});
  layout.padding_ = starlight::DirectionValue<float>({5.f, 5.f, 5.f, 5.f});
  layout.size_ = FloatSize(10.f, 10.f);
  fragment.UpdateLayout(layout);

  // Inset shadow with large spread that inverts the rect
  element->computed_css_style()->box_shadow_ =
      base::InlineVector<starlight::ShadowData, 1>();
  starlight::ShadowData shadow;
  shadow.h_offset = 0.0f;
  shadow.v_offset = 0.0f;
  shadow.blur = 0.0f;
  shadow.spread = 10.0f;  // Larger than half the padding box
  shadow.color = 0xFF000000;
  shadow.option = starlight::ShadowOption::kInset;
  element->computed_css_style()->box_shadow_->push_back(shadow);

  DisplayListBuilder builder;
  fragment.DrawBoxShadow(builder);

  DisplayList list = builder.Build();

  // The shadow should be skipped because spread inverts the rect
  // But padding box RecordBox is still added by DefinePaddingBox
  EXPECT_EQ(list.GetContentOpTypesSize(), 1u);
  EXPECT_EQ(list.GetContentOpTypesData()[0],
            static_cast<int32_t>(DisplayListOpType::kRecordBox));
}

TEST_F(FragmentTest, DrawBoxShadowInsetWithNegativeSpread) {
  auto element = manager->CreateFiberView();
  Fragment fragment(element.get());

  starlight::LayoutResultForRendering layout;
  layout.border_ = starlight::DirectionValue<float>({0.f, 0.f, 0.f, 0.f});
  layout.padding_ = starlight::DirectionValue<float>({0.f, 0.f, 0.f, 0.f});
  layout.size_ = FloatSize(40.f, 40.f);
  fragment.UpdateLayout(layout);

  auto* lcs = element->computed_css_style()->GetLayoutComputedStyle();
  lcs->surround_data_.border_data_ = starlight::BordersData();
  auto& bd = *lcs->surround_data_.border_data_;
  bd.radius_x_top_left = starlight::NLength::MakeUnitNLength(10.f);
  bd.radius_y_top_left = starlight::NLength::MakeUnitNLength(10.f);
  bd.radius_x_top_right = starlight::NLength::MakeUnitNLength(10.f);
  bd.radius_y_top_right = starlight::NLength::MakeUnitNLength(10.f);
  bd.radius_x_bottom_right = starlight::NLength::MakeUnitNLength(10.f);
  bd.radius_y_bottom_right = starlight::NLength::MakeUnitNLength(10.f);
  bd.radius_x_bottom_left = starlight::NLength::MakeUnitNLength(10.f);
  bd.radius_y_bottom_left = starlight::NLength::MakeUnitNLength(10.f);
  fragment.UpdateLayout(layout);

  // Inset shadow with negative spread expands outward.
  element->computed_css_style()->box_shadow_ =
      base::InlineVector<starlight::ShadowData, 1>();
  starlight::ShadowData shadow;
  shadow.h_offset = 0.0f;
  shadow.v_offset = 0.0f;
  shadow.blur = 0.0f;
  shadow.spread = -20.0f;
  shadow.color = 0xFF000000;
  shadow.option = starlight::ShadowOption::kInset;
  element->computed_css_style()->box_shadow_->push_back(shadow);

  DisplayListBuilder builder;
  fragment.DrawBoxShadow(builder);

  DisplayList list = builder.Build();
  const int32_t* ops = list.GetContentOpTypesData();
  const float* floats = list.GetContentFloatData();

  ASSERT_NE(ops, nullptr);
  ASSERT_NE(floats, nullptr);
  ASSERT_GE(list.GetContentOpTypesSize(), 3u);
  ASSERT_GE(list.GetContentFloatDataSize(), 24u);

  EXPECT_EQ(ops[0], static_cast<int32_t>(DisplayListOpType::kRecordBox));
  EXPECT_EQ(ops[1], static_cast<int32_t>(DisplayListOpType::kRecordBox));
  EXPECT_EQ(ops[2], static_cast<int32_t>(DisplayListOpType::kBoxShadow));

  // Padding box radii are at floats[4..11]; shadow box radii at floats[16..23].
  // radius=10, effective outset=20, coverage=2*min(10/40, 10/40)=0.5:
  //   ratio = 10/20 = 0.5
  //   result = 10 + 20 * (1 - 0.5^3 * (1 - 0.5^3))
  //          = 10 + 20 * (1 - 0.125 * 0.875)
  //          = 27.8125
  const float kExpectedRadius = 27.8125f;
  EXPECT_FLOAT_EQ(floats[16], kExpectedRadius);
  EXPECT_FLOAT_EQ(floats[17], kExpectedRadius);
  EXPECT_FLOAT_EQ(floats[18], kExpectedRadius);
  EXPECT_FLOAT_EQ(floats[19], kExpectedRadius);
  EXPECT_FLOAT_EQ(floats[20], kExpectedRadius);
  EXPECT_FLOAT_EQ(floats[21], kExpectedRadius);
  EXPECT_FLOAT_EQ(floats[22], kExpectedRadius);
  EXPECT_FLOAT_EQ(floats[23], kExpectedRadius);
}

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

TEST_F(FragmentTest, LinearGradientGeneratesLinearGradientOp) {
  auto element = manager->CreateFiberView();
  Fragment fragment(element.get());

  auto* style = element->computed_css_style();
  style->background_data_ = starlight::BackgroundData();
  style->background_data_->image_data =
      starlight::BackgroundData::BackgroundImageData();
  auto& image_data = *style->background_data_->image_data;
  image_data.image_count = 1;
  image_data.repeat.push_back(starlight::BackgroundRepeatType::kRepeat);
  image_data.repeat.push_back(starlight::BackgroundRepeatType::kNoRepeat);

  auto color_array = lepus::CArray::Create();
  color_array->emplace_back(0xFFFF0000);  // Red
  color_array->emplace_back(0xFF0000FF);  // Blue

  auto position_array = lepus::CArray::Create();
  position_array->emplace_back(0.0f);
  position_array->emplace_back(100.0f);

  auto gradient_obj = lepus::CArray::Create();
  gradient_obj->emplace_back(90.0f);  // Angle
  gradient_obj->emplace_back(std::move(color_array));
  gradient_obj->emplace_back(std::move(position_array));
  gradient_obj->emplace_back(
      static_cast<int32_t>(starlight::LinearGradientDirection::kRight));

  auto image_array = lepus::CArray::Create();
  image_array->emplace_back(
      static_cast<int32_t>(starlight::BackgroundImageType::kLinearGradient));
  image_array->emplace_back(std::move(gradient_obj));

  image_data.image = lepus::Value(std::move(image_array));

  DisplayListBuilder builder;
  fragment.DrawBackground(builder);

  DisplayList list = builder.Build();
  const int32_t* ops = list.GetContentOpTypesData();
  ASSERT_NE(ops, nullptr);

  // Op 0 is RecordBox (for clip)
  // Op 1 is Fill (background color)
  // Op 2 is RecordBox (for tiling box)
  // Op 3 is LinearGradient
  EXPECT_EQ(ops[3], static_cast<int32_t>(DisplayListOpType::kLinearGradient));

  const int32_t* ints = list.GetContentIntData();
  const float* floats = list.GetContentFloatData();

  // Verify gradient params
  // Op 0 (RecordBox): ints[0,1] = [0, 4]
  // Op 1 (Fill): ints[2,3] = [2, 0], ints[4,5] = [color, clip_index]
  // Op 2 (RecordBox): ints[6,7] = [0, 4]
  // Op 3 (LinearGradient): ints[8,9] = [int_count, float_count], ints[10] =
  // color_count
  EXPECT_EQ(ints[8], 8);   // int_count (1 + 2 + 1 + 4)
  EXPECT_EQ(ints[10], 2);  // color_count
  EXPECT_EQ(static_cast<uint32_t>(ints[11]), 0xFFFF0000);
  EXPECT_EQ(static_cast<uint32_t>(ints[12]), 0xFF0000FF);
  EXPECT_EQ(ints[13], 2);  // stop_count
  // repeat_x, repeat_y are at ints[16], ints[17]
  // params start at ints[10]: color_count (10), colors (11,12), stop_count
  // (13), tiling (14), clip (15), repeat_x (16), repeat_y (17)
  EXPECT_EQ(ints[16],
            static_cast<int32_t>(starlight::BackgroundRepeatType::kRepeat));
  EXPECT_EQ(ints[17],
            static_cast<int32_t>(starlight::BackgroundRepeatType::kNoRepeat));

  // Verify floats
  // Op 0: floats[0-3]
  // Op 1: (none)
  // Op 2: floats[4-7]
  // Op 3: floats[8] = angle, floats[9,10] = stops
  EXPECT_FLOAT_EQ(floats[8], 90.0f);
  EXPECT_FLOAT_EQ(floats[9], 0.0f);
  EXPECT_FLOAT_EQ(floats[10], 1.0f);
}

TEST_F(FragmentTest, BackgroundColorUsesBottomImageLayerClip) {
  auto element = manager->CreateFiberView();
  Fragment fragment(element.get());

  starlight::LayoutResultForRendering layout;
  layout.border_ = starlight::DirectionValue<float>({3.f, 5.f, 7.f, 11.f});
  layout.padding_ = starlight::DirectionValue<float>({13.f, 17.f, 19.f, 23.f});
  layout.size_ = FloatSize(100.f, 80.f);
  fragment.UpdateLayout(layout);

  auto* style = element->computed_css_style();
  style->background_data_ = starlight::BackgroundData();
  style->background_data_->color = 0xFF00FF00;
  style->background_data_->image_data =
      starlight::BackgroundData::BackgroundImageData();
  auto& image_data = *style->background_data_->image_data;
  image_data.image_count = 2;
  image_data.clip.push_back(starlight::BackgroundClipType::kBorderBox);
  image_data.clip.push_back(starlight::BackgroundClipType::kContentBox);
  image_data.clip.push_back(starlight::BackgroundClipType::kBorderBox);

  DisplayListBuilder builder;
  fragment.DrawBackground(builder);

  DisplayList list = builder.Build();
  const int32_t* ops = list.GetContentOpTypesData();
  const int32_t* ints = list.GetContentIntData();
  const float* floats = list.GetContentFloatData();

  ASSERT_NE(ops, nullptr);
  ASSERT_NE(ints, nullptr);
  ASSERT_NE(floats, nullptr);

  EXPECT_EQ(ops[0], static_cast<int32_t>(DisplayListOpType::kRecordBox));
  EXPECT_EQ(ops[1], static_cast<int32_t>(DisplayListOpType::kFill));
  EXPECT_FLOAT_EQ(floats[0], 16.f);
  EXPECT_FLOAT_EQ(floats[1], 26.f);
  EXPECT_FLOAT_EQ(floats[2], 62.f);
  EXPECT_FLOAT_EQ(floats[3], 20.f);
  EXPECT_EQ(static_cast<uint32_t>(ints[4]), 0xFF00FF00);
  EXPECT_EQ(ints[5], 0);
}

TEST_F(FragmentTest, OutsetShadowWithZeroSizeElement) {
  // Zero-sized element with border-radius and outset box-shadow.
  // Previously caused division-by-zero in apply_outset_radius.
  // Should produce valid ops without crash.

  auto element = manager->CreateFiberView();
  Fragment fragment(element.get());

  starlight::LayoutResultForRendering layout;
  layout.border_ = starlight::DirectionValue<float>({0.f, 0.f, 0.f, 0.f});
  layout.padding_ = starlight::DirectionValue<float>({0.f, 0.f, 0.f, 0.f});
  layout.size_ = FloatSize(0.f, 0.f);  // zero size
  fragment.UpdateLayout(layout);

  // Set border radius on the element
  auto* lcs = element->computed_css_style()->GetLayoutComputedStyle();
  lcs->surround_data_.border_data_ = starlight::BordersData();
  auto& bd = *lcs->surround_data_.border_data_;
  bd.radius_x_top_left = starlight::NLength::MakeUnitNLength(10.f);
  bd.radius_y_top_left = starlight::NLength::MakeUnitNLength(10.f);
  bd.radius_x_top_right = starlight::NLength::MakeUnitNLength(10.f);
  bd.radius_y_top_right = starlight::NLength::MakeUnitNLength(10.f);
  bd.radius_x_bottom_right = starlight::NLength::MakeUnitNLength(10.f);
  bd.radius_y_bottom_right = starlight::NLength::MakeUnitNLength(10.f);
  bd.radius_x_bottom_left = starlight::NLength::MakeUnitNLength(10.f);
  bd.radius_y_bottom_left = starlight::NLength::MakeUnitNLength(10.f);

  // Outset shadow with negative spread (triggers negative spread in
  // apply_outset_radius, and zero-size triggers division-by-zero guard)
  element->computed_css_style()->box_shadow_ =
      base::InlineVector<starlight::ShadowData, 1>();
  starlight::ShadowData shadow;
  shadow.h_offset = 0.0f;
  shadow.v_offset = 0.0f;
  shadow.blur = 0.0f;
  shadow.spread = -5.0f;
  shadow.color = 0xFF000000;
  shadow.option = starlight::ShadowOption::kNone;  // outset (default)
  element->computed_css_style()->box_shadow_->push_back(shadow);

  DisplayListBuilder builder;
  ASSERT_NO_FATAL_FAILURE(fragment.DrawBoxShadow(builder));

  DisplayList list = builder.Build();
  // Should produce at least one op without crash
  EXPECT_GE(list.GetContentOpTypesSize(), 1u);
}

}  // namespace tasm
}  // namespace lynx
