// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/json_parser.h"

#define private public
#define protected public

#include "core/list/decoupled_list_container_impl.h"
#include "core/public/pipeline_option.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "testing/fiber_data_source.h"
#include "testing/mock_animation_manager.h"
#include "testing/mock_list_element.h"
#include "testing/radon_data_source.h"
#include "testing/utils.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace list {

class ListContainerImplTest : public ::testing::Test {
 public:
  ListContainerImplTest() = default;
  ~ListContainerImplTest() override = default;

  std::unique_ptr<MockListElement> mock_list_element_{nullptr};
  std::unique_ptr<ListContainerImpl> list_container_impl_{nullptr};
  std::shared_ptr<pub::PubValueFactoryDefault> value_factory_{nullptr};
  ListLayoutManager* list_layout_manager_{nullptr};
  ListAdapter* list_adapter_{nullptr};
  ListEventManager* list_event_manager_{nullptr};
  MockAnimationManager* mock_animation_manager_{nullptr};

  void SetUp() override {
    value_factory_ = std::make_shared<pub::PubValueFactoryDefault>();
    mock_list_element_ = std::make_unique<MockListElement>();
    list_container_impl_ = std::make_unique<ListContainerImpl>(
        mock_list_element_.get(), value_factory_);
    list_layout_manager_ = list_container_impl_->list_layout_manager();
    list_adapter_ = list_container_impl_->list_adapter();
    list_event_manager_ = list_container_impl_->list_event_manager();
  }

  UpdateAnimationConfig ResolveAnimationConfig(const char* json) {
    auto key = value_factory_->CreateString(kPropExperimentalUpdateAnimation);
    pub::ValueImplLepus value(lepus::jsonValueTolepusValue(json));
    EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, value));
    EXPECT_TRUE(list_container_impl_->new_update_animation_config_.has_value());
    return list_container_impl_->new_update_animation_config_.value_or(
        UpdateAnimationConfig{});
  }

  void InstallMockAnimationManager() {
    auto mock_animation_manager =
        std::make_unique<::testing::NiceMock<MockAnimationManager>>();
    mock_animation_manager_ = mock_animation_manager.get();
    list_container_impl_->animation_manager_ =
        std::move(mock_animation_manager);
  }
};

TEST_F(ListContainerImplTest, Constructor) {
  EXPECT_EQ(list_container_impl_->list_delegate(), mock_list_element_.get());
  EXPECT_TRUE(
      base::FloatsEqual(list_container_impl_->physical_pixels_per_layout_unit_,
                        kDefaultPhysicalPixelsPerLayoutUnit));
}

TEST_LIST_CONTAINER_RESOLVE_PROP(CustomListName) {
  LIST_CONTAINER_DEFINE_PROP_VALUE(CustomListName, String,
                                   kPropValueListContainer);
  EXPECT_CALL(*mock_list_element_, UpdateListLayoutNodeAttribute()).Times(1);
  EXPECT_TRUE(list_container_impl_->ResolveAttribute(*key, *value));
}

TEST_LIST_CONTAINER_RESOLVE_PROP(VerticalOrientation) {
  LIST_CONTAINER_DEFINE_PROP_VALUE(VerticalOrientation, Bool, true);
  EXPECT_TRUE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_EQ(list_layout_manager_->orientation(), Orientation::kVertical);
  EXPECT_TRUE(list_layout_manager_->list_orientation_helper_->IsVertical());
  value = value_factory_->CreateBool(false);
  EXPECT_TRUE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_EQ(list_layout_manager_->orientation(), Orientation::kHorizontal);
  EXPECT_FALSE(list_layout_manager_->list_orientation_helper_->IsVertical());
}

TEST_LIST_CONTAINER_RESOLVE_PROP(ScrollOrientation) {
  LIST_CONTAINER_DEFINE_PROP_VALUE(ScrollOrientation, String,
                                   kPropValueScrollOrientationVertical);
  EXPECT_TRUE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_EQ(list_layout_manager_->orientation(), Orientation::kVertical);
  EXPECT_TRUE(list_layout_manager_->list_orientation_helper_->IsVertical());
  value = value_factory_->CreateString(kPropValueScrollOrientationHorizontal);
  EXPECT_TRUE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_EQ(list_layout_manager_->orientation(), Orientation::kHorizontal);
  EXPECT_FALSE(list_layout_manager_->list_orientation_helper_->IsVertical());
}

