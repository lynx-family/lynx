// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "devtool/lynx_devtool/agent/inspector_tasm_executor.h"

#include <sys/wait.h>

#include <chrono>
#include <cstddef>
#include <future>
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/renderer/css/css_fragment_decorator.h"
#include "core/renderer/css/css_parser_token.h"
#include "core/renderer/css/ng/parser/css_parser_token_range.h"
#include "core/renderer/css/ng/parser/css_tokenizer.h"
#include "core/renderer/css/ng/parser/media_query_parser.h"
#include "core/renderer/css/ng/parser/supports_condition_parser.h"
#include "core/renderer/css/ng/selector/css_parser_context.h"
#include "core/renderer/css/ng/selector/css_selector_parser.h"
#include "core/renderer/css/ng/style/condition_rule.h"
#include "core/renderer/css/ng/style/style_rule.h"
#include "core/renderer/css/shared_css_fragment.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/component_element.h"
#include "core/renderer/dom/fiber/page_element.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "devtool/base_devtool/native/public/devtool_status.h"
#include "devtool/base_devtool/native/test/message_sender_mock.h"
#include "devtool/base_devtool/native/test/mock_receiver.h"
#include "devtool/lynx_devtool/agent/inspector_ui_executor.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
#include "devtool/lynx_devtool/element/element_helper.h"
#include "devtool/lynx_devtool/element/element_inspector.h"
#include "devtool/testing/mock/devtool_platform_facade_mock.h"
#include "devtool/testing/mock/lynx_devtool_ng_mock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/jsoncpp/include/json/value.h"

namespace lynx {
namespace testing {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;
static constexpr size_t kBoxModelSize = 34;

static std::vector<double> BuildBoxModel(double start) {
  std::vector<double> box_model(kBoxModelSize);
  for (size_t i = 0; i < box_model.size(); ++i) {
    box_model[i] = start + static_cast<double>(i);
  }
  return box_model;
}

static std::vector<double> BuildBoxModelWithBorder(double left, double top,
                                                   double right,
                                                   double bottom) {
  std::vector<double> box_model(kBoxModelSize, 0.0);
  box_model[0] = 100;
  box_model[1] = 100;
  box_model[18] = 100;
  box_model[10] = box_model[18] + left;
  box_model[12] = 200;
  box_model[20] = box_model[12] + right;
  box_model[19] = 300;
  box_model[11] = box_model[19] + top;
  box_model[17] = 400;
  box_model[25] = box_model[17] + bottom;
  return box_model;
}

class RecordingMessageSender : public devtool::MessageSender {
 public:
  void SendMessage(const std::string& type, const Json::Value& msg) override {
    json_messages_.emplace_back(type, msg);
  }

  void SendMessage(const std::string& type, const std::string& msg) override {
    string_messages_.emplace_back(type, msg);
  }

  std::vector<std::pair<std::string, Json::Value>> json_messages_;
  std::vector<std::pair<std::string, std::string>> string_messages_;
};

std::unique_ptr<css::LynxCSSSelector[]> CreateSelectorArray(
    const std::string& selector) {
  css::CSSParserContext context;
  css::CSSTokenizer tokenizer(selector);
  auto parser_tokens = tokenizer.TokenizeToEOF();
  css::CSSParserTokenRange range(parser_tokens);
  auto selector_vector = css::CSSSelectorParser::ParseSelector(range, &context);
  size_t flattened_size =
      css::CSSSelectorParser::FlattenedSize(selector_vector);
  auto selector_arr = std::make_unique<css::LynxCSSSelector[]>(flattened_size);
  css::CSSSelectorParser::AdoptSelectorVector(
      selector_vector, selector_arr.get(), flattened_size);
  return selector_arr;
}

fml::RefPtr<tasm::CSSParseToken> CreateParseToken(
    const std::string& selector, tasm::CSSPropertyID property_id,
    const std::string& value) {
  tasm::CSSParserConfigs parser_configs;
  auto token = fml::MakeRefCounted<tasm::CSSParseToken>(parser_configs);
  token->raw_attributes_[property_id] =
      tasm::CSSValue::MakePlainString(value.c_str());
  auto sheet = std::make_shared<tasm::CSSSheet>(selector);
  token->sheets().emplace_back(sheet);
  return token;
}

struct MediaRuleForTest {
  std::string selector;
  tasm::CSSPropertyID property_id;
  std::string value;
  std::string media_text;
  std::vector<std::string> layer_path;
};

struct SupportsRuleForTest {
  std::string selector;
  tasm::CSSPropertyID property_id;
  std::string value;
  std::string supports_text;
};

std::shared_ptr<tasm::SharedCSSFragment> CreateMediaCSSFragment(
    std::initializer_list<MediaRuleForTest> rules) {
  tasm::CSSParserTokenMap token_map;
  std::vector<fml::RefPtr<tasm::CSSParseToken>> tokens;
  tokens.reserve(rules.size());

  for (const auto& rule : rules) {
    auto token = CreateParseToken(rule.selector, rule.property_id, rule.value);
    token_map.insert(std::make_pair(rule.selector, token));
    tokens.push_back(std::move(token));
  }

  const std::vector<int32_t> dependent_ids;
  tasm::CSSKeyframesTokenMap keyframes;
  tasm::CSSFontFaceRuleMap font_faces;
  auto fragment = std::make_shared<tasm::SharedCSSFragment>(
      1, dependent_ids, token_map, keyframes, font_faces);
  fragment->SetEnableCSSSelector();

  size_t index = 0;
  for (const auto& rule : rules) {
    css::CascadeLayer* layer = nullptr;
    if (!rule.layer_path.empty()) {
      fragment->SetEnableCSSRule(true);
      layer =
          fragment->GetOrCreateRootLayer()->GetOrAddSubLayer(rule.layer_path);
    }
    auto condition_rule =
        fml::MakeRefCounted<css::ConditionRule>(fragment.get());
    condition_rule->SetMediaQueries(
        css::MediaQueryParser::ParseMediaQuerySet(rule.media_text));
    condition_rule->AddStyleRule(
        fml::MakeRefCounted<css::StyleRule>(CreateSelectorArray(rule.selector),
                                            tokens[index++]),
        layer);
    fragment->AddConditionRule(std::move(condition_rule));
  }

  return fragment;
}

std::shared_ptr<tasm::SharedCSSFragment> CreateSupportsCSSFragment(
    std::initializer_list<SupportsRuleForTest> rules) {
  tasm::CSSParserTokenMap token_map;
  std::vector<fml::RefPtr<tasm::CSSParseToken>> tokens;
  tokens.reserve(rules.size());

  for (const auto& rule : rules) {
    auto token = CreateParseToken(rule.selector, rule.property_id, rule.value);
    token_map.insert(std::make_pair(rule.selector, token));
    tokens.push_back(std::move(token));
  }

  const std::vector<int32_t> dependent_ids;
  tasm::CSSKeyframesTokenMap keyframes;
  tasm::CSSFontFaceRuleMap font_faces;
  auto fragment = std::make_shared<tasm::SharedCSSFragment>(
      1, dependent_ids, token_map, keyframes, font_faces);
  fragment->SetEnableCSSSelector();

  size_t index = 0;
  for (const auto& rule : rules) {
    auto supports_condition =
        css::SupportsConditionParser::Parse(rule.supports_text);
    auto condition_rule =
        fml::MakeRefCounted<css::ConditionRule>(fragment.get());
    condition_rule->SetSupportsCondition(std::move(supports_condition));
    condition_rule->AddStyleRule(fml::MakeRefCounted<css::StyleRule>(
        CreateSelectorArray(rule.selector), tokens[index++]));
    fragment->AddConditionRule(std::move(condition_rule));
  }

  return fragment;
}

struct MediaQueryTestDom {
  fml::RefPtr<tasm::PageElement> page;
  fml::RefPtr<tasm::ComponentElement> component;
  fml::RefPtr<tasm::Element> element;
  tasm::Element* style_value = nullptr;
  std::shared_ptr<tasm::SharedCSSFragment> fragment;
};

std::string FirstMediaText(tasm::SharedCSSFragment* fragment) {
  std::string media_text;
  fragment->rule_set()->ForEachConditionRule(
      [&media_text](const css::ConditionRule& rule) {
        if (media_text.empty() && rule.HasStructuredMediaQuery()) {
          media_text = rule.MediaQueries()->Serialize();
        }
      });
  return media_text;
}

std::string FirstSupportsText(tasm::SharedCSSFragment* fragment) {
  std::string supports_text;
  fragment->rule_set()->ForEachConditionRule(
      [&supports_text](const css::ConditionRule& rule) {
        if (supports_text.empty() && rule.HasStructuredSupportsRules()) {
          supports_text = rule.SupportsCondition()->Serialize();
        }
      });
  return supports_text;
}

void CreateStyleValueElement(tasm::ElementManager* manager,
                             tasm::Element* component, MediaQueryTestDom& dom) {
  auto style_value_ref = manager->CreateFiberElement("stylevalue");
  dom.style_value = style_value_ref.get();
  devtool::ElementInspector::InitForInspector(std::make_tuple(dom.style_value));
  devtool::ElementInspector::InitStyleValueElement(
      std::make_tuple(dom.style_value, component));
  devtool::ElementInspector::SetStyleRoot(
      std::make_tuple(dom.style_value, dom.style_value));
  dom.style_value->set_parent(component);
  style_value_ref.AbandonRef();
}

MediaQueryTestDom BuildMediaQueryTestDom(
    tasm::ElementManager* manager,
    std::initializer_list<MediaRuleForTest> rules) {
  MediaQueryTestDom dom;
  dom.fragment = CreateMediaCSSFragment(rules);
  dom.page = manager->CreateFiberPage("page", 0);
  manager->SetFiberPageElement(dom.page);
  devtool::ElementInspector::InitForInspector(std::make_tuple(dom.page.get()));

  dom.component = manager->CreateFiberComponent(
      lynx::base::String("21"), 100, lynx::base::String("__Card__"),
      lynx::base::String("TestComp"),
      lynx::base::String("/index/components/TestComp"));
  dom.component->style_sheet_ =
      std::make_unique<tasm::CSSFragmentDecorator>(dom.fragment.get(), manager);
  devtool::ElementInspector::InitForInspector(
      std::make_tuple(dom.component.get()));
  dom.page->InsertNode(dom.component);

  dom.element = manager->CreateFiberElement("view");
  dom.element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(dom.component->impl_id()));
  for (const auto& rule : rules) {
    if (!rule.selector.empty() && rule.selector[0] == '.') {
      const std::string class_name = rule.selector.substr(1);
      dom.element->data_model()->SetClass(class_name.c_str());
    }
  }
  devtool::ElementInspector::InitForInspector(
      std::make_tuple(dom.element.get()));
  CreateStyleValueElement(manager, dom.component.get(), dom);
  devtool::ElementInspector::SetStyleValueElement(
      std::make_tuple(dom.element.get(), dom.style_value));
  devtool::ElementInspector::SetStyleRoot(
      std::make_tuple(dom.element.get(), dom.style_value));
  dom.component->InsertNode(dom.element);
  return dom;
}

MediaQueryTestDom BuildSupportsTestDom(
    tasm::ElementManager* manager,
    std::initializer_list<SupportsRuleForTest> rules) {
  MediaQueryTestDom dom;
  dom.fragment = CreateSupportsCSSFragment(rules);
  dom.page = manager->CreateFiberPage("page", 0);
  manager->SetFiberPageElement(dom.page);
  devtool::ElementInspector::InitForInspector(std::make_tuple(dom.page.get()));

  dom.component = manager->CreateFiberComponent(
      lynx::base::String("21"), 100, lynx::base::String("__Card__"),
      lynx::base::String("TestComp"),
      lynx::base::String("/index/components/TestComp"));
  dom.component->style_sheet_ =
      std::make_unique<tasm::CSSFragmentDecorator>(dom.fragment.get());
  devtool::ElementInspector::InitForInspector(
      std::make_tuple(dom.component.get()));
  dom.page->InsertNode(dom.component);

  dom.element = manager->CreateFiberElement("view");
  dom.element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(dom.component->impl_id()));
  for (const auto& rule : rules) {
    if (!rule.selector.empty() && rule.selector[0] == '.') {
      const std::string class_name = rule.selector.substr(1);
      dom.element->data_model()->SetClass(class_name.c_str());
    }
  }
  devtool::ElementInspector::InitForInspector(
      std::make_tuple(dom.element.get()));
  CreateStyleValueElement(manager, dom.component.get(), dom);
  devtool::ElementInspector::SetStyleValueElement(
      std::make_tuple(dom.element.get(), dom.style_value));
  devtool::ElementInspector::SetStyleRoot(
      std::make_tuple(dom.element.get(), dom.style_value));
  dom.component->InsertNode(dom.element);
  return dom;
}

