// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_component/list/list_children_helper.h"

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/component_element.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
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

class ListChildrenHelperTest : public ::testing::Test {
 public:
  std::unique_ptr<lynx::tasm::ElementManager> manager;
  std::shared_ptr<::testing::NiceMock<test::MockTasmDelegate>> tasm_mediator;
  std::unique_ptr<ListChildrenHelper> list_children_helper;
  std::unordered_map<std::string, std::unique_ptr<ItemHolder>> item_holder_map;
  std::vector<fml::RefPtr<ComponentElement>> list_item_elements;
  const std::vector<std::string> item_keys = {
      "A_0", "B_1", "C_2", "D_3", "E_4", "F_5", "G_6", "H_7", "I_8", "J_9"};

  ItemHolder* GetItemHolder(const std::string& item_key) {
    if (item_holder_map.find(item_key) != item_holder_map.end()) {
      return item_holder_map[item_key].get();
    }
    return nullptr;
  }

  void Init(const std::vector<std::string>& on_screen_keys,
            const std::vector<std::string>& in_preload_keys,
            const std::vector<std::string>& in_sticky_keys,
            const std::vector<std::string>& attached_keys) {
    list_children_helper->ClearOnScreenChildren();
    list_children_helper->ClearInPreloadChildren();
    list_children_helper->ClearInStickyChildren();
    list_children_helper->ClearAttachedChildren();
    const ItemHolderSet& on_screen_children =
        list_children_helper->on_screen_children();
    const ItemHolderSet& in_preload_children =
        list_children_helper->in_preload_children();
    const ItemHolderSet& in_sticky_children =
        list_children_helper->in_sticky_children();
    for (const std::string& item_key : on_screen_keys) {
      list_children_helper->AddChild(on_screen_children,
                                     GetItemHolder(item_key));
    }
    for (const std::string& item_key : in_preload_keys) {
      list_children_helper->AddChild(in_preload_children,
                                     GetItemHolder(item_key));
    }
    for (const std::string& item_key : in_sticky_keys) {
      list_children_helper->AddChild(in_sticky_children,
                                     GetItemHolder(item_key));
    }
    for (const std::string& item_key : attached_keys) {
      ItemHolder* item_holder = GetItemHolder(item_key);
      list_children_helper->AttachChild(
          item_holder, list_item_elements[item_holder->index()].get());
    }
  }

 protected:
  ListChildrenHelperTest() = default;
  ~ListChildrenHelperTest() override = default;
  void SetUp() override {
    LynxEnvConfig lynx_env_config(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    manager = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<MockPaintingContext>(), tasm_mediator.get(),
        lynx_env_config);
    auto config = std::make_shared<PageConfig>();
    manager->SetConfig(config);

    list_children_helper = std::make_unique<ListChildrenHelper>();
    for (int i = 0; i < static_cast<int>(item_keys.size()); ++i) {
      base::String component_id(std::to_string(i));
      int32_t css_id = i;
      base::String entry_name("__ListItem__");
      base::String component_name("__ListItem__");
      base::String path("/index/components/ListItem");
      list_item_elements.emplace_back(manager->CreateFiberComponent(
          component_id, css_id, entry_name, component_name, path));
      const auto& item_key = item_keys[i];
      item_holder_map[item_key] = std::make_unique<ItemHolder>(i, item_key);
    }
  }
};  // ListChildrenHelperTest

TEST_F(ListChildrenHelperTest, HandleLayoutOrScrollResult0) {
  // Case0: No attached children.
  std::vector<std::string> on_screen_keys = {"A_0", "B_1", "C_2", "D_3"};
  std::vector<std::string> in_preload_keys = {"E_4"};
  std::vector<std::string> in_sticky_keys = {"I_8"};
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
  list_children_helper->HandleLayoutOrScrollResult(
      insert_handler, recycle_handler, update_handler);
  EXPECT_EQ(list_children_helper->last_binding_children().size(), 0);
  EXPECT_EQ(insert_times, 0);
  EXPECT_EQ(recycle_times, 0);
  EXPECT_EQ(update_times, 0);
}

TEST_F(ListChildrenHelperTest, HandleLayoutOrScrollResult1) {
  // Case1: No attached children.
  std::vector<std::string> on_screen_keys = {"A_0", "B_1", "C_2", "D_3"};
  std::vector<std::string> in_preload_keys = {"E_4"};
  std::vector<std::string> in_sticky_keys = {"I_8"};
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
  list_children_helper->HandleLayoutOrScrollResult(
      insert_handler, recycle_handler, update_handler);
  EXPECT_EQ(list_children_helper->last_binding_children().size(),
            attached_keys.size());
  EXPECT_EQ(insert_times, 6);
  EXPECT_EQ(recycle_times, 0);
  EXPECT_EQ(update_times, 6);
}