TEST_LIST_CONTAINER_RESOLVE_PROP(EnableDynamicSpanCount) {
  EXPECT_TRUE(list_container_impl_->enable_dynamic_span_count_);
  LIST_CONTAINER_DEFINE_PROP_VALUE(EnableDynamicSpanCount, Bool, false);
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_FALSE(list_container_impl_->enable_dynamic_span_count_);
}

TEST_LIST_CONTAINER_RESOLVE_PROP(SpanCount) {
  LIST_CONTAINER_DEFINE_PROP_VALUE(SpanCount, Number, 2);
  EXPECT_CALL(*mock_list_element_, MarkListElementLayoutDirty()).Times(1);
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_EQ(list_layout_manager_->span_count_, 2);
}

TEST_LIST_CONTAINER_RESOLVE_PROP(ColumnCount) {
  LIST_CONTAINER_DEFINE_PROP_VALUE(ColumnCount, Number, 2);
  EXPECT_CALL(*mock_list_element_, MarkListElementLayoutDirty()).Times(1);
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_EQ(list_layout_manager_->span_count_, 2);
}

TEST_LIST_CONTAINER_RESOLVE_PROP(AnchorPriority) {
  LIST_CONTAINER_DEFINE_PROP_VALUE(AnchorPriority, String,
                                   kPropValueAnchorPriorityFromBegin);
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_TRUE(
      list_layout_manager_->list_anchor_manager_->anchor_priority_from_begin_);
  value = value_factory_->CreateString(kPropValueAnchorPriorityFromEnd);
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_FALSE(
      list_layout_manager_->list_anchor_manager_->anchor_priority_from_begin_);
}

TEST_LIST_CONTAINER_RESOLVE_PROP(AnchorAlign) {
  LIST_CONTAINER_DEFINE_PROP_VALUE(AnchorAlign, String,
                                   kPropValueAnchorAlignToBottom);
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_TRUE(
      list_layout_manager_->list_anchor_manager_->anchor_align_to_bottom_);
  value = value_factory_->CreateString(kPropValueAnchorAlignToTop);
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_FALSE(
      list_layout_manager_->list_anchor_manager_->anchor_align_to_bottom_);
}

TEST_LIST_CONTAINER_RESOLVE_PROP(AnchorVisibility) {
  LIST_CONTAINER_DEFINE_PROP_VALUE(AnchorVisibility, String,
                                   kPropValueAnchorVisibilityHide);
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_EQ(list_layout_manager_->list_anchor_manager_->anchor_visibility_,
            AnchorVisibility::kAnchorVisibilityHide);
  value = value_factory_->CreateString(kPropValueAnchorVisibilityShow);
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_EQ(list_layout_manager_->list_anchor_manager_->anchor_visibility_,
            AnchorVisibility::kAnchorVisibilityShow);
}

TEST_LIST_CONTAINER_RESOLVE_PROP(RadonListPlatformInfo) {
  EXPECT_FALSE(list_container_impl_->has_valid_diff_);
  EXPECT_FALSE(list_container_impl_->need_preload_section_on_next_frame_);
  EXPECT_FALSE(list_container_impl_->need_update_item_holders_);
  testing::RadonDataSource radon_data_source{
      .item_keys_ = {"A_0", "B_1", "C_2", "D_3", "E_4", "F_5", "G_6", "H_7",
                     "I_8"},
      .insertion_ = {0, 1, 2, 3, 4, 5, 6, 7, 8},
      .estimated_height_pxs_ = {100, 100, 100, 100, 100, 100, 100, 100, 100},
  };
  LIST_CONTAINER_DEFINE_PROP_LEPUS_VALUE(
      RadonListPlatformInfo,
      lepus::Value(radon_data_source.GenerateDataSource()));
  EXPECT_CALL(*mock_list_element_, MarkListElementLayoutDirty()).Times(1);
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, value));
  EXPECT_TRUE(list_container_impl_->has_valid_diff_);
  EXPECT_TRUE(list_container_impl_->need_preload_section_on_next_frame_);
  EXPECT_TRUE(list_container_impl_->need_update_item_holders_);
  EXPECT_EQ(list_container_impl_->GetDataCount(),
            radon_data_source.GetItemCount());
}

