
// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/list/decoupled_staggered_grid_layout_manager.h"

#include <algorithm>
#include <random>
#include <vector>

#include "base/include/string/string_utils.h"
#include "core/list/decoupled_list_container_impl.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "testing/fiber_data_source.h"
#include "testing/mock_list_element.h"
#include "testing/mock_list_item_element.h"
#include "testing/utils.h"

namespace lynx {
namespace list {

using ::testing::_;
using ::testing::Return;
using ::testing::UnorderedElementsAre;

using LayoutState = StaggeredGridLayoutManager::LayoutState;

class StaggeredGridLayoutManagerTest : public ::testing::Test {
 public:
  StaggeredGridLayoutManagerTest() = default;
  ~StaggeredGridLayoutManagerTest() override = default;

  std::unique_ptr<MockListElement> mock_list_element_{nullptr};
  std::unique_ptr<ListContainerImpl> list_container_impl_{nullptr};
  std::shared_ptr<pub::PubValueFactoryDefault> value_factory_{nullptr};
  ListLayoutManager* list_layout_manager_{nullptr};
  ListAdapter* list_adapter_{nullptr};

  class BaseLayoutResult {
   public:
    BaseLayoutResult(int data_count, int span_count)
        : data_count_(data_count), span_count_(span_count) {
      item_sizes_.resize(data_count, 0.f);
      span_indexes_.resize(span_count, {});
    }

    void ReLayout() {
      // Reset layout result.
      span_indexes_.assign(span_count_, {});
      content_size_ = 0.f;
      // Re-layout
      std::vector<float> span_sizes(span_count_, 0.f);
      for (int i = 0; i < data_count_; ++i) {
        int item_size = item_sizes_[i];
        if (IsItemFullSpan(i)) {
          float max_size =
              *std::max_element(span_sizes.begin(), span_sizes.end());
          for (int j = 0; j < span_count_; ++j) {
            span_sizes[j] = max_size + item_size;
            span_indexes_[j].push_back(i);
          }
        } else {
          auto min_size_it =
              std::min_element(span_sizes.begin(), span_sizes.end());
          int min_span_index = std::distance(span_sizes.begin(), min_size_it);
          span_sizes[min_span_index] += item_size;
          span_indexes_[min_span_index].push_back(i);
        }
      }
      content_size_ = *std::max_element(span_sizes.begin(), span_sizes.end());
    }

    bool IsItemFullSpan(int index) const {
      return full_span_indexes_.find(index) != full_span_indexes_.end();
    }

    int data_count_{0};
    int span_count_{0};
    float content_offset_{0.f};
    float content_size_{0.f};
    std::vector<int> item_sizes_;
    std::unordered_set<int> full_span_indexes_;
    std::vector<std::vector<int>> span_indexes_;
  };

  void SetUp() override {
    value_factory_ = std::make_shared<pub::PubValueFactoryDefault>();
    mock_list_element_ = std::make_unique<MockListElement>();
    list_container_impl_ = std::make_unique<ListContainerImpl>(
        mock_list_element_.get(), value_factory_);
    list_layout_manager_ = list_container_impl_->list_layout_manager();
    list_adapter_ = list_container_impl_->list_adapter();
  }

  StaggeredGridLayoutManager* GetStaggeredGridLayoutManager() {
    return static_cast<StaggeredGridLayoutManager*>(list_layout_manager_);
  }

  bool IsFullSpan(int index) {
    static int full_span_flag = 5;
    return !(index % full_span_flag);
  }

  ListOrientationHelper* GetOrientationHelper() {
    return list_layout_manager_->list_orientation_helper_.get();
  }

  void InitFiberDataSource(int data_count, std::vector<int> estimated_sizes,
                           bool mock_full_span) {
    testing::InsertAction insert_action;
    for (int i = 0; i < data_count; ++i) {
      insert_action.insert_ops_.push_back(
          {i, base::FormatString("list-item-%d", i), estimated_sizes[i],
           mock_full_span && IsFullSpan(i), false, false, true});
    }
    testing::FiberDataSource fiber_data_source{
        .insert_action_ = insert_action,
    };
    LIST_CONTAINER_DEFINE_PROP_LEPUS_VALUE(
        FiberUpdateListInfo, lepus::Value(fiber_data_source.Resolve()));
    list_container_impl_->ResolveAttribute(*key, value);
    list_container_impl_->PropsUpdateFinish();
  }

