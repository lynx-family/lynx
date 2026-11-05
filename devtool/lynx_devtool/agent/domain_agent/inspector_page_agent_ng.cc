// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_page_agent_ng.h"

#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace devtool {

InspectorPageAgentNG::InspectorPageAgentNG(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_(devtool_mediator) {
  functions_map_["Page.enable"] = &InspectorPageAgentNG::Enable;
  functions_map_["Page.canEmulate"] = &InspectorPageAgentNG::CanEmulate;
  functions_map_["Page.canScreencast"] = &InspectorPageAgentNG::CanScreencast;
  functions_map_["Page.getResourceTree"] =
      &InspectorPageAgentNG::GetResourceTree;
  functions_map_["Page.getResourceContent"] =
      &InspectorPageAgentNG::GetResourceContent;
  functions_map_["Page.startScreencast"] =
      &InspectorPageAgentNG::StartScreencast;
  functions_map_["Page.stopScreencast"] = &InspectorPageAgentNG::StopScreencast;
  functions_map_["Page.screencastFrameAck"] =
      &InspectorPageAgentNG::ScreencastFrameAck;
  functions_map_["Page.reload"] = &InspectorPageAgentNG::Reload;
  functions_map_["Page.navigate"] = &InspectorPageAgentNG::Navigate;
}

InspectorPageAgentNG::~InspectorPageAgentNG() = default;

void InspectorPageAgentNG::Enable(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->PageEnable(responder, params);
}

void InspectorPageAgentNG::CanScreencast(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->PageCanScreencast(responder, params);
}

void InspectorPageAgentNG::CanEmulate(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->PageCanEmulate(responder, params);
}

void InspectorPageAgentNG::GetResourceTree(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->PageGetResourceTree(responder, params);
}

void InspectorPageAgentNG::GetResourceContent(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->PageGetResourceContent(responder, params);
}

void InspectorPageAgentNG::StartScreencast(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->StartScreencast(responder, params);
}

void InspectorPageAgentNG::StopScreencast(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->StopScreencast(responder, params);
}

void InspectorPageAgentNG::ScreencastFrameAck(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->ScreencastFrameAck(responder, params);
}

void InspectorPageAgentNG::Reload(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->PageReload(responder, params);
}

void InspectorPageAgentNG::Navigate(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->PageNavigate(responder, params);
}

void InspectorPageAgentNG::CallMethod(
    const std::shared_ptr<CDPResponder>& responder,
    const Json::Value& message) {
  const std::string method = message["method"].asString();
  const auto iter = functions_map_.find(method);
  if (iter == functions_map_.end()) {
    responder->SendError(CDPErrorCode::MethodNotFound,
                         "'" + method + "' wasn't found");
    return;
  }
  (this->*(iter->second))(responder, message["params"]);
}

}  // namespace devtool
}  // namespace lynx
