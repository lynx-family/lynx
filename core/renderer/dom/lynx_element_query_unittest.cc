// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/lynx_element_query.h"

#include <memory>
#include <vector>

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/block_element.h"
#include "core/renderer/dom/fiber/page_element.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace tasm {
namespace {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

class LynxElementQueryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    LynxEnvConfig lynx_env_config(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
    manager_ = std::make_unique<ElementManager>(
        std::make_unique<MockPaintingContext>(), &tasm_mediator_,
        lynx_env_config, PageOptions());
    auto config = std::make_shared<PageConfig>();
    config->SetEnableFiberArch(true);
    manager_->SetConfig(config);
  }

  fml::RefPtr<ViewElement> CreateView(const base::String& id) {
    auto view = manager_->CreateFiberView();
    view->SetIdSelector(id);
    return view;
  }

  fml::RefPtr<Element> CreateRawText(const base::String& text) {
    auto raw_text = manager_->CreateFiberElement("raw-text");
    raw_text->data_model()->SetStaticAttribute(base::String("text"),
                                               lepus::Value(text));
    return raw_text;
  }

  fml::RefPtr<BlockElement> CreateBlock() {
    return fml::AdoptRef<BlockElement>(
        new BlockElement(manager_.get(), "block"));
  }

  fml::RefPtr<PageElement> CreatePage() {
    auto page = manager_->CreateFiberPage(base::String("0"), 0);
    manager_->SetFiberPageElement(page);
    manager_->SetRoot(page.get());
    return page;
  }

  ::testing::NiceMock<test::MockTasmDelegate> tasm_mediator_;
  std::unique_ptr<ElementManager> manager_;
};

TEST_F(LynxElementQueryTest, GetRootSignReturnsPageElement) {
  auto page = CreatePage();

  EXPECT_EQ(LynxElementQuery::GetRootSign(manager_.get()), page->impl_id());
}

TEST_F(LynxElementQueryTest, QueryIdentityReturnsTagIdAndSign) {
  auto page = CreatePage();
  auto child = CreateView(base::String("title"));
  page->InsertNode(child);

  LynxElementIdentity identity;
  ASSERT_TRUE(LynxElementQuery::GetIdentity(manager_.get(), child->impl_id(),
                                            identity));
  EXPECT_EQ(identity.sign, child->impl_id());
  EXPECT_EQ(identity.tag, "view");
  EXPECT_EQ(identity.id, "title");
}

TEST_F(LynxElementQueryTest, FindByIdSearchesDescendants) {
  auto page = CreatePage();
  auto parent = CreateView(base::String("container"));
  auto child = CreateView(base::String("target"));
  parent->InsertNode(child);
  page->InsertNode(parent);

  EXPECT_EQ(
      LynxElementQuery::FindById(manager_.get(), page->impl_id(), "target"),
      child->impl_id());
  EXPECT_EQ(
      LynxElementQuery::FindById(manager_.get(), page->impl_id(), "missing"),
      LynxElementQuery::kInvalidSign);
}

TEST_F(LynxElementQueryTest, FindByIdMatchesIdLiterally) {
  auto page = CreatePage();
  auto literal_id = CreateView(base::String("card.title"));
  auto selector_like_match = CreateView(base::String("card"));
  selector_like_match->data_model()->SetStaticClass({base::String("title")});
  page->InsertNode(selector_like_match);
  page->InsertNode(literal_id);

  EXPECT_EQ(
      LynxElementQuery::FindById(manager_.get(), page->impl_id(), "card.title"),
      literal_id->impl_id());
}

TEST_F(LynxElementQueryTest, QueriesUseScopedChildrenInAirModeFiber) {
  manager_->GetConfig()->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_FIBER);
  auto page = CreatePage();
  auto block = CreateBlock();
  block->SetIdSelector(base::String("logical-block"));
  auto rendered_child = CreateView(base::String("rendered-child"));
  page->InsertNode(block);
  block->InsertNode(rendered_child);

  std::vector<int32_t> children;
  ASSERT_TRUE(LynxElementQuery::GetChildrenSigns(manager_.get(),
                                                 page->impl_id(), children));
  ASSERT_EQ(children.size(), 1U);
  EXPECT_EQ(children[0], rendered_child->impl_id());
  EXPECT_EQ(LynxElementQuery::FindById(manager_.get(), page->impl_id(),
                                       "rendered-child"),
            rendered_child->impl_id());
  EXPECT_EQ(LynxElementQuery::FindById(manager_.get(), page->impl_id(),
                                       "logical-block"),
            LynxElementQuery::kInvalidSign);

  std::string json =
      LynxElementQuery::ToJSONString(manager_.get(), page->impl_id());
  rapidjson::Document document;
  document.Parse(json.c_str());

  ASSERT_FALSE(document.HasParseError());
  ASSERT_TRUE(document.HasMember("children"));
  ASSERT_EQ(document["children"].Size(), 1U);
  ASSERT_STREQ(document["children"][0]["tagName"].GetString(), "view");
  ASSERT_STREQ(document["children"][0]["id"].GetString(), "rendered-child");
}