TEST_LIST_CONTAINER_RESOLVE_PROP(FiberUpdateListInfo) {
  EXPECT_FALSE(list_container_impl_->has_valid_diff_);
  EXPECT_FALSE(list_container_impl_->need_preload_section_on_next_frame_);
  EXPECT_FALSE(list_container_impl_->need_update_item_holders_);
  testing::InsertAction insert_action{
      .insert_ops_ = {
          {.position_ = 0, "A_0", 100, false, false, false, false},
          {.position_ = 1, "B_1", 100, false, false, false, false},
          {.position_ = 2, "C_2", 100, false, false, false, false},
          {.position_ = 3, "D_3", 100, false, false, false, false},
          {.position_ = 4, "E_4", 100, false, false, false, false},
          {.position_ = 5, "F_5", 100, false, false, false, false},
          {.position_ = 6, "G_6", 100, false, false, false, false},
          {.position_ = 7, "H_7", 100, false, false, false, false},
          {.position_ = 8, "I_8", 100, false, false, false, false},
          {.position_ = 9, "J_9", 100, false, false, false, false},
      }};
  testing::FiberDataSource fiber_data_source{
      .insert_action_ = insert_action,
  };
  LIST_CONTAINER_DEFINE_PROP_LEPUS_VALUE(
      FiberUpdateListInfo, lepus::Value(fiber_data_source.Resolve()));
  EXPECT_CALL(*mock_list_element_, MarkListElementLayoutDirty()).Times(1);
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, value));
  EXPECT_TRUE(list_container_impl_->has_valid_diff_);
  EXPECT_TRUE(list_container_impl_->need_preload_section_on_next_frame_);
  EXPECT_TRUE(list_container_impl_->need_update_item_holders_);
  EXPECT_EQ(list_container_impl_->GetDataCount(), 10);
}

TEST_LIST_CONTAINER_RESOLVE_PROP(UpdateAnimation) {
  EXPECT_FALSE(list_container_impl_->update_animation_);
  LIST_CONTAINER_DEFINE_PROP_VALUE(UpdateAnimation, String,
                                   kPropValueUpdateAnimationDefault);
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_TRUE(list_container_impl_->update_animation_);
}

// Verifies that the first parsed animation-pipeline selection is locked and
// later property updates cannot switch implementations.
TEST_F(ListContainerImplTest, FreezesNewUpdateAnimationSelection) {
  auto key =
      value_factory_->CreateString(kPropExperimentalUseNewUpdateAnimation);
  auto value = value_factory_->CreateBool(true);

  // 1. Parsing the selection property for the first time immediately selects
  // and locks the new update-animation pipeline.
  EXPECT_FALSE(list_container_impl_->use_new_update_animation_.has_value());
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, *value));
  ASSERT_TRUE(list_container_impl_->use_new_update_animation_.has_value());
  EXPECT_TRUE(list_container_impl_->use_new_update_animation());

  // 2. PropsUpdateFinish preserves the locked pipeline selection.
  list_container_impl_->PropsUpdateFinish();
  EXPECT_TRUE(list_container_impl_->use_new_update_animation());

  // 3. A later update to false is ignored so the lifecycle implementation
  // cannot change at runtime.
  value = value_factory_->CreateBool(false);
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_TRUE(list_container_impl_->use_new_update_animation());
}

// Verifies that an absent selection defaults to the legacy pipeline and that
// enabling the new pipeline after the first PropsUpdateFinish is ignored.
TEST_F(ListContainerImplTest, DefaultsToLegacyAnimationSelection) {
  // 1. The first PropsUpdateFinish locks an unset pipeline selection to false.
  EXPECT_FALSE(list_container_impl_->use_new_update_animation_.has_value());
  list_container_impl_->PropsUpdateFinish();
  ASSERT_TRUE(list_container_impl_->use_new_update_animation_.has_value());
  EXPECT_FALSE(list_container_impl_->use_new_update_animation());

  // 2. Setting the selection property after it is locked does not switch to
  // the new implementation.
  auto key =
      value_factory_->CreateString(kPropExperimentalUseNewUpdateAnimation);
  auto value = value_factory_->CreateBool(true);
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_FALSE(list_container_impl_->use_new_update_animation());
}

