// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_TEMPLATE_AGENT_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_TEMPLATE_AGENT_H_

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
#include "devtool/base_devtool/native/public/message_sender.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace devtool {

class InspectorTemplateAgent : public CDPDomainAgentBase {
 public:
  InspectorTemplateAgent(
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator);
  virtual ~InspectorTemplateAgent();
  virtual void CallMethod(const std::shared_ptr<MessageSender>& sender,
                          const Json::Value& message) override;

 private:
  typedef void (InspectorTemplateAgent::*TemplateAgentMethod)(
      const std::shared_ptr<MessageSender>& sender, const Json::Value& message);
  void GetTemplateData(const std::shared_ptr<MessageSender>& sender,
                       const Json::Value& message);
  void GetTemplateConfigInfo(const std::shared_ptr<MessageSender>& sender,
                             const Json::Value& message);
  void GetTemplateApiInfo(const std::shared_ptr<MessageSender>& sender,
                          const Json::Value& message);
  void GetTemplateJsInfo(const std::shared_ptr<MessageSender>& sender,
                         const Json::Value& message);

  std::map<std::string, TemplateAgentMethod> functions_map_;
  const std::shared_ptr<LynxDevToolMediator> devtool_mediator_;
};

}  // namespace devtool
}  // namespace lynx
#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_TEMPLATE_AGENT_H_
