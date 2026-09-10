// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_LYNX_NATIVE_MODULE_AGENT_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_LYNX_NATIVE_MODULE_AGENT_H_

#include <map>
#include <memory>
#include <string>

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace devtool {

// Handles the custom `LynxNativeModule` CDP domain: enable/disable live
// reporting and getRecords history replay. It parses the CDP method and
// dispatches to LynxDevToolMediator. Record collection is added separately;
// until then the mediator returns a protocol-valid empty history.
class InspectorLynxNativeModuleAgent : public CDPDomainAgentBase {
 public:
  explicit InspectorLynxNativeModuleAgent(
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator);
  ~InspectorLynxNativeModuleAgent() override;

  void CallMethod(const std::shared_ptr<MessageSender>& sender,
                  const Json::Value& message) override;

 private:
  typedef void (InspectorLynxNativeModuleAgent::*NativeModuleAgentMethod)(
      const std::shared_ptr<MessageSender>& sender, const Json::Value& message);
  void Enable(const std::shared_ptr<MessageSender>& sender,
              const Json::Value& message);
  void Disable(const std::shared_ptr<MessageSender>& sender,
               const Json::Value& message);
  void GetRecords(const std::shared_ptr<MessageSender>& sender,
                  const Json::Value& message);

  std::map<std::string, NativeModuleAgentMethod> functions_map_;
  const std::shared_ptr<LynxDevToolMediator> devtool_mediator_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_LYNX_NATIVE_MODULE_AGENT_H_