class InspectorTasmExecutorTest : public ::testing::Test {
 public:
  InspectorTasmExecutorTest() = default;
  ~InspectorTasmExecutorTest() override {}

  void SetUp() override {
    lynx::tasm::LynxEnvConfig lynx_env_config(
        kWidth, kHeight, kDefaultLayoutsUnitPerPx,
        kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator_ = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    manager_ = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<lynx::tasm::MockPaintingContext>(),
        tasm_mediator_.get(), lynx_env_config);
    devtool::MockReceiver::GetInstance().ResetAll();
    devtool_mediator_ = std::make_shared<lynx::devtool::LynxDevToolMediator>();
    devtools_ng_ = std::make_shared<lynx::testing::LynxDevToolNGMock>();
    message_sender_ = std::make_shared<devtool::MessageSenderMock>();
    devtools_ng_->message_sender_ = message_sender_;
    devtool_mediator_->devtool_wp_ = devtools_ng_;
    element_executor_ = std::make_shared<devtool::InspectorTasmExecutor>(
        devtool_mediator_, nullptr, 1);
    ui_thread_ = std::make_unique<fml::Thread>("ui");
    devtool_mediator_->ui_task_runner_ = ui_thread_->GetTaskRunner();
    devtool_mediator_->default_task_runner_ = ui_thread_->GetTaskRunner();
  }

  void FlushDevtoolTasks() {
    std::promise<void> p;
    auto f = p.get_future();
    ASSERT_TRUE(
        devtool_mediator_->RunOnDevToolThread([&p]() { p.set_value(); }, true));
    ASSERT_EQ(f.wait_for(std::chrono::seconds(5)), std::future_status::ready);
  }

  void FlushUITasks() {
    std::promise<void> p;
    auto f = p.get_future();
    ASSERT_TRUE(
        devtool_mediator_->RunOnUIThread([&p]() { p.set_value(); }, true));
    ASSERT_EQ(f.wait_for(std::chrono::seconds(5)), std::future_status::ready);
  }

  Json::Value ReceivedMessage() {
    Json::Value message;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(
        devtool::MockReceiver::GetInstance().received_message_.second,
        message));
    return message;
  }

 private:
  std::shared_ptr<devtool::InspectorTasmExecutor> element_executor_;
  std::shared_ptr<devtool::LynxDevToolMediator> devtool_mediator_;
  std::shared_ptr<devtool::MessageSender> message_sender_;
  std::shared_ptr<testing::LynxDevToolNGMock> devtools_ng_;
  std::shared_ptr<lynx::tasm::ElementManager> manager_;
  std::unique_ptr<fml::Thread> ui_thread_;
  std::shared_ptr<::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>
      tasm_mediator_;
};

TEST_F(InspectorTasmExecutorTest, GlobalPropsEnableDisableCase) {
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 1;

  element_executor_->GlobalPropsEnable(message_sender_, message);
  EXPECT_TRUE(element_executor_->IsGlobalPropsEnabled());
  Json::Value response = ReceivedMessage();
  EXPECT_EQ(response["id"], 1);
  EXPECT_TRUE(response["result"].isObject());

  element_executor_->GlobalPropsDisable(message_sender_, message);
  EXPECT_FALSE(element_executor_->IsGlobalPropsEnabled());
}

TEST_F(InspectorTasmExecutorTest, GlobalPropsGetWithoutTasmReturnsEmptyObject) {
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 3;

  element_executor_->GlobalPropsGet(message_sender_, message);

  Json::Value response = ReceivedMessage();
  EXPECT_EQ(response["id"], 3);
  EXPECT_TRUE(response["result"]["globalProps"].isObject());
  EXPECT_TRUE(response["result"]["globalProps"].empty());
  EXPECT_EQ(response["result"]["timestamp"].asUInt64(), 0u);
}

TEST_F(InspectorTasmExecutorTest, GlobalPropsReplaceRejectsInvalidParams) {
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 4;
  Json::Value array(Json::ValueType::arrayValue);
  message["params"]["globalProps"] = array;
  element_executor_->GlobalPropsReplace(message_sender_, message);

  Json::Value response = ReceivedMessage();
  EXPECT_EQ(response["error"]["code"], devtool::kInvalidParams);
  EXPECT_EQ(response["error"]["message"], "globalProps must be an object");
}

TEST_F(InspectorTasmExecutorTest,
       GlobalPropsChangedEmitsReplaceMarkerWithTimestamp) {
  element_executor_->global_props_enabled_ = true;

  element_executor_->GlobalPropsChanged();
  FlushDevtoolTasks();

  Json::Value event = ReceivedMessage();
  EXPECT_EQ(event["method"], "GlobalProps.changed");
  EXPECT_GT(event["params"]["timestamp"].asUInt64(), 0u);
  EXPECT_EQ(event["params"]["changes"].size(), 1u);
  EXPECT_EQ(event["params"]["changes"][0]["operation"], "replace");
}

TEST_F(InspectorTasmExecutorTest, SetDevtoolPlatformAbilityCase) {
  LOGI("InspectorTasmExecutorTest SetDevtoolPlatformAbilityCase start");

  std::shared_ptr<testing::DevToolPlatformFacadeMock> facade =
      std::make_shared<testing::DevToolPlatformFacadeMock>();
  element_executor_->SetDevToolPlatformFacade(facade);
  EXPECT_EQ(element_executor_->devtool_platform_facade_.get(), facade.get());
}

