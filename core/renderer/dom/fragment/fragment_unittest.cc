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

#include "base/include/value/array.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/event/platform_event_handler.h"
#include "core/renderer/dom/fragment/event/platform_event_target.h"
#include "core/renderer/dom/fragment/event/platform_event_target_exposure.h"
#include "core/renderer/dom/fragment/fragment_behavior.h"
#include "core/renderer/dom/fragment/image_fragment_behavior.h"
#include "core/renderer/dom/lynx_get_ui_result.h"
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
  TestPlatformRenderer(int id, PlatformRendererType type,
                       const base::String& tag)
      : PlatformRendererImpl(id, type, tag) {}

 protected:
  void OnUpdateDisplayList(DisplayList display_list) override {
    if (display_list.HasContent()) {
      display_list_ = std::move(display_list);
    }
  }
  void OnUpdateAttributes(const fml::RefPtr<PropBundle>&, bool) override {}
  void OnAddChild(PlatformRenderer*, int) override {}
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
      int id, const base::String& tag_name,
      const fml::RefPtr<PropBundle>&) override {
    return fml::MakeRefCounted<TestPlatformRenderer>(
        id, PlatformRendererType::kExtended, tag_name);
  }
};

class TestNativePaintingCtxPlatformRef : public NativePaintingCtxPlatformRef {
 public:
  TestNativePaintingCtxPlatformRef()
      : NativePaintingCtxPlatformRef(
            std::make_unique<TestPlatformRendererFactory>()) {}

  void GetPlatformRendererScrollOffset(int32_t sign, float offset[2]) override {
    scroll_offset_query_counts[sign]++;
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

  void GetRootViewLocationOnScreen(float location[2]) override {
    root_view_location_on_screen_query_count++;
    location[0] = root_view_location_on_screen[0];
    location[1] = root_view_location_on_screen[1];
  }

  void GetScreenSize(float size[2]) override {
    screen_size_query_count++;
    size[0] = screen_size[0];
    size[1] = screen_size[1];
  }

  void MarkEventTargetRootDirtyForTest(int32_t root_id) {
    MarkEventTargetRootDirty(root_id);
  }

  void MarkAllEventTargetRootsDirtyForTest() {
    MarkEventTargetRootDirty(kRootId);
    for (const auto root_id : GetEventTargetHelper()->GetActiveEventRootIds()) {
      MarkEventTargetRootDirty(root_id);
    }
  }

  std::unordered_map<int32_t, std::array<float, 2>> scroll_offsets;
  std::unordered_map<int32_t, int32_t> scroll_offset_query_counts;
  std::unordered_set<int32_t> scrollable_signs;
  std::array<float, 2> root_view_location_on_screen{0.f, 0.f};
  int32_t root_view_location_on_screen_query_count{0};
  std::array<float, 2> screen_size{1000.f, 1000.f};
  int32_t screen_size_query_count{0};
};

lepus::Value GetProperty(const lepus::Value& value, const char* key) {
  return value.GetProperty(base::String(key));
}

lepus::Value CreateEventThroughActiveRegionsValue(
    const std::array<const char*, 4>& region_values) {
  auto region = lepus::CArray::Create();
  for (const auto* value : region_values) {
    region->push_back(lepus::Value(value));
  }
  auto regions = lepus::CArray::Create();
  regions->push_back(lepus::Value(region));
  return lepus::Value(regions);
}

lepus::Value CreateNumericEventThroughActiveRegionsValue(
    const std::array<int32_t, 4>& region_values) {
  auto region = lepus::CArray::Create();
  for (const auto value : region_values) {
    region->push_back(lepus::Value(value));
  }
  auto regions = lepus::CArray::Create();
  regions->push_back(lepus::Value(region));
  return lepus::Value(regions);
}

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
  platform_ref.MarkEventTargetRootDirtyForTest(kRootId);

  auto root_target = platform_ref.EnsureEventTargetTree(kRootId);
  ASSERT_NE(root_target, nullptr);

  float point[2] = {10.f, 40.f};
  auto hit_target = root_target->HitTest(point);
  ASSERT_NE(hit_target, nullptr);
  EXPECT_EQ(hit_target->Sign(), 1);

  platform_ref.scroll_offsets[1] = {0.f, 30.f};
  auto reused_root = platform_ref.EnsureEventTargetTree(kRootId);

  EXPECT_EQ(root_target.get(), reused_root.get());
  hit_target = reused_root->HitTest(point);
  ASSERT_NE(hit_target, nullptr);
  EXPECT_EQ(hit_target->Sign(), 2);
}

