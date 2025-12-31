// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/list/decoupled_list_children_helper.h"

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

class ListChildrenHelperTest : public ::testing::Test {
 public:
  ListChildrenHelperTest() = default;
  ~ListChildrenHelperTest() override = default;

  std::unique_ptr<MockListElement> mock_list_element_{nullptr};
  std::unique_ptr<ListContainerImpl> list_container_impl_{nullptr};
  std::shared_ptr<pub::PubValueFactoryDefault> value_factory_{nullptr};
  ListLayoutManager* list_layout_manager_{nullptr};
  ListAdapter* list_adapter_{nullptr};
  ListChildrenHelper* list_children_helper_{nullptr};

  void SetUp() override {
    value_factory_ = std::make_shared<pub::PubValueFactoryDefault>();
    mock_list_element_ = std::make_unique<MockListElement>();
    list_container_impl_ = std::make_unique<ListContainerImpl>(
        mock_list_element_.get(), value_factory_);
    list_layout_manager_ = list_container_impl_->list_layout_manager();
    list_adapter_ = list_container_impl_->list_adapter();
    list_children_helper_ = list_container_impl_->list_children_helper();
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

  ItemHolder* GetItemHolderForKey(const std::string& item_key) {
    const auto& item_holder_map = list_adapter_->item_holder_map_;
    if (item_holder_map->end() != item_holder_map->find(item_key)) {
      return ((*item_holder_map)[item_key]).get();
    }
    return nullptr;
  }

  void Init(const std::vector<std::string>& on_screen_keys,
            const std::vector<std::string>& in_preload_keys,
            const std::vector<std::pair<std::string, bool>>& in_sticky_keys,
            const std::vector<std::string>& attached_keys) {
    list_children_helper_->ClearOnScreenChildren();
    list_children_helper_->ClearInPreloadChildren();
    list_children_helper_->ClearInStickyChildren();
    list_children_helper_->ClearAttachedChildren();
    const ItemHolderSet& on_screen_children =
        list_children_helper_->on_screen_children();
    const ItemHolderSet& in_preload_children =
        list_children_helper_->in_preload_children();
    const ItemHolderSet& in_sticky_children =
        list_children_helper_->in_sticky_children();
    for (const std::string& item_key : on_screen_keys) {
      list_children_helper_->AddChild(on_screen_children,
                                      GetItemHolderForKey(item_key));
    }
    for (const std::string& item_key : in_preload_keys) {
      list_children_helper_->AddChild(in_preload_children,
                                      GetItemHolderForKey(item_key));
    }
    if (list_children_helper_->recycle_item_holder_) {
      list_children_helper_->InitStickyItemHolderSet(
          base::ThreadStrategyForRendering::ALL_ON_UI);
    }
    for (const auto& item : in_sticky_keys) {
      if (list_children_helper_->recycle_item_holder_) {
        if (item.second) {
          list_children_helper_->in_sticky_top_children_.AddItemHolder(
              GetItemHolderForKey(item.first));
        } else {
          list_children_helper_->in_sticky_bottom_children_.AddItemHolder(
              GetItemHolderForKey(item.first));
        }
      } else {
        list_children_helper_->AddChild(in_sticky_children,
                                        GetItemHolderForKey(item.first));
      }
    }
    mock_list_element_->ClearListItemElements();
    int list_item_element_id = mock_list_element_->GetImplId() + 1;
    for (const std::string& item_key : attached_keys) {
      ItemHolder* item_holder = GetItemHolderForKey(item_key);
      mock_list_element_->AddListItemElement(
          item_key,
          std::make_unique<MockListItemElement>(list_item_element_id++));
      list_children_helper_->AttachChild(
          item_holder, mock_list_element_->GetListItemElement(item_key));
    }
  }
};

// Case 1. Has no attached children.
TEST_F(ListChildrenHelperTest, HandleLayoutOrScrollResult0) {
  InitFiberDataSource();
  std::vector<std::string> on_screen_keys = {"A_0", "B_1", "C_2", "D_3"};
  std::vector<std::string> in_preload_keys = {"E_4"};
  std::vector<std::pair<std::string, bool>> in_sticky_keys = {{"I_8", false}};
  Init(on_screen_keys, in_preload_keys, in_sticky_keys, {});
  int insert_times = 0;
  int recycle_times = 0;
  int update_times = 0;
  auto insert_handler = [&insert_times](ItemHolder* item_holder) {
    insert_times += 1;
    return false;
  };
  auto recycle_handler = [&recycle_times](ItemHolder* item_holder) {
    recycle_times += 1;
    return false;
  };
  auto update_handler = [&update_times](ItemHolder* item_holder) {
    update_times += 1;
    return false;
  };
  list_children_helper_->HandleLayoutOrScrollResult(
      insert_handler, recycle_handler, update_handler);
  EXPECT_EQ(list_children_helper_->last_binding_children().size(), 0);
  EXPECT_EQ(insert_times, 0);
  EXPECT_EQ(recycle_times, 0);
  EXPECT_EQ(update_times, 0);
}

// Case 1. Has attached children.
TEST_F(ListChildrenHelperTest, HandleLayoutOrScrollResult1) {
  InitFiberDataSource();
  std::vector<std::string> on_screen_keys = {"A_0", "B_1", "C_2", "D_3"};
  std::vector<std::string> in_preload_keys = {"E_4"};
  std::vector<std::pair<std::string, bool>> in_sticky_keys = {{"I_8", false}};
  std::vector<std::string> attached_keys = {"A_0", "B_1", "C_2",
                                            "D_3", "E_4", "I_8"};
  Init(on_screen_keys, in_preload_keys, in_sticky_keys, attached_keys);
  int insert_times = 0;
  int recycle_times = 0;
  int update_times = 0;
  auto insert_handler = [&insert_times](ItemHolder* item_holder) {
    insert_times += 1;
    return false;
  };
  auto recycle_handler = [&recycle_times](ItemHolder* item_holder) {
    recycle_times += 1;
    return false;
  };
  auto update_handler = [&update_times](ItemHolder* item_holder) {
    update_times += 1;
    return false;
  };
  list_children_helper_->HandleLayoutOrScrollResult(
      insert_handler, recycle_handler, update_handler);
  EXPECT_EQ(list_children_helper_->last_binding_children().size(),
            attached_keys.size());
  EXPECT_EQ(insert_times, 6);
  EXPECT_EQ(recycle_times, 0);
  EXPECT_EQ(update_times, 6);
}

// Case2: Test scroll
TEST_F(ListChildrenHelperTest, HandleLayoutOrScrollResult2) {
  InitFiberDataSource();
  // Before scroll
  std::vector<std::string> on_screen_keys = {"A_0", "B_1", "C_2", "D_3"};
  std::vector<std::string> in_preload_keys = {"E_4"};
  std::vector<std::pair<std::string, bool>> in_sticky_keys = {{"I_8", false}};
  std::vector<std::string> attached_keys = {"A_0", "B_1", "C_2",
                                            "D_3", "E_4", "I_8"};
  Init(on_screen_keys, in_preload_keys, in_sticky_keys, attached_keys);
  int insert_times = 0;
  int recycle_times = 0;
  int update_times = 0;
  auto insert_handler = [&insert_times](ItemHolder* item_holder) {
    insert_times += 1;
    return false;
  };
  auto recycle_handler = [&recycle_times](ItemHolder* item_holder) {
    recycle_times += 1;
    return false;
  };
  auto update_handler = [&update_times](ItemHolder* item_holder) {
    update_times += 1;
    return false;
  };
  list_children_helper_->HandleLayoutOrScrollResult(
      insert_handler, recycle_handler, update_handler);
  EXPECT_EQ(list_children_helper_->last_binding_children().size(),
            attached_keys.size());
  EXPECT_EQ(insert_times, 6);
  EXPECT_EQ(recycle_times, 0);
  EXPECT_EQ(update_times, 6);

  // After scroll
  on_screen_keys = {"C_2", "D_3", "E_4", "F_5"};
  in_preload_keys = {"B_1", "G_6"};
  in_sticky_keys = {{"I_8", false}, {"J_9", false}};
  attached_keys = {"B_1", "C_2", "D_3", "E_4", "F_5", "G_6", "I_8", "J_9"};
  Init(on_screen_keys, in_preload_keys, in_sticky_keys, attached_keys);
  insert_times = 0;
  recycle_times = 0;
  update_times = 0;
  list_children_helper_->HandleLayoutOrScrollResult(
      insert_handler, recycle_handler, update_handler);
  EXPECT_EQ(list_children_helper_->last_binding_children().size(),
            attached_keys.size());
  EXPECT_EQ(insert_times, 3);
  EXPECT_EQ(recycle_times, 1);
  EXPECT_EQ(update_times, 8);
}

// Case3: Enable recycle item holder
TEST_F(ListChildrenHelperTest, HandleLayoutOrScrollResult3) {
  InitFiberDataSource();
  // Before scroll
  std::vector<std::string> on_screen_keys = {"B_1", "C_2", "D_3", "E_4"};
  std::vector<std::string> in_preload_keys = {};
  std::vector<std::pair<std::string, bool>> in_sticky_keys = {{"A_0", true}};
  std::vector<std::string> attached_keys = {"A_0", "B_1", "C_2", "D_3", "E_4"};
  Init(on_screen_keys, in_preload_keys, in_sticky_keys, attached_keys);
  int insert_times = 0;
  int recycle_times = 0;
  int update_times = 0;
  auto insert_handler = [&insert_times](ItemHolder* item_holder) {
    insert_times += 1;
    return false;
  };
  auto recycle_handler = [&recycle_times](ItemHolder* item_holder) {
    recycle_times += 1;
    return false;
  };
  auto update_handler = [&update_times](ItemHolder* item_holder) {
    update_times += 1;
    return false;
  };
  list_children_helper_->HandleLayoutOrScrollResult(
      insert_handler, recycle_handler, update_handler);
  EXPECT_EQ(list_children_helper_->last_binding_children().size(),
            attached_keys.size());
  EXPECT_EQ(insert_times, 5);
  EXPECT_EQ(recycle_times, 0);
  EXPECT_EQ(update_times, 5);

  // After scroll
  on_screen_keys = {"D_3", "E_4", "F_5", "G_6"};
  in_preload_keys = {};
  in_sticky_keys = {{"C_2", true}};
  attached_keys = {"C_2", "D_3", "E_4", "F_5", "G_6"};
  Init(on_screen_keys, in_preload_keys, in_sticky_keys, attached_keys);
  insert_times = 0;
  recycle_times = 0;
  update_times = 0;
  list_children_helper_->HandleLayoutOrScrollResult(
      insert_handler, recycle_handler, update_handler);
  EXPECT_EQ(list_children_helper_->last_binding_children().size(),
            attached_keys.size());
  EXPECT_EQ(insert_times, 2);
  EXPECT_EQ(recycle_times, 2);
  EXPECT_EQ(update_times, 5);
}

TEST_F(ListChildrenHelperTest, AddChild) {
  InitFiberDataSource();
  EXPECT_EQ(list_children_helper_->children().size(),
            list_container_impl_->GetDataCount());
}

TEST_F(ListChildrenHelperTest, AttachChild) {
  InitFiberDataSource();
  int list_item_element_id = mock_list_element_->GetImplId() + 1;
  for (int i = 0; i < list_container_impl_->GetDataCount(); ++i) {
    ItemHolder* item_holder = list_container_impl_->GetItemHolderForIndex(i);
    const std::string& item_key = item_holder->item_key();
    mock_list_element_->AddListItemElement(
        item_key,
        std::make_unique<MockListItemElement>(list_item_element_id++));
    list_children_helper_->AttachChild(
        item_holder, mock_list_element_->GetListItemElement(item_key));
  }
  EXPECT_EQ(list_children_helper_->attached_children().size(),
            list_container_impl_->GetDataCount());
  EXPECT_EQ(list_children_helper_->attached_delegate_item_holder_map().size(),
            list_container_impl_->GetDataCount());
}

TEST_F(ListChildrenHelperTest, DetachChild) {
  InitFiberDataSource();
  int list_item_element_id = mock_list_element_->GetImplId() + 1;
  for (int i = 0; i < list_container_impl_->GetDataCount(); ++i) {
    ItemHolder* item_holder = list_container_impl_->GetItemHolderForIndex(i);
    const std::string& item_key = item_holder->item_key();
    mock_list_element_->AddListItemElement(
        item_key,
        std::make_unique<MockListItemElement>(list_item_element_id++));
    list_children_helper_->AttachChild(
        item_holder, mock_list_element_->GetListItemElement(item_key));
  }
  EXPECT_EQ(list_children_helper_->attached_children().size(),
            list_container_impl_->GetDataCount());
  EXPECT_EQ(list_children_helper_->attached_delegate_item_holder_map().size(),
            list_container_impl_->GetDataCount());
  for (int i = 0; i < list_container_impl_->GetDataCount(); ++i) {
    ItemHolder* item_holder = list_container_impl_->GetItemHolderForIndex(i);
    const std::string& item_key = item_holder->item_key();
    list_children_helper_->DetachChild(
        item_holder, mock_list_element_->GetListItemElement(item_key));
  }
  EXPECT_EQ(list_children_helper_->attached_children().size(), 0);
  EXPECT_EQ(list_children_helper_->attached_delegate_item_holder_map().size(),
            0);
}

TEST_F(ListChildrenHelperTest, ClearAttachedChildren) {
  InitFiberDataSource();
  int list_item_element_id = mock_list_element_->GetImplId() + 1;
  for (int i = 0; i < list_container_impl_->GetDataCount(); ++i) {
    ItemHolder* item_holder = list_container_impl_->GetItemHolderForIndex(i);
    const std::string& item_key = item_holder->item_key();
    mock_list_element_->AddListItemElement(
        item_key,
        std::make_unique<MockListItemElement>(list_item_element_id++));
    list_children_helper_->AttachChild(
        item_holder, mock_list_element_->GetListItemElement(item_key));
  }
  list_children_helper_->ClearAttachedChildren();
  EXPECT_EQ(list_children_helper_->attached_children().size(), 0);
  EXPECT_EQ(list_children_helper_->attached_delegate_item_holder_map().size(),
            0);
}

TEST_F(ListChildrenHelperTest, GetFirstChild) {
  InitFiberDataSource();
  ItemHolder* item_holder =
      list_children_helper_->GetFirstChild([](const ItemHolder* item_holder) {
        return item_holder->item_key() == "C_2";
      });
  EXPECT_TRUE(item_holder != nullptr);
  EXPECT_TRUE(item_holder->item_key() == "C_2");

  item_holder = list_children_helper_->GetFirstChild(
      list_children_helper_->children(), [](const ItemHolder* item_holder) {
        return item_holder->item_key() == "D_3";
      });
  EXPECT_TRUE(item_holder != nullptr);
  EXPECT_TRUE(item_holder->item_key() == "D_3");
}

TEST_F(ListChildrenHelperTest, GetLastChild) {
  InitFiberDataSource();
  ItemHolder* item_holder =
      list_children_helper_->GetLastChild([](const ItemHolder* item_holder) {
        return item_holder->item_key() == "G_6";
      });
  EXPECT_TRUE(item_holder != nullptr);
  EXPECT_TRUE(item_holder->item_key() == "G_6");

  item_holder = list_children_helper_->GetLastChild(
      list_children_helper_->children(), [](const ItemHolder* item_holder) {
        return item_holder->item_key() == "H_7";
      });
  EXPECT_TRUE(item_holder != nullptr);
  EXPECT_TRUE(item_holder->item_key() == "H_7");
}

TEST_F(ListChildrenHelperTest, GetFirstChildFrom) {
  InitFiberDataSource();
  const ItemHolderSet& children = list_children_helper_->children();
  // 1. Forward search
  // Find an element with valid item key that exists after the starting point.
  ItemHolder* start_child = GetItemHolderForKey("C_2");
  ItemHolder* result = list_children_helper_->GetFirstChildFrom(
      children, start_child,
      [](const ItemHolder* item) {
        return !item->item_key().empty() && item->item_key() != "C_2";
      },
      false);
  EXPECT_NE(result, nullptr);
  EXPECT_EQ(result->item_key(), "D_3");

  // Try to find an element that only exists before the starting point.
  result = list_children_helper_->GetFirstChildFrom(
      children, start_child,
      [](const ItemHolder* item) { return item->item_key() == "A_0"; }, false);
  EXPECT_EQ(result, nullptr);

  // The condition is met by the starting element itself.
  result = list_children_helper_->GetFirstChildFrom(
      children, start_child,
      [](const ItemHolder* item) {
        return !item->item_key().empty() && item->item_key() == "C_2";
      },
      false);
  EXPECT_NE(result, nullptr);
  EXPECT_EQ(result->item_key(), "C_2");

  // No element meets the condition.
  result = list_children_helper_->GetFirstChildFrom(
      children, start_child,
      [](const ItemHolder* item) { return item->item_key() == "Z_25"; }, false);
  EXPECT_EQ(result, nullptr);

  // 2. Reverse search
  // Find an element that exists before the starting point.
  start_child = GetItemHolderForKey("G_6");
  result = list_children_helper_->GetFirstChildFrom(
      children, start_child,
      [](const ItemHolder* item) {
        return !item->item_key().empty() && item->item_key() != "G_6";
      },
      true);
  EXPECT_NE(result, nullptr);
  EXPECT_EQ(result->item_key(), "F_5");

  // Try to find an element that only exists after the starting point.
  result = list_children_helper_->GetFirstChildFrom(
      children, start_child,
      [](const ItemHolder* item) { return item->item_key() == "I_8"; }, true);
  EXPECT_EQ(result, nullptr);

  // The condition is met by the starting element itself.
  result = list_children_helper_->GetFirstChildFrom(
      children, start_child,
      [](const ItemHolder* item) {
        return !item->item_key().empty() && item->item_key() == "G_6";
      },
      true);
  EXPECT_NE(result, nullptr);
  EXPECT_EQ(result->item_key(), "G_6");

  // No element meets the condition.
  result = list_children_helper_->GetFirstChildFrom(
      children, start_child,
      [](const ItemHolder* item) { return item->item_key() == "Z_25"; }, true);
  EXPECT_EQ(result, nullptr);

  // 3. Edge cases
  // start_child is not in children.
  auto temp_holder = std::make_unique<ItemHolder>(
      99, "NotInSet", list_container_impl_->list_animation_manager());
  result = list_children_helper_->GetFirstChildFrom(
      children, temp_holder.get(),
      [](const ItemHolder* item) { return !item->item_key().empty(); }, false);
  EXPECT_EQ(result, nullptr);

  // 4. The set is empty.
  start_child = GetItemHolderForKey("F_5");
  list_children_helper_->ClearChildren();
  EXPECT_EQ(list_children_helper_->children().size(), 0);
  result = list_children_helper_->GetFirstChildFrom(
      children, start_child,
      [](const ItemHolder* item) { return !item->item_key().empty(); }, false);
  EXPECT_EQ(result, nullptr);

  // 5. The set only has one element.
  start_child = GetItemHolderForKey("F_5");
  list_children_helper_->ClearChildren();
  list_children_helper_->AddChild(children, start_child);
  EXPECT_EQ(list_children_helper_->children().size(), 1);
  result = list_children_helper_->GetFirstChildFrom(
      children, start_child,
      [](const ItemHolder* item) {
        return !item->item_key().empty() && item->item_key() == "F_5";
      },
      false);
  EXPECT_NE(result, nullptr);
  EXPECT_EQ(result->item_key(), "F_5");

  result = list_children_helper_->GetFirstChildFrom(
      children, start_child,
      [](const ItemHolder* item) {
        return !item->item_key().empty() && item->item_key() != "F_5";
      },
      false);
  EXPECT_EQ(result, nullptr);

  result = list_children_helper_->GetFirstChildFrom(
      children, start_child,
      [](const ItemHolder* item) {
        return !item->item_key().empty() && item->item_key() == "F_5";
      },
      true);
  EXPECT_NE(result, nullptr);
  EXPECT_EQ(result->item_key(), "F_5");

  result = list_children_helper_->GetFirstChildFrom(
      children, start_child,
      [](const ItemHolder* item) {
        return !item->item_key().empty() && item->item_key() != "F_5";
      },
      true);
  EXPECT_EQ(result, nullptr);
}

}  // namespace list
}  // namespace lynx