TEST_F(InspectorTasmExecutorTest, LayerTreeEnableCase) {
  LOGI("InspectorTasmExecutorTest LayerTreeEnableCase start");
  Json::Value message;
  message["id"] = 2;
  element_executor_->LayerTreeEnable(message_sender_, message);

  Json::Value res;
  Json::Reader reader;
  bool is_valid_json = reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, res);

  EXPECT_TRUE(is_valid_json);
  EXPECT_TRUE(element_executor_->layer_tree_enabled_);
}

TEST_F(InspectorTasmExecutorTest, LayerTreeDisableCase) {
  LOGI("InspectorTasmExecutorTest LayerTreeDisableCase start");
  Json::Value message;
  message["id"] = 6;
  element_executor_->LayerTreeDisable(message_sender_, message);

  Json::Value res;
  Json::Reader reader;
  bool is_valid_json = reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, res);

  EXPECT_TRUE(is_valid_json);
  EXPECT_EQ(res["id"], 6);
  EXPECT_FALSE(element_executor_->layer_tree_enabled_);
}

TEST_F(InspectorTasmExecutorTest, GetLayersForNodeReturnsCanonicalLayerTree) {
  auto dom = BuildMediaQueryTestDom(manager_.get(),
                                    {{".layered",
                                      tasm::CSSPropertyID::kPropertyIDWidth,
                                      "10px",
                                      "(min-width: 1000px)",
                                      {"framework", "components"}}});
  dom.fragment->GetOrCreateRootLayer()->GetOrAddSubLayer({"unused"});
  element_executor_->element_root_ = dom.element.get();

  auto response_sender = std::make_shared<RecordingMessageSender>();
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 100;
  message["params"]["nodeId"] =
      devtool::ElementInspector::NodeId(dom.element.get());
  element_executor_->GetLayersForNode(response_sender, message);

  ASSERT_EQ(response_sender->json_messages_.size(), 1U);
  const Json::Value& response = response_sender->json_messages_[0].second;
  EXPECT_EQ(response["id"].asInt(), 100);
  EXPECT_TRUE(response["error"].isNull());
  const Json::Value& root_layer = response["result"]["rootLayer"];
  EXPECT_EQ(root_layer["name"].asString(), "implicit outer layer");
  EXPECT_EQ(root_layer["order"].asUInt(), 3U);
  ASSERT_EQ(root_layer["subLayers"].size(), 2U);
  EXPECT_EQ(root_layer["subLayers"][0]["name"].asString(), "framework");
  EXPECT_EQ(root_layer["subLayers"][0]["order"].asUInt(), 1U);
  ASSERT_EQ(root_layer["subLayers"][0]["subLayers"].size(), 1U);
  EXPECT_EQ(root_layer["subLayers"][0]["subLayers"][0]["name"].asString(),
            "components");
  EXPECT_EQ(root_layer["subLayers"][0]["subLayers"][0]["order"].asUInt(), 0U);
  EXPECT_EQ(root_layer["subLayers"][1]["name"].asString(), "unused");
  EXPECT_EQ(root_layer["subLayers"][1]["order"].asUInt(), 2U);
}

TEST_F(InspectorTasmExecutorTest,
       GetLayersForNodeHandlesNodesWithoutLayersAndErrors) {
  auto dom = BuildMediaQueryTestDom(
      manager_.get(), {{".plain", tasm::CSSPropertyID::kPropertyIDWidth, "10px",
                        "(min-width: 1000px)"}});
  element_executor_->element_root_ = dom.element.get();
  auto response_sender = std::make_shared<RecordingMessageSender>();

  Json::Value node_without_layers(Json::ValueType::objectValue);
  node_without_layers["id"] = 101;
  node_without_layers["params"]["nodeId"] =
      devtool::ElementInspector::NodeId(dom.element.get());
  element_executor_->GetLayersForNode(response_sender, node_without_layers);
  ASSERT_EQ(response_sender->json_messages_.size(), 1U);
  const Json::Value& root_layer =
      response_sender->json_messages_[0].second["result"]["rootLayer"];
  EXPECT_EQ(root_layer["name"].asString(), "implicit outer layer");
  EXPECT_EQ(root_layer["order"].asUInt(), 0U);
  EXPECT_FALSE(root_layer.isMember("subLayers"));

  response_sender->json_messages_.clear();
  Json::Value invalid_params(Json::ValueType::objectValue);
  invalid_params["id"] = 102;
  element_executor_->GetLayersForNode(response_sender, invalid_params);
  ASSERT_EQ(response_sender->json_messages_.size(), 1U);
  EXPECT_EQ(response_sender->json_messages_[0].second["error"]["code"].asInt(),
            devtool::kInvalidParams);

  response_sender->json_messages_.clear();
  Json::Value missing_node(Json::ValueType::objectValue);
  missing_node["id"] = 103;
  missing_node["params"]["nodeId"] = 999999;
  element_executor_->GetLayersForNode(response_sender, missing_node);
  ASSERT_EQ(response_sender->json_messages_.size(), 1U);
  EXPECT_EQ(response_sender->json_messages_[0].second["error"]["code"].asInt(),
            devtool::kServerError);
  EXPECT_EQ(
      response_sender->json_messages_[0].second["error"]["message"].asString(),
      "Node is not an Element");
}

TEST_F(InspectorTasmExecutorTest, GetMediaQueriesReturnsMediaRules) {
  auto dom = BuildMediaQueryTestDom(
      manager_.get(),
      {{".active-media", tasm::CSSPropertyID::kPropertyIDWidth, "10px",
        "(min-width: 1000px)"},
       {".inactive-media", tasm::CSSPropertyID::kPropertyIDHeight, "20px",
        "(max-width: 10px)"}});

  element_executor_->element_root_ = dom.element.get();

  auto response_sender = std::make_shared<RecordingMessageSender>();
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 101;
  element_executor_->GetMediaQueries(response_sender, message);

  ASSERT_EQ(response_sender->json_messages_.size(), 1U);
  const Json::Value& response = response_sender->json_messages_[0].second;
  EXPECT_EQ(response["id"].asInt(), 101);
  ASSERT_TRUE(response["result"]["medias"].isArray());
  const Json::Value& medias = response["result"]["medias"];
  ASSERT_EQ(medias.size(), 2U);
  EXPECT_EQ(medias[0]["text"].asString(), "(min-width: 1000px)");
  EXPECT_EQ(medias[0]["source"].asString(), "mediaRule");
  EXPECT_EQ(medias[0]["styleSheetId"].asString(),
            std::to_string(devtool::ElementInspector::NodeId(dom.style_value)));
  EXPECT_EQ(medias[0]["range"]["startLine"].asInt(), 0);
  EXPECT_EQ(medias[0]["range"]["endLine"].asInt(), 0);
  EXPECT_EQ(medias[0]["range"]["startColumn"].asInt(), 0);
  EXPECT_EQ(medias[0]["range"]["endColumn"].asInt(),
            static_cast<int>(std::string("(min-width: 1000px)").length()));
  EXPECT_EQ(medias[1]["text"].asString(), "(max-width: 10px)");
  EXPECT_EQ(medias[1]["range"]["startLine"].asInt(), 1);
}

TEST_F(InspectorTasmExecutorTest, SetMediaTextUpdatesRuleCacheAndSendsEvents) {
  auto dom = BuildMediaQueryTestDom(
      manager_.get(), {{".active-media", tasm::CSSPropertyID::kPropertyIDWidth,
                        "10px", "(min-width: 1000px)"}});
  element_executor_->element_root_ = dom.element.get();

  auto matched_styles =
      devtool::ElementInspector::GetMatchedStyleSheet(dom.element.get());
  ASSERT_EQ(matched_styles.size(), 1U);
  EXPECT_EQ(matched_styles[0].media_text_, "(min-width: 1000px)");

  auto event_sender = std::make_shared<RecordingMessageSender>();
  devtools_ng_->message_sender_ = event_sender;
  auto response_sender = std::make_shared<RecordingMessageSender>();

  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 102;
  message["params"]["styleSheetId"] =
      std::to_string(devtool::ElementInspector::NodeId(dom.style_value));
  message["params"]["range"]["startLine"] = 0;
  message["params"]["range"]["startColumn"] = 0;
  message["params"]["range"]["endLine"] = 0;
  message["params"]["range"]["endColumn"] =
      static_cast<int>(std::string("(min-width: 1000px)").length());
  message["params"]["text"] = "@media (min-width: 100px)";

  element_executor_->SetMediaText(response_sender, message);

  ASSERT_EQ(response_sender->json_messages_.size(), 1U);
  const Json::Value& response = response_sender->json_messages_[0].second;
  EXPECT_EQ(response["id"].asInt(), 102);
  EXPECT_TRUE(response["error"].isNull());
  EXPECT_EQ(response["result"]["media"]["text"].asString(),
            "(min-width: 100px)");
  EXPECT_EQ(FirstMediaText(dom.fragment.get()), "(min-width: 100px)");

  auto updated_sheet = devtool::ElementInspector::GetStyleSheetByName(
      dom.element.get(), ".active-media");
  ASSERT_FALSE(updated_sheet.empty);
  EXPECT_EQ(updated_sheet.media_text_, "(min-width: 100px)");
  EXPECT_EQ(updated_sheet.media_range_.start_line_, 0);
  EXPECT_EQ(updated_sheet.media_range_.end_column_,
            static_cast<int>(std::string("(min-width: 100px)").length()));

  ASSERT_EQ(event_sender->json_messages_.size(), 2U);
  EXPECT_EQ(event_sender->json_messages_[0].second["method"].asString(),
            "CSS.mediaQueryResultChanged");
  EXPECT_EQ(event_sender->json_messages_[1].second["method"].asString(),
            "CSS.styleSheetChanged");
  EXPECT_EQ(event_sender->json_messages_[1]
                .second["params"]["styleSheetId"]
                .asString(),
            std::to_string(devtool::ElementInspector::NodeId(dom.style_value)));
}

