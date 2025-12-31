// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/list/decoupled_default_list_adapter.h"

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

class DefaultListAdapterTest : public ::testing::Test {
 public:
  DefaultListAdapterTest() = default;
  ~DefaultListAdapterTest() override = default;

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

  void InitFiberDataSource() {
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
    list_container_impl_->ResolveAttribute(*key, value);
    list_container_impl_->PropsUpdateFinish();
  }
};

TEST_F(DefaultListAdapterTest, OnItemHolderRemoved) {
  std::unique_ptr<ItemHolder> item_holder = std::make_unique<ItemHolder>(
      0, "A_0", list_container_impl_->list_animation_manager());
  item_holder->MarkDirty(false);
  item_holder->MarkRemoved(false);
  list_adapter_->OnItemHolderRemoved(item_holder.get());
  EXPECT_TRUE(item_holder->dirty());
  EXPECT_TRUE(item_holder->removed());
}

TEST_F(DefaultListAdapterTest, OnItemHolderUpdateFrom) {
  std::unique_ptr<ItemHolder> item_holder = std::make_unique<ItemHolder>(
      0, "A_0", list_container_impl_->list_animation_manager());
  item_holder->MarkDirty(false);
  list_adapter_->OnItemHolderUpdateFrom(item_holder.get());
  EXPECT_TRUE(item_holder->dirty());
}

TEST_F(DefaultListAdapterTest, OnItemHolderUpdateTo) {
  std::unique_ptr<ItemHolder> item_holder = std::make_unique<ItemHolder>(
      0, "A_0", list_container_impl_->list_animation_manager());
  item_holder->MarkDirty(false);
  item_holder->MarkDiffStatus(DiffStatus::kValid);
  list_adapter_->OnItemHolderUpdateTo(item_holder.get(), false);
  EXPECT_TRUE(item_holder->dirty());
  EXPECT_TRUE(item_holder->is_updated());
}

TEST_F(DefaultListAdapterTest, OnItemHolderMovedFrom) {
  std::unique_ptr<ItemHolder> item_holder = std::make_unique<ItemHolder>(
      0, "A_0", list_container_impl_->list_animation_manager());
  item_holder->MarkDirty(false);
  list_adapter_->OnItemHolderMovedFrom(item_holder.get());
  EXPECT_TRUE(item_holder->dirty());
}

TEST_F(DefaultListAdapterTest, OnItemHolderMovedTo) {
  std::unique_ptr<ItemHolder> item_holder = std::make_unique<ItemHolder>(
      0, "A_0", list_container_impl_->list_animation_manager());
  item_holder->MarkDirty(false);
  list_adapter_->OnItemHolderMovedTo(item_holder.get());
  EXPECT_TRUE(item_holder->dirty());
}

TEST_F(DefaultListAdapterTest, OnItemHolderReInsert) {
  std::unique_ptr<ItemHolder> item_holder = std::make_unique<ItemHolder>(
      0, "A_0", list_container_impl_->list_animation_manager());
  item_holder->MarkRemoved(true);
  item_holder->MarkDirty(true);
  list_adapter_->OnItemHolderReInsert(item_holder.get());
  EXPECT_TRUE(item_holder->dirty());
  EXPECT_FALSE(item_holder->removed());
}

TEST_F(DefaultListAdapterTest, OnDataSetChanged) {
  InitFiberDataSource();
  for (int i = 0; i < list_adapter_->GetDataCount(); ++i) {
    ItemHolder* item_holder = list_adapter_->GetItemHolderForIndex(i);
    item_holder->MarkDirty(false);
  }
  list_adapter_->OnDataSetChanged();
  for (int i = 0; i < list_adapter_->GetDataCount(); ++i) {
    ItemHolder* item_holder = list_adapter_->GetItemHolderForIndex(i);
    EXPECT_TRUE(list_adapter_->IsDirty(item_holder));
  }
}

