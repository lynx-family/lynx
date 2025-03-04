// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_MEMORY_AGENT_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_MEMORY_AGENT_H_

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"

namespace lynx {
namespace devtool {

class InspectorMemoryAgent : public CDPDomainAgentBase {
 public:
  InspectorMemoryAgent();
  virtual ~InspectorMemoryAgent();
  virtual void CallMethod(const std::shared_ptr<MessageSender>& sender,
                          const Json::Value& message) override;

 private:
  typedef void (InspectorMemoryAgent::*MemoryAgentMethod)(
      const std::shared_ptr<MessageSender>& sender, const Json::Value& message);

  void StartTracing(const std::shared_ptr<MessageSender>& sender,
                    const Json::Value& message);
  void StopTracing(const std::shared_ptr<MessageSender>& sender,
                   const Json::Value& message);

  std::map<std::string, MemoryAgentMethod> functions_map_;
};

}  // namespace devtool
}  // namespace lynx
#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_MEMORY_AGENT_H_