TEST_F(FragmentTest, ReconstructEventTargetTreeWithOverlayRoot) {
  auto root_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      kRootId, PlatformRendererType::kPage);
  DisplayListBuilder root_builder;
  root_builder
      .Begin(kRootId, PlatformRendererType::kPage, 0.f, 0.f, 100.f, 100.f)
      .DrawView(1)
      .End();
  root_renderer->UpdateDisplayList(root_builder.Build());

  auto overlay_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      1, PlatformRendererType::kExtended, base::String("overlay"));
  DisplayListBuilder overlay_builder;
  overlay_builder.Begin(1, PlatformRendererType::kExtended, 0.f, 0.f, 0.f, 0.f)
      .Begin(2, PlatformRendererType::kView, 0.f, 0.f, 100.f, 100.f)
      .End()
      .End();
  overlay_renderer->UpdateDisplayList(overlay_builder.Build());
  root_renderer->AddChild(overlay_renderer);

  TestNativePaintingCtxPlatformRef platform_ref;
  platform_ref.renderers_.insert_or_assign(kRootId, root_renderer);
  platform_ref.renderers_.insert_or_assign(1, overlay_renderer);
  platform_ref.MarkAllEventTargetRootsDirtyForTest();

  auto overlay_target = platform_ref.EnsureEventTargetTree(1);
  EXPECT_EQ(overlay_target, nullptr);

  auto root_target = platform_ref.EnsureEventTargetTree(kRootId);
  ASSERT_NE(root_target, nullptr);
  EXPECT_TRUE(root_target->IsRoot());
  EXPECT_EQ(platform_ref.GetEventTargetHelper()->GetEventTarget(1), nullptr);

  platform_ref.SetPlatformEventRootActive(1, true);
  overlay_target = platform_ref.EnsureEventTargetTree(1);
  ASSERT_NE(overlay_target, nullptr);
  EXPECT_EQ(overlay_target->Sign(), 1);
  EXPECT_TRUE(overlay_target->IsRoot());
  EXPECT_EQ(overlay_target->ParentTarget().get(), nullptr);

  auto active_overlay_target =
      platform_ref.GetEventTargetHelper()->GetEventTarget(1);
  ASSERT_NE(active_overlay_target, nullptr);
  EXPECT_EQ(active_overlay_target.get(), overlay_target.get());

  float point[2] = {10.f, 10.f};
  auto hit_target = overlay_target->HitTest(point);
  ASSERT_NE(hit_target, nullptr);
  EXPECT_EQ(hit_target->Sign(), 2);
  EXPECT_FALSE(hit_target->IsRoot());
  auto hit_parent = hit_target->ParentTarget();
  ASSERT_NE(hit_parent, nullptr);
  EXPECT_EQ(hit_parent.get(), overlay_target.get());
  EXPECT_EQ(hit_parent->ParentTarget().get(), nullptr);

  platform_ref.GetEventTargetHelper()->event_target_trees_.erase(1);
  EXPECT_TRUE(platform_ref.EnsureEventTargetTreeForTarget(2));
  overlay_target = platform_ref.GetEventTargetHelper()->GetEventRootTree(1);
  ASSERT_NE(overlay_target, nullptr);
  EXPECT_NE(platform_ref.GetEventTargetHelper()->GetEventTarget(2), nullptr);

  platform_ref.GetEventTargetHelper()->RemoveEventTargetsInEventRoot(1);
  platform_ref.GetEventTargetHelper()->event_target_trees_.erase(1);
  EXPECT_EQ(platform_ref.GetEventTargetHelper()->GetEventTarget(2), nullptr);
  EXPECT_TRUE(platform_ref.EnsureEventTargetTreeForTarget(2));
  EXPECT_NE(platform_ref.GetEventTargetHelper()->GetEventTarget(2), nullptr);
  EXPECT_NE(platform_ref.GetEventTargetHelper()->GetEventRootTree(1), nullptr);

  DisplayListBuilder overlay_without_child_builder;
  overlay_without_child_builder
      .Begin(1, PlatformRendererType::kExtended, 0.f, 0.f, 0.f, 0.f)
      .End();
  overlay_renderer->UpdateDisplayList(overlay_without_child_builder.Build());
  platform_ref.GetEventTargetHelper()->event_target_trees_.erase(1);
  ASSERT_NE(platform_ref.GetEventTargetHelper()->GetEventTarget(2), nullptr);
  ASSERT_NE(platform_ref.EnsureEventTargetTree(1), nullptr);
  EXPECT_EQ(platform_ref.GetEventTargetHelper()->GetEventTarget(2), nullptr);

  platform_ref.SetPlatformEventRootActive(1, false);
  EXPECT_EQ(platform_ref.EnsureEventTargetTree(1), nullptr);
  EXPECT_EQ(platform_ref.GetEventTargetHelper()->GetEventTarget(1), nullptr);
}

TEST_F(FragmentTest, ReusedOverlayRootOnlyRefreshesOverlayScrollOffset) {
  auto root_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      kRootId, PlatformRendererType::kPage);
  DisplayListBuilder root_builder;
  root_builder
      .Begin(kRootId, PlatformRendererType::kPage, 0.f, 0.f, 100.f, 100.f)
      .DrawView(1)
      .DrawView(2)
      .End();
  root_renderer->UpdateDisplayList(root_builder.Build());

  auto page_scroll_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      1, PlatformRendererType::kScroll);
  DisplayListBuilder page_scroll_builder;
  page_scroll_builder
      .Begin(1, PlatformRendererType::kScroll, 0.f, 0.f, 100.f, 100.f)
      .End();
  page_scroll_renderer->UpdateDisplayList(page_scroll_builder.Build());
  root_renderer->AddChild(page_scroll_renderer);

  auto overlay_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      2, PlatformRendererType::kExtended, base::String("overlay"));
  DisplayListBuilder overlay_builder;
  overlay_builder
      .Begin(2, PlatformRendererType::kExtended, 0.f, 0.f, 100.f, 100.f)
      .Begin(3, PlatformRendererType::kScroll, 0.f, 0.f, 100.f, 100.f)
      .End()
      .End();
  overlay_renderer->UpdateDisplayList(overlay_builder.Build());
  root_renderer->AddChild(overlay_renderer);

  TestNativePaintingCtxPlatformRef platform_ref;
  platform_ref.renderers_.insert_or_assign(kRootId, root_renderer);
  platform_ref.renderers_.insert_or_assign(1, page_scroll_renderer);
  platform_ref.renderers_.insert_or_assign(2, overlay_renderer);
  platform_ref.scrollable_signs.insert(1);
  platform_ref.scrollable_signs.insert(3);
  platform_ref.MarkAllEventTargetRootsDirtyForTest();

  platform_ref.SetPlatformEventRootActive(2, true);
  platform_ref.scroll_offset_query_counts.clear();

  auto overlay_target = platform_ref.EnsureEventTargetTree(2);
  ASSERT_NE(overlay_target, nullptr);
  EXPECT_EQ(platform_ref.scroll_offset_query_counts[1], 0);
  EXPECT_EQ(platform_ref.scroll_offset_query_counts[3], 1);
}