TEST_F(DefaultListAdapterTest, BindItemHolder) {
  InitFiberDataSource();
  list_container_impl_->StartInterceptListElementUpdated();
  int index = 0;
  ItemHolder* item_holder = list_adapter_->GetItemHolderForIndex(index);
  // Check Bind Item Holder
  EXPECT_CALL(*mock_list_element_, ComponentAtIndex(_, _, _))
      .WillOnce(Return(true));
  EXPECT_TRUE(list_adapter_->BindItemHolder(item_holder, index, false));
  EXPECT_FALSE(list_adapter_->IsDirty(item_holder));
  EXPECT_TRUE(list_adapter_->IsBinding(item_holder));
  EXPECT_FALSE(list_adapter_->IsRecycled(item_holder));
  EXPECT_FALSE(list_adapter_->IsFinishedBinding(item_holder));
  EXPECT_EQ(list_adapter_->GetItemElementDelegate(item_holder), nullptr);
  list_container_impl_->StopInterceptListElementUpdated();
}

TEST_F(DefaultListAdapterTest, OnFinishBindItemHolder) {
  InitFiberDataSource();
  list_container_impl_->StartInterceptListElementUpdated();
  int index = 0;
  ItemHolder* item_holder = list_adapter_->GetItemHolderForIndex(index);
  EXPECT_CALL(*mock_list_element_, ComponentAtIndex(_, _, _))
      .WillOnce(Return(true));
  EXPECT_TRUE(list_adapter_->BindItemHolder(item_holder, index, false));
  EXPECT_FALSE(list_adapter_->IsDirty(item_holder));
  EXPECT_TRUE(list_adapter_->IsBinding(item_holder));
  EXPECT_FALSE(list_adapter_->IsRecycled(item_holder));
  EXPECT_FALSE(list_adapter_->IsFinishedBinding(item_holder));
  EXPECT_EQ(list_adapter_->GetItemElementDelegate(item_holder), nullptr);
  // Check Finish Bind Item Holder.
  auto pipeline = std::make_shared<tasm::PipelineOptions>();
  pipeline->operation_id = item_holder->operation_id_;
  const auto& item_key = item_holder->item_key();
  mock_list_element_->AddListItemElement(
      item_key, std::make_unique<MockListItemElement>(
                    mock_list_element_->GetImplId() + 1));
  list_adapter_->OnFinishBindItemHolder(
      mock_list_element_->GetListItemElement(item_key), pipeline);
  EXPECT_FALSE(list_adapter_->IsBinding(item_holder));
  EXPECT_FALSE(list_adapter_->IsRecycled(item_holder));
  EXPECT_TRUE(list_adapter_->IsFinishedBinding(item_holder));
  EXPECT_FALSE(list_adapter_->IsDirty(item_holder));
  EXPECT_TRUE(list_adapter_->GetItemElementDelegate(item_holder) != nullptr);
  list_container_impl_->StopInterceptListElementUpdated();
}

TEST_F(DefaultListAdapterTest, RadonDiffCase0) {
  // Before
  testing::RadonDataSource radon_data_source{
      .item_keys_ = {"A_0", "B_1", "C_2", "D_3", "E_4", "F_5", "G_6", "H_7",
                     "I_8"},
      .insertion_ = {0, 1, 2, 3, 4, 5, 6, 7, 8},
      .estimated_main_axis_size_pxs_ = {100, 100, 100, 100, 100, 100, 100, 100,
                                        100},
  };
  auto result = list_adapter_->UpdateRadonDataSource(pub::ValueImplLepus(
      lepus::Value(radon_data_source.GenerateDataSource())));
  EXPECT_EQ(result.first, ListAdapterDiffResult::kInsert);
  list_adapter_->UpdateItemHolderToLatest(
      list_container_impl_->list_children_helper());

  list_container_impl_->StartInterceptListElementUpdated();
  for (int i = 0; i < static_cast<int>(radon_data_source.GetItemCount()); ++i) {
    ItemHolder* item_holder = list_adapter_->GetItemHolderForIndex(i);
    EXPECT_NE(item_holder, nullptr);
    EXPECT_FALSE(list_adapter_->IsRecycled(item_holder));
    EXPECT_FALSE(list_adapter_->IsBinding(item_holder));
    EXPECT_FALSE(list_adapter_->IsFinishedBinding(item_holder));
    EXPECT_TRUE(list_adapter_->IsDirty(item_holder));
    EXPECT_FALSE(list_adapter_->IsUpdated(item_holder));
    EXPECT_FALSE(list_adapter_->IsRemoved(item_holder));
    EXPECT_TRUE(list_adapter_->GetItemElementDelegate(item_holder) == nullptr);
  }
  list_container_impl_->StopInterceptListElementUpdated();
}

