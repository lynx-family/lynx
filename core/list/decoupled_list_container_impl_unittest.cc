// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/list/decoupled_list_container_impl.h"

#include "core/value_wrapper/value_impl_lepus.h"
#include "testing/fiber_data_source.h"
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

  void SetUp() override {
    value_factory_ = std::make_shared<pub::PubValueFactoryDefault>();
    mock_list_element_ = std::make_unique<MockListElement>();
    list_container_impl_ = std::make_unique<ListContainerImpl>(
        mock_list_element_.get(), value_factory_);
    list_layout_manager_ = list_container_impl_->list_layout_manager();
    list_adapter_ = list_container_impl_->list_adapter();
    list_event_manager_ = list_container_impl_->list_event_manager();
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