TEST_F(FragmentTest, ReusedPageRootOnlyRefreshesPageRootScrollOffset) {
  auto root_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      kRootId, PlatformRendererType::kPage);
  DisplayListBuilder root_builder;
  root_builder
      .Begin(kRootId, PlatformRendererType::kPage, 0.f, 0.f, 100.f, 100.f)
      .DrawView(1)
      .DrawView(2)
      .End();
  root_renderer->UpdateDisplayList(root_builder.Build());

  auto page_scroll_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      1, PlatformRendererType::kScroll);
  DisplayListBuilder page_scroll_builder;
  page_scroll_builder
      .Begin(1, PlatformRendererType::kScroll, 0.f, 0.f, 100.f, 100.f)
      .End();
  page_scroll_renderer->UpdateDisplayList(page_scroll_builder.Build());
  root_renderer->AddChild(page_scroll_renderer);

  auto overlay_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      2, PlatformRendererType::kExtended, base::String("overlay"));
  DisplayListBuilder overlay_builder;
  overlay_builder
      .Begin(2, PlatformRendererType::kExtended, 0.f, 0.f, 100.f, 100.f)
      .Begin(3, PlatformRendererType::kScroll, 0.f, 0.f, 100.f, 100.f)
      .End()
      .End();
  overlay_renderer->UpdateDisplayList(overlay_builder.Build());
  root_renderer->AddChild(overlay_renderer);

  TestNativePaintingCtxPlatformRef platform_ref;
  platform_ref.renderers_.insert_or_assign(kRootId, root_renderer);
  platform_ref.renderers_.insert_or_assign(1, page_scroll_renderer);
  platform_ref.renderers_.insert_or_assign(2, overlay_renderer);
  platform_ref.scrollable_signs.insert(1);
  platform_ref.scrollable_signs.insert(3);
  platform_ref.MarkAllEventTargetRootsDirtyForTest();

  platform_ref.SetPlatformEventRootActive(2, true);
  platform_ref.scroll_offset_query_counts.clear();

  auto root_target = platform_ref.EnsureEventTargetTree(kRootId);
  ASSERT_NE(root_target, nullptr);
  EXPECT_EQ(platform_ref.scroll_offset_query_counts[1], 1);
  EXPECT_EQ(platform_ref.scroll_offset_query_counts[3], 0);
}

TEST_F(FragmentTest, DirtyOverlayRootOnlyReconstructsOverlayRoot) {
  auto root_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      kRootId, PlatformRendererType::kPage);
  DisplayListBuilder root_builder;
  root_builder
      .Begin(kRootId, PlatformRendererType::kPage, 0.f, 0.f, 100.f, 100.f)
      .DrawView(1)
      .DrawView(2)
      .End();
  root_renderer->UpdateDisplayList(root_builder.Build());

  auto page_scroll_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      1, PlatformRendererType::kScroll);
  DisplayListBuilder page_scroll_builder;
  page_scroll_builder
      .Begin(1, PlatformRendererType::kScroll, 0.f, 0.f, 100.f, 100.f)
      .End();
  page_scroll_renderer->UpdateDisplayList(page_scroll_builder.Build());
  root_renderer->AddChild(page_scroll_renderer);

  auto overlay_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      2, PlatformRendererType::kExtended, base::String("overlay"));
  DisplayListBuilder overlay_builder;
  overlay_builder
      .Begin(2, PlatformRendererType::kExtended, 0.f, 0.f, 100.f, 100.f)
      .Begin(3, PlatformRendererType::kScroll, 0.f, 0.f, 100.f, 100.f)
      .End()
      .End();
  overlay_renderer->UpdateDisplayList(overlay_builder.Build());
  root_renderer->AddChild(overlay_renderer);

  TestNativePaintingCtxPlatformRef platform_ref;
  platform_ref.renderers_.insert_or_assign(kRootId, root_renderer);
  platform_ref.renderers_.insert_or_assign(1, page_scroll_renderer);
  platform_ref.renderers_.insert_or_assign(2, overlay_renderer);
  platform_ref.scrollable_signs.insert(1);
  platform_ref.scrollable_signs.insert(3);
  platform_ref.MarkAllEventTargetRootsDirtyForTest();
  platform_ref.SetPlatformEventRootActive(2, true);
  platform_ref.scroll_offset_query_counts.clear();

  DisplayListBuilder dirty_overlay_builder;
  dirty_overlay_builder
      .Begin(2, PlatformRendererType::kExtended, 0.f, 0.f, 100.f, 100.f)
      .Begin(3, PlatformRendererType::kScroll, 0.f, 0.f, 100.f, 100.f)
      .End()
      .Begin(4, PlatformRendererType::kView, 0.f, 0.f, 10.f, 10.f)
      .End()
      .End();
  platform_ref.UpdateDisplayList(2, dirty_overlay_builder.Build());

  auto overlay_target = platform_ref.EnsureEventTargetTree(2);
  ASSERT_NE(overlay_target, nullptr);
  EXPECT_EQ(platform_ref.scroll_offset_query_counts[1], 0);
  EXPECT_EQ(platform_ref.scroll_offset_query_counts[3], 1);
  EXPECT_NE(platform_ref.GetEventTargetHelper()->GetEventTarget(4), nullptr);
}

