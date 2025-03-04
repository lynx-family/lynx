// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_TIMING_HANDLER_TIMING_MEDIATOR_H_
#define CORE_SERVICES_TIMING_HANDLER_TIMING_MEDIATOR_H_

#include <memory>
#include <string>

#include "base/include/lynx_actor.h"
#include "core/public/pub_value.h"
#include "core/runtime/piper/js/lynx_runtime.h"
#include "core/services/timing_handler/timing_handler.h"
#include "core/services/timing_handler/timing_handler_delegate.h"
#include "core/services/timing_handler/timing_info.h"
#include "core/shell/lynx_engine.h"
#include "core/shell/native_facade.h"
#include "core/shell/native_facade_reporter.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {
namespace timing {

/**
 * The TimingMediator class serves as an intermediary for handling
 * timing-related information and events. It implements the Delegate interface
 * from TimingHandler to respond to timing setup and update events. The class
 * allows for the setting of runtime and facade actors to facilitate
 * communication and event handling between the JavaScript runtime and the
 * native platform layer. It also use EventReporter to report timing events.
 */
class TimingMediator : public TimingHandlerDelegate {
 public:
  TimingMediator(int32_t instance_id);
  ~TimingMediator() override = default;

  // Setter methods for various properties.
  inline void SetRuntimeActor(
      const std::shared_ptr<shell::LynxActor<runtime::LynxRuntime>>& actor) {
    runtime_actor_ = actor;
  }
  inline void SetEngineActor(
      const std::shared_ptr<shell::LynxActor<shell::LynxEngine>>& actor) {
    engine_actor_ = actor;
  }
  void SetFacadeActor(
      const std::shared_ptr<shell::LynxActor<shell::NativeFacade>>& actor) {
    facade_actor_ = actor;
  }
  void SetFacadeReporterActor(
      const std::shared_ptr<shell::LynxActor<shell::NativeFacadeReporter>>&
          actor) {
    facade_reporter_actor_ = actor;
  }
  inline void SetEnableJSRuntime(bool enable_js_runtime) {
    enable_js_runtime_ = enable_js_runtime;
  }

  // Inherit from TimingHandler::Delegate
  const std::shared_ptr<pub::PubValueFactory>& GetValueFactory()
      const override {
    return value_factory_;
  }
  void OnTimingSetup(const TimingInfo& timing_info) const override;
  void OnTimingUpdate(const TimingInfo& timing_info,
                      const std::string& update_flag) const override;
  void OnPerformanceEvent(
      const std::unique_ptr<lynx::pub::Value> performance_entry,
      bool enable_engine_callback) const override;

 private:
  const int32_t instance_id_ = 0;
  bool enable_js_runtime_{true};
  std::shared_ptr<shell::LynxActor<runtime::LynxRuntime>> runtime_actor_;
  std::shared_ptr<shell::LynxActor<shell::NativeFacade>> facade_actor_;
  std::shared_ptr<shell::LynxActor<shell::NativeFacadeReporter>>
      facade_reporter_actor_;
  std::shared_ptr<shell::LynxActor<shell::LynxEngine>> engine_actor_;
  std::shared_ptr<pub::PubValueFactory> value_factory_ =
      std::make_shared<pub::PubValueFactoryDefault>();

  // Internal methods for triggering runtime OnTimingSetup callback.
  void TriggerSetupRuntimeCallback(const TimingInfo& timing_info) const;
  // Internal methods for triggering platform OnTimingSetup callback.
  void TriggerSetupClientCallback(const TimingInfo& timing_info) const;
  // Internal methods for reporting OnTimingSetup event.
  void ReportSetupEvent(const TimingInfo& timing_info) const;

  // Internal methods for triggering runtime OnTimingUpdate callback.
  void TriggerUpdateRuntimeCallback(const TimingInfo& timing_info,
                                    const std::string& update_flag) const;
  // Internal methods for triggering platform OnTimingUpdate callback.
  void TriggerUpdateClientCallback(const TimingInfo& timing_info,
                                   const std::string& update_flag) const;
  // Internal methods for reporting OnTimingUpdate event.
  void ReportUpdateEvent(const TimingInfo& timing_info,
                         const std::string& update_flag) const;
};

}  // namespace timing
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_TIMING_HANDLER_TIMING_MEDIATOR_H_
