// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/inspector_ui_executor.h"

#include <utility>

#include "base/include/fml/task_runner.h"
#include "base/include/log/logging.h"
#include "core/renderer/dom/element_manager.h"
#include "core/runtime/lepus/json_parser.h"
#include "devtool/base_devtool/native/public/cdp_param_utils.h"
#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/base_devtool/native/public/devtool_status.h"
#include "devtool/lynx_devtool/agent/input_request_handler.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
#include "devtool/lynx_devtool/element/helper_util.h"

namespace lynx {
namespace devtool {

#define BANNER ""

extern const char* kLynxLocalUrl;
extern const char* kLynxSecurityOrigin;
extern const char* kLynxMimeType;

namespace {

bool IsValidScreencastFormat(const std::string& format) {
  return format == "jpeg" || format == "png";
}

bool IsValidScreencastMode(const std::string& mode) {
  return mode == DevToolStatus::SCREENSHOT_MODE_FULLSCREEN ||
         mode == DevToolStatus::SCREENSHOT_MODE_LYNXVIEW;
}

}  // namespace

InspectorUIExecutor::InspectorUIExecutor(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_wp_(devtool_mediator),
      input_request_handler_(
          std::make_unique<InputRequestHandler>(devtool_mediator)) {}

InspectorUIExecutor::~InspectorUIExecutor() {
  LOGI("~InspectorUIExecutor this: " << this);
}

void InspectorUIExecutor::RunOnUIThreadOrNow(lynx::base::closure task) {
  auto mediator = devtool_mediator_wp_.lock();
  const auto task_runner = mediator ? mediator->GetUITaskRunner() : nullptr;
  if (task_runner) {
    fml::TaskRunner::RunNowOrPostTask(task_runner, std::move(task));
  } else {
    task();
  }
}

void InspectorUIExecutor::SetDevToolPlatformFacade(
    const std::shared_ptr<DevToolPlatformFacade>& devtool_platform_facade) {
  devtool_platform_facade_ = devtool_platform_facade;
  // The facade is stored synchronously; only the gesture controller reset that
  // a facade change triggers needs to run on the UI thread.
  if (input_request_handler_->SetDevToolPlatformFacade(
          devtool_platform_facade)) {
    auto self = shared_from_this();
    RunOnUIThreadOrNow([self]() { self->input_request_handler_->Reset(); });
  }
}

void InspectorUIExecutor::ResetInputHandler() {
  auto self = shared_from_this();
  RunOnUIThreadOrNow([self]() { self->input_request_handler_->Reset(); });
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
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  ScreenshotRequest screen_request;
  auto& format = screen_request.format_;
  auto& quality = screen_request.quality_;
  int max_width = 0;
  int max_height = 0;
  auto& every_nth_frame = screen_request.every_nth_frame_;
  std::string mode;
  if ((params.isMember("format") &&
       !ReadStringParam(params["format"], format)) ||
      (params.isMember("quality") &&
       !ReadIntParam(params["quality"], quality)) ||
      (params.isMember("maxWidth") &&
       !ReadIntParam(params["maxWidth"], max_width)) ||
      (params.isMember("maxHeight") &&
       !ReadIntParam(params["maxHeight"], max_height)) ||
      (params.isMember("everyNthFrame") &&
       !ReadIntParam(params["everyNthFrame"], every_nth_frame)) ||
      (params.isMember("mode") && !ReadStringParam(params["mode"], mode))) {
    responder->SendError(CDPErrorCode::InvalidParams,
                         "Invalid screencast parameter type");
    return;
  }
  if (!IsValidScreencastFormat(format)) {
    responder->SendError(CDPErrorCode::InvalidParams,
                         "Invalid format: expected jpeg or png");
    return;
  }
  if (quality < 0 || quality > 100) {
    responder->SendError(CDPErrorCode::InvalidParams,
                         "Invalid quality: expected integer from 0 to 100");
    return;
  }
  if (max_width < 0 || max_height < 0) {
    responder->SendError(CDPErrorCode::InvalidParams,
                         "Invalid dimensions: expected non-negative integers");
    return;
  }
  if (params.isMember("everyNthFrame") && every_nth_frame <= 0) {
    responder->SendError(CDPErrorCode::InvalidParams,
                         "Invalid everyNthFrame: expected positive integer");
    return;
  }
  if (params.isMember("mode") && !IsValidScreencastMode(mode)) {
    responder->SendError(CDPErrorCode::InvalidParams,
                         "Invalid mode: expected fullscreen or lynxview");
    return;
  }
  if (devtool_platform_facade_ == nullptr) {
    responder->SendError(CDPErrorCode::ServerError,
                         "Page target is unavailable");
    return;
  }

  screen_request.type_ =
      format == "png" ? ScreenshotType::PNG : ScreenshotType::JPEG;
  screen_request.max_width_ = static_cast<size_t>(max_width);
  screen_request.max_height_ = static_cast<size_t>(max_height);
  if (!mode.empty()) {
    DevToolStatus::GetInstance().SetStatus(
        DevToolStatus::kDevToolStatusKeyScreenShotMode, mode);
  }
  devtool_platform_facade_->StartScreenCast(std::move(screen_request));
  responder->SendSuccess();
}

void InspectorUIExecutor::StopScreencast(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value&) {
  if (devtool_platform_facade_ == nullptr) {
    responder->SendError(CDPErrorCode::ServerError,
                         "Page target is unavailable");
    return;
  }
  devtool_platform_facade_->StopScreenCast();
  responder->SendSuccess();
}

void InspectorUIExecutor::PageEnable(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value&) {
  // SendWelcomeMessage
  Json::Value content;
  Json::Value params;
  Json::Value message;

  auto ts = lynx::base::CurrentTimeMilliseconds();

  message["source"] = "javascript";
  message["level"] = "verbose";
  message["text"] = BANNER;
  message["timestamp"] = ts;
  params["entry"] = std::move(message);
  content["method"] = "Log.entryAdded";
  content["params"] = std::move(params);
  auto devtool_mediator = devtool_mediator_wp_.lock();
  if (devtool_mediator) {
    devtool_mediator->SendCDPEvent(content);
  }
  responder->SendSuccess();
}

void InspectorUIExecutor::PageCanEmulate(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value&) {
  Json::Value result(Json::ValueType::objectValue);
  result["result"] = true;
  responder->SendSuccess(std::move(result));
}

void InspectorUIExecutor::PageCanScreencast(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value&) {
  Json::Value result(Json::ValueType::objectValue);
  result["result"] = true;
  responder->SendSuccess(std::move(result));
}

void InspectorUIExecutor::PageGetResourceTree(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value&) {
  Json::Value result(Json::ValueType::objectValue);
  Json::Value frameTree(Json::ValueType::objectValue);
  frameTree["frame"] = Json::ValueType::objectValue;
  frameTree["frame"]["url"] = kLynxLocalUrl;
  frameTree["frame"]["securityOrigin"] = kLynxSecurityOrigin;
  frameTree["frame"]["mimeType"] = kLynxMimeType;
  frameTree["resources"] = Json::ValueType::arrayValue;
  result["frameTree"] = std::move(frameTree);
  responder->SendSuccess(std::move(result));

  auto devtool_mediator = devtool_mediator_wp_.lock();
  if (devtool_mediator) {
    devtool_mediator->SetRuntimeEnableNeeded(true);
  }
}

void InspectorUIExecutor::PageReload(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  if (devtool_platform_facade_ == nullptr) {
    responder->SendError(CDPErrorCode::ServerError,
                         "Page target is unavailable");
    return;
  }

  if (!params.empty()) {
    bool ignore_cache = false;
    std::string template_binary;
    bool from_template_fragments = false;
    int template_size = 0;
    std::string reload_url;
    if ((params.isMember("ignoreCache") &&
         !ReadBoolParam(params["ignoreCache"], ignore_cache)) ||
        (params.isMember("pageData") &&
         !ReadStringParam(params["pageData"], template_binary)) ||
        (params.isMember("fromPageDataFragments") &&
         !ReadBoolParam(params["fromPageDataFragments"],
                        from_template_fragments)) ||
        (params.isMember("pageDataLength") &&
         (!ReadIntParam(params["pageDataLength"], template_size) ||
          template_size < 0)) ||
        (params.isMember("url") &&
         !ReadStringParam(params["url"], reload_url))) {
      responder->SendError(CDPErrorCode::InvalidParams,
                           "Invalid reload parameters");
      return;
    }
    PageReload(ignore_cache, template_binary, reload_url,
               from_template_fragments, template_size);
  } else {
    PageReload(false);
  }
  responder->SendSuccess();
}

void InspectorUIExecutor::PageNavigate(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  std::string url;
  if (!ReadStringParam(params["url"], url) || url.empty()) {
    responder->SendError(CDPErrorCode::InvalidParams,
                         "Invalid url: expected non-empty string");
    return;
  }

  Json::Value result(Json::ValueType::objectValue);
  result["frameId"] = "";
  if (url == "about:blank") {
    responder->SendSuccess(std::move(result));
    SendPageFrameNavigatedEvent(url);
    return;
  }
  if (devtool_platform_facade_ == nullptr) {
    responder->SendError(CDPErrorCode::ServerError,
                         "Page target is unavailable");
    return;
  }
  devtool_platform_facade_->Navigate(url);
  responder->SendSuccess(std::move(result));
}

void InspectorUIExecutor::UITree_Enable(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  bool use_compression = uitree_use_compression_;
  int compression_threshold = uitree_compression_threshold_;
  if (params.isMember("useCompression")) {
    if (!ReadBoolParam(params["useCompression"], use_compression)) {
      responder->SendError(CDPErrorCode::InvalidParams,
                           "Invalid useCompression: expected boolean");
      return;
    }
  }
  if (params.isMember("compressionThreshold")) {
    if (!ReadIntParam(params["compressionThreshold"], compression_threshold) ||
        compression_threshold < 0) {
      responder->SendError(
          CDPErrorCode::InvalidParams,
          "Invalid compressionThreshold: expected non-negative integer");
      return;
    }
  }
  uitree_use_compression_ = use_compression;
  uitree_compression_threshold_ = compression_threshold;
  uitree_enabled_ = true;
  responder->SendSuccess();
}

void InspectorUIExecutor::UITree_Disable(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value&) {
  uitree_enabled_ = false;
  responder->SendSuccess();
}

void InspectorUIExecutor::GetLynxUITree(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value&) {
  if (!uitree_enabled_) {
    responder->SendError(CDPErrorCode::ServerError, "UITree is not enabled");
    return;
  }
  if (devtool_platform_facade_ == nullptr) {
    responder->SendError(CDPErrorCode::ServerError,
                         "UITree target is unavailable");
    return;
  }
  std::string tree_str = devtool_platform_facade_->GetLynxUITree();
  const bool use_compression = uitree_use_compression_;
  const int compression_threshold = uitree_compression_threshold_;

  auto devtool_mediator = devtool_mediator_wp_.lock();
  if (devtool_mediator == nullptr) {
    responder->SendError(CDPErrorCode::ServerError,
                         "UITree target is unavailable");
    return;
  }
  // The platform facade is UI-thread-affine. Parse, format, and compress large
  // UITrees on the DevTool thread to avoid blocking the UI thread.
  if (!devtool_mediator->RunOnDevToolThread(
          [responder, tree_str = std::move(tree_str), use_compression,
           compression_threshold]() mutable {
            Json::Value result(Json::ValueType::objectValue);
            Json::Value tree;
            Json::Reader reader;
            if (!tree_str.empty() && !reader.parse(tree_str, tree, false)) {
              responder->SendError(CDPErrorCode::InternalError,
                                   "Invalid UITree data");
              return;
            }
            result["root"] = std::move(tree);
            result["compress"] = false;
            std::string root_str = result["root"].toStyledString();
            if (use_compression &&
                root_str.size() > static_cast<size_t>(compression_threshold)) {
              InspectorUtil::CompressData("getLynxUITree", root_str, result,
                                          "root");
            }
            responder->SendSuccess(std::move(result));
          },
          true)) {
    responder->SendError(CDPErrorCode::ServerError,
                         "UITree target is unavailable");
  }
}

void InspectorUIExecutor::GetUIInfoForNode(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  if (!uitree_enabled_) {
    responder->SendError(CDPErrorCode::ServerError, "UITree is not enabled");
    return;
  }
  int id = 0;
  if (!ReadIntParam(params["UINodeId"], id)) {
    responder->SendError(CDPErrorCode::InvalidParams,
                         "Invalid UINodeId: expected integer");
    return;
  }
  if (devtool_platform_facade_ == nullptr) {
    responder->SendError(CDPErrorCode::ServerError,
                         "UITree target is unavailable");
    return;
  }
  std::string info_str = devtool_platform_facade_->GetUINodeInfo(id);

  Json::Value result(Json::ValueType::objectValue);
  Json::Reader reader;
  if (!info_str.empty() && !reader.parse(info_str, result, false)) {
    responder->SendError(CDPErrorCode::InternalError, "Invalid UI node data");
    return;
  }
  responder->SendSuccess(std::move(result));
}

void InspectorUIExecutor::SetUIStyle(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  if (!uitree_enabled_) {
    responder->SendError(CDPErrorCode::ServerError, "UITree is not enabled");
    return;
  }
  int id = 0;
  std::string style_name;
  std::string style_content;
  if (!ReadIntParam(params["UINodeId"], id) ||
      !ReadStringParam(params["styleName"], style_name) ||
      !ReadStringParam(params["styleContent"], style_content)) {
    responder->SendError(
        CDPErrorCode::InvalidParams,
        "Invalid params: expected integer UINodeId and string style values");
    return;
  }
  if (devtool_platform_facade_ == nullptr) {
    responder->SendError(CDPErrorCode::ServerError,
                         "UITree target is unavailable");
    return;
  }
  int ret = devtool_platform_facade_->SetUIStyle(id, style_name, style_content);

  if (ret == -1) {
    responder->SendError(CDPErrorCode::ServerError, "Failed to set UI style");
    return;
  }

  responder->SendSuccess();
}

void InspectorUIExecutor::ScreencastFrameAck(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value&) {
  if (devtool_platform_facade_ == nullptr) {
    responder->SendError(CDPErrorCode::ServerError,
                         "Page target is unavailable");
    return;
  }
  devtool_platform_facade_->OnAckReceived();
  responder->SendSuccess();
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

void InspectorUIExecutor::LynxSetLogLevel(
    const std::shared_ptr<CDPResponder>& responder, int level) {
  base::logging::SetPlatformMinLogLevel(level);
  responder->SendSuccess();
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

// start template protocol
void InspectorUIExecutor::TemplateGetTemplateData(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  if (devtool_platform_facade_ == nullptr) {
    responder->SendError(CDPErrorCode::ServerError,
                         "Template target is unavailable");
    return;
  }
  Json::Value result(Json::ValueType::objectValue);
  lynx::lepus::Value* value =
      devtool_platform_facade_->GetLepusValueFromTemplateData();
  if (value != nullptr) {
    std::string template_data_str = lynx::lepus::lepusValueToString(*value);
    result["content"] = std::move(template_data_str);
  }
  responder->SendSuccess(std::move(result));
}

void InspectorUIExecutor::TemplateGetTemplateJsInfo(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  if (devtool_platform_facade_ == nullptr) {
    responder->SendError(CDPErrorCode::ServerError,
                         "Template target is unavailable");
    return;
  }
  int offset = 0;
  int size = 0;
  if (!ReadIntParam(params["offset"], offset) || offset < 0 ||
      !ReadIntParam(params["size"], size) || size < 0) {
    responder->SendError(
        CDPErrorCode::InvalidParams,
        "Invalid params: offset and size must be non-negative integers");
    return;
  }
  Json::Value result(Json::ValueType::objectValue);
  result["data"] = devtool_platform_facade_->GetTemplateJsInfo(offset, size);
  responder->SendSuccess(std::move(result));
}

// end template protocol

// start performance protocol
void InspectorUIExecutor::PerformanceEnable(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value&) {
  performance_ready_ = true;
  responder->SendSuccess();
  LOGI("performance_ready_ : " << performance_ready_);
}

void InspectorUIExecutor::PerformanceDisable(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value&) {
  performance_ready_ = false;
  responder->SendSuccess();
  LOGI("performance_ready_ : " << performance_ready_);
}

void InspectorUIExecutor::getAllTimingInfo(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value&) {
  if (ShellIsDestroyed()) {
    responder->SendSuccess();
    return;
  }

  Json::Value result;
  Json::Reader reader;
  lynx::lepus::Value timing_info = shell_->GetAllTimingInfo();
  std::string timing_info_string = ConvertLepusValueToJsonValue(timing_info);
  reader.parse(timing_info_string, result);
  responder->SendSuccess(std::move(result));
}

void InspectorUIExecutor::getAllPerformanceEntries(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value&) {
  if (ShellIsDestroyed()) {
    responder->SendSuccess();
    return;
  }

  Json::Value entries;
  Json::Value result(Json::ValueType::objectValue);
  Json::Reader reader;
  lynx::lepus::Value all_performance_entries =
      shell_->GetAllPerformanceEntries();
  std::string entries_string =
      ConvertLepusValueToJsonValue(all_performance_entries);
  reader.parse(entries_string, entries);
  result["entries"] = std::move(entries);
  responder->SendSuccess(std::move(result));
}

// end performance protocol

// start input protocol
void InspectorUIExecutor::EmulateTouchFromMouseEvent(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  input_request_handler_->EmulateTouchFromMouseEvent(responder, params);
}

void InspectorUIExecutor::DispatchMouseEvent(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  input_request_handler_->DispatchMouseEvent(responder, params);
}

void InspectorUIExecutor::InsertText(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  input_request_handler_->InsertText(responder, params);
}

void InspectorUIExecutor::SynthesizeTapGesture(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  input_request_handler_->SynthesizeTapGesture(responder, params);
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