// Verify that the config is cached, then delivered in full by PropsUpdateFinish
// to AnimationManager exactly once.
TEST_F(ListContainerImplTest, ParsesAndAppliesNewUpdateAnimationConfig) {
  InstallMockAnimationManager();
  auto use_new_key =
      value_factory_->CreateString(kPropExperimentalUseNewUpdateAnimation);
  auto use_new_value = value_factory_->CreateBool(true);
  EXPECT_FALSE(
      list_container_impl_->ResolveAttribute(*use_new_key, *use_new_value));
  const std::vector<AnimationStageEntries> expected{
      {{ItemAnimationType::kAppearance, 80},
       {ItemAnimationType::kPersistence, 80}},
      {{ItemAnimationType::kDisappearance, 0},
       {ItemAnimationType::kChange, 33}},
  };

  // 1. ResolveAttribute only parses and caches the config; it must not apply
  // it.
  // The config is applied in PropsUpdateFinish, so parsing must not call
  // AnimationManager::SetUpdateAnimationConfig.
  EXPECT_CALL(*mock_animation_manager_, SetUpdateAnimationConfig(::testing::_))
      .Times(0);
  const auto parsed = ResolveAnimationConfig(R"({"enable":true,"stages":[
    {"animations":["add","move"],"durations":80.9},
    {"animations":["remove","change"],"durations":[-20,33.9]}
  ]})");
  // Check cached values: add/move share 80ms in stage one; remove/change use
  // 0ms and 33ms in stage two, verifying negative clamping and fraction
  // truncation.
  EXPECT_TRUE(parsed.enable);
  EXPECT_EQ(parsed.stages, expected);
  // Verify and clear Times(0) before checking that the config is applied once.
  ::testing::Mock::VerifyAndClearExpectations(mock_animation_manager_);

  // 2. Apply the complete config after parsing attributes and clear the pending
  // cache.
  EXPECT_CALL(*mock_animation_manager_, SetUpdateAnimationConfig(::testing::_))
      .WillOnce(::testing::Invoke([&](const UpdateAnimationConfig& config) {
        EXPECT_TRUE(config.enable);
        EXPECT_EQ(config.stages, expected);
      }));
  list_container_impl_->PropsUpdateFinish();
  EXPECT_FALSE(list_container_impl_->new_update_animation_config_.has_value());
  ::testing::Mock::VerifyAndClearExpectations(mock_animation_manager_);

  // 3. A subsequent update without new attributes must not reapply the old
  // config.
  EXPECT_CALL(*mock_animation_manager_, SetUpdateAnimationConfig(::testing::_))
      .Times(0);
  list_container_impl_->PropsUpdateFinish();
}

// Verify default stages when omitted and accept only boolean enable values.
TEST_F(ListContainerImplTest, DefaultsMissingStagesAndValidatesEnable) {
  EXPECT_EQ(ResolveAnimationConfig(R"({"enable":true})").stages,
            MakeDefaultAnimationStages());
  EXPECT_TRUE(ResolveAnimationConfig(R"({"enable":true})").enable);
  for (const char* json : {R"({})", R"({"enable":false})", R"({"enable":1})",
                           R"({"enable":"true"})"}) {
    SCOPED_TRACE(json);
    const auto config = ResolveAnimationConfig(json);
    EXPECT_FALSE(config.enable);
    EXPECT_EQ(config.stages, MakeDefaultAnimationStages());
  }
}

// Verify valid configs preserve stage order, type order, and corresponding
// durations.
TEST_F(ListContainerImplTest, ParsesValidAnimationStageConfigurations) {
  // 1. A scalar duration applies to all animations in the same stage.
  auto config = ResolveAnimationConfig(R"({"enable":true,"stages":[
    {"animations":["remove","move","add","change"],"durations":200}
  ]})");
  const std::vector<AnimationStageEntries> one_stage{
      {{ItemAnimationType::kDisappearance, 200},
       {ItemAnimationType::kPersistence, 200},
       {ItemAnimationType::kAppearance, 200},
       {ItemAnimationType::kChange, 200}},
  };
  EXPECT_TRUE(config.enable);
  EXPECT_EQ(config.stages, one_stage);

  // 2. Stages follow array order; move/change run concurrently with independent
  // durations.
  config = ResolveAnimationConfig(R"({"enable":true,"stages":[
    {"animations":["remove"],"durations":120},
    {"animations":["move","change"],"durations":[100,200]},
    {"animations":["add"],"durations":200}
  ]})");
  const std::vector<AnimationStageEntries> three_stages{
      {{ItemAnimationType::kDisappearance, 120}},
      {{ItemAnimationType::kPersistence, 100},
       {ItemAnimationType::kChange, 200}},
      {{ItemAnimationType::kAppearance, 200}},
  };
  EXPECT_TRUE(config.enable);
  EXPECT_EQ(config.stages, three_stages);

  // 3. Partial configs are allowed; omitted types are not added to the stages.
  config = ResolveAnimationConfig(R"({"enable":true,"stages":[
    {"animations":["add"],"durations":80}
  ]})");
  const std::vector<AnimationStageEntries> add_only{
      {{ItemAnimationType::kAppearance, 80}},
  };
  EXPECT_TRUE(config.enable);
  EXPECT_EQ(config.stages, add_only);

  // 4. Partial configs can span stages; durations match animation entries by
  // index.
  config = ResolveAnimationConfig(R"({"enable":true,"stages":[
    {"animations":["remove"],"durations":80},
    {"animations":["move","add"],"durations":[120,160]}
  ]})");
  const std::vector<AnimationStageEntries> partial_stages{
      {{ItemAnimationType::kDisappearance, 80}},
      {{ItemAnimationType::kPersistence, 120},
       {ItemAnimationType::kAppearance, 160}},
  };
  EXPECT_TRUE(config.enable);
  EXPECT_EQ(config.stages, partial_stages);

  // 5. Individual and cumulative stage durations up to INT32_MAX are valid.
  config = ResolveAnimationConfig(R"({"enable":true,"stages":[
    {"animations":["remove"],"durations":2147483644},
    {"animations":["move"],"durations":1},
    {"animations":["add"],"durations":2},
    {"animations":["change"],"durations":0}
  ]})");
  const std::vector<AnimationStageEntries> duration_boundary{
      {{ItemAnimationType::kDisappearance, 2147483644}},
      {{ItemAnimationType::kPersistence, 1}},
      {{ItemAnimationType::kAppearance, 2}},
      {{ItemAnimationType::kChange, 0}},
  };
  EXPECT_TRUE(config.enable);
  EXPECT_EQ(config.stages, duration_boundary);
}

