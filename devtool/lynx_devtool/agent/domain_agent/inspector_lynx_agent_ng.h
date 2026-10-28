// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_LYNX_AGENT_NG_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_LYNX_AGENT_NG_H_

#include <unordered_map>

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
#include "devtool/lynx_devtool/agent/agent_defines.h"

namespace lynx {
namespace devtool {

class LynxDevToolMediator;

class InspectorLynxAgentNG : public CDPDomainAgentBase {
 public:
  explicit InspectorLynxAgentNG(
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator);
  ~InspectorLynxAgentNG() override;
  void CallMethod(const std::shared_ptr<CDPResponder>& responder,
                  const Json::Value& message) override;

 private:
  using LynxAgentMethod = void (InspectorLynxAgentNG::*)(
      const std::shared_ptr<CDPResponder>& responder,
      const Json::Value& params);

  DECLARE_DEVTOOL_CDP_METHOD(SetObservePayloadEnabled);
  DECLARE_DEVTOOL_CDP_METHOD(GetLogLevel);
  DECLARE_DEVTOOL_CDP_METHOD(SetLogLevel);
  DECLARE_DEVTOOL_CDP_METHOD(GetProperties);
  DECLARE_DEVTOOL_CDP_METHOD(GetData);
  DECLARE_DEVTOOL_CDP_METHOD(GetComponentId);
  DECLARE_DEVTOOL_CDP_METHOD(GetLynxViewRectToWindow);
  DECLARE_DEVTOOL_CDP_METHOD(GetLynxVersion);
  DECLARE_DEVTOOL_CDP_METHOD(TransferData);
  DECLARE_DEVTOOL_CDP_METHOD(SetTraceMode);
  DECLARE_DEVTOOL_CDP_METHOD(GetScreenshot);
  DECLARE_DEVTOOL_CDP_METHOD(GetViewLocationOnScreen);
  DECLARE_DEVTOOL_CDP_METHOD(SendVMEvent);

  std::unordered_map<std::string, LynxAgentMethod> functions_map_;
  std::shared_ptr<LynxDevToolMediator> devtool_mediator_;
};
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_LYNX_AGENT_NG_H_
