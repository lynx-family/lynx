// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/dom/fragment/fragment.h"

#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "base/include/fml/message_loop.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/display_list_reader.h"
#include "core/renderer/dom/fragment/fragment_behavior.h"
#include "core/renderer/dom/fragment/image_fragment_behavior.h"
#include "core/renderer/lynx_env_config.h"
#include "core/renderer/starlight/types/layout_result.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/renderer/ui_wrapper/common/testing/prop_bundle_mock.h"
#include "core/renderer/ui_wrapper/painting/native_painting_context_platform_ref.h"
#include "core/renderer/ui_wrapper/painting/paint_image.h"
#include "core/renderer/ui_wrapper/painting/platform_renderer_impl.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "gfx/geometry/matrix44.h"
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

static std::vector<DisplayListItem> CollectDisplayListItems(
    const DisplayList& list) {
  std::vector<DisplayListItem> items;
  DisplayListReader reader(list);
  while (reader.HasNext()) {
    items.push_back(reader.Next());
  }
  return items;
}

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
      const fml::RefPtr<PropBundle>& attributes,
      const PlatformRendererInitConfig& init_config) override {
    init_config_ = init_config;
    attributes_ = attributes;
  }

  PlatformRendererType GetType() const override {
    return PlatformRendererType::kText;
  }

  PlatformRendererInitConfig init_config_;
  fml::RefPtr<PropBundle> attributes_;
};

class TestPlatformRenderer : public PlatformRendererImpl {
 public:
  TestPlatformRenderer(int id, PlatformRendererType type)
      : PlatformRendererImpl(id, type, base::String()) {}

 protected:
  void OnUpdateDisplayList(DisplayList display_list) override {
    if (display_list.GetContentItemsSize() > 0) {
      display_list_ = std::move(display_list);
    }
  }
  void OnUpdateAttributes(const fml::RefPtr<PropBundle>&, bool) override {}
  void OnAddChild(PlatformRenderer*, int, bool) override {}
  void OnRemoveFromParent(bool) override {}
  void OnUpdateSubtreeProperties(const DisplayList&) override {}
};

class TestPlatformRendererFactory : public PlatformRendererFactory {
 public:
  fml::RefPtr<PlatformRenderer> CreateRenderer(
      int id, PlatformRendererType type, const fml::RefPtr<PropBundle>&,
      const PlatformRendererInitConfig&) override {
    return fml::MakeRefCounted<TestPlatformRenderer>(id, type);
  }

  fml::RefPtr<PlatformRenderer> CreateExtendedRenderer(
      int id, const base::String&, const fml::RefPtr<PropBundle>&,
      const PlatformRendererInitConfig&) override {
    return fml::MakeRefCounted<TestPlatformRenderer>(
        id, PlatformRendererType::kExtended);
  }
};

TEST_F(FragmentTest, FilterResetInvalidatesSubtreeAndEmitsNone) {
  auto element = manager->CreateFiberView();
  auto* fragment = element->element_container()->CastToFragment();
  ASSERT_NE(fragment, nullptr);
  auto* style = element->computed_css_style();

  auto filter_array = lepus::CArray::Create();
  filter_array->emplace_back(
      static_cast<int32_t>(starlight::FilterType::kGrayscale));
  filter_array->emplace_back(40.f);
  filter_array->emplace_back(static_cast<int32_t>(CSSValuePattern::PERCENT));
  element->SetStyleInternal(
      CSSPropertyID::kPropertyIDFilter,
      CSSValue(lepus::Value(filter_array), CSSValuePattern::ARRAY));
  ASSERT_TRUE(element->element_container()->NeedUpdateSubtreeProperty());

  DisplayListBuilder filter_builder;
  fragment->DrawFilter(filter_builder);
  DisplayList filter_list = filter_builder.Build();
  const SubtreeProperty* filter_properties =
      filter_list.GetSubtreePropertiesData();
  ASSERT_NE(filter_properties, nullptr);
  ASSERT_EQ(filter_list.GetSubtreePropertiesSize(), 1u);
  EXPECT_EQ(filter_properties[0].type,
            DisplayListSubtreePropertyOpType::kFilter);
  EXPECT_EQ(filter_properties[0].data.filter.type,
            static_cast<int32_t>(starlight::FilterType::kGrayscale));
  EXPECT_FLOAT_EQ(filter_properties[0].data.filter.amount, 0.4f);

  style->ClearChanged();
  element->element_container()->ClearPaintDirtyState();
  base::Vector<CSSPropertyID> reset_styles = {CSSPropertyID::kPropertyIDFilter};
  element->ResetStyle(reset_styles);
  ASSERT_TRUE(element->element_container()->NeedUpdateSubtreeProperty());
  EXPECT_FALSE(element->element_container()->NeedRedraw());

  DisplayListBuilder reset_builder;
  fragment->DrawFilter(reset_builder);
  DisplayList reset_list = reset_builder.Build();
  const SubtreeProperty* reset_properties =
      reset_list.GetSubtreePropertiesData();
  ASSERT_NE(reset_properties, nullptr);
  ASSERT_EQ(reset_list.GetSubtreePropertiesSize(), 1u);
  EXPECT_EQ(reset_properties[0].type,
            DisplayListSubtreePropertyOpType::kFilter);
  EXPECT_EQ(reset_properties[0].data.filter.type,
            static_cast<int32_t>(starlight::FilterType::kNone));
  EXPECT_FLOAT_EQ(reset_properties[0].data.filter.amount, 0.f);
}

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

  void UseCurrentThreadAsTaskRunnerForTest() {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    event_target_task_runner_ = fml::MessageLoop::GetCurrent().GetTaskRunner();
  }

  std::unordered_map<int32_t, std::array<float, 2>> scroll_offsets;
  std::unordered_set<int32_t> scrollable_signs;
  std::vector<int32_t> destroyed_image_keys;

 protected:
  void DestroyImageOnPlatformThread(int32_t image_key) override {
    destroyed_image_keys.push_back(image_key);
  }
};

// Adapter that exposes the NativePaintingContext interface expected by
// Fragment::Draw() and forwards to a TestNativePaintingCtxPlatformRef.
class TestNativePaintingContext : public NativePaintingContext {
 public:
  struct CreatedImage {
    int id;
    base::String src;
    ImageFitMode mode;
    base::String blur_radius;
    bool auto_size;
    base::String placeholder;
    base::String tint_color;
    base::String cap_insets;
    float cap_insets_scale;
    bool skip_redirection;
    bool autoplay;
    int32_t loop_count;
    float width;
    float height;
    int32_t event_mask;
    bool disable_default_resize;
    int32_t image_key;
  };

  void SetPlatformRef(TestNativePaintingCtxPlatformRef* ref) { ref_ = ref; }

  void OnFirstScreen() override {}
  void FinishTasmOperation(
      const std::shared_ptr<PipelineOptions>& options) override {}
  void FinishLayoutOperation(
      const std::shared_ptr<PipelineOptions>& options) override {
    operations.emplace_back("finish_layout");
  }
  void CreatePlatformRenderer(
      int id, PlatformRendererType type,
      const fml::RefPtr<PropBundle>& init_data,
      const PlatformRendererInitConfig& init_config) override {
    if (ref_) {
      ref_->CreatePlatformRenderer(id, type, init_data, init_config);
    }
  }
  void CreatePlatformExtendedRenderer(
      int id, const base::String& tag_name,
      const fml::RefPtr<PropBundle>& init_data,
      const PlatformRendererInitConfig& init_config) override {
    if (ref_) {
      ref_->CreatePlatformExtendedRenderer(id, tag_name, init_data,
                                           init_config);
    }
  }
  void UpdateDisplayList(int id, DisplayList list) override {
    operations.emplace_back("update_display_list");
    if (ref_) {
      ref_->UpdateDisplayList(id, std::move(list));
    }
  }
  fml::RefPtr<PaintImage> CreateImage(
      int id, base::String src, const ImagePaintInfo& paint_info, float width,
      float height, int32_t event_mask = 0,
      bool disable_default_resize = false) override {
    if (fail_image_creation_) {
      return nullptr;
    }
    int32_t image_key = next_image_key_++;
    created_images_.push_back(
        {id, src, paint_info.mode, paint_info.blur_radius, paint_info.auto_size,
         paint_info.placeholder, paint_info.tint_color, paint_info.cap_insets,
         paint_info.cap_insets_scale, paint_info.skip_redirection,
         paint_info.autoplay, paint_info.loop_count, width, height, event_mask,
         disable_default_resize, image_key});
    return fml::MakeRefCounted<PaintImage>(image_key);
  }
  void UpdateTextBundle(int id, intptr_t bundle) override {}
  void DestroyTextBundle(int id) override {}
  void ReconstructEventTargetTreeRecursively() override {
    if (ref_) {
      ref_->ReconstructEventTargetTreeRecursively();
    }
  }
  void UpdatePlatformEventBundle(int id, PlatformEventBundle bundle) override {
    if (ref_) {
      ref_->UpdatePlatformEventBundle(id, std::move(bundle));
    }
  }

