// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/dom/element_tree_serializer.h"

#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/tasm_operation_queue.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

namespace {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

lepus::Value GetProperty(const lepus::Value& value, const char* key) {
  return value.GetProperty(base::String(key));
}

}  // namespace

class ElementTreeSerializerTest : public ::testing::Test {
 public:
  void SetUp() override {
    LynxEnvConfig lynx_env_config(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator_ = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    manager_ = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<MockPaintingContext>(), tasm_mediator_.get(),
        lynx_env_config);
    auto config = std::make_shared<PageConfig>();
    config->SetEnableFiberArch(true);
    config->SetEnableZIndex(true);
    manager_->SetConfig(config);
  }

 protected:
  std::unique_ptr<lynx::tasm::ElementManager> manager_;
  std::shared_ptr<::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>
      tasm_mediator_;
};

TEST_F(ElementTreeSerializerTest, ToLepusValueIncludesElementFields) {
  auto root = manager_->CreateFiberPage("0", 10);
  root->SetNodeIndex(100);
  auto child = manager_->CreateFiberNode("text");
  child->SetIdSelector("label");
  child->SetNodeIndex(200);
  root->InsertNode(child);

  auto root_info = ElementTreeSerializer::ToLepusValue(root.get());

  EXPECT_EQ(GetProperty(root_info, "sign").Int32(), root->impl_id());
  EXPECT_EQ(GetProperty(root_info, "tagName").StdString(), "page");
  EXPECT_EQ(GetProperty(root_info, "id").StdString(), "");
  EXPECT_EQ(GetProperty(root_info, "nodeIndex").UInt32(), 100U);

  auto children = GetProperty(root_info, "children");
  ASSERT_TRUE(children.IsArray());
  ASSERT_EQ(children.Array()->size(), 1U);
  auto child_info = children.Array()->get(0);
  EXPECT_EQ(GetProperty(child_info, "sign").Int32(), child->impl_id());
  EXPECT_EQ(GetProperty(child_info, "tagName").StdString(), "text");
  EXPECT_EQ(GetProperty(child_info, "id").StdString(), "label");
  EXPECT_EQ(GetProperty(child_info, "nodeIndex").UInt32(), 200U);
}

TEST_F(ElementTreeSerializerTest, ToLepusValueKeepsChildrenOrder) {
  auto root = manager_->CreateFiberPage("0", 10);
  auto first = manager_->CreateFiberNode("view");
  first->SetIdSelector("first");
  auto second = manager_->CreateFiberNode("image");
  second->SetIdSelector("second");
  root->InsertNode(first);
  root->InsertNode(second);

  auto children =
      GetProperty(ElementTreeSerializer::ToLepusValue(root.get()), "children");

  ASSERT_TRUE(children.IsArray());
  ASSERT_EQ(children.Array()->size(), 2U);
  EXPECT_EQ(GetProperty(children.Array()->get(0), "id").StdString(), "first");
  EXPECT_EQ(GetProperty(children.Array()->get(1), "id").StdString(), "second");
}

TEST_F(ElementTreeSerializerTest, ToLepusValueReturnsNilForNullElement) {
  EXPECT_TRUE(ElementTreeSerializer::ToLepusValue(nullptr).IsNil());
  EXPECT_TRUE(ElementTreeSerializer::ToJSONString(nullptr).empty());
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
