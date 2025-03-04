// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_SYSTEM_INFO_AGENT_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_SYSTEM_INFO_AGENT_H_

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"

namespace lynx {
namespace devtool {

class SystemInfoAgent : public CDPDomainAgentBase {
 public:
  SystemInfoAgent();
  virtual ~SystemInfoAgent();
  virtual void CallMethod(const std::shared_ptr<MessageSender>& sender,
                          const Json::Value& message) override;

 private:
  typedef void (SystemInfoAgent::*PerformanceAgentMethod)(
      const std::shared_ptr<MessageSender>& sender, const Json::Value& message);

  void getInfo(const std::shared_ptr<MessageSender>& sender,
               const Json::Value& message);

  std::map<std::string, PerformanceAgentMethod> functions_map_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_SYSTEM_INFO_AGENT_H_