  std::vector<std::string> operations;
  std::vector<CreatedImage> created_images_;
  bool fail_image_creation_{false};

 private:
  TestNativePaintingCtxPlatformRef* ref_ = nullptr;
  int32_t next_image_key_{1000};
};

// A MockPaintingContext whose platform ref is a real
// NativePaintingCtxPlatformRef so that Fragment::Draw() can exercise
// UpdateDisplayList / event-target reconstruction paths.
class NativeMockPaintingContext : public MockPaintingContext,
                                  public TestNativePaintingContext {
 public:
  NativeMockPaintingContext() {
    auto ref = std::make_shared<TestNativePaintingCtxPlatformRef>();
    platform_ref_ = ref;
    SetPlatformRef(ref.get());
  }

  NativePaintingContext* CastToNativeCtx() override { return this; }

  TestNativePaintingCtxPlatformRef* GetNativePlatformRef() {
    return static_cast<TestNativePaintingCtxPlatformRef*>(platform_ref_.get());
  }
};

TEST(NativePaintingCtxPlatformRefTest,
     ScheduleDestroyImageUsesWeakRefAndNoOpsAfterDestroy) {
  auto ref = std::make_shared<TestNativePaintingCtxPlatformRef>();
  ref->UseCurrentThreadAsTaskRunnerForTest();

  ref->ScheduleDestroyImage(11);
  EXPECT_THAT(ref->destroyed_image_keys, ::testing::ElementsAre(11));

  ref->Destroy();
  ref->ScheduleDestroyImage(12);
  ref->Destroy();
  EXPECT_THAT(ref->destroyed_image_keys, ::testing::ElementsAre(11));
}

class FragmentDrawTest : public ::testing::Test {
 public:
  FragmentDrawTest() {}
  ~FragmentDrawTest() override {}

  void SetUp() override {
    LynxEnvConfig lynx_env_config(kConfigWidth, kConfigHeight,
                                  kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    manager = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<NativeMockPaintingContext>(), tasm_mediator.get(),
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

TEST_F(FragmentDrawTest, DrawViewRecordsFinalOffsetWithRenderOffset) {
  auto element = manager->CreateFiberView();
  Fragment fragment(element.get());
  fragment.has_platform_renderer_ = true;

  starlight::LayoutResultForRendering layout;
  layout.offset_ = starlight::FloatPoint(5.f, 6.f);
  layout.size_ = FloatSize(30.f, 40.f);
  fragment.UpdateLayout(layout);
  fragment.render_offset_[0] = 10.f;
  fragment.render_offset_[1] = 20.f;

  DisplayListBuilder builder;
  fragment.Draw(builder);

  DisplayList display_list = builder.Build();
  DisplayListReader reader(display_list);
  ASSERT_TRUE(reader.HasNext());
  const auto& view_item = reader.Next();
  EXPECT_EQ(view_item.type, DisplayListOpType::kDrawView);
  EXPECT_EQ(view_item.payload.draw_view.view_id, fragment.id());
  EXPECT_FLOAT_EQ(view_item.payload.draw_view.offset_x, 15.f);
  EXPECT_FLOAT_EQ(view_item.payload.draw_view.offset_y, 26.f);
  EXPECT_FALSE(reader.HasNext());
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
  EXPECT_EQ(behavior_ptr->init_config_.fragment_parent_id, -1);
  EXPECT_TRUE(
      behavior_ptr->init_config_.is_direct_child_of_compatible_component);
}

TEST_F(FragmentTest, UpdatePaintingNodeUsesCurrentFlattenStateForLayer) {
  auto parent_element = manager->CreateFiberPage("0", 0);
  Fragment parent_fragment(parent_element.get());

  auto element = manager->CreateFiberView();
  Fragment fragment(element.get());
  parent_fragment.AddChildBefore(&fragment, nullptr);
  auto behavior = std::make_unique<RecordingFragmentBehavior>(&fragment);
  auto* behavior_ptr = behavior.get();
  fragment.SetBehavior(std::move(behavior));

  ASSERT_TRUE(element->TendToFlatten());
  ASSERT_FALSE(fragment.CreateLayerIfNeeded(nullptr));
  ASSERT_FALSE(fragment.has_platform_renderer_);

  parent_fragment.ResetDirtyState(BaseElementContainer::kNeedRedraw);
  fragment.ResetDirtyState(BaseElementContainer::kNeedRedraw);
  element->has_non_flatten_attrs_ = true;

  auto painting_data = PropBundleMock::CreateForMock();
  painting_data->SetProps(
      CSSProperty::GetPropertyNameCStr(CSSPropertyID::kPropertyIDTransform),
      "translateX(1px)");
  fragment.UpdatePaintingNode(true, painting_data);

  EXPECT_TRUE(fragment.has_platform_renderer_);
  EXPECT_TRUE(parent_fragment.NeedRedraw());
  EXPECT_TRUE(fragment.NeedRedraw());
  ASSERT_TRUE(behavior_ptr->attributes_);
  EXPECT_TRUE(behavior_ptr->attributes_->Contains(
      CSSProperty::GetPropertyNameCStr(CSSPropertyID::kPropertyIDTransform)));
  auto* props = static_cast<PropBundleMock*>(behavior_ptr->attributes_.get());
  EXPECT_FALSE(props->GetPropsMap().at(kTendsToFlattenInitDataKey).Bool());
}

TEST_F(FragmentTest, DrawFullSyncsOverflowToBeginOperation) {
  auto element = manager->CreateFiberView();
  element->set_is_layout_only(true);
  Fragment fragment(element.get());
  fragment.SetBehavior(std::make_unique<RecordingFragmentBehavior>(&fragment));

  starlight::LayoutResultForRendering layout;
  layout.size_ = FloatSize(100.f, 100.f);
  fragment.UpdateLayout(layout);
  element->computed_css_style()->origin_overflow_ =
      starlight::ComputedCSSStyle::OVERFLOW_X;

  DisplayListBuilder builder;
  fragment.DrawFull(builder);
  DisplayList display_list = builder.Build();
  DisplayListReader reader(display_list);

  ASSERT_TRUE(reader.HasNext());
  const auto& begin = reader.Next();
  ASSERT_EQ(begin.type, DisplayListOpType::kBegin);
  EXPECT_EQ(begin.payload.begin.overflow_x, 1);
  EXPECT_EQ(begin.payload.begin.overflow_y, 0);
  EXPECT_EQ(begin.payload.begin.is_layout_only, 0);
}

TEST_F(FragmentTest, DrawFullSyncsLayoutOnlyWhenPageConfigEnabled) {
  auto element = manager->CreateFiberView();
  element->set_is_layout_only(true);
  Fragment fragment(element.get());
  fragment.SetBehavior(std::make_unique<RecordingFragmentBehavior>(&fragment));

  starlight::LayoutResultForRendering layout;
  layout.size_ = FloatSize(100.f, 100.f);
  fragment.UpdateLayout(layout);

  manager->GetConfig()->SetEnableLayoutOnlyEventThrough(true);
  DisplayListBuilder builder;
  fragment.DrawFull(builder);
  DisplayList display_list = builder.Build();
  DisplayListReader reader(display_list);

  ASSERT_TRUE(reader.HasNext());
  const auto& begin = reader.Next();
  ASSERT_EQ(begin.type, DisplayListOpType::kBegin);
  EXPECT_EQ(begin.payload.begin.is_layout_only, 1);
}

TEST_F(FragmentTest, ReusedEventTargetTreeRefreshesScrollOffsetForHitTest) {
  auto root_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      kRootId, PlatformRendererType::kPage);
  DisplayListBuilder root_builder;
  root_builder
      .Begin(kRootId, PlatformRendererType::kPage, 0.f, 0.f, 100.f, 100.f)
      .DrawView(1, 0.f, 0.f)
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

  auto root_target = platform_ref.ReconstructEventTargetTreeRecursively();
  ASSERT_NE(root_target, nullptr);

  float point[2] = {10.f, 40.f};
  auto hit_target = root_target->HitTest(point);
  ASSERT_NE(hit_target, nullptr);
  EXPECT_EQ(hit_target->Sign(), 1);

  platform_ref.scroll_offsets[1] = {0.f, 30.f};
  auto reused_root = platform_ref.ReconstructEventTargetTreeRecursively();

  EXPECT_EQ(root_target.get(), reused_root.get());
  hit_target = reused_root->HitTest(point);
  ASSERT_NE(hit_target, nullptr);
  EXPECT_EQ(hit_target->Sign(), 2);
}

TEST_F(FragmentTest, PlatformEventTargetHitTestAccountsForTransform) {
  auto root_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      kRootId, PlatformRendererType::kPage);
  DisplayListBuilder root_builder;
  root_builder
      .Begin(kRootId, PlatformRendererType::kPage, 0.f, 0.f, 200.f, 100.f)
      .DrawView(1, 0.f, 0.f)
      .End();
  root_renderer->UpdateDisplayList(root_builder.Build());

  auto child_renderer =
      fml::MakeRefCounted<TestPlatformRenderer>(1, PlatformRendererType::kView);
  gfx::Matrix44 transform;
  transform.preTranslate(40.f, 0.f, 0.f);
  DisplayListBuilder child_builder;
  child_builder.Begin(1, PlatformRendererType::kView, 20.f, 0.f, 20.f, 20.f)
      .End()
      .Transform(transform);
  child_renderer->UpdateDisplayList(child_builder.Build());
  root_renderer->AddChild(child_renderer);

  TestNativePaintingCtxPlatformRef platform_ref;
  platform_ref.renderers_.insert_or_assign(kRootId, root_renderer);
  platform_ref.renderers_.insert_or_assign(1, child_renderer);

  auto root_target = platform_ref.ReconstructEventTargetTreeRecursively();
  ASSERT_NE(root_target, nullptr);
  auto child_target = platform_ref.GetEventTargetHelper()->GetEventTarget(1);
  ASSERT_NE(child_target, nullptr);
  ASSERT_NE(child_target->Transform(), nullptr);

  float root_point[2] = {65.f, 5.f};
  auto hit_target = root_target->HitTest(root_point);
  ASSERT_NE(hit_target, nullptr);
  EXPECT_EQ(hit_target->Sign(), 1);

  float child_point[2] = {0.f, 0.f};
  platform_ref.GetEventTargetHelper()->ConvertPointFromAncestorToDescendant(
      child_point, root_target, child_target, root_point);
  EXPECT_FLOAT_EQ(child_point[0], 5.f);
  EXPECT_FLOAT_EQ(child_point[1], 5.f);

  float converted_root_point[2] = {0.f, 0.f};
  platform_ref.GetEventTargetHelper()->ConvertPointFromDescendantToAncestor(
      converted_root_point, child_target, root_target, child_point);
  EXPECT_FLOAT_EQ(converted_root_point[0], root_point[0]);
  EXPECT_FLOAT_EQ(converted_root_point[1], root_point[1]);
}

TEST_F(FragmentTest,
       PlatformEventTargetHitTestDescendsIntoOverflowVisibleChild) {
  auto root_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      kRootId, PlatformRendererType::kPage);
  DisplayListBuilder builder;
  builder.Begin(kRootId, PlatformRendererType::kPage, 0.f, 0.f, 100.f, 100.f)
      .Begin(1, PlatformRendererType::kView, 0.f, 0.f, 20.f, 20.f, true, true)
      .Begin(2, PlatformRendererType::kView, 30.f, 0.f, 10.f, 10.f)
      .End()
      .End()
      .End();
  root_renderer->UpdateDisplayList(builder.Build());

