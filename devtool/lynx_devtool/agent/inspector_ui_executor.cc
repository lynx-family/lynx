// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/inspector_ui_executor.h"

#include <cmath>
#include <limits>
#include <utility>

#include "core/renderer/dom/element_manager.h"
#include "core/runtime/lepus/json_parser.h"
#include "devtool/base_devtool/native/public/devtool_status.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
#include "devtool/lynx_devtool/element/helper_util.h"
#include "devtool/lynx_devtool/input/synthetic_gesture_controller.h"
#include "devtool/lynx_devtool/input/synthetic_tap_gesture.h"

namespace lynx {
namespace devtool {

#define BANNER ""

extern const char* kLynxLocalUrl;
extern const char* kLynxSecurityOrigin;
extern const char* kLynxMimeType;

namespace {

bool IsValidScreencastMode(const std::string& mode) {
  return mode == DevToolStatus::SCREENSHOT_MODE_FULLSCREEN ||
         mode == DevToolStatus::SCREENSHOT_MODE_LYNXVIEW;
}

constexpr int kDefaultTapDurationMs = 50;
constexpr int kDefaultTapCount = 1;
constexpr int kMaxSyntheticTapCount = 200;
constexpr int64_t kMaxSyntheticTapSequenceDurationMs = 10000;

class TapGestureResponse {
 public:
  TapGestureResponse(std::shared_ptr<lynx::devtool::MessageSender> sender,
                     int64_t id, int tap_count)
      : sender_(std::move(sender)), id_(id), remaining_(tap_count) {}

  void OnGestureResult(input::SyntheticGestureResult result) {
    if (responded_) {
      return;
    }
    if (result != input::SyntheticGestureResult::kDone) {
      responded_ = true;
      sender_->SendErrorResponse(id_, "Input.synthesizeTapGesture failed");
      return;
    }
    if (--remaining_ == 0) {
      responded_ = true;
      sender_->SendOKResponse(id_);
    }
  }