TEST_F(FragmentTest, OverlayRootConvertsToPageRootCoordinates) {
  auto root_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      kRootId, PlatformRendererType::kPage);
  DisplayListBuilder root_builder;
  root_builder
      .Begin(kRootId, PlatformRendererType::kPage, 0.f, 0.f, 200.f, 200.f)
      .DrawView(1)
      .End();
  root_renderer->UpdateDisplayList(root_builder.Build());

  auto overlay_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      1, PlatformRendererType::kExtended, base::String("overlay"));
  DisplayListBuilder overlay_builder;
  overlay_builder
      .Begin(1, PlatformRendererType::kExtended, 0.f, 0.f, 100.f, 100.f)
      .Begin(2, PlatformRendererType::kView, 5.f, 7.f, 40.f, 30.f)
      .End()
      .End();
  overlay_renderer->UpdateDisplayList(overlay_builder.Build());
  root_renderer->AddChild(overlay_renderer);

  TestNativePaintingCtxPlatformRef platform_ref;
  platform_ref.renderers_.insert_or_assign(kRootId, root_renderer);
  platform_ref.renderers_.insert_or_assign(1, overlay_renderer);
  platform_ref.root_view_location_on_screen = {100.f, 200.f};
  platform_ref.MarkAllEventTargetRootsDirtyForTest();
  platform_ref.SetPlatformEventRootOffset(1, 20.f, 30.f);
  platform_ref.SetPlatformEventRootActive(1, true);

  auto overlay_target = platform_ref.EnsureEventTargetTree(1);
  ASSERT_NE(overlay_target, nullptr);
  auto child_target = platform_ref.GetEventTargetHelper()->GetEventTarget(2);
  ASSERT_NE(child_target, nullptr);

  float root_point[2] = {6.f, 8.f};
  float target_point[2] = {0.f, 0.f};
  platform_ref.GetEventTargetHelper()->ConvertPointFromAncestorToDescendant(
      target_point, overlay_target, child_target, root_point);
  EXPECT_FLOAT_EQ(target_point[0], 1.f);
  EXPECT_FLOAT_EQ(target_point[1], 1.f);

  float page_point[2] = {root_point[0], root_point[1]};
  platform_ref.GetEventTargetHelper()->ConvertPointFromTargetToPageRootTarget(
      page_point, overlay_target, page_point);
  EXPECT_FLOAT_EQ(page_point[0], 26.f);
  EXPECT_FLOAT_EQ(page_point[1], 38.f);

  float client_point[2] = {root_point[0], root_point[1]};
  platform_ref.GetEventTargetHelper()->ConvertPointFromTargetToScreen(
      client_point, overlay_target, client_point);
  EXPECT_FLOAT_EQ(client_point[0], 126.f);
  EXPECT_FLOAT_EQ(client_point[1], 238.f);

  int32_t callback_code = LynxGetUIResult::UNKNOWN;
  lepus::Value callback_data;
  platform_ref.GetEventTargetHelper()->InvokeMethod(
      2, "boundingClientRect", lepus::Value(),
      [&callback_code, &callback_data](int32_t code, const lepus::Value& data) {
        callback_code = code;
        callback_data = data;
      });
  EXPECT_EQ(callback_code, LynxGetUIResult::SUCCESS);
  EXPECT_FLOAT_EQ(GetProperty(callback_data, "left").Number(), 25.f);
  EXPECT_FLOAT_EQ(GetProperty(callback_data, "top").Number(), 37.f);
  EXPECT_FLOAT_EQ(GetProperty(callback_data, "right").Number(), 65.f);
  EXPECT_FLOAT_EQ(GetProperty(callback_data, "bottom").Number(), 67.f);
}