  TestNativePaintingCtxPlatformRef platform_ref;
  platform_ref.renderers_.insert_or_assign(kRootId, root_renderer);

  auto root_target = platform_ref.ReconstructEventTargetTreeRecursively();
  ASSERT_NE(root_target, nullptr);
  auto overflow_target = platform_ref.GetEventTargetHelper()->GetEventTarget(1);
  ASSERT_NE(overflow_target, nullptr);
  EXPECT_TRUE(overflow_target->OverflowX());
  EXPECT_TRUE(overflow_target->OverflowY());

  float point[2] = {35.f, 5.f};
  auto hit_target = root_target->HitTest(point);
  ASSERT_NE(hit_target, nullptr);
  EXPECT_EQ(hit_target->Sign(), 2);
}

TEST_F(FragmentTest,
       PlatformEventTargetHitTestSkipsLayoutOnlyButKeepsDescendants) {
  auto root_renderer = fml::MakeRefCounted<TestPlatformRenderer>(
      kRootId, PlatformRendererType::kPage);
  DisplayListBuilder builder;
  builder.Begin(kRootId, PlatformRendererType::kPage, 0.f, 0.f, 100.f, 100.f)
      .Begin(3, PlatformRendererType::kView, 0.f, 0.f, 100.f, 100.f)
      .End()
      .Begin(1, PlatformRendererType::kView, 0.f, 0.f, 20.f, 20.f, true, true,
             true)
      .Begin(4, PlatformRendererType::kView, 0.f, 0.f, 20.f, 20.f, true, true,
             true)
      .Begin(2, PlatformRendererType::kView, 30.f, 0.f, 10.f, 10.f)
      .End()
      .End()
      .End()
      .End();
  root_renderer->UpdateDisplayList(builder.Build());

  TestNativePaintingCtxPlatformRef platform_ref;
  platform_ref.renderers_.insert_or_assign(kRootId, root_renderer);

  auto root_target = platform_ref.ReconstructEventTargetTreeRecursively();
  ASSERT_NE(root_target, nullptr);
  auto layout_only_target =
      platform_ref.GetEventTargetHelper()->GetEventTarget(1);
  ASSERT_NE(layout_only_target, nullptr);
  EXPECT_TRUE(layout_only_target->IsLayoutOnly());
  auto nested_layout_only_target =
      platform_ref.GetEventTargetHelper()->GetEventTarget(4);
  ASSERT_NE(nested_layout_only_target, nullptr);
  EXPECT_TRUE(nested_layout_only_target->IsLayoutOnly());

  float descendant_point[2] = {35.f, 5.f};
  auto hit_target = root_target->HitTest(descendant_point);
  ASSERT_NE(hit_target, nullptr);
  EXPECT_EQ(hit_target->Sign(), 2);

  float event_through_point[2] = {5.f, 5.f};
  hit_target = root_target->HitTest(event_through_point);
  ASSERT_NE(hit_target, nullptr);
  EXPECT_EQ(hit_target->Sign(), 3);
}

TEST_F(FragmentTest, PlatformEventTargetInheritsEventThroughFromPage) {
  auto root_target = fml::MakeRefCounted<PlatformEventTarget>(
      nullptr, kRootId, kRootId, 0.f, 0.f, 100.f, 100.f);
  auto child_target = fml::MakeRefCounted<PlatformEventTarget>(
      nullptr, kRootId, 1, 0.f, 0.f, 100.f, 100.f);
  root_target->SetEventThrough(LynxEventPropStatus::kEnable);
  root_target->AddChildTarget(child_target);

  float point[2] = {10.f, 10.f};
  EXPECT_FALSE(child_target->EventThrough(point));
  EXPECT_TRUE(child_target->EventThrough(point, true));

  child_target->SetEventThrough(LynxEventPropStatus::kDisable);
  EXPECT_FALSE(child_target->EventThrough(point, true));
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
  auto items = CollectDisplayListItems(list);
  ASSERT_GE(items.size(), 3u);

  EXPECT_EQ(items[0].type, DisplayListOpType::kRecordBox);
  EXPECT_EQ(items[1].type, DisplayListOpType::kRecordBox);
  EXPECT_EQ(items[2].type, DisplayListOpType::kBoxShadow);

  const auto& box_shadow = items[2].payload.box_shadow;
  EXPECT_EQ(box_shadow.shadow_box_index, 1);
  EXPECT_EQ(box_shadow.clip_box_index, 0);
  EXPECT_EQ(box_shadow.color, 0xFF000000u);
  EXPECT_EQ(box_shadow.clip_mode, 0);
  EXPECT_FLOAT_EQ(box_shadow.blur_radius, 5.0f);
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
  auto items = CollectDisplayListItems(list);
  ASSERT_GE(items.size(), 3u);

  EXPECT_EQ(items[0].type, DisplayListOpType::kRecordBox);
  EXPECT_EQ(items[1].type, DisplayListOpType::kRecordBox);
  EXPECT_EQ(items[2].type, DisplayListOpType::kBoxShadow);

  const auto& box_shadow = items[2].payload.box_shadow;
  EXPECT_EQ(box_shadow.shadow_box_index, 1);
  EXPECT_EQ(box_shadow.clip_box_index, 0);
  EXPECT_EQ(box_shadow.color, 0x80FF0000u);
  EXPECT_EQ(box_shadow.clip_mode, 1);
  EXPECT_FLOAT_EQ(box_shadow.blur_radius, 4.0f);
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
  auto items = CollectDisplayListItems(list);
  ASSERT_GE(items.size(), 6u);

  // Shadows are drawn in reverse order (painter's algorithm):
  // shadow2 (inset) first, then shadow1 (outset)
  // For each shadow: RecordBox (clip), RecordBox (shadow), BoxShadow
  EXPECT_EQ(items[0].type, DisplayListOpType::kRecordBox);
  EXPECT_EQ(items[1].type, DisplayListOpType::kRecordBox);
  EXPECT_EQ(items[2].type, DisplayListOpType::kBoxShadow);
  EXPECT_EQ(items[3].type, DisplayListOpType::kRecordBox);
  EXPECT_EQ(items[4].type, DisplayListOpType::kRecordBox);
  EXPECT_EQ(items[5].type, DisplayListOpType::kBoxShadow);
}