TEST_F(ListChildrenHelperTest, HandleLayoutOrScrollResult2) {
  // Case2: Test scroll
  // Before scroll
  std::vector<std::string> on_screen_keys = {"A_0", "B_1", "C_2", "D_3"};
  std::vector<std::string> in_preload_keys = {"E_4"};
  std::vector<std::string> in_sticky_keys = {"I_8"};
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
  list_children_helper->HandleLayoutOrScrollResult(
      insert_handler, recycle_handler, update_handler);
  EXPECT_EQ(list_children_helper->last_binding_children().size(),
            attached_keys.size());
  EXPECT_EQ(insert_times, 6);
  EXPECT_EQ(recycle_times, 0);
  EXPECT_EQ(update_times, 6);

  // After scroll
  on_screen_keys = {"C_2", "D_3", "E_4", "F_5"};
  in_preload_keys = {"B_1", "G_6"};
  in_sticky_keys = {"I_8", "J_9"};
  attached_keys = {"B_1", "C_2", "D_3", "E_4", "F_5", "G_6", "I_8", "J_9"};
  Init(on_screen_keys, in_preload_keys, in_sticky_keys, attached_keys);
  insert_times = 0;
  recycle_times = 0;
  update_times = 0;
  list_children_helper->HandleLayoutOrScrollResult(
      insert_handler, recycle_handler, update_handler);
  EXPECT_EQ(list_children_helper->last_binding_children().size(),
            attached_keys.size());
  EXPECT_EQ(insert_times, 3);
  EXPECT_EQ(recycle_times, 1);
  EXPECT_EQ(update_times, 8);
}

TEST_F(ListChildrenHelperTest, GetChildCount) {
  const ItemHolderSet& children = list_children_helper->children();
  for (auto& it : item_holder_map) {
    list_children_helper->AddChild(children, it.second.get());
  }
  EXPECT_EQ(list_children_helper->GetChildCount(), item_holder_map.size());
}

TEST_F(ListChildrenHelperTest, ClearChildren) {
  const ItemHolderSet& children = list_children_helper->children();
  for (auto& it : item_holder_map) {
    list_children_helper->AddChild(children, it.second.get());
  }
  EXPECT_EQ(list_children_helper->GetChildCount(), item_holder_map.size());
  list_children_helper->ClearChildren();
  EXPECT_EQ(list_children_helper->GetChildCount(), 0);
}

TEST_F(ListChildrenHelperTest, GetFirstChild) {
  const ItemHolderSet& children = list_children_helper->children();
  for (auto& it : item_holder_map) {
    list_children_helper->AddChild(children, it.second.get());
  }
  ItemHolder* item_holder =
      list_children_helper->GetFirstChild([](const ItemHolder* item_holder) {
        return item_holder->item_key() == "C_2";
      });
  EXPECT_TRUE(item_holder != nullptr);
  EXPECT_TRUE(item_holder->item_key() == "C_2");

  item_holder = list_children_helper->GetFirstChild(
      list_children_helper->children(), [](const ItemHolder* item_holder) {
        return item_holder->item_key() == "D_3";
      });
  EXPECT_TRUE(item_holder != nullptr);
  EXPECT_TRUE(item_holder->item_key() == "D_3");
}

TEST_F(ListChildrenHelperTest, GetLastChild) {
  const ItemHolderSet& children = list_children_helper->children();
  for (auto& it : item_holder_map) {
    list_children_helper->AddChild(children, it.second.get());
  }
  ItemHolder* item_holder =
      list_children_helper->GetLastChild([](const ItemHolder* item_holder) {
        return item_holder->item_key() == "G_6";
      });
  EXPECT_TRUE(item_holder != nullptr);
  EXPECT_TRUE(item_holder->item_key() == "G_6");

  item_holder = list_children_helper->GetLastChild(
      list_children_helper->children(), [](const ItemHolder* item_holder) {
        return item_holder->item_key() == "H_7";
      });
  EXPECT_TRUE(item_holder != nullptr);
  EXPECT_TRUE(item_holder->item_key() == "H_7");
}

TEST_F(ListChildrenHelperTest, AddChild) {
  const ItemHolderSet& children = list_children_helper->children();
  for (auto& it : item_holder_map) {
    list_children_helper->AddChild(children, it.second.get());
  }
  EXPECT_EQ(list_children_helper->children().size(), item_holder_map.size());
}

TEST_F(ListChildrenHelperTest, AttachChild) {
  for (int i = 0; i < static_cast<int>(item_keys.size()); ++i) {
    const auto& item_key = item_keys[i];
    list_children_helper->AttachChild(item_holder_map[item_key].get(),
                                      list_item_elements[i].get());
  }
  EXPECT_EQ(list_children_helper->attached_children().size(),
            item_holder_map.size());
  EXPECT_EQ(list_children_helper->attached_element_item_holder_map().size(),
            item_holder_map.size());
}

TEST_F(ListChildrenHelperTest, DetachChild) {
  for (int i = 0; i < static_cast<int>(item_keys.size()); ++i) {
    const auto& item_key = item_keys[i];
    list_children_helper->AttachChild(item_holder_map[item_key].get(),
                                      list_item_elements[i].get());
  }
  EXPECT_EQ(list_children_helper->attached_children().size(),
            item_holder_map.size());
  EXPECT_EQ(list_children_helper->attached_element_item_holder_map().size(),
            item_holder_map.size());
  for (int i = 0; i < static_cast<int>(item_keys.size()); ++i) {
    const auto& item_key = item_keys[i];
    list_children_helper->DetachChild(item_holder_map[item_key].get(),
                                      list_item_elements[i].get());
  }
  EXPECT_EQ(list_children_helper->attached_children().size(), 0);
  EXPECT_EQ(list_children_helper->attached_element_item_holder_map().size(), 0);
}

TEST_F(ListChildrenHelperTest, ClearAttachedChildren) {
  for (int i = 0; i < static_cast<int>(item_keys.size()); ++i) {
    const auto& item_key = item_keys[i];
    list_children_helper->AttachChild(item_holder_map[item_key].get(),
                                      list_item_elements[i].get());
  }
  list_children_helper->ClearAttachedChildren();
  EXPECT_EQ(list_children_helper->attached_children().size(), 0);
  EXPECT_EQ(list_children_helper->attached_element_item_holder_map().size(), 0);
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
