// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_SYSTEM_INFO_AGENT_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_SYSTEM_INFO_AGENT_H_

#include <map>
#include <string>

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
#include "devtool/lynx_devtool/agent/agent_defines.h"

namespace lynx {
namespace devtool {

class SystemInfoAgent : public CDPDomainAgentBase {
 public:
  SystemInfoAgent();
  virtual ~SystemInfoAgent();
  void CallMethod(const std::shared_ptr<CDPResponder>& responder,
                  const Json::Value& message) override;

 private:
  typedef void (SystemInfoAgent::*SystemInfoAgentMethod)(
      const std::shared_ptr<CDPResponder>& responder,
      const Json::Value& message);

  DECLARE_DEVTOOL_CDP_METHOD(getInfo);

  std::map<std::string, SystemInfoAgentMethod> functions_map_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_SYSTEM_INFO_AGENT_H_