TEST_F(FragmentTest, DrawBoxShadowNoShadowData) {
  auto element = manager->CreateFiberView();
  Fragment fragment(element.get());

  DisplayListBuilder builder;
  fragment.DrawBoxShadow(builder);

  DisplayList list = builder.Build();

  EXPECT_EQ(list.GetContentItemsSize(), 0u);
  EXPECT_EQ(list.GetContentDataSize(), 0u);
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
  auto items = CollectDisplayListItems(list);
  ASSERT_EQ(items.size(), 1u);
  EXPECT_EQ(items[0].type, DisplayListOpType::kRecordBox);
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
  auto items = CollectDisplayListItems(list);
  ASSERT_GE(items.size(), 3u);

  EXPECT_EQ(items[0].type, DisplayListOpType::kRecordBox);
  EXPECT_EQ(items[1].type, DisplayListOpType::kRecordBox);
  EXPECT_EQ(items[2].type, DisplayListOpType::kBoxShadow);

  // radius=10, effective outset=20, coverage=2*min(10/40, 10/40)=0.5:
  //   ratio = 10/20 = 0.5
  //   result = 10 + 20 * (1 - 0.5^3 * (1 - 0.5^3))
  //          = 10 + 20 * (1 - 0.125 * 0.875)
  //          = 27.8125
  const float kExpectedRadius = 27.8125f;
  const auto& shadow_box = items[1].payload.record_box;
  for (float radius : shadow_box.radii) {
    EXPECT_FLOAT_EQ(radius, kExpectedRadius);
  }
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
  auto items = CollectDisplayListItems(list);
  ASSERT_EQ(items.size(), 1u);

  EXPECT_EQ(items[0].type, DisplayListOpType::kClipRect);
  const auto& clip_rect = items[0].payload.clip_rect;
  EXPECT_EQ(clip_rect.has_radii, 0u);
  EXPECT_FLOAT_EQ(clip_rect.x, 1.f);
  EXPECT_FLOAT_EQ(clip_rect.y, 3.f);
  EXPECT_FLOAT_EQ(clip_rect.w, 100.f - 1.f - 2.f);
  EXPECT_FLOAT_EQ(clip_rect.h, 60.f - 3.f - 4.f);
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
  auto items = CollectDisplayListItems(list);
  ASSERT_EQ(items.size(), 1u);

  EXPECT_EQ(items[0].type, DisplayListOpType::kClipRect);
  const auto& clip_rect = items[0].payload.clip_rect;
  EXPECT_EQ(clip_rect.has_radii, 1u);
  EXPECT_FLOAT_EQ(clip_rect.x, 1.f);
  EXPECT_FLOAT_EQ(clip_rect.y, 3.f);
  EXPECT_FLOAT_EQ(clip_rect.w, 100.f - 1.f - 2.f);
  EXPECT_FLOAT_EQ(clip_rect.h, 60.f - 3.f - 4.f);

  EXPECT_FLOAT_EQ(clip_rect.radii[0], 10.f - 1.f);
  EXPECT_FLOAT_EQ(clip_rect.radii[1], 12.f - 3.f);
  EXPECT_FLOAT_EQ(clip_rect.radii[2], 14.f - 2.f);
  EXPECT_FLOAT_EQ(clip_rect.radii[3], 16.f - 3.f);
  EXPECT_FLOAT_EQ(clip_rect.radii[4], 18.f - 2.f);
  EXPECT_FLOAT_EQ(clip_rect.radii[5], 20.f - 4.f);
  EXPECT_FLOAT_EQ(clip_rect.radii[6], 22.f - 1.f);
  EXPECT_FLOAT_EQ(clip_rect.radii[7], 24.f - 4.f);
}

TEST_F(FragmentTest, UpdateLayoutNormalizesOversizedBorderRadii) {
  auto element = manager->CreateFiberView();
  Fragment fragment(element.get());

  auto* layout_style = element->computed_css_style()->GetLayoutComputedStyle();
  layout_style->surround_data_.border_data_ = starlight::BordersData();
  auto& border = *layout_style->surround_data_.border_data_;
  const auto radius = starlight::NLength::MakeUnitNLength(9999.f);
  border.radius_x_top_left = radius;
  border.radius_y_top_left = radius;
  border.radius_x_top_right = radius;
  border.radius_y_top_right = radius;
  border.radius_x_bottom_right = radius;
  border.radius_y_bottom_right = radius;
  border.radius_x_bottom_left = radius;
  border.radius_y_bottom_left = radius;

  starlight::LayoutResultForRendering layout;
  layout.size_ = FloatSize(382.f, 44.f);
  fragment.UpdateLayout(layout);

  const auto& radii = *fragment.LayoutResult().border_radius_info;
  EXPECT_FLOAT_EQ(radii.x_top_left, 22.f);
  EXPECT_FLOAT_EQ(radii.y_top_left, 22.f);
  EXPECT_FLOAT_EQ(radii.x_top_right, 22.f);
  EXPECT_FLOAT_EQ(radii.y_top_right, 22.f);
  EXPECT_FLOAT_EQ(radii.x_bottom_right, 22.f);
  EXPECT_FLOAT_EQ(radii.y_bottom_right, 22.f);
  EXPECT_FLOAT_EQ(radii.x_bottom_left, 22.f);
  EXPECT_FLOAT_EQ(radii.y_bottom_left, 22.f);
}