// Verify invalid structure, types, duplicates, or durations fall back without
// partial results.
TEST_F(ListContainerImplTest, FallsBackForInvalidAnimationStages) {
  const char* invalid_stages[] = {
      "null",
      "{}",
      "[]",
      "[null]",
      R"([{}, {}, {}, {}, {}])",
      R"([{"animations":[],"durations":10}])",
      R"([{"animations":"add","durations":10}])",
      R"([{"animations":[1],"durations":10}])",
      R"([{"animations":["unknown"],"durations":10}])",
      R"([{"animations":["Add"],"durations":10}])",
      R"([{"animations":["add","add"],"durations":10}])",
      R"([{"animations":["add"],"durations":10},{"animations":["add"],"durations":20}])",
      R"([{"durations":10}])",
      R"([{"animations":["remove","move","add","change"]}])",
      R"([{"animations":["remove","move","add","change"],"durations":"10"}])",
      R"([{"animations":["remove","move","add","change"],"durations":true}])",
      R"([{"animations":["remove","move","add","change"],"durations":[10]}])",
      R"([{"animations":["remove","move","add","change"],"durations":[10,20,30,40,50]}])",
      R"([{"animations":["remove","move","add","change"],"durations":[10,20,null,40]}])",
      R"([{"animations":["remove","move","add","change"],"durations":2147483648}])",
      R"([{"animations":["remove"],"durations":2147483647},{"animations":["move","add","change"],"durations":1}])",
      R"([{"animations":["remove"],"durations":13},{"animations":["move","add","change"],"durations":[20,"bad",30]}])",
  };
  for (const char* stages : invalid_stages) {
    SCOPED_TRACE(stages);
    // Cache a custom config first to verify invalid input restores defaults
    // instead of retaining it.
    ResolveAnimationConfig(R"({"enable":true,"stages":[
      {"animations":["remove","move","add","change"],"durations":7}
    ]})");
    const std::string json =
        std::string(R"({"enable":true,"stages":)") + stages + "}";
    const auto config = ResolveAnimationConfig(json.c_str());
    EXPECT_TRUE(config.enable);
    EXPECT_EQ(config.stages, MakeDefaultAnimationStages());
  }
}

// Verifies that a Linear layout forwards the three animation-manager layout
// hooks in a fixed order.
TEST_F(ListContainerImplTest, ForwardsNewAnimationLayoutHooksInOrder) {
  InstallMockAnimationManager();
  list_container_impl_->use_new_update_animation_ = true;

  // 1. Even an empty List forwards BeforeLayout, AfterLayoutBeforeFlush, and
  // AfterFlush in that order.
  ::testing::InSequence sequence;
  EXPECT_CALL(*mock_animation_manager_, BeforeLayout());
  EXPECT_CALL(*mock_animation_manager_, AfterLayoutBeforeFlush());
  EXPECT_CALL(*mock_animation_manager_, AfterFlush());

  auto options = std::make_shared<tasm::PipelineOptions>();
  list_container_impl_->OnLayoutChildren(options);

  // 2. Completing the layout records initial-layout completion, one condition
  // used when deciding whether a later update can create a transaction.
  EXPECT_TRUE(list_container_impl_->has_completed_first_layout());
}