TEST_F(FragmentTest, MeaningfulPaintingAreaUsesPageRootCoordinatesForOverlay) {
  auto root_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      kRootId, PlatformRendererType::kPage);
  DisplayListBuilder root_builder;
  root_builder
      .Begin(kRootId, PlatformRendererType::kPage, 0.f, 0.f, 200.f, 200.f)
      .DrawView(1)
      .End();
  root_renderer->UpdateDisplayList(root_builder.Build());

  auto overlay_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      1, PlatformRendererType::kExtended, base::String("overlay"));
  DisplayListBuilder overlay_builder;
  overlay_builder
      .Begin(1, PlatformRendererType::kExtended, 0.f, 0.f, 100.f, 100.f)
      .Begin(2, PlatformRendererType::kText, 5.f, 7.f, 40.f, 30.f)
      .End()
      .End();
  overlay_renderer->UpdateDisplayList(overlay_builder.Build());
  root_renderer->AddChild(overlay_renderer);

  TestNativePaintingCtxPlatformRef platform_ref;
  platform_ref.renderers_.insert_or_assign(kRootId, root_renderer);
  platform_ref.renderers_.insert_or_assign(1, overlay_renderer);
  platform_ref.MarkAllEventTargetRootsDirtyForTest();
  platform_ref.SetPlatformEventRootOffset(1, 20.f, 30.f);
  platform_ref.SetPlatformEventRootActive(1, true);

  platform_ref.GetEventTargetHelper()->event_target_trees_.erase(1);
  auto records = platform_ref.CollectMeaningfulPaintingAreaRecords();

  bool found_overlay_child = false;
  for (size_t i = 0; i + 5 < records.size(); i += 6) {
    if (records[i] != 2) {
      continue;
    }
    found_overlay_child = true;
    EXPECT_EQ(records[i + 1],
              static_cast<int32_t>(PlatformRendererType::kText));
    EXPECT_EQ(records[i + 2], 25);
    EXPECT_EQ(records[i + 3], 37);
    EXPECT_EQ(records[i + 4], 40);
    EXPECT_EQ(records[i + 5], 30);
  }
  EXPECT_TRUE(found_overlay_child);
}

TEST_F(FragmentTest, ExposureCheckCachesScreenSizeUntilInvalidated) {
  auto root_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      kRootId, PlatformRendererType::kPage);
  DisplayListBuilder root_builder;
  root_builder
      .Begin(kRootId, PlatformRendererType::kPage, 0.f, 0.f, 200.f, 200.f)
      .Begin(1, PlatformRendererType::kView, 0.f, 0.f, 50.f, 50.f)
      .End()
      .End();
  root_renderer->UpdateDisplayList(root_builder.Build());

  TestNativePaintingCtxPlatformRef platform_ref;
  platform_ref.renderers_.insert_or_assign(kRootId, root_renderer);
  platform_ref.screen_size = {200.f, 200.f};
  platform_ref.MarkAllEventTargetRootsDirtyForTest();

  auto root_target = platform_ref.EnsureEventTargetTree(kRootId);
  ASSERT_NE(root_target, nullptr);
  auto target = platform_ref.GetEventTargetHelper()->GetEventTarget(1);
  ASSERT_NE(target, nullptr);

  BASE_STATIC_STRING_DECL(kUniqueId, "unique-id");
  BASE_STATIC_STRING_DECL(kIsCustomEvent, "is-custom-event");
  BASE_STATIC_STRING_DECL(kIsGlobalEvent, "is-global-event");
  BASE_STATIC_STRING_DECL(kInterceptGlobalEvent, "intercept-global-event");
  auto option = lepus::Dictionary::Create();
  option->SetValue(kUniqueId, "target-1");
  option->SetValue(kIsCustomEvent, true);
  option->SetValue(kIsGlobalEvent, false);
  option->SetValue(kInterceptGlobalEvent, false);
  platform_ref.AddPlatformEventTargetToExposure(target, lepus::Value(option));
  platform_ref.screen_size_query_count = 0;

  platform_ref.event_target_exposure_->DoExposureCheck();
  platform_ref.event_target_exposure_->DoExposureCheck();
  EXPECT_EQ(platform_ref.screen_size_query_count, 1);

  platform_ref.SetPlatformEventRootOffset(1, 10.f, 10.f);
  platform_ref.event_target_exposure_->DoExposureCheck();
  EXPECT_EQ(platform_ref.screen_size_query_count, 1);

  platform_ref.event_target_exposure_->InvalidateWindowRect();
  platform_ref.event_target_exposure_->DoExposureCheck();
  EXPECT_EQ(platform_ref.screen_size_query_count, 2);
}

TEST_F(FragmentTest, ExposureCheckRefreshesEachEventTreeOnce) {
  auto root_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      kRootId, PlatformRendererType::kPage);
  DisplayListBuilder root_builder;
  root_builder
      .Begin(kRootId, PlatformRendererType::kPage, 0.f, 0.f, 200.f, 200.f)
      .DrawView(1)
      .DrawView(2)
      .End();
  root_renderer->UpdateDisplayList(root_builder.Build());

  auto page_scroll_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      1, PlatformRendererType::kScroll);
  DisplayListBuilder page_scroll_builder;
  page_scroll_builder
      .Begin(1, PlatformRendererType::kScroll, 0.f, 0.f, 100.f, 100.f)
      .End();
  page_scroll_renderer->UpdateDisplayList(page_scroll_builder.Build());
  root_renderer->AddChild(page_scroll_renderer);

  auto overlay_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      2, PlatformRendererType::kExtended, base::String("overlay"));
  DisplayListBuilder overlay_builder;
  overlay_builder
      .Begin(2, PlatformRendererType::kExtended, 0.f, 0.f, 100.f, 100.f)
      .Begin(3, PlatformRendererType::kScroll, 0.f, 0.f, 100.f, 100.f)
      .End()
      .End();
  overlay_renderer->UpdateDisplayList(overlay_builder.Build());
  root_renderer->AddChild(overlay_renderer);

  TestNativePaintingCtxPlatformRef platform_ref;
  platform_ref.renderers_.insert_or_assign(kRootId, root_renderer);
  platform_ref.renderers_.insert_or_assign(1, page_scroll_renderer);
  platform_ref.renderers_.insert_or_assign(2, overlay_renderer);
  platform_ref.scrollable_signs.insert(1);
  platform_ref.scrollable_signs.insert(3);
  platform_ref.screen_size = {200.f, 200.f};
  base::Vector<PlatformEventName> event_names;
  event_names.push_back(PlatformEventName::kUIAppear);
  platform_ref.UpdatePlatformEventBundle(
      3, PlatformEventBundle(PlatformEventPropMap(), std::move(event_names)));
  platform_ref.MarkAllEventTargetRootsDirtyForTest();
  platform_ref.SetPlatformEventRootActive(2, true);
  platform_ref.scroll_offset_query_counts.clear();

  platform_ref.event_target_exposure_->DoExposureCheck();

  EXPECT_EQ(platform_ref.scroll_offset_query_counts[1], 1);
  EXPECT_EQ(platform_ref.scroll_offset_query_counts[3], 1);
}