TEST_F(FragmentTest, TestUpdateLayoutAndDefineBoxAndDrawImage) {
  auto element = manager->CreateFiberImage("image");
  element->SetAttributeInternal("src", lepus::Value("image-src://"));
  element->SetAttributeInternal("mode", lepus::Value("aspectFit"));

  Fragment fragment(element.get());
  fragment.SetBehavior(std::make_unique<ImageFragmentBehavior>(&fragment));
  TestNativePaintingContext native_painting_context;
  fragment.behavior_->painting_context_ = &native_painting_context;

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

  fragment.behavior_->OnUpdateLayout(fragment.LayoutResult());

  DisplayListBuilder builder;
  EXPECT_EQ(fragment.DefineBorderBox(builder), 0);
  EXPECT_EQ(fragment.DefinePaddingBox(builder), 1);
  EXPECT_EQ(fragment.DefineContentBox(builder), 2);

  fragment.behavior_->OnDraw(builder);

  DisplayList list = builder.Build();
  auto items = CollectDisplayListItems(list);
  ASSERT_EQ(items.size(), 4u);

  EXPECT_EQ(items[0].type, DisplayListOpType::kRecordBox);
  const auto& border_box = items[0].payload.record_box;
  EXPECT_EQ(border_box.has_radii, 1u);
  EXPECT_FLOAT_EQ(border_box.x, 0.f);
  EXPECT_FLOAT_EQ(border_box.y, 0.f);
  EXPECT_FLOAT_EQ(border_box.w, 100.f);
  EXPECT_FLOAT_EQ(border_box.h, 60.f);
  EXPECT_FLOAT_EQ(border_box.radii[0], 10.f);
  EXPECT_FLOAT_EQ(border_box.radii[1], 12.f);
  EXPECT_FLOAT_EQ(border_box.radii[2], 14.f);
  EXPECT_FLOAT_EQ(border_box.radii[3], 16.f);
  EXPECT_FLOAT_EQ(border_box.radii[4], 18.f);
  EXPECT_FLOAT_EQ(border_box.radii[5], 20.f);
  EXPECT_FLOAT_EQ(border_box.radii[6], 22.f);
  EXPECT_FLOAT_EQ(border_box.radii[7], 24.f);

  EXPECT_EQ(items[1].type, DisplayListOpType::kRecordBox);
  const auto& padding_box = items[1].payload.record_box;
  EXPECT_EQ(padding_box.has_radii, 1u);
  EXPECT_FLOAT_EQ(padding_box.x, 1.f);
  EXPECT_FLOAT_EQ(padding_box.y, 3.f);
  EXPECT_FLOAT_EQ(padding_box.w, 97.f);
  EXPECT_FLOAT_EQ(padding_box.h, 53.f);
  EXPECT_FLOAT_EQ(padding_box.radii[0], 10.f - 1.f);
  EXPECT_FLOAT_EQ(padding_box.radii[1], 12.f - 3.f);
  EXPECT_FLOAT_EQ(padding_box.radii[2], 14.f - 2.f);
  EXPECT_FLOAT_EQ(padding_box.radii[3], 16.f - 3.f);
  EXPECT_FLOAT_EQ(padding_box.radii[4], 18.f - 2.f);
  EXPECT_FLOAT_EQ(padding_box.radii[5], 20.f - 4.f);
  EXPECT_FLOAT_EQ(padding_box.radii[6], 22.f - 1.f);
  EXPECT_FLOAT_EQ(padding_box.radii[7], 24.f - 4.f);

  EXPECT_EQ(items[2].type, DisplayListOpType::kRecordBox);
  const auto& content_box = items[2].payload.record_box;
  EXPECT_EQ(content_box.has_radii, 1u);
  EXPECT_FLOAT_EQ(content_box.x, 1.f);
  EXPECT_FLOAT_EQ(content_box.y, 3.f);
  EXPECT_FLOAT_EQ(content_box.w, 100.f - 1.f - 2.f);
  EXPECT_FLOAT_EQ(content_box.h, 60.f - 3.f - 4.f);
  EXPECT_FLOAT_EQ(content_box.radii[0], 10.f - 1.f);
  EXPECT_FLOAT_EQ(content_box.radii[1], 12.f - 3.f);
  EXPECT_FLOAT_EQ(content_box.radii[2], 14.f - 2.f);
  EXPECT_FLOAT_EQ(content_box.radii[3], 16.f - 3.f);
  EXPECT_FLOAT_EQ(content_box.radii[4], 18.f - 2.f);
  EXPECT_FLOAT_EQ(content_box.radii[5], 20.f - 4.f);
  EXPECT_FLOAT_EQ(content_box.radii[6], 22.f - 1.f);
  EXPECT_FLOAT_EQ(content_box.radii[7], 24.f - 4.f);

  EXPECT_EQ(items[3].type, DisplayListOpType::kImage);
  ASSERT_EQ(native_painting_context.created_images_.size(), 1u);
  EXPECT_EQ(native_painting_context.created_images_[0].id, fragment.id());
  EXPECT_EQ(native_painting_context.created_images_[0].mode,
            ImageFitMode::kAspectFit);
  const int32_t image_key =
      native_painting_context.created_images_[0].image_key;
  EXPECT_EQ(items[3].payload.image.image_id, image_key);
  EXPECT_EQ(items[3].payload.image.box_index, 2);
  ASSERT_EQ(list.Images().size(), 1u);
  ASSERT_NE(list.Images()[0], nullptr);
  EXPECT_EQ(list.Images()[0]->image_key_, image_key);
}

TEST_F(FragmentTest, ImageModesNormalizeBeforeCreation) {
  static_assert(static_cast<int32_t>(ImageFitMode::kScaleToFill) == 0);
  static_assert(static_cast<int32_t>(ImageFitMode::kAspectFit) == 1);
  static_assert(static_cast<int32_t>(ImageFitMode::kAspectFill) == 2);
  static_assert(static_cast<int32_t>(ImageFitMode::kCenter) == 3);

  struct ModeCase {
    const char* value;
    ImageFitMode expected;
  };
  constexpr std::array<ModeCase, 6> kModeCases = {{
      {"scaleToFill", ImageFitMode::kScaleToFill},
      {"aspectFit", ImageFitMode::kAspectFit},
      {"aspectFill", ImageFitMode::kAspectFill},
      {"center", ImageFitMode::kCenter},
      {"", ImageFitMode::kScaleToFill},
      {"unsupported", ImageFitMode::kScaleToFill},
  }};

  for (const auto& mode_case : kModeCases) {
    SCOPED_TRACE(mode_case.value);
    auto element = manager->CreateFiberImage("image");
    element->SetAttributeInternal("src", lepus::Value("image-src://mode"));
    element->SetAttributeInternal("mode", lepus::Value(mode_case.value));

    Fragment fragment(element.get());
    fragment.SetBehavior(std::make_unique<ImageFragmentBehavior>(&fragment));
    TestNativePaintingContext native_painting_context;
    fragment.behavior_->painting_context_ = &native_painting_context;

    starlight::LayoutResultForRendering layout;
    layout.size_ = FloatSize(100.f, 60.f);
    fragment.UpdateLayout(layout);
    fragment.behavior_->OnUpdateLayout(fragment.LayoutResult());

    ASSERT_EQ(native_painting_context.created_images_.size(), 1u);
    EXPECT_EQ(native_painting_context.created_images_[0].mode,
              mode_case.expected);
  }

  auto element = manager->CreateFiberImage("image");
  element->SetAttributeInternal("src", lepus::Value("image-src://default"));
  element->SetAttributeInternal("mode", lepus::Value(42));
  Fragment fragment(element.get());
  fragment.SetBehavior(std::make_unique<ImageFragmentBehavior>(&fragment));
  TestNativePaintingContext native_painting_context;
  fragment.behavior_->painting_context_ = &native_painting_context;
  starlight::LayoutResultForRendering layout;
  layout.size_ = FloatSize(100.f, 60.f);
  fragment.UpdateLayout(layout);
  fragment.behavior_->OnUpdateLayout(fragment.LayoutResult());
  ASSERT_EQ(native_painting_context.created_images_.size(), 1u);
  EXPECT_EQ(native_painting_context.created_images_[0].mode,
            ImageFitMode::kScaleToFill);
}

TEST_F(FragmentTest, ImageModeUpdateRecreatesOnlyForEffectiveChanges) {
  auto element = manager->CreateFiberImage("image");
  element->SetAttributeInternal("src", lepus::Value("image-src://initial"));
  element->SetAttributeInternal("mode", lepus::Value("aspectFit"));

  Fragment fragment(element.get());
  fragment.SetBehavior(std::make_unique<ImageFragmentBehavior>(&fragment));
  TestNativePaintingContext native_painting_context;
  fragment.behavior_->painting_context_ = &native_painting_context;

  starlight::LayoutResultForRendering layout;
  layout.size_ = FloatSize(100.f, 60.f);
  fragment.UpdateLayout(layout);
  fragment.behavior_->OnUpdateLayout(fragment.LayoutResult());
  DisplayListBuilder initial_builder;
  fragment.OnDraw(initial_builder);
  ASSERT_EQ(native_painting_context.created_images_.size(), 1u);

  element->SetAttributeInternal("mode", lepus::Value("aspectFit"));
  fragment.UpdatePaintingNode(true, nullptr);
  EXPECT_EQ(native_painting_context.created_images_.size(), 1u);

  element->SetAttributeInternal("mode", lepus::Value("aspectFill"));
  fragment.UpdatePaintingNode(true, nullptr);
  ASSERT_EQ(native_painting_context.created_images_.size(), 2u);
  EXPECT_EQ(native_painting_context.created_images_.back().mode,
            ImageFitMode::kAspectFill);
  EXPECT_TRUE(fragment.NeedRedraw());

  fragment.behavior_->OnUpdateLayout(fragment.LayoutResult());
  EXPECT_EQ(native_painting_context.created_images_.size(), 2u);

  element->SetAttributeInternal("mode", lepus::Value("unsupported"));
  fragment.UpdatePaintingNode(true, nullptr);
  ASSERT_EQ(native_painting_context.created_images_.size(), 3u);
  EXPECT_EQ(native_painting_context.created_images_.back().mode,
            ImageFitMode::kScaleToFill);

  element->SetAttributeInternal("mode", lepus::Value("still-unsupported"));
  fragment.UpdatePaintingNode(true, nullptr);
  EXPECT_EQ(native_painting_context.created_images_.size(), 3u);

  element->ResetAttribute(base::String("mode"));
  fragment.UpdatePaintingNode(true, nullptr);
  EXPECT_EQ(native_painting_context.created_images_.size(), 3u);
}

