
// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/list/decoupled_list_anchor_manager.h"

#include "base/include/string/string_utils.h"
#include "core/list/decoupled_list_container_impl.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "testing/fiber_data_source.h"
#include "testing/mock_list_element.h"
#include "testing/mock_list_item_element.h"
#include "testing/radon_data_source.h"
#include "testing/utils.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace list {

using ::testing::_;
using ::testing::Return;

class ListAnchorManagerTest : public ::testing::Test {
 public:
  ListAnchorManagerTest() = default;
  ~ListAnchorManagerTest() override = default;

  std::unique_ptr<MockListElement> mock_list_element_{nullptr};
  std::unique_ptr<ListContainerImpl> list_container_impl_{nullptr};
  std::shared_ptr<pub::PubValueFactoryDefault> value_factory_{nullptr};
  ListAdapter* list_adapter_{nullptr};
  ListLayoutManager* list_layout_manager_{nullptr};
  ListAnchorManager* list_anchor_manager_{nullptr};

  void SetUp() override {
    value_factory_ = std::make_shared<pub::PubValueFactoryDefault>();
    mock_list_element_ = std::make_unique<MockListElement>();
    list_container_impl_ = std::make_unique<ListContainerImpl>(
        mock_list_element_.get(), value_factory_);
    list_adapter_ = list_container_impl_->list_adapter();
    list_layout_manager_ = list_container_impl_->list_layout_manager();
    list_anchor_manager_ = list_layout_manager_->list_anchor_manager_.get();
  }

  ListOrientationHelper* GetOrientationHelper() {
    return list_layout_manager_->list_orientation_helper_.get();
  }

