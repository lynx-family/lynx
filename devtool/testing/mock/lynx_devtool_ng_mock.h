// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_TESTING_MOCK_LYNX_DEVTOOL_NG_MOCK_H_
#define DEVTOOL_TESTING_MOCK_LYNX_DEVTOOL_NG_MOCK_H_

#include <memory>
#include <string>

#include "devtool/lynx_devtool/lynx_devtool_ng.h"

namespace lynx {
namespace testing {
class LynxDevToolNGMock : public lynx::devtool::LynxDevToolNG {
 public:
  LynxDevToolNGMock() = default;
  ~LynxDevToolNGMock() override = default;

  int32_t Attach(const std::string& url) override;

  void SendMessageToDebugPlatform(const std::string& type,
                                  const std::string& message);

  void OnTasmCreated(intptr_t shell_ptr);

  void SetDevToolPlatformFacade(
      const std::shared_ptr<devtool::DevToolPlatformFacade>& platform_facade);

  std::shared_ptr<lynx::piper::InspectorRuntimeObserverNG>
  OnBackgroundRuntimeCreated(const std::string& group_thread_name);

  std::shared_ptr<devtool::MessageSender> GetMessageSender() const override;

  std::shared_ptr<devtool::MessageSender> message_sender_ = nullptr;
};
}  // namespace testing
}  // namespace lynx

#endif  // DEVTOOL_TESTING_MOCK_LYNX_DEVTOOL_NG_MOCK_H_
