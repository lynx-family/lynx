// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/dom/air/air_element/air_element_container.h"

#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/dom/air/air_element/air_element.h"
#include "core/renderer/dom/element_manager.h"
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

class AirElementContainerTest : public ::testing::Test {
 public:
  AirElementContainerTest() {}
  ~AirElementContainerTest() override {}

  std::unique_ptr<lynx::tasm::ElementManager> manager;
  std::shared_ptr<::testing::NiceMock<test::MockTasmDelegate>> tasm_mediator;

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
  }

  fml::RefPtr<AirLepusRef> CreateAirNode(const base::String& tag,
                                         int32_t lepus_id) {
    std::shared_ptr<AirElement> element =
        std::make_shared<AirElement>(kAirNormal, manager.get(), tag, lepus_id);
    manager->air_node_manager()->Record(element->impl_id(), element);
    manager->air_node_manager()->RecordForLepusId(
        lepus_id, static_cast<uint64_t>(lepus_id),
        AirLepusRef::Create(element));
    return AirLepusRef::Create(element);
  }
};

TEST_F(AirElementContainerTest, Create) {
  uint32_t lepus_id = 1;
  auto element = CreateAirNode("view", lepus_id++)->Get();
  auto element_container = std::make_unique<AirElementContainer>(element);
  EXPECT_EQ(element_container->air_element(), element);

  auto child = CreateAirNode("view", lepus_id++)->Get();
  child->CreateElementContainer(false);
  auto child_container = child->element_container();
  EXPECT_EQ(child_container->air_element(), child);
}

TEST_F(AirElementContainerTest, InsertAndDestroy) {
  uint32_t lepus_id = 1;
  auto element = CreateAirNode("view", lepus_id++)->Get();
  element->CreateElementContainer(false);
  auto element_container = element->element_container();

  auto child = CreateAirNode("view", lepus_id++)->Get();
  element->AddChildAt(child, 0);
  EXPECT_EQ(child->parent(), element);

  child->CreateElementContainer(false);
  auto child_container = child->element_container();
  child_container->InsertSelf();
  EXPECT_EQ(child_container->parent(), element_container);
  EXPECT_EQ(element_container->children().size(), static_cast<size_t>(1));

  child_container->RemoveSelf(true);
  EXPECT_EQ(child_container->parent(), nullptr);
  EXPECT_EQ(element_container->children().size(), static_cast<size_t>(0));
}

TEST_F(AirElementContainerTest, Children) {
  uint32_t lepus_id = 1;
  auto element = CreateAirNode("view", lepus_id++)->Get();
  element->CreateElementContainer(false);
  auto element_container = element->element_container();

  auto child = CreateAirNode("view", lepus_id++)->Get();
  element->AddChildAt(child, 0);
  child->CreateElementContainer(false);
  auto child_container = child->element_container();
  child_container->InsertSelf();
  element_container->AddChild(child_container, 0);  // test insert again

  EXPECT_EQ(child_container->parent(), element_container);
  EXPECT_EQ(element_container->children().size(), static_cast<size_t>(1));
  child_container->RemoveSelf(false);

  EXPECT_EQ(element_container->children().size(), static_cast<size_t>(0));
}

TEST_F(AirElementContainerTest, AirElementCase0) {
  auto config = std::make_shared<PageConfig>();
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_STRICT);
  manager->SetConfig(config);
  uint32_t lepus_id = 1;
  auto page = CreateAirNode("view", lepus_id++)->Get();
  page->SetAttribute("enable-layout", lepus::Value("false"));
  page->FlushProps();

  auto element0 = CreateAirNode("view", lepus_id++)->Get();
  element0->SetAttribute("enable-layout", lepus::Value("false"));

  auto element_before_black = CreateAirNode("view", lepus_id++)->Get();
  element_before_black->SetAttribute("enable-layout", lepus::Value("false"));

  auto text = CreateAirNode("text", lepus_id++)->Get();

  auto element_after_yellow = CreateAirNode("view", lepus_id++)->Get();
  element_after_yellow->SetAttribute("enable-layout", lepus::Value("false"));

  page->InsertNode(element0);
  page->InsertNode(element_before_black);
  page->InsertNode(text);
  page->InsertNode(element_after_yellow);

  page->FlushRecursively();

  auto page_container = page->element_container();
  auto page_container_children = page_container->children();

  EXPECT_EQ(static_cast<int>(page_container_children.size()), 4);
  EXPECT_EQ(page_container->none_layout_only_children_size_, 4);

  auto element0_container_index = page->GetUIIndexForChild(element0);
  auto element_before_black_index =
      page->GetUIIndexForChild(element_before_black);
  auto text_index = page->GetUIIndexForChild(text);
  auto element_after_yellow_index =
      page->GetUIIndexForChild(element_after_yellow);

  EXPECT_EQ(element0_container_index, 0);
  EXPECT_EQ(element_before_black_index, 1);
  EXPECT_EQ(text_index, 2);
  EXPECT_EQ(element_after_yellow_index, 3);

  auto painting_context =
      static_cast<MockPaintingContext*>(page->painting_context()->impl());

  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();
  auto page_painting_children = page_painting_node->children_;
  EXPECT_TRUE(page_painting_children.size() == 4);
  EXPECT_TRUE(page_painting_children[0]->id_ == element0->impl_id());
  EXPECT_TRUE(page_painting_children[1]->id_ ==
              element_before_black->impl_id());

  EXPECT_TRUE(page_painting_children[2]->id_ == text->impl_id());

  EXPECT_TRUE(page_painting_children[3]->id_ ==
              element_after_yellow->impl_id());
}

