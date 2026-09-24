// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_UI_TREE_AGENT_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_UI_TREE_AGENT_H_

#include <map>

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
#include "devtool/lynx_devtool/agent/agent_defines.h"

namespace lynx {
namespace devtool {

class LynxDevToolMediator;

class InspectorUITreeAgent : public CDPDomainAgentBase {
 public:
  explicit InspectorUITreeAgent(
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator);
  ~InspectorUITreeAgent() override = default;
  // Temporary compatibility entry point for commands migrated in patch2.
  void CallMethod(const std::shared_ptr<MessageSender>& sender,
                  const Json::Value& message) override;
  void CallMethod(const std::shared_ptr<CDPResponder>& responder,
                  const Json::Value& message) override;

 private:
  // Temporary compatibility type for commands migrated in patch2.
  using LegacyUITreeAgentMethod = void (InspectorUITreeAgent::*)(
      const std::shared_ptr<MessageSender>& sender, const Json::Value& message);
  using UITreeAgentMethod = void (InspectorUITreeAgent::*)(
      const std::shared_ptr<CDPResponder>& responder,
      const Json::Value& params);

  DECLARE_DEVTOOL_CDP_METHOD(Enable);
  DECLARE_DEVTOOL_CDP_METHOD(Disable);
  DECLARE_DEVTOOL_CDP_METHOD(GetLynxUITree);
  // Temporary legacy handlers retained until patch2 migrates these commands.
  void GetUIInfoForNode(const std::shared_ptr<MessageSender>& sender,
                        const Json::Value& message);
  void SetUIStyle(const std::shared_ptr<MessageSender>& sender,
                  const Json::Value& message);

 private:
  std::map<std::string, UITreeAgentMethod> functions_map_;
  // Temporary compatibility map removed after the remaining commands migrate.
  std::map<std::string, LegacyUITreeAgentMethod> legacy_functions_map_;
  const std::shared_ptr<LynxDevToolMediator> devtool_mediator_;
};
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_UI_TREE_AGENT_H_