// Verifies that ListAdapter reports the actual diff state to AnimationManager
// before creating holders inserted by the current update.
TEST_F(ListContainerImplTest, NotifiesAnimationManagerBeforeCreatingNewHolder) {
  testing::RadonDataSource initial_data_source{
      .item_keys_ = {"A_0"},
      .insertion_ = {0},
      .estimated_main_axis_size_pxs_ = {100},
  };
  list_adapter_->UpdateRadonDataSource(pub::ValueImplLepus(
      lepus::Value(initial_data_source.GenerateDataSource())));
  list_adapter_->UpdateItemHolderToLatest(
      list_container_impl_->list_children_helper());
  list_adapter_->ClearDiffResult();
  ASSERT_NE(list_adapter_->GetItemHolderForIndex(0), nullptr);

  testing::RadonDataSource updated_data_source{
      .item_keys_ = {"A_0", "B_1"},
      .insertion_ = {1},
      .estimated_main_axis_size_pxs_ = {100, 100},
  };
  auto diff_result = list_adapter_->UpdateRadonDataSource(pub::ValueImplLepus(
      lepus::Value(updated_data_source.GenerateDataSource())));
  ASSERT_EQ(diff_result.first, ListAdapterDiffResult::kInsert);
  ASSERT_EQ(list_adapter_->GetItemHolderForIndex(1), nullptr);

  InstallMockAnimationManager();
  list_container_impl_->use_new_update_animation_ = true;
  list_container_impl_->has_completed_first_layout_ = true;

  // 1. ListAdapter derives the three BeforeDataUpdate arguments from an actual
  // partial-insertion diff.
  EXPECT_CALL(*mock_animation_manager_, BeforeDataUpdate(true, true, true))
      .WillOnce(::testing::Invoke([this](bool, bool, bool) {
        // 2. The inserted holder is not yet in the map when the callback runs,
        // allowing a real manager to snapshot the targets attached before the
        // update.
        EXPECT_EQ(list_adapter_->GetItemHolderForIndex(1), nullptr);
      }));
  list_adapter_->UpdateItemHolderToLatest(
      list_container_impl_->list_children_helper());

  // 3. ListAdapter creates the inserted holder only after BeforeDataUpdate
  // returns.
  EXPECT_NE(list_adapter_->GetItemHolderForIndex(1), nullptr);
}

// Verifies that Waterfall's separate LayoutManager forwards the three layout
// hooks in the same fixed order.
TEST_F(ListContainerImplTest, ForwardsWaterfallAnimationLayoutHooksInOrder) {
  auto key = value_factory_->CreateString(kPropListType);
  auto value = value_factory_->CreateString(kPropValueListTypeWaterFall);

  // 1. Switch to Waterfall so the subsequent layout uses
  // StaggeredGridLayoutManager.
  EXPECT_CALL(*mock_list_element_, MarkListElementLayoutDirty()).Times(1);
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_EQ(list_container_impl_->layout_type_, LayoutType::kWaterFall);

  InstallMockAnimationManager();
  list_container_impl_->use_new_update_animation_ = true;

  // 2. Waterfall also forwards BeforeLayout, AfterLayoutBeforeFlush, and
  // AfterFlush in that order.
  ::testing::InSequence sequence;
  EXPECT_CALL(*mock_animation_manager_, BeforeLayout());
  EXPECT_CALL(*mock_animation_manager_, AfterLayoutBeforeFlush());
  EXPECT_CALL(*mock_animation_manager_, AfterFlush());

  auto options = std::make_shared<tasm::PipelineOptions>();
  list_container_impl_->OnLayoutChildren(options);

  // 3. Completing a Waterfall layout also records initial-layout completion.
  EXPECT_TRUE(list_container_impl_->has_completed_first_layout());
}

// Verifies that a property affecting animation coordinates cancels the active
// transaction only when its value actually changes.
TEST_F(ListContainerImplTest, CancelsAnimationWhenOrientationChanges) {
  InstallMockAnimationManager();
  list_container_impl_->use_new_update_animation_ = true;

  auto key = value_factory_->CreateString(kPropScrollOrientation);
  auto value =
      value_factory_->CreateString(kPropValueScrollOrientationHorizontal);

  // 1. Switching from vertical to horizontal invalidates the current PRE and
  // POST coordinates, so the transaction is canceled.
  EXPECT_CALL(
      *mock_animation_manager_,
      CancelAnimationTransaction(AnimationCancelReason::kLayoutInvalidated))
      .Times(1);
  EXPECT_TRUE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_EQ(list_layout_manager_->orientation(), Orientation::kHorizontal);

  // 2. Setting the same orientation again does not issue another cancellation.
  EXPECT_TRUE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_EQ(list_layout_manager_->orientation(), Orientation::kHorizontal);
}

