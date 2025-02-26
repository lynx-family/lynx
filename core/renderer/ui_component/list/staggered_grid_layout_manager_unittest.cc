// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/ui_component/list/staggered_grid_layout_manager.h"

#include <algorithm>
#include <memory>
#include <random>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/fml/memory/ref_ptr.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/component_element.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/renderer/ui_component/list/default_list_adapter.h"
#include "core/renderer/ui_component/list/list_container_impl.h"
#include "core/renderer/ui_component/list/testing/mock_diff_result.h"
#include "core/renderer/ui_component/list/testing/mock_list_element.h"
#include "core/renderer/ui_component/list/testing/utils.h"
#include "core/shell/tasm_operation_queue.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

using LayoutState = StaggeredGridLayoutManager::LayoutState;

class StaggeredGridLayoutManagerTest : public ::testing::Test {
 public:
  class BaseLayoutResult {
   public:
    BaseLayoutResult(int data_count, int span_count)
        : data_count_(data_count), span_count_(span_count) {
      item_sizes_.resize(data_count, 0.f);
      span_indexes_.resize(span_count, {});
    }

    int data_count_{0};
    int span_count_{0};
    float content_offset_{0.f};
    float content_size_{0.f};
    std::vector<int> item_sizes_;
    std::vector<std::vector<int>> span_indexes_;
  };

 public:
  std::unique_ptr<lynx::tasm::ElementManager> element_manager_;
  std::shared_ptr<::testing::NiceMock<test::MockTasmDelegate>> tasm_mediator_;
  fml::RefPtr<list::MockListElement> list_element_ref_;
  std::unique_ptr<ListContainerImpl> list_container_;
  ListAdapter* list_adapter_;
  ListChildrenHelper* list_children_helper_;
  int full_span_tag_{7};

 protected:
  StaggeredGridLayoutManagerTest() = default;
  ~StaggeredGridLayoutManagerTest() override = default;

  void SetUp() override {
    LynxEnvConfig lynx_env_config(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator_ = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    element_manager_ = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<MockPaintingContext>(), tasm_mediator_.get(),
        lynx_env_config);
    auto config = std::make_shared<PageConfig>();
    config->SetEnableFiberArch(true);
    element_manager_->SetConfig(config);

    lepus::Value component_at_index;
    lepus::Value enqueue_component;
    lepus::Value component_at_indexes;
    list_element_ref_ =
        fml::AdoptRef<list::MockListElement>(new list::MockListElement(
            element_manager_.get(), "list", component_at_index,
            enqueue_component, component_at_indexes));
    list_container_ =
        std::make_unique<ListContainerImpl>(list_element_ref_.get());
    list_adapter_ = list_container_->list_adapter();
    list_children_helper_ = list_container_->list_children_helper();
  }

  ListLayoutManager* list_layout_manager() {
    return list_container_->list_layout_manager();
  }

  ListOrientationHelper* list_orientation_helper() {
    return list_layout_manager()->list_orientation_helper_.get();
  }

  StaggeredGridLayoutManager* staggered_grid_layout_manager() {
    return static_cast<StaggeredGridLayoutManager*>(list_layout_manager());
  }

  void InitLayoutAttrs(std::string list_type, int span_count,
                       std::string scroll_orientation, float main_axis_gap,
                       float cross_axis_gap, float list_main_size,
                       float list_cross_size) {
    list_container_->ResolveAttribute(list::kListType, lepus::Value(list_type));
    list_container_->ResolveAttribute(list::kSpanCount,
                                      lepus::Value(span_count));
    list_container_->ResolveAttribute(list::kScrollOrientation,
                                      lepus::Value(scroll_orientation));
    list_container_->ResolveListAxisGap(
        CSSPropertyID::kPropertyIDListMainAxisGap, lepus::Value(main_axis_gap));
    list_container_->ResolveListAxisGap(
        CSSPropertyID::kPropertyIDListCrossAxisGap,
        lepus::Value(cross_axis_gap));
    if (scroll_orientation == "vertical") {
      list_element_ref_->height_ = list_main_size;
      list_element_ref_->width_ = list_cross_size;
    } else if (scroll_orientation == "horizontal") {
      list_element_ref_->height_ = list_cross_size;
      list_element_ref_->width_ = list_main_size;
    }
  }

