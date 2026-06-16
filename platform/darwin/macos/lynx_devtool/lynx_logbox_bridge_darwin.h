// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_DARWIN_MACOS_LYNX_DEVTOOL_LYNX_LOGBOX_BRIDGE_DARWIN_H_
#define PLATFORM_DARWIN_MACOS_LYNX_DEVTOOL_LYNX_LOGBOX_BRIDGE_DARWIN_H_

#include <memory>

#include "platform/embedder/lynx_devtool/logbox/lynx_logbox_bridge.h"

namespace lynx {
namespace embedder {

class LogBoxResourceProvider;

class LynxLogBoxBridgeDarwin final : public LynxLogBoxBridge {
 public:
  explicit LynxLogBoxBridgeDarwin(LogBoxResourceProvider* provider);
  ~LynxLogBoxBridgeDarwin() override;
  LynxLogBoxBridgeDarwin(const LynxLogBoxBridgeDarwin&) = delete;
  LynxLogBoxBridgeDarwin& operator=(const LynxLogBoxBridgeDarwin&) = delete;

  void OnHostViewAttached() override;
  void OnError(const LogBoxErrorInfo& error) override;
  void OnReload() override;
  void OnDestroy() override;

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_DARWIN_MACOS_LYNX_DEVTOOL_LYNX_LOGBOX_BRIDGE_DARWIN_H_