TEST_F(FragmentTest, ExposureRebuildRefreshesVisibleTargetRef) {
  auto root_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      kRootId, PlatformRendererType::kPage);
  DisplayListBuilder root_builder;
  root_builder
      .Begin(kRootId, PlatformRendererType::kPage, 0.f, 0.f, 200.f, 200.f)
      .Begin(1, PlatformRendererType::kView, 0.f, 0.f, 50.f, 50.f)
      .End()
      .End();
  root_renderer->UpdateDisplayList(root_builder.Build());

  TestNativePaintingCtxPlatformRef platform_ref;
  platform_ref.renderers_.insert_or_assign(kRootId, root_renderer);
  platform_ref.MarkAllEventTargetRootsDirtyForTest();
  base::Vector<PlatformEventName> event_names;
  event_names.push_back(PlatformEventName::kUIAppear);
  platform_ref.UpdatePlatformEventBundle(
      1, PlatformEventBundle(PlatformEventPropMap(), std::move(event_names)));

  ASSERT_NE(platform_ref.EnsureEventTargetTree(kRootId), nullptr);
  auto old_target = platform_ref.GetEventTargetHelper()->GetEventTarget(1);
  ASSERT_NE(old_target, nullptr);

  auto detail_it =
      platform_ref.event_target_exposure_->exposure_target_map_.find("1__");
  ASSERT_NE(detail_it,
            platform_ref.event_target_exposure_->exposure_target_map_.end());
  platform_ref.event_target_exposure_->visible_target_before_.insert(
      detail_it->second);

  DisplayListBuilder updated_root_builder;
  updated_root_builder
      .Begin(kRootId, PlatformRendererType::kPage, 0.f, 0.f, 200.f, 200.f)
      .Begin(1, PlatformRendererType::kView, 0.f, 0.f, 60.f, 60.f)
      .End()
      .End();
  platform_ref.UpdateDisplayList(kRootId, updated_root_builder.Build());
  ASSERT_NE(platform_ref.EnsureEventTargetTree(kRootId), nullptr);

  auto new_target = platform_ref.GetEventTargetHelper()->GetEventTarget(1);
  ASSERT_NE(new_target, nullptr);
  EXPECT_NE(old_target.get(), new_target.get());
  ASSERT_EQ(platform_ref.event_target_exposure_->visible_target_before_.size(),
            1u);
  const auto& rebuilt_visible_detail =
      *platform_ref.event_target_exposure_->visible_target_before_.begin();
  EXPECT_EQ(rebuilt_visible_detail.Target().get(), new_target.get());

  platform_ref.event_target_exposure_->ClearExposureTargetMap();
  platform_ref.event_target_exposure_->DidRebuildExposureTargetMap();
  ASSERT_EQ(platform_ref.event_target_exposure_->visible_target_before_.size(),
            1u);
  const auto& stale_visible_detail =
      *platform_ref.event_target_exposure_->visible_target_before_.begin();
  EXPECT_EQ(stale_visible_detail.UniqueId(), "1__");
  EXPECT_EQ(stale_visible_detail.Target().get(), nullptr);
}

TEST_F(FragmentTest, PlatformEventTargetEventThroughAndActiveRegions) {
  auto root_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      kRootId, PlatformRendererType::kPage);
  DisplayListBuilder root_builder;
  root_builder
      .Begin(kRootId, PlatformRendererType::kPage, 0.f, 0.f, 100.f, 100.f)
      .Begin(1, PlatformRendererType::kView, 10.f, 10.f, 80.f, 80.f)
      .Begin(2, PlatformRendererType::kView, 10.f, 10.f, 40.f, 40.f)
      .End()
      .End()
      .End();
  root_renderer->UpdateDisplayList(root_builder.Build());

  PlatformEventPropMap parent_props;
  parent_props.insert_or_assign(PlatformEventPropName::kEventThrough,
                                lepus::Value(true));
  PlatformEventPropMap child_props;
  child_props.insert_or_assign(
      PlatformEventPropName::kEventThroughActiveRegions,
      CreateEventThroughActiveRegionsValue({"0px", "0px", "40px", "20px"}));

  TestNativePaintingCtxPlatformRef platform_ref;
  platform_ref.renderers_.insert_or_assign(kRootId, root_renderer);
  platform_ref.UpdatePlatformEventBundle(
      1, PlatformEventBundle(std::move(parent_props),
                             base::Vector<PlatformEventName>()));
  platform_ref.UpdatePlatformEventBundle(
      2, PlatformEventBundle(std::move(child_props),
                             base::Vector<PlatformEventName>()));
  platform_ref.MarkAllEventTargetRootsDirtyForTest();

  auto root_target = platform_ref.EnsureEventTargetTree(kRootId);
  ASSERT_NE(root_target, nullptr);

  float root_point[2] = {25.f, 25.f};
  auto hit_target = root_target->HitTest(root_point);
  ASSERT_NE(hit_target, nullptr);
  EXPECT_EQ(hit_target->Sign(), 2);

  float hit_region_point[2] = {5.f, 5.f};
  EXPECT_TRUE(hit_target->EventThrough(hit_region_point));

  float outside_region_point[2] = {5.f, 30.f};
  EXPECT_FALSE(hit_target->EventThrough(outside_region_point));

  int int_event_data[] = {0, 0, 0, 1};
  float hit_region_event_data[] = {0.f, 25.f, 25.f};
  EXPECT_FALSE(platform_ref.DispatchPlatformInputEvent(
      int_event_data, hit_region_event_data, kRootId));

  float outside_region_event_data[] = {0.f, 25.f, 50.f};
  EXPECT_TRUE(platform_ref.DispatchPlatformInputEvent(
      int_event_data, outside_region_event_data, kRootId));
}