  void InitLayoutAttrs(std::string list_type, int span_count,
                       std::string scroll_orientation, float main_axis_gap,
                       float cross_axis_gap, float list_main_size,
                       float list_cross_size) {
    {
      LIST_CONTAINER_DEFINE_PROP_VALUE(ListType, String, list_type);
      list_container_impl_->ResolveAttribute(*key, *value);
      list_layout_manager_ = list_container_impl_->list_layout_manager();
    }
    {
      LIST_CONTAINER_DEFINE_PROP_VALUE(SpanCount, Number, span_count);
      list_container_impl_->ResolveAttribute(*key, *value);
    }
    {
      LIST_CONTAINER_DEFINE_PROP_VALUE(ScrollOrientation, String,
                                       scroll_orientation);
      list_container_impl_->ResolveAttribute(*key, *value);
    }
    list_container_impl_->ResolveListAxisGap(
        tasm::CSSPropertyID::kPropertyIDListMainAxisGap, main_axis_gap);
    list_container_impl_->ResolveListAxisGap(
        tasm::CSSPropertyID::kPropertyIDListCrossAxisGap, cross_axis_gap);
    if (scroll_orientation == kPropValueScrollOrientationVertical) {
      mock_list_element_->height_ = list_main_size;
      mock_list_element_->width_ = list_cross_size;
    } else {
      mock_list_element_->height_ = list_cross_size;
      mock_list_element_->width_ = list_main_size;
    }
    list_container_impl_->PropsUpdateFinish();
  }

  // Set up a 200 x 200 viewport with two columns. Use estimated_sizes before
  // binding and measure newly bound items synchronously at 100 x 100. Clear
  // the initial layout's binding requests before recording scroll requests.
  void InitOptimizedWaterfall(const std::vector<int>& estimated_sizes,
                              std::vector<uint32_t>& bind_requests) {
    InitLayoutAttrs("waterfall", 2, "vertical", 0.f, 0.f, 200.f, 200.f);
    LIST_CONTAINER_DEFINE_PROP_VALUE(ExperimentalOptimizeWaterfallScrollFill,
                                     Bool, true);
    list_container_impl_->ResolveAttribute(*key, *value);
    InitFiberDataSource(static_cast<int>(estimated_sizes.size()),
                        estimated_sizes, false);
    EXPECT_CALL(*mock_list_element_, ComponentAtIndex(_, _, _))
        .WillRepeatedly([this, &bind_requests](uint32_t index,
                                               int64_t operation_id, bool) {
          bind_requests.push_back(index);
          const auto& item_key =
              list_container_impl_->GetItemHolderForIndex(index)->item_key();
          if (!mock_list_element_->GetListItemElement(item_key)) {
            auto item = std::make_unique<MockListItemElement>(100 + index);
            item->width_ = 100.f;
            item->height_ = 100.f;
            mock_list_element_->AddListItemElement(item_key, std::move(item));
          }
          auto options = std::make_shared<tasm::PipelineOptions>();
          options->operation_id = operation_id;
          options->has_layout = true;
          list_container_impl_->FinishBindItemHolder(
              mock_list_element_->GetListItemElement(item_key), options);
          return true;
        });
    list_container_impl_->OnLayoutChildren(
        std::make_shared<tasm::PipelineOptions>());
    bind_requests.clear();
  }

