// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_overlay_agent_ng.h"

#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace devtool {

InspectorOverlayAgentNG::InspectorOverlayAgentNG(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_(devtool_mediator) {
  functions_map_["Overlay.highlightNode"] =
      &InspectorOverlayAgentNG::HighlightNode;
  functions_map_["Overlay.hideHighlight"] =
      &InspectorOverlayAgentNG::HideHighlight;
}

void InspectorOverlayAgentNG::HighlightNode(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->HighlightNode(responder, params);
}

void InspectorOverlayAgentNG::HideHighlight(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->HideHighlight(responder, params);
}

void InspectorOverlayAgentNG::CallMethod(
    const std::shared_ptr<CDPResponder>& responder,
    const Json::Value& message) {
  std::string method = message["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter == functions_map_.end()) {
    responder->SendError(CDPErrorCode::MethodNotFound,
                         "'" + method + "' wasn't found");
  } else {
    (this->*(iter->second))(responder, message["params"]);
  }
}

}  // namespace devtool
}  // namespace lynx