TEST_F(FragmentTest, PlatformEventTargetMovePastTapSlopCannotRespondTap) {
  auto root_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      kRootId, PlatformRendererType::kPage);
  DisplayListBuilder root_builder;
  root_builder
      .Begin(kRootId, PlatformRendererType::kPage, 0.f, 0.f, 100.f, 100.f)
      .Begin(1, PlatformRendererType::kView, 0.f, 0.f, 100.f, 100.f)
      .End()
      .End();
  root_renderer->UpdateDisplayList(root_builder.Build());

  base::Vector<PlatformEventName> event_names;
  event_names.push_back(PlatformEventName::kClick);

  TestNativePaintingCtxPlatformRef platform_ref;
  platform_ref.renderers_.insert_or_assign(kRootId, root_renderer);
  platform_ref.UpdatePlatformEventBundle(
      1, PlatformEventBundle(PlatformEventPropMap(), std::move(event_names)));
  platform_ref.MarkAllEventTargetRootsDirtyForTest();

  auto root_target = platform_ref.EnsureEventTargetTree(kRootId);
  ASSERT_NE(root_target, nullptr);

  int down_event_data[] = {0, 0, 0, 1};
  float down_pointer_data[] = {0.f, 10.f, 10.f};
  EXPECT_TRUE(platform_ref.DispatchPlatformInputEvent(
      down_event_data, down_pointer_data, kRootId));
  EXPECT_FALSE(platform_ref.event_handler_->first_pointer_moved_);

  int move_event_data[] = {0, 2, 0, 1};
  float move_pointer_data[] = {0.f, 30.f, 10.f};
  EXPECT_TRUE(platform_ref.DispatchPlatformInputEvent(
      move_event_data, move_pointer_data, kRootId));

  EXPECT_TRUE(platform_ref.event_handler_->first_pointer_moved_);
  EXPECT_FALSE(platform_ref.event_handler_->CanRespondTap(
      platform_ref.event_handler_->first_target_));
}

TEST_F(FragmentTest, PlatformEventTargetActiveRegionsRejectNumberValues) {
  auto root_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      kRootId, PlatformRendererType::kPage);
  DisplayListBuilder root_builder;
  root_builder
      .Begin(kRootId, PlatformRendererType::kPage, 0.f, 0.f, 100.f, 100.f)
      .Begin(1, PlatformRendererType::kView, 0.f, 0.f, 100.f, 100.f)
      .End()
      .End();
  root_renderer->UpdateDisplayList(root_builder.Build());

  PlatformEventPropMap child_props;
  child_props.insert_or_assign(PlatformEventPropName::kEventThrough,
                               lepus::Value(true));
  child_props.insert_or_assign(
      PlatformEventPropName::kEventThroughActiveRegions,
      CreateNumericEventThroughActiveRegionsValue({0, 0, 50, 50}));

  TestNativePaintingCtxPlatformRef platform_ref;
  platform_ref.renderers_.insert_or_assign(kRootId, root_renderer);
  platform_ref.UpdatePlatformEventBundle(
      1, PlatformEventBundle(std::move(child_props),
                             base::Vector<PlatformEventName>()));
  platform_ref.MarkAllEventTargetRootsDirtyForTest();

  auto root_target = platform_ref.EnsureEventTargetTree(kRootId);
  ASSERT_NE(root_target, nullptr);

  float point[2] = {75.f, 75.f};
  auto hit_target = root_target->HitTest(point);
  ASSERT_NE(hit_target, nullptr);
  EXPECT_EQ(hit_target->Sign(), 1);
  EXPECT_TRUE(hit_target->EventThrough(point));
}

