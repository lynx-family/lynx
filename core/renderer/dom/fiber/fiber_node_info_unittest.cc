// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/dom/fiber/fiber_node_info.h"

#include <sstream>

#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/renderer/dom/fiber/wrapper_element.h"
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

class FiberNodeInfoTest : public ::testing::Test {
 public:
  FiberNodeInfoTest() = default;
  ~FiberNodeInfoTest() override = default;
  lynx::tasm::ElementManager* manager;
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
    config->SetEnableZIndex(true);
    manager->SetConfig(config);
  }
};

TEST_F(FiberNodeInfoTest, GetNodeInfo) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  std::vector<std::string> fields{"id",    "tag",  "dataset",
                                  "class", "name", "index"};

  auto element_parent = manager->CreateFiberNode("view");
  auto element = manager->CreateFiberNode("view");
  element_parent->InsertNode(element);
  {
    auto info = FiberNodeInfo::GetNodeInfo(element.get(), fields);
    std::stringstream ss;
    info.PrintValue(ss);
    EXPECT_EQ(ss.str(), "{class:[],dataset:{},tag:view,index:0,name:,id:}");
  }

  auto element_0 = manager->CreateFiberNode("view");
  element_parent->InsertNode(element_0, 0);
  element->SetClass("test_class");
  element->SetIdSelector("test_id");
  element->AddDataset("test_dataset", lepus::Value("test_dataset_value"));
  element->SetAttribute("name", lepus::Value("test_name"));
  {
    auto info = FiberNodeInfo::GetNodeInfo(element.get(), fields);
    std::stringstream ss;
    info.PrintValue(ss);
    EXPECT_EQ(ss.str(),
              "{class:[test_class],dataset:{test_dataset:test_dataset_value},"
              "tag:view,index:1,name:test_name,id:test_id}");
  }
}

TEST_F(FiberNodeInfoTest, GetNodesInfo) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);

  auto element_0 = manager->CreateFiberNode("view");
  auto element = manager->CreateFiberNode("view");
  element->SetClass("test_class");
  element->SetIdSelector("test_id");
  element->AddDataset("test_dataset", lepus::Value("test_dataset_value"));
  element->SetAttribute("name", lepus::Value("test_name"));
  {
    auto info =
        FiberNodeInfo::GetNodesInfo({element_0.get(), element.get()},
                                    {"tag", "id", "dataSet", "index", "class"});
    std::stringstream ss;
    info.PrintValue(ss);
    EXPECT_EQ(ss.str(),
              "[{class:[],dataSet:{},id:,index:0,tag:view},{class:[test_class],"
              "dataSet:{test_dataset:test_dataset_value},id:test_id,index:0,"
              "tag:view}]");
  }
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
