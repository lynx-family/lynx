// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_HSR_AGENT_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_HSR_AGENT_H_

#include <map>

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"

namespace lynx {
namespace devtool {

class InspectorHSRAgent : public CDPDomainAgentBase {
 public:
  InspectorHSRAgent();
  void CallMethod(const std::shared_ptr<CDPResponder>& responder,
                  const Json::Value& message) override;

 private:
  using HSRAgentMethod = void (InspectorHSRAgent::*)(
      const std::shared_ptr<CDPResponder>&, const Json::Value&);

  std::map<std::string, HSRAgentMethod> functions_map_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_HSR_AGENT_H_