TEST_F(LynxElementQueryTest, InvalidSignDoesNotMapBackToRoot) {
  auto page = CreatePage();
  auto child = CreateView(base::String("target"));
  page->InsertNode(child);

  int32_t missing_sign =
      LynxElementQuery::FindById(manager_.get(), page->impl_id(), "missing");
  EXPECT_EQ(missing_sign, LynxElementQuery::kInvalidSign);
  EXPECT_FALSE(LynxElementQuery::IsAlive(manager_.get(), missing_sign));

  LynxElementIdentity identity;
  EXPECT_FALSE(
      LynxElementQuery::GetIdentity(manager_.get(), missing_sign, identity));
  EXPECT_EQ(LynxElementQuery::GetParentSign(manager_.get(), missing_sign),
            LynxElementQuery::kInvalidSign);
}

TEST_F(LynxElementQueryTest, ParentAndChildrenUseScopedTree) {
  auto page = CreatePage();
  auto first = CreateView(base::String("first"));
  auto second = CreateView(base::String("second"));
  page->InsertNode(first);
  page->InsertNode(second);

  EXPECT_EQ(LynxElementQuery::GetParentSign(manager_.get(), first->impl_id()),
            page->impl_id());

  std::vector<int32_t> children;
  ASSERT_TRUE(LynxElementQuery::GetChildrenSigns(manager_.get(),
                                                 page->impl_id(), children));
  ASSERT_EQ(children.size(), 2U);
  EXPECT_EQ(children[0], first->impl_id());
  EXPECT_EQ(children[1], second->impl_id());

  EXPECT_FALSE(LynxElementQuery::GetChildrenSigns(
      manager_.get(), LynxElementQuery::kInvalidSign, children));
}

TEST_F(LynxElementQueryTest, AttributesUsePersistentDataModel) {
  auto page = CreatePage();
  auto child = CreateView(base::String("title"));
  child->data_model()->SetStaticAttribute(base::String("mode"),
                                          lepus::Value("readonly"));
  page->InsertNode(child);

  lepus::Value attributes =
      LynxElementQuery::GetAttributes(manager_.get(), child->impl_id());
  ASSERT_TRUE(attributes.IsTable());
  EXPECT_EQ(attributes.Table()->GetValue(base::String("mode")).StdString(),
            "readonly");

  lepus::Value attribute =
      LynxElementQuery::GetAttribute(manager_.get(), child->impl_id(), "mode");
  ASSERT_TRUE(attribute.IsString());
  EXPECT_EQ(attribute.StdString(), "readonly");
}

TEST_F(LynxElementQueryTest, PositionInfoValueUsesQueryKeys) {
  auto page = CreatePage();
  auto child = CreateView(base::String("positioned"));
  child->UpdateLayout(1, 2, 3, 4, {5, 6, 7, 8}, {9, 10, 11, 12},
                      {13, 14, 15, 16}, nullptr, 0);
  page->InsertNode(child);

  lepus::Value value =
      LynxElementQuery::GetPositionInfoValue(manager_.get(), child->impl_id());

  ASSERT_TRUE(value.IsTable());
  auto table = value.Table();
  EXPECT_EQ(
      table->GetValue(BASE_STATIC_STRING(LynxElementQuery::kLeftKey)).Number(),
      1);
  EXPECT_EQ(
      table->GetValue(BASE_STATIC_STRING(LynxElementQuery::kTopKey)).Number(),
      2);
  EXPECT_EQ(
      table->GetValue(BASE_STATIC_STRING(LynxElementQuery::kWidthKey)).Number(),
      3);
  EXPECT_EQ(table->GetValue(BASE_STATIC_STRING(LynxElementQuery::kHeightKey))
                .Number(),
            4);
  EXPECT_EQ(
      table->GetValue(BASE_STATIC_STRING(LynxElementQuery::kPaddingLeftKey))
          .Number(),
      5);
  EXPECT_EQ(
      table->GetValue(BASE_STATIC_STRING(LynxElementQuery::kMarginLeftKey))
          .Number(),
      9);
  EXPECT_EQ(
      table->GetValue(BASE_STATIC_STRING(LynxElementQuery::kBorderLeftWidthKey))
          .Number(),
      13);

  EXPECT_TRUE(LynxElementQuery::GetPositionInfoValue(
                  manager_.get(), LynxElementQuery::kInvalidSign)
                  .IsNil());
}