TEST_F(DefaultListAdapterTest, RadonDiffCase1) {
  // Before
  testing::RadonDataSource radon_data_source{
      .item_keys_ = {"A_0", "B_1", "C_2", "D_3", "E_4", "F_5", "G_6", "H_7",
                     "I_8"},
      .insertion_ = {0, 1, 2, 3, 4, 5, 6, 7, 8},
      .estimated_main_axis_size_pxs_ = {100, 100, 100, 100, 100, 100, 100, 100,
                                        100},
  };
  list_adapter_->UpdateRadonDataSource(pub::ValueImplLepus(
      lepus::Value(radon_data_source.GenerateDataSource())));
  list_adapter_->UpdateItemHolderToLatest(
      list_container_impl_->list_children_helper());
  int list_item_impl_id = mock_list_element_->GetImplId() + 1;

  {
    list_container_impl_->StartInterceptListElementUpdated();
    for (int i = 0; i < static_cast<int>(radon_data_source.GetItemCount());
         ++i) {
      ItemHolder* item_holder = list_adapter_->GetItemHolderForIndex(i);
      // Bind.
      EXPECT_CALL(*mock_list_element_, ComponentAtIndex(_, _, _))
          .WillOnce(Return(true));
      EXPECT_TRUE(list_adapter_->BindItemHolder(item_holder, i, false));
      // Finish bind.
      auto pipeline = std::make_shared<tasm::PipelineOptions>();
      pipeline->operation_id = item_holder->operation_id_;
      const auto& item_key = item_holder->item_key();
      mock_list_element_->AddListItemElement(
          item_key, std::make_unique<MockListItemElement>(list_item_impl_id++));
      list_adapter_->OnFinishBindItemHolder(
          mock_list_element_->GetListItemElement(item_key), pipeline);
      EXPECT_TRUE(list_adapter_->GetItemElementDelegate(item_holder) !=
                  nullptr);
    }
    list_container_impl_->StopInterceptListElementUpdated();
  }

  // after
  testing::RadonDataSource radon_data_source_1{
      .item_keys_ = {"New_A_0", "New_B_1", "New_C_2", "New_D_3", "E_4",
                     "New_F_5", "New_G_6", "New_H_7", "New_I_8"},
      .insertion_ = {0, 1, 2, 3, 5, 6, 7, 8},
      .removal_ = {0, 1, 2, 3, 5, 6, 7, 8},
  };
  list_adapter_->UpdateRadonDataSource(pub::ValueImplLepus(
      lepus::Value(radon_data_source_1.GenerateDataSource())));
  list_adapter_->UpdateItemHolderToLatest(
      list_container_impl_->list_children_helper());

  {
    list_container_impl_->StartInterceptListElementUpdated();
    for (int i = 0; i < static_cast<int>(radon_data_source_1.GetItemCount());
         ++i) {
      ItemHolder* item_holder = list_adapter_->GetItemHolderForIndex(i);
      if (i == 4) {
        // No need to bind
        EXPECT_FALSE(list_adapter_->BindItemHolder(item_holder, i));
      } else {
        // Need to bind
        EXPECT_CALL(*mock_list_element_, ComponentAtIndex(_, _, _))
            .WillOnce(Return(true));
        EXPECT_TRUE(list_adapter_->BindItemHolder(item_holder, i));
        auto pipeline = std::make_shared<tasm::PipelineOptions>();
        pipeline->operation_id = item_holder->operation_id_;
        const auto& item_key = item_holder->item_key();
        mock_list_element_->AddListItemElement(
            item_key,
            std::make_unique<MockListItemElement>(list_item_impl_id++));
        list_adapter_->OnFinishBindItemHolder(
            mock_list_element_->GetListItemElement(item_key), pipeline);
        EXPECT_TRUE(list_adapter_->GetItemElementDelegate(item_holder) !=
                    nullptr);
      }
    }
    list_adapter_->RecycleRemovedItemHolders();
    EXPECT_EQ(list_adapter_->item_holder_map_->size(),
              radon_data_source_1.GetItemCount());
    list_container_impl_->StopInterceptListElementUpdated();
  }
}

