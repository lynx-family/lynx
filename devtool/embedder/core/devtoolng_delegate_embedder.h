// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_EMBEDDER_CORE_DEVTOOLNG_DELEGATE_EMBEDDER_H_
#define DEVTOOL_EMBEDDER_CORE_DEVTOOLNG_DELEGATE_EMBEDDER_H_

#include <memory>
#include <string>

#include "devtool/embedder/core/devtools_message_handler.h"
#include "devtool/lynx_devtool/agent/devtool_platform_facade.h"
#include "devtool/lynx_devtool/lynx_devtool_ng.h"

namespace lynx {
namespace devtool {

class DevToolNGDelegateEmbedder : public lynx::devtool::LynxDevToolNG {
 public:
  explicit DevToolNGDelegateEmbedder(bool debuggable = true);
  int32_t getSessionId() { return session_id_; }
  bool isAttachToDebugRouter() { return session_id_ != 0; }
  void attachToDebug(const std::string &url);
  void detachFromDebug();
  void sendMessageToDebugPlatform(const std::string &type,
                                  const std::string &msg);
  void SetDevtoolPlatformAbility(
      const std::shared_ptr<devtool::DevToolPlatformFacade> &platform_ptr);

  void subscribeMessage(const std::string &type,
                        const std::shared_ptr<MessageHandler> &handler);

 private:
  int32_t session_id_ = 0;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_EMBEDDER_CORE_DEVTOOLNG_DELEGATE_EMBEDDER_H_
