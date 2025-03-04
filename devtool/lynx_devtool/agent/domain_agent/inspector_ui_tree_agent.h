// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_UI_TREE_AGENT_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_UI_TREE_AGENT_H_

#include <unordered_map>

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace devtool {

class DevToolAgentNG;

class InspectorUITreeAgent : public CDPDomainAgentBase {
 public:
  explicit InspectorUITreeAgent(
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator);
  virtual ~InspectorUITreeAgent() = default;
  virtual void CallMethod(const std::shared_ptr<MessageSender>& sender,
                          const Json::Value& message) override;

 private:
  typedef void (InspectorUITreeAgent::*UITreeAgentMethod)(
      const std::shared_ptr<MessageSender>& sender, const Json::Value& message);
  void Enable(const std::shared_ptr<MessageSender>& sender,
              const Json::Value& message);
  void Disable(const std::shared_ptr<MessageSender>& sender,
               const Json::Value& message);
  void GetLynxUITree(const std::shared_ptr<MessageSender>& sender,
                     const Json::Value& message);
  void GetUIInfoForNode(const std::shared_ptr<MessageSender>& sender,
                        const Json::Value& message);
  void SetUIStyle(const std::shared_ptr<MessageSender>& sender,
                  const Json::Value& message);

 private:
  std::map<std::string, UITreeAgentMethod> functions_map_;
  const std::shared_ptr<LynxDevToolMediator> devtool_mediator_;
};
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_UI_TREE_AGENT_H_