TEST_F(DefaultListAdapterTest, RadonDiffCase2) {
  // Before
  testing::RadonDataSource radon_data_source{
      .item_keys_ = {"A_0", "B_1", "C_2", "D_3", "E_4", "F_5", "G_6", "H_7",
                     "I_8"},
      .insertion_ = {0, 1, 2, 3, 4, 5, 6, 7, 8},
      .estimated_main_axis_size_pxs_ = {100, 100, 100, 100, 100, 100, 100, 100,
                                        100},
  };
  list_adapter_->UpdateRadonDataSource(pub::ValueImplLepus(
      lepus::Value(radon_data_source.GenerateDataSource())));
  list_adapter_->UpdateItemHolderToLatest(
      list_container_impl_->list_children_helper());
  int list_item_impl_id = mock_list_element_->GetImplId() + 1;
  int raw_data_count = static_cast<int>(radon_data_source.GetItemCount());

  {
    // Trigger all item holders to bind.
    list_container_impl_->StartInterceptListElementUpdated();
    for (int i = 0; i < raw_data_count; ++i) {
      ItemHolder* item_holder = list_adapter_->GetItemHolderForIndex(i);
      // Bind.
      EXPECT_CALL(*mock_list_element_, ComponentAtIndex(_, _, _))
          .WillOnce(Return(true));
      EXPECT_TRUE(list_adapter_->BindItemHolder(item_holder, i, false));
    }
    list_container_impl_->StopInterceptListElementUpdated();
  }

  std::vector<std::shared_ptr<tasm::PipelineOptions>> pipeline_options;
  {
    // The item holder with index < raw_data_count / 2 is finished bind.
    list_container_impl_->StartInterceptListElementUpdated();
    for (int i = 0; i < raw_data_count; ++i) {
      ItemHolder* item_holder = list_adapter_->GetItemHolderForIndex(i);
      auto pipeline = std::make_shared<tasm::PipelineOptions>();
      pipeline->operation_id = item_holder->operation_id_;
      pipeline_options.emplace_back(pipeline);
      const auto& item_key = item_holder->item_key();
      mock_list_element_->AddListItemElement(
          item_key, std::make_unique<MockListItemElement>(list_item_impl_id++));
      if (i < raw_data_count / 2) {
        list_adapter_->OnFinishBindItemHolder(
            mock_list_element_->GetListItemElement(item_key), pipeline);
        EXPECT_TRUE(list_adapter_->GetItemElementDelegate(item_holder) !=
                    nullptr);
      }
    }
    list_container_impl_->StopInterceptListElementUpdated();
  }

  int remaining_count = raw_data_count - raw_data_count / 2;
  EXPECT_EQ(static_cast<DefaultListAdapter*>(list_adapter_)
                ->binding_item_holder_weak_map_->size(),
            remaining_count);

  // after
  testing::RadonDataSource radon_data_source_1{
      .item_keys_ = {"New_A_0", "New_B_1", "New_C_2", "New_D_3", "New_E_4",
                     "New_F_5", "New_G_6", "New_H_7", "New_I_8"},
      .insertion_ = {0, 1, 2, 3, 4, 5, 6, 7, 8},
      .removal_ = {0, 1, 2, 3, 4, 5, 6, 7, 8},
  };
  list_adapter_->UpdateRadonDataSource(pub::ValueImplLepus(
      lepus::Value(radon_data_source_1.GenerateDataSource())));
  list_adapter_->UpdateItemHolderToLatest(
      list_container_impl_->list_children_helper());
  // Simulate destroy removed item holders.
  list_adapter_->RecycleRemovedItemHolders();

  {
    // The item holder with index >= raw_data_count / 2 is finished bind.
    for (int i = raw_data_count / 2; i < raw_data_count; ++i) {
      auto pipeline = pipeline_options[i];
      const auto& item_key = radon_data_source.item_keys_[i];
      EXPECT_CALL(*mock_list_element_, EnqueueComponent(_)).Times(1);
      list_adapter_->OnFinishBindItemHolder(
          mock_list_element_->GetListItemElement(item_key), pipeline);
    }
  }
}

TEST_F(DefaultListAdapterTest, FiberDiffCase0) {
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
  list_adapter_->UpdateItemHolderToLatest(
      list_container_impl_->list_children_helper());
  for (int i = 0; i < 10; ++i) {
    ItemHolder* item_holder = list_adapter_->GetItemHolderForIndex(i);
    EXPECT_NE(item_holder, nullptr);
    EXPECT_FALSE(list_adapter_->IsRecycled(item_holder));
    EXPECT_FALSE(list_adapter_->IsBinding(item_holder));
    EXPECT_FALSE(list_adapter_->IsFinishedBinding(item_holder));
    EXPECT_TRUE(list_adapter_->IsDirty(item_holder));
    EXPECT_FALSE(list_adapter_->IsUpdated(item_holder));
    EXPECT_FALSE(list_adapter_->IsRemoved(item_holder));
    EXPECT_TRUE(list_adapter_->GetItemElementDelegate(item_holder) == nullptr);
  }
}

