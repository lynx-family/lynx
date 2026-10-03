// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_WHITE_BOARD_AGENT_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_WHITE_BOARD_AGENT_H_

#include <map>
#include <memory>

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
#include "devtool/lynx_devtool/agent/agent_defines.h"

namespace lynx {
namespace devtool {

class LynxDevToolMediator;

class InspectorWhiteBoardAgent : public CDPDomainAgentBase {
 public:
  explicit InspectorWhiteBoardAgent(
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator);
  ~InspectorWhiteBoardAgent() override = default;

  void CallMethod(const std::shared_ptr<CDPResponder>& responder,
                  const Json::Value& message) override;

 private:
  using WhiteBoardAgentMethod = void (InspectorWhiteBoardAgent::*)(
      const std::shared_ptr<CDPResponder>& responder,
      const Json::Value& params);

  DECLARE_DEVTOOL_CDP_METHOD(Enable);
  DECLARE_DEVTOOL_CDP_METHOD(Disable);
  DECLARE_DEVTOOL_CDP_METHOD(SetSharedData);
  DECLARE_DEVTOOL_CDP_METHOD(GetSharedData);
  DECLARE_DEVTOOL_CDP_METHOD(RemoveSharedData);
  DECLARE_DEVTOOL_CDP_METHOD(Clear);

  std::map<std::string, WhiteBoardAgentMethod> functions_map_;
  const std::shared_ptr<LynxDevToolMediator> devtool_mediator_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_WHITE_BOARD_AGENT_H_
