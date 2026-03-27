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
        *tasm_mediator.get(), std::move(unique_manager), tasm_mediator.get(),
        0);

    auto test_entry = std::make_shared<TemplateEntry>();
    tasm->template_entries_.insert({"test_entry", test_entry});

    auto config = std::make_shared<PageConfig>();
    config->SetEnableFiberArch(true);
    config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_FIBER);
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
  auto parent = manager->CreateFiberNode("view");
  auto block1 = CreateBlockNode("block");
  auto child1 = manager->CreateFiberNode("view");

  parent->InsertNode(block1);
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 0);

  block1->InsertNode(child1);
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 1);

  EXPECT_EQ(parent->GetChildAt(0), child1.get());
  EXPECT_EQ((*parent->scoped_virtual_children_)[0].get(), block1.get());

  auto block2 = CreateBlockNode("block");
  auto child2 = manager->CreateFiberNode("view");

  block1->InsertNode(block2);
  block2->InsertNode(child2);
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 2);
  EXPECT_EQ(static_cast<int>(parent->scoped_virtual_children_->size()), 2);

  EXPECT_EQ(parent->GetChildAt(1), child2.get());
  EXPECT_EQ((*parent->scoped_virtual_children_)[1].get(), block2.get());

  EXPECT_EQ(static_cast<int>(block1->block_children_.size()), 2);
  EXPECT_EQ(block1->block_children_[0].get(), child1.get());
  EXPECT_EQ(block1->block_children_[1].get(), block2.get());
}

