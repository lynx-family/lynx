// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_PAGE_AGENT_NG_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_PAGE_AGENT_NG_H_

#include <map>
#include <memory>

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
#include "devtool/lynx_devtool/agent/agent_defines.h"

namespace lynx {
namespace devtool {

class LynxDevToolMediator;

class InspectorPageAgentNG : public CDPDomainAgentBase {
 public:
  explicit InspectorPageAgentNG(
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator);
  ~InspectorPageAgentNG() override;
  void CallMethod(const std::shared_ptr<CDPResponder>& responder,
                  const Json::Value& message) override;

 private:
  using PageAgentMethod = void (InspectorPageAgentNG::*)(
      const std::shared_ptr<CDPResponder>& responder,
      const Json::Value& params);

  DECLARE_DEVTOOL_CDP_METHOD(Enable);
  DECLARE_DEVTOOL_CDP_METHOD(CanScreencast);
  DECLARE_DEVTOOL_CDP_METHOD(CanEmulate);
  DECLARE_DEVTOOL_CDP_METHOD(GetResourceTree);
  DECLARE_DEVTOOL_CDP_METHOD(GetResourceContent);
  DECLARE_DEVTOOL_CDP_METHOD(StartScreencast);
  DECLARE_DEVTOOL_CDP_METHOD(StopScreencast);
  DECLARE_DEVTOOL_CDP_METHOD(ScreencastFrameAck);
  DECLARE_DEVTOOL_CDP_METHOD(Reload);
  DECLARE_DEVTOOL_CDP_METHOD(Navigate);

  std::map<std::string, PageAgentMethod> functions_map_;
  std::shared_ptr<LynxDevToolMediator> devtool_mediator_;
};
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_PAGE_AGENT_NG_H_
