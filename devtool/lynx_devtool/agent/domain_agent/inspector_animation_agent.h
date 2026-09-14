// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_ANIMATION_AGENT_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_ANIMATION_AGENT_H_

#include <map>
#include <memory>
#include <string>

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
#include "devtool/lynx_devtool/agent/agent_defines.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace devtool {

// CDP Animation domain agent. Monitoring, seekAnimations, setPaused, and
// releaseAnimations are implemented; the remaining control methods
// (setPlaybackRate, setTiming, resolveAnimation, getPlaybackRate, ...) fall
// through to a MethodNotFound response (-32601). Actual work runs on the TASM
// thread via the mediator bridges; see InspectorTasmExecutor.
class InspectorAnimationAgent : public CDPDomainAgentBase {
 public:
  explicit InspectorAnimationAgent(
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator);
  ~InspectorAnimationAgent() override = default;

  void CallMethod(const std::shared_ptr<CDPResponder>& responder,
                  const Json::Value& message) override;

 private:
  typedef void (InspectorAnimationAgent::*AnimationAgentMethod)(
      const std::shared_ptr<CDPResponder>& responder,
      const Json::Value& params);

  DECLARE_DEVTOOL_CDP_METHOD(Enable);
  DECLARE_DEVTOOL_CDP_METHOD(Disable);
  DECLARE_DEVTOOL_CDP_METHOD(GetCurrentTime);
  DECLARE_DEVTOOL_CDP_METHOD(SeekAnimations);
  DECLARE_DEVTOOL_CDP_METHOD(SetPaused);
  DECLARE_DEVTOOL_CDP_METHOD(ReleaseAnimations);

  std::map<std::string, AnimationAgentMethod> functions_map_;
  const std::shared_ptr<LynxDevToolMediator> devtool_mediator_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_ANIMATION_AGENT_H_