TEST_F(FragmentTest, ImageBlurRadiusUpdateRecreatesImage) {
  auto element = manager->CreateFiberImage("image");
  element->SetAttributeInternal("src", lepus::Value("image-src://initial"));
  element->SetAttributeInternal("blur-radius", lepus::Value("0px"));

  Fragment fragment(element.get());
  fragment.SetBehavior(std::make_unique<ImageFragmentBehavior>(&fragment));
  TestNativePaintingContext native_painting_context;
  fragment.behavior_->painting_context_ = &native_painting_context;

  starlight::LayoutResultForRendering layout;
  layout.size_ = FloatSize(100.f, 60.f);
  fragment.UpdateLayout(layout);
  fragment.behavior_->OnUpdateLayout(fragment.LayoutResult());

  ASSERT_EQ(native_painting_context.created_images_.size(), 1u);
  EXPECT_EQ(native_painting_context.created_images_[0].blur_radius, "0px");

  element->SetAttributeInternal("blur-radius", lepus::Value("5px"));
  fragment.UpdatePaintingNode(true, nullptr);

  ASSERT_EQ(native_painting_context.created_images_.size(), 2u);
  EXPECT_EQ(native_painting_context.created_images_.back().blur_radius, "5px");
  EXPECT_TRUE(fragment.NeedRedraw());
}

TEST_F(FragmentTest, ImagePaintInfoAttributesReachNativePaintingContext) {
  auto element = manager->CreateFiberImage("image");
  element->SetAttributeInternal("src", lepus::Value("image-src://initial"));
  element->SetAttributeInternal("auto-size", lepus::Value(true));
  element->SetAttributeInternal("placeholder",
                                lepus::Value("image-src://placeholder"));
  element->SetAttributeInternal("tint-color", lepus::Value("#ff0000"));
  element->SetAttributeInternal("cap-insets", lepus::Value("1px 2px 3px 4px"));
  element->SetAttributeInternal("cap-insets-scale", lepus::Value("2.5"));
  element->SetAttributeInternal("skip-redirection", lepus::Value(true));
  element->SetAttributeInternal("autoplay", lepus::Value(false));
  element->SetAttributeInternal("loop-count", lepus::Value(3));

  Fragment fragment(element.get());
  fragment.SetBehavior(std::make_unique<ImageFragmentBehavior>(&fragment));
  TestNativePaintingContext native_painting_context;
  fragment.behavior_->painting_context_ = &native_painting_context;

  starlight::LayoutResultForRendering layout;
  layout.size_ = FloatSize(100.f, 60.f);
  fragment.UpdateLayout(layout);
  fragment.behavior_->OnUpdateLayout(fragment.LayoutResult());

  ASSERT_EQ(native_painting_context.created_images_.size(), 1u);
  const auto& image = native_painting_context.created_images_.back();
  EXPECT_TRUE(image.auto_size);
  EXPECT_EQ(image.placeholder, "image-src://placeholder");
  EXPECT_EQ(image.tint_color, "#ff0000");
  EXPECT_EQ(image.cap_insets, "1px 2px 3px 4px");
  EXPECT_FLOAT_EQ(image.cap_insets_scale, 2.5f);
  EXPECT_TRUE(image.skip_redirection);
  EXPECT_FALSE(image.autoplay);
  EXPECT_EQ(image.loop_count, 3);
}

TEST_F(FragmentTest, ImageSrcUpdateInvalidatesWithoutDuplicateImageCreation) {
  // Given: an image has completed its initial layout and draw.
  auto element = manager->CreateFiberImage("image");
  element->SetAttributeInternal("src", lepus::Value("image-src://initial"));

  Fragment fragment(element.get());
  fragment.SetBehavior(std::make_unique<ImageFragmentBehavior>(&fragment));
  TestNativePaintingContext native_painting_context;
  fragment.behavior_->painting_context_ = &native_painting_context;

  starlight::LayoutResultForRendering layout;
  layout.size_ = FloatSize(100.f, 60.f);
  fragment.UpdateLayout(layout);
  fragment.behavior_->OnUpdateLayout(fragment.LayoutResult());
  DisplayListBuilder initial_builder;
  fragment.OnDraw(initial_builder);

  ASSERT_EQ(native_painting_context.created_images_.size(), 1u);
  EXPECT_EQ(native_painting_context.created_images_[0].mode,
            ImageFitMode::kScaleToFill);
  ASSERT_FALSE(fragment.NeedRedraw());

  // When: src changes and the same-size update, layout, and draw path runs.
  element->SetAttributeInternal("src", lepus::Value("image-src://updated"));
  fragment.UpdatePaintingNode(true, nullptr);

  // Then: the attribute update has already refreshed and invalidated the image.
  EXPECT_TRUE(fragment.NeedRedraw());
  ASSERT_EQ(native_painting_context.created_images_.size(), 2u);
  EXPECT_EQ(native_painting_context.created_images_.back().src,
            "image-src://updated");
  const int32_t updated_image_key =
      native_painting_context.created_images_.back().image_key;

  fragment.behavior_->OnUpdateLayout(fragment.LayoutResult());
  DisplayListBuilder updated_builder;
  fragment.OnDraw(updated_builder);
  DisplayList updated_list = updated_builder.Build();

  EXPECT_EQ(native_painting_context.created_images_.size(), 2u);
  EXPECT_FALSE(fragment.NeedRedraw());
  ASSERT_EQ(updated_list.Images().size(), 1u);
  ASSERT_NE(updated_list.Images()[0], nullptr);
  EXPECT_EQ(updated_list.Images()[0]->image_key_, updated_image_key);
}

TEST_F(FragmentTest, ImageSrcUpdateRecreatesForChangedLayoutSize) {
  // Given: an image has completed its initial layout.
  auto element = manager->CreateFiberImage("image");
  element->SetAttributeInternal("src", lepus::Value("image-src://initial"));

  Fragment fragment(element.get());
  fragment.SetBehavior(std::make_unique<ImageFragmentBehavior>(&fragment));
  TestNativePaintingContext native_painting_context;
  fragment.behavior_->painting_context_ = &native_painting_context;

  starlight::LayoutResultForRendering initial_layout;
  initial_layout.size_ = FloatSize(100.f, 60.f);
  fragment.UpdateLayout(initial_layout);
  fragment.behavior_->OnUpdateLayout(fragment.LayoutResult());
  ASSERT_EQ(native_painting_context.created_images_.size(), 1u);

  // When: src changes before the following layout changes the content size.
  element->SetAttributeInternal("src", lepus::Value("image-src://updated"));
  fragment.UpdatePaintingNode(true, nullptr);
  ASSERT_EQ(native_painting_context.created_images_.size(), 2u);

  starlight::LayoutResultForRendering updated_layout;
  updated_layout.size_ = FloatSize(120.f, 80.f);
  fragment.UpdateLayout(updated_layout);
  fragment.behavior_->OnUpdateLayout(fragment.LayoutResult());

  // Then: the image is recreated once with the updated dimensions and retained.
  ASSERT_EQ(native_painting_context.created_images_.size(), 3u);
  const auto& updated_image = native_painting_context.created_images_.back();
  EXPECT_EQ(updated_image.src, "image-src://updated");
  EXPECT_FLOAT_EQ(updated_image.width, 120.f);
  EXPECT_FLOAT_EQ(updated_image.height, 80.f);

  DisplayListBuilder updated_builder;
  fragment.OnDraw(updated_builder);
  DisplayList updated_list = updated_builder.Build();
  ASSERT_EQ(updated_list.Images().size(), 1u);
  ASSERT_NE(updated_list.Images()[0], nullptr);
  EXPECT_EQ(updated_list.Images()[0]->image_key_, updated_image.image_key);
}

TEST_F(FragmentTest, ImageUpdateWaitsForNativePaintingContext) {
  // Given: image behavior exists before a native painting context is available.
  auto element = manager->CreateFiberImage("image");
  element->SetAttributeInternal("src", lepus::Value("image-src://initial"));

  Fragment fragment(element.get());
  fragment.SetBehavior(std::make_unique<ImageFragmentBehavior>(&fragment));
  ASSERT_EQ(fragment.behavior_->painting_context_, nullptr);

  starlight::LayoutResultForRendering layout;
  layout.size_ = FloatSize(100.f, 60.f);
  fragment.UpdateLayout(layout);

  // When: the attribute update arrives without a native context.
  fragment.UpdatePaintingNode(true, nullptr);

  // Then: the image remains pending and is created by a later valid layout.
  TestNativePaintingContext native_painting_context;
  fragment.behavior_->painting_context_ = &native_painting_context;
  fragment.behavior_->OnUpdateLayout(fragment.LayoutResult());
  ASSERT_EQ(native_painting_context.created_images_.size(), 1u);
  EXPECT_EQ(native_painting_context.created_images_.back().src,
            "image-src://initial");
  EXPECT_FLOAT_EQ(native_painting_context.created_images_.back().width, 100.f);
  EXPECT_FLOAT_EQ(native_painting_context.created_images_.back().height, 60.f);
}