// Verifies that container destruction clears animation state before members
// such as ListAdapter are destroyed.
TEST_F(ListContainerImplTest, DestroysAnimationManagerBeforeContainerMembers) {
  InstallMockAnimationManager();
  ListContainerImpl* list_container = list_container_impl_.get();

  // 1. ListAdapter is still alive when Destroy runs, proving that animation
  // cleanup precedes destruction of the dependent members.
  EXPECT_CALL(*mock_animation_manager_, Destroy())
      .WillOnce(::testing::Invoke([list_container]() {
        EXPECT_NE(list_container->list_adapter(), nullptr);
      }));

  // 2. Destroy the container and verify that AnimationManager::Destroy is
  // called exactly once.
  list_container_impl_.reset();
  mock_animation_manager_ = nullptr;
}

TEST_LIST_CONTAINER_RESOLVE_PROP(ListType) {
  EXPECT_EQ(list_container_impl_->layout_type_, LayoutType::kSingle);
  EXPECT_CALL(*mock_list_element_, MarkListElementLayoutDirty()).Times(3);
  LIST_CONTAINER_DEFINE_PROP_VALUE(ListType, String, kPropValueListTypeSingle);
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_EQ(list_container_impl_->layout_type_, LayoutType::kSingle);
  value = value_factory_->CreateString(kPropValueListTypeFlow);
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_EQ(list_container_impl_->layout_type_, LayoutType::kFlow);
  value = value_factory_->CreateString(kPropValueListTypeWaterFall);
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_EQ(list_container_impl_->layout_type_, LayoutType::kWaterFall);
}

TEST_LIST_CONTAINER_RESOLVE_PROP(InitialScrollIndex) {
  EXPECT_EQ(list_container_impl_->initial_scroll_index_, kInvalidIndex);
  LIST_CONTAINER_DEFINE_PROP_VALUE(InitialScrollIndex, Number, 1);
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_EQ(list_container_impl_->initial_scroll_index_, 1);
}

TEST_LIST_CONTAINER_RESOLVE_PROP(UpperThresholdItemCount) {
  LIST_CONTAINER_DEFINE_PROP_VALUE(UpperThresholdItemCount, Number, 1);
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_EQ(list_event_manager_->upper_threshold_item_count_, 1);
}

TEST_LIST_CONTAINER_RESOLVE_PROP(LowerThresholdItemCount) {
  LIST_CONTAINER_DEFINE_PROP_VALUE(LowerThresholdItemCount, Number, 1);
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_EQ(list_event_manager_->lower_threshold_item_count_, 1);
}

TEST_LIST_CONTAINER_RESOLVE_PROP(NeedLayoutCompleteInfo) {
  LIST_CONTAINER_DEFINE_PROP_VALUE(NeedLayoutCompleteInfo, Bool, true);
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_TRUE(list_event_manager_->need_layout_complete_info_);
}

TEST_LIST_CONTAINER_RESOLVE_PROP(LayoutId) {
  EXPECT_EQ(list_container_impl_->layout_id_, kInvalidIndex);
  LIST_CONTAINER_DEFINE_PROP_VALUE(LayoutId, Number, 0);
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_EQ(list_container_impl_->layout_id_, 0);
}

TEST_LIST_CONTAINER_RESOLVE_PROP(ScrollEventThrottle) {
  LIST_CONTAINER_DEFINE_PROP_VALUE(ScrollEventThrottle, Number, 16);
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_EQ(list_event_manager_->scroll_event_throttle_ms_, 16);
}

TEST_LIST_CONTAINER_RESOLVE_PROP(NeedsVisibleCells) {
  LIST_CONTAINER_DEFINE_PROP_VALUE(NeedsVisibleCells, Bool, true);
  EXPECT_TRUE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_TRUE(list_event_manager_->need_visible_cell_);
}

TEST_LIST_CONTAINER_RESOLVE_PROP(NeedVisibleItemInfo) {
  LIST_CONTAINER_DEFINE_PROP_VALUE(NeedVisibleItemInfo, Bool, true);
  EXPECT_TRUE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_TRUE(list_event_manager_->need_visible_cell_);
}

TEST_LIST_CONTAINER_RESOLVE_PROP(ShouldRequestStateRestore) {
  EXPECT_FALSE(list_container_impl_->should_request_state_restore_);
  LIST_CONTAINER_DEFINE_PROP_VALUE(ShouldRequestStateRestore, Bool, true);
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_TRUE(list_container_impl_->should_request_state_restore_);
}

