// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#define private public
#define protected public

#include "core/renderer/dom/fiber/block_element.h"

#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/renderer/dom/fiber/for_element.h"
#include "core/renderer/dom/fiber/if_element.h"
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

class BlockElementTest : public ::testing::Test {
 public:
  BlockElementTest() {}
  ~BlockElementTest() override {}
  lynx::tasm::ElementManager *manager;
  std::shared_ptr<::testing::NiceMock<test::MockTasmDelegate>> tasm_mediator;
  std::shared_ptr<lynx::tasm::TemplateAssembler> tasm;

  void SetUp() override {
    LynxEnvConfig lynx_env_config(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);

    tasm_mediator = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    auto unique_manager = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<MockPaintingContext>(), tasm_mediator.get(),
        lynx_env_config);
    manager = unique_manager.get();
    tasm = std::make_shared<lynx::tasm::TemplateAssembler>(
        *tasm_mediator.get(), std::move(unique_manager), 0);

    auto test_entry = std::make_shared<TemplateEntry>();
    tasm->template_entries_.insert({"test_entry", test_entry});

    auto config = std::make_shared<PageConfig>();
    manager->SetConfig(config);
    tasm->page_config_ = config;
  }

  fml::RefPtr<BlockElement> CreateBlockNode(const base::String &tag) {
    fml::RefPtr<BlockElement> element =
        fml::AdoptRef<BlockElement>(new BlockElement(manager, tag));
    return element;
  }

  fml::RefPtr<IfElement> CreateIfNode(const base::String &tag) {
    fml::RefPtr<IfElement> element =
        fml::AdoptRef<IfElement>(new IfElement(manager, tag));
    return element;
  }

  fml::RefPtr<ForElement> CreateForNode(const base::String &tag) {
    fml::RefPtr<ForElement> element =
        fml::AdoptRef<ForElement>(new ForElement(manager, tag));
    return element;
  }
};

TEST_F(BlockElementTest, InsertNode) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_FIBER);
  manager->SetConfig(config);

  auto parent = manager->CreateFiberNode("view");
  auto block1 = CreateBlockNode("block");
  auto child1 = manager->CreateFiberNode("view");

  parent->InsertNode(block1);
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 0);

  block1->InsertNode(child1);
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 1);

  EXPECT_EQ(parent->GetChildAt(0), child1.get());
  EXPECT_EQ(parent->scoped_virtual_children_[0].get(), block1.get());

  auto block2 = CreateBlockNode("block");
  auto child2 = manager->CreateFiberNode("view");

  block1->InsertNode(block2);
  block2->InsertNode(child2);
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 2);
  EXPECT_EQ(static_cast<int>(parent->scoped_virtual_children_.size()), 2);

  EXPECT_EQ(parent->GetChildAt(1), child2.get());
  EXPECT_EQ(parent->scoped_virtual_children_[1].get(), block2.get());

  EXPECT_EQ(static_cast<int>(block1->block_children_.size()), 2);
  EXPECT_EQ(block1->block_children_[0].get(), child1.get());
  EXPECT_EQ(block1->block_children_[1].get(), block2.get());
}

TEST_F(BlockElementTest, UpdateIfNodeIndex) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_FIBER);
  manager->SetConfig(config);

  auto parent = manager->CreateFiberNode("view");
  auto if_node = CreateIfNode("if");
  if_node->UpdateIfIndex(1);
  auto child1 = manager->CreateFiberNode("view");
  auto child2 = manager->CreateFiberNode("view");

  parent->InsertNode(if_node);
  if_node->InsertNode(child1);
  parent->InsertNode(child2);
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 2);
  EXPECT_EQ(parent->GetChildAt(0), child1.get());
  EXPECT_EQ(parent->GetChildAt(1), child2.get());

  if_node->UpdateIfIndex(-1);
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 1);
  EXPECT_EQ(parent->GetChildAt(0), child2.get());

  if_node->UpdateIfIndex(2);
  auto child3 = manager->CreateFiberNode("view");
  if_node->InsertNode(child3);
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 2);
  EXPECT_EQ(parent->GetChildAt(0), child3.get());
  EXPECT_EQ(parent->GetChildAt(1), child2.get());
}

TEST_F(BlockElementTest, UpdateForNodeCount) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_FIBER);
  manager->SetConfig(config);

  auto parent = manager->CreateFiberNode("view");
  auto for_node = CreateForNode("for");
  for_node->UpdateChildrenCount(3);
  auto child0 = manager->CreateFiberNode("view");
  auto child1 = manager->CreateFiberNode("view");
  auto child2 = manager->CreateFiberNode("view");
  auto child3 = manager->CreateFiberNode("view");

  parent->InsertNode(for_node);
  for_node->InsertNode(child1);
  for_node->InsertNode(child2);
  for_node->InsertNode(child3);
  parent->InsertNode(child0);
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 4);
  EXPECT_EQ(parent->GetChildAt(0), child1.get());
  EXPECT_EQ(parent->GetChildAt(3), child0.get());

  for_node->UpdateChildrenCount(0);
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 1);
  EXPECT_EQ(parent->GetChildAt(0), child0.get());

  auto child4 = manager->CreateFiberNode("view");
  for_node->UpdateChildrenCount(1);
  for_node->InsertNode(child4);
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 2);
  EXPECT_EQ(parent->GetChildAt(0), child4.get());
  EXPECT_EQ(parent->GetChildAt(1), child0.get());
}

