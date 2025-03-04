// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_INSPECTOR_COMMON_OBSERVER_IMPL_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_INSPECTOR_COMMON_OBSERVER_IMPL_H_

#include <memory>

#include "core/inspector/observer/inspector_common_observer.h"
#include "devtool/base_devtool/native/public/message_sender.h"

namespace lynx {
namespace devtool {
class LynxDevToolMediator;

class InspectorCommonObserverImpl
    : public lynx::tasm::InspectorCommonObserver,
      public std::enable_shared_from_this<InspectorCommonObserverImpl> {
 public:
  InspectorCommonObserverImpl(
      std::shared_ptr<MessageSender> sender,
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
      : sender_(sender), mediator_wp_(devtool_mediator) {}

  ~InspectorCommonObserverImpl() noexcept override = default;
  void EndReplayTest(const std::string& file_path) override;
  void SendLayoutTree() override;

 private:
  std::weak_ptr<MessageSender> sender_;
  std::weak_ptr<LynxDevToolMediator> mediator_wp_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_INSPECTOR_COMMON_OBSERVER_IMPL_H_
