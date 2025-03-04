// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_OVERLAY_AGENT_NG_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_OVERLAY_AGENT_NG_H_

#include <memory>
#include <unordered_map>

#include "core/inspector/style_sheet.h"
#include "core/renderer/dom/element.h"
#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace devtool {

class DevToolAgentNG;

class InspectorOverlayAgentNG : public CDPDomainAgentBase {
 public:
  InspectorOverlayAgentNG(
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator);
  virtual ~InspectorOverlayAgentNG() = default;
  virtual void CallMethod(const std::shared_ptr<MessageSender>& sender,
                          const Json::Value& message) override;

 private:
  typedef void (InspectorOverlayAgentNG::*OverlayAgentMethod)(
      const std::shared_ptr<MessageSender>& sender, const Json::Value& params);

  void HighlightNode(const std::shared_ptr<MessageSender>& sender,
                     const Json::Value& message);
  void HideHighlight(const std::shared_ptr<MessageSender>& sender,
                     const Json::Value& message);

  std::map<std::string, OverlayAgentMethod> functions_map_;
  InspectorStyleSheet origin_inline_style_;
  const std::shared_ptr<LynxDevToolMediator>& devtool_mediator_;
};
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_OVERLAY_AGENT_NG_H_