TEST_F(FragmentTest, ImageSrcResetCreatesEmptyReplacement) {
  // Given: an image has completed layout with a non-empty src.
  auto element = manager->CreateFiberImage("image");
  element->SetAttributeInternal("src", lepus::Value("image-src://initial"));

  Fragment fragment(element.get());
  fragment.SetBehavior(std::make_unique<ImageFragmentBehavior>(&fragment));
  TestNativePaintingContext native_painting_context;
  fragment.behavior_->painting_context_ = &native_painting_context;

  starlight::LayoutResultForRendering layout;
  layout.size_ = FloatSize(100.f, 60.f);
  fragment.UpdateLayout(layout);
  fragment.behavior_->OnUpdateLayout(fragment.LayoutResult());
  ASSERT_EQ(native_painting_context.created_images_.size(), 1u);

  // When: the src attribute is removed through the production update path.
  element->ResetAttribute(BASE_STATIC_STRING(kSrc));
  fragment.UpdatePaintingNode(true, nullptr);

  // Then: an empty replacement clears the old retained image and redraws.
  ASSERT_EQ(native_painting_context.created_images_.size(), 2u);
  EXPECT_TRUE(native_painting_context.created_images_.back().src.empty());
  EXPECT_TRUE(fragment.NeedRedraw());

  DisplayListBuilder updated_builder;
  fragment.OnDraw(updated_builder);
  DisplayList updated_list = updated_builder.Build();
  ASSERT_EQ(updated_list.Images().size(), 1u);
  ASSERT_NE(updated_list.Images()[0], nullptr);
  EXPECT_EQ(updated_list.Images()[0]->image_key_,
            native_painting_context.created_images_.back().image_key);
}

TEST_F(FragmentTest, ImageUpdateRetriesAfterFailedCreation) {
  auto element = manager->CreateFiberImage("image");
  element->SetAttributeInternal("src", lepus::Value("image-src://initial"));

  Fragment fragment(element.get());
  fragment.SetBehavior(std::make_unique<ImageFragmentBehavior>(&fragment));
  TestNativePaintingContext native_painting_context;
  native_painting_context.fail_image_creation_ = true;
  fragment.behavior_->painting_context_ = &native_painting_context;

  starlight::LayoutResultForRendering layout;
  layout.size_ = FloatSize(100.f, 60.f);
  fragment.UpdateLayout(layout);
  fragment.behavior_->OnUpdateLayout(fragment.LayoutResult());
  EXPECT_TRUE(native_painting_context.created_images_.empty());

  native_painting_context.fail_image_creation_ = false;
  fragment.behavior_->OnUpdateLayout(fragment.LayoutResult());
  ASSERT_EQ(native_painting_context.created_images_.size(), 1u);
  EXPECT_EQ(native_painting_context.created_images_.back().src,
            "image-src://initial");
  EXPECT_FLOAT_EQ(native_painting_context.created_images_.back().width, 100.f);
  EXPECT_FLOAT_EQ(native_painting_context.created_images_.back().height, 60.f);
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
  auto items = CollectDisplayListItems(list);
  ASSERT_GE(items.size(), 4u);

  EXPECT_EQ(items[0].type, DisplayListOpType::kRecordBox);
  EXPECT_EQ(items[1].type, DisplayListOpType::kFill);
  EXPECT_EQ(items[2].type, DisplayListOpType::kRecordBox);
  EXPECT_EQ(items[3].type, DisplayListOpType::kLinearGradient);

  const auto& gradient = items[3].payload.linear_gradient;
  EXPECT_EQ(gradient.color_count, 2u);
  EXPECT_EQ(gradient.stop_count, 2u);
  EXPECT_EQ(gradient.repeat_x,
            static_cast<int32_t>(starlight::BackgroundRepeatType::kRepeat));
  EXPECT_EQ(gradient.repeat_y,
            static_cast<int32_t>(starlight::BackgroundRepeatType::kNoRepeat));
  EXPECT_FLOAT_EQ(gradient.angle, 90.0f);

  DisplayListReader reader(list);
  const uint32_t* colors = reader.Colors(items[3]);
  const float* stops = reader.Stops(items[3]);
  ASSERT_NE(colors, nullptr);
  ASSERT_NE(stops, nullptr);
  EXPECT_EQ(colors[0], 0xFFFF0000u);
  EXPECT_EQ(colors[1], 0xFF0000FFu);
  EXPECT_FLOAT_EQ(stops[0], 0.0f);
  EXPECT_FLOAT_EQ(stops[1], 1.0f);
}

TEST_F(FragmentTest, LinearGradientCornerDirectionUsesTilingBoxSize) {
  struct TestCase {
    starlight::LinearGradientDirection direction;
    float expected_angle;
  };
  const TestCase test_cases[] = {
      {starlight::LinearGradientDirection::kTopRight, 63.434948f},
      {starlight::LinearGradientDirection::kTopLeft, 296.565063f},
      {starlight::LinearGradientDirection::kBottomRight, 116.565048f},
      {starlight::LinearGradientDirection::kBottomLeft, 243.434952f},
  };

  for (const auto& test_case : test_cases) {
    SCOPED_TRACE(static_cast<int32_t>(test_case.direction));
    auto element = manager->CreateFiberView();
    Fragment fragment(element.get());

    starlight::LayoutResultForRendering layout;
    layout.size_ = FloatSize(300.f, 300.f);
    fragment.UpdateLayout(layout);

    auto* style = element->computed_css_style();
    style->background_data_ = starlight::BackgroundData();
    style->background_data_->image_data =
        starlight::BackgroundData::BackgroundImageData();
    auto& image_data = *style->background_data_->image_data;
    image_data.image_count = 1;
    image_data.size.push_back(starlight::NLength::MakeUnitNLength(100.f));
    image_data.size.push_back(starlight::NLength::MakeUnitNLength(200.f));

    auto color_array = lepus::CArray::Create();
    color_array->emplace_back(0xFFFF0000);
    color_array->emplace_back(0xFF0000FF);

    auto position_array = lepus::CArray::Create();
    position_array->emplace_back(0.0f);
    position_array->emplace_back(100.0f);

    auto gradient_obj = lepus::CArray::Create();
    gradient_obj->emplace_back(45.0f);
    gradient_obj->emplace_back(std::move(color_array));
    gradient_obj->emplace_back(std::move(position_array));
    gradient_obj->emplace_back(static_cast<int32_t>(test_case.direction));

    auto image_array = lepus::CArray::Create();
    image_array->emplace_back(
        static_cast<int32_t>(starlight::BackgroundImageType::kLinearGradient));
    image_array->emplace_back(std::move(gradient_obj));
    image_data.image = lepus::Value(std::move(image_array));

    DisplayListBuilder builder;
    fragment.DrawBackground(builder);

    DisplayList list = builder.Build();
    DisplayListReader reader(list);
    ASSERT_TRUE(reader.HasNext());
    reader.Next();  // clip box
    ASSERT_TRUE(reader.HasNext());
    reader.Next();  // background color
    ASSERT_TRUE(reader.HasNext());
    reader.Next();  // tiling box
    ASSERT_TRUE(reader.HasNext());
    const auto& gradient_item = reader.Next();
    ASSERT_EQ(gradient_item.type, DisplayListOpType::kLinearGradient);
    EXPECT_NEAR(gradient_item.payload.linear_gradient.angle,
                test_case.expected_angle, 0.0001f);
  }
}