TEST_F(DefaultListAdapterTest, FiberDiffCase1) {
  InitFiberDataSource();
  testing::RemoveAction remove_action{
      .remove_ops_ = {0, 1, 2, 3, 4},
  };
  testing::UpdateAction update_action{
      .update_ops_ =
          {
              {.from_ = 0, .to_ = 0, false, "F_5", 50, true, true, true, true},
              {.from_ = 1, .to_ = 1, false, "G_6", 50, true, true, true, true},
              {.from_ = 2, .to_ = 2, false, "H_7", 50, true, true, true, true},
              {.from_ = 3, .to_ = 3, false, "I_8", 50, true, true, true, true},
              {.from_ = 4, .to_ = 4, false, "J_9", 50, true, true, true, true},
          },
  };
  testing::FiberDataSource fiber_data_source{
      .remove_action_ = remove_action,
      .update_action_ = update_action,
  };
  auto result = list_adapter_->UpdateFiberDataSource(
      pub::ValueImplLepus(lepus::Value(fiber_data_source.Resolve())));
  EXPECT_EQ(result.first, ListAdapterDiffResult::kRemove);
  list_adapter_->UpdateItemHolderToLatest(
      list_container_impl_->list_children_helper());
  for (int i = 0; i < list_adapter_->GetDataCount(); ++i) {
    ItemHolder* item_holder = list_adapter_->GetItemHolderForIndex(i);
    EXPECT_TRUE(item_holder->item_full_span());
    EXPECT_TRUE(item_holder->sticky_top());
    EXPECT_TRUE(item_holder->sticky_bottom());
    EXPECT_TRUE(base::FloatsEqual(item_holder->height(), 50));
    EXPECT_TRUE(item_holder->recyclable());
  }
}

TEST_F(DefaultListAdapterTest, FiberDiffCase2) {
  // test no illegal item key
  testing::InsertAction insert_action{
      .insert_ops_ = {
          {.position_ = 0, "A_0", 100, false, false, false},
          {.position_ = 1, "B_1", 100, false, false, false},
          {.position_ = 2, "C_2", 100, false, false, false},
          {.position_ = 3, "D_3", 100, false, false, false},
          {.position_ = 4, "E_4", 100, false, false, false},
      }};
  testing::FiberDataSource fiber_data_source{
      .insert_action_ = insert_action,
  };
  EXPECT_CALL(*mock_list_element_, OnErrorOccurred(_)).Times(0);
  list_adapter_->UpdateFiberDataSource(
      pub::ValueImplLepus(lepus::Value(fiber_data_source.Resolve())));
  list_adapter_->UpdateItemHolderToLatest(
      list_container_impl_->list_children_helper());

  // test illegal item key
  testing::InsertAction insert_action_1{
      .insert_ops_ = {
          {.position_ = 0, "", 100, false, false, false},
      }};
  testing::FiberDataSource fiber_data_source_1{
      .insert_action_ = insert_action_1,
  };
  EXPECT_CALL(*mock_list_element_, OnErrorOccurred(_)).Times(1);
  list_adapter_->UpdateFiberDataSource(
      pub::ValueImplLepus(lepus::Value(fiber_data_source_1.Resolve())));
}

TEST_F(DefaultListAdapterTest, FiberDiffCase3) {
  // test duplicated item key
  testing::InsertAction insert_action{
      .insert_ops_ = {
          {.position_ = 0, "A_0", 100, false, false, false},
          {.position_ = 1, "B_1", 100, false, false, false},
          {.position_ = 2, "C_2", 100, false, false, false},
          {.position_ = 3, "D_3", 100, false, false, false},
          {.position_ = 4, "D_3", 100, false, false, false},
      }};
  testing::FiberDataSource fiber_data_source{
      .insert_action_ = insert_action,
  };
  EXPECT_CALL(*mock_list_element_, OnErrorOccurred(_)).Times(1);
  list_adapter_->UpdateFiberDataSource(
      pub::ValueImplLepus(lepus::Value(fiber_data_source.Resolve())));
}

}  // namespace list
}  // namespace lynx