  void InitFiberDataSource(int data_count, std::vector<int> estimated_sizes,
                           bool enable_full_span = false) {
    list::InsertAction insert_action;
    for (int i = 0; i < data_count; ++i) {
      insert_action.insert_ops_.push_back(
          {i, base::FormatString("list-item-%d", i), estimated_sizes[i],
           enable_full_span && (!(i % full_span_tag_)), false, false});
    }
    list::FiberDiffResult fiber_diff_result{
        .insert_action_ = insert_action,
    };
    list_container_->ResolveAttribute(
        list::kFiberListDiffInfo, lepus::Value(fiber_diff_result.Resolve()));
    list_adapter_->UpdateItemHolderToLatest(list_children_helper_);
  }

  BaseLayoutResult GenerateBaseLayoutResult(int data_count, int span_count,
                                            int min_item_size,
                                            int max_item_size,
                                            bool enable_full_span = false) {
    BaseLayoutResult base_layout_result(data_count, span_count);
    auto& item_sizes = base_layout_result.item_sizes_;
    auto& span_indexes = base_layout_result.span_indexes_;
    std::vector<float> span_sizes(span_count, 0.f);
    std::mt19937 gen(0);
    std::uniform_int_distribution<> dis(min_item_size, max_item_size);
    for (int i = 0; i < data_count; ++i) {
      bool is_full_span = enable_full_span && (!(i % full_span_tag_));
      int item_size = dis(gen);
      if (is_full_span) {
        float max_size =
            *std::max_element(span_sizes.begin(), span_sizes.end());
        for (int j = 0; j < span_count; ++j) {
          span_sizes[j] = max_size + item_size;
          span_indexes[j].push_back(i);
        }
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

 public:
  fml::RefPtr<Element> CreateComponentElement() {
    static int32_t base_css_id = 0;
    base::String component_id(std::to_string(++base_css_id));
    base::String entry_name("__ListItem__");
    base::String component_name("__ListItem__");
    base::String path("/index/components/ListItem");
    auto list_item_ref = element_manager_->CreateFiberComponent(
        component_id, base_css_id, entry_name, component_name, path);
    list_element_ref_->AddChildAt(list_item_ref, -1);
    return list_item_ref;
  }
};  // StaggeredGridLayoutManagerTest

TEST_F(StaggeredGridLayoutManagerTest, InitLayoutAttrs) {
  InitLayoutAttrs("waterfall", 3, "vertical", 10.f, 5.f, 2000.f, 1000.f);
  EXPECT_EQ(list_layout_manager()->IsHorizontal(), false);
  EXPECT_EQ(list_layout_manager()->span_count(), 3);
  EXPECT_TRUE(base::FloatsEqual(list_layout_manager()->main_axis_gap(), 10.f));
  EXPECT_TRUE(base::FloatsEqual(list_layout_manager()->cross_axis_gap(), 5.f));
  EXPECT_TRUE(base::FloatsEqual(list_layout_manager()->content_offset(), 0.f));
  EXPECT_TRUE(base::FloatsEqual(list_layout_manager()->content_size(), 0.f));
  EXPECT_TRUE(
      base::FloatsEqual(list_orientation_helper()->GetMeasurement(), 2000.f));
  EXPECT_TRUE(base::FloatsEqual(
      list_orientation_helper()->GetMeasurementInOther(), 1000.f));
}

TEST_F(StaggeredGridLayoutManagerTest, InitFiberDataSource) {
  InitFiberDataSource(100, std::vector<int>(100, 50));
  EXPECT_EQ(list_container_->GetDataCount(), 100);
  EXPECT_EQ(list_container_->GetDataCount(),
            list_children_helper_->children_.size());
}

TEST_F(StaggeredGridLayoutManagerTest, GenerateBaseLayoutResult) {
  int data_count = 1000;
  int span_count = 3;
  float min_item_size = 50;
  float max_item_size = 100;
  BaseLayoutResult layout_result =
      GenerateBaseLayoutResult(1000, 3, min_item_size, max_item_size);
  EXPECT_EQ(layout_result.data_count_, data_count);
  EXPECT_EQ(layout_result.span_count_, span_count);
  for (auto item_size : layout_result.item_sizes_) {
    EXPECT_TRUE(base::FloatsLargerOrEqual(item_size, min_item_size) &&
                base::FloatsLargerOrEqual(max_item_size, item_size));
  }
  for (auto span_index : layout_result.span_indexes_) {
    EXPECT_TRUE(span_index.size() >= 1);
  }
}

TEST_F(StaggeredGridLayoutManagerTest, LayoutInvalidItemHolder) {
  int data_count = 1000;
  int span_count = 3;
  BaseLayoutResult layout_result =
      GenerateBaseLayoutResult(data_count, span_count, 50, 100, false);
  InitLayoutAttrs("waterfall", span_count, "vertical", 0.f, 0.f, 2000.f,
                  1000.f);
  InitFiberDataSource(data_count, layout_result.item_sizes_, false);
  auto* layout_mananger = staggered_grid_layout_manager();
  layout_mananger->LayoutInvalidItemHolder(0);
  EXPECT_TRUE(base::FloatsEqual(layout_mananger->GetTargetContentSize(),
                                layout_result.content_size_));
  for (int i = 0; i < span_count; ++i) {
    auto& basic_span_index = layout_result.span_indexes_[i];
    auto& test_span_index = layout_mananger->column_indexes_[i];
    EXPECT_EQ(test_span_index.size(), basic_span_index.size());
    for (int j = 0; j < static_cast<int>(basic_span_index.size()); ++j) {
      EXPECT_EQ(test_span_index[j], basic_span_index[j]);
    }
  }
}

TEST_F(StaggeredGridLayoutManagerTest, LayoutInvalidItemHolderWithFullSpan) {
  int data_count = 10;
  int span_count = 3;
  BaseLayoutResult layout_result =
      GenerateBaseLayoutResult(data_count, span_count, 50, 100, true);
  InitLayoutAttrs("waterfall", span_count, "vertical", 0.f, 0.f, 2000.f,
                  1000.f);
  InitFiberDataSource(data_count, layout_result.item_sizes_, true);
  auto* layout_mananger = staggered_grid_layout_manager();
  layout_mananger->LayoutInvalidItemHolder(0);
  EXPECT_TRUE(base::FloatsEqual(layout_mananger->GetTargetContentSize(),
                                layout_result.content_size_));
  for (int i = 0; i < span_count; ++i) {
    auto& basic_span_index = layout_result.span_indexes_[i];
    auto& test_span_index = layout_mananger->column_indexes_[i];
    EXPECT_EQ(test_span_index.size(), basic_span_index.size());
    for (int j = 0; j < static_cast<int>(basic_span_index.size()); ++j) {
      EXPECT_EQ(test_span_index[j], basic_span_index[j]);
    }
  }
}

TEST_F(StaggeredGridLayoutManagerTest, HasRemainSpaceToFillEnd) {
  int span_count = 3;
  float list_main_size = 2000.f;
  float list_cross_size = 1000.f;
  float content_offset = 1000.f;
  float content_size = 5000.f;
  InitFiberDataSource(100, std::vector<int>(100, 50));
  InitLayoutAttrs("waterfall", span_count, "vertical", 0.f, 0.f, list_main_size,
                  list_cross_size);
  StaggeredGridLayoutManager* layout_mananger = staggered_grid_layout_manager();
  EXPECT_TRUE(layout_mananger != nullptr);
  layout_mananger->content_offset_ = content_offset;
  layout_mananger->content_size_ = content_size;

  // case 1. current end line is full span
  LayoutState layout_state(3, list::LayoutDirection::kLayoutToEnd);
  auto& end_lines = layout_state.end_lines;
  layout_state.is_end_full_span_ = true;
  std::fill(end_lines.begin(), end_lines.end(),
            list_main_size + content_offset - 1.f);
  EXPECT_TRUE(layout_mananger->HasRemainSpaceToFillEnd(0, layout_state));
  layout_state.Reset(span_count);
  layout_state.is_end_full_span_ = true;
  std::fill(end_lines.begin(), end_lines.end(),
            list_main_size + content_offset + 1.f);
  EXPECT_FALSE(layout_mananger->HasRemainSpaceToFillEnd(0, layout_state));

  // case 2. current end line is not full span but next item is full span.
  int next_bind_item_index = 10;
  list_adapter_->adapter_helper_->full_spans_.insert(next_bind_item_index);
  list_container_->GetItemHolderForIndex(next_bind_item_index)
      ->item_full_span_ = true;
  layout_state.Reset(span_count);
  for (int i = 0; i < span_count; ++i) {
    // 2950, 3000, 3050.
    end_lines[i] = list_main_size + content_offset + (i - 1) * 50.f;
  }
  EXPECT_FALSE(layout_mananger->HasRemainSpaceToFillEnd(next_bind_item_index,
                                                        layout_state));
  layout_state.Reset(span_count);
  for (int i = 0; i < span_count; ++i) {
    // 2850, 2900, 2950
    end_lines[i] = list_main_size + content_offset - (i + 1) * 50.f;
  }
  EXPECT_TRUE(layout_mananger->HasRemainSpaceToFillEnd(next_bind_item_index,
                                                       layout_state));

  // case 3. current end line is not full span and next item is not full span.
  list_adapter_->adapter_helper_->full_spans_.erase(next_bind_item_index);
  list_container_->GetItemHolderForIndex(next_bind_item_index)
      ->item_full_span_ = false;
  layout_state.Reset(span_count);
  for (int i = 0; i < span_count; ++i) {
    // 2950, 3000, 3050.
    end_lines[i] = list_main_size + content_offset + (i - 1) * 50.f;
  }
  EXPECT_TRUE(layout_mananger->HasRemainSpaceToFillEnd(next_bind_item_index,
                                                       layout_state));
  layout_state.Reset(span_count);
  for (int i = 0; i < span_count; ++i) {
    // 2850, 2900, 2950
    end_lines[i] = list_main_size + content_offset - (i + 1) * 50.f;
  }
  EXPECT_TRUE(layout_mananger->HasRemainSpaceToFillEnd(next_bind_item_index,
                                                       layout_state));
  layout_state.Reset(span_count);
  for (int i = 0; i < span_count; ++i) {
    // 3050, 3100, 3150
    end_lines[i] = list_main_size + content_offset + (i + 1) * 50.f;
  }
  EXPECT_FALSE(layout_mananger->HasRemainSpaceToFillEnd(next_bind_item_index,
                                                        layout_state));
}

TEST_F(StaggeredGridLayoutManagerTest, HasRemainSpaceToFillStart) {
  int span_count = 3;
  float list_main_size = 2000.f;
  float list_cross_size = 1000.f;
  float content_offset = 1000.f;
  float content_size = 5000.f;
  InitFiberDataSource(100, std::vector<int>(100, 50));
  InitLayoutAttrs("waterfall", span_count, "vertical", 0.f, 0.f, list_main_size,
                  list_cross_size);
  StaggeredGridLayoutManager* layout_mananger = staggered_grid_layout_manager();
  EXPECT_TRUE(layout_mananger != nullptr);
  layout_mananger->content_offset_ = content_offset;
  layout_mananger->content_size_ = content_size;

  // case 1. current start line is full span
  LayoutState layout_state(3, list::LayoutDirection::kLayoutToStart);
  auto& start_lines = layout_state.start_lines;
  layout_state.is_start_full_span_ = true;
  std::fill(start_lines.begin(), start_lines.end(), content_offset - 1.f);
  EXPECT_FALSE(layout_mananger->HasRemainSpaceToFillStart(layout_state));
  layout_state.Reset(span_count);
  layout_state.is_start_full_span_ = true;
  std::fill(start_lines.begin(), start_lines.end(), content_offset + 1.f);
  EXPECT_TRUE(layout_mananger->HasRemainSpaceToFillStart(layout_state));

  // case 2. current lines has no full span
  layout_state.Reset(span_count);
  for (int i = 0; i < span_count; ++i) {
    // 950, 1000, 1050
    start_lines[i] = content_offset + (i - 1) * 50.f;
  }
  EXPECT_TRUE(layout_mananger->HasRemainSpaceToFillStart(layout_state));
  layout_state.Reset(span_count);
  for (int i = 0; i < span_count; ++i) {
    // 850, 900, 950
    start_lines[i] = content_offset - i * 50.f;
  }
  EXPECT_FALSE(layout_mananger->HasRemainSpaceToFillStart(layout_state));
}

TEST_F(StaggeredGridLayoutManagerTest, FindNextIndexToFillStart) {
  int data_count = 1000;
  int span_count = 3;
  float list_main_size = 2000.f;
  float list_cross_size = 1000.f;
  float content_offset = 1000.f;
  BaseLayoutResult layout_result =
      GenerateBaseLayoutResult(data_count, span_count, 50, 100);
  InitLayoutAttrs("waterfall", span_count, "vertical", 0.f, 0.f, list_main_size,
                  list_cross_size);
  InitFiberDataSource(data_count, layout_result.item_sizes_);
  auto* layout_manager = staggered_grid_layout_manager();
  layout_manager->LayoutInvalidItemHolder(0);
  layout_manager->content_offset_ = content_offset;
  layout_manager->content_size_ = layout_manager->GetTargetContentSize();

  // case 1. start index is full span and start index is full span
  LayoutState layout_state(3, list::LayoutDirection::kLayoutToStart);
  // auto& start_lines = layout_state.start_lines;
  auto& start_indexes = layout_state.start_index;
  layout_state.is_start_full_span_ = true;
  std::fill(start_indexes.begin(), start_indexes.end(), 0);
  EXPECT_EQ(layout_manager->FindNextIndexToFillStart(layout_state),
            list::kInvalidIndex);

  // case 2. start index is full span and next index is full span
  layout_state.Reset(span_count);
  int next_bind_item_index = 10;
  std::fill(start_indexes.begin(), start_indexes.end(),
            next_bind_item_index + 1);
  layout_state.is_start_full_span_ = true;
  list_adapter_->adapter_helper_->full_spans_.insert(next_bind_item_index);
  list_container_->GetItemHolderForIndex(next_bind_item_index)
      ->item_full_span_ = true;
  EXPECT_EQ(layout_manager->FindNextIndexToFillStart(layout_state),
            next_bind_item_index);
}

TEST_F(StaggeredGridLayoutManagerTest, GetItemIndexBeforeTargetIndex) {
  int data_count = 1000;
  int span_count = 3;
  float list_main_size = 2000.f;
  float list_cross_size = 1000.f;
  float content_offset = 1000.f;
  BaseLayoutResult layout_result =
      GenerateBaseLayoutResult(data_count, span_count, 50, 100);
  InitLayoutAttrs("waterfall", span_count, "vertical", 0.f, 0.f, list_main_size,
                  list_cross_size);
  InitFiberDataSource(data_count, layout_result.item_sizes_);
  auto* layout_manager = staggered_grid_layout_manager();
  layout_manager->LayoutInvalidItemHolder(0);
  layout_manager->content_offset_ = content_offset;
  layout_manager->content_size_ = layout_manager->GetTargetContentSize();

  for (auto& span_indexes : layout_manager->column_indexes_) {
    for (int i = 0; i < static_cast<int>(span_indexes.size()); ++i) {
      int target_index = span_indexes[i];
      int result_index = layout_manager->GetItemIndexBeforeTargetIndex(
          span_indexes, target_index);
      if (target_index == 0 || i == 0) {
        EXPECT_EQ(result_index, list::kInvalidIndex);
      } else {
        ItemHolder* next_item_holder =
            list_container_->GetItemHolderForIndex(span_indexes[i - 1]);
        EXPECT_TRUE(next_item_holder != nullptr);
        if (base::FloatsLargerOrEqual(
                list_orientation_helper()->GetDecoratedEnd(next_item_holder),
                content_offset)) {
          EXPECT_EQ(result_index, next_item_holder->index());
        } else {
          EXPECT_EQ(result_index, list::kInvalidIndex);
        }
      }
    }
  }
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