TEST_LIST_CONTAINER_RESOLVE_PROP(StickyOffset) {
  EXPECT_EQ(list_container_impl_->sticky_offset_, 0.f);
  LIST_CONTAINER_DEFINE_PROP_VALUE(StickyOffset, Number, 100.f);
  EXPECT_TRUE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_EQ(list_container_impl_->sticky_offset_, 100.f);
}

TEST_LIST_CONTAINER_RESOLVE_PROP(Sticky) {
  EXPECT_FALSE(list_container_impl_->sticky_enabled_);
  LIST_CONTAINER_DEFINE_PROP_VALUE(Sticky, Bool, true);
  EXPECT_TRUE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_TRUE(list_container_impl_->sticky_enabled_);
}

TEST_LIST_CONTAINER_RESOLVE_PROP(ExperimentalRecycleStickyItem) {
  EXPECT_TRUE(list_container_impl_->recycle_sticky_item_);
  LIST_CONTAINER_DEFINE_PROP_VALUE(ExperimentalRecycleStickyItem, Bool, false);
  EXPECT_TRUE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_FALSE(list_container_impl_->recycle_sticky_item_);
}

TEST_LIST_CONTAINER_RESOLVE_PROP(StickyBufferCount) {
  EXPECT_EQ(list_container_impl_->sticky_buffer_count_, kInvalidItemCount);
  LIST_CONTAINER_DEFINE_PROP_VALUE(StickyBufferCount, Number, 10);
  EXPECT_TRUE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_EQ(list_container_impl_->sticky_buffer_count_, 10);
}

TEST_LIST_CONTAINER_RESOLVE_PROP(EnablePreloadSection) {
  LIST_CONTAINER_DEFINE_PROP_VALUE(EnablePreloadSection, Bool, true);
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_TRUE(list_layout_manager_->enable_preload_section_);
}

TEST_LIST_CONTAINER_RESOLVE_PROP(PreloadBufferCount) {
  LIST_CONTAINER_DEFINE_PROP_VALUE(PreloadBufferCount, Number, 10);
  EXPECT_CALL(*mock_list_element_, MarkListElementLayoutDirty()).Times(1);
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_EQ(list_layout_manager_->preload_buffer_count_, 10);
}

TEST_LIST_CONTAINER_RESOLVE_PROP(EnableInsertPlatformViewOperation) {
  EXPECT_FALSE(list_container_impl_->enable_insert_platform_view_operation_);
  LIST_CONTAINER_DEFINE_PROP_VALUE(EnableInsertPlatformViewOperation, Bool,
                                   true);
  EXPECT_TRUE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_TRUE(list_container_impl_->enable_insert_platform_view_operation_);
}

TEST_LIST_CONTAINER_RESOLVE_PROP(ExperimentalBatchRenderStrategy) {
  LIST_CONTAINER_DEFINE_PROP_VALUE(
      ExperimentalBatchRenderStrategy, Number,
      static_cast<int>(
          BatchRenderStrategy::kAsyncResolvePropertyAndElementTree));
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, *value));
}

TEST_LIST_CONTAINER_RESOLVE_PROP(ListDebugInfoLevel) {
  LIST_CONTAINER_DEFINE_PROP_VALUE(
      ListDebugInfoLevel, Number,
      static_cast<int>(ListDebugInfoLevel::kListDebugInfoLevelInfo));
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_EQ(list_event_manager_->debug_info_level_,
            ListDebugInfoLevel::kListDebugInfoLevelInfo);
}

TEST_LIST_CONTAINER_RESOLVE_PROP(ExperimentalRecycleAvailableItemBeforeLayout) {
  EXPECT_FALSE(list_container_impl_->recycle_available_item_before_layout_);
  LIST_CONTAINER_DEFINE_PROP_VALUE(ExperimentalRecycleAvailableItemBeforeLayout,
                                   Bool, true);
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_TRUE(list_container_impl_->recycle_available_item_before_layout_);
}

TEST_LIST_CONTAINER_RESOLVE_PROP(ExperimentalSearchRefAnchorStrategy) {
  EXPECT_EQ(list_container_impl_->search_ref_anchor_strategy_,
            SearchRefAnchorStrategy::kNone);
  LIST_CONTAINER_DEFINE_PROP_VALUE(
      ExperimentalSearchRefAnchorStrategy, Number,
      static_cast<int>(SearchRefAnchorStrategy::kToEnd));
  EXPECT_FALSE(list_container_impl_->ResolveAttribute(*key, *value));
  EXPECT_EQ(list_container_impl_->search_ref_anchor_strategy_,
            SearchRefAnchorStrategy::kToEnd);
}

}  // namespace list
}  // namespace lynx
