// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_AGENT_DEFINES_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_AGENT_DEFINES_H_

// Declares a legacy sender-based CDP method handler. The trailing semicolon is
// part of the macro, so call sites are written without one. Prefer
// DECLARE_DEVTOOL_CDP_METHOD for agents migrated to the CDPResponder API.
#define DECLARE_DEVTOOL_METHOD(methodName)                                     \
  void methodName(const std::shared_ptr<lynx::devtool::MessageSender>& sender, \
                  const Json::Value& message);

// Responder-based counterpart of DECLARE_DEVTOOL_METHOD, used by agents that
// have been migrated to the CDPResponder API. The including translation unit
// must make lynx::devtool::CDPResponder visible (e.g. via
// cdp_domain_agent_base.h).
#define DECLARE_DEVTOOL_CDP_METHOD(methodName)                       \
  void methodName(                                                   \
      const std::shared_ptr<lynx::devtool::CDPResponder>& responder, \
      const Json::Value& message)

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_AGENT_DEFINES_H_