TEST_F(FragmentDrawTest, BackgroundUrlGeneratesBackgroundImageOp) {
  auto element = manager->CreateFiberView();
  Fragment fragment(element.get());

  starlight::LayoutResultForRendering layout;
  layout.border_ = starlight::DirectionValue<float>({0.f, 0.f, 0.f, 0.f});
  layout.padding_ = starlight::DirectionValue<float>({0.f, 0.f, 0.f, 0.f});
  layout.size_ = FloatSize(100.f, 80.f);
  fragment.UpdateLayout(layout);

  auto* style = element->computed_css_style();
  style->background_data_ = starlight::BackgroundData();
  style->background_data_->image_data =
      starlight::BackgroundData::BackgroundImageData();
  auto& image_data = *style->background_data_->image_data;
  image_data.image_count = 1;
  image_data.origin.push_back(starlight::BackgroundOriginType::kBorderBox);
  image_data.clip.push_back(starlight::BackgroundClipType::kBorderBox);
  image_data.repeat.push_back(starlight::BackgroundRepeatType::kRepeat);
  image_data.repeat.push_back(starlight::BackgroundRepeatType::kNoRepeat);
  image_data.size.push_back(starlight::NLength::MakeUnitNLength(40.f));
  image_data.size.push_back(starlight::NLength::MakeUnitNLength(20.f));
  image_data.position.push_back(starlight::NLength::MakeUnitNLength(10.f));
  image_data.position.push_back(starlight::NLength::MakeUnitNLength(15.f));

  auto image_array = lepus::CArray::Create();
  image_array->emplace_back(
      static_cast<int32_t>(starlight::BackgroundImageType::kUrl));
  image_array->emplace_back("https://example.com/bg.png");
  image_data.image = lepus::Value(std::move(image_array));

  auto* native_context = static_cast<NativeMockPaintingContext*>(
      manager->painting_context()->impl());

  DisplayListBuilder builder;
  fragment.DrawBackground(builder);

  ASSERT_EQ(native_context->created_images_.size(), 1u);
  EXPECT_EQ(native_context->created_images_[0].src.str(),
            "https://example.com/bg.png");
  EXPECT_FLOAT_EQ(native_context->created_images_[0].width, 40.f);
  EXPECT_FLOAT_EQ(native_context->created_images_[0].height, 20.f);
  EXPECT_EQ(native_context->created_images_[0].event_mask, 0);
  EXPECT_TRUE(native_context->created_images_[0].disable_default_resize);

  DisplayList list = builder.Build();
  auto items = CollectDisplayListItems(list);
  ASSERT_EQ(items.size(), 4u);
  EXPECT_EQ(items[0].type, DisplayListOpType::kRecordBox);
  EXPECT_EQ(items[1].type, DisplayListOpType::kFill);
  EXPECT_EQ(items[2].type, DisplayListOpType::kRecordBox);
  EXPECT_EQ(items[3].type, DisplayListOpType::kBackgroundImage);

  const auto& tiling_box = items[2].payload.record_box;
  EXPECT_FLOAT_EQ(tiling_box.x, 10.f);
  EXPECT_FLOAT_EQ(tiling_box.y, 15.f);
  EXPECT_FLOAT_EQ(tiling_box.w, 40.f);
  EXPECT_FLOAT_EQ(tiling_box.h, 20.f);

  const auto& background_image = items[3].payload.background_image;
  EXPECT_EQ(background_image.image_id,
            native_context->created_images_[0].image_key);
  EXPECT_EQ(background_image.tiling_index, 1);
  EXPECT_EQ(background_image.clip_index, 0);
  EXPECT_EQ(background_image.repeat_x,
            static_cast<int32_t>(starlight::BackgroundRepeatType::kRepeat));
  EXPECT_EQ(background_image.repeat_y,
            static_cast<int32_t>(starlight::BackgroundRepeatType::kNoRepeat));
  ASSERT_EQ(list.Images().size(), 1u);
  ASSERT_NE(list.Images()[0], nullptr);
  EXPECT_EQ(list.Images()[0]->image_key_, background_image.image_id);

  image_data.size.clear();
  image_data.size.push_back(starlight::NLength::MakeUnitNLength(60.f));
  image_data.size.push_back(starlight::NLength::MakeUnitNLength(30.f));

  DisplayListBuilder repaint_builder;
  fragment.DrawBackground(repaint_builder);

  EXPECT_EQ(native_context->created_images_.size(), 1u);
  DisplayList repaint_list = repaint_builder.Build();
  auto repaint_items = CollectDisplayListItems(repaint_list);
  ASSERT_EQ(repaint_items.size(), 3u);
  EXPECT_EQ(repaint_items[2].type, DisplayListOpType::kBackgroundImage);
  EXPECT_EQ(repaint_items[2].payload.background_image.image_id,
            native_context->created_images_[0].image_key);
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
  auto items = CollectDisplayListItems(list);
  ASSERT_GE(items.size(), 2u);

  EXPECT_EQ(items[0].type, DisplayListOpType::kRecordBox);
  EXPECT_EQ(items[1].type, DisplayListOpType::kFill);

  const auto& box = items[0].payload.record_box;
  EXPECT_FLOAT_EQ(box.x, 16.f);
  EXPECT_FLOAT_EQ(box.y, 26.f);
  EXPECT_FLOAT_EQ(box.w, 62.f);
  EXPECT_FLOAT_EQ(box.h, 20.f);
  EXPECT_EQ(items[1].payload.fill.color, 0xFF00FF00u);
  EXPECT_EQ(items[1].payload.fill.clip_index, 0);
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
  EXPECT_GE(list.GetContentItemsSize(), 1u);
}

TEST_F(FragmentDrawTest,
       DisplayNoneEmitsEmptyDisplayListAndReconstructsExposure) {
  auto page = manager->CreateFiberPage("0", 0);
  ASSERT_NE(page, nullptr);
  page->FlushActionsAsRoot();
  ASSERT_TRUE(page->HasElementContainer());

  auto* fragment = static_cast<Fragment*>(page->element_container());
  ASSERT_NE(fragment, nullptr);

  // Ensure the page fragment has a behavior and a backing platform renderer.
  page->SetupFragmentBehavior(fragment);
  auto* native_ctx = static_cast<NativeMockPaintingContext*>(
                         fragment->painting_context()->impl())
                         ->GetNativePlatformRef();
  ASSERT_NE(native_ctx, nullptr);
  native_ctx->CreatePlatformRenderer(fragment->id(),
                                     PlatformRendererType::kPage, nullptr);
  fragment->has_platform_renderer_ = true;

  starlight::LayoutResultForRendering layout;
  layout.border_ = starlight::DirectionValue<float>({0.f, 0.f, 0.f, 0.f});
  layout.padding_ = starlight::DirectionValue<float>({0.f, 0.f, 0.f, 0.f});
  layout.size_ = FloatSize(100.f, 60.f);
  fragment->UpdateLayout(layout);

  // Give the page a background so the visible draw produces more than
  // Begin/End.
  page->computed_css_style()->background_data_ = starlight::BackgroundData();
  page->computed_css_style()->background_data_->color = 0xFF00FF00;

  auto renderer_it = native_ctx->renderers_.find(fragment->id());
  ASSERT_NE(renderer_it, native_ctx->renderers_.end());
  auto* renderer =
      static_cast<TestPlatformRenderer*>(renderer_it->second.get());

  // When visible, Draw() produces a display list with background content.
  page->display_none_ = false;
  fragment->Draw();
  const size_t visible_op_count = renderer->display_list_.GetContentItemsSize();
  EXPECT_GT(visible_op_count, 2u);

  // When display_none becomes true, Draw() must still send a display list that
  // contains only this node's Begin/End so the platform layer clears stale
  // content / sublayers / event-target state instead of keeping the previous
  // frame. It must also still run ReconstructEventTargetTreeForExposure for the
  // root.
  page->display_none_ = true;
  manager->MarkNeedReconstructEventTargetTreeForExposure();
  fragment->Draw();
  EXPECT_FALSE(manager->NeedReconstructEventTargetTreeForExposure());
  auto display_none_items = CollectDisplayListItems(renderer->display_list_);
  ASSERT_EQ(display_none_items.size(), 2u);
  EXPECT_EQ(display_none_items[0].type, DisplayListOpType::kBegin);
  EXPECT_EQ(display_none_items[1].type, DisplayListOpType::kEnd);
}

TEST_F(FragmentDrawTest, FragmentLayerRenderFinishesLayoutAfterDisplayList) {
  auto page = manager->CreateFiberPage("0", 0);
  ASSERT_NE(page, nullptr);
  page->FlushActionsAsRoot();
  ASSERT_TRUE(page->HasElementContainer());

  auto* fragment = static_cast<Fragment*>(page->element_container());
  ASSERT_NE(fragment, nullptr);
  page->SetupFragmentBehavior(fragment);

  auto* native_context = static_cast<NativeMockPaintingContext*>(
      fragment->painting_context()->impl());
  auto* native_ref = native_context->GetNativePlatformRef();
  ASSERT_NE(native_ref, nullptr);
  native_ref->CreatePlatformRenderer(fragment->id(),
                                     PlatformRendererType::kPage, nullptr);
  fragment->has_platform_renderer_ = true;

  auto options = std::make_shared<PipelineOptions>();
  native_context->operations.clear();
  page->Layout(options);

  EXPECT_TRUE(options->has_layout);
  EXPECT_TRUE(native_context->operations.empty());

  fragment->Draw();
  fragment->FinishLayoutOperation(options);

  ASSERT_EQ(native_context->operations.size(), 2u);
  EXPECT_EQ(native_context->operations[0], "update_display_list");
  EXPECT_EQ(native_context->operations[1], "finish_layout");
}

}  // namespace tasm
}  // namespace lynx
