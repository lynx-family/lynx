// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_TESTING_MOCK_LYNX_DEVTOOL_MEDIATOR_MOCK_H_
#define DEVTOOL_TESTING_MOCK_LYNX_DEVTOOL_MEDIATOR_MOCK_H_

#define protected public
#define private public
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace testing {

class LynxDevToolMediatorMock : public lynx::devtool::LynxDevToolMediator {
  void Init(lynx::shell::LynxShell* shell) {
    LynxDevToolMediator::Init(shell, nullptr);
  }
  void SendLogEntryAddedEvent(
      const lynx::piper::ConsoleMessage& message) override {
    message_ = message;
  }

 private:
  piper::ConsoleMessage message_{"", -2, -1};
};

}  // namespace testing
}  // namespace lynx

#endif  // DEVTOOL_TESTING_MOCK_LYNX_DEVTOOL_MEDIATOR_MOCK_H_
