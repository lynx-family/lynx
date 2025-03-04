// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_LYNX_AGENT_NG_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_LYNX_AGENT_NG_H_

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace devtool {

class DevToolAgentNG;

class InspectorLynxAgentNG : public CDPDomainAgentBase {
 public:
  explicit InspectorLynxAgentNG(
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator);
  virtual ~InspectorLynxAgentNG();
  virtual void CallMethod(const std::shared_ptr<MessageSender>& sender,
                          const Json::Value& message) override;

 private:
  typedef void (InspectorLynxAgentNG::*LynxAgentMethod)(
      const std::shared_ptr<MessageSender>& sender, const Json::Value& params);

  void GetProperties(const std::shared_ptr<MessageSender>& sender,
                     const Json::Value& message);
  void GetData(const std::shared_ptr<MessageSender>& sender,
               const Json::Value& message);
  void GetComponentId(const std::shared_ptr<MessageSender>& sender,
                      const Json::Value& message);
  void GetLynxViewRectToWindow(const std::shared_ptr<MessageSender>& sender,
                               const Json::Value& message);
  void GetLynxVersion(const std::shared_ptr<MessageSender>& sender,
                      const Json::Value& message);

  void TransferData(const std::shared_ptr<MessageSender>& sender,
                    const Json::Value& message);

  void SetTraceMode(const std::shared_ptr<MessageSender>& sender,
                    const Json::Value& message);

  void GetScreenshot(const std::shared_ptr<MessageSender>& sender,
                     const Json::Value& message);

  void GetViewLocationOnScreen(const std::shared_ptr<MessageSender>& sender,
                               const Json::Value& message);

  void SendVMEvent(const std::shared_ptr<MessageSender>& sender,
                   const Json::Value& message);

  std::map<std::string, LynxAgentMethod> functions_map_;

  std::shared_ptr<LynxDevToolMediator> devtool_mediator_;
};
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_LYNX_AGENT_NG_H_
