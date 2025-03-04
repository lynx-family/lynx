// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/css_style_sheet_manager.h"

#include <fstream>

#include "core/base/threading/task_runner_manufactor.h"
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

class CSSStyleSheetManagerTest : public ::testing::Test {
 public:
  CSSStyleSheetManagerTest() = default;
  ~CSSStyleSheetManagerTest() override = default;

  void SetUp() override {
    base::UIThread::Init();
    LynxEnvConfig lynx_env_config(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
    delegate = std::make_unique<::testing::NiceMock<test::MockTasmDelegate>>();
    auto manager = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<MockPaintingContext>(), delegate.get(),
        lynx_env_config);
    tasm = std::make_shared<lynx::tasm::TemplateAssembler>(
        *delegate.get(), std::move(manager), 0);
  }

  std::unique_ptr<::testing::NiceMock<test::MockTasmDelegate>> delegate;
  std::shared_ptr<TemplateAssembler> tasm;
};

TEST_F(CSSStyleSheetManagerTest, LoadTemplate) {
  std::string path = "../../core/renderer/css/testing/counter.js";
  std::ifstream istream(path, std::ios::in | std::ios::binary);
  EXPECT_TRUE(istream.good());
  std::vector<uint8_t> data((std::istreambuf_iterator<char>(istream)),
                            std::istreambuf_iterator<char>());
  PipelineOptions pipeline_options;
  tasm->LoadTemplate(path, std::move(data), nullptr, pipeline_options);
  auto entry = tasm->FindEntry(DEFAULT_ENTRY_NAME);

  auto manager = entry->GetStyleSheetManager();
  EXPECT_EQ(manager->raw_fragments().size(), static_cast<size_t>(3));
  // pm->css_id();
  auto pf = manager->GetCSSStyleSheetForPage(2);
  EXPECT_TRUE(pf);
  EXPECT_TRUE(pf->is_baked());
  // The fragment has merged with common.ttss
  EXPECT_TRUE(pf->css().find(".common") != pf->css().end());
  // cm->css_id()
  auto cf = manager->GetCSSStyleSheetForComponent(1);
  EXPECT_TRUE(cf);
  EXPECT_TRUE(cf->is_baked());
  // The component has no common selector
  EXPECT_TRUE(cf->css().find(".common") == cf->css().end());
}

TEST_F(CSSStyleSheetManagerTest, Pseudo) {
  // testing/pseudo
  std::string path = "../../core/renderer/css/testing/pseudo.js";
  std::ifstream istream(path, std::ios::in | std::ios::binary);
  EXPECT_TRUE(istream.good());
  std::vector<uint8_t> data((std::istreambuf_iterator<char>(istream)),
                            std::istreambuf_iterator<char>());
  PipelineOptions pipeline_options;
  tasm->LoadTemplate(path, std::move(data), nullptr, pipeline_options);
  auto entry = tasm->FindEntry(DEFAULT_ENTRY_NAME);

  auto manager = entry->GetStyleSheetManager();
  EXPECT_TRUE(manager);
  // cm->css_id()
  auto cf = manager->GetCSSStyleSheetForPage(1);
  EXPECT_TRUE(cf);
  EXPECT_TRUE(cf->HasPseudoStyle());
  EXPECT_TRUE(cf->HasPseudoNotStyle());

  EXPECT_EQ(cf->pseudo_map().size(), static_cast<size_t>(4));
  EXPECT_TRUE(cf->GetPseudoStyle(":not(.xxx)"));
  // FIXME: The cascade style should be '.box .container'
  EXPECT_TRUE(cf->GetCascadeStyle(".box.container"));
  EXPECT_TRUE(cf->GetFontFaceRule("roboto")[0]);
  EXPECT_TRUE(cf->GetKeyframesRule("opacity-ani"));
}

// TODO(songshourui.null): enable this test after replace fontfaces.js
TEST_F(CSSStyleSheetManagerTest, DISABLED_Fontfaces) {
  // testing/fontfaces
  std::string path = "../../core/renderer/css/testing/fontfaces.js";
  std::ifstream istream(path, std::ios::in | std::ios::binary);
  EXPECT_TRUE(istream.good());
  std::vector<uint8_t> data((std::istreambuf_iterator<char>(istream)),
                            std::istreambuf_iterator<char>());
  PipelineOptions pipeline_options;
  tasm->LoadTemplate(path, std::move(data), nullptr, pipeline_options);
  auto entry = tasm->FindEntry(DEFAULT_ENTRY_NAME);

  auto manager = entry->GetStyleSheetManager();
  EXPECT_TRUE(manager);
  // cm->css_id()
  auto cf = manager->GetCSSStyleSheetForPage(1);
  EXPECT_TRUE(cf);
  EXPECT_FALSE(cf->HasPseudoStyle());
  EXPECT_FALSE(cf->HasPseudoNotStyle());

  auto token_list = cf->GetFontFaceRule("DroidSerif");
  EXPECT_EQ(token_list.size(), static_cast<size_t>(4));
  auto token = token_list[0];
  auto attrs = token->second;
  EXPECT_EQ(attrs.size(), static_cast<size_t>(4));
  EXPECT_EQ(attrs["font-weight"], "normal");
  EXPECT_EQ(attrs["font-style"], "normal");
  EXPECT_EQ(attrs["font-family"], "DroidSerif");
  EXPECT_EQ(attrs["src"],
            "url('DroidSerif-Regular-webfont.ttf') format('truetype')");

  token = token_list[1];
  attrs = token->second;
  EXPECT_EQ(attrs.size(), static_cast<size_t>(4));
  EXPECT_EQ(attrs["font-weight"], "normal");
  EXPECT_EQ(attrs["font-style"], "italic");
  EXPECT_EQ(attrs["font-family"], "DroidSerif");
  EXPECT_EQ(attrs["src"],
            "url('DroidSerif-Italic-webfont.ttf') format('truetype')");

  token = token_list[2];
  attrs = token->second;
  EXPECT_EQ(attrs.size(), static_cast<size_t>(4));
  EXPECT_EQ(attrs["font-weight"], "bold");
  EXPECT_EQ(attrs["font-style"], "normal");
  EXPECT_EQ(attrs["font-family"], "DroidSerif");
  EXPECT_EQ(attrs["src"],
            "url('DroidSerif-Bold-webfont.ttf') format('truetype')");

  token = token_list[3];
  attrs = token->second;
  EXPECT_EQ(attrs.size(), static_cast<size_t>(4));
  EXPECT_EQ(attrs["font-weight"], "bold");
  EXPECT_EQ(attrs["font-style"], "italic");
  EXPECT_EQ(attrs["font-family"], "DroidSerif");
  EXPECT_EQ(attrs["src"],
            "url('DroidSerif-BoldItalic-webfont.ttf') format('truetype')");
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
