// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_PERFORMANCE_PERFORMANCE_MEDIATOR_H_
#define CORE_SERVICES_PERFORMANCE_PERFORMANCE_MEDIATOR_H_

#include <memory>
#include <string>

#include "base/include/lynx_actor.h"
#include "core/public/pub_value.h"
#include "core/services/performance/performance_event_sender.h"

namespace lynx {
namespace shell {
class BTSRuntime;
class LynxEngine;
}  // namespace shell
namespace tasm {
namespace performance {

class PerformanceMediator : public PerformanceEventSender {
 public:
  PerformanceMediator() : PerformanceEventSender(nullptr) {}
  ~PerformanceMediator() = default;

  // Setter methods for various properties.
  inline void SetRuntimeActor(
      const std::shared_ptr<shell::LynxActor<shell::BTSRuntime>>& actor) {
    runtime_actor_ = actor;
  }
  inline void SetEngineActor(
      const std::shared_ptr<shell::LynxActor<shell::LynxEngine>>& actor) {
    engine_actor_ = actor;
  }

  void OnPerformanceEvent(std::unique_ptr<pub::Value> entry,
                          EventType type = kEventTypeAll) override;

 private:
  std::shared_ptr<shell::LynxActor<shell::BTSRuntime>> runtime_actor_;
  std::shared_ptr<shell::LynxActor<shell::LynxEngine>> engine_actor_;
};

}  // namespace performance
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_PERFORMANCE_PERFORMANCE_MEDIATOR_H_