TEST_F(InspectorTasmExecutorTest,
       SetMediaTextKeepsOtherStyleSheetCacheWithSameMediaIndex) {
  auto dom = BuildMediaQueryTestDom(
      manager_.get(), {{".active-media", tasm::CSSPropertyID::kPropertyIDWidth,
                        "10px", "(min-width: 1000px)"}});
  element_executor_->element_root_ = dom.element.get();

  auto matched_styles =
      devtool::ElementInspector::GetMatchedStyleSheet(dom.element.get());
  ASSERT_EQ(matched_styles.size(), 1U);

  const std::string target_style_sheet_id =
      std::to_string(devtool::ElementInspector::NodeId(dom.style_value));
  const std::string foreign_style_sheet_id = "foreign-style-sheet";
  const std::string foreign_media_text = "(max-width: 1px)";
  auto foreign_style_sheet = matched_styles[0];
  foreign_style_sheet.style_sheet_id_ = foreign_style_sheet_id;
  foreign_style_sheet.style_name_ = ".foreign-media";
  foreign_style_sheet.media_text_ = foreign_media_text;
  foreign_style_sheet.media_range_ = {
      0, 0, 0, static_cast<int>(foreign_media_text.length())};

  auto& style_sheet_map =
      devtool::ElementInspector::GetStyleSheetMap(dom.style_value);
  style_sheet_map.insert(
      {foreign_style_sheet.style_name_, foreign_style_sheet});

  auto response_sender = std::make_shared<RecordingMessageSender>();
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 105;
  message["params"]["styleSheetId"] = target_style_sheet_id;
  message["params"]["range"]["startLine"] = 0;
  message["params"]["range"]["startColumn"] = 0;
  message["params"]["range"]["endLine"] = 0;
  message["params"]["range"]["endColumn"] =
      static_cast<int>(std::string("(min-width: 1000px)").length());
  message["params"]["text"] = "@media (min-width: 100px)";

  element_executor_->SetMediaText(response_sender, message);

  ASSERT_EQ(response_sender->json_messages_.size(), 1U);
  const Json::Value& response = response_sender->json_messages_[0].second;
  EXPECT_EQ(response["id"].asInt(), 105);
  EXPECT_TRUE(response["error"].isNull());
  EXPECT_EQ(FirstMediaText(dom.fragment.get()), "(min-width: 100px)");

  auto updated_sheet = devtool::ElementInspector::GetStyleSheetByName(
      dom.element.get(), ".active-media");
  ASSERT_FALSE(updated_sheet.empty);
  EXPECT_EQ(updated_sheet.style_sheet_id_, target_style_sheet_id);
  EXPECT_EQ(updated_sheet.media_text_, "(min-width: 100px)");
  EXPECT_EQ(updated_sheet.media_range_.start_line_, 0);

  bool found_foreign_style_sheet = false;
  for (const auto& item : style_sheet_map) {
    const auto& style_sheet = item.second;
    if (style_sheet.style_sheet_id_ != foreign_style_sheet_id) {
      continue;
    }
    found_foreign_style_sheet = true;
    EXPECT_EQ(style_sheet.media_text_, foreign_media_text);
    EXPECT_EQ(style_sheet.media_range_.start_line_, 0);
    EXPECT_EQ(style_sheet.media_range_.end_column_,
              static_cast<int>(foreign_media_text.length()));
  }
  EXPECT_TRUE(found_foreign_style_sheet);
}

TEST_F(InspectorTasmExecutorTest, SetMediaTextReportsErrors) {
  auto dom = BuildMediaQueryTestDom(
      manager_.get(), {{".active-media", tasm::CSSPropertyID::kPropertyIDWidth,
                        "10px", "(min-width: 1000px)"}});
  element_executor_->element_root_ = dom.element.get();

  auto response_sender = std::make_shared<RecordingMessageSender>();
  Json::Value invalid_message(Json::ValueType::objectValue);
  invalid_message["id"] = 103;
  invalid_message["params"]["styleSheetId"] = "";
  invalid_message["params"]["text"] = "(min-width: 100px)";
  element_executor_->SetMediaText(response_sender, invalid_message);

  ASSERT_EQ(response_sender->json_messages_.size(), 1U);
  EXPECT_EQ(response_sender->json_messages_[0].second["error"]["code"].asInt(),
            devtool::kInvalidParams);

  response_sender->json_messages_.clear();
  Json::Value not_found_message(Json::ValueType::objectValue);
  not_found_message["id"] = 104;
  not_found_message["params"]["styleSheetId"] =
      std::to_string(devtool::ElementInspector::NodeId(dom.style_value));
  not_found_message["params"]["range"]["startLine"] = 99;
  not_found_message["params"]["range"]["startColumn"] = 0;
  not_found_message["params"]["range"]["endLine"] = 99;
  not_found_message["params"]["range"]["endColumn"] = 10;
  not_found_message["params"]["text"] = "(min-width: 100px)";
  element_executor_->SetMediaText(response_sender, not_found_message);

  ASSERT_EQ(response_sender->json_messages_.size(), 1U);
  EXPECT_EQ(response_sender->json_messages_[0].second["error"]["code"].asInt(),
            devtool::kServerError);
  EXPECT_EQ(
      response_sender->json_messages_[0].second["error"]["message"].asString(),
      "Media rule not found");
}

TEST_F(InspectorTasmExecutorTest,
       SetSupportsTextUpdatesRuleCacheAndSendsEvents) {
  auto dom = BuildSupportsTestDom(
      manager_.get(),
      {{".active-supports", tasm::CSSPropertyID::kPropertyIDWidth, "10px",
        "(display: flex)"}});
  element_executor_->element_root_ = dom.element.get();

  auto matched_styles =
      devtool::ElementInspector::GetMatchedStyleSheet(dom.element.get());
  ASSERT_EQ(matched_styles.size(), 1U);
  EXPECT_EQ(matched_styles[0].supports_text_, "(display: flex)");
  EXPECT_EQ(matched_styles[0].supports_range_.start_line_, 0);

  Json::Value matched =
      devtool::ElementHelper::GetMatchedStylesForNode(dom.element.get());
  ASSERT_TRUE(matched["matchedCSSRules"].isArray());
  ASSERT_EQ(matched["matchedCSSRules"].size(), 1U);
  const Json::Value& supports =
      matched["matchedCSSRules"][0]["rule"]["supports"];
  ASSERT_TRUE(supports.isArray());
  ASSERT_EQ(supports.size(), 1U);
  EXPECT_EQ(supports[0]["text"].asString(), "(display: flex)");
  EXPECT_TRUE(supports[0]["active"].asBool());

  auto event_sender = std::make_shared<RecordingMessageSender>();
  devtools_ng_->message_sender_ = event_sender;
  auto response_sender = std::make_shared<RecordingMessageSender>();

  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 106;
  message["params"]["styleSheetId"] =
      std::to_string(devtool::ElementInspector::NodeId(dom.style_value));
  message["params"]["range"]["startLine"] = 0;
  message["params"]["range"]["startColumn"] = 0;
  message["params"]["range"]["endLine"] = 0;
  message["params"]["range"]["endColumn"] =
      static_cast<int>(std::string("(display: flex)").length());
  message["params"]["text"] = "@supports (color: red)";

  element_executor_->SetSupportsText(response_sender, message);

  ASSERT_EQ(response_sender->json_messages_.size(), 1U);
  const Json::Value& response = response_sender->json_messages_[0].second;
  EXPECT_EQ(response["id"].asInt(), 106);
  EXPECT_TRUE(response["error"].isNull());
  EXPECT_EQ(response["result"]["supports"]["text"].asString(), "(color: red)");
  EXPECT_TRUE(response["result"]["supports"]["active"].asBool());
  EXPECT_EQ(FirstSupportsText(dom.fragment.get()), "(color: red)");

  auto updated_sheet = devtool::ElementInspector::GetStyleSheetByName(
      dom.element.get(), ".active-supports");
  ASSERT_FALSE(updated_sheet.empty);
  EXPECT_EQ(updated_sheet.supports_text_, "(color: red)");
  EXPECT_EQ(updated_sheet.supports_range_.start_line_, 0);
  EXPECT_EQ(updated_sheet.supports_range_.end_column_,
            static_cast<int>(std::string("(color: red)").length()));

  ASSERT_EQ(event_sender->json_messages_.size(), 1U);
  EXPECT_EQ(event_sender->json_messages_[0].second["method"].asString(),
            "CSS.styleSheetChanged");
  EXPECT_EQ(event_sender->json_messages_[0]
                .second["params"]["styleSheetId"]
                .asString(),
            std::to_string(devtool::ElementInspector::NodeId(dom.style_value)));
}

