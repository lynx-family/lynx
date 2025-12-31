// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/list/decoupled_list_adapter.h"

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

class ListAdapterTest : public ::testing::Test {
 public:
  ListAdapterTest() = default;
  ~ListAdapterTest() override = default;

  std::unique_ptr<MockListElement> mock_list_element_{nullptr};
  std::unique_ptr<ListContainerImpl> list_container_impl_{nullptr};
  std::shared_ptr<pub::PubValueFactoryDefault> value_factory_{nullptr};
  ListLayoutManager* list_layout_manager_{nullptr};
  ListAdapter* list_adapter_{nullptr};
  AdapterHelper* list_adapter_helper_{nullptr};

  void SetUp() override {
    value_factory_ = std::make_shared<pub::PubValueFactoryDefault>();
    mock_list_element_ = std::make_unique<MockListElement>();
    list_container_impl_ = std::make_unique<ListContainerImpl>(
        mock_list_element_.get(), value_factory_);
    list_layout_manager_ = list_container_impl_->list_layout_manager();
    list_adapter_ = list_container_impl_->list_adapter();
    list_adapter_helper_ = list_adapter_->list_adapter_helper();
  }
};

TEST_F(ListAdapterTest, UpdateRadonDataSource) {
  testing::RadonDataSource radon_data_source{
      .item_keys_ = {"A_0", "B_1", "C_2", "D_3", "E_4", "F_5", "G_6", "H_7",
                     "I_8"},
      .insertion_ = {0, 1, 2, 3, 4, 5, 6, 7, 8},
      .estimated_height_pxs_ = {100, 100, 100, 100, 100, 100, 100, 100, 100},
      .estimated_main_axis_size_pxs_ = {100, 100, 100, 100, 100, 100, 100, 100,
                                        100},
      .sticky_tops_ = {0, 1},
      .sticky_bottoms_ = {7, 8},
      .full_spans_ = {0, 1, 7, 8},
  };
  auto result = list_adapter_->UpdateRadonDataSource(pub::ValueImplLepus(
      lepus::Value(radon_data_source.GenerateDataSource())));
  EXPECT_EQ(result.first, ListAdapterDiffResult::kInsert);
  EXPECT_EQ(list_adapter_helper_->item_keys().size(),
            radon_data_source.GetItemCount());
  EXPECT_EQ(list_adapter_helper_->item_key_map().size(),
            radon_data_source.GetItemCount());
  EXPECT_EQ(list_adapter_->GetDataCount(), radon_data_source.GetItemCount());
  EXPECT_EQ(list_adapter_helper_->estimated_heights_px().size(),
            radon_data_source.GetItemCount());
  EXPECT_EQ(list_adapter_helper_->estimated_sizes_px().size(),
            radon_data_source.GetItemCount());
  EXPECT_EQ(list_adapter_helper_->full_spans().size(),
            radon_data_source.full_spans_.size());
  EXPECT_EQ(list_adapter_helper_->sticky_tops().size(),
            radon_data_source.sticky_tops_.size());
  EXPECT_EQ(list_adapter_helper_->sticky_bottoms().size(),
            radon_data_source.sticky_bottoms_.size());
}

TEST_F(ListAdapterTest, UpdateFiberDataSource) {
  testing::InsertAction insert_action{
      .insert_ops_ = {
          {.position_ = 0, "A_0", 100, true, true, false, false},
          {.position_ = 1, "B_1", 100, true, true, false, false},
          {.position_ = 2, "C_2", 100, true, true, false, false},
          {.position_ = 3, "D_3", 100, true, true, false, false},
          {.position_ = 4, "E_4", 100, true, true, false, false},
          {.position_ = 5, "F_5", 100, true, true, false, false},
          {.position_ = 6, "G_6", 100, true, true, false, false},
          {.position_ = 7, "H_7", 100, true, true, false, false},
          {.position_ = 8, "I_8", 100, true, true, false, false},
          {.position_ = 9, "J_9", 100, true, true, false, false},
      }};
  testing::FiberDataSource fiber_data_source{
      .insert_action_ = insert_action,
  };
  auto result = list_adapter_->UpdateFiberDataSource(
      pub::ValueImplLepus(lepus::Value(fiber_data_source.Resolve())));
  EXPECT_EQ(result.first, ListAdapterDiffResult::kInsert);
  EXPECT_EQ(list_adapter_helper_->item_keys().size(), 10);
  EXPECT_EQ(list_adapter_helper_->item_key_map().size(), 10);
  EXPECT_EQ(list_adapter_->GetDataCount(), 10);
  EXPECT_EQ(list_adapter_helper_->estimated_sizes_px().size(), 10);
  EXPECT_EQ(list_adapter_helper_->full_spans().size(), 10);
  EXPECT_EQ(list_adapter_helper_->sticky_tops().size(), 10);
  EXPECT_EQ(list_adapter_helper_->sticky_bottoms().size(), 0);
  EXPECT_EQ(list_adapter_helper_->unrecyclable().size(), 10);
}

}  // namespace list
}  // namespace lynx
