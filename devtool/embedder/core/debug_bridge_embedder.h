// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_EMBEDDER_CORE_DEBUG_BRIDGE_EMBEDDER_H_
#define DEVTOOL_EMBEDDER_CORE_DEBUG_BRIDGE_EMBEDDER_H_

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "base/include/no_destructor.h"
#include "devtool/embedder/core/debug_state_listener_embedder.h"
#include "devtool/embedder/core/inspector_owner_embedder.h"
#include "third_party/debug_router/src/debug_router/common/debug_router_global_handler.h"

namespace lynx {
namespace devtool {

using DevtoolsOpenCardCallback = std::function<void(const std::string&)>;
using DevtoolsCloseCardCallback = std::function<void()>;

using DevtoolAgentDispatcher = InspectorOwnerEmbedder;

class DebugBridgeEmbedder
    : public debugrouter::common::DebugRouterGlobalHandler {
 public:
  static DebugBridgeEmbedder& GetInstance();

  bool IsEnabled();

  void Init();

  bool Enable(const std::string& url,
              const std::unordered_map<std::string, std::string>& app_infos);
  void SetOpenCardCallback(DevtoolsOpenCardCallback callback);
  void SetCloseCardCallback(DevtoolsCloseCardCallback callback);

  // debugrouter::common::DebugRouterGlobalHandler
  void OpenCard(const std::string& url) override;
  void OnMessage(const std::string& message, const std::string& type) override;

  void SetAppInfo(
      const std::unordered_map<std::string, std::string>& app_infos);

 private:
  friend class lynx::base::NoDestructor<DebugBridgeEmbedder>;

  DebugBridgeEmbedder();
  ~DebugBridgeEmbedder() = default;

  void EnableDebugging(const std::string& schema);

  std::shared_ptr<DebugStateListenerEmbedder> debug_state_listener_;
  std::shared_ptr<DevtoolAgentDispatcher> agent_dispatcher_;
  std::mutex callback_mutex_;
  DevtoolsOpenCardCallback open_card_callback_;
  DevtoolsCloseCardCallback close_card_callback_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_EMBEDDER_CORE_DEBUG_BRIDGE_EMBEDDER_H_