TEST_F(InspectorTasmExecutorTest,
       SetSupportsTextDeactivatesUnsupportedCondition) {
  auto dom = BuildSupportsTestDom(
      manager_.get(),
      {{".active-supports", tasm::CSSPropertyID::kPropertyIDWidth, "10px",
        "(display: flex)"}});
  element_executor_->element_root_ = dom.element.get();

  auto initial_styles =
      devtool::ElementInspector::GetMatchedStyleSheet(dom.element.get());
  ASSERT_EQ(initial_styles.size(), 1U);

  auto response_sender = std::make_shared<RecordingMessageSender>();
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 107;
  message["params"]["styleSheetId"] =
      std::to_string(devtool::ElementInspector::NodeId(dom.style_value));
  message["params"]["range"]["startLine"] = 0;
  message["params"]["range"]["startColumn"] = 0;
  message["params"]["range"]["endLine"] = 0;
  message["params"]["range"]["endColumn"] =
      static_cast<int>(std::string("(display: flex)").length());
  message["params"]["text"] = "(unknown-property: value)";

  element_executor_->SetSupportsText(response_sender, message);

  ASSERT_EQ(response_sender->json_messages_.size(), 1U);
  const Json::Value& response = response_sender->json_messages_[0].second;
  EXPECT_FALSE(response["result"]["supports"]["active"].asBool());
  EXPECT_EQ(response["result"]["supports"]["text"].asString(),
            "(unknown-property: value)");
  EXPECT_EQ(FirstSupportsText(dom.fragment.get()), "(unknown-property: value)");

  auto& style_sheet_map =
      devtool::ElementInspector::GetStyleSheetMap(dom.style_value);
  auto cached_styles = style_sheet_map.equal_range(".active-supports");
  ASSERT_NE(cached_styles.first, cached_styles.second);
  EXPECT_EQ(cached_styles.first->second.supports_text_,
            "(unknown-property: value)");
  EXPECT_TRUE(devtool::ElementInspector::GetMatchedStyleSheet(dom.element.get())
                  .empty());
}

TEST_F(InspectorTasmExecutorTest, SetSupportsTextReportsErrors) {
  auto dom = BuildSupportsTestDom(
      manager_.get(),
      {{".active-supports", tasm::CSSPropertyID::kPropertyIDWidth, "10px",
        "(display: flex)"}});
  element_executor_->element_root_ = dom.element.get();

  auto response_sender = std::make_shared<RecordingMessageSender>();
  Json::Value invalid_location_message(Json::ValueType::objectValue);
  invalid_location_message["id"] = 107;
  invalid_location_message["params"]["styleSheetId"] = "";
  invalid_location_message["params"]["text"] = "(color: red)";
  element_executor_->SetSupportsText(response_sender, invalid_location_message);

  ASSERT_EQ(response_sender->json_messages_.size(), 1U);
  EXPECT_EQ(response_sender->json_messages_[0].second["error"]["code"].asInt(),
            devtool::kInvalidParams);

  response_sender->json_messages_.clear();
  Json::Value invalid_condition_message(Json::ValueType::objectValue);
  invalid_condition_message["id"] = 108;
  invalid_condition_message["params"]["styleSheetId"] =
      std::to_string(devtool::ElementInspector::NodeId(dom.style_value));
  invalid_condition_message["params"]["range"]["startLine"] = 0;
  invalid_condition_message["params"]["range"]["startColumn"] = 0;
  invalid_condition_message["params"]["range"]["endLine"] = 0;
  invalid_condition_message["params"]["range"]["endColumn"] = 15;
  invalid_condition_message["params"]["text"] = "not not (display: flex)";
  element_executor_->SetSupportsText(response_sender,
                                     invalid_condition_message);

  ASSERT_EQ(response_sender->json_messages_.size(), 1U);
  EXPECT_EQ(response_sender->json_messages_[0].second["error"]["code"].asInt(),
            devtool::kInvalidParams);
  EXPECT_EQ(
      response_sender->json_messages_[0].second["error"]["message"].asString(),
      "Invalid supports condition");

  response_sender->json_messages_.clear();
  Json::Value not_found_message(Json::ValueType::objectValue);
  not_found_message["id"] = 109;
  not_found_message["params"]["styleSheetId"] =
      std::to_string(devtool::ElementInspector::NodeId(dom.style_value));
  not_found_message["params"]["range"]["startLine"] = 99;
  not_found_message["params"]["range"]["startColumn"] = 0;
  not_found_message["params"]["range"]["endLine"] = 99;
  not_found_message["params"]["range"]["endColumn"] = 10;
  not_found_message["params"]["text"] = "(color: red)";
  element_executor_->SetSupportsText(response_sender, not_found_message);

  ASSERT_EQ(response_sender->json_messages_.size(), 1U);
  EXPECT_EQ(response_sender->json_messages_[0].second["error"]["code"].asInt(),
            devtool::kServerError);
  EXPECT_EQ(
      response_sender->json_messages_[0].second["error"]["message"].asString(),
      "Supports rule not found");
}

TEST_F(InspectorTasmExecutorTest, GetComputedStyleOfNodeStyleOrderCase) {
  auto comp = std::shared_ptr<lynx::tasm::RadonComponent>(
      new lynx::tasm::RadonComponent(nullptr, 0, nullptr, nullptr, 0, 0, 0));
  auto element = manager_->CreateFiberElement("view");
  element->SetAttributeHolder(comp.get()->attribute_holder());
  element->MarkAttached();
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(element.get()));
  element_executor_->element_root_ = element.get();

  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 100;
  message["params"]["nodeId"] =
      devtool::ElementInspector::NodeId(element.get());
  element_executor_->GetComputedStyleForNode(message_sender_, message);
  FlushUITasks();

  Json::Value response;
  Json::Reader reader;
  ASSERT_TRUE(reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, response));

  const Json::Value& computed_style = response["result"]["computedStyle"];
  ASSERT_TRUE(computed_style.isArray());

  auto find_style_index = [&computed_style](const std::string& name) {
    for (Json::ArrayIndex i = 0; i < computed_style.size(); ++i) {
      if (computed_style[i]["name"].asString() == name) {
        return static_cast<int>(i);
      }
    }
    return -1;
  };

  int caret_gradient = find_style_index("-x-caret-gradient");
  int caret_width = find_style_index("-x-caret-width");
  int caret_height = find_style_index("-x-caret-height");
  int caret_radius = find_style_index("-x-caret-radius");

  ASSERT_GE(caret_gradient, 0);
  ASSERT_GE(caret_width, 0);
  ASSERT_GE(caret_height, 0);
  ASSERT_GE(caret_radius, 0);
  EXPECT_LT(caret_gradient, caret_width);
  EXPECT_LT(caret_width, caret_height);
  EXPECT_LT(caret_height, caret_radius);
}

TEST_F(InspectorTasmExecutorTest,
       GetComputedStyleForNodeUsesBoxModelBorderShorthandCase) {
  auto facade = std::make_shared<testing::DevToolPlatformFacadeMock>();
  facade->box_model_response_ = BuildBoxModelWithBorder(3, 3, 5, 8);
  devtool_mediator_->ui_executor_ =
      std::make_shared<devtool::InspectorUIExecutor>(devtool_mediator_);
  devtool_mediator_->ui_executor_->SetDevToolPlatformFacade(facade);

  auto element = manager_->CreateFiberElement("view");
  element->MarkCanBeLayoutOnly(false);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(element.get()));
  element->CreateElementContainer(false);
  element->MarkAttached();
  element_executor_->element_root_ = element.get();

  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 13;
  message["params"]["nodeId"] =
      devtool::ElementInspector::NodeId(element.get());
  element_executor_->GetComputedStyleForNode(message_sender_, message);
  FlushUITasks();

  Json::Value res;
  Json::Reader reader;
  ASSERT_TRUE(reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, res));
  const Json::Value& computed_style = res["result"]["computedStyle"];
  ASSERT_TRUE(computed_style.isArray());
  int border_index = -1;
  for (Json::ArrayIndex i = 0; i < computed_style.size(); ++i) {
    if (computed_style[i]["name"].asString() == "border") {
      border_index = static_cast<int>(i);
      break;
    }
  }
  ASSERT_GE(border_index, 0);
  EXPECT_EQ(computed_style[static_cast<Json::ArrayIndex>(border_index)]["value"]
                .asString(),
            "3px 5px 8px 3px");
}