TEST_F(BlockElementTest, UpdateIfNodeIndex) {
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

// 1. Basic scenario test

TEST_F(BlockElementTest, ReplaceElements_SimpleReorder) {
  auto parent = manager->CreateFiberNode("view");
  auto block = CreateBlockNode("block");
  parent->InsertNode(block);

  auto childA = manager->CreateFiberNode("view");
  auto childB = manager->CreateFiberNode("view");
  auto childC = manager->CreateFiberNode("view");
  auto childD = manager->CreateFiberNode("view");

  block->InsertNode(childA);
  block->InsertNode(childB);
  block->InsertNode(childC);
  block->InsertNode(childD);

  EXPECT_EQ(parent->GetChildCount(), 4);

  base::Vector<fml::RefPtr<FiberElement>> inserted = {childD, childB, childA,
                                                      childC};
  base::Vector<fml::RefPtr<FiberElement>> removed = {childA, childB, childC,
                                                     childD};
  block->ReplaceElements(inserted, removed);

  EXPECT_EQ(parent->GetChildCount(), 4);
  EXPECT_EQ(parent->GetChildAt(0), childD.get());
  EXPECT_EQ(parent->GetChildAt(1), childB.get());
  EXPECT_EQ(parent->GetChildAt(2), childA.get());
  EXPECT_EQ(parent->GetChildAt(3), childC.get());

  EXPECT_EQ(block->block_children_.size(), 4);
  EXPECT_EQ(block->block_children_[0].get(), childD.get());
  EXPECT_EQ(block->block_children_[1].get(), childB.get());
  EXPECT_EQ(block->block_children_[2].get(), childA.get());
  EXPECT_EQ(block->block_children_[3].get(), childC.get());
}

TEST_F(BlockElementTest, ReplaceElements_SimpleDelete) {
  auto parent = manager->CreateFiberNode("view");
  auto block = CreateBlockNode("block");
  parent->InsertNode(block);

  auto childA = manager->CreateFiberNode("view");
  auto childB = manager->CreateFiberNode("view");
  auto childC = manager->CreateFiberNode("view");
  auto childD = manager->CreateFiberNode("view");

  block->InsertNode(childA);
  block->InsertNode(childB);
  block->InsertNode(childC);
  block->InsertNode(childD);

  EXPECT_EQ(parent->GetChildCount(), 4);

  base::Vector<fml::RefPtr<FiberElement>> inserted = {childA, childD};
  base::Vector<fml::RefPtr<FiberElement>> removed = {childA, childB, childC,
                                                     childD};
  block->ReplaceElements(inserted, removed);

  EXPECT_EQ(parent->GetChildCount(), 2);
  EXPECT_EQ(parent->GetChildAt(0), childA.get());
  EXPECT_EQ(parent->GetChildAt(1), childD.get());

  EXPECT_EQ(block->block_children_.size(), 2);
  EXPECT_EQ(block->block_children_[0].get(), childA.get());
  EXPECT_EQ(block->block_children_[1].get(), childD.get());
}

TEST_F(BlockElementTest, ReplaceElements_SimpleInsert) {
  auto parent = manager->CreateFiberNode("view");
  auto block = CreateBlockNode("block");
  parent->InsertNode(block);

  auto childA = manager->CreateFiberNode("view");
  auto childB = manager->CreateFiberNode("view");

  block->InsertNode(childA);
  block->InsertNode(childB);

  EXPECT_EQ(parent->GetChildCount(), 2);

  auto childC = manager->CreateFiberNode("view");
  auto childD = manager->CreateFiberNode("view");

  base::Vector<fml::RefPtr<FiberElement>> inserted = {childA, childB, childC,
                                                      childD};
  base::Vector<fml::RefPtr<FiberElement>> removed = {childA, childB};
  block->ReplaceElements(inserted, removed);

  EXPECT_EQ(parent->GetChildCount(), 4);
  EXPECT_EQ(parent->GetChildAt(0), childA.get());
  EXPECT_EQ(parent->GetChildAt(1), childB.get());
  EXPECT_EQ(parent->GetChildAt(2), childC.get());
  EXPECT_EQ(parent->GetChildAt(3), childD.get());

  EXPECT_EQ(block->block_children_.size(), 4);
}

// 2.  Mixed operations test

TEST_F(BlockElementTest, ReplaceElements_DeleteAndReorder) {
  auto parent = manager->CreateFiberNode("view");
  auto block = CreateBlockNode("block");
  parent->InsertNode(block);

  auto childA = manager->CreateFiberNode("view");
  auto childB = manager->CreateFiberNode("view");
  auto childC = manager->CreateFiberNode("view");
  auto childD = manager->CreateFiberNode("view");
  auto childE = manager->CreateFiberNode("view");

  block->InsertNode(childA);
  block->InsertNode(childB);
  block->InsertNode(childC);
  block->InsertNode(childD);
  block->InsertNode(childE);

  EXPECT_EQ(parent->GetChildCount(), 5);

  base::Vector<fml::RefPtr<FiberElement>> inserted = {childD, childB, childE};
  base::Vector<fml::RefPtr<FiberElement>> removed = {childA, childB, childC,
                                                     childD, childE};
  block->ReplaceElements(inserted, removed);

  EXPECT_EQ(parent->GetChildCount(), 3);
  EXPECT_EQ(parent->GetChildAt(0), childD.get());
  EXPECT_EQ(parent->GetChildAt(1), childB.get());
  EXPECT_EQ(parent->GetChildAt(2), childE.get());

  EXPECT_EQ(block->block_children_.size(), 3);
}

TEST_F(BlockElementTest, ReplaceElements_InsertAndReorder) {
  auto parent = manager->CreateFiberNode("view");
  auto block = CreateBlockNode("block");
  parent->InsertNode(block);

  auto childA = manager->CreateFiberNode("view");
  auto childB = manager->CreateFiberNode("view");
  auto childC = manager->CreateFiberNode("view");

  block->InsertNode(childA);
  block->InsertNode(childB);
  block->InsertNode(childC);

  auto childD = manager->CreateFiberNode("view");
  auto childE = manager->CreateFiberNode("view");

  base::Vector<fml::RefPtr<FiberElement>> inserted = {childC, childD, childA,
                                                      childE, childB};
  base::Vector<fml::RefPtr<FiberElement>> removed = {childA, childB, childC};
  block->ReplaceElements(inserted, removed);

  EXPECT_EQ(parent->GetChildCount(), 5);
  EXPECT_EQ(parent->GetChildAt(0), childC.get());
  EXPECT_EQ(parent->GetChildAt(1), childD.get());
  EXPECT_EQ(parent->GetChildAt(2), childA.get());
  EXPECT_EQ(parent->GetChildAt(3), childE.get());
  EXPECT_EQ(parent->GetChildAt(4), childB.get());

  EXPECT_EQ(block->block_children_.size(), 5);
}

TEST_F(BlockElementTest, ReplaceElements_DeleteInsertReorder) {
  auto parent = manager->CreateFiberNode("view");
  auto block = CreateBlockNode("block");
  parent->InsertNode(block);

  auto childA = manager->CreateFiberNode("view");
  auto childB = manager->CreateFiberNode("view");
  auto childC = manager->CreateFiberNode("view");
  auto childD = manager->CreateFiberNode("view");
  auto childE = manager->CreateFiberNode("view");

  block->InsertNode(childA);
  block->InsertNode(childB);
  block->InsertNode(childC);
  block->InsertNode(childD);
  block->InsertNode(childE);

  auto childF = manager->CreateFiberNode("view");
  auto childG = manager->CreateFiberNode("view");

  base::Vector<fml::RefPtr<FiberElement>> inserted = {childE, childF, childB,
                                                      childG};
  base::Vector<fml::RefPtr<FiberElement>> removed = {childA, childB, childC,
                                                     childD, childE};
  block->ReplaceElements(inserted, removed);

  EXPECT_EQ(parent->GetChildCount(), 4);
  EXPECT_EQ(parent->GetChildAt(0), childE.get());
  EXPECT_EQ(parent->GetChildAt(1), childF.get());
  EXPECT_EQ(parent->GetChildAt(2), childB.get());
  EXPECT_EQ(parent->GetChildAt(3), childG.get());

  EXPECT_EQ(block->block_children_.size(), 4);
}

// 3. Boundary conditions test

TEST_F(BlockElementTest, ReplaceElements_EmptyToEmpty) {
  auto parent = manager->CreateFiberNode("view");
  auto block = CreateBlockNode("block");
  parent->InsertNode(block);

  EXPECT_EQ(parent->GetChildCount(), 0);

  base::Vector<fml::RefPtr<FiberElement>> inserted = {};
  base::Vector<fml::RefPtr<FiberElement>> removed = {};
  block->ReplaceElements(inserted, removed);

  EXPECT_EQ(parent->GetChildCount(), 0);
  EXPECT_EQ(block->block_children_.size(), 0);
}

TEST_F(BlockElementTest, ReplaceElements_EmptyToNonEmpty) {
  auto parent = manager->CreateFiberNode("view");
  auto block = CreateBlockNode("block");
  parent->InsertNode(block);

  auto childA = manager->CreateFiberNode("view");
  auto childB = manager->CreateFiberNode("view");
  auto childC = manager->CreateFiberNode("view");

  base::Vector<fml::RefPtr<FiberElement>> inserted = {childA, childB, childC};
  base::Vector<fml::RefPtr<FiberElement>> removed = {};
  block->ReplaceElements(inserted, removed);

  EXPECT_EQ(parent->GetChildCount(), 3);
  EXPECT_EQ(parent->GetChildAt(0), childA.get());
  EXPECT_EQ(parent->GetChildAt(1), childB.get());
  EXPECT_EQ(parent->GetChildAt(2), childC.get());

  EXPECT_EQ(block->block_children_.size(), 3);
}

TEST_F(BlockElementTest, ReplaceElements_NonEmptyToEmpty) {
  auto parent = manager->CreateFiberNode("view");
  auto block = CreateBlockNode("block");
  parent->InsertNode(block);

  auto childA = manager->CreateFiberNode("view");
  auto childB = manager->CreateFiberNode("view");
  auto childC = manager->CreateFiberNode("view");

  block->InsertNode(childA);
  block->InsertNode(childB);
  block->InsertNode(childC);

  EXPECT_EQ(parent->GetChildCount(), 3);

  base::Vector<fml::RefPtr<FiberElement>> inserted = {};
  base::Vector<fml::RefPtr<FiberElement>> removed = {childA, childB, childC};
  block->ReplaceElements(inserted, removed);

  EXPECT_EQ(parent->GetChildCount(), 0);
  EXPECT_EQ(block->block_children_.size(), 0);
}

TEST_F(BlockElementTest, ReplaceElements_SingleElement) {
  auto parent = manager->CreateFiberNode("view");
  auto block = CreateBlockNode("block");
  parent->InsertNode(block);

  auto childA = manager->CreateFiberNode("view");
  block->InsertNode(childA);

  auto childB = manager->CreateFiberNode("view");

  base::Vector<fml::RefPtr<FiberElement>> inserted = {childB};
  base::Vector<fml::RefPtr<FiberElement>> removed = {childA};
  block->ReplaceElements(inserted, removed);

  EXPECT_EQ(parent->GetChildCount(), 1);
  EXPECT_EQ(parent->GetChildAt(0), childB.get());
  EXPECT_EQ(block->block_children_.size(), 1);
  EXPECT_EQ(block->block_children_[0].get(), childB.get());
}

// 4. Nested BlockElement test

TEST_F(BlockElementTest, ReplaceElements_WithNestedBlock) {
  auto parent = manager->CreateFiberNode("view");
  auto block = CreateBlockNode("block");
  parent->InsertNode(block);

  auto child1 = manager->CreateFiberNode("view");
  auto child2 = manager->CreateFiberNode("view");
  auto child3 = manager->CreateFiberNode("view");
  auto child4 = manager->CreateFiberNode("view");

  auto nested_block = CreateBlockNode("nested_block");

  block->InsertNode(child1);
  block->InsertNode(nested_block);
  nested_block->InsertNode(child2);
  nested_block->InsertNode(child3);
  block->InsertNode(child4);

  EXPECT_EQ(parent->GetChildCount(), 4);

  base::Vector<fml::RefPtr<FiberElement>> inserted = {nested_block, child1,
                                                      child4};
  base::Vector<fml::RefPtr<FiberElement>> removed = {child1, nested_block,
                                                     child4};
  block->ReplaceElements(inserted, removed);

  EXPECT_EQ(parent->GetChildCount(), 4);
  EXPECT_EQ(parent->GetChildAt(0), child2.get());
  EXPECT_EQ(parent->GetChildAt(1), child3.get());
  EXPECT_EQ(parent->GetChildAt(2), child1.get());
  EXPECT_EQ(parent->GetChildAt(3), child4.get());

  EXPECT_EQ(block->block_children_.size(), 3);
  EXPECT_EQ(block->block_children_[0].get(), nested_block.get());
  EXPECT_EQ(block->block_children_[1].get(), child1.get());
  EXPECT_EQ(block->block_children_[2].get(), child4.get());
}

TEST_F(BlockElementTest, ReplaceElements_DeleteNestedBlock) {
  auto parent = manager->CreateFiberNode("view");
  auto block = CreateBlockNode("block");
  parent->InsertNode(block);

  auto child1 = manager->CreateFiberNode("view");
  auto child2 = manager->CreateFiberNode("view");
  auto child3 = manager->CreateFiberNode("view");
  auto child4 = manager->CreateFiberNode("view");

  auto nested_block = CreateBlockNode("nested_block");

  block->InsertNode(child1);
  block->InsertNode(nested_block);
  nested_block->InsertNode(child2);
  nested_block->InsertNode(child3);
  block->InsertNode(child4);

  EXPECT_EQ(parent->GetChildCount(), 4);

  base::Vector<fml::RefPtr<FiberElement>> inserted = {child1, child4};
  base::Vector<fml::RefPtr<FiberElement>> removed = {child1, nested_block,
                                                     child4};
  block->ReplaceElements(inserted, removed);

  EXPECT_EQ(parent->GetChildCount(), 2);
  EXPECT_EQ(parent->GetChildAt(0), child1.get());
  EXPECT_EQ(parent->GetChildAt(1), child4.get());

  EXPECT_EQ(block->block_children_.size(), 2);
}

TEST_F(BlockElementTest, ReplaceElements_WithForElement) {
  auto parent = manager->CreateFiberNode("view");
  auto block = CreateBlockNode("block");
  parent->InsertNode(block);

  auto child1 = manager->CreateFiberNode("view");
  auto child2 = manager->CreateFiberNode("view");
  auto child3 = manager->CreateFiberNode("view");

  auto for_node = CreateForNode("for");
  for_node->UpdateChildrenCount(1);

  block->InsertNode(child1);
  block->InsertNode(for_node);
  for_node->InsertNode(child2);
  block->InsertNode(child3);

  EXPECT_EQ(parent->GetChildCount(), 3);

  base::Vector<fml::RefPtr<FiberElement>> inserted = {for_node, child3, child1};
  base::Vector<fml::RefPtr<FiberElement>> removed = {child1, for_node, child3};
  block->ReplaceElements(inserted, removed);

  EXPECT_EQ(parent->GetChildCount(), 3);
  EXPECT_EQ(parent->GetChildAt(0), child2.get());
  EXPECT_EQ(parent->GetChildAt(1), child3.get());
  EXPECT_EQ(parent->GetChildAt(2), child1.get());

  EXPECT_EQ(block->block_children_.size(), 3);
  EXPECT_EQ(block->block_children_[0].get(), for_node.get());
}

// 5. Ref node test

TEST_F(BlockElementTest, ReplaceElements_WithRefNode) {
  auto parent = manager->CreateFiberNode("view");
  auto block = CreateBlockNode("block");
  parent->InsertNode(block);

  auto childA = manager->CreateFiberNode("view");
  auto childB = manager->CreateFiberNode("view");
  auto childC = manager->CreateFiberNode("view");

  block->InsertNode(childA);
  block->InsertNode(childB);
  block->InsertNode(childC);

  auto ref_node = manager->CreateFiberNode("view");
  auto childX = manager->CreateFiberNode("view");
  auto childY = manager->CreateFiberNode("view");

  parent->InsertNode(ref_node);
  parent->InsertNode(childX);
  parent->InsertNode(childY);

  EXPECT_EQ(parent->GetChildCount(), 6);

  base::Vector<fml::RefPtr<FiberElement>> inserted = {childC, childB, childA};
  base::Vector<fml::RefPtr<FiberElement>> removed = {childA, childB, childC};
  block->ReplaceElements(inserted, removed);

  EXPECT_EQ(parent->GetChildCount(), 6);
  EXPECT_EQ(parent->GetChildAt(0), childC.get());
  EXPECT_EQ(parent->GetChildAt(1), childB.get());
  EXPECT_EQ(parent->GetChildAt(2), childA.get());
  EXPECT_EQ(parent->GetChildAt(3), ref_node.get());
  EXPECT_EQ(parent->GetChildAt(4), childX.get());
  EXPECT_EQ(parent->GetChildAt(5), childY.get());
}

// 6. Complex scenario test

TEST_F(BlockElementTest, ReplaceElements_MixedBlockAndNormal) {
  auto parent = manager->CreateFiberNode("view");
  auto block = CreateBlockNode("block");
  parent->InsertNode(block);

  auto childA = manager->CreateFiberNode("view");
  auto childB = manager->CreateFiberNode("view");
  auto childC = manager->CreateFiberNode("view");
  auto childD = manager->CreateFiberNode("view");
  auto childE = manager->CreateFiberNode("view");

  auto nested_block = CreateBlockNode("nested_block");

  block->InsertNode(childA);
  block->InsertNode(nested_block);
  nested_block->InsertNode(childB);
  nested_block->InsertNode(childC);
  block->InsertNode(childD);
  block->InsertNode(childE);

  EXPECT_EQ(parent->GetChildCount(), 5);

  auto childF = manager->CreateFiberNode("view");

  base::Vector<fml::RefPtr<FiberElement>> inserted = {childE, nested_block,
                                                      childF, childA};
  base::Vector<fml::RefPtr<FiberElement>> removed = {childA, nested_block,
                                                     childD, childE};
  block->ReplaceElements(inserted, removed);

  EXPECT_EQ(parent->GetChildCount(), 5);
  EXPECT_EQ(parent->GetChildAt(0), childE.get());
  EXPECT_EQ(parent->GetChildAt(1), childB.get());
  EXPECT_EQ(parent->GetChildAt(2), childC.get());
  EXPECT_EQ(parent->GetChildAt(3), childF.get());
  EXPECT_EQ(parent->GetChildAt(4), childA.get());

  EXPECT_EQ(block->block_children_.size(), 4);
  EXPECT_EQ(block->block_children_[0].get(), childE.get());
  EXPECT_EQ(block->block_children_[1].get(), nested_block.get());
  EXPECT_EQ(block->block_children_[2].get(), childF.get());
  EXPECT_EQ(block->block_children_[3].get(), childA.get());
}

TEST_F(BlockElementTest, ReplaceElements_AllNewElements) {
  auto parent = manager->CreateFiberNode("view");
  auto block = CreateBlockNode("block");
  parent->InsertNode(block);

  auto childA = manager->CreateFiberNode("view");
  auto childB = manager->CreateFiberNode("view");
  auto childC = manager->CreateFiberNode("view");

  block->InsertNode(childA);
  block->InsertNode(childB);
  block->InsertNode(childC);

  auto childD = manager->CreateFiberNode("view");
  auto childE = manager->CreateFiberNode("view");
  auto childF = manager->CreateFiberNode("view");

  base::Vector<fml::RefPtr<FiberElement>> inserted = {childD, childE, childF};
  base::Vector<fml::RefPtr<FiberElement>> removed = {childA, childB, childC};
  block->ReplaceElements(inserted, removed);

  EXPECT_EQ(parent->GetChildCount(), 3);
  EXPECT_EQ(parent->GetChildAt(0), childD.get());
  EXPECT_EQ(parent->GetChildAt(1), childE.get());
  EXPECT_EQ(parent->GetChildAt(2), childF.get());

  EXPECT_EQ(block->block_children_.size(), 3);
}

TEST_F(BlockElementTest, ReplaceElements_MixedForAndIfElements) {
  auto parent = manager->CreateFiberNode("view");
  auto block = CreateBlockNode("block");
  parent->InsertNode(block);

  auto child1 = manager->CreateFiberNode("view");
  auto child2 = manager->CreateFiberNode("view");
  auto child3 = manager->CreateFiberNode("view");
  auto child4 = manager->CreateFiberNode("view");

  auto for_node = CreateForNode("for");
  for_node->UpdateChildrenCount(1);

  auto if_node = CreateIfNode("if");
  if_node->UpdateIfIndex(1);

  block->InsertNode(child1);
  block->InsertNode(for_node);
  for_node->InsertNode(child2);
  block->InsertNode(if_node);
  if_node->InsertNode(child3);
  block->InsertNode(child4);

  EXPECT_EQ(parent->GetChildCount(), 4);

  base::Vector<fml::RefPtr<FiberElement>> inserted = {if_node, child1, for_node,
                                                      child4};
  base::Vector<fml::RefPtr<FiberElement>> removed = {child1, for_node, if_node,
                                                     child4};
  block->ReplaceElements(inserted, removed);

  EXPECT_EQ(parent->GetChildCount(), 4);
  EXPECT_EQ(parent->GetChildAt(0), child3.get());
  EXPECT_EQ(parent->GetChildAt(1), child1.get());
  EXPECT_EQ(parent->GetChildAt(2), child2.get());
  EXPECT_EQ(parent->GetChildAt(3), child4.get());

  EXPECT_EQ(block->block_children_.size(), 4);
}

// 7. State verification test

TEST_F(BlockElementTest, ReplaceElements_VerifyVirtualParent) {
  auto parent = manager->CreateFiberNode("view");
  auto block = CreateBlockNode("block");
  parent->InsertNode(block);

  auto childA = manager->CreateFiberNode("view");
  auto childB = manager->CreateFiberNode("view");
  auto childC = manager->CreateFiberNode("view");

  block->InsertNode(childA);
  block->InsertNode(childB);

  base::Vector<fml::RefPtr<FiberElement>> inserted = {childB, childC, childA};
  base::Vector<fml::RefPtr<FiberElement>> removed = {childA, childB};
  block->ReplaceElements(inserted, removed);

  EXPECT_EQ(childA->virtual_parent(), block.get());
  EXPECT_EQ(childB->virtual_parent(), block.get());
  EXPECT_EQ(childC->virtual_parent(), block.get());
}

TEST_F(BlockElementTest, ReplaceElements_VerifyParentChildren) {
  auto parent = manager->CreateFiberNode("view");
  auto block = CreateBlockNode("block");
  parent->InsertNode(block);

  auto child1 = manager->CreateFiberNode("view");
  auto child2 = manager->CreateFiberNode("view");
  auto nested_block = CreateBlockNode("nested_block");
  auto child3 = manager->CreateFiberNode("view");

  block->InsertNode(child1);
  block->InsertNode(nested_block);
  nested_block->InsertNode(child2);
  block->InsertNode(child3);

  base::Vector<fml::RefPtr<FiberElement>> inserted = {child3, nested_block,
                                                      child1};
  base::Vector<fml::RefPtr<FiberElement>> removed = {child1, nested_block,
                                                     child3};
  block->ReplaceElements(inserted, removed);

  EXPECT_EQ(parent->GetChildCount(), 3);
  EXPECT_EQ(parent->scoped_children_.size(), 3);
  EXPECT_TRUE(parent->scoped_virtual_children_.has_value());
  EXPECT_EQ(parent->scoped_virtual_children_->size(), 2);
}

TEST_F(BlockElementTest, ReplaceElements_VerifyBlockChildren) {
  auto parent = manager->CreateFiberNode("view");
  auto block = CreateBlockNode("block");
  parent->InsertNode(block);

  auto childA = manager->CreateFiberNode("view");
  auto childB = manager->CreateFiberNode("view");
  auto childC = manager->CreateFiberNode("view");

  block->InsertNode(childA);
  block->InsertNode(childB);
  block->InsertNode(childC);

  base::Vector<fml::RefPtr<FiberElement>> inserted = {childC, childA, childB};
  base::Vector<fml::RefPtr<FiberElement>> removed = {childA, childB, childC};
  block->ReplaceElements(inserted, removed);

  EXPECT_EQ(block->block_children_.size(), 3);
  EXPECT_EQ(block->block_children_[0].get(), childC.get());
  EXPECT_EQ(block->block_children_[2].get(), childB.get());
  EXPECT_EQ(block->block_children_[1].get(), childA.get());
}

// 8. InsertNode after ReplaceElements test

TEST_F(BlockElementTest, InsertNode_AfterReplaceElements) {
  auto parent = manager->CreateFiberNode("view");
  auto block = CreateBlockNode("block");
  parent->InsertNode(block);

  // Create child nodes with incremental impl_id
  auto child0 = manager->CreateFiberNode("view");
  auto child1 = manager->CreateFiberNode("view");
  auto block0 = CreateBlockNode("block0");

  // Initial insertion
  block->InsertNode(child0);
  block->InsertNode(child1);
  block->InsertNode(block0);

  EXPECT_EQ(parent->GetChildCount(), 2);
  EXPECT_EQ(parent->GetChildAt(0), child0.get());
  EXPECT_EQ(parent->GetChildAt(1), child1.get());

  // Reorder via ReplaceElements: move block0 to the front
  base::Vector<fml::RefPtr<FiberElement>> inserted = {block0, child0, child1};
  base::Vector<fml::RefPtr<FiberElement>> removed = {child0, child1, block0};
  block->ReplaceElements(inserted, removed);

  // Verify the order after rearrangement
  EXPECT_EQ(block->block_children_.size(), 3);
  EXPECT_EQ(block->block_children_[0].get(), block0.get());
  EXPECT_EQ(block->block_children_[1].get(), child0.get());
  EXPECT_EQ(block->block_children_[2].get(), child1.get());

  // Insert new node grandChild1 into block0
  auto grandChild1 = manager->CreateFiberNode("view");
  block0->InsertNode(grandChild1);

  // Verify: grandChild1 should precede child0 in parent->scoped_children_
  EXPECT_EQ(parent->GetChildCount(), 3);
  EXPECT_EQ(parent->GetChildAt(0), grandChild1.get());
  EXPECT_EQ(parent->GetChildAt(1), child0.get());
  EXPECT_EQ(parent->GetChildAt(2), child1.get());
}

TEST_F(BlockElementTest, InsertNode_IntoBornEmptyBlockAfterParentAddsSibling) {
  auto parent = manager->CreateFiberNode("view");
  auto block = CreateBlockNode("block");
  parent->InsertNode(block);

  auto sibling = manager->CreateFiberNode("view");
  parent->InsertNode(sibling);

  auto nested_block = CreateBlockNode("nested_block");
  block->InsertNode(nested_block);

  auto nested_child = manager->CreateFiberNode("view");
  nested_block->InsertNode(nested_child);

  EXPECT_EQ(parent->GetChildCount(), 2);
  EXPECT_EQ(parent->GetChildAt(0), nested_child.get());
  EXPECT_EQ(parent->GetChildAt(1), sibling.get());
}

TEST_F(BlockElementTest,
       NestedForUpdateChildrenCount_AfterOuterReplaceElements) {
  auto parent = manager->CreateFiberNode("view");
  auto block = CreateBlockNode("block");
  auto for_node = CreateForNode("for");
  for_node->UpdateChildrenCount(2);
  parent->InsertNode(block);

  auto childA = manager->CreateFiberNode("view");
  auto for_child1 = manager->CreateFiberNode("view");
  auto for_child2 = manager->CreateFiberNode("view");
  auto childB = manager->CreateFiberNode("view");

  block->InsertNode(childA);
  block->InsertNode(for_node);
  for_node->InsertNode(for_child1);
  for_node->InsertNode(for_child2);
  block->InsertNode(childB);

  base::Vector<fml::RefPtr<FiberElement>> inserted = {for_node, childB, childA};
  base::Vector<fml::RefPtr<FiberElement>> removed = {childA, for_node, childB};
  block->ReplaceElements(inserted, removed);

  EXPECT_EQ(parent->GetChildCount(), 4);
  EXPECT_EQ(parent->GetChildAt(0), for_child1.get());
  EXPECT_EQ(parent->GetChildAt(1), for_child2.get());
  EXPECT_EQ(parent->GetChildAt(2), childB.get());
  EXPECT_EQ(parent->GetChildAt(3), childA.get());

  for_node->UpdateChildrenCount(0);

  EXPECT_EQ(parent->GetChildCount(), 2);
  EXPECT_EQ(parent->GetChildAt(0), childB.get());
  EXPECT_EQ(parent->GetChildAt(1), childA.get());

  auto for_child3 = manager->CreateFiberNode("view");
  for_node->InsertNode(for_child3);

  EXPECT_EQ(parent->GetChildCount(), 3);
  EXPECT_EQ(parent->GetChildAt(0), for_child3.get());
  EXPECT_EQ(parent->GetChildAt(1), childB.get());
  EXPECT_EQ(parent->GetChildAt(2), childA.get());
}

TEST_F(BlockElementTest, NestedIfUpdateIfIndex_AfterOuterReplaceElements) {
  auto parent = manager->CreateFiberNode("view");
  auto block = CreateBlockNode("block");
  auto if_node = CreateIfNode("if");
  if_node->UpdateIfIndex(1);
  parent->InsertNode(block);

  auto childA = manager->CreateFiberNode("view");
  auto if_child1 = manager->CreateFiberNode("view");
  auto childB = manager->CreateFiberNode("view");

  block->InsertNode(childA);
  block->InsertNode(if_node);
  if_node->InsertNode(if_child1);
  block->InsertNode(childB);

  base::Vector<fml::RefPtr<FiberElement>> inserted = {childA, childB, if_node};
  base::Vector<fml::RefPtr<FiberElement>> removed = {childA, if_node, childB};
  block->ReplaceElements(inserted, removed);

  EXPECT_EQ(parent->GetChildCount(), 3);
  EXPECT_EQ(parent->GetChildAt(0), childA.get());
  EXPECT_EQ(parent->GetChildAt(1), childB.get());
  EXPECT_EQ(parent->GetChildAt(2), if_child1.get());

  if_node->UpdateIfIndex(-1);

  EXPECT_EQ(parent->GetChildCount(), 2);
  EXPECT_EQ(parent->GetChildAt(0), childA.get());
  EXPECT_EQ(parent->GetChildAt(1), childB.get());

  if_node->UpdateIfIndex(2);
  auto if_child2 = manager->CreateFiberNode("view");
  if_node->InsertNode(if_child2);

  EXPECT_EQ(parent->GetChildCount(), 3);
  EXPECT_EQ(parent->GetChildAt(0), childA.get());
  EXPECT_EQ(parent->GetChildAt(1), childB.get());
  EXPECT_EQ(parent->GetChildAt(2), if_child2.get());
}

TEST_F(BlockElementTest, DeepNestedIfUpdateIfIndex_AfterOuterReplaceElements) {
  auto parent = manager->CreateFiberNode("view");
  auto outer_block = CreateBlockNode("outer_block");
  auto nested_block = CreateBlockNode("nested_block");
  auto if_node = CreateIfNode("if");
  if_node->UpdateIfIndex(1);
  parent->InsertNode(outer_block);

  auto childA = manager->CreateFiberNode("view");
  auto if_child1 = manager->CreateFiberNode("view");
  auto nested_child = manager->CreateFiberNode("view");
  auto childB = manager->CreateFiberNode("view");

  outer_block->InsertNode(childA);
  outer_block->InsertNode(nested_block);
  nested_block->InsertNode(if_node);
  if_node->InsertNode(if_child1);
  nested_block->InsertNode(nested_child);
  outer_block->InsertNode(childB);

  base::Vector<fml::RefPtr<FiberElement>> inserted = {childB, nested_block,
                                                      childA};
  base::Vector<fml::RefPtr<FiberElement>> removed = {childA, nested_block,
                                                     childB};
  outer_block->ReplaceElements(inserted, removed);

  EXPECT_EQ(parent->GetChildCount(), 4);
  EXPECT_EQ(parent->GetChildAt(0), childB.get());
  EXPECT_EQ(parent->GetChildAt(1), if_child1.get());
  EXPECT_EQ(parent->GetChildAt(2), nested_child.get());
  EXPECT_EQ(parent->GetChildAt(3), childA.get());

  if_node->UpdateIfIndex(-1);

  EXPECT_EQ(parent->GetChildCount(), 3);
  EXPECT_EQ(parent->GetChildAt(0), childB.get());
  EXPECT_EQ(parent->GetChildAt(1), nested_child.get());
  EXPECT_EQ(parent->GetChildAt(2), childA.get());

  if_node->UpdateIfIndex(3);
  auto if_child2 = manager->CreateFiberNode("view");
  if_node->InsertNode(if_child2);

  EXPECT_EQ(parent->GetChildCount(), 4);
  EXPECT_EQ(parent->GetChildAt(0), childB.get());
  EXPECT_EQ(parent->GetChildAt(1), if_child2.get());
  EXPECT_EQ(parent->GetChildAt(2), nested_child.get());
  EXPECT_EQ(parent->GetChildAt(3), childA.get());
}

TEST_F(BlockElementTest,
       InsertNode_IntoEmptyNestedBlockAfterOuterReplaceElements) {
  auto parent = manager->CreateFiberNode("view");
  auto outer_block = CreateBlockNode("outer_block");
  auto empty_nested_block = CreateBlockNode("empty_nested_block");
  parent->InsertNode(outer_block);

  auto childA = manager->CreateFiberNode("view");
  auto sibling = manager->CreateFiberNode("view");

  outer_block->InsertNode(childA);
  outer_block->InsertNode(empty_nested_block);
  parent->InsertNode(sibling);

  base::Vector<fml::RefPtr<FiberElement>> inserted = {empty_nested_block,
                                                      childA};
  base::Vector<fml::RefPtr<FiberElement>> removed = {childA,
                                                     empty_nested_block};
  outer_block->ReplaceElements(inserted, removed);

  EXPECT_EQ(parent->GetChildCount(), 2);
  EXPECT_EQ(parent->GetChildAt(0), childA.get());
  EXPECT_EQ(parent->GetChildAt(1), sibling.get());

  auto nested_child = manager->CreateFiberNode("view");
  empty_nested_block->InsertNode(nested_child);

  EXPECT_EQ(parent->GetChildCount(), 3);
  EXPECT_EQ(parent->GetChildAt(0), nested_child.get());
  EXPECT_EQ(parent->GetChildAt(1), childA.get());
  EXPECT_EQ(parent->GetChildAt(2), sibling.get());
}

TEST_F(BlockElementTest,
       NestedReplaceElements_AfterOuterForUpdateChildrenCount) {
  auto parent = manager->CreateFiberNode("view");
  auto prefix = manager->CreateFiberNode("view");
  auto outer_for = CreateForNode("for");
  outer_for->UpdateChildrenCount(3);
  auto suffix = manager->CreateFiberNode("view");
  auto childA = manager->CreateFiberNode("view");
  auto nested_block = CreateBlockNode("nested_block");
  auto nested1 = manager->CreateFiberNode("view");
  auto nested2 = manager->CreateFiberNode("view");
  auto childB = manager->CreateFiberNode("view");

  parent->InsertNode(prefix);
  parent->InsertNode(outer_for);
  parent->InsertNode(suffix);

  outer_for->InsertNode(childA);
  outer_for->InsertNode(nested_block);
  nested_block->InsertNode(nested1);
  nested_block->InsertNode(nested2);
  outer_for->InsertNode(childB);

  EXPECT_EQ(parent->GetChildCount(), 6);
  EXPECT_EQ(parent->GetChildAt(0), prefix.get());
  EXPECT_EQ(parent->GetChildAt(1), childA.get());
  EXPECT_EQ(parent->GetChildAt(2), nested1.get());
  EXPECT_EQ(parent->GetChildAt(3), nested2.get());
  EXPECT_EQ(parent->GetChildAt(4), childB.get());
  EXPECT_EQ(parent->GetChildAt(5), suffix.get());

  outer_for->UpdateChildrenCount(2);

  EXPECT_EQ(parent->GetChildCount(), 5);
  EXPECT_EQ(parent->GetChildAt(0), prefix.get());
  EXPECT_EQ(parent->GetChildAt(1), childA.get());
  EXPECT_EQ(parent->GetChildAt(2), nested1.get());
  EXPECT_EQ(parent->GetChildAt(3), nested2.get());
  EXPECT_EQ(parent->GetChildAt(4), suffix.get());

  auto nested3 = manager->CreateFiberNode("view");
  base::Vector<fml::RefPtr<FiberElement>> inserted = {nested2, nested3,
                                                      nested1};
  base::Vector<fml::RefPtr<FiberElement>> removed = {nested1, nested2};
  nested_block->ReplaceElements(inserted, removed);

  EXPECT_EQ(parent->GetChildCount(), 6);
  EXPECT_EQ(parent->GetChildAt(0), prefix.get());
  EXPECT_EQ(parent->GetChildAt(1), childA.get());
  EXPECT_EQ(parent->GetChildAt(2), nested2.get());
  EXPECT_EQ(parent->GetChildAt(3), nested3.get());
  EXPECT_EQ(parent->GetChildAt(4), nested1.get());
  EXPECT_EQ(parent->GetChildAt(5), suffix.get());
}

TEST_F(BlockElementTest,
       DeepNestedForUpdateChildrenCount_AfterOuterAndInnerReplaceElements) {
  auto parent = manager->CreateFiberNode("view");
  auto outer_block = CreateBlockNode("outer_block");
  auto nested_block = CreateBlockNode("nested_block");
  auto for_node = CreateForNode("for");
  for_node->UpdateChildrenCount(2);
  parent->InsertNode(outer_block);

  auto childA = manager->CreateFiberNode("view");
  auto childB = manager->CreateFiberNode("view");
  auto childC = manager->CreateFiberNode("view");
  auto childD = manager->CreateFiberNode("view");
  auto for_child1 = manager->CreateFiberNode("view");
  auto for_child2 = manager->CreateFiberNode("view");

  outer_block->InsertNode(childA);
  outer_block->InsertNode(nested_block);
  nested_block->InsertNode(childC);
  nested_block->InsertNode(for_node);
  for_node->InsertNode(for_child1);
  for_node->InsertNode(for_child2);
  nested_block->InsertNode(childD);
  outer_block->InsertNode(childB);

  EXPECT_EQ(parent->GetChildCount(), 6);
  EXPECT_EQ(parent->GetChildAt(0), childA.get());
  EXPECT_EQ(parent->GetChildAt(1), childC.get());
  EXPECT_EQ(parent->GetChildAt(2), for_child1.get());
  EXPECT_EQ(parent->GetChildAt(3), for_child2.get());
  EXPECT_EQ(parent->GetChildAt(4), childD.get());
  EXPECT_EQ(parent->GetChildAt(5), childB.get());

  base::Vector<fml::RefPtr<FiberElement>> outer_inserted = {
      childB, nested_block, childA};
  base::Vector<fml::RefPtr<FiberElement>> outer_removed = {childA, nested_block,
                                                           childB};
  outer_block->ReplaceElements(outer_inserted, outer_removed);

  EXPECT_EQ(parent->GetChildCount(), 6);
  EXPECT_EQ(parent->GetChildAt(0), childB.get());
  EXPECT_EQ(parent->GetChildAt(1), childC.get());
  EXPECT_EQ(parent->GetChildAt(2), for_child1.get());
  EXPECT_EQ(parent->GetChildAt(3), for_child2.get());
  EXPECT_EQ(parent->GetChildAt(4), childD.get());
  EXPECT_EQ(parent->GetChildAt(5), childA.get());

  base::Vector<fml::RefPtr<FiberElement>> nested_inserted = {childD, for_node,
                                                             childC};
  base::Vector<fml::RefPtr<FiberElement>> nested_removed = {childC, for_node,
                                                            childD};
  nested_block->ReplaceElements(nested_inserted, nested_removed);

  EXPECT_EQ(parent->GetChildCount(), 6);
  EXPECT_EQ(parent->GetChildAt(0), childB.get());
  EXPECT_EQ(parent->GetChildAt(1), childD.get());
  EXPECT_EQ(parent->GetChildAt(2), for_child1.get());
  EXPECT_EQ(parent->GetChildAt(3), for_child2.get());
  EXPECT_EQ(parent->GetChildAt(4), childC.get());
  EXPECT_EQ(parent->GetChildAt(5), childA.get());

  for_node->UpdateChildrenCount(0);

  EXPECT_EQ(parent->GetChildCount(), 4);
  EXPECT_EQ(parent->GetChildAt(0), childB.get());
  EXPECT_EQ(parent->GetChildAt(1), childD.get());
  EXPECT_EQ(parent->GetChildAt(2), childC.get());
  EXPECT_EQ(parent->GetChildAt(3), childA.get());

  auto for_child3 = manager->CreateFiberNode("view");
  for_node->InsertNode(for_child3);

  EXPECT_EQ(parent->GetChildCount(), 5);
  EXPECT_EQ(parent->GetChildAt(0), childB.get());
  EXPECT_EQ(parent->GetChildAt(1), childD.get());
  EXPECT_EQ(parent->GetChildAt(2), for_child3.get());
  EXPECT_EQ(parent->GetChildAt(3), childC.get());
  EXPECT_EQ(parent->GetChildAt(4), childA.get());
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