TEST_F(FragmentTest, PlatformEventTargetEventsPassThroughOnlyAffectsSelf) {
  auto root_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      kRootId, PlatformRendererType::kPage);
  DisplayListBuilder root_builder;
  root_builder
      .Begin(kRootId, PlatformRendererType::kPage, 0.f, 0.f, 100.f, 100.f)
      .DrawView(1)
      .End();
  root_renderer->UpdateDisplayList(root_builder.Build());

  auto overlay_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      1, PlatformRendererType::kExtended, base::String("overlay"));
  DisplayListBuilder overlay_builder;
  overlay_builder
      .Begin(1, PlatformRendererType::kExtended, 0.f, 0.f, 100.f, 100.f)
      .Begin(2, PlatformRendererType::kView, 0.f, 0.f, 50.f, 50.f)
      .End()
      .End();
  overlay_renderer->UpdateDisplayList(overlay_builder.Build());
  root_renderer->AddChild(overlay_renderer);

  PlatformEventPropMap overlay_props;
  overlay_props.insert_or_assign(PlatformEventPropName::kEventsPassThrough,
                                 lepus::Value(true));

  TestNativePaintingCtxPlatformRef platform_ref;
  platform_ref.renderers_.insert_or_assign(kRootId, root_renderer);
  platform_ref.renderers_.insert_or_assign(1, overlay_renderer);
  platform_ref.UpdatePlatformEventBundle(
      1, PlatformEventBundle(std::move(overlay_props),
                             base::Vector<PlatformEventName>()));
  platform_ref.MarkAllEventTargetRootsDirtyForTest();

  platform_ref.SetPlatformEventRootActive(1, true);
  auto overlay_target = platform_ref.EnsureEventTargetTree(1);
  ASSERT_NE(overlay_target, nullptr);

  float root_area_point[2] = {75.f, 75.f};
  auto hit_target = overlay_target->HitTest(root_area_point);
  ASSERT_NE(hit_target, nullptr);
  EXPECT_EQ(hit_target->Sign(), 1);
  EXPECT_TRUE(hit_target->EventThrough(root_area_point));
  int int_event_data[] = {0, 0, 0, 1};
  float root_area_pointer_data[] = {0.f, root_area_point[0],
                                    root_area_point[1]};
  EXPECT_FALSE(platform_ref.DispatchPlatformInputEvent(
      int_event_data, root_area_pointer_data, 1));
  EXPECT_EQ(platform_ref.GetPlatformEventHandlerState(),
            PlatformEventHandler::kStateEventThrough);

  float child_area_point[2] = {10.f, 10.f};
  hit_target = overlay_target->HitTest(child_area_point);
  ASSERT_NE(hit_target, nullptr);
  EXPECT_EQ(hit_target->Sign(), 2);
  EXPECT_FALSE(hit_target->EventThrough(child_area_point));
  float child_area_pointer_data[] = {0.f, child_area_point[0],
                                     child_area_point[1]};
  EXPECT_TRUE(platform_ref.DispatchPlatformInputEvent(
      int_event_data, child_area_pointer_data, 1));
  EXPECT_EQ(platform_ref.GetPlatformEventHandlerState(),
            PlatformEventHandler::kStateNone);
}

TEST_F(FragmentTest, EventThroughStopsInheritingAtPageRoot) {
  auto page_root = fml::MakeRefCounted<PlatformEventTarget>(
      nullptr, kRootId, kRootId, 0.f, 0.f, 100.f, 100.f);
  auto child = fml::MakeRefCounted<PlatformEventTarget>(nullptr, kRootId, 1,
                                                        0.f, 0.f, 20.f, 20.f);
  page_root->SetEventThrough(LynxEventPropStatus::kEnable);
  page_root->AddChildTarget(child);

  float point[2] = {10.f, 10.f};
  EXPECT_FALSE(child->EventThrough(point));
}

TEST_F(FragmentTest, EventThroughInheritsFromIndependentEventRoot) {
  constexpr int32_t kOverlayRootId = 42;
  auto overlay_root = fml::MakeRefCounted<PlatformEventTarget>(
      nullptr, kOverlayRootId, kOverlayRootId, 0.f, 0.f, 100.f, 100.f);
  auto child = fml::MakeRefCounted<PlatformEventTarget>(
      nullptr, kOverlayRootId, 43, 0.f, 0.f, 100.f, 100.f);
  overlay_root->SetEventThrough(LynxEventPropStatus::kEnable);
  overlay_root->AddChildTarget(child);

  float point[2] = {10.f, 10.f};
  EXPECT_TRUE(child->EventThrough(point));
}

TEST_F(FragmentTest, EventThroughActiveRegionsInheritFromIndependentEventRoot) {
  constexpr int32_t kOverlayRootId = 42;
  auto overlay_root = fml::MakeRefCounted<PlatformEventTarget>(
      nullptr, kOverlayRootId, kOverlayRootId, 0.f, 0.f, 100.f, 100.f);
  auto child = fml::MakeRefCounted<PlatformEventTarget>(
      nullptr, kOverlayRootId, 43, 0.f, 0.f, 100.f, 100.f);

  using EventThroughSizeValue = PlatformEventTarget::EventThroughSizeValue;
  PlatformEventTarget::EventThroughRegion region;
  region[0] = {EventThroughSizeValue::Type::kDevicePx, 0.f};
  region[1] = {EventThroughSizeValue::Type::kDevicePx, 0.f};
  region[2] = {EventThroughSizeValue::Type::kDevicePx, 50.f};
  region[3] = {EventThroughSizeValue::Type::kDevicePx, 50.f};
  overlay_root->SetEventThroughActiveRegions({region});
  overlay_root->AddChildTarget(child);

  float point_inside_active_region[2] = {10.f, 10.f};
  EXPECT_FALSE(child->EventThrough(point_inside_active_region));

  float point_outside_active_region[2] = {60.f, 60.f};
  EXPECT_TRUE(child->EventThrough(point_outside_active_region));
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