  bool IsFullSpan(int index) {
    static int full_span_flag = 5;
    return !(index % full_span_flag);
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

  void InitRadonDataSource(int data_count) {
    testing::RadonDataSource radon_data_source;
    for (int i = 0; i < data_count; ++i) {
      radon_data_source.item_keys_.emplace_back(
          base::FormatString("list-item-%d", i));
      radon_data_source.insertion_.emplace_back(i);
      radon_data_source.estimated_main_axis_size_pxs_.emplace_back(100);
    }
    LIST_CONTAINER_DEFINE_PROP_LEPUS_VALUE(
        RadonListPlatformInfo,
        lepus::Value(radon_data_source.GenerateDataSource()));
    list_container_impl_->ResolveAttribute(*key, value);
    list_container_impl_->PropsUpdateFinish();
  }

  void OnLayoutChildren() {
    static int list_item_impl_id = mock_list_element_->GetImplId() + 1;
    auto pipeline = std::make_shared<tasm::PipelineOptions>();
    EXPECT_CALL(*mock_list_element_, ComponentAtIndex(_, _, _))
        .WillRepeatedly([&](uint32_t index, int64_t operationId,
                            bool enable_reuse_notification) {
          ItemHolder* item_holder =
              list_container_impl_->GetItemHolderForIndex(index);
          const auto& item_key = item_holder->item_key();
          // mock pipeline when list item finish rendering.
          auto pipeline = std::make_shared<tasm::PipelineOptions>();
          pipeline->operation_id = operationId;
          pipeline->has_layout = true;
          mock_list_element_->AddListItemElement(
              item_key,
              std::make_unique<MockListItemElement>(list_item_impl_id++));
          // set list item's real height.
          MockListItemElement* mock_list_item_element =
              mock_list_element_->GetListItemElement(item_key);
          mock_list_item_element->height_ = 100.f;
          // invoke OnFinishBindItemHolder.
          list_adapter_->OnFinishBindItemHolder(mock_list_item_element,
                                                pipeline);
          return true;
        });
    list_container_impl_->OnLayoutChildren(pipeline);
  }

  void CheckAnchorInfo(const ListAnchorManager::AnchorInfo& target,
                       const ListAnchorManager::AnchorInfo& basic) {
    EXPECT_TRUE(target.valid_ == basic.valid_ &&
                target.index_ == basic.index_ &&
                base::FloatsEqual(target.start_offset_, basic.start_offset_) &&
                base::FloatsEqual(target.start_alignment_delta_,
                                  basic.start_alignment_delta_) &&
                target.is_removed_child_ref_ == basic.is_removed_child_ref_ &&
                target.align_start_ == basic.align_start_ &&
                target.item_holder_ == basic.item_holder_);
  }
};

TEST_F(ListAnchorManagerTest, IsValidInitialScrollIndex) {
  int data_count = 100;
  InitFiberDataSource(data_count, std::vector<int>(data_count, 50), false);
  list_anchor_manager_->SetInitialScrollIndex(1);
  EXPECT_TRUE(list_anchor_manager_->IsValidInitialScrollIndex());
  list_anchor_manager_->SetInitialScrollIndex(101);
  EXPECT_FALSE(list_anchor_manager_->IsValidInitialScrollIndex());
}

TEST_F(ListAnchorManagerTest, FindAnchor1) {
  int data_count = 100;
  InitFiberDataSource(data_count, std::vector<int>(data_count, 50), false);
  float padding_top = 10.f;
  mock_list_element_->paddings_ = {0.f, padding_top, 0.f, 0.f};
  ListAnchorManager::AnchorInfo anchor_info;
  list_anchor_manager_->FindAnchor(anchor_info, false, kInvalidIndex);
  CheckAnchorInfo(
      anchor_info,
      {.valid_ = true,
       .index_ = 0,
       .start_offset_ = padding_top,
       .start_alignment_delta_ = padding_top,
       .item_holder_ = list_container_impl_->GetItemHolderForIndex(0)});
}

TEST_F(ListAnchorManagerTest, FindAnchor2) {
  int data_count = 100;
  InitFiberDataSource(data_count, std::vector<int>(data_count, 50), false);
  // Clear all data
  testing::RemoveAction remove_action;
  for (int i = 0; i < data_count; ++i) {
    remove_action.remove_ops_.emplace_back(i);
  }
  testing::FiberDataSource fiber_data_source{
      .remove_action_ = remove_action,
  };
  LIST_CONTAINER_DEFINE_PROP_LEPUS_VALUE(
      FiberUpdateListInfo, lepus::Value(fiber_data_source.Resolve()));
  list_container_impl_->ResolveAttribute(*key, value);
  list_container_impl_->PropsUpdateFinish();

  ListAnchorManager::AnchorInfo anchor_info;
  list_anchor_manager_->FindAnchor(anchor_info, false, kInvalidIndex);
  CheckAnchorInfo(
      anchor_info,
      {.valid_ = false, .index_ = kInvalidIndex, .item_holder_ = nullptr});
}

TEST_F(ListAnchorManagerTest, FindAnchor3) {
  int data_count = 100;
  InitLayoutAttrs("single", 1, "vertical", 0.f, 0.f, 2000.f, 500.f);
  InitRadonDataSource(data_count);
  OnLayoutChildren();

  // update from: [0, ... 9]
  // update to: [0, ... 9]
  int first_valid_index = 10;
  ItemHolder* first_valid_item_holder =
      list_container_impl_->GetItemHolderForIndex(first_valid_index);
  testing::RadonDataSource radon_data_source;
  for (int i = 0; i < data_count; ++i) {
    radon_data_source.item_keys_.emplace_back(
        base::FormatString("list-item-%d", i));
    radon_data_source.estimated_main_axis_size_pxs_.emplace_back(100);
    if (i < first_valid_index) {
      radon_data_source.update_from_.emplace_back(i);
      radon_data_source.update_to_.emplace_back(i);
    }
  }
  LIST_CONTAINER_DEFINE_PROP_LEPUS_VALUE(
      RadonListPlatformInfo,
      lepus::Value(radon_data_source.GenerateDataSource()));
  list_container_impl_->ResolveAttribute(*key, value);
  list_container_impl_->PropsUpdateFinish();

  ListAnchorManager::AnchorInfo anchor_info;
  list_anchor_manager_->FindAnchor(anchor_info, false, kInvalidIndex);
  CheckAnchorInfo(
      anchor_info,
      {.valid_ = true,
       .index_ = first_valid_index,
       .start_offset_ =
           GetOrientationHelper()->GetDecoratedStart(first_valid_item_holder),
       .start_alignment_delta_ =
           GetOrientationHelper()->GetStart(first_valid_item_holder) -
           list_layout_manager_->content_offset(),
       .item_holder_ = first_valid_item_holder});
}

TEST_F(ListAnchorManagerTest, GetAnchorCandidates) {
  int data_count = 100;
  InitLayoutAttrs("single", 1, "vertical", 0.f, 0.f, 2000.f, 500.f);
  InitRadonDataSource(data_count);
  OnLayoutChildren();

  // case1. No update
  {
    testing::RadonDataSource radon_data_source;
    for (int i = 0; i < data_count; ++i) {
      radon_data_source.item_keys_.emplace_back(
          base::FormatString("list-item-%d", i));
      radon_data_source.estimated_main_axis_size_pxs_.emplace_back(100);
    }
    LIST_CONTAINER_DEFINE_PROP_LEPUS_VALUE(
        RadonListPlatformInfo,
        lepus::Value(radon_data_source.GenerateDataSource()));
    list_container_impl_->ResolveAttribute(*key, value);
    list_container_impl_->PropsUpdateFinish();

    const auto& anchor_candidates =
        list_anchor_manager_->GetAnchorCandidates(kInvalidIndex, false);
    EXPECT_EQ(anchor_candidates.size(), kAnchorCandidateSize);
    EXPECT_TRUE(anchor_candidates[0] != nullptr);
    EXPECT_TRUE(anchor_candidates[1] == nullptr);
    EXPECT_TRUE(anchor_candidates[2] == nullptr);
    EXPECT_TRUE(anchor_candidates[3] == nullptr);
  }

  // case2. Update
  {
    testing::RadonDataSource radon_data_source;
    for (int i = 0; i < data_count; ++i) {
      radon_data_source.item_keys_.emplace_back(
          base::FormatString("list-item-%d", i));
      radon_data_source.estimated_main_axis_size_pxs_.emplace_back(100);
    }
    // remove: [0 - 9]
    // insert: [0 - 9]
    // update: [10 - 99]
    int first_updated_index = 10;
    ItemHolder* first_updated_item_holder =
        list_container_impl_->GetItemHolderForIndex(first_updated_index);
    for (int i = 0; i < data_count; ++i) {
      if (i < first_updated_index) {
        radon_data_source.removal_.emplace_back(i);
        radon_data_source.insertion_.emplace_back(i);
        radon_data_source.item_keys_.emplace_back(
            base::FormatString("new-list-item-%d", i));
      } else {
        radon_data_source.update_from_.emplace_back(i);
        radon_data_source.update_to_.emplace_back(i);
        radon_data_source.item_keys_.emplace_back(
            base::FormatString("list-item-%d", i));
        radon_data_source.estimated_main_axis_size_pxs_.emplace_back(100);
      }
    }
    LIST_CONTAINER_DEFINE_PROP_LEPUS_VALUE(
        RadonListPlatformInfo,
        lepus::Value(radon_data_source.GenerateDataSource()));
    list_container_impl_->ResolveAttribute(*key, value);
    list_container_impl_->PropsUpdateFinish();

    const auto& anchor_candidates =
        list_anchor_manager_->GetAnchorCandidates(kInvalidIndex, false);
    EXPECT_EQ(anchor_candidates.size(), kAnchorCandidateSize);
    EXPECT_TRUE(anchor_candidates[0] == nullptr);
    EXPECT_TRUE(anchor_candidates[1] == first_updated_item_holder);
    EXPECT_TRUE(anchor_candidates[2] == nullptr);
    EXPECT_TRUE(anchor_candidates[3] == nullptr);
  }
}

}  // namespace list
}  // namespace lynx