TEST_F(LynxElementQueryTest, ToJSONStringDumpsTemplateLikeTreeInNative) {
  auto page = CreatePage();
  page->UpdateLayout(0, 0, 1080, 1920, {0}, {0}, {0}, nullptr, 0);

  auto dump_root = CreateView(base::String("dump-root"));
  dump_root->AddDataset(base::String("case"), lepus::Value("root"));
  dump_root->data_model()->SetStaticAttribute(base::String("lynx-test-tag"),
                                              lepus::Value("dump-root"));
  dump_root->UpdateLayout(90, 120, 900, 1200, {0}, {0}, {0}, nullptr, 0);

  auto title = manager_->CreateFiberElement("text");
  title->SetIdSelector(base::String("title"));
  title->data_model()->SetStaticAttribute(base::String("lynx-test-tag"),
                                          lepus::Value("title"));
  title->data_model()->SetStaticAttribute(base::String("text-maxline"),
                                          lepus::Value("1"));
  title->InsertNode(CreateRawText(base::String("Dump Title")));

  auto logo = manager_->CreateFiberElement("image");
  logo->SetIdSelector(base::String("logo"));
  logo->data_model()->SetStaticAttribute(base::String("lynx-test-tag"),
                                         lepus::Value("logo"));
  logo->data_model()->SetStaticAttribute(
      base::String("src"), lepus::Value("https://example.com/logo.png"));

  auto list = manager_->CreateFiberElement("scroll-view");
  list->SetIdSelector(base::String("list"));
  list->data_model()->SetStaticAttribute(base::String("lynx-test-tag"),
                                         lepus::Value("list"));
  list->data_model()->SetStaticAttribute(base::String("scroll-y"),
                                         lepus::Value(true));
  list->data_model()->SetStaticAttribute(base::String("scroll-bar-enable"),
                                         lepus::Value(true));

  auto item = CreateView(base::String("item-1"));
  item->data_model()->SetStaticAttribute(base::String("lynx-test-tag"),
                                         lepus::Value("item-1"));
  item->AddDataset(base::String("index"), lepus::Value("1"));
  item->InsertNode(CreateRawText(base::String("Item 1")));

  list->InsertNode(item);
  dump_root->InsertNode(title);
  dump_root->InsertNode(logo);
  dump_root->InsertNode(list);
  page->InsertNode(dump_root);

  std::string json =
      LynxElementQuery::ToJSONString(manager_.get(), page->impl_id());
  rapidjson::Document document;
  document.Parse(json.c_str());

  ASSERT_FALSE(document.HasParseError());
  ASSERT_TRUE(document.HasMember("sign"));
  ASSERT_TRUE(document["sign"].IsInt());
  EXPECT_EQ(document["sign"].GetInt(), page->impl_id());
  ASSERT_TRUE(document.HasMember("positionInfo"));
  ASSERT_TRUE(document["positionInfo"].IsObject());
  EXPECT_TRUE(document["positionInfo"].HasMember("left"));
  EXPECT_TRUE(document["positionInfo"].HasMember("top"));
  EXPECT_TRUE(document["positionInfo"].HasMember("width"));
  EXPECT_TRUE(document["positionInfo"].HasMember("height"));
  EXPECT_FALSE(document.HasMember("left"));
  EXPECT_FALSE(document.HasMember("top"));
  EXPECT_FALSE(document.HasMember("width"));
  EXPECT_FALSE(document.HasMember("height"));
  ASSERT_TRUE(document.HasMember("children"));
  ASSERT_TRUE(document["children"].IsArray());
  ASSERT_EQ(document["children"].Size(), 1U);

  const auto& root_json = document["children"][0];
  ASSERT_STREQ(root_json["tagName"].GetString(), "view");
  ASSERT_STREQ(root_json["id"].GetString(), "dump-root");
  ASSERT_EQ(root_json["positionInfo"]["left"].GetDouble(), 90);
  ASSERT_EQ(root_json["positionInfo"]["top"].GetDouble(), 120);
  ASSERT_EQ(root_json["positionInfo"]["width"].GetDouble(), 900);
  ASSERT_EQ(root_json["positionInfo"]["height"].GetDouble(), 1200);
  ASSERT_STREQ(root_json["dataset"]["case"].GetString(), "root");
  ASSERT_STREQ(root_json["attributes"]["lynx-test-tag"].GetString(),
               "dump-root");

  const auto& title_json = root_json["children"][0];
  ASSERT_STREQ(title_json["tagName"].GetString(), "text");
  ASSERT_STREQ(title_json["attributes"]["text-maxline"].GetString(), "1");
  ASSERT_STREQ(title_json["children"][0]["tagName"].GetString(), "raw-text");
  ASSERT_STREQ(title_json["children"][0]["attributes"]["text"].GetString(),
               "Dump Title");

  const auto& logo_json = root_json["children"][1];
  ASSERT_STREQ(logo_json["tagName"].GetString(), "image");
  ASSERT_STREQ(logo_json["attributes"]["src"].GetString(),
               "https://example.com/logo.png");

  const auto& list_json = root_json["children"][2];
  ASSERT_STREQ(list_json["tagName"].GetString(), "scroll-view");
  ASSERT_TRUE(list_json["attributes"]["scroll-y"].GetBool());
  ASSERT_TRUE(list_json["attributes"]["scroll-bar-enable"].GetBool());

  const auto& item_json = list_json["children"][0];
  ASSERT_STREQ(item_json["id"].GetString(), "item-1");
  ASSERT_STREQ(item_json["dataset"]["index"].GetString(), "1");
  ASSERT_STREQ(item_json["children"][0]["attributes"]["text"].GetString(),
               "Item 1");
}

}  // namespace
}  // namespace tasm
}  // namespace lynx
