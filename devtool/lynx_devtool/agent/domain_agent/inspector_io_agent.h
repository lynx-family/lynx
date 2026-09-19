// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_IO_AGENT_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_IO_AGENT_H_

#include <memory>

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"

namespace lynx {
namespace devtool {

class InspectorIOAgent : public CDPDomainAgentBase {
 public:
  InspectorIOAgent();

  virtual ~InspectorIOAgent();

  virtual void CallMethod(const std::shared_ptr<MessageSender>& sender,
                          const Json::Value& message) override;

 private:
  using IOAgentMethod = void (InspectorIOAgent::*)(
      const std::shared_ptr<MessageSender>& sender, const Json::Value& message);
  std::map<std::string, IOAgentMethod> functions_map_;
  void Read(const std::shared_ptr<MessageSender>& sender,
            const Json::Value& message);
  void Close(const std::shared_ptr<MessageSender>& sender,
             const Json::Value& message);
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_IO_AGENT_H_
