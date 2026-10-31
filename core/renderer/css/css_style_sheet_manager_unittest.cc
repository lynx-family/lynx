// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/css_style_sheet_manager.h"

#include <fstream>

#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/css/css_fragment_decorator.h"
#include "core/renderer/css/ng/parser/css_tokenizer.h"
#include "core/renderer/css/ng/selector/css_parser_context.h"
#include "core/renderer/css/ng/selector/css_selector_parser.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/tasm_operation_queue.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "core/template_bundle/template_codec/binary_decoder/template_binary_reader.h"
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
    tasm = std::make_unique<lynx::tasm::TemplateAssembler>(
        *delegate.get(), std::move(manager), delegate.get(), 0);
  }

  std::unique_ptr<::testing::NiceMock<test::MockTasmDelegate>> delegate;
  std::unique_ptr<TemplateAssembler> tasm;
};

TEST_F(CSSStyleSheetManagerTest, LoadTemplate) {
  std::string path = "../../core/renderer/css/testing/counter.js";
  std::ifstream istream(path, std::ios::in | std::ios::binary);
  EXPECT_TRUE(istream.good());
  std::vector<uint8_t> data((std::istreambuf_iterator<char>(istream)),
                            std::istreambuf_iterator<char>());
  auto pipeline_options = std::make_shared<PipelineOptions>();
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
  auto pipeline_options = std::make_shared<PipelineOptions>();
  tasm->LoadTemplate(path, std::move(data), nullptr, pipeline_options);
  auto entry = tasm->FindEntry(DEFAULT_ENTRY_NAME);

  auto manager = entry->GetStyleSheetManager();
  EXPECT_TRUE(manager);
  // cm->css_id()
  auto cf = manager->GetCSSStyleSheetForPage(1);
  cf = manager->GetCSSStyleSheetForComponent(1);
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

namespace {
class TrackedCSSFragment : public SharedCSSFragment {
 public:
  TrackedCSSFragment(int32_t id, bool& destroyed)
      : SharedCSSFragment(id), destroyed_(destroyed) {}
  ~TrackedCSSFragment() override { destroyed_ = true; }

 private:
  bool& destroyed_;
};
class StyleInvalidationView : public ViewElement {
 public:
  explicit StyleInvalidationView(ElementManager* manager)
      : ViewElement(manager) {}
  using Element::CheckHasInvalidationForClass;
  size_t PendingInvalidationCount() const {
    return invalidation_lists_.descendants.size();
  }
};
}  // namespace

TEST_F(CSSStyleSheetManagerTest, HotReplaceDoesNotModifySharedBundle) {
  bool destroyed = false;
  CSSStyleSheetManager bundle(nullptr);
  bundle.AddSharedCSSFragment(
      std::make_unique<TrackedCSSFragment>(1, destroyed));
  CSSStyleSheetManager first_page(nullptr);
  CSSStyleSheetManager second_page(nullptr);
  first_page.CopyFrom(bundle);
  second_page.CopyFrom(bundle);
  auto* original = second_page.GetCSSStyleSheetForComponent(1);
  CSSStyleSheetManager::CSSFragmentMap replacement;
  replacement.emplace(1, std::make_unique<SharedCSSFragment>(1));
  first_page.ReplaceStyleSheetsForHotUpdate(std::move(replacement));
  ASSERT_FALSE(destroyed);
  EXPECT_EQ(second_page.GetCSSStyleSheetForComponent(1), original);
  EXPECT_EQ(bundle.GetCSSStyleSheetForComponent(1), original);
  EXPECT_NE(first_page.GetCSSStyleSheetForComponent(1), original);
}

TEST_F(CSSStyleSheetManagerTest, HotRemoveDoesNotModifySharedBundle) {
  bool destroyed = false;
  CSSStyleSheetManager bundle(nullptr);
  bundle.AddSharedCSSFragment(
      std::make_unique<TrackedCSSFragment>(1, destroyed));
  CSSStyleSheetManager first_page(nullptr);
  CSSStyleSheetManager second_page(nullptr);
  first_page.CopyFrom(bundle);
  second_page.CopyFrom(bundle);
  auto* original = second_page.GetCSSStyleSheetForComponent(1);
  first_page.ReplaceStyleSheetsForHotUpdate({});
  ASSERT_FALSE(destroyed);
  EXPECT_EQ(first_page.GetCSSStyleSheetForComponent(1), nullptr);
  EXPECT_EQ(second_page.GetCSSStyleSheetForComponent(1), original);
  EXPECT_EQ(bundle.GetCSSStyleSheetForComponent(1), original);
}

TEST_F(CSSStyleSheetManagerTest, HotUpdateRebuildsImportsAndPageCache) {
  const std::string path = "../../core/renderer/css/testing/counter.js";
  std::ifstream stream(path, std::ios::binary);
  ASSERT_TRUE(stream.good());
  std::vector<uint8_t> binary((std::istreambuf_iterator<char>(stream)), {});
  TemplateBinaryReader reader(
      std::make_unique<lepus::ByteArrayInputStream>(binary));
  ASSERT_TRUE(reader.DecodeForHotUpdate());
  auto original = reader.DecodeCSSForHotUpdate();
  ASSERT_NE(original, nullptr);
  original->FlattenAllCSSFragment();

  TemplateEntry first_page;
  first_page.template_bundle() = reader.template_bundle();
  first_page.template_bundle().SetBinary(binary);
  auto manager = std::make_shared<CSSStyleSheetManager>(nullptr);
  manager->CopyFrom(*original);
  first_page.template_bundle().SetCSSStyleManager(manager);
  CSSStyleSheetManager second_page(nullptr);
  second_page.CopyFrom(*original);
  auto* second_sheet = second_page.GetCSSStyleSheetForPage(2);
  ASSERT_NE(second_sheet->GetCSSStyle(".common"), nullptr);
  ASSERT_NE(manager->GetCSSStyleSheetForPage(2)->GetCSSStyle(".common"),
            nullptr);

  auto& elements = *tasm->page_proxy()->element_manager();
  for (int i = 0; i < 3; ++i) {
    ASSERT_TRUE(
        first_page.UpdateStyleSheetForHotUpdate(0, nullptr, elements).empty());
    EXPECT_EQ(manager->GetCSSStyleSheetForPage(2)->GetCSSStyle(".common"),
              nullptr);
    EXPECT_EQ(manager->GetCSSStyleSheet(0), nullptr);
    ASSERT_TRUE(
        first_page.UpdateStyleSheetForHotUpdate(0, &binary, elements).empty());
    EXPECT_NE(manager->GetCSSStyleSheetForPage(2)->GetCSSStyle(".common"),
              nullptr);
    EXPECT_EQ(second_page.GetCSSStyleSheetForPage(2), second_sheet);
    EXPECT_NE(second_sheet->GetCSSStyle(".common"), nullptr);
  }

  // A failed update must leave the last working graph and update sources
  // intact.
  auto* last_sheet = manager->GetCSSStyleSheetForPage(2);
  EXPECT_FALSE(
      first_page.UpdateStyleSheetForHotUpdate(99, &binary, elements).empty());
  EXPECT_EQ(manager->GetCSSStyleSheetForPage(2), last_sheet);
  ASSERT_TRUE(
      first_page.UpdateStyleSheetForHotUpdate(0, nullptr, elements).empty());
  EXPECT_EQ(manager->GetCSSStyleSheet(0), nullptr);
}

TEST_F(CSSStyleSheetManagerTest, HotUpdateInvalidatesDetachedElements) {
  auto& elements = *tasm->page_proxy()->element_manager();
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  elements.SetConfig(config);
  auto manager = std::make_shared<CSSStyleSheetManager>(nullptr);
  auto fragment = std::make_unique<SharedCSSFragment>(1);
  fragment->SetEnableCSSInvalidation();
  fragment->SetEnableCSSSelector();
  css::CSSParserContext context;
  css::CSSTokenizer tokenizer(".theme .row");
  const auto tokens = tokenizer.TokenizeToEOF();
  css::CSSParserTokenRange range(tokens);
  auto selectors = css::CSSSelectorParser::ParseSelector(range, &context);
  const auto size = css::CSSSelectorParser::FlattenedSize(selectors);
  auto selector_array = std::make_unique<css::LynxCSSSelector[]>(size);
  css::CSSSelectorParser::AdoptSelectorVector(selectors, selector_array.get(),
                                              size);
  fragment->AddStyleRule(std::move(selector_array), nullptr);
  manager->AddSharedCSSFragment(std::move(fragment));
  auto parent = elements.CreateFiberView();
  auto child = fml::AdoptRef(new StyleInvalidationView(&elements));
  parent->InsertNode(child);
  child->SetCSSID(1);
  child->set_style_sheet_manager(manager);
  ASSERT_TRUE(static_cast<CSSFragmentDecorator*>(child->GetRelatedCSSFragment())
                  ->HasIntrinsicStyleSheets());
  ASSERT_TRUE(child->CheckHasInvalidationForClass({}, {"theme"}));
  ASSERT_EQ(child->PendingInvalidationCount(), 1u);
  parent->RemoveNode(child);

  elements.InvalidateStyleSheetsForHotUpdate();
  EXPECT_EQ(child->PendingInvalidationCount(), 0u);
  manager->ReplaceStyleSheetsForHotUpdate({});
  EXPECT_FALSE(
      static_cast<CSSFragmentDecorator*>(child->GetRelatedCSSFragment())
          ->HasIntrinsicStyleSheets());
  parent->InsertNode(child);
  EXPECT_FALSE(
      static_cast<CSSFragmentDecorator*>(child->GetRelatedCSSFragment())
          ->HasIntrinsicStyleSheets());
}

TEST_F(CSSStyleSheetManagerTest, HotUpdateRebindsSelectorImports) {
  auto make_graph = [](bool include_dependency) {
    CSSStyleSheetManager::CSSFragmentMap graph;
    auto root = std::make_unique<SharedCSSFragment>(
        1, std::vector<int32_t>{0}, CSSParserTokenMap{}, CSSKeyframesTokenMap{},
        CSSFontFaceRuleMap{});
    root->SetEnableCSSSelector();
    graph.emplace(1, std::move(root));
    if (include_dependency) {
      auto dependency = std::make_unique<SharedCSSFragment>(0);
      dependency->SetEnableCSSSelector();
      dependency->rule_set()->AddFeatureFlag(css::RuleSet::kHasMediaQuery);
      graph.emplace(0, std::move(dependency));
    }
    return graph;
  };
  CSSStyleSheetManager first_page(nullptr);
  first_page.ReplaceStyleSheetsForHotUpdate(make_graph(true));
  auto* original = first_page.GetCSSStyleSheetForComponent(1);
  ASSERT_TRUE(original->HasMediaQueryRules());
  CSSStyleSheetManager second_page(nullptr);
  second_page.CopyFrom(first_page);

  for (int i = 0; i < 3; ++i) {
    first_page.ReplaceStyleSheetsForHotUpdate(make_graph(false));
    EXPECT_TRUE(
        first_page.GetCSSStyleSheetForComponent(1)->rule_set()->deps().empty());
    EXPECT_FALSE(
        first_page.GetCSSStyleSheetForComponent(1)->HasMediaQueryRules());
    first_page.ReplaceStyleSheetsForHotUpdate(make_graph(true));
    auto* updated = first_page.GetCSSStyleSheetForComponent(1);
    ASSERT_EQ(updated->rule_set()->deps().size(), 1u);
    EXPECT_EQ(updated->rule_set()->deps()[0],
              first_page.GetCSSStyleSheet(0)->rule_set());
    EXPECT_TRUE(updated->HasMediaQueryRules());
    EXPECT_TRUE(original->HasMediaQueryRules());
    EXPECT_EQ(second_page.GetCSSStyleSheetForComponent(1), original);
  }
}

TEST_F(CSSStyleSheetManagerTest, HotUpdateUsesLazyReaderWithoutRecordedBinary) {
  const std::string path = "../../core/renderer/css/testing/counter.js";
  std::ifstream stream(path, std::ios::binary);
  ASSERT_TRUE(stream.good());
  std::vector<uint8_t> binary((std::istreambuf_iterator<char>(stream)), {});
  TemplateEntry entry;
  ASSERT_TRUE(entry.template_bundle().FromBinary(binary, true, path).empty());
  EXPECT_TRUE(entry.template_bundle().GetBinary().empty());
  auto manager = entry.GetStyleSheetManager();
  ASSERT_NE(manager->GetCSSStyleSheetForPage(2)->GetCSSStyle(".common"),
            nullptr);
  auto& elements = *tasm->page_proxy()->element_manager();
  ASSERT_TRUE(entry.UpdateStyleSheetForHotUpdate(0, nullptr, elements).empty());
  EXPECT_EQ(manager->GetCSSStyleSheetForPage(2)->GetCSSStyle(".common"),
            nullptr);
  ASSERT_TRUE(entry.UpdateStyleSheetForHotUpdate(0, &binary, elements).empty());
  EXPECT_NE(manager->GetCSSStyleSheetForPage(2)->GetCSSStyle(".common"),
            nullptr);
}

TEST_F(CSSStyleSheetManagerTest, HotUpdateRejectsLateDecodeWrites) {
  CSSStyleSheetManager manager(nullptr);
  manager.AddSharedCSSFragment(std::make_unique<SharedCSSFragment>(1));
  manager.ReplaceStyleSheetsForHotUpdate({});
  // Model a decoder that passed its stop check before the update was published.
  manager.AddSharedCSSFragment(std::make_unique<SharedCSSFragment>(1));
  EXPECT_EQ(manager.GetCSSStyleSheet(1), nullptr);
}

TEST_F(CSSStyleSheetManagerTest, HotUpdateRebindsLazyImportsAfterRemoval) {
  CSSStyleSheetManager manager(nullptr);
  manager.SetEnableCSSLazyImport(true);
  auto token = fml::MakeRefCounted<CSSParseToken>(CSSParserConfigs{});
  auto make_graph = [&token](bool include_dependency) {
    CSSStyleSheetManager::CSSFragmentMap graph;
    graph.emplace(1, std::make_unique<SharedCSSFragment>(
                         1, std::vector<int32_t>{0}, CSSParserTokenMap{},
                         CSSKeyframesTokenMap{}, CSSFontFaceRuleMap{}));
    if (include_dependency) {
      graph.emplace(0, std::make_unique<SharedCSSFragment>(
                           0, std::vector<int32_t>{},
                           CSSParserTokenMap{{".shared", token}},
                           CSSKeyframesTokenMap{}, CSSFontFaceRuleMap{}));
    }
    return graph;
  };
  manager.ReplaceStyleSheetsForHotUpdate(make_graph(true));
  auto* sheet = manager.GetCSSStyleSheetForComponent(1);
  EXPECT_EQ(sheet->GetCSSStyle(".shared"), token.get());
  manager.ReplaceStyleSheetsForHotUpdate(make_graph(false));
  sheet = manager.GetCSSStyleSheetForComponent(1);
  EXPECT_FALSE(sheet->HasCSSStyle());
  EXPECT_EQ(sheet->GetCSSStyle(".shared"), nullptr);
  EXPECT_EQ(sheet->GetSharedCSSStyle(".shared"), nullptr);
}

// TODO(songshourui.null): enable this test after replace fontfaces.js
TEST_F(CSSStyleSheetManagerTest, DISABLED_Fontfaces) {
  // testing/fontfaces
  std::string path = "../../core/renderer/css/testing/fontfaces.js";
  std::ifstream istream(path, std::ios::in | std::ios::binary);
  EXPECT_TRUE(istream.good());
  std::vector<uint8_t> data((std::istreambuf_iterator<char>(istream)),
                            std::istreambuf_iterator<char>());
  auto pipeline_options = std::make_shared<PipelineOptions>();
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