TEST_F(InspectorTasmExecutorTest,
       GetComputedStyleForNodeUsesBoxModelEqualBorderShorthandCase) {
  auto facade = std::make_shared<testing::DevToolPlatformFacadeMock>();
  facade->box_model_response_ = BuildBoxModelWithBorder(4, 4, 4, 4);
  devtool_mediator_->ui_executor_ =
      std::make_shared<devtool::InspectorUIExecutor>(devtool_mediator_);
  devtool_mediator_->ui_executor_->SetDevToolPlatformFacade(facade);

  auto element = manager_->CreateFiberElement("view");
  element->MarkCanBeLayoutOnly(false);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(element.get()));
  element->CreateElementContainer(false);
  element->MarkAttached();
  element_executor_->element_root_ = element.get();

  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 14;
  message["params"]["nodeId"] =
      devtool::ElementInspector::NodeId(element.get());
  element_executor_->GetComputedStyleForNode(message_sender_, message);
  FlushUITasks();

  Json::Value res;
  Json::Reader reader;
  ASSERT_TRUE(reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, res));
  const Json::Value& computed_style = res["result"]["computedStyle"];
  ASSERT_TRUE(computed_style.isArray());
  int border_index = -1;
  for (Json::ArrayIndex i = 0; i < computed_style.size(); ++i) {
    if (computed_style[i]["name"].asString() == "border") {
      border_index = static_cast<int>(i);
      break;
    }
  }
  ASSERT_GE(border_index, 0);
  EXPECT_EQ(computed_style[static_cast<Json::ArrayIndex>(border_index)]["value"]
                .asString(),
            "4px");
}

TEST_F(InspectorTasmExecutorTest, SendLayerTreeDidChangeEventCase) {
  element_executor_->layer_tree_enabled_ = true;

  auto element = manager_->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(element.get()));
  element->CreateElementContainer(false);
  auto element_container = element->element_container_impl();

  auto child = manager_->CreateFiberElement("view");
  child->MarkCanBeLayoutOnly(true);
  child->computed_css_style()->SetOverflowDefaultVisible(true);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(child.get()));
  element->AddChildAt(child, 0);
  EXPECT_EQ(child->parent(), element.get());

  child->CreateElementContainer(false);
  auto child_container = child->element_container_impl();
  child_container->InsertSelf();
  EXPECT_EQ(child_container->parent(), element_container);
  EXPECT_EQ(element_container->children().size(), static_cast<size_t>(1));
  element_executor_->element_root_ = element.get();

  element_executor_->SendLayerTreeDidChangeEvent();

  std::string expected_str = R"({
  "method": "LayerTree.layerTreeDidChange",
  "params": {
    "layers": [
      {
        "backendNodeId": 10,
        "drawsContent": true,
        "height": null,
        "invisible": true,
        "layerId": "10",
        "name": "view",
        "offsetX": null,
        "offsetY": null,
        "paintCount": 1,
        "width": null
      },
      {
        "backendNodeId": 11,
        "drawsContent": true,
        "height": null,
        "invisible": true,
        "layerId": "11",
        "name": "view",
        "offsetX": null,
        "offsetY": null,
        "paintCount": 1,
        "parentLayerId": "10",
        "width": null
      }
    ]
  }
})";

  Json::Reader reader;
  Json::Value expected_json;
  bool success = reader.parse(expected_str, expected_json);
  EXPECT_TRUE(success);
  Json::Value res;
  reader.parse(devtool::MockReceiver::GetInstance().received_message_.second,
               res);
  EXPECT_EQ(expected_json, res);
}

TEST_F(InspectorTasmExecutorTest, GetLayerContentFromElementCase) {
  LOGI("InspectorTasmExecutorTest GetLayerContentFromElementCase start");
  auto element = manager_->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(std::make_tuple(element));
  auto res = element_executor_->GetLayerContentFromElement(element.get());
  Json::Value layer(Json::ValueType::objectValue);

  layer["layerId"] =
      std::to_string(devtool::ElementInspector::NodeId(element.get()));
  layer["backendNodeId"] = devtool::ElementInspector::NodeId(element.get());
  layer["paintCount"] = 1;
  layer["drawsContent"] = true;
  layer["invisible"] = true;
  layer["name"] = "view";
  Json::Value layout(Json::ValueType::objectValue);
  layer["offsetX"] = layout["offsetX"];
  layer["offsetY"] = layout["offsetY"];
  layer["width"] = layout["width"];
  layer["height"] = layout["height"];

  EXPECT_EQ(layer, res);
}

TEST_F(InspectorTasmExecutorTest, BuildBoxModelQueryWithLayoutOnlyNodeCase) {
  auto root = manager_->CreateFiberElement("view");
  root->MarkCanBeLayoutOnly(false);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(root.get()));
  root->CreateElementContainer(false);
  root->MarkAttached();

  auto child = manager_->CreateFiberElement("view");
  child->MarkCanBeLayoutOnly(true);
  child->computed_css_style()->SetOverflowDefaultVisible(true);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(child.get()));
  root->AddChildAt(child, 0);
  child->CreateElementContainer(false);
  child->MarkAttached();

  devtool::InspectorBoxModelQuery query;
  ASSERT_TRUE(element_executor_->BuildBoxModelQuery(child.get(), query));
  EXPECT_FALSE(query.has_ui_primitive);
  EXPECT_EQ(query.layout_object.id, child->impl_id());
  EXPECT_EQ(query.transform_node.id, root->impl_id());
  ASSERT_EQ(query.layout_only_nodes.size(), 1U);
  EXPECT_EQ(query.layout_only_nodes[0].id, child->impl_id());
  EXPECT_FALSE(query.is_overlay);
}

TEST_F(InspectorTasmExecutorTest,
       GetDocumentBodyWithBoxModelStoresPathsOutsideJsonCase) {
  auto root = manager_->CreateFiberElement("view");
  root->MarkCanBeLayoutOnly(false);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(root.get()));
  root->CreateElementContainer(false);
  root->MarkAttached();

  auto child = manager_->CreateFiberElement("view");
  child->MarkCanBeLayoutOnly(false);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(child.get()));
  root->AddChildAt(child, 0);
  child->CreateElementContainer(false);
  child->MarkAttached();

  std::vector<devtool::InspectorBoxModelQuery> queries;
  std::vector<std::vector<Json::ArrayIndex>> box_model_paths;
  Json::Value document = element_executor_->GetDocumentBodyFromNodeWithBoxModel(
      root.get(), queries, box_model_paths, {});

  ASSERT_EQ(queries.size(), 2U);
  ASSERT_EQ(box_model_paths.size(), 2U);
  EXPECT_TRUE(box_model_paths[0].empty());
  ASSERT_EQ(box_model_paths[1].size(), 1U);
  EXPECT_EQ(box_model_paths[1][0], 0U);
  EXPECT_TRUE(document["box_model"].isNull());
  ASSERT_TRUE(document["children"].isArray());
  ASSERT_EQ(document["children"].size(), 1U);
  EXPECT_TRUE(document["children"][0]["box_model"].isNull());
  EXPECT_FALSE(document.isMember("box_model_index"));
  EXPECT_FALSE(document["children"][0].isMember("box_model_index"));
}

TEST_F(InspectorTasmExecutorTest, DOMGetBoxModelRequestsBoxModelOnUICase) {
  auto facade = std::make_shared<testing::DevToolPlatformFacadeMock>();
  facade->box_model_response_ = BuildBoxModel(20);
  devtool_mediator_->ui_executor_ =
      std::make_shared<devtool::InspectorUIExecutor>(devtool_mediator_);
  devtool_mediator_->ui_executor_->SetDevToolPlatformFacade(facade);

  auto element = manager_->CreateFiberElement("view");
  element->MarkCanBeLayoutOnly(false);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(element.get()));
  element->CreateElementContainer(false);
  element->MarkAttached();
  element_executor_->element_root_ = element.get();
  std::string screen_shot_mode =
      devtool::DevToolStatus::GetInstance().GetStatus(
          devtool::DevToolStatus::kDevToolStatusKeyScreenShotMode);
  devtool::DevToolStatus::GetInstance().SetStatus(
      devtool::DevToolStatus::kDevToolStatusKeyScreenShotMode,
      devtool::DevToolStatus::SCREENSHOT_MODE_FULLSCREEN);

  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 9;
  message["params"]["nodeId"] =
      devtool::ElementInspector::NodeId(element.get());
  element_executor_->DOM_GetBoxModel(message_sender_, message);
  FlushUITasks();
  devtool::DevToolStatus::GetInstance().SetStatus(
      devtool::DevToolStatus::kDevToolStatusKeyScreenShotMode,
      screen_shot_mode);

  Json::Value res;
  Json::Reader reader;
  ASSERT_TRUE(reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, res));
  EXPECT_EQ(res["id"], 9);
  EXPECT_TRUE(res["error"].isNull());
  ASSERT_TRUE(res["result"]["model"].isObject());
  EXPECT_DOUBLE_EQ(res["result"]["model"]["width"].asDouble(),
                   facade->box_model_response_[0]);
  EXPECT_DOUBLE_EQ(res["result"]["model"]["height"].asDouble(),
                   facade->box_model_response_[1]);
  ASSERT_EQ(facade->box_model_queries_.size(), 1U);
  EXPECT_EQ(facade->box_model_queries_[0].layout_object.id, element->impl_id());
}