 private:
  std::shared_ptr<lynx::devtool::MessageSender> sender_;
  int64_t id_;
  int remaining_;
  bool responded_{false};
};

bool ParseFiniteFloat(const Json::Value& value, float& result) {
  if (!value.isNumeric()) {
    return false;
  }
  const double parsed = value.asDouble();
  if (!std::isfinite(parsed) ||
      parsed < -static_cast<double>(std::numeric_limits<float>::max()) ||
      parsed > static_cast<double>(std::numeric_limits<float>::max())) {
    return false;
  }
  result = static_cast<float>(parsed);
  return true;
}

const char* SourceTypeToString(input::PointerSourceType source_type) {
  switch (source_type) {
    case input::PointerSourceType::kDefault:
      return "default";
    case input::PointerSourceType::kTouch:
      return "touch";
    case input::PointerSourceType::kMouse:
      return "mouse";
  }
  return "unknown";
}

bool ParseGestureSourceType(const Json::Value& params,
                            input::PointerSourceType& source_type) {
  if (!params.isMember("gestureSourceType") ||
      params["gestureSourceType"].isNull()) {
    source_type = input::PointerSourceType::kDefault;
    return true;
  }
  if (!params["gestureSourceType"].isString()) {
    return false;
  }

  const std::string value = params["gestureSourceType"].asString();
  if (value == "default") {
    source_type = input::PointerSourceType::kDefault;
  } else if (value == "touch") {
    source_type = input::PointerSourceType::kTouch;
  } else if (value == "mouse") {
    source_type = input::PointerSourceType::kMouse;
  } else {
    return false;
  }
  return true;
}

}  // namespace

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
  if (devtool_platform_facade_ != devtool_platform_facade) {
    synthetic_gesture_target_.reset();
    synthetic_gesture_controller_.reset();
  }
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

void InspectorUIExecutor::Focus(int node_id) {
  CHECK_NULL_AND_LOG_RETURN(devtool_platform_facade_,
                            "devtool_platform_facade_ is null");
  devtool_platform_facade_->Focus(node_id);
}

void InspectorUIExecutor::PageReload(bool ignore_cache,
                                     const std::string& template_binary,
                                     const std::string& reload_url,
                                     bool from_template_fragments,
                                     int32_t template_size) {
  CHECK_NULL_AND_LOG_RETURN(devtool_platform_facade_,
                            "devtool_platform_facade_ is null");
  devtool_platform_facade_->PageReload(ignore_cache, template_binary,
                                       reload_url, from_template_fragments,
                                       template_size);
}

void InspectorUIExecutor::StartScreencast(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  ScreenshotRequest screen_request;
  if (params["format"].isString()) {
    std::string format = params["format"].asString();
    if (format == "png") {
      screen_request.format_ = format;
      screen_request.type_ = ScreenshotType::PNG;
    }
  }
  if (params["quality"].isInt()) {
    screen_request.quality_ = params["quality"].asInt();
  }
  screen_request.max_width_ = params["maxWidth"].asInt();
  screen_request.max_height_ = params["maxHeight"].asInt();
  screen_request.every_nth_frame_ = params["everyNthFrame"].asInt();
  if (params["mode"].isString()) {
    std::string mode = params["mode"].asString();
    if (IsValidScreencastMode(mode)) {
      lynx::devtool::DevToolStatus::GetInstance().SetStatus(
          lynx::devtool::DevToolStatus::kDevToolStatusKeyScreenShotMode, mode);
    }
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
  std::string reload_url = "";
  if (!params.empty()) {
    ignore_cache = params["ignoreCache"].asBool();
    template_bin = params["pageData"].asString();
    from_template_fragments = params["fromPageDataFragments"].asBool();
    template_size = params["pageDataLength"].asInt();
    reload_url = params["url"].asString();
  }

  PageReload(ignore_cache, template_bin, reload_url, from_template_fragments,
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
  if (dict.size() < 4) {
    sender->SendErrorResponse(message["id"].asInt64(),
                              "Lynx.getRectToWindow is unavailable");
    return;
  }
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

std::vector<double> InspectorUIExecutor::GetBoxModel(
    const InspectorBoxModelQuery& query) {
  CHECK_NULL_AND_LOG_RETURN_VALUE(devtool_platform_facade_,
                                  "devtool_platform_facade_ is null", {});
  return devtool_platform_facade_->GetBoxModel(query);
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

void InspectorUIExecutor::getAllPerformanceEntries(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  if (!ShellIsDestroyed()) {
    Json::Value entries;
    Json::Value result(Json::ValueType::objectValue);
    Json::Reader reader;

    lynx::lepus::Value all_performance_entries =
        shell_->GetAllPerformanceEntries();
    std::string entries_string =
        lynx::devtool::ConvertLepusValueToJsonValue(all_performance_entries);

    reader.parse(entries_string, entries);
    result["entries"] = entries;
    response["result"] = result;
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

void InspectorUIExecutor::InsertText(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];

  CHECK_NULL_AND_LOG_RETURN(devtool_platform_facade_,
                            "devtool_platform_facade_ is null");
  devtool_platform_facade_->InsertText(params["text"].asString());

  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorUIExecutor::SynthesizeTapGesture(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  const int64_t id = message["id"].asInt64();
  const Json::Value& params = message["params"];
  float x = 0.f;
  float y = 0.f;
  if (!params.isObject() || !ParseFiniteFloat(params["x"], x) ||
      !ParseFiniteFloat(params["y"], y)) {
    sender->SendErrorResponse(
        id, "Invalid params: expected finite numeric x and y");
    return;
  }

  if (!devtool_platform_facade_) {
    sender->SendErrorResponse(id, "Input target is unavailable");
    return;
  }

  auto target = devtool_platform_facade_->GetInputEventTarget();
  if (!target) {
    sender->SendErrorResponse(id,
                              "Not implemented: Input.synthesizeTapGesture");
    return;
  }

  input::PointerSourceType source_type = input::PointerSourceType::kDefault;
  if (!ParseGestureSourceType(params, source_type)) {
    sender->SendErrorResponse(
        id,
        "Invalid params: expected gestureSourceType default, touch, or mouse");
    return;
  }

  const auto capabilities = target->GetPointerCapabilities();
  if (source_type == input::PointerSourceType::kDefault) {
    source_type = capabilities.default_source_type;
  }
  if (source_type == input::PointerSourceType::kDefault ||
      !capabilities.Supports(source_type)) {
    sender->SendErrorResponse(
        id, std::string("Not implemented: Input.synthesizeTapGesture source ") +
                SourceTypeToString(source_type));
    return;
  }

  if (params.isMember("duration") &&
      (!params["duration"].isInt() || params["duration"].asInt() < 0)) {
    sender->SendErrorResponse(
        id, "Invalid params: duration must be a non-negative integer");
    return;
  }
  if (params.isMember("tapCount") &&
      (!params["tapCount"].isInt() || params["tapCount"].asInt() < 0)) {
    sender->SendErrorResponse(
        id, "Invalid params: tapCount must be a non-negative integer");
    return;
  }

  const int duration_ms = params.isMember("duration")
                              ? params["duration"].asInt()
                              : kDefaultTapDurationMs;
  const int tap_count = params.isMember("tapCount") ? params["tapCount"].asInt()
                                                    : kDefaultTapCount;
  if (tap_count == 0) {
    sender->SendOKResponse(id);
    return;
  }
  if (tap_count > kMaxSyntheticTapCount) {
    sender->SendErrorResponse(id, "Invalid params: tapCount exceeds 200");
    return;
  }

  const int64_t sequence_duration_ms =
      static_cast<int64_t>(duration_ms) * tap_count;
  if (sequence_duration_ms > kMaxSyntheticTapSequenceDurationMs) {
    sender->SendErrorResponse(
        id, "Invalid params: tap sequence duration exceeds 10000 ms");
    return;
  }

  auto devtool_mediator = devtool_mediator_wp_.lock();
  const auto task_runner =
      devtool_mediator ? devtool_mediator->GetUITaskRunner() : nullptr;
  if (!task_runner) {
    sender->SendErrorResponse(id, "Input UI task runner is unavailable");
    return;
  }

  auto controller_target = synthetic_gesture_target_.lock();
  if (!synthetic_gesture_controller_ || controller_target != target) {
    synthetic_gesture_target_ = target;
    synthetic_gesture_controller_ =
        input::SyntheticGestureController::Create(target, task_runner);
  }
  auto response = std::make_shared<TapGestureResponse>(sender, id, tap_count);
  for (int tap_index = 0; tap_index < tap_count; ++tap_index) {
    synthetic_gesture_controller_->QueueSyntheticGesture(
        std::make_unique<input::SyntheticTapGesture>(x, y, duration_ms,
                                                     source_type),
        [response](input::SyntheticGestureResult result) {
          response->OnGestureResult(result);
        });
  }
}

// end input protocol

// The following three functions are used for handling Layout Nodes
void InspectorUIExecutor::OnLayoutObjectCreated(int32_t id, SLNode* ptr) {
  layout_objects_[id] = ptr;
}

void InspectorUIExecutor::OnLayoutObjectDestroy(int32_t id) {
  layout_objects_.erase(id);
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

SLNode* InspectorUIExecutor::GetLayoutObjectById(int32_t id) {
  auto it = layout_objects_.find(id);
  if (it != layout_objects_.end()) {
    return it->second;
  }
  return nullptr;
}
// End of Layout Nodes

}  // namespace devtool
}  // namespace lynx