TEST_F(AirElementContainerTest, AirElementCase1) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  uint32_t lepus_id = 1;
  auto page = CreateAirNode("view", lepus_id++)->Get();
  page->SetAttribute("enable-layout", lepus::Value("false"));
  page->FlushProps();

  auto element0 = CreateAirNode("view", lepus_id++)->Get();
  element0->SetAttribute("enable-layout", lepus::Value("false"));

  auto element = CreateAirNode("view", lepus_id++)->Get();
  element->SetAttribute("enable-layout", lepus::Value("false"));

  // layout_only node
  auto container = CreateAirNode("view", lepus_id++)->Get();
  container->InsertNode(element);

  auto element_before_black = CreateAirNode("view", lepus_id++)->Get();
  element_before_black->SetAttribute("enable-layout", lepus::Value("false"));

  auto element_after_yellow = CreateAirNode("view", lepus_id++)->Get();
  element_after_yellow->SetAttribute("enable-layout", lepus::Value("false"));

  page->InsertNode(element0);
  page->InsertNode(container);
  page->InsertNode(element_after_yellow);

  page->InsertNodeBefore(element_before_black, container);
  page->FlushRecursively();

  EXPECT_TRUE(container->IsLayoutOnly());

  // Check the element container tree!
  auto* page_container = page->element_container();

  auto page_container_children = page_container->children();

  EXPECT_EQ(static_cast<int>(page_container_children.size()), 5);
  EXPECT_EQ(page_container->none_layout_only_children_size_, 4);

  auto painting_context =
      static_cast<MockPaintingContext*>(page->painting_context()->impl());
  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();
  auto page_painting_children = page_painting_node->children_;
  EXPECT_EQ(page_painting_children.size(), 4);

  EXPECT_TRUE(page_painting_children[0]->id_ == element0->impl_id());
  EXPECT_TRUE(page_painting_children[1]->id_ ==
              element_before_black->impl_id());
  EXPECT_TRUE(page_painting_children[2]->id_ == element->impl_id());
  EXPECT_TRUE(page_painting_children[3]->id_ ==
              element_after_yellow->impl_id());

  auto text = CreateAirNode("text", lepus_id++)->Get();
  page->InsertNodeBefore(text, container);

  page->FlushRecursively();

  EXPECT_EQ(page_container->none_layout_only_children_size_, 5);

  page_painting_children = page_painting_node->children_;
  EXPECT_TRUE(page_painting_children.size() == 5);

  EXPECT_TRUE(page_painting_children[0]->id_ == element0->impl_id());
  EXPECT_TRUE(page_painting_children[1]->id_ ==
              element_before_black->impl_id());
  EXPECT_TRUE(page_painting_children[2]->id_ == text->impl_id());
  EXPECT_TRUE(page_painting_children[3]->id_ == element->impl_id());
  EXPECT_TRUE(page_painting_children[4]->id_ ==
              element_after_yellow->impl_id());
}

TEST_F(AirElementContainerTest, AirElementCase2) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  uint32_t lepus_id = 1;
  auto page = CreateAirNode("view", lepus_id++)->Get();
  page->SetAttribute("enable-layout", lepus::Value("false"));
  page->FlushProps();

  auto element0 = CreateAirNode("view", lepus_id++)->Get();
  element0->SetAttribute("enable-layout", lepus::Value("false"));

  auto element_before_black = CreateAirNode("view", lepus_id++)->Get();
  element_before_black->SetAttribute("enable-layout", lepus::Value("false"));

  auto element = CreateAirNode("view", lepus_id++)->Get();
  element->SetAttribute("enable-layout", lepus::Value("false"));

  auto text = CreateAirNode("text", lepus_id++)->Get();

  // layout_only node
  auto container = CreateAirNode("view", lepus_id++)->Get();
  container->InsertNode(element);
  container->InsertNode(text);

  auto element_after_yellow = CreateAirNode("view", lepus_id++)->Get();
  element_after_yellow->SetAttribute("enable-layout", lepus::Value("false"));

  page->InsertNode(element0);
  page->InsertNode(element_after_yellow);

  page->InsertNodeBefore(element_before_black, element_after_yellow);
  page->InsertNodeBefore(container, element_before_black);
  page->FlushRecursively();

  auto painting_context =
      static_cast<MockPaintingContext*>(page->painting_context()->impl());
  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();
  auto page_painting_children = page_painting_node->children_;
  EXPECT_EQ(page_painting_children.size(), 5);

  page->RemoveNode(element_after_yellow);
  page->FlushRecursively();

  EXPECT_TRUE(container->IsLayoutOnly());

  // Check the element container tree!
  auto* page_container = page->element_container();

  auto page_container_children = page_container->children();

  EXPECT_EQ(static_cast<int>(page_container_children.size()), 5);
  EXPECT_EQ(page_container->none_layout_only_children_size_, 4);

  page_painting_children = page_painting_node->children_;
  EXPECT_EQ(page_painting_children.size(), 4);

  EXPECT_TRUE(page_painting_children[0]->id_ == element0->impl_id());
  EXPECT_TRUE(page_painting_children[1]->id_ == element->impl_id());
  EXPECT_TRUE(page_painting_children[2]->id_ == text->impl_id());
  EXPECT_TRUE(page_painting_children[3]->id_ ==
              element_before_black->impl_id());
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
