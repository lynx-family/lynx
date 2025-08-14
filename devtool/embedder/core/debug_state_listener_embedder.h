// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_EMBEDDER_CORE_DEBUG_STATE_LISTENER_EMBEDDER_H_
#define DEVTOOL_EMBEDDER_CORE_DEBUG_STATE_LISTENER_EMBEDDER_H_

#include <string>

#include "third_party/debug_router/src/debug_router/common/debug_router.h"

namespace lynx {
namespace devtool {

class DebugStateListenerEmbedder
    : public debugrouter::common::DebugRouterStateListener,
      public std::enable_shared_from_this<DebugStateListenerEmbedder> {
 public:
  void OnOpen(debugrouter::common::ConnectionType type) override;
  void OnClose(int32_t code, const std::string &reason) override {}
  void OnMessage(const std::string &message) override {}
  void OnError(const std::string &error) override {}

  DebugStateListenerEmbedder() = default;
  virtual ~DebugStateListenerEmbedder() = default;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_EMBEDDER_CORE_DEBUG_STATE_LISTENER_EMBEDDER_H_
