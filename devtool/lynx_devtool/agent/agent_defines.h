// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_AGENT_DEFINES_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_AGENT_DEFINES_H_

#define DECLARE_DEVTOOL_METHOD(methodName)                                     \
  void methodName(const std::shared_ptr<lynx::devtool::MessageSender>& sender, \
                  const Json::Value& message);

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_AGENT_DEFINES_H_
