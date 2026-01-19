// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/performance/performance_mediator.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "base/include/value/base_value.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/js/runtime_constant.h"
#include "core/services/timing_handler/timing_constants.h"
#include "core/services/trace/service_trace_event_def.h"
#include "core/shell/lynx_engine.h"
#include "core/shell/runtime/bts/bts_runtime.h"

namespace lynx {
namespace tasm {
namespace performance {
// onPerformance event name. The global event name that needs to be listened
// to in order to receive PerformanceObserver callbacks in the frontend
// framework.
inline constexpr const char kPerformanceRuntimeCallback[] =
    "lynx.performance.onPerformanceEvent";

void PerformanceMediator::OnPerformanceEvent(
    std::unique_ptr<pub::Value> performance_entry, EventType type) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, PERFORMANCE_MEDIATOR_ON_PERFORMANCE_EVENT);
  lepus::Value lepus_entry =
      pub::ValueUtils::ConvertValueToLepusValue(*performance_entry);
  // Runtime Performance Callback.
  if ((type & kEventTypeBTSEngine) && runtime_actor_) {
    runtime_actor_->ActAsync([entry = lepus_entry](auto& runtime) {
      TRACE_EVENT(LYNX_TRACE_CATEGORY,
                  PERFORMANCE_MEDIATOR_ON_PERFORMANCE_EVENT_BTS_ENGINE);
      auto args = lepus::CArray::Create();
      args->emplace_back(BASE_STATIC_STRING(kPerformanceRuntimeCallback));
      args->emplace_back((lepus_value::ShallowCopy(entry)));
      auto event = fml::MakeRefCounted<runtime::MessageEvent>(
          runtime::kMessageEventTypeSendGlobalEvent,
          runtime::ContextProxy::Type::kCoreContext,
          runtime::ContextProxy::Type::kJSContext,
          std::make_unique<pub::ValueImplLepus>(lepus::Value(std::move(args))));
      runtime->OnReceiveMessageEvent(std::move(event));
    });
  }
  if (GetEnableMainThreadCallback() && (type & kEventTypeMTSEngine) &&
      engine_actor_) {
    engine_actor_->ActAsync(
        [entry = std::move(lepus_entry)](auto& engine) mutable {
          TRACE_EVENT(LYNX_TRACE_CATEGORY,
                      PERFORMANCE_MEDIATOR_ON_PERFORMANCE_EVENT_MTS_ENGINE);
          auto arguments = lepus::CArray::Create();
          arguments->emplace_back(std::move(entry));
          engine->TriggerEventBus(kPerformanceRuntimeCallback,
                                  lepus_value(std::move(arguments)));
        });
  }
}

}  // namespace performance
}  // namespace tasm
}  // namespace lynx
