// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/inspector_ui_executor.h"

#include <regex>

#include "core/renderer/dom/element_manager.h"
#include "core/runtime/vm/lepus/json_parser.h"
#include "devtool/base_devtool/native/public/devtool_status.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
#include "devtool/lynx_devtool/element/element_inspector.h"
#include "devtool/lynx_devtool/element/helper_util.h"

namespace lynx {
namespace devtool {

#define BANNER ""

extern const char* kLynxLocalUrl;
extern const char* kLynxSecurityOrigin;
extern const char* kLynxMimeType;

InspectorUIExecutor::InspectorUIExecutor(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : shell_(nullptr),
      devtool_mediator_wp_(devtool_mediator),
      uitree_use_compression_(false),
      uitree_compression_threshold_(10240) {}

InspectorUIExecutor::~InspectorUIExecutor() {
  LOGI("~InspectorUIExecutor this: " << this);
}

void InspectorUIExecutor::SetDevToolPlatformFacade(
    const std::shared_ptr<DevToolPlatformFacade>& devtool_platform_facade) {
  devtool_platform_facade_ = devtool_platform_facade;
}

void InspectorUIExecutor::SetShell(lynx::shell::LynxShell* shell) {
  shell_ = shell;
}

void InspectorUIExecutor::GetNodeForLocation(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  CHECK_NULL_AND_LOG_RETURN(shell_, "GetNodeForLocation: shell_ is null");
  const std::unique_ptr<tasm::ElementManager>& element_manager =
      shell_->GetTasm()->page_proxy()->element_manager();
  CHECK_NULL_AND_LOG_RETURN(element_manager,
                            "GetNodeForLocation: element_manager is null");
  float layouts_unit_per_px =
      element_manager->GetLynxEnvConfig().LayoutsUnitPerPx();
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  int x = params["x"].asInt();
  int y = params["y"].asInt();

  x = x * layouts_unit_per_px;
  y = y * layouts_unit_per_px;

  std::string screen_shot_mode =
      lynx::devtool::DevToolStatus::GetInstance().GetStatus(
          lynx::devtool::DevToolStatus::kDevToolStatusKeyScreenShotMode,
          lynx::devtool::DevToolStatus::SCREENSHOT_MODE_FULLSCREEN);

  CHECK_NULL_AND_LOG_RETURN(devtool_platform_facade_,
                            "devtool_platform_facade_ is null");

  int node_id =
      devtool_platform_facade_->FindNodeIdForLocation(x, y, screen_shot_mode);

  content["backendNodeId"] = node_id;
  content["nodeId"] = node_id;
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorUIExecutor::ScrollIntoView(int node_id) {
  CHECK_NULL_AND_LOG_RETURN(devtool_platform_facade_,
                            "devtool_platform_facade_ is null");
  devtool_platform_facade_->ScrollIntoView(node_id);
}

void InspectorUIExecutor::PageReload(bool ignore_cache,
                                     std::string template_binary,
                                     bool from_template_fragments,
                                     int32_t template_size) {
  CHECK_NULL_AND_LOG_RETURN(devtool_platform_facade_,
                            "devtool_platform_facade_ is null");
  devtool_platform_facade_->PageReload(ignore_cache);
}

void InspectorUIExecutor::StartScreencast(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  ScreenshotRequest screen_request;
  screen_request.format_ = params["format"].asString();
  screen_request.quality_ = params["quality"].asInt();
  screen_request.max_width_ = params["maxWidth"].asInt();
  screen_request.max_height_ = params["maxHeight"].asInt();
  screen_request.every_nth_frame_ = params["everyNthFrame"].asInt();
  if (params["mode"].isString()) {
    std::string mode = params["mode"].asString();
    lynx::devtool::DevToolStatus::GetInstance().SetStatus(
        lynx::devtool::DevToolStatus::kDevToolStatusKeyScreenShotMode, mode);
  }
  CHECK_NULL_AND_LOG_RETURN(devtool_platform_facade_,
                            "devtool_platform_facade_ is null");
  devtool_platform_facade_->StartScreenCast(std::move(screen_request));

  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorUIExecutor::StopScreencast(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  CHECK_NULL_AND_LOG_RETURN(devtool_platform_facade_,
                            "devtool_platform_facade_ is null");
  devtool_platform_facade_->StopScreenCast();
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorUIExecutor::PageEnable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  // SendWelcomeMessage
  {
    Json::Value content;
    Json::Value params;
    Json::Value message;

    auto ts = lynx::base::CurrentTimeMilliseconds();

    message["source"] = "javascript";
    message["level"] = "verbose";
    message["text"] = BANNER;
    message["timestamp"] = ts;
    params["entry"] = message;
    content["method"] = "Log.entryAdded";
    content["params"] = params;
    sender->SendMessage("CDP", content);
  }

  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorUIExecutor::PageCanEmulate(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  content["result"] = true;
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorUIExecutor::PageCanScreencast(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  content["result"] = true;
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorUIExecutor::PageGetResourceTree(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value frameTree(Json::ValueType::objectValue);
  frameTree["frame"] = Json::ValueType::objectValue;
  frameTree["frame"]["url"] = kLynxLocalUrl;
  frameTree["frame"]["securityOrigin"] = kLynxSecurityOrigin;
  frameTree["frame"]["mimeType"] = kLynxMimeType;
  frameTree["resources"] = Json::ValueType::arrayValue;
  content["frameTree"] = frameTree;
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);

  auto devtool_mediator = devtool_mediator_wp_.lock();
  if (devtool_mediator) {
    devtool_mediator->SetRuntimeEnableNeeded(true);
  }
}

void InspectorUIExecutor::PageReload(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];

  bool ignore_cache = false;
  std::string template_bin = "";
  bool from_template_fragments = false;
  int32_t template_size = 0;
  if (!params.empty()) {
    ignore_cache = params["ignoreCache"].asBool();
    template_bin = params["pageData"].asString();
    from_template_fragments = params["fromPageDataFragments"].asBool();
    template_size = params["pageDataLength"].asInt();
  }

  PageReload(ignore_cache, std::move(template_bin), from_template_fragments,
             template_size);
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorUIExecutor::PageNavigate(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  auto url = params["url"].asString();
  content["loaderId"] = "";
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
  if (url == "about:blank") {
    SendPageFrameNavigatedEvent(url);
  } else {
    CHECK_NULL_AND_LOG_RETURN(devtool_platform_facade_,
                              "devtool_platform_facade_ is null");
    devtool_platform_facade_->Navigate(url);
  }
}

void InspectorUIExecutor::UITree_Enable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value params = message["params"];
  if (params.isMember("useCompression")) {
    uitree_use_compression_ = params["useCompression"].asBool();
  }
  if (params.isMember("compressionThreshold")) {
    uitree_compression_threshold_ = params["compressionThreshold"].asBool();
  }
  uitree_enabled_ = true;

  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorUIExecutor::UITree_Disable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  uitree_enabled_ = false;
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorUIExecutor::GetLynxUITree(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (!uitree_enabled_) return;
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content = Json::Value(Json::ValueType::objectValue);
  CHECK_NULL_AND_LOG_RETURN(devtool_platform_facade_,
                            "devtool_platform_facade_ is null");
  std::string tree_str = devtool_platform_facade_->GetLynxUITree();

  Json::Value tree;
  Json::Reader reader;
  if (tree_str.size()) {
    reader.parse(tree_str, tree, false);
  }
  content["root"] = tree;
  content["compress"] = false;

  response["result"] = content;
  response["id"] = message["id"].asInt64();

  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");
  devtool_mediator->RunOnDevToolThread(
      [sender, self = shared_from_this(), content, response]() mutable {
        std::string root_str = content["root"].toStyledString();
        if (self->uitree_use_compression_ &&
            root_str.size() >
                static_cast<size_t>(self->uitree_compression_threshold_)) {
          InspectorUtil::CompressData("getLynxUITree", root_str, content,
                                      "root");
        }
        response["result"] = content;
        sender->SendMessage("CDP", response);
      },
      true);
}

void InspectorUIExecutor::GetUIInfoForNode(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (!uitree_enabled_) return;
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content = Json::Value(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  int id = static_cast<int>(params["UINodeId"].asInt64());
  CHECK_NULL_AND_LOG_RETURN(devtool_platform_facade_,
                            "devtool_platform_facade_ is null");
  std::string info_str = devtool_platform_facade_->GetUINodeInfo(id);

  Json::Reader reader;
  if (info_str.size()) {
    reader.parse(info_str, content, false);
  }

  response["id"] = message["id"].asInt64();
  response["result"] = content;

  sender->SendMessage("CDP", response);
}

void InspectorUIExecutor::SetUIStyle(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (!uitree_enabled_) return;
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  int id = static_cast<int>(params["UINodeId"].asInt64());
  std::string style_name = params["styleName"].asString();
  std::string style_content = params["styleContent"].asString();
  CHECK_NULL_AND_LOG_RETURN(devtool_platform_facade_,
                            "devtool_platform_facade_ is null");
  int ret = devtool_platform_facade_->SetUIStyle(id, style_name, style_content);

  if (ret == -1) {
    Json::Value error = Json::Value(Json::ValueType::objectValue);
    error["code"] = Json::Value(-32000);
    error["message"] = Json::Value("set ui style fail");
    content["error"] = error;
  }

  response["id"] = message["id"].asInt64();
  response["result"] = content;
  sender->SendMessage("CDP", response);
}

void InspectorUIExecutor::ScreencastFrameAck(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  response["result"] = content;
  response["id"] = message["id"].asInt64();

  CHECK_NULL_AND_LOG_RETURN(devtool_platform_facade_,
                            "devtool_platform_facade_ is null");
  devtool_platform_facade_->OnAckReceived();

  sender->SendMessage("CDP", response);
}

void InspectorUIExecutor::GetScreenshot(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  CHECK_NULL_AND_LOG_RETURN(devtool_platform_facade_,
                            "devtool_platform_facade_ is null");
  devtool_platform_facade_->GetLynxScreenShot();
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content = Json::Value(Json::ValueType::objectValue);
}

void InspectorUIExecutor::LynxGetRectToWindow(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value rect(Json::ValueType::objectValue);
  CHECK_NULL_AND_LOG_RETURN(devtool_platform_facade_,
                            "devtool_platform_facade_ is null");
  auto dict = devtool_platform_facade_->GetRectToWindow();
  rect["left"] = dict[0];
  rect["top"] = dict[1];
  rect["width"] = dict[2];
  rect["height"] = dict[3];
  response["result"] = rect;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorUIExecutor::LynxTransferData(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value params = message["params"];
  if (params.empty()) {
    return;
  }

  Json::Value data_type = params["dataType"];
  if (!data_type.empty() && !data_type.asString().compare("template")) {
    Json::Value data = params["data"];
    Json::Value eof = params["eof"];
    if (data.isString() && eof.isBool()) {
      CHECK_NULL_AND_LOG_RETURN(devtool_platform_facade_,
                                "devtool_platform_facade_ is null");
      devtool_platform_facade_->OnReceiveTemplateFragment(data.asString(),
                                                          eof.asBool());
    }
  }
}

void InspectorUIExecutor::LynxGetViewLocationOnScreen(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  CHECK_NULL_AND_LOG_RETURN(devtool_platform_facade_,
                            "devtool_platform_facade_ is null");
  std::vector<int32_t> res =
      devtool_platform_facade_->GetViewLocationOnScreen();
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content = Json::Value(Json::ValueType::objectValue);
  const int invalid_pos = -1;
  if (res.size() < 2) {
    content["x"] = invalid_pos;
    content["y"] = invalid_pos;
  } else {
    content["x"] = res[0];
    content["y"] = res[1];
  }
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorUIExecutor::SendPageScreencastFrameEvent(
    const std::string& data, std::shared_ptr<ScreenMetadata> metadata) {
  Json::Value metadata_json;
  Json::Value params;
  Json::Value event;

  metadata_json["offsetTop"] = metadata->offset_top_;
  metadata_json["pageScaleFactor"] = metadata->page_scale_factor_;
  metadata_json["deviceWidth"] = metadata->device_width_;
  metadata_json["deviceHeight"] = metadata->device_height_;
  metadata_json["scrollOffsetX"] = metadata->scroll_off_set_x_;
  metadata_json["scrollOffsetY"] = metadata->scroll_off_set_y_;
  metadata_json["timestamp"] = metadata->timestamp_;

  params["data"] = data;
  params["metadata"] = metadata_json;
  event["method"] = "Page.screencastFrame";
  event["params"] = params;

  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");
  devtool_mediator->SendCDPEvent(event);
}

void InspectorUIExecutor::SendPageScreencastVisibilityChangedEvent(
    bool status) {
  Json::Value event;
  event["method"] = "Page.screencastVisibilityChanged";
  event["params"] = Json::Value(Json::ValueType::objectValue);
  event["params"]["visible"] = status;
  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");
  devtool_mediator->SendCDPEvent(event);
}

void InspectorUIExecutor::SendPageFrameNavigatedEvent(const std::string& url) {
  Json::Value event;
  event["method"] = "Page.frameNavigated";
  event["params"] = Json::ValueType::objectValue;
  event["params"]["frame"] = Json::ValueType::objectValue;
  event["params"]["frame"]["url"] = url;
  event["params"]["frame"]["id"] = "";
  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");
  devtool_mediator->SendCDPEvent(event);
}

void InspectorUIExecutor::SendLynxScreenshotCapturedEvent(
    const std::string& data) {
  Json::Value params;
  Json::Value event;

  params["data"] = data;

  event["params"] = params;
  event["method"] = "Lynx.screenshotCaptured";

  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");
  devtool_mediator->SendCDPEvent(event);
}

std::vector<double> InspectorUIExecutor::GetBoxModel(tasm::Element* element) {
  // Forward the request to get the box model for the given element
  // to the platform-specific implementation
  CHECK_NULL_AND_LOG_RETURN_VALUE(devtool_platform_facade_,
                                  "devtool_platform_facade_ is null", {});
  return devtool_platform_facade_->GetBoxModel(element);
}

void InspectorUIExecutor::LynxSendEventToVM(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value params = message["params"];
  if (!params.empty()) {
    Json::Value vm_type = params["vmType"];
    Json::Value event_name = params["event"];
    Json::Value data = params["data"];
    if (vm_type.isString() && event_name.isString()) {
      CHECK_NULL_AND_LOG_RETURN(devtool_platform_facade_,
                                "devtool_platform_facade_ is null");
      devtool_platform_facade_->SendEventToVM(
          vm_type.asString(), event_name.asString(),
          data.isString() ? data.asString() : "");
    }
  }
  sender->SendOKResponse(message["id"].asInt64());
}

void InspectorUIExecutor::TemplateGetTemplateData(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value result(Json::ValueType::objectValue);

  CHECK_NULL_AND_LOG_RETURN(devtool_platform_facade_,
                            "devtool_platform_facade_ is null");
  lynx::lepus::Value* value =
      devtool_platform_facade_->GetLepusValueFromTemplateData();
  if (value != nullptr) {
    std::string template_data_str = lynx::lepus::lepusValueToString(*value);
    result["content"] = template_data_str;
  }

  response["result"] = result;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorUIExecutor::TemplateGetTemplateJsInfo(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value result(Json::ValueType::objectValue);
  const auto& params = message["params"];
  const auto id = message["id"].asInt();
  if (params.isMember("offset") && params.isMember("size")) {
    const uint32_t offset = params["offset"].asUInt();
    const uint32_t size = params["size"].asUInt();
    CHECK_NULL_AND_LOG_RETURN(devtool_platform_facade_,
                              "devtool_platform_facade_ is null");
    std::string content =
        devtool_platform_facade_->GetTemplateJsInfo(offset, size);
    result["data"] = content;
    response["result"] = result;
    response["id"] = id;
    sender->SendMessage("CDP", response);
  } else {
    sender->SendErrorResponse(id,
                              "Params must have offset and size properties");
  }
}

// start performance protocol
void InspectorUIExecutor::PerformanceEnable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
  performance_ready_ = true;
  LOGI("performance_ready_ : " << performance_ready_);
}

void InspectorUIExecutor::PerformanceDisable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  performance_ready_ = false;
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
  LOGI("performance_ready_ : " << performance_ready_);
}

void InspectorUIExecutor::getAllTimingInfo(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  if (!ShellIsDestroyed()) {
    Json::Value value;
    Json::Reader reader;

    lynx::lepus::Value timing_info = shell_->GetAllTimingInfo();
    std::string timing_info_string =
        lynx::devtool::ConvertLepusValueToJsonValue(timing_info);

    reader.parse(timing_info_string, value);
    response["result"] = value;
  }
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

// end performance protocol

// start input protocol
void InspectorUIExecutor::EmulateTouchFromMouseEvent(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];

  std::shared_ptr<MouseEvent> input = std::make_shared<MouseEvent>();
  input->button_ = params["button"].asString();
  input->click_count_ = params["clickCount"].asInt();
  input->delta_x_ = params["deltaX"].asFloat();
  input->delta_y_ = params["deltaY"].asFloat();
  input->modifiers_ = params["modifiers"].asInt();
  input->type_ = params["type"].asString();
  input->x_ = params["x"].asInt();
  input->y_ = params["y"].asInt();
  devtool_platform_facade_->EmulateTouch(input);
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

// end input protocol

// The following three functions are used for handling Layout Nodes
void InspectorUIExecutor::OnLayoutNodeCreated(int32_t id,
                                              tasm::LayoutNode* ptr) {
  layout_nodes[id] = ptr;
}

void InspectorUIExecutor::OnLayoutNodeDestroy(int32_t id) {
  layout_nodes.erase(id);
}

void InspectorUIExecutor::OnComponentUselessUpdate(
    const std::string& component_name, const lepus::Value& properties) {
  Json::Value result(Json::ValueType::objectValue);
  result["componentName"] = component_name;
  std::ostringstream s;
  properties.PrintValue(s);
  result["properties"] = s.str();
  Json::Value msg(Json::ValueType::objectValue);
  msg["method"] = "Component.uselessUpdate";
  msg["params"] = result;

  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");
  devtool_mediator->SendCDPEvent(msg);
}

tasm::LayoutNode* InspectorUIExecutor::GetLayoutNodeForElement(
    lynx::tasm::Element* element) {
  // IsDecoupleLayoutNode is an AB switch, in next version(Maybe 2.13), the Else
  // branch can be deleted.
  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", nullptr);
  return GetLayoutNodeById(element->impl_id());
}

tasm::LayoutNode* InspectorUIExecutor::GetLayoutNodeById(int32_t id) {
  auto it = layout_nodes.find(id);
  if (it != layout_nodes.end()) {
    return it->second;
  }
  return nullptr;
}
// End of Layout Nodes

}  // namespace devtool
}  // namespace lynx
