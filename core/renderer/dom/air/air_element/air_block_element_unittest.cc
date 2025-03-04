// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#define private public
#include "core/renderer/dom/air/air_element/air_block_element.h"
#undef private

#include "core/renderer/dom/air/air_element/air_page_element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/tasm_operation_queue.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "core/template_bundle/template_codec/compile_options.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

class AirBlockElementTest : public ::testing::Test {
 public:
  AirBlockElementTest() {}
  ~AirBlockElementTest() override {}
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

  fml::RefPtr<AirLepusRef> CreateAirNode(const base::String &tag,
                                         int32_t lepus_id) {
    std::shared_ptr<AirElement> element =
        std::make_shared<AirElement>(kAirNormal, manager.get(), tag, lepus_id);
    manager->air_node_manager()->Record(element->impl_id(), element);
    manager->air_node_manager()->RecordForLepusId(
        lepus_id, static_cast<uint64_t>(lepus_id),
        AirLepusRef::Create(element));
    return AirLepusRef::Create(element);
  }

  AirBlockElement *CreateAirBlockNode(int32_t lepus_id) {
    std::shared_ptr<AirBlockElement> element =
        std::make_shared<AirBlockElement>(manager.get(), lepus_id);
    manager->air_node_manager()->Record(element->impl_id(), element);
    manager->air_node_manager()->RecordForLepusId(
        lepus_id, static_cast<uint64_t>(lepus_id),
        AirLepusRef::Create(element));
    return element.get();
  }
};

TEST_F(AirBlockElementTest, InsertNode) {
  auto config = std::make_shared<PageConfig>();
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_STRICT);
  manager->SetConfig(config);
  uint32_t lepus_id = 1;
  auto parent = CreateAirNode("view", lepus_id++)->Get();
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 0);

  auto block_element = CreateAirBlockNode(lepus_id++);
  CSSValue css_value(lepus::Value("visible"), CSSValuePattern::STRING);
  block_element->SetStyle(CSSPropertyID::kPropertyIDOverflow, css_value);
  parent->InsertNode(block_element);

  auto block_child = CreateAirNode("view", lepus_id++)->Get();
  block_element->InsertNode(block_child, false);

  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 1);
  EXPECT_EQ(static_cast<int>(block_element->GetChildCount()), 0);
  EXPECT_EQ(parent->GetChildAt(0), block_child);
}

TEST_F(AirBlockElementTest, RemoveNode) {
  auto config = std::make_shared<PageConfig>();
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_STRICT);
  manager->SetConfig(config);
  uint32_t lepus_id = 1;
  auto parent = CreateAirNode("view", lepus_id++)->Get();
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 0);

  auto block_element = CreateAirBlockNode(lepus_id++);
  CSSValue css_value(lepus::Value("visible"), CSSValuePattern::STRING);
  block_element->SetStyle(CSSPropertyID::kPropertyIDOverflow, css_value);
  parent->InsertNode(block_element);

  auto block_child = CreateAirNode("view", lepus_id++)->Get();
  block_element->InsertNode(block_child, false);
  parent->FlushRecursively();

  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 1);
  EXPECT_EQ(static_cast<int>(block_element->GetChildCount()), 0);
  EXPECT_EQ(parent->GetChildAt(0), block_child);

  block_element->RemoveNode(block_child);
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 0);
  EXPECT_EQ(static_cast<int>(block_element->GetChildCount()), 0);
}

TEST_F(AirBlockElementTest, NonVirtualNodeCountInParent) {
  auto config = std::make_shared<PageConfig>();
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_STRICT);
  manager->SetConfig(config);
  uint32_t lepus_id = 1;
  auto parent = CreateAirNode("view", lepus_id++)->Get();
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 0);

  auto block_element = CreateAirBlockNode(lepus_id++);
  tasm::CSSValue css_value(lepus::Value("visible"), CSSValuePattern::STRING);
  block_element->SetStyle(CSSPropertyID::kPropertyIDOverflow, css_value);
  parent->InsertNode(block_element);

  auto block_child = CreateAirNode("view", lepus_id++)->Get();
  block_element->InsertNode(block_child, false);

  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 1);
  EXPECT_EQ(static_cast<int>(block_element->GetChildCount()), 0);
  EXPECT_EQ(parent->GetChildAt(0), block_child);

  auto red_child = CreateAirNode("view", lepus_id++)->Get();
  block_element->InsertNode(red_child, false);

  auto green_child = CreateAirNode("view", lepus_id++)->Get();
  block_element->InsertNode(green_child, false);
  EXPECT_EQ(static_cast<int>(block_element->GetChildCount()), 0);
  EXPECT_EQ(static_cast<int>(block_element->NonVirtualNodeCountInParent()), 3);

  auto red_block_element = CreateAirBlockNode(lepus_id++);
  block_element->SetStyle(CSSPropertyID::kPropertyIDOverflow, css_value);
  block_element->InsertNode(red_block_element, false);

  auto blue_child = CreateAirNode("view", lepus_id++)->Get();
  red_block_element->InsertNode(blue_child, false);

  EXPECT_EQ(static_cast<int>(block_element->NonVirtualNodeCountInParent()), 4);
  EXPECT_EQ(static_cast<int>(red_block_element->NonVirtualNodeCountInParent()),
            1);
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 4);
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
