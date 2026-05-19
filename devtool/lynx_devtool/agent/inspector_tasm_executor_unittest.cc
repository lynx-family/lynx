// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "devtool/lynx_devtool/agent/inspector_tasm_executor.h"

#include <sys/wait.h>

#include <cstddef>
#include <future>
#include <memory>
#include <string>

#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "devtool/base_devtool/native/test/message_sender_mock.h"
#include "devtool/base_devtool/native/test/mock_receiver.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
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
  }

  void FlushDevtoolTasks() {
    std::promise<void> p;
    auto f = p.get_future();
    devtool_mediator_->RunOnDevToolThread([&p]() { p.set_value(); }, true);
    f.wait();
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

TEST_F(InspectorTasmExecutorTest, GetComputedStyleOfNodeStyleOrderCase) {
  auto comp = std::shared_ptr<lynx::tasm::RadonComponent>(
      new lynx::tasm::RadonComponent(nullptr, 0, nullptr, nullptr, 0, 0, 0));
  auto element = manager_->CreateFiberElement("view");
  element->SetAttributeHolder(comp.get()->attribute_holder());
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(element.get()));

  Json::Value computed_style =
      element_executor_->GetComputedStyleOfNode(element.get());
  ASSERT_TRUE(computed_style.isArray());

  auto find_style_index = [&computed_style](const std::string& name) {
    for (Json::ArrayIndex i = 0; i < computed_style.size(); ++i) {
      if (computed_style[i]["name"].asString() == name) {
        return static_cast<int>(i);
      }
    }
    return -1;
  };

  int caret_gradient = find_style_index("caret-gradient");
  int caret_width = find_style_index("caret-width");
  int caret_height = find_style_index("caret-height");
  int caret_radius = find_style_index("caret-radius");

  ASSERT_GE(caret_gradient, 0);
  ASSERT_GE(caret_width, 0);
  ASSERT_GE(caret_height, 0);
  ASSERT_GE(caret_radius, 0);
  EXPECT_LT(caret_gradient, caret_width);
  EXPECT_LT(caret_width, caret_height);
  EXPECT_LT(caret_height, caret_radius);
}

TEST_F(InspectorTasmExecutorTest, SendLayerTreeDidChangeEventCase) {
  element_executor_->layer_tree_enabled_ = true;

  auto element = manager_->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(element.get()));
  element->CreateElementContainer(false);
  auto element_container = element->element_container_impl();

  auto child = manager_->CreateFiberElement("view");
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

TEST_F(InspectorTasmExecutorTest, BuildLayerTreeFromElement) {
  auto element = manager_->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(element.get()));
  element->CreateElementContainer(false);
  auto element_container = element->element_container_impl();

  auto child = manager_->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(child.get()));
  element->AddChildAt(child, 0);
  EXPECT_EQ(child->parent(), element.get());

  child->CreateElementContainer(false);
  auto child_container = child->element_container_impl();
  child_container->InsertSelf();
  EXPECT_EQ(child_container->parent(), element_container);
  EXPECT_EQ(element_container->children().size(), static_cast<size_t>(1));

  auto res = element_executor_->BuildLayerTreeFromElement(element.get());
  std::string layer_str = R"([
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
])";
  Json::Reader reader;
  Json::Value layer;
  reader.parse(layer_str, layer);
  EXPECT_EQ(res, layer);
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
