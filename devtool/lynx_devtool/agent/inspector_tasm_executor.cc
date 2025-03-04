// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/inspector_tasm_executor.h"

#include "base/include/log/logging.h"
#include "core/renderer/css/css_decoder.h"
#include "core/services/replay/replay_controller.h"
#include "devtool/base_devtool/native/public/devtool_status.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
#include "devtool/lynx_devtool/element/element_helper.h"
#include "devtool/lynx_devtool/element/helper_util.h"
#include "third_party/modp_b64/modp_b64.h"
#include "third_party/zlib/zlib.h"

namespace lynx {
namespace devtool {

InspectorTasmExecutor::InspectorTasmExecutor(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : dom_use_compression_(false),
      dom_compression_threshold_(10240),
      element_root_(nullptr),
      tasm_(),
      devtool_mediator_wp_(devtool_mediator) {}
InspectorTasmExecutor::InspectorTasmExecutor(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator,
    const std::shared_ptr<tasm::TemplateAssembler> tasm)
    : dom_use_compression_(false),
      dom_compression_threshold_(10240),
      element_root_(nullptr),
      tasm_(tasm),
      devtool_mediator_wp_(devtool_mediator) {}

void InspectorTasmExecutor::SetDevToolPlatformFacade(
    const std::shared_ptr<DevToolPlatformFacade>& devtool_platform_facade) {
  devtool_platform_facade_ = devtool_platform_facade;
}

void InspectorTasmExecutor::SendDOMEventMsg(const DomCdpEvent& event_name,
                                            int nodeId, const std::string& name,
                                            int parentNodeId) {
  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");

  Json::Value msg(Json::ValueType::objectValue);
  msg["params"] = Json::ValueType::objectValue;
  if (event_name == DomCdpEvent::DOCUMENT_UPDATED) {
    msg["method"] = "DOM.documentUpdated";
  } else if (event_name == DomCdpEvent::ATTRIBUTE_REMOVED) {
    msg["method"] = "DOM.attributeRemoved";
    msg["params"]["nodeId"] = nodeId;
    msg["params"]["name"] = name;
  } else if (event_name == DomCdpEvent::ATTRIBUTE_MODIFIED) {
    msg["method"] = "DOM.attributeModified";
    msg["params"]["nodeId"] = nodeId;
    msg["params"]["name"] = name;
    Element* ptr = GetElementById(nodeId);
    if (ptr != nullptr) {
      msg["params"]["value"] =
          ElementHelper::GetAttributesAsTextOfNode(ptr, name);
    }
  } else if (event_name == DomCdpEvent::CHILD_NODE_REMOVED) {
    msg["method"] = "DOM.childNodeRemoved";
    msg["params"]["parentNodeId"] = parentNodeId;
    msg["params"]["nodeId"] = nodeId;
  } else {
    msg["method"] = "ERROR method";
  }
  devtool_mediator->SendCDPEvent(msg);
}

void InspectorTasmExecutor::OnDocumentUpdated() {
  Json::Value msg(Json::ValueType::objectValue);
  SendDOMEventMsg(DomCdpEvent::DOCUMENT_UPDATED, -1, "", -1);
}

void InspectorTasmExecutor::OnElementNodeAdded(lynx::tasm::Element* ptr) {
  CHECK_NULL_AND_LOG_RETURN(ptr, "ptr is null");
  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");

  if (ElementInspector::SelectorTag(ptr) == "page") {
    element_root_ = ptr;
#if LYNX_ENABLE_TRACING
    lynx::base::tracing::InstanceCounterTraceImpl::InitNodeCounter();
#endif
  } else {
    // For Radon diff test case as follows:
    // class Condition extends Component<{ condition: boolean,
    // removeComponentElement: true }> {
    //      render() {
    //        const { condition} = this.props;
    //        if (typeof condition !== 'boolean') {
    //          return null;
    //        }
    //        return condition ? <Loading1 /> : <Loading2 />;
    //      }
    //    }
    // when Loading1 is removed and Loading2 is added, Loading1 won't be
    // correctly removed from dom tree because it can't find
    // parentComponentElement which has  been moved to new RadonComponent.
    // Given Loading1 and Loading2 has the same parentComponentElement, and when
    // Loading1 is removed, it is actually remove parentComponentElement from
    // dom tree,so the effect of removing loading1 and loading2 is the same, and
    // even though loading1 doesn't exist,It doesn't matter if you send one more
    // message.
    // as above, before loading2 is added, remove it first.
    if (ElementInspector::GetParentComponentElementFromDataModel(ptr) &&
        ElementInspector::IsNeedEraseId(
            ElementInspector::GetParentComponentElementFromDataModel(ptr))) {
      OnElementNodeRemoved(ptr);
    }

    Element* parentNode = ptr->parent();
    if (parentNode != nullptr) {
      Json::Value msg(Json::ValueType::objectValue);
      msg["method"] = "DOM.childNodeInserted";
      msg["params"] = Json::ValueType::objectValue;
      msg["params"]["parentNodeId"] = ElementInspector::NodeId(parentNode);
      // TODO: tanxuelian.rovic
      // optimize data processing uniformly later, which can be done in the
      // following way

      // Json::Value msg(Json:ValueType:objectValue);
      // msg["method"] = "DOM.childNodeInserted";
      // Json::Value params(Json:ValueType:objectValue);
      // params["parentNodeId"] = ElementInspector::NodeId(parent);
      // msg["params"] = std::move(params);

      Element* previous_node = ElementHelper::GetPreviousNode(ptr);
      if (!previous_node) {
        msg["params"]["previousNodeId"] = 0;
      } else {
        Element* parent =
            ElementInspector::GetParentComponentElementFromDataModel(
                previous_node);
        while (parent && ElementInspector::IsNeedEraseId(parent)) {
          previous_node = parent;
          parent = ElementInspector::GetParentComponentElementFromDataModel(
              previous_node);
        }
        msg["params"]["previousNodeId"] =
            ElementInspector::NodeId(previous_node);
      }

      msg["params"]["node"] = ElementHelper::GetDocumentBodyFromNode(ptr);
      msg["compress"] = false;

      devtool_mediator->RunOnDevToolThread(
          [devtool_mediator, self = shared_from_this(), msg]() mutable {
            std::string params_str = msg["params"].toStyledString();
            if (self->dom_use_compression_ &&
                params_str.size() >
                    static_cast<size_t>(self->dom_compression_threshold_)) {
              InspectorUtil::CompressData("childNodeInserted",
                                          msg["params"].toStyledString(), msg,
                                          "params");
            }
            devtool_mediator->SendCDPEvent(msg);
          },
          true);
    }
  }
#if LYNX_ENABLE_TRACING
  lynx::base::tracing::InstanceCounterTraceImpl::IncrementNodeCounter(ptr);
#endif
}

void InspectorTasmExecutor::OnElementNodeRemoved(Element* ptr) {
  CHECK_NULL_AND_LOG_RETURN(ptr, "ptr is null");
  Element* parent = ptr->parent();
  if (parent) {
    Element* remove_element = ptr;
    while (ElementInspector::GetParentComponentElementFromDataModel(
               remove_element) &&
           ElementInspector::IsNeedEraseId(
               ElementInspector::GetParentComponentElementFromDataModel(
                   remove_element))) {
      remove_element = ElementInspector::GetParentComponentElementFromDataModel(
          remove_element);
    }

    SendDOMEventMsg(DomCdpEvent::CHILD_NODE_REMOVED,
                    ElementInspector::NodeId(remove_element), "",
                    ElementInspector::NodeId(parent));
  }

#if LYNX_ENABLE_TRACING
  lynx::base::tracing::InstanceCounterTraceImpl::DecrementNodeCounter(ptr);
#endif
}

// not used yet
void InspectorTasmExecutor::OnCharacterDataModified(lynx::tasm::Element* ptr) {
  CHECK_NULL_AND_LOG_RETURN(ptr, "ptr is null");
  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");

  Json::Value msg(Json::ValueType::objectValue);
  Json::Value res(Json::ValueType::objectValue);
  msg["method"] = "DOM.characterDataModified";
  msg["params"]["nodeId"] = ElementInspector::NodeId(ptr);
  msg["params"]["characterData"] = ElementHelper::GetStyleNodeText(ptr);
  devtool_mediator->SendCDPEvent(msg);
}

void InspectorTasmExecutor::OnElementDataModelSet(lynx::tasm::Element* ptr) {
  CHECK_NULL_AND_LOG_RETURN(ptr, "ptr is null");
  DiffID(ptr);
  DiffAttr(ptr);
  DiffClass(ptr);
  DiffStyle(ptr);
}

void InspectorTasmExecutor::OnElementManagerWillDestroy() {
  element_root_ = nullptr;
}

void InspectorTasmExecutor::DiffID(lynx::tasm::Element* ptr) {
  std::string old_id = ElementInspector::SelectorId(ptr);
  std::string new_id = ElementInspector::GetSelectorIDFromAttributeHolder(ptr);
  ElementInspector::SetSelectorId(ptr, new_id);
  if (!old_id.empty()) {
    ElementInspector::DeleteAttr(ptr, "id");
    SendDOMEventMsg(DomCdpEvent::ATTRIBUTE_REMOVED,
                    ElementInspector::NodeId(ptr), "id", -1);
  }
  if (!new_id.empty()) {
    ElementInspector::UpdateAttr(ptr, "id", new_id);
    SendDOMEventMsg(DomCdpEvent::ATTRIBUTE_MODIFIED,
                    ElementInspector::NodeId(ptr), "id", -1);
  }
}

void InspectorTasmExecutor::DiffAttr(lynx::tasm::Element* ptr) {
  const auto old_attr = ElementInspector::AttrMap(ptr);
  const auto& new_attr =
      ElementInspector::GetAttrFromAttributeHolder(ptr).second;

  // Events are also a type of attribute, so when DiffAttr is performed, events
  // are also diffed.
  const auto old_event_attr = ElementInspector::EventMap(ptr);
  const auto& new_event_attr =
      ElementInspector::GetEventMapFromAttributeHolder(ptr).second;

  const auto old_data_attr = ElementInspector::DataMap(ptr);
  const auto& new_data_attr =
      ElementInspector::GetDataSetFromAttributeHolder(ptr).second;

  const auto& diff_attr_map_function = [this, ptr](const auto& new_attr,
                                                   const auto& old_attr) {
    for (const auto& pair : new_attr) {
      auto iter = old_attr.find(pair.first);
      if (iter == old_attr.end() || iter->second != pair.second) {
        ElementInspector::UpdateAttr(ptr, pair.first, pair.second);
        SendDOMEventMsg(DomCdpEvent::ATTRIBUTE_MODIFIED,
                        ElementInspector::NodeId(ptr), pair.first, -1);
      }
    }
    for (const auto& pair : old_attr) {
      if (new_attr.find(pair.first) == new_attr.end()) {
        ElementInspector::DeleteAttr(ptr, pair.first);
        SendDOMEventMsg(DomCdpEvent::ATTRIBUTE_REMOVED,
                        ElementInspector::NodeId(ptr), pair.first, -1);
      }
    }
  };

  diff_attr_map_function(new_attr, old_attr);
  diff_attr_map_function(new_event_attr, old_event_attr);
  diff_attr_map_function(new_data_attr, old_data_attr);
}

void InspectorTasmExecutor::DiffClass(lynx::tasm::Element* ptr) {
  std::vector<std::string> old_class = ElementInspector::ClassOrder(ptr);
  std::vector<std::string> new_class =
      ElementInspector::GetClassOrderFromAttributeHolder(ptr);
  if (old_class != new_class) {
    ElementInspector::DeleteClasses(ptr);
    {
      SendDOMEventMsg(DomCdpEvent::ATTRIBUTE_REMOVED,
                      ElementInspector::NodeId(ptr), "class", -1);
    }

    ElementInspector::UpdateClasses(ptr, new_class);
    {
      SendDOMEventMsg(DomCdpEvent::ATTRIBUTE_MODIFIED,
                      ElementInspector::NodeId(ptr), "class", -1);
    }
  }
}

void InspectorTasmExecutor::DiffStyle(lynx::tasm::Element* ptr) {
  CHECK_NULL_AND_LOG_RETURN(ptr, "ptr is null");
  auto* inspector_attribute = ptr->inspector_attribute();
  CHECK_NULL_AND_LOG_RETURN(inspector_attribute, "inspector_attribute is null");

  std::unordered_map<std::string, std::string> old_style;
  for (auto& iter : inspector_attribute->inline_style_sheet_.css_properties_) {
    old_style[iter.first] = iter.second.value_;
  }
  auto new_style = ElementInspector::GetInlineStylesFromAttributeHolder(ptr);
  for (const auto& pair : new_style) {
    ElementInspector::UpdateStyle(ptr, pair.first, pair.second);
    {
      SendDOMEventMsg(DomCdpEvent::ATTRIBUTE_MODIFIED,
                      ElementInspector::NodeId(ptr), "style", -1);
    }
    for (const auto& pair : old_style) {
      if (new_style.find(pair.first) == new_style.end()) {
        ElementInspector::DeleteStyle(ptr, pair.first);
        {
          if (ElementInspector::GetInlineStyleSheet(ptr)
                  .css_properties_.empty()) {
            SendDOMEventMsg(DomCdpEvent::ATTRIBUTE_REMOVED,
                            ElementInspector::NodeId(ptr), "style", -1);
          } else {
            SendDOMEventMsg(DomCdpEvent::ATTRIBUTE_MODIFIED,
                            ElementInspector::NodeId(ptr), "style", -1);
          }
        }
      }
    }
  }
}

void InspectorTasmExecutor::OnSetNativeProps(lynx::tasm::Element* ptr,
                                             const std::string& name,
                                             const std::string& value,
                                             bool is_style) {
  if (!ptr) {
    return;
  }
  if (is_style) {
    ElementInspector::UpdateStyle(ptr, name, value);
    {
      SendDOMEventMsg(DomCdpEvent::ATTRIBUTE_MODIFIED,
                      ElementInspector::NodeId(ptr), "style", -1);
    }
  } else {
    ElementInspector::UpdateAttr(ptr, name, value);
    {
      SendDOMEventMsg(DomCdpEvent::ATTRIBUTE_MODIFIED,
                      ElementInspector::NodeId(ptr), name, -1);
    }
  }
}

lynx::tasm::Element* InspectorTasmExecutor::GetElementById(int node_id) {
  CHECK_NULL_AND_LOG_RETURN_VALUE(element_root_, "element_root_ is null",
                                  nullptr);
  auto* element_manager = element_root_->element_manager();
  CHECK_NULL_AND_LOG_RETURN_VALUE(element_manager, "element_manager is null",
                                  nullptr);
  auto* node_manager = element_manager->node_manager();
  CHECK_NULL_AND_LOG_RETURN_VALUE(node_manager, "node_manager is null",
                                  nullptr);
  return node_manager->Get(node_id);
}

lynx::tasm::Element* InspectorTasmExecutor::GetElementRoot() {
  return element_root_;
}

void InspectorTasmExecutor::QuerySelector(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  std::string selector = params["selector"].asString();
  Element* start_node;
  if (params.isMember("nodeId")) {
    int node_id = params["nodeId"].asInt();
    start_node = GetElementById(node_id);
  } else {
    start_node = element_root_;
  }
  content["nodeId"] =
      start_node ? ElementHelper::QuerySelector(start_node, selector) : -1;

  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::GetAttributes(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  int node_id = params["nodeId"].asInt();
  Element* ptr = GetElementById(node_id);
  if (ptr) {
    content["attributes"] = ElementHelper::GetAttributesImpl(ptr);
  }
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::InnerText(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  int nodeId = params["nodeId"].asInt();
  Element* element = GetElementById(nodeId);
  Json::Value raw_text_value_array(Json::ValueType::arrayValue);
  // find all raw-text on text element
  if (element && !ElementInspector::LocalName(element).compare("text")) {
    for (Element* raw_text_child : element->GetChildren()) {
      if (ElementInspector::LocalName(raw_text_child).compare("raw-text")) {
        continue;
      }
      auto itr = ElementInspector::AttrMap(raw_text_child).find("text");
      if (itr != ElementInspector::AttrMap(element).end()) {
        Json::Value attr_text_value(Json::ValueType::objectValue);
        attr_text_value["nodeId"] = ElementInspector::NodeId(raw_text_child);
        attr_text_value["text"] = itr->second;
        raw_text_value_array.append(attr_text_value);
      }
    }
  }
  content["nodeId"] = static_cast<int>(nodeId);
  content["rawTextValues"] = raw_text_value_array;
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::QuerySelectorAll(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  std::string selector = params["selector"].asString();
  Element* start_node;
  if (params.isMember("nodeId")) {
    int node_id = params["nodeId"].asInt();
    start_node = GetElementById(node_id);
  } else {
    start_node = element_root_;
  }
  content["nodeIds"] =
      start_node ? ElementHelper::QuerySelectorAll(start_node, selector)
                 : Json::Value(Json::ValueType::arrayValue);

  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

const std::map<DevToolFunction, std::function<void(const lynx::base::any&)>>&
InspectorTasmExecutor::GetFunctionForElementMap() {
  static lynx::base::NoDestructor<
      std::map<DevToolFunction, std::function<void(const lynx::base::any&)>>>
      function_map{
          {{DevToolFunction::InitForInspector,
            &ElementInspector::InitForInspector},
           {DevToolFunction::InitPlugForInspector,
            &ElementInspector::InitPlugForInspector},
           {DevToolFunction::InitStyleValueElement,
            &ElementInspector::InitStyleValueElement},
           {DevToolFunction::InitStyleRoot, &ElementInspector::InitStyleRoot},
           {DevToolFunction::SetDocElement, &ElementInspector::SetDocElement},
           {DevToolFunction::SetStyleValueElement,
            &ElementInspector::SetStyleValueElement},
           {DevToolFunction::SetStyleRoot, &ElementInspector::SetStyleRoot}}};
  return *function_map;
}

void InspectorTasmExecutor::DOM_Enable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value params = message["params"];
  if (params.isMember("useCompression")) {
    dom_use_compression_ = params["useCompression"].asBool();
  }
  if (params.isMember("compressionThreshold")) {
    dom_compression_threshold_ = params["compressionThreshold"].asInt();
  }

  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::DOM_Disable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::GetDocument(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content = Json::Value(Json::ValueType::objectValue);
  if (element_root_ == nullptr) {
    response["result"] = content;
    response["id"] = message["id"].asInt64();
    sender->SendMessage("CDP", response);
    return;
  }

  content["root"] = ElementHelper::GetDocumentBodyFromNode(element_root_);
  content["compress"] = false;

  response["result"] = content;
  response["id"] = message["id"].asInt64();

  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");
  devtool_mediator->RunOnDevToolThread(
      [sender, self = shared_from_this(), content, response]() mutable {
        std::string root_str = content["root"].toStyledString();
        if (self->dom_use_compression_ &&
            root_str.size() >
                static_cast<size_t>(self->dom_compression_threshold_)) {
          InspectorUtil::CompressData("getDocument", root_str, content, "root");
        }
        response["result"] = content;
        sender->SendMessage("CDP", response);
      },
      true);
}

void InspectorTasmExecutor::GetDocumentWithBoxModel(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content = Json::Value(Json::ValueType::objectValue);
  tasm::Element* root = GetElementRoot();
  CHECK_NULL_AND_LOG_RETURN(root, "root is null");

  content["root"] = GetDocumentBodyFromNodeWithBoxModel(root);
  content["compress"] = false;

  response["result"] = content;
  response["id"] = message["id"].asInt64();

  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");
  devtool_mediator->RunOnDevToolThread(
      [this, sender, content, response, message]() mutable {
        std::string root_str = content["root"].toStyledString();
        if (dom_use_compression_ &&
            root_str.size() > static_cast<size_t>(dom_compression_threshold_)) {
          InspectorUtil::CompressData("getDocumentWithBoxModel", root_str,
                                      content, "root");
        }
        response["result"] = content;
        sender->SendMessage("CDP", response);
      },
      true);
}

void InspectorTasmExecutor::RequestChildNodes(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content = Json::Value(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  int node_id = params["nodeId"].asInt();
  [[maybe_unused]] int depth = 1;
  if (params.isMember("depth")) {
    depth = params["depth"].asInt();
  }
  Json::Value nodes(Json::ValueType::arrayValue);
  Element* cur_node = GetElementById(node_id);
  if (cur_node != nullptr) {
    for (Element* child : cur_node->GetChildren()) {
      Json::Value node_info(Json::ValueType::objectValue);
      node_info["parentId"] = ElementInspector::NodeId(child->parent());
      node_info["backendNodeId"] = 0;
      node_info["childNodeCount"] =
          static_cast<int>(child->GetChildren().size());
      node_info["localName"] = ElementInspector::LocalName(child);
      node_info["nodeId"] = ElementInspector::NodeId(child);
      node_info["nodeName"] = ElementInspector::NodeName(child);
      node_info["nodeType"] = ElementInspector::NodeType(child);
      node_info["nodeValue"] = ElementInspector::NodeValue(child);
      node_info["attributes"] = ElementHelper::GetAttributesImpl(child);
      ;
      nodes.append(node_info);
    }
  }

  content["parentId"] = node_id;
  content["nodes"] = nodes;
  response["result"] = content;
  response["id"] = message["id"].asInt64();

  // call method
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::DOM_GetBoxModel(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content = Json::Value(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  int index = params["nodeId"].asInt();
  tasm::Element* ptr = GetElementById(index);
  double screen_scale_factor = 1.0f;
  if (ptr != nullptr) {
    std::string screen_shot_mode = DevToolStatus::GetInstance().GetStatus(
        DevToolStatus::kDevToolStatusKeyScreenShotMode);
    content = GetBoxModelOfNode(ptr, screen_scale_factor, screen_shot_mode,
                                GetElementRoot());
  }

  if (content.empty()) {
    Json::Value error = Json::Value(Json::ValueType::objectValue);
    error["code"] = Json::Value(-32000);
    error["message"] = Json::Value("Could not compute box model");
    content["error"] = error;
  }
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

std::vector<double> InspectorTasmExecutor::GetBoxModel(tasm::Element* element) {
  std::vector<double> box_model;
  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN_VALUE(devtool_mediator, "devtool_mediator is null",
                                  {});
  return devtool_mediator->GetBoxModel(element);
}

void InspectorTasmExecutor::SetAttributesAsText(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  std::vector<Json::Value> msg_v;
  int index = params["nodeId"].asInt();
  std::string name = params["name"].asString();
  std::string text = params["text"].asString();
  Element* ptr = GetElementById(index);
  if (ptr != nullptr) {
    msg_v = ElementHelper::SetAttributesAsText(ptr, name, text);
  }

  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");
  for (const Json::Value& msg : msg_v) {
    devtool_mediator->SendCDPEvent(msg);
  }
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

// This protocol has not been implemented yet
void InspectorTasmExecutor::MarkUndoableState(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::PushNodesByBackendIdsToFrontend(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value nodeIds(Json::ValueType::arrayValue);
  Json::Value params = message["params"];
  nodeIds = params["backendNodeIds"];
  content["nodeIds"] = nodeIds;
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

// This protocol has not been implemented yet
void InspectorTasmExecutor::RemoveNode(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value content(Json::ValueType::objectValue);
}

// This protocol has not been implemented yet
void InspectorTasmExecutor::MoveTo(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value content(Json::ValueType::objectValue);
}

// This protocol has not been implemented yet
void InspectorTasmExecutor::CopyTo(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value content(Json::ValueType::objectValue);
}

void InspectorTasmExecutor::GetOuterHTML(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  size_t index = static_cast<size_t>(params["nodeId"].asInt64());
  Element* ptr = GetElementById(static_cast<int>(index));
  if (ptr != nullptr) {
    content["outerHTML"] = ElementHelper::GetElementContent(ptr, 0);
  }
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

// This protocol has not been implemented yet
void InspectorTasmExecutor::SetOuterHTML(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value content(Json::ValueType::objectValue);
}

void InspectorTasmExecutor::SetInspectedNode(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::PerformSearch(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  std::string query = params["query"].asString();
  uint64_t searchId = lynx::base::CurrentTimeMilliseconds();
  std::vector<int> searchResults;
  ElementHelper::PerformSearchFromNode(element_root_, query, searchResults);
  search_results_[searchId] = searchResults;
  content["searchId"] = searchId;
  content["resultCount"] = static_cast<int>(searchResults.size());
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::GetSearchResults(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  uint64_t searchId = params["searchId"].asUInt64();
  int fromIndex = params["fromIndex"].asInt();
  int toIndex = params["toIndex"].asInt();
  Json::Value nodeIds(Json::ValueType::arrayValue);
  auto iter = search_results_.find(searchId);
  if (iter != search_results_.end()) {
    std::vector<int> searchResults = iter->second;
    for (int index = fromIndex; index < toIndex; ++index) {
      if (index >= static_cast<int>(searchResults.size())) {
        break;
      }
      nodeIds.append(Json::Value(searchResults[index]));
    }
    content["nodeIds"] = nodeIds;
    response["result"] = content;
  } else {
    Json::Value error(Json::ValueType::objectValue);
    error["code"] = 32000;
    error["message"] = "SearchId not found.";
    response["error"] = error;
  }
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::DiscardSearchResults(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  uint64_t searchId = params["searchId"].asUInt64();
  if (search_results_.find(searchId) != search_results_.end()) {
    search_results_.erase(searchId);
    response["result"] = content;
  } else {
    Json::Value error(Json::ValueType::objectValue);
    error["code"] = 32000;
    error["message"] = "SearchId not found.";
    response["error"] = error;
  }
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::GetOriginalNodeIndex(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);

  Json::Value params = message["params"];
  size_t node_id = static_cast<size_t>(params["nodeId"].asInt64());

  Element* element = GetElementById(static_cast<int>(node_id));
  if (element != nullptr) {
    uint32_t node_index = element->NodeIndex();
    content["nodeIndex"] = node_index;
  }
  response["id"] = message["id"].asInt64();
  response["result"] = content;
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::ScrollIntoViewIfNeeded(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);

  Json::Value params = message["params"];
  size_t node_id = static_cast<size_t>(params["nodeId"].asInt64());
  Element* current_element = GetElementById(static_cast<int>(node_id));
  while (current_element != nullptr && (current_element->is_virtual() ||
                                        current_element->CanBeLayoutOnly())) {
    current_element = current_element->parent();
  }
  if (current_element == nullptr) {
    Json::Value error(Json::ValueType::objectValue);
    error["code"] = Json::Value(-32000);
    ;
    error["message"] = Json::Value("Element not found.");
    response["error"] = error;
    response["id"] = message["id"].asInt64();
    sender->SendMessage("CDP", response);
    return;
  }
  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");

  devtool_mediator->ScrollIntoView(ElementInspector::NodeId(current_element));

  response["id"] = message["id"].asInt64();
  response["result"] = content;
  sender->SendMessage("CDP", response.toStyledString());
}

void InspectorTasmExecutor::DOMEnableDomTree(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  lynx::tasm::LynxEnv::GetInstance().SetBoolLocalEnv(
      lynx::tasm::LynxEnv::kLynxEnableDomTree, true);
  Json::Value params = message["params"];
  bool ignore_cache = false;
  if (!params.empty()) {
    ignore_cache = params["ignoreCache"].asBool();
  }
  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");
  devtool_mediator->PageReload(ignore_cache);
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::DOMDisableDomTree(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  lynx::tasm::LynxEnv::GetInstance().SetBoolLocalEnv(
      lynx::tasm::LynxEnv::kLynxEnableDomTree, false);
  Json::Value params = message["params"];
  bool ignore_cache = false;
  if (!params.empty()) {
    ignore_cache = params["ignoreCache"].asBool();
  }
  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");
  devtool_mediator->PageReload(ignore_cache);
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

// CSS protocol
void GetElementByType(InspectorElementType type, std::vector<Element*>& res,
                      Element* root) {
  CHECK_NULL_AND_LOG_RETURN(root, "root is null");
  if (ElementInspector::Type(root) == type) {
    res.push_back(root);
    return;
  } else if (ElementInspector::Type(root) == InspectorElementType::COMPONENT) {
    Element* style_value = ElementInspector::StyleValueElement(root);
    GetElementByType(type, res, style_value);
  }

  Element* comp_ptr =
      ElementInspector::GetParentComponentElementFromDataModel(root);
  while (comp_ptr && ElementInspector::IsNeedEraseId(comp_ptr)) {
    Element* style_value = ElementInspector::StyleValueElement(comp_ptr);
    GetElementByType(type, res, style_value);

    comp_ptr =
        ElementInspector::GetParentComponentElementFromDataModel(comp_ptr);
  }

  if (!root->GetChildren().empty()) {
    for (Element* child : root->GetChildren()) {
      GetElementByType(type, res, child);
    }
  }
}

Json::Value InspectorTasmExecutor::GetUsageItem(
    const std::string& stylesheet_id, const std::string& content,
    const std::string& selector) {
  Json::Value usage_item(Json::ValueType::objectValue);
  usage_item["styleSheetId"] = stylesheet_id;

  // find the start index and end index of the selector in 'content'
  auto start_offset =
      content.find(selector + lynx::devtool::kPaddingCurlyBrackets);
  usage_item["startOffset"] = static_cast<Json::Int64>(start_offset);
  usage_item["endOffset"] =
      static_cast<Json::Int64>(content.find('\n', start_offset)) + 1;
  usage_item["used"] = true;
  return usage_item;
}

void InspectorTasmExecutor::SendCSSEventMsg(const CssCdpEvent& event_name,
                                            const std::string& style_sheet_id,
                                            lynx::tasm::Element* ptr) {
  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");
  Json::Value msg(Json::ValueType::objectValue);
  msg["params"] = Json::Value(Json::ValueType::objectValue);
  if (event_name == CssCdpEvent::STYLE_SHEET_ADDED) {
    msg["method"] = "CSS.styleSheetAdded";
    if (ptr) {
      msg["params"]["header"] = ElementHelper::GetStyleSheetHeader(ptr);
    }
  } else if (event_name == CssCdpEvent::STYLE_SHEET_REMOVED) {
    // not called ever
    msg["method"] = "CSS.styleSheetRemoved";
    msg["params"]["styleSheetId"] = style_sheet_id;
  } else if (event_name == CssCdpEvent::STYLE_SHEET_CHANGED) {
    msg["method"] = "CSS.styleSheetChanged";
    msg["params"]["styleSheetId"] = style_sheet_id;
  } else {
    msg["method"] = "ERROR method";
  }
  devtool_mediator->SendCDPEvent(msg);
}

void InspectorTasmExecutor::OnCSSStyleSheetAdded(lynx::tasm::Element* ptr) {
  SendCSSEventMsg(CssCdpEvent::STYLE_SHEET_ADDED, "", ptr);
}

// Enable CSS debugging, get all style sheet of current page
void InspectorTasmExecutor::CSS_Enable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);

  // then send styleSheetAdded event
  std::vector<Element*> style_values;
  GetElementByType(InspectorElementType::STYLEVALUE, style_values,
                   element_root_);
  for (Element* ptr : style_values) {
    if (ptr != nullptr &&
        ElementInspector::Type(ptr) == InspectorElementType::STYLEVALUE) {
      SendCSSEventMsg(CssCdpEvent::STYLE_SHEET_ADDED, "", ptr);
    }
  }
}

// This protocol has not been implemented yet
void InspectorTasmExecutor::CSS_Disable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::GetMatchedStylesForNode(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  size_t index = static_cast<size_t>(params["nodeId"].asInt64());
  Element* ptr = GetElementById(static_cast<int>(index));
  if (ptr != nullptr) {
    content = ElementHelper::GetMatchedStylesForNode(ptr);
  } else {
    Json::Value error = Json::Value(Json::ValueType::objectValue);
    error["code"] = Json::Value(-32000);
    error["message"] = Json::Value("Node is not an Element");
    content["error"] = error;
  }
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::GetComputedStyleForNode(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  size_t index = static_cast<size_t>(params["nodeId"].asInt64());
  Element* ptr = GetElementById(static_cast<int>(index));
  if (ptr != nullptr) {
    content["computedStyle"] = GetComputedStyleOfNode(ptr);
  }
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::GetInlineStylesForNode(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  size_t index = static_cast<size_t>(params["nodeId"].asInt64());
  Element* ptr = GetElementById(static_cast<int>(index));
  if (ptr != nullptr) {
    content["inlineStyle"] = ElementHelper::GetInlineStyleOfNode(ptr);
  }
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::SetStyleTexts(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  Json::Value edits =
      params.get("edits", Json::Value(Json::ValueType::arrayValue));
  for (const Json::Value& edit : edits) {
    std::string style_sheet_id = edit["styleSheetId"].asString();
    int index =
        atoi(style_sheet_id.substr(style_sheet_id.find('.') + 1).c_str());
    Json::Value range_json =
        edit.get("range", Json::Value(Json::ValueType::objectValue));
    Range range;
    range.start_line_ = range_json["startLine"].asInt();
    range.start_column_ = range_json["startColumn"].asInt();
    range.end_line_ = range_json["endLine"].asInt();
    range.end_column_ = range_json["endColumn"].asInt();
    std::string text = edit["text"].asString();
    Element* ptr = GetElementById(static_cast<int>(index));
    if (ptr != nullptr) {
      ElementHelper::SetStyleTexts(element_root_, ptr, text, range);
      content =
          ElementHelper::GetStyleSheetAsTextOfNode(ptr, style_sheet_id, range);
      std::string res = content.toStyledString();
      Json::Value msg(Json::ValueType::objectValue);
      if (ElementInspector::Type(ptr) != InspectorElementType::STYLEVALUE ||
          ElementInspector::Type(ptr) != InspectorElementType::DOCUMENT) {
        int nodeid =
            atoi(style_sheet_id.substr(style_sheet_id.find('.') + 1).c_str());
        SendDOMEventMsg(DomCdpEvent::ATTRIBUTE_MODIFIED, nodeid, "style", -1);
      }
      SendCSSEventMsg(CssCdpEvent::STYLE_SHEET_CHANGED, style_sheet_id, ptr);
    }
  }
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

// Get the content of the specified style sheet ( Unsure of usage scenario)
void InspectorTasmExecutor::GetStyleSheetText(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  std::string style_sheet_id = params["styleSheetId"].asString();
  int index = atoi(style_sheet_id.substr(style_sheet_id.find('.') + 1).c_str());
  Element* ptr = GetElementById(static_cast<int>(index));
  if (ptr != nullptr) {
    content = ElementHelper::GetStyleSheetText(ptr, style_sheet_id);
  } else {
    Json::Value error = Json::Value(Json::ValueType::objectValue);
    error["code"] = Json::Value(-32000);
    error["message"] = Json::Value("Node is not an Element");
    content["error"] = error;
  }
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

// Get info of the specified node, Unsure of usage scenario
void InspectorTasmExecutor::GetBackgroundColors(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  size_t index = static_cast<size_t>(params["nodeId"].asInt64());
  Element* ptr = GetElementById(static_cast<int>(index));
  if (ptr != nullptr) {
    content = ElementHelper::GetBackGroundColorsOfNode(ptr);
  }
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::SetStyleSheetText(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  std::string style_sheet_id = params["styleSheetId"].asString();
  content["sourceMapURL"] = "";
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

// Create a style sheet (not sure where to use it)
void InspectorTasmExecutor::CreateStyleSheet(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  Json::Value header = ElementHelper::CreateStyleSheet(element_root_);
  content["styleSheetId"] = header["styleSheetId"];
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);

  // then send styleSheetAdded event
  Json::Value msg(Json::ValueType::objectValue);
  msg["method"] = "CSS.styleSheetAdded";
  msg["params"]["header"] = header;
  devtool_mediator->SendCDPEvent(msg);
}

// Add a CSS rule (such as a class) to the specified style sheet (not sure where
// to use it)
void InspectorTasmExecutor::AddRule(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  std::string style_sheet_id = params["styleSheetId"].asString();
  std::string rule_text = params["ruleText"].asString();
  Range range;
  range.start_line_ = params["location"]["startLine"].asInt();
  range.start_column_ = params["location"]["startColumn"].asInt();
  range.end_line_ = params["location"]["endLine"].asInt();
  range.end_column_ = params["location"]["endColumn"].asInt();
  int index = atoi(style_sheet_id.substr(style_sheet_id.find('.') + 1).c_str());
  Element* ptr = GetElementById(index);
  response["result"] =
      ElementHelper::AddRule(ptr, style_sheet_id, rule_text, range);

  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::CollectDomTreeCssUsage(
    Json::Value& rule_usage_array, const std::string& stylesheet_id,
    const std::string& content) {
  Element* root = element_root_;
  CHECK_NULL_AND_LOG_RETURN(root, "root is null");
  std::queue<Element*> inspect_node_queue;
  inspect_node_queue.push(root);
  while (!inspect_node_queue.empty()) {
    Element* element = inspect_node_queue.front();
    inspect_node_queue.pop();
    for (Element* child : element->GetChildren()) {
      inspect_node_queue.push(child);
    }
    if (ElementInspector::Type(element) == InspectorElementType::DOCUMENT) {
      continue;
    }

    std::string select_id = ElementInspector::SelectorId(element);
    if (!select_id.empty()) {
      rule_usage_array.append(GetUsageItem(stylesheet_id, content, select_id));
    }

    std::vector<std::string> class_order =
        ElementInspector::ClassOrder(element);
    for (std::string& order : class_order) {
      if (!order.empty()) {
        rule_usage_array.append(GetUsageItem(stylesheet_id, content, order));
      }
    }
  }
}

// CSS.startRuleUsageTracking/CSS.updateRuleUsageTracking/CSS.stopRuleUsageTracking
// added for Perf (wangjiankang)
void InspectorTasmExecutor::StartRuleUsageTracking(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  rule_usage_tracking_ = true;
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::UpdateRuleUsageTracking(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (rule_usage_tracking_) {
    Json::Value selectors(Json::ValueType::arrayValue);
    selectors = message["params"]["selector"];
    for (const Json::Value& selector : selectors) {
      css_used_selector_.insert(selector.asString());
    }
  }
}

void InspectorTasmExecutor::StopRuleUsageTracking(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value res(Json::ValueType::objectValue);
  Json::Value rule_usage(Json::ValueType::arrayValue);

  std::vector<Element*> elements;
  GetElementByType(InspectorElementType::STYLEVALUE, elements, element_root_);
  Element* ptr = elements.empty() ? element_root_ : elements[0];
  std::string style_sheet_id = std::to_string(ElementInspector::NodeId(ptr));

  std::string content;
  Element* element_ptr = GetElementById(ElementInspector::NodeId(ptr));
  if (element_ptr != nullptr) {
    content =
        ElementHelper::GetStyleSheetText(element_ptr, style_sheet_id)["text"]
            .asString();
  }

  if (css_used_selector_.empty()) {
    CollectDomTreeCssUsage(rule_usage, style_sheet_id, content);
  } else {
    for (const std::string& selector : css_used_selector_) {
      if (!selector.empty()) {
        rule_usage.append(GetUsageItem(style_sheet_id, content, selector));
      }
    }
  }

  res["ruleUsage"] = rule_usage;
  response["result"] = res;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);

  css_used_selector_.clear();
  rule_usage_tracking_ = false;
}

// CSS protocol end

// Overlay protocol
void InspectorTasmExecutor::RestoreOriginNodeInlineStyle() {
  if (origin_node_id_ == 0) return;
  Element* origin_node = GetElementById(static_cast<int>(origin_node_id_));
  CHECK_NULL_AND_LOG_RETURN(origin_node, "origin_node is null");
  ElementHelper::SetInlineStyleSheet(origin_node, origin_inline_style_);
}

void InspectorTasmExecutor::HighlightNode(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  if (!params.isNull()) {
    auto node_id = static_cast<size_t>(params["nodeId"].asInt64());
    auto highlight_config = params["highlightConfig"];
    auto selector = params["selector"].asString();
    auto* current_node = GetElementById(static_cast<int>(node_id));
    if (current_node == nullptr ||
        !ElementInspector::HasDataModel(current_node) ||
        ElementInspector::IsNeedEraseId(current_node)) {
      Json::Value error = Json::Value(Json::ValueType::objectValue);
      error["code"] = Json::Value(-32000);
      error["message"] = Json::Value("Node is not an Element");
      content["error"] = error;
    } else {
      if (node_id != origin_node_id_) {
        RestoreOriginNodeInlineStyle();
        origin_inline_style_ = ElementHelper::GetInlineStyleTexts(current_node);
        origin_node_id_ = node_id;
        auto json_content_color = highlight_config["contentColor"];
        std::stringstream highlightStyle;
        highlightStyle << "background-color:"
                       << lynx::tasm::CSSDecoder::ToRgbaFromRgbaValue(
                              json_content_color["r"].asString(),
                              json_content_color["g"].asString(),
                              json_content_color["b"].asString(),
                              json_content_color["a"].asString())
                       << ";";

        std::string inline_style_str =
            highlightStyle.str() + origin_inline_style_.css_text_;
        ElementHelper::SetInlineStyleTexts(current_node, inline_style_str,
                                           Range());
      }
    }
  }
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::HideHighlight(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  RestoreOriginNodeInlineStyle();
  sender->SendMessage("CDP", response);
}
// Overlay protocol end

void InspectorTasmExecutor::LynxGetProperties(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  content["properties"] = "";
  Json::Value params = message["params"];
  size_t index = static_cast<size_t>(params["nodeId"].asInt64());
  auto* ptr = GetElementById(index);
  if (ptr && ElementInspector::Type(ptr) == InspectorElementType::COMPONENT) {
    content["properties"] = ElementHelper::GetProperties(ptr);
  } else {
    content["properties"] = "";
  }
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::LynxGetData(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  content["data"] = "";
  Json::Value params = message["params"];
  size_t index = static_cast<size_t>(params["nodeId"].asInt64());
  auto* ptr = GetElementById(index);
  if (ptr && ElementInspector::Type(ptr) == InspectorElementType::COMPONENT) {
    content["data"] = ElementHelper::GetData(ptr);
  } else {
    content["data"] = "";
  }
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::LynxGetComponentId(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  content["componentId"] = -1;
  Json::Value params = message["params"];
  size_t index = static_cast<size_t>(params["nodeId"].asInt64());
  auto* ptr = GetElementById(index);
  if (ptr) {
    if (ElementInspector::Type(ptr) == InspectorElementType::COMPONENT) {
      content["componentId"] = ElementHelper::GetComponentId(ptr);
    }
  }
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::TemplateGetTemplateApiInfo(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value result(Json::ValueType::objectValue);
  std::shared_ptr<tasm::TemplateAssembler> tasm = tasm_.lock();
  if (tasm) {
    lynx::lepus::Value default_processor_value = tasm->GetDefaultProcessor();
    result["useDefault"] = default_processor_value.IsClosure();
    std::unordered_map<std::string, lynx::lepus::Value> processor_map =
        tasm->GetProcessorMap();
    if (!processor_map.empty()) {
      Json::Value keys(Json::ValueType::arrayValue);
      for (auto& element : processor_map) {
        keys.append(element.first);
      }
      result["processMapKeys"] = keys;
    }
  } else {
    result["useDefault"] = false;
  }

  response["result"] = result;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::LayerTreeEnable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  sender->SendOKResponse(message["id"].asInt64());
  layer_tree_enabled_ = true;

  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");
  LayerPainted(sender, message);
  LayerTreeDidChange(sender);
}

void InspectorTasmExecutor::LayerTreeDisable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  sender->SendOKResponse(message["id"].asInt64());
  layer_tree_enabled_ = false;
}

void InspectorTasmExecutor::LayerTreeDidChange(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender) {
  if (layer_tree_enabled_) {
    Json::Value response(Json::ValueType::objectValue);
    response["method"] = "LayerTree.layerTreeDidChange";
    Json::Value layers(Json::ValueType::arrayValue);
    lynx::tasm::Element* element = GetElementRoot();

    if (element != nullptr) {
      layers = BuildLayerTreeFromElement(sender, element);
    }
    response["params"]["layers"] = layers;
    sender->SendMessage("CDP", response);
  }
}

void InspectorTasmExecutor::LayerPainted(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value layerId(Json::ValueType::stringValue);
  Json::Value clip(Json::ValueType::objectValue);
  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");

  lynx::tasm::Element* element = GetElementRoot();
  if (element != nullptr) {
    Json::Value rootLayer = GetLayerContentFromElement(sender, element);

    clip["x"] = rootLayer["offsetX"];
    clip["y"] = rootLayer["offsetY"];
    clip["width"] = rootLayer["width"];
    clip["height"] = rootLayer["height"];
    layerId = rootLayer["layerId"].asString();
  }
  response["method"] = "LayerTree.layerPrinted";
  response["params"]["layerId"] = layerId;
  response["params"]["clip"] = clip;
  devtool_mediator->SendCDPEvent(response);
}

void InspectorTasmExecutor::CompositingReasons(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value compositingReasons(Json::ValueType::arrayValue);
  Json::Value compositingReasonsIds(Json::ValueType::arrayValue);
  Json::Value params = message["params"];
  int layerId = std::stoi(params["layerId"].asString());
  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");
  lynx::tasm::Element* element = GetElementById(layerId);

  if (element) {
    compositingReasons.append(ElementInspector::LocalName(element));
    compositingReasonsIds.append(ElementInspector::NodeId(element));
  }
  content["compositingReasons"] = compositingReasons;
  content["compositingReasonsIds"] = compositingReasonsIds;
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  devtool_mediator->SendCDPEvent(response);
}

Json::Value InspectorTasmExecutor::GetLayerContentFromElement(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    lynx::tasm::Element* element) {
  Json::Value layer(Json::ValueType::objectValue);
  if (element) {
    layer["layerId"] = std::to_string(ElementInspector::NodeId(element));
    layer["backendNodeId"] = ElementInspector::NodeId(element);
    if (element->parent()) {
      layer["parentLayerId"] =
          std::to_string(ElementInspector::NodeId(element->parent()));
    }
    layer["paintCount"] = 1;
    layer["drawsContent"] = true;
    layer["invisible"] = true;
    layer["name"] = ElementInspector::LocalName(element);
    Json::Value layout = GetLayoutInfoFromElement(sender, element);
    layer["offsetX"] = layout["offsetX"];
    layer["offsetY"] = layout["offsetY"];
    layer["width"] = layout["width"];
    layer["height"] = layout["height"];
  }
  return layer;
}

Json::Value InspectorTasmExecutor::GetLayoutInfoFromElement(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    lynx::tasm::Element* element) {
  Json::Value layout(Json::ValueType::objectValue);
  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", layout);
  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN_VALUE(devtool_mediator, "devtool_mediator is null",
                                  layout);
  std::vector<double> box_model = GetBoxModel(element);
  if (!box_model.empty()) {
    layout["width"] = box_model[28] - box_model[26];
    layout["height"] = box_model[31] - box_model[29];
    if (element->parent() == nullptr) {
      layout["offsetX"] = box_model[26];
      layout["offsetY"] = box_model[27];
    } else {
      std::vector<double> parent_box_model = GetBoxModel(element->parent());
      if (parent_box_model.empty()) {
        layout["offsetX"] = box_model[26];
        layout["offsetY"] = box_model[27];
      } else {
        layout["offsetX"] = box_model[26] - parent_box_model[26];
        layout["offsetY"] = box_model[27] - parent_box_model[27];
      }
    }
  }
  return layout;
}

Json::Value InspectorTasmExecutor::BuildLayerTreeFromElement(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    lynx::tasm::Element* root_element) {
  Json::Value layers(Json::ValueType::arrayValue);
  CHECK_NULL_AND_LOG_RETURN_VALUE(root_element, "root_element is null", layers);
  std::queue<lynx::tasm::Element*> element_queue;
  element_queue.push(root_element);
  while (!element_queue.empty()) {
    lynx::tasm::Element* element = element_queue.front();
    element_queue.pop();
    Json::Value layer = GetLayerContentFromElement(sender, element);
    layers.append(layer);
    for (auto& child : element->GetChildren()) {
      element_queue.push(child);
    }
  }
  return layers;
}

std::string InspectorTasmExecutor::GetLayoutTree(tasm::Element* element) {
  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN_VALUE(devtool_mediator, "devtool_mediator is null",
                                  "");
  LayoutNode* layout_node = devtool_mediator->GetLayoutNodeForElement(element);
  CHECK_NULL_AND_LOG_RETURN_VALUE(layout_node, "layout_node is null", "");
  return lynx::tasm::replay::ReplayController::GetLayoutTree(
      layout_node->slnode());
}

void InspectorTasmExecutor::SendLayoutTree() {
  auto root = GetElementRoot();
  if (root) {
    lynx::tasm::replay::ReplayController::SendFileByAgent("Layout",
                                                          GetLayoutTree(root));
  }
}

void InspectorTasmExecutor::PageGetResourceContent(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  content["base64Encoded"] = false;
  std::string html_content = "";
  if (element_root_ != nullptr) {
    html_content = ElementHelper::GetElementContent(element_root_, 0);
  }
  content["content"] = html_content;
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

Json::Value InspectorTasmExecutor::GetBoxModelOfNode(tasm::Element* ptr,
                                                     double screen_scale_factor,
                                                     std::string mode,
                                                     tasm::Element* root) {
  Json::Value res(Json::ValueType::objectValue);
  if (ElementInspector::IsNeedEraseId(ptr)) {
    ptr = ElementInspector::GetChildElementForComponentRemoveView(ptr);
  }
  if (ptr != nullptr && ElementInspector::HasDataModel(ptr) &&
      GetBoxModel(ptr).size() == 34) {
    Json::Value model(Json::ValueType::objectValue);
    std::vector<double> box_model = GetBoxModel(ptr);
    if (mode == DevToolStatus::SCREENSHOT_MODE_LYNXVIEW && root != nullptr) {
      std::vector<double> root_box_model = GetBoxModel(root);
      if (root_box_model.size() == 34) {
        // use lynxview's left top point of border box as origin x y of lynxview
        // point_to_lynxview = point_to_screen - lynxview_to_screen
        double origin_x = root_box_model[18];
        double origin_y = root_box_model[19];
        for (int i = 1; i <= 16; i++) {
          box_model[2 * i] -= origin_x;
          box_model[2 * i + 1] -= origin_y;
        }
      }
    }

    auto* element_manager = ptr->element_manager();
    CHECK_NULL_AND_LOG_RETURN_VALUE(element_manager, "element_manager is null",
                                    res);
    const float layouts_unit_per_px =
        element_manager->GetLynxEnvConfig().LayoutsUnitPerPx();
    model["width"] = box_model[0] / layouts_unit_per_px * screen_scale_factor;
    model["height"] = box_model[1] / layouts_unit_per_px * screen_scale_factor;
    // content
    model["content"] = Json::Value(Json::ValueType::arrayValue);
    for (int i = 2; i <= 9; ++i) {
      model["content"].append(box_model[i] / layouts_unit_per_px *
                              screen_scale_factor);
    }
    // padding
    model["padding"] = Json::Value(Json::ValueType::arrayValue);
    for (int i = 10; i <= 17; ++i) {
      model["padding"].append(box_model[i] / layouts_unit_per_px *
                              screen_scale_factor);
    }
    // border
    model["border"] = Json::Value(Json::ValueType::arrayValue);
    for (int i = 18; i <= 25; ++i) {
      model["border"].append(box_model[i] / layouts_unit_per_px *
                             screen_scale_factor);
    }
    // margin
    model["margin"] = Json::Value(Json::ValueType::arrayValue);
    for (int i = 26; i <= 33; ++i) {
      model["margin"].append(box_model[i] / layouts_unit_per_px *
                             screen_scale_factor);
    }
    res["model"] = model;
  } else {
    Json::Value error = Json::Value(Json::ValueType::objectValue);
    error["code"] = Json::Value(-32000);
    error["message"] = Json::Value("Could not compute box model.");
    res["error"] = error;
  }
  return res;
}

Json::Value InspectorTasmExecutor::GetDocumentBodyFromNodeWithBoxModel(
    tasm::Element* ptr) {
  Json::Value res = Json::Value(Json::ValueType::objectValue);

  CHECK_NULL_AND_LOG_RETURN_VALUE(ptr, "ptr is null", res);

  auto set_node_func = [this](Json::Value& res, Element* ptr) {
    ElementHelper::SetJsonValueOfNode(ptr, res);
    double screen_scale_factor = 1.0f;
    std::string screen_shot_mode = DevToolStatus::GetInstance().GetStatus(
        DevToolStatus::kDevToolStatusKeyScreenShotMode,
        DevToolStatus::SCREENSHOT_MODE_FULLSCREEN);
    Json::Value box_model = GetBoxModelOfNode(
        ptr, screen_scale_factor, screen_shot_mode, GetElementRoot());
    res["box_model"] = box_model["model"];

    res["childNodeCount"] = static_cast<int>(ptr->GetChildren().size());
    res["children"] = Json::Value(Json::ValueType::arrayValue);
    for (Element* child : ptr->GetChildren()) {
      res["children"].append(GetDocumentBodyFromNodeWithBoxModel(child));
    }
  };

  Element* comp_ptr =
      ElementInspector::GetParentComponentElementFromDataModel(ptr);

  if (comp_ptr && ElementInspector::IsNeedEraseId(comp_ptr)) {
    // when the element tree is nested component tree like as below
    // fake component
    //    --> fake component
    //          -->fake component
    //               -->  true element
    // Then after we have finished constructing the subtree with the child
    // element of the bottom-most component as the root node, we need to
    // continuously loop upwards until we find a node that is not a
    // fake component element
    set_node_func(res, ptr);

    Json::Value current_res = res;
    while (1) {
      Json::Value comp = Json::Value(Json::ValueType::objectValue);
      set_node_func(comp, comp_ptr);
      comp["childNodeCount"] = 1;
      comp["children"].append(current_res);

      comp_ptr =
          ElementInspector::GetParentComponentElementFromDataModel(comp_ptr);
      if (!comp_ptr || !ElementInspector::IsNeedEraseId(comp_ptr)) {
        return comp;
      }
      current_res = std::move(comp);
    }
  } else if (ElementInspector::Type(ptr) == InspectorElementType::COMPONENT) {
    set_node_func(res, ptr);
    if (ElementInspector::SelectorTag(ptr) == "page") {
      Json::Value doc = Json::Value(Json::ValueType::objectValue);
      set_node_func(doc, ElementInspector::DocElement(ptr));
      doc["childNodeCount"] = 1;
      doc["children"].append(res);
      return doc;
    }
  } else {
    set_node_func(res, ptr);
  }
  return res;
}

Json::Value InspectorTasmExecutor::GetComputedStyleOfNode(tasm::Element* ptr) {
  Json::Value res = Json::Value(Json::ValueType::arrayValue);
  Json::Value temp = Json::Value(Json::ValueType::objectValue);
  if (ptr != nullptr && ElementInspector::HasDataModel(ptr)) {
    auto dict = ElementInspector::GetDefaultCss();

    if (ElementInspector::IsEnableCSSSelector(ptr)) {
      const std::vector<InspectorStyleSheet>& match_rules =
          ElementInspector::GetMatchedStyleSheet(ptr);
      for (const InspectorStyleSheet& match : match_rules) {
        ReplaceDefaultComputedStyle(dict, match.css_properties_);
      }
    } else {
      ReplaceDefaultComputedStyle(
          dict,
          ElementInspector::GetStyleSheetByName(ptr, "*").css_properties_);
      ReplaceDefaultComputedStyle(
          dict,
          ElementInspector::GetStyleSheetByName(ptr, "body *").css_properties_);
      for (size_t i = 0; i < ElementInspector::ClassOrder(ptr).size(); ++i) {
        ReplaceDefaultComputedStyle(
            dict, ElementInspector::GetStyleSheetByName(
                      ptr, ElementInspector::ClassOrder(ptr)[i])
                      .css_properties_);
      }
      ReplaceDefaultComputedStyle(dict,
                                  ElementInspector::GetStyleSheetByName(
                                      ptr, ElementInspector::SelectorId(ptr))
                                      .css_properties_);
      ReplaceDefaultComputedStyle(dict,
                                  ElementInspector::GetStyleSheetByName(
                                      ptr, ElementInspector::SelectorTag(ptr))
                                      .css_properties_);
    }

    ReplaceDefaultComputedStyle(
        dict, ElementInspector::GetInlineStyleSheet(ptr).css_properties_);
    std::vector<double> box_info = GetBoxModel(ptr);
    if (box_info.size() == 34) {
      dict["width"] = lynx::tasm::CSSDecoder::ToPxValue(box_info[0]);
      dict["height"] = lynx::tasm::CSSDecoder::ToPxValue(box_info[1]);

      // clang-format off
      //margin 26-33 border 18-25 padding 10-17 content 2-9
      /* 

         (26,27)---------------------------------------------------(28,29)
            |   (18,19) ------------------------------------(20,21)   |
            |      |    (10,11)--------------------(12,13)     |      |
            |      |       |       (2,3) ------(4,5)  |        |      |
            |      |       |         |           |    |        |      |
            |      |       |         |           |    |        |      |
            |      |       |       (8,9)-------(6,7)  |        |      |
            |      |    (16,17)--------------------(14,15)     |      |
            |   (24,25)-------------------------------------(22,23)   |
         (32,33)---------------------------------------------------(30,31)

       */
      // clang-format on
      // margin
      dict["margin-left"] =
          lynx::tasm::CSSDecoder::ToPxValue(box_info[18] - box_info[26]);
      dict["margin-top"] =
          lynx::tasm::CSSDecoder::ToPxValue(box_info[19] - box_info[27]);
      dict["margin-right"] =
          lynx::tasm::CSSDecoder::ToPxValue(box_info[28] - box_info[20]);
      dict["margin-bottom"] =
          lynx::tasm::CSSDecoder::ToPxValue(box_info[33] - box_info[25]);
      if (dict["margin-left"] == dict["margin-right"] &&
          dict["margin-left"] == dict["margin-top"] &&
          dict["margin-left"] == dict["margin-bottom"]) {
        dict["margin"] = dict["margin-left"];
      } else {
        std::ostringstream margin_str;
        margin_str << dict["margin-top"] << " " << dict["margin-right"] << " "
                   << dict["margin-bottom"] << " " << dict["margin-left"];
        dict["margin"] = margin_str.str();
      }

      // border
      dict["border-left-width"] =
          lynx::tasm::CSSDecoder::ToPxValue(box_info[10] - box_info[18]);
      dict["border-right-width"] =
          lynx::tasm::CSSDecoder::ToPxValue(box_info[20] - box_info[12]);
      dict["border-top-width"] =
          lynx::tasm::CSSDecoder::ToPxValue(box_info[11] - box_info[19]);
      dict["border-bottom-width"] =
          lynx::tasm::CSSDecoder::ToPxValue(box_info[25] - box_info[17]);

      if (dict["border-left"] == dict["border-right"] &&
          dict["border-left"] == dict["border-top"] &&
          dict["border-left"] == dict["border-bottom"]) {
        dict["border"] = dict["border-left"];
      } else {
        std::ostringstream margin_str;
        margin_str << dict["border-top"] << " " << dict["border-right"] << " "
                   << dict["border-bottom"] << " " << dict["border-left"];
        dict["border"] = margin_str.str();
      }

      // padding
      dict["padding-left"] =
          lynx::tasm::CSSDecoder::ToPxValue(box_info[2] - box_info[10]);
      dict["padding-top"] =
          lynx::tasm::CSSDecoder::ToPxValue(box_info[3] - box_info[11]);
      dict["padding-right"] =
          lynx::tasm::CSSDecoder::ToPxValue(box_info[12] - box_info[4]);
      dict["padding-bottom"] =
          lynx::tasm::CSSDecoder::ToPxValue(box_info[17] - box_info[9]);

      if (dict["padding-left"] == dict["padding-right"] &&
          dict["padding-left"] == dict["padding-top"] &&
          dict["padding-left"] == dict["padding-bottom"]) {
        dict["padding"] = dict["padding-left"];
      } else {
        std::ostringstream margin_str;
        margin_str << dict["padding-top"] << " " << dict["padding-right"] << " "
                   << dict["padding-bottom"] << " " << dict["padding-left"];
        dict["border"] = margin_str.str();
      }
    }

    auto* element_manager = ptr->element_manager();
    if (element_manager) {
      dict["font-size"] = lynx::tasm::CSSDecoder::ToPxValue(
          ptr->GetFontSize() /
          element_manager->GetLynxEnvConfig().LayoutsUnitPerPx());
    }

    for (const auto& pair : dict) {
      if (pair.first != "") {
        temp["name"] = pair.first;
        if (pair.first.find("color") != std::string::npos &&
            pair.first != "-x-animation-color-interpolation" &&
            pair.first != "border-color") {
          temp["value"] =
              lynx::tasm::CSSDecoder::ToRgbaFromColorValue(pair.second);
        } else {
          temp["value"] = pair.second;
        }
        res.append(temp);
      }
    }
  }
  return res;
}

}  // namespace devtool
}  // namespace lynx
