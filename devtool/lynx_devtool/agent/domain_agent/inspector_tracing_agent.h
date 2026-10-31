// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_TRACING_AGENT_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_TRACING_AGENT_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
#include "devtool/lynx_devtool/agent/agent_defines.h"

namespace lynx {
namespace devtool {

class InspectorTracingAgent : public CDPDomainAgentBase {
 public:
  InspectorTracingAgent();

  ~InspectorTracingAgent() override;

  void CallMethod(const std::shared_ptr<CDPResponder>& responder,
                  const Json::Value& message) override;

 private:
  using TracingAgentMethod = void (InspectorTracingAgent::*)(
      const std::shared_ptr<CDPResponder>& responder,
      const Json::Value& params);

  DECLARE_DEVTOOL_CDP_METHOD(Start);
  DECLARE_DEVTOOL_CDP_METHOD(End);
  DECLARE_DEVTOOL_CDP_METHOD(SetStartupTracingConfig);
  DECLARE_DEVTOOL_CDP_METHOD(GetStartupTracingConfig);
  DECLARE_DEVTOOL_CDP_METHOD(GetStartupTracingFile);
  DECLARE_DEVTOOL_CDP_METHOD(TakeVMSnapshotByUrl);

  std::unordered_map<std::string, TracingAgentMethod> functions_map_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_TRACING_AGENT_H_