TEST_F(InspectorTasmExecutorTest, DOMGetBoxModelAppliesRootOffsetCase) {
  auto facade = std::make_shared<testing::DevToolPlatformFacadeMock>();
  facade->box_model_response_ = BuildBoxModel(20);
  devtool_mediator_->ui_executor_ =
      std::make_shared<devtool::InspectorUIExecutor>(devtool_mediator_);
  devtool_mediator_->ui_executor_->SetDevToolPlatformFacade(facade);

  auto root = manager_->CreateFiberElement("view");
  root->MarkCanBeLayoutOnly(false);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(root.get()));
  root->CreateElementContainer(false);
  root->MarkAttached();

  auto child = manager_->CreateFiberElement("view");
  child->MarkCanBeLayoutOnly(false);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(child.get()));
  root->AddChildAt(child, 0);
  child->CreateElementContainer(false);
  child->MarkAttached();
  element_executor_->element_root_ = root.get();

  std::string screen_shot_mode =
      devtool::DevToolStatus::GetInstance().GetStatus(
          devtool::DevToolStatus::kDevToolStatusKeyScreenShotMode);
  devtool::DevToolStatus::GetInstance().SetStatus(
      devtool::DevToolStatus::kDevToolStatusKeyScreenShotMode,
      devtool::DevToolStatus::SCREENSHOT_MODE_LYNXVIEW);

  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 10;
  message["params"]["nodeId"] = devtool::ElementInspector::NodeId(child.get());
  element_executor_->DOM_GetBoxModel(message_sender_, message);
  FlushUITasks();
  devtool::DevToolStatus::GetInstance().SetStatus(
      devtool::DevToolStatus::kDevToolStatusKeyScreenShotMode,
      screen_shot_mode);

  Json::Value res;
  Json::Reader reader;
  ASSERT_TRUE(reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, res));
  EXPECT_EQ(res["id"], 10);
  ASSERT_TRUE(res["result"]["model"].isObject());
  EXPECT_DOUBLE_EQ(res["result"]["model"]["width"].asDouble(),
                   facade->box_model_response_[0]);
  EXPECT_DOUBLE_EQ(
      res["result"]["model"]["content"][0U].asDouble(),
      facade->box_model_response_[2] - facade->box_model_response_[18]);
  ASSERT_EQ(facade->box_model_queries_.size(), 2U);
  EXPECT_EQ(facade->box_model_queries_[0].layout_object.id, child->impl_id());
  EXPECT_EQ(facade->box_model_queries_[1].layout_object.id, root->impl_id());
}

TEST_F(InspectorTasmExecutorTest,
       DOMGetBoxModelReturnsErrorForMissingNodeCase) {
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 11;
  message["params"]["nodeId"] = 99999;
  element_executor_->DOM_GetBoxModel(message_sender_, message);
  FlushDevtoolTasks();

  Json::Value res;
  Json::Reader reader;
  ASSERT_TRUE(reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, res));
  EXPECT_EQ(res["id"], 11);
  EXPECT_EQ(res["result"]["error"]["code"], -32000);
  EXPECT_EQ(res["result"]["error"]["message"].asString(),
            "Could not compute box model.");
}

TEST_F(InspectorTasmExecutorTest, GetComputedStyleForNodeUsesBoxModelCase) {
  auto facade = std::make_shared<testing::DevToolPlatformFacadeMock>();
  facade->box_model_response_ = BuildBoxModel(30);
  devtool_mediator_->ui_executor_ =
      std::make_shared<devtool::InspectorUIExecutor>(devtool_mediator_);
  devtool_mediator_->ui_executor_->SetDevToolPlatformFacade(facade);

  auto element = manager_->CreateFiberElement("view");
  element->MarkCanBeLayoutOnly(false);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(element.get()));
  element->CreateElementContainer(false);
  element->MarkAttached();
  element_executor_->element_root_ = element.get();

  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 12;
  message["params"]["nodeId"] =
      devtool::ElementInspector::NodeId(element.get());
  element_executor_->GetComputedStyleForNode(message_sender_, message);
  FlushUITasks();

  Json::Value res;
  Json::Reader reader;
  ASSERT_TRUE(reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, res));
  EXPECT_EQ(res["id"], 12);
  ASSERT_TRUE(res["result"]["computedStyle"].isArray());
  EXPECT_GT(res["result"]["computedStyle"].size(), 0U);
  ASSERT_EQ(facade->box_model_queries_.size(), 1U);
  EXPECT_EQ(facade->box_model_queries_[0].layout_object.id, element->impl_id());
}

TEST_F(InspectorTasmExecutorTest, SendLayerTreeDidChangeAppliesBoxModelsCase) {
  auto facade = std::make_shared<testing::DevToolPlatformFacadeMock>();
  facade->box_model_response_ = BuildBoxModel(40);
  devtool_mediator_->ui_executor_ =
      std::make_shared<devtool::InspectorUIExecutor>(devtool_mediator_);
  devtool_mediator_->ui_executor_->SetDevToolPlatformFacade(facade);

  auto root = manager_->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(root.get()));
  auto child = manager_->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(child.get()));
  root->AddChildAt(child, 0);
  element_executor_->element_root_ = root.get();
  element_executor_->layer_tree_enabled_ = true;

  element_executor_->SendLayerTreeDidChangeEvent();
  FlushUITasks();

  Json::Value res;
  Json::Reader reader;
  ASSERT_TRUE(reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, res));
  EXPECT_EQ(res["method"], "LayerTree.layerTreeDidChange");
  ASSERT_TRUE(res["params"]["layers"].isArray());
  ASSERT_EQ(res["params"]["layers"].size(), 2U);
  EXPECT_DOUBLE_EQ(res["params"]["layers"][0U]["width"].asDouble(), 2.0);
  EXPECT_DOUBLE_EQ(res["params"]["layers"][0U]["offsetX"].asDouble(),
                   facade->box_model_response_[26]);
  EXPECT_DOUBLE_EQ(res["params"]["layers"][1U]["offsetX"].asDouble(), 0.0);
}

TEST_F(InspectorTasmExecutorTest, SendLayerPaintedAppliesBoxModelClipCase) {
  auto facade = std::make_shared<testing::DevToolPlatformFacadeMock>();
  facade->box_model_response_ = BuildBoxModel(50);
  devtool_mediator_->ui_executor_ =
      std::make_shared<devtool::InspectorUIExecutor>(devtool_mediator_);
  devtool_mediator_->ui_executor_->SetDevToolPlatformFacade(facade);

  auto root = manager_->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(root.get()));
  element_executor_->element_root_ = root.get();

  element_executor_->SendLayerPaintedEvent();
  FlushUITasks();

  Json::Value res;
  Json::Reader reader;
  ASSERT_TRUE(reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, res));
  EXPECT_EQ(res["method"], "LayerTree.layerPainted");
  EXPECT_DOUBLE_EQ(res["params"]["clip"]["x"].asDouble(),
                   facade->box_model_response_[26]);
  EXPECT_DOUBLE_EQ(res["params"]["clip"]["width"].asDouble(), 2.0);
}

TEST_F(InspectorTasmExecutorTest, GetDocumentWithDepthCase) {
  auto root = manager_->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(root.get()));

  auto child = manager_->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(child.get()));
  auto grandchild = manager_->CreateFiberElement("text");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(grandchild.get()));

  root->AddChildAt(child, 0);
  child->AddChildAt(grandchild, 0);
  element_executor_->element_root_ = root.get();

  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 7;
  message["params"]["depth"] = 1;
  element_executor_->GetDocument(message_sender_, message);
  FlushDevtoolTasks();

  Json::Value res;
  Json::Reader reader;
  ASSERT_TRUE(reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, res));
  EXPECT_EQ(res["id"], 7);
  EXPECT_TRUE(res["error"].isNull());
  EXPECT_EQ(res["result"]["root"]["nodeId"],
            devtool::ElementInspector::NodeId(root.get()));
  ASSERT_TRUE(res["result"]["root"]["children"].isArray());
  ASSERT_EQ(res["result"]["root"]["children"].size(), 1U);
  EXPECT_EQ(res["result"]["root"]["children"][0]["nodeId"],
            devtool::ElementInspector::NodeId(child.get()));
  EXPECT_TRUE(res["result"]["root"]["children"][0]["children"].isNull());
}

TEST_F(InspectorTasmExecutorTest, GetDocumentDefaultDepthReturnsFullTreeCase) {
  auto root = manager_->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(root.get()));

  auto child = manager_->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(child.get()));
  auto grandchild = manager_->CreateFiberElement("text");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(grandchild.get()));

  root->AddChildAt(child, 0);
  child->AddChildAt(grandchild, 0);
  element_executor_->element_root_ = root.get();

  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 8;
  element_executor_->GetDocument(message_sender_, message);
  FlushDevtoolTasks();

  Json::Value res;
  Json::Reader reader;
  ASSERT_TRUE(reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, res));
  EXPECT_EQ(res["id"], 8);
  ASSERT_TRUE(res["result"]["root"]["children"].isArray());
  ASSERT_EQ(res["result"]["root"]["children"].size(), 1U);
  ASSERT_TRUE(res["result"]["root"]["children"][0]["children"].isArray());
  ASSERT_EQ(res["result"]["root"]["children"][0]["children"].size(), 1U);
  EXPECT_EQ(res["result"]["root"]["children"][0]["children"][0]["nodeId"],
            devtool::ElementInspector::NodeId(grandchild.get()));
}