  BaseLayoutResult GenerateBaseLayoutResult(int data_count, int span_count,
                                            int min_item_size,
                                            int max_item_size,
                                            bool mock_full_span = false) {
    BaseLayoutResult base_layout_result(data_count, span_count);
    auto& item_sizes = base_layout_result.item_sizes_;
    auto& span_indexes = base_layout_result.span_indexes_;
    auto& full_span_indexes = base_layout_result.full_span_indexes_;
    std::vector<float> span_sizes(span_count, 0.f);
    std::mt19937 gen(0);
    std::uniform_int_distribution<> dis(min_item_size, max_item_size);
    for (int i = 0; i < data_count; ++i) {
      bool is_full_span = mock_full_span && IsFullSpan(i);
      int item_size = dis(gen);
      if (is_full_span) {
        float max_size =
            *std::max_element(span_sizes.begin(), span_sizes.end());
        for (int j = 0; j < span_count; ++j) {
          span_sizes[j] = max_size + item_size;
          span_indexes[j].push_back(i);
        }
        full_span_indexes.insert(i);
      } else {
        auto min_size_it =
            std::min_element(span_sizes.begin(), span_sizes.end());
        int min_span_index = std::distance(span_sizes.begin(), min_size_it);
        span_sizes[min_span_index] += item_size;
        span_indexes[min_span_index].push_back(i);
      }
      item_sizes[i] = item_size;
    }
    base_layout_result.content_size_ =
        *std::max_element(span_sizes.begin(), span_sizes.end());
    return base_layout_result;
  }
};

TEST_F(StaggeredGridLayoutManagerTest, LayoutInvalidItemHolder) {
  int data_count = 1000;
  int span_count = 3;
  float min_item_size = 50;
  float max_item_size = 100;
  bool mock_full_span = true;
  BaseLayoutResult base_layout_result = GenerateBaseLayoutResult(
      data_count, span_count, min_item_size, max_item_size, mock_full_span);
  InitLayoutAttrs("waterfall", span_count, "vertical", 0.f, 0.f, 2000.f,
                  1000.f);
  InitFiberDataSource(data_count, base_layout_result.item_sizes_,
                      mock_full_span);

  auto* waterfall_layout_manager = GetStaggeredGridLayoutManager();
  waterfall_layout_manager->LayoutInvalidItemHolder(0);
  EXPECT_TRUE(
      base::FloatsEqual(waterfall_layout_manager->GetTargetContentSize(),
                        base_layout_result.content_size_));
  for (int i = 0; i < span_count; ++i) {
    auto& basic_span_index = base_layout_result.span_indexes_[i];
    auto& test_span_index = waterfall_layout_manager->column_indexes_[i];
    EXPECT_EQ(test_span_index.size(), basic_span_index.size());
    for (int j = 0; j < static_cast<int>(basic_span_index.size()); ++j) {
      EXPECT_EQ(test_span_index[j], basic_span_index[j]);
    }
  }
}

// The viewport covers [50, 150). Prefer bound item 1 at screen offset -50.
// When it is pinned or removed, use unbound item 2 at screen offset +50.
//
// Content y  Column 0 Column 1   Layout before pinning
//       0    +--------+--------+  Item 1 starts at screen y = -50
//            |   0    |   1    |
//      50    |........|........|  Viewport top
//     100    |   0    +--------+  Item 2 starts at screen y = +50
//            |   0    |   2    |
//     150    +--------+........|  Viewport bottom (item 3 starts in column 0)
TEST_F(StaggeredGridLayoutManagerTest,
       ScrollAnchorPrefersBoundItemAndSkipsPinnedOrRemovedItems) {
  InitLayoutAttrs("waterfall", 2, "vertical", 0.f, 0.f, 100.f, 200.f);
  LIST_CONTAINER_DEFINE_PROP_VALUE(Sticky, Bool, true);
  list_container_impl_->ResolveAttribute(*key, *value);
  InitFiberDataSource(4, {150, 100, 100, 100}, false);
  auto* manager = GetStaggeredGridLayoutManager();
  manager->LayoutInvalidItemHolder(0);
  manager->ResetContentOffsetAndContentSize(50.f,
                                            manager->GetTargetContentSize());
  list_container_impl_->list_children_helper()->UpdateOnScreenChildren(
      GetOrientationHelper(), manager->content_offset_);

  auto* bound_item = list_container_impl_->GetItemHolderForIndex(1);
  mock_list_element_->AddListItemElement(
      bound_item->item_key(), std::make_unique<MockListItemElement>(2));
  bound_item->SetItemDelegate(
      mock_list_element_->GetListItemElement(bound_item->item_key()));
  bound_item->MarkDirty(false);
  ListAnchorManager::AnchorInfo anchor;
  manager->UpdateScrollAnchorInfo(anchor);
  ASSERT_TRUE(anchor.valid_);
  EXPECT_EQ(anchor.item_holder_, bound_item);
  EXPECT_FLOAT_EQ(anchor.start_alignment_delta_, -50.f);

  auto* visible_item = list_container_impl_->GetItemHolderForIndex(2);
  bound_item->SetSticky(true, false);
  manager->UpdateScrollAnchorInfo(anchor);
  ASSERT_TRUE(anchor.valid_);
  EXPECT_EQ(anchor.item_holder_, visible_item);
  EXPECT_FLOAT_EQ(anchor.start_alignment_delta_, 50.f);

  bound_item->SetSticky(false, false);
  bound_item->MarkRemoved(true);
  manager->UpdateScrollAnchorInfo(anchor);
  ASSERT_TRUE(anchor.valid_);
  EXPECT_EQ(anchor.item_holder_, visible_item);
  EXPECT_FLOAT_EQ(anchor.start_alignment_delta_, 50.f);
}

// With no item start inside [200, 300), fall back to the unbound item covering
// the viewport. Its screen offset is -200. Removing it invalidates the anchor
// and clears the holder and index in the reused AnchorInfo.
//
// Content y  Column 0 Column 1
//       0    +--------+           Item 0 starts
//            |   0    |  (empty)
//     200    |........|.........  Viewport top
//            |   0    |
//     300    |........|.........  Viewport bottom
//            |   0    |
//     500    +--------+
TEST_F(StaggeredGridLayoutManagerTest, ScrollAnchorFallsBackToSingleTallItem) {
  InitLayoutAttrs("waterfall", 2, "vertical", 0.f, 0.f, 100.f, 200.f);
  InitFiberDataSource(1, {500}, false);
  auto* manager = GetStaggeredGridLayoutManager();
  manager->LayoutInvalidItemHolder(0);
  manager->ResetContentOffsetAndContentSize(200.f,
                                            manager->GetTargetContentSize());
  list_container_impl_->list_children_helper()->UpdateOnScreenChildren(
      GetOrientationHelper(), manager->content_offset_);

  ListAnchorManager::AnchorInfo anchor;
  manager->UpdateScrollAnchorInfo(anchor);
  auto* tall_item = list_container_impl_->GetItemHolderForIndex(0);
  ASSERT_TRUE(anchor.valid_);
  EXPECT_EQ(anchor.item_holder_, tall_item);
  EXPECT_EQ(anchor.index_, 0);
  EXPECT_FLOAT_EQ(anchor.start_offset_, 0.f);
  EXPECT_FLOAT_EQ(anchor.start_alignment_delta_, -200.f);

  tall_item->MarkRemoved(true);
  manager->UpdateScrollAnchorInfo(anchor);
  EXPECT_FALSE(anchor.valid_);
  EXPECT_EQ(anchor.item_holder_, nullptr);
  EXPECT_EQ(anchor.index_, kInvalidIndex);
}

TEST_F(StaggeredGridLayoutManagerTest, LayoutInvalidItemHolder1) {
  int data_count = 1000;
  int span_count = 3;
  float min_item_size = 50;
  float max_item_size = 100;
  bool mock_full_span = true;
  BaseLayoutResult base_layout_result = GenerateBaseLayoutResult(
      data_count, span_count, min_item_size, max_item_size, mock_full_span);
  InitLayoutAttrs("waterfall", span_count, "vertical", 0.f, 0.f, 2000.f,
                  1000.f);
  InitFiberDataSource(data_count, base_layout_result.item_sizes_,
                      mock_full_span);

  auto* waterfall_layout_manager = GetStaggeredGridLayoutManager();
  waterfall_layout_manager->LayoutInvalidItemHolder(0);

  // Change item size of index 4 and 9.
  list_container_impl_->GetItemHolderForIndex(4)->SetEstimatedSize(200);
  list_container_impl_->GetItemHolderForIndex(9)->SetEstimatedSize(500);
  base_layout_result.item_sizes_[4] = 200;
  base_layout_result.item_sizes_[9] = 500;
  base_layout_result.ReLayout();
  // Layout from index 4.
  waterfall_layout_manager->LayoutInvalidItemHolder(4);
  EXPECT_TRUE(
      base::FloatsEqual(waterfall_layout_manager->GetTargetContentSize(),
                        base_layout_result.content_size_));

  // Change item size of index 5 and 15 (full span).
  list_container_impl_->GetItemHolderForIndex(5)->SetEstimatedSize(300);
  list_container_impl_->GetItemHolderForIndex(15)->SetEstimatedSize(300);
  base_layout_result.item_sizes_[5] = 300;
  base_layout_result.item_sizes_[15] = 300;
  base_layout_result.ReLayout();
  // Layout from 7.
  waterfall_layout_manager->LayoutInvalidItemHolder(5);
  EXPECT_TRUE(
      base::FloatsEqual(waterfall_layout_manager->GetTargetContentSize(),
                        base_layout_result.content_size_));
  // Check span index
  for (int i = 0; i < span_count; ++i) {
    auto& basic_span_index = base_layout_result.span_indexes_[i];
    auto& test_span_index = waterfall_layout_manager->column_indexes_[i];
    EXPECT_EQ(test_span_index.size(), basic_span_index.size());
    for (int j = 0; j < static_cast<int>(basic_span_index.size()); ++j) {
      EXPECT_EQ(test_span_index[j], basic_span_index[j]);
    }
  }
}

TEST_F(StaggeredGridLayoutManagerTest, HasRemainSpaceToFillEnd) {
  int data_count = 100;
  int span_count = 3;
  float list_main_size = 2000.f;
  float list_cross_size = 1000.f;
  float content_offset = 1000.f;
  float content_size = 10000.f;
  bool mock_full_span = true;
  InitLayoutAttrs("waterfall", span_count, "vertical", 0.f, 0.f, list_main_size,
                  list_cross_size);
  InitFiberDataSource(data_count, std::vector<int>(data_count, 100),
                      mock_full_span);
  auto* waterfall_layout_manager = GetStaggeredGridLayoutManager();
  waterfall_layout_manager->content_offset_ = content_offset;
  waterfall_layout_manager->content_size_ = content_size;

  // case 1. current end line is full span
  // end_lines: 2999, 2999, 2999
  // end_indexes: 10, 10, 10
  LayoutState layout_state(span_count, LayoutDirection::kLayoutToEnd);
  auto& end_lines = layout_state.end_lines;
  auto& end_indexes = layout_state.end_index;
  layout_state.is_end_full_span_ = true;
  std::fill(end_lines.begin(), end_lines.end(),
            list_main_size + content_offset - 1.f);
  std::fill(end_indexes.begin(), end_indexes.end(), 10);
  EXPECT_TRUE(
      waterfall_layout_manager->HasRemainSpaceToFillEnd(0, layout_state));
  layout_state.Reset(span_count);
  layout_state.is_end_full_span_ = true;
  // end_lines: 3001, 3001, 3001
  // end_indexes: 10, 10, 10
  std::fill(end_lines.begin(), end_lines.end(),
            list_main_size + content_offset + 1.f);
  std::fill(end_indexes.begin(), end_indexes.end(), 10);
  EXPECT_FALSE(
      waterfall_layout_manager->HasRemainSpaceToFillEnd(0, layout_state));

  // case 2. current end line is not full span but next item is full span.
  int next_bind_item_index = 10;
  list_adapter_->adapter_helper_->full_spans_.insert(next_bind_item_index);
  list_container_impl_->GetItemHolderForIndex(next_bind_item_index)
      ->item_full_span_ = true;
  // end_lines: 2950, 3000, 3050
  // end_indexes: 7, 8, 9
  layout_state.Reset(span_count);
  for (int i = 0; i < span_count; ++i) {
    end_lines[i] = list_main_size + content_offset + (i - 1) * 50.f;
    end_indexes[i] = next_bind_item_index - span_count + i;
  }
  EXPECT_FALSE(waterfall_layout_manager->HasRemainSpaceToFillEnd(
      next_bind_item_index, layout_state));
  // end_lines: 2850, 2900, 2950
  // end_indexes: 7, 8, 9
  layout_state.Reset(span_count);
  for (int i = 0; i < span_count; ++i) {
    end_lines[i] = list_main_size + content_offset - (i + 1) * 50.f;
    end_indexes[i] = next_bind_item_index - span_count + i;
  }
  EXPECT_TRUE(waterfall_layout_manager->HasRemainSpaceToFillEnd(
      next_bind_item_index, layout_state));

  // case 3. current end line is not full span and next item is not full span.
  list_adapter_->adapter_helper_->full_spans_.erase(next_bind_item_index);
  list_container_impl_->GetItemHolderForIndex(next_bind_item_index)
      ->item_full_span_ = false;
  // end_lines: 2950, 3000, 3050
  // end_indexes: 7, 8, 9
  layout_state.Reset(span_count);
  for (int i = 0; i < span_count; ++i) {
    end_lines[i] = list_main_size + content_offset + (i - 1) * 50.f;
    end_indexes[i] = next_bind_item_index - span_count + i;
  }
  EXPECT_TRUE(waterfall_layout_manager->HasRemainSpaceToFillEnd(
      next_bind_item_index, layout_state));
  // end_lines: 2850, 2900, 2950
  // end_indexes: 7, 8, 9
  layout_state.Reset(span_count);
  for (int i = 0; i < span_count; ++i) {
    // 2850, 2900, 2950
    end_lines[i] = list_main_size + content_offset - (i + 1) * 50.f;
    end_indexes[i] = next_bind_item_index - span_count + i;
  }
  EXPECT_TRUE(waterfall_layout_manager->HasRemainSpaceToFillEnd(
      next_bind_item_index, layout_state));
  // end_lines: 3050, 3100, 3150
  // end_indexes: 7, 8, 9
  layout_state.Reset(span_count);
  for (int i = 0; i < span_count; ++i) {
    end_lines[i] = list_main_size + content_offset + (i + 1) * 50.f;
    end_indexes[i] = next_bind_item_index - span_count + i;
  }
  EXPECT_FALSE(waterfall_layout_manager->HasRemainSpaceToFillEnd(
      next_bind_item_index, layout_state));
}

TEST_F(StaggeredGridLayoutManagerTest, HasRemainSpaceToFillStart) {
  int data_count = 100;
  int span_count = 3;
  float list_main_size = 2000.f;
  float list_cross_size = 1000.f;
  float content_offset = 1000.f;
  float content_size = 10000.f;
  bool mock_full_span = true;
  InitLayoutAttrs("waterfall", span_count, "vertical", 0.f, 0.f, list_main_size,
                  list_cross_size);
  InitFiberDataSource(data_count, std::vector<int>(data_count, 100),
                      mock_full_span);
  auto* waterfall_layout_manager = GetStaggeredGridLayoutManager();
  waterfall_layout_manager->content_offset_ = content_offset;
  waterfall_layout_manager->content_size_ = content_size;

  // case 1. current start line is full span
  LayoutState layout_state(span_count, LayoutDirection::kLayoutToStart);
  auto& start_lines = layout_state.start_lines;
  auto& start_indexes = layout_state.start_index;
  // start_lines: 999, 999, 999
  // start_indexes: 10, 10, 10
  layout_state.is_start_full_span_ = true;
  std::fill(start_lines.begin(), start_lines.end(), content_offset - 1.f);
  std::fill(start_indexes.begin(), start_indexes.end(), 10);
  EXPECT_FALSE(
      waterfall_layout_manager->HasRemainSpaceToFillStart(layout_state));
  // start_lines: 1001, 1001, 1001
  // start_indexes: 10, 10, 10
  layout_state.Reset(span_count);
  layout_state.is_start_full_span_ = true;
  std::fill(start_lines.begin(), start_lines.end(), content_offset + 1.f);
  std::fill(start_indexes.begin(), start_indexes.end(), 10);
  EXPECT_TRUE(
      waterfall_layout_manager->HasRemainSpaceToFillStart(layout_state));

  // case 2. current lines has no full span
  // start_lines: 950, 1000, 1050
  // start_indexes: 11, 12, 13
  int next_bind_item_index = 10;
  layout_state.Reset(span_count);
  for (int i = 0; i < span_count; ++i) {
    start_lines[i] = content_offset + (i - 1) * 50.f;
    start_indexes[i] = next_bind_item_index + i + 1;
  }
  EXPECT_TRUE(
      waterfall_layout_manager->HasRemainSpaceToFillStart(layout_state));
  // start_lines: 850, 900, 950
  // start_indexes: 11, 12, 13
  layout_state.Reset(span_count);
  for (int i = 0; i < span_count; ++i) {
    start_lines[i] = content_offset - i * 50.f;
    start_indexes[i] = next_bind_item_index + i + 1;
  }
  EXPECT_FALSE(
      waterfall_layout_manager->HasRemainSpaceToFillStart(layout_state));
}

TEST_F(StaggeredGridLayoutManagerTest, GetItemIndexBeforeTargetIndex) {
  int data_count = 1000;
  int span_count = 3;
  float min_item_size = 50;
  float max_item_size = 100;
  float list_main_size = 2000.f;
  float list_cross_size = 1000.f;
  bool mock_full_span = true;
  float content_offset = 1000.f;
  BaseLayoutResult base_layout_result = GenerateBaseLayoutResult(
      data_count, span_count, min_item_size, max_item_size, mock_full_span);
  InitLayoutAttrs("waterfall", span_count, "vertical", 0.f, 0.f, list_main_size,
                  list_cross_size);
  InitFiberDataSource(data_count, base_layout_result.item_sizes_,
                      mock_full_span);

  auto* waterfall_layout_manager = GetStaggeredGridLayoutManager();
  waterfall_layout_manager->LayoutInvalidItemHolder(0);
  waterfall_layout_manager->content_offset_ = content_offset;
  waterfall_layout_manager->content_size_ =
      waterfall_layout_manager->GetTargetContentSize();
  for (auto& span_indexes : waterfall_layout_manager->column_indexes_) {
    for (int i = 0; i < static_cast<int>(span_indexes.size()); ++i) {
      int target_index = span_indexes[i];
      int result_index =
          waterfall_layout_manager->GetItemIndexBeforeTargetIndex(span_indexes,
                                                                  target_index);
      if (target_index == 0 || i == 0) {
        EXPECT_EQ(result_index, kInvalidIndex);
      } else {
        ItemHolder* next_item_holder =
            list_container_impl_->GetItemHolderForIndex(span_indexes[i - 1]);
        EXPECT_TRUE(next_item_holder != nullptr);
        if (base::FloatsLargerOrEqual(
                GetOrientationHelper()->GetDecoratedEnd(next_item_holder),
                content_offset)) {
          EXPECT_EQ(result_index, next_item_holder->index());
        } else {
          EXPECT_EQ(result_index, kInvalidIndex);
        }
      }
    }
  }
}

// With two columns, 100 items of height 100, and a viewport height of 200,
// jump from offset 0 to 4000 and back. Each jump requests bindings only for
// its destination. The return to offset 0 also recycles items 80 through 83.
//
// Content y  Column 0 Column 1
//       0       0        1        [0, 200) is visible after jumping back
//     100       2        3
//     200          ...            Items 4 through 79 are skipped
//    4000      80       81        [4000, 4200) is visible after jumping down
//    4100      82       83
TEST_F(StaggeredGridLayoutManagerTest, LargeScrollBindsOnlyDestinationItems) {
  std::vector<uint32_t> bind_requests;
  InitOptimizedWaterfall(std::vector<int>(100, 100), bind_requests);

  // Jump to row 40 (zero-based). The viewport covers rows 40 and 41.
  list_container_impl_->ScrollByPlatformContainer(0.f, 4000.f, 0.f, 4000.f);
  ASSERT_THAT(bind_requests, UnorderedElementsAre(80, 81, 82, 83));
  bind_requests.clear();

  // Jump back to the top. Count all binding requests so intermediate items
  // are detected even if they are recycled before the scroll finishes.
  list_container_impl_->ScrollByPlatformContainer(0.f, 0.f, 0.f, 0.f);
  EXPECT_THAT(bind_requests, UnorderedElementsAre(0, 1, 2, 3));
  EXPECT_FLOAT_EQ(list_layout_manager_->content_offset(), 0.f);
  for (int index = 80; index <= 83; ++index) {
    EXPECT_TRUE(list_adapter_->IsRecycled(
        list_container_impl_->GetItemHolderForIndex(index)))
        << index;
  }
}

// With platform view insertion enabled, scroll to [200, 400).
//
// 1. The first binding pass uses estimated heights. Each box shows an item
//    index. Items 8 and 9 are partially visible, so both bind.
//           Col 0   Col 1
// y=200   +-------+-------+  Viewport top
//         |   4   |   5   |  Estimated height of 50
// y=250   +-------+-------+
//         |   6   |   7   |
//         |       |       |
// y=350   +-------+-------+  Items 8 and 9 start here
//         |   8   |   9   |
// y=400   |.......|.......|  Viewport bottom crosses both items
//         |       |       |
// y=450   +-------+-------+
//
// 2. Items 4 and 5 measure 100. The row grows by 50 and moves each later
//    row down by 50. Items 8 and 9 remain unrecycled when the scroll returns.
//           Col 0   Col 1
// y=200   +-------+-------+  Viewport top stays at 200
//         |   4   |   5   |
//         |       |       |
// y=300   +-------+-------+
//         |   6   |   7   |
//         |       |       |
// y=400   +-------+-------+  Viewport bottom meets the top of items 8 and 9
//         |   8   |   9   |  Both items are fully offscreen
//         |       |       |
// y=500   +-------+-------+
TEST_F(StaggeredGridLayoutManagerTest,
       InsertPlatformViewDoesNotRecycleNewlyOffscreenItems) {
  list_container_impl_->EnableInsertPlatformView();
  std::vector<int> estimated_sizes(10, 100);
  estimated_sizes[4] = 50;
  estimated_sizes[5] = 50;
  std::vector<uint32_t> bind_requests;
  InitOptimizedWaterfall(estimated_sizes, bind_requests);

  list_container_impl_->ScrollByPlatformContainer(0.f, 200.f, 0.f, 200.f);

  ASSERT_FLOAT_EQ(list_layout_manager_->content_offset(), 200.f);
  for (uint32_t index : {8u, 9u}) {
    SCOPED_TRACE(index);
    ASSERT_THAT(bind_requests, ::testing::Contains(index));
    auto* holder = list_container_impl_->GetItemHolderForIndex(index);
    ASSERT_NE(holder, nullptr);
    ASSERT_FALSE(list_layout_manager_->ItemHolderVisibleInList(holder));
    // TODO: Expect recycling once the scroll-result handler also visits
    // items that were bound during this scroll and are now offscreen.
    EXPECT_FALSE(list_adapter_->IsRecycled(holder));
  }
}

// Start with the target viewport [750, 950). Items 4 through 7 have estimated
// heights of 600, 650, 600, and 600. Each measures 100 after binding.
// Item 6 starts at content y = 800, so keep its screen offset of +50.
// After relayout it starts at y = 300, and the viewport moves to [250, 450).
// Items 8 and 9 become visible, so the scroll binds only items 4 through 9.
//
// Screen y  Before binding           After relayout
//           Viewport at 750          Viewport at 250
//           Column 0 Column 1        Column 0 Column 1
//       0   |...4....|...5....|      |...4....|...5....|  Viewport top
//     +50   +--------+   5    |      +--------+--------+  Item 6 starts
//           |   6    |   5    |      |   6    |   7    |
//    +100   |   6    +--------+      |   6    |   7    |
//           |   6    |   7    |      |   6    |   7    |
//    +150   |   6    |   7    |      +--------+--------+  Items 8 and 9 enter
//           |   6    |   7    |      |   8    |   9    |
//    +200   |........|........|      |........|........|  Viewport bottom
TEST_F(StaggeredGridLayoutManagerTest, ShrinkingItemsPreserveVisibleAnchor) {
  std::vector<int> estimated_sizes(30, 100);
  // Items 4 and 5 both start at 200. Their different estimates put items 6
  // and 7 at 800 and 850, respectively.
  std::fill(estimated_sizes.begin() + 4, estimated_sizes.begin() + 8, 600);
  estimated_sizes[5] = 650;
  std::vector<uint32_t> bind_requests;
  InitOptimizedWaterfall(estimated_sizes, bind_requests);
  auto* anchor = list_container_impl_->GetItemHolderForIndex(6);
  ASSERT_FLOAT_EQ(GetOrientationHelper()->GetStart(anchor), 800.f);

  list_container_impl_->ScrollByPlatformContainer(0.f, 750.f, 0.f, 750.f);

  // After binding items 4 through 7, the viewport [250, 450) intersects
  // items 8 and 9, which both start at 400.
  EXPECT_FLOAT_EQ(GetOrientationHelper()->GetStart(anchor), 300.f);
  EXPECT_FLOAT_EQ(list_layout_manager_->content_offset(), 250.f);
  EXPECT_THAT(bind_requests, UnorderedElementsAre(4, 5, 6, 7, 8, 9));
}

// Scroll to 100 to recycle the first row. Change the measured heights of items
// 0 and 1 from 100 to 150, then jump to offset 0. Only those two items rebind.
// The second row moves from y = 100 to y = 150 and the offset stays at zero.
//
// Content y  Before rebinding         After rebinding
//       0    +--------+--------+      +--------+--------+  Viewport top
//            |   0    |   1    |      |   0    |   1    |
//     100    +--------+--------+      |   0    |   1    |
//            |   2    |   3    |      |   0    |   1    |
//     150    |   2    |   3    |      +--------+--------+  Second row starts
//            |   2    |   3    |      |   2    |   3    |
//     200    +--------+--------+      |........|........|  Viewport bottom
TEST_F(StaggeredGridLayoutManagerTest, ScrollToTopIgnoresAnchorCompensation) {
  std::vector<uint32_t> bind_requests;
  InitOptimizedWaterfall(std::vector<int>(10, 100), bind_requests);
  list_container_impl_->ScrollByPlatformContainer(0.f, 100.f, 0.f, 100.f);
  for (int index : {0, 1}) {
    auto* holder = list_container_impl_->GetItemHolderForIndex(index);
    ASSERT_TRUE(list_adapter_->IsRecycled(holder));
    auto* item = mock_list_element_->GetListItemElement(holder->item_key());
    ASSERT_NE(item, nullptr);
    item->height_ = 150.f;
  }
  bind_requests.clear();

  list_container_impl_->ScrollByPlatformContainer(0.f, 0.f, 0.f, 0.f);

  EXPECT_THAT(bind_requests, UnorderedElementsAre(0, 1));
  EXPECT_FLOAT_EQ(GetOrientationHelper()->GetStart(
                      list_container_impl_->GetItemHolderForIndex(2)),
                  150.f);
  EXPECT_FLOAT_EQ(list_layout_manager_->content_offset(), 0.f);
}

}  // namespace list
}  // namespace lynx
