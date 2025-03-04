// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/dom/snapshot_element.h"

#include <string>
#include <vector>

#include "core/renderer/css/css_property_id.h"
#include "core/renderer/css/css_utils.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/lynx_env_config.h"
#include "core/renderer/page_config.h"
#include "core/renderer/starlight/style/auto_gen_css_type.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace dom {
namespace test {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

class SnapshotElementTest : public ::testing::Test {
 public:
  SnapshotElementTest() = default;
  ~SnapshotElementTest() override = default;
  std::unique_ptr<lynx::tasm::ElementManager> manager;
  std::shared_ptr<::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>
      tasm_mediator;

  void SetUp() override {
    tasm::LynxEnvConfig lynx_env_config(kWidth, kHeight,
                                        kDefaultLayoutsUnitPerPx,
                                        kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    manager = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<tasm::MockPaintingContext>(), tasm_mediator.get(),
        lynx_env_config);

    auto config = std::make_shared<tasm::PageConfig>();
    config->SetEnableFiberArch(true);
    manager->SetConfig(config);
  }
};

TEST_F(SnapshotElementTest, SnapshotElementJsonContent) {
  auto page = manager->CreateFiberPage("11", 20);
  manager->SetRoot(page.get());
  manager->SetRootOnLayout(page->impl_id());

  auto element = manager->CreateFiberNode("view");
  element->UpdateLayout(50, 51);
  element->set_config_flatten(false);
  const auto key = base::String("attribute_key");
  const auto value_id = lepus::Value("attribute_value");
  element->SetAttribute(key, value_id);
  std::string clazz = "class1 class2 class3";
  tasm::ClassList classes = tasm::SplitClasses(clazz.c_str(), clazz.length());
  element->SetClasses(std::move(classes));
  page->InsertNode(element);

  SnapshotElement* new_root = dom::constructSnapshotElementTree(page.get());
  rapidjson::Document dumped_document;
  rapidjson::Value dumped_tree =
      dom::DumpSnapshotElementTreeRecursively(new_root, dumped_document);
  dumped_document.Swap(dumped_tree);
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  dumped_document.Accept(writer);
  const char* dump_str = buffer.GetString();
  const char* expect_str =
      "{\"i\":10,\"w\":0.0,\"h\":0.0,\"l\":0.0,\"t\":0.0,\"n\":\"page\",\"id\":"
      "\"\",\"f\":true,\"o\":0,\"c\":[{\"i\":11,\"w\":0.0,\"h\":0.0,\"l\":50.0,"
      "\"t\":51.0,\"n\":\"view\",\"id\":\"\",\"f\":false,\"o\":0,\"cl\":["
      "\"class1\",\"class2\",\"class3\"],\"at\":{\"attribute_key\":\"attribute_"
      "value\"}}]}";
  EXPECT_STREQ(dump_str, expect_str);
}

}  // namespace test
}  // namespace dom
}  // namespace lynx