TEST_F(BlockElementTest, FindInsertIndex) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_FIBER);
  manager->SetConfig(config);

  auto parent = manager->CreateFiberNode("view");
  auto if_node = CreateIfNode("if");
  if_node->UpdateIfIndex(1);
  parent->InsertNode(if_node);

  auto child1 = manager->CreateFiberNode("view");
  if_node->InsertNode(child1);

  auto child_if_node1 = CreateIfNode("if");
  child_if_node1->UpdateIfIndex(1);
  if_node->InsertNode(child_if_node1);

  auto child2 = manager->CreateFiberNode("view");
  child_if_node1->InsertNode(child2);

  auto for_node = CreateForNode("for");
  for_node->UpdateChildrenCount(1);
  child_if_node1->InsertNode(for_node);

  auto child3 = manager->CreateFiberNode("view");
  for_node->InsertNode(child3);

  auto child4 = manager->CreateFiberNode("view");
  child_if_node1->InsertNode(child4);

  auto child_if_node2 = CreateIfNode("if");
  child_if_node2->UpdateIfIndex(1);
  if_node->InsertNode(child_if_node2);

  auto child5 = manager->CreateFiberNode("view");
  child_if_node2->InsertNode(child5);

  auto child6 = manager->CreateFiberNode("view");
  if_node->InsertNode(child6);

  auto child7 = manager->CreateFiberNode("view");
  child7->set_virtual_parent(for_node.get());

  EXPECT_EQ(static_cast<int>(for_node->FindInsertIndex(child7)), 3);
}

TEST_F(BlockElementTest, GetAllNodeCountExcludeBlock) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_FIBER);
  manager->SetConfig(config);

  auto parent = manager->CreateFiberNode("view");
  auto if_node = CreateIfNode("if");
  if_node->UpdateIfIndex(1);
  parent->InsertNode(if_node);

  auto child1 = manager->CreateFiberNode("view");
  if_node->InsertNode(child1);

  auto child_if_node1 = CreateIfNode("if");
  child_if_node1->UpdateIfIndex(1);
  if_node->InsertNode(child_if_node1);

  auto child2 = manager->CreateFiberNode("view");
  child_if_node1->InsertNode(child2);

  auto for_node = CreateForNode("for");
  for_node->UpdateChildrenCount(1);
  child_if_node1->InsertNode(for_node);

  auto child3 = manager->CreateFiberNode("view");
  for_node->InsertNode(child3);

  auto child4 = manager->CreateFiberNode("view");
  child_if_node1->InsertNode(child4);

  auto child_if_node2 = CreateIfNode("if");
  child_if_node2->UpdateIfIndex(1);
  if_node->InsertNode(child_if_node2);

  auto child5 = manager->CreateFiberNode("view");
  child_if_node2->InsertNode(child5);

  auto child6 = manager->CreateFiberNode("view");
  if_node->InsertNode(child6);

  EXPECT_EQ(static_cast<int>(if_node->GetAllNodeCountExcludeBlock()), 6);
}

TEST_F(BlockElementTest, UpdateBlockNodeCase1) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_FIBER);
  manager->SetConfig(config);

  auto parent = manager->CreateFiberNode("view");
  auto if_node = CreateIfNode("if");
  if_node->UpdateIfIndex(1);
  parent->InsertNode(if_node);

  auto child1 = manager->CreateFiberNode("view");
  if_node->InsertNode(child1);
  auto for_node = CreateForNode("for");
  for_node->UpdateChildrenCount(1);
  if_node->InsertNode(for_node);
  auto child3 = manager->CreateFiberNode("view");
  for_node->InsertNode(child3);

  auto child_if_node = CreateIfNode("if");
  child_if_node->UpdateIfIndex(1);
  if_node->InsertNode(child_if_node);
  auto child4 = manager->CreateFiberNode("view");
  child_if_node->InsertNode(child4);

  auto child2 = manager->CreateFiberNode("view");
  if_node->InsertNode(child2);

  auto child0 = manager->CreateFiberNode("view");
  parent->InsertNode(child0);

  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 5);
  EXPECT_EQ(parent->GetChildAt(0), child1.get());
  EXPECT_EQ(parent->GetChildAt(1), child3.get());
  EXPECT_EQ(parent->GetChildAt(2), child4.get());
  EXPECT_EQ(parent->GetChildAt(3), child2.get());
  EXPECT_EQ(parent->GetChildAt(4), child0.get());

  auto child5 = manager->CreateFiberNode("view");
  for_node->UpdateChildrenCount(2);
  for_node->InsertNode(child5);
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 6);
  EXPECT_EQ(parent->GetChildAt(0), child1.get());
  EXPECT_EQ(parent->GetChildAt(1), child3.get());
  EXPECT_EQ(parent->GetChildAt(2), child5.get());
  EXPECT_EQ(parent->GetChildAt(3), child4.get());
  EXPECT_EQ(parent->GetChildAt(4), child2.get());
  EXPECT_EQ(parent->GetChildAt(5), child0.get());
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