TEST_F(InspectorTasmExecutorTest, DescribeNodeByNodeIdCase) {
  auto root = manager_->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(root.get()));
  root->SetAttribute("id", lepus::Value("root"));

  auto child = manager_->CreateFiberElement("text");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(child.get()));
  child->SetAttribute("text", lepus::Value("hello"));
  root->AddChildAt(child, 0);
  element_executor_->element_root_ = root.get();

  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 1;
  message["params"]["nodeId"] = devtool::ElementInspector::NodeId(root.get());
  element_executor_->DescribeNode(message_sender_, message);
  FlushDevtoolTasks();

  Json::Value res;
  Json::Reader reader;
  ASSERT_TRUE(reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, res));
  EXPECT_EQ(res["id"], 1);
  EXPECT_TRUE(res["error"].isNull());
  EXPECT_EQ(res["result"]["node"]["nodeId"],
            devtool::ElementInspector::NodeId(root.get()));
  EXPECT_EQ(res["result"]["node"]["backendNodeId"],
            devtool::ElementInspector::NodeId(root.get()));
  EXPECT_EQ(res["result"]["node"]["childNodeCount"], 1);
  ASSERT_TRUE(res["result"]["node"]["children"].isArray());
  ASSERT_EQ(res["result"]["node"]["children"].size(), 1U);
  EXPECT_EQ(res["result"]["node"]["children"][0]["nodeId"],
            devtool::ElementInspector::NodeId(child.get()));
}

TEST_F(InspectorTasmExecutorTest, DescribeNodeDepthZeroCase) {
  auto root = manager_->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(root.get()));

  auto child = manager_->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(child.get()));
  root->AddChildAt(child, 0);
  element_executor_->element_root_ = root.get();

  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 2;
  message["params"]["nodeId"] = devtool::ElementInspector::NodeId(root.get());
  message["params"]["depth"] = 0;
  element_executor_->DescribeNode(message_sender_, message);
  FlushDevtoolTasks();

  Json::Value res;
  Json::Reader reader;
  ASSERT_TRUE(reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, res));
  EXPECT_EQ(res["result"]["node"]["childNodeCount"], 1);
  EXPECT_TRUE(res["result"]["node"]["children"].isNull());
}

TEST_F(InspectorTasmExecutorTest, DescribeNodeWithFullDepthCase) {
  auto root = manager_->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(root.get()));

  auto child = manager_->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(child.get()));
  auto grandchild = manager_->CreateFiberElement("text");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(grandchild.get()));

  root->AddChildAt(child, 0);
  child->AddChildAt(grandchild, 0);
  element_executor_->element_root_ = root.get();

  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 3;
  message["params"]["backendNodeId"] =
      devtool::ElementInspector::NodeId(child.get());
  message["params"]["depth"] = -1;
  element_executor_->DescribeNode(message_sender_, message);
  FlushDevtoolTasks();

  Json::Value res;
  Json::Reader reader;
  ASSERT_TRUE(reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, res));
  EXPECT_EQ(res["result"]["node"]["nodeId"],
            devtool::ElementInspector::NodeId(child.get()));
  ASSERT_TRUE(res["result"]["node"]["children"].isArray());
  ASSERT_EQ(res["result"]["node"]["children"].size(), 1U);
  EXPECT_EQ(res["result"]["node"]["children"][0]["nodeId"],
            devtool::ElementInspector::NodeId(grandchild.get()));
}

TEST_F(InspectorTasmExecutorTest, DescribeNodeMissingNodeCase) {
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 4;
  message["params"]["nodeId"] = 99999;
  element_executor_->DescribeNode(message_sender_, message);

  Json::Value res;
  Json::Reader reader;
  ASSERT_TRUE(reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, res));
  EXPECT_EQ(res["id"], 4);
  EXPECT_TRUE(res["error"].isNull());
  EXPECT_TRUE(res["result"]["node"].isNull());
}

TEST_F(InspectorTasmExecutorTest, DescribeNodeUnsupportedObjectIdCase) {
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 5;
  message["params"]["objectId"] = "remote-object-id";
  element_executor_->DescribeNode(message_sender_, message);

  Json::Value res;
  Json::Reader reader;
  ASSERT_TRUE(reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, res));
  EXPECT_EQ(res["id"], 5);
  EXPECT_TRUE(res["error"].isNull());
  EXPECT_TRUE(res["result"]["node"].isNull());
}
TEST_F(InspectorTasmExecutorTest, SendDOMEventMsgCase) {
  auto element = manager_->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(element.get()));
  element->CreateElementContainer(false);
  int node_id = devtool::ElementInspector::NodeId(element.get());
  element_executor_->element_root_ = element.get();
  devtool_mediator_->default_task_runner_ = ui_thread_->GetTaskRunner();

  element_executor_->SendDOMEventMsg(
      devtool::InspectorTasmExecutor::DomCdpEvent::ATTRIBUTE_MODIFIED, node_id,
      "style", -1);
  FlushDevtoolTasks();

  Json::Value res;
  Json::Reader reader;
  bool is_valid = reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, res);
  EXPECT_TRUE(is_valid);
  EXPECT_EQ(res["method"], "DOM.attributeModified");
  EXPECT_EQ(res["params"]["nodeId"], node_id);
  EXPECT_EQ(res["params"]["name"], "style");
  EXPECT_TRUE(res["params"]["value"].isString());

  std::string prev =
      devtool::MockReceiver::GetInstance().received_message_.second;
  element_executor_->SendDOMEventMsg(
      devtool::InspectorTasmExecutor::DomCdpEvent::ATTRIBUTE_MODIFIED, -1,
      "style", -1);
  FlushDevtoolTasks();
  EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
            prev);

  element_executor_->SendDOMEventMsg(
      devtool::InspectorTasmExecutor::DomCdpEvent::CHILD_NODE_REMOVED, node_id,
      "", node_id);
  FlushDevtoolTasks();
  is_valid = reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, res);
  EXPECT_TRUE(is_valid);
  EXPECT_EQ(res["method"], "DOM.childNodeRemoved");
  EXPECT_EQ(res["params"]["nodeId"], node_id);
  EXPECT_EQ(res["params"]["parentNodeId"], node_id);

  element_executor_->SendDOMEventMsg(
      devtool::InspectorTasmExecutor::DomCdpEvent::DOCUMENT_UPDATED, -1, "",
      -1);
  FlushDevtoolTasks();
  is_valid = reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, res);
  EXPECT_TRUE(is_valid);
  EXPECT_EQ(res["method"], "DOM.documentUpdated");
  EXPECT_TRUE(res["params"].isObject());
}

TEST_F(InspectorTasmExecutorTest, SearchProtocolUsesStringSearchIdCase) {
  auto element = manager_->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(element.get()));
  element_executor_->element_root_ = element.get();

  Json::Value perform_search_message(Json::ValueType::objectValue);
  perform_search_message["id"] = 1;
  perform_search_message["params"]["query"] = "view";
  element_executor_->PerformSearch(message_sender_, perform_search_message);

  Json::Reader reader;
  Json::Value perform_search_response;
  bool is_valid = reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second,
      perform_search_response);
  EXPECT_TRUE(is_valid);
  EXPECT_TRUE(perform_search_response["result"]["searchId"].isString());
  std::string search_id =
      perform_search_response["result"]["searchId"].asString();
  EXPECT_FALSE(search_id.empty());

  Json::Value get_search_results_message(Json::ValueType::objectValue);
  get_search_results_message["id"] = 2;
  get_search_results_message["params"]["searchId"] = search_id;
  get_search_results_message["params"]["fromIndex"] = 0;
  get_search_results_message["params"]["toIndex"] = 1;
  element_executor_->GetSearchResults(message_sender_,
                                      get_search_results_message);

  Json::Value get_search_results_response;
  is_valid = reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second,
      get_search_results_response);
  EXPECT_TRUE(is_valid);
  EXPECT_TRUE(get_search_results_response.isMember("result"));
  EXPECT_TRUE(get_search_results_response["result"]["nodeIds"].isArray());

  Json::Value discard_search_results_message(Json::ValueType::objectValue);
  discard_search_results_message["id"] = 3;
  discard_search_results_message["params"]["searchId"] = search_id;
  element_executor_->DiscardSearchResults(message_sender_,
                                          discard_search_results_message);

  Json::Value discard_search_results_response;
  is_valid = reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second,
      discard_search_results_response);
  EXPECT_TRUE(is_valid);
  EXPECT_TRUE(discard_search_results_response.isMember("result"));

  get_search_results_message["id"] = 4;
  element_executor_->GetSearchResults(message_sender_,
                                      get_search_results_message);
  Json::Value get_after_discard_response;
  is_valid = reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second,
      get_after_discard_response);
  EXPECT_TRUE(is_valid);
  EXPECT_TRUE(get_after_discard_response.isMember("error"));
  EXPECT_EQ(get_after_discard_response["error"]["code"], 32000);
}

}  // namespace testing
}  // namespace lynx
