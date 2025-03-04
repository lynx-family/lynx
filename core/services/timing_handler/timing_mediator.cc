// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/timing_handler/timing_mediator.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "core/renderer/utils/value_utils.h"
#include "core/runtime/piper/js/runtime_constant.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/table.h"
#include "core/services/event_report/event_tracker_platform_impl.h"
#include "core/services/timing_handler/timing_constants.h"
#include "core/services/timing_handler/timing_constants_deprecated.h"

namespace lynx {
namespace tasm {
namespace timing {
namespace {
// Define an enum for error codes.
enum DurationError {
  kErrorNotTables = 0,
  kErrorStartIsZero = -1,
  kErrorEndIsZero = -2,
  kErrorStartAndEndAreZero = -3,
  kErrorStartTimeGreaterThanEndTime = -4
};
// Helper function to calculate duration.
TimestampMsFraction CalculateDuration(const lepus::Value& startTable,
                                      const base::String& startKey,
                                      const lepus::Value& endTable,
                                      const base::String& endKey) {
  if (!startTable.IsTable() || !endTable.IsTable()) {
    return kErrorNotTables;
  }
  auto end_time = endTable.GetProperty(endKey).Number();
  auto start_time = startTable.GetProperty(startKey).Number();
  // Check for abnormal or error cases and handle them.
  if (start_time == 0) {
    if (end_time != 0) {
      // Case 1.start = 0, end != 0,  duration = -1
      return kErrorStartIsZero;
    }
    // Case 2.start == 0, end = 0,  duration = -3
    return kErrorStartAndEndAreZero;
  }
  if (end_time == 0) {
    // Case 3. start != 0, end = 0,  duration = -2
    return kErrorEndIsZero;
  }
  if (start_time > end_time) {
    // Case 4. start > end > 0, duration = -4
    return kErrorStartTimeGreaterThanEndTime;
  }
  // Normal case: both times are valid and start is before end.
  return end_time - start_time;
};

}  // namespace

TimingMediator::TimingMediator(int32_t instance_id)
    : instance_id_(instance_id) {}

void TimingMediator::OnTimingSetup(const TimingInfo& timing_info) const {
  TriggerSetupClientCallback(timing_info);
  TriggerSetupRuntimeCallback(timing_info);
  ReportSetupEvent(timing_info);
}

void TimingMediator::OnTimingUpdate(const TimingInfo& timing_info,
                                    const std::string& update_flag) const {
  TriggerUpdateClientCallback(timing_info, update_flag);
  TriggerUpdateRuntimeCallback(timing_info, update_flag);
  ReportUpdateEvent(timing_info, update_flag);
}

void TimingMediator::OnPerformanceEvent(
    std::unique_ptr<lynx::pub::Value> performance_entry,
    bool enable_engine_callback) const {
  lepus::Value lepus_entry =
      pub::ValueUtils::ConvertValueToLepusValue(*performance_entry);
  // Platform Performance Callback.
  if (facade_reporter_actor_) {
    facade_reporter_actor_->ActAsync([entry = lepus_entry](auto& facade) {
      TRACE_EVENT(LYNX_TRACE_CATEGORY,
                  "TimingMediator::TriggerPerformanceClientCallback");
      facade->OnPerformanceEvent(entry);
    });
  }
  // Runtime Performance Callback.
  if (runtime_actor_ && enable_js_runtime_) {
    runtime_actor_->ActAsync([entry = std::move(lepus_entry)](auto& runtime) {
      TRACE_EVENT(LYNX_TRACE_CATEGORY,
                  "TimingMediator::TriggerPerformanceRuntimeCallback");
      auto args = lepus::CArray::Create();
      args->emplace_back(BASE_STATIC_STRING(kPerformanceRuntimeCallback));
      args->emplace_back((lepus_value::ShallowCopy(entry)));
      runtime::MessageEvent event(runtime::kMessageEventTypeSendGlobalEvent,
                                  runtime::ContextProxy::Type::kCoreContext,
                                  runtime::ContextProxy::Type::kJSContext,
                                  lepus::Value(std::move(args)));
      runtime->OnReceiveMessageEvent(std::move(event));
    });
  } else if (engine_actor_ && enable_engine_callback) {
    engine_actor_->ActAsync(
        [entry = std::move(lepus_entry)](auto& engine) mutable {
          TRACE_EVENT(LYNX_TRACE_CATEGORY,
                      "TimingMediator::TriggerPerformanceEngineCallback");
          auto arguments = lepus::CArray::Create();
          arguments->emplace_back(std::move(entry));
          engine->TriggerEventBus(kSetupRuntimeCallback,
                                  lepus_value(std::move(arguments)));
        });
  }
}

// OnTimingSetup callback
void TimingMediator::TriggerSetupClientCallback(
    const TimingInfo& timing_info) const {
  // Platform OnTimingSetup Callback.
  if (facade_actor_) {
    auto timing = timing_info.GetAllTimingInfoAsMillisecond();
    lepus::Value lepus_timing =
        pub::ValueUtils::ConvertValueToLepusValue(*timing);

    facade_actor_->ActAsync([timing = std::move(lepus_timing)](auto& facade) {
      // TODO(kechenglong): set timingHandler PropBundleCreator before
      //  TimingHandler ctor, covert lepus to platform data structure
      //  in timing thread.
      TRACE_EVENT(LYNX_TRACE_CATEGORY,
                  "TimingMediator::TriggerSetupClientCallback");
      facade->OnTimingSetup(timing);
    });
  }
}

void TimingMediator::TriggerSetupRuntimeCallback(
    const TimingInfo& timing_info) const {
  auto timing = timing_info.GetAllTimingInfoAsMicrosecond();
  lepus::Value lepus_timing =
      pub::ValueUtils::ConvertValueToLepusValue(*timing);

  // Runtime OnTimingSetup Callback.
  if (runtime_actor_ && enable_js_runtime_) {
    runtime_actor_->ActAsync([timing = std::move(lepus_timing)](auto& runtime) {
      TRACE_EVENT(LYNX_TRACE_CATEGORY,
                  "TimingMediator::TriggerSetupRuntimeCallback");
      auto args = lepus::CArray::Create();
      args->emplace_back(BASE_STATIC_STRING(kSetupRuntimeCallback));
      args->emplace_back((lepus_value::ShallowCopy(timing)));
      runtime::MessageEvent event(runtime::kMessageEventTypeSendGlobalEvent,
                                  runtime::ContextProxy::Type::kCoreContext,
                                  runtime::ContextProxy::Type::kJSContext,
                                  lepus::Value(std::move(args)));
      runtime->OnReceiveMessageEvent(std::move(event));
    });
  } else if (engine_actor_ && timing_info.GetEnableAirStrictMode()) {
    engine_actor_->ActAsync(
        [timing = std::move(lepus_timing)](auto& engine) mutable {
          TRACE_EVENT(LYNX_TRACE_CATEGORY,
                      "TimingMediator::TriggerSetupEngineCallback");
          auto arguments = lepus::CArray::Create();
          arguments->emplace_back(std::move(timing));
          engine->TriggerEventBus(kSetupRuntimeCallback,
                                  lepus_value(std::move(arguments)));
        });
  }
}

void TimingMediator::ReportSetupEvent(const TimingInfo& timing_info) const {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TimingMediator::ReportSetupEvent");
  tasm::report::MoveOnlyEvent event;
  event.SetName(kLynxSDKSetupTiming);
  auto timing = timing_info.GetAllTimingInfoAsMicrosecond();
  lepus::Value lepus_timing =
      pub::ValueUtils::ConvertValueToLepusValue(*timing);

  auto metrics = lepus_timing.GetProperty(BASE_STATIC_STRING(kMetrics));
  auto setup_timing =
      lepus_timing.GetProperty(BASE_STATIC_STRING(kSetupTiming));
  auto extra_timing =
      lepus_timing.GetProperty(BASE_STATIC_STRING(kExtraTiming));
  tasm::ForEachLepusValue(
      metrics, [&event](const lepus::Value& key, const lepus::Value& value) {
        if (key.IsString() && value.IsNumber()) {
          event.SetProps(key.ToString().c_str(), value.Number());
        }
      });

  event.SetProps(kCreateLynxView,
                 CalculateDuration(
                     setup_timing, BASE_STATIC_STRING(kCreateLynxStartPolyfill),
                     setup_timing, BASE_STATIC_STRING(kCreateLynxEndPolyfill)));

  event.SetProps(
      kPrepareTemplate,
      CalculateDuration(
          extra_timing, BASE_STATIC_STRING(kPrepareTemplateStartPolyfill),
          extra_timing, BASE_STATIC_STRING(kPrepareTemplateEndPolyfill)));
  event.SetProps(kLoadTemplate,
                 CalculateDuration(
                     setup_timing, BASE_STATIC_STRING(kLoadBundleStartPolyfill),
                     setup_timing, BASE_STATIC_STRING(kLoadBundleEndPolyfill)));
  event.SetProps(
      kDecode,
      CalculateDuration(setup_timing, BASE_STATIC_STRING(kParseStartPolyfill),
                        setup_timing, BASE_STATIC_STRING(kParseEndPolyfill)));
  event.SetProps(kSetupLepusExecute,
                 CalculateDuration(
                     setup_timing, BASE_STATIC_STRING(kVmExecuteStartPolyfill),
                     setup_timing, BASE_STATIC_STRING(kVmExecuteEndPolyfill)));
  event.SetProps(
      kSetupDataProcessor,
      CalculateDuration(
          setup_timing, BASE_STATIC_STRING(kDataProcessorStartPolyfill),
          setup_timing, BASE_STATIC_STRING(kDataProcessorEndPolyfill)));
  event.SetProps(
      kSetupSetInitData,
      CalculateDuration(
          setup_timing, BASE_STATIC_STRING(kSetInitDataStartPolyfill),
          setup_timing, BASE_STATIC_STRING(kSetInitDataEndPolyfill)));
  event.SetProps(kSetupCreateVDom,
                 CalculateDuration(
                     setup_timing, BASE_STATIC_STRING(kMtsRenderStartPolyfill),
                     setup_timing, BASE_STATIC_STRING(kMtsRenderEndPolyfill)));
  event.SetProps(
      kSetupDispatch,
      CalculateDuration(setup_timing, BASE_STATIC_STRING(kResolveStartPolyfill),
                        setup_timing, BASE_STATIC_STRING(kResolveEndPolyfill)));
  event.SetProps(
      kSetupLayout,
      CalculateDuration(setup_timing, BASE_STATIC_STRING(kLayoutStartPolyfill),
                        setup_timing, BASE_STATIC_STRING(kLayoutEndPolyfill)));
  event.SetProps(
      kSetupUiOperationFlush,
      CalculateDuration(
          setup_timing,
          BASE_STATIC_STRING(kPaintingUiOperationExecuteStartPolyfill),
          setup_timing,
          BASE_STATIC_STRING(kLayoutUiOperationExecuteEndPolyfill)));
  event.SetProps(
      kSetupPaintingUiOperationFlush,
      CalculateDuration(
          setup_timing,
          BASE_STATIC_STRING(kPaintingUiOperationExecuteStartPolyfill),
          setup_timing,
          BASE_STATIC_STRING(kPaintingUiOperationExecuteEndPolyfill)));
  event.SetProps(kSetupLayoutUiOperationFlush,
                 CalculateDuration(
                     setup_timing,
                     BASE_STATIC_STRING(kLayoutUiOperationExecuteStartPolyfill),
                     setup_timing,
                     BASE_STATIC_STRING(kLayoutUiOperationExecuteEndPolyfill)));
  event.SetProps(kLoadCore,
                 CalculateDuration(
                     setup_timing, BASE_STATIC_STRING(kLoadCoreStartPolyfill),
                     setup_timing, BASE_STATIC_STRING(kLoadCoreEndPolyfill)));
  event.SetProps(
      kLoadApp,
      CalculateDuration(
          setup_timing, BASE_STATIC_STRING(kLoadBackgroundStartPolyfill),
          setup_timing, BASE_STATIC_STRING(kLoadBackgroundEndPolyfill)));
  event.SetProps(
      kSetupDrawWaiting,
      std::max(CalculateDuration(
                   setup_timing, BASE_STATIC_STRING(kLoadBundleEndPolyfill),
                   setup_timing, BASE_STATIC_STRING(kPaintEndPolyfill)),
               CalculateDuration(
                   setup_timing,
                   BASE_STATIC_STRING(kLayoutUiOperationExecuteEndPolyfill),
                   setup_timing, BASE_STATIC_STRING(kPaintEndPolyfill))));

  event.SetProps(kListRenderChildren,
                 CalculateDuration(
                     setup_timing, BASE_STATIC_STRING(kListRenderChildrenStart),
                     setup_timing, BASE_STATIC_STRING(kListRenderChildrenEnd)));

  event.SetProps(
      kSetupLoadTemplateWaiting,
      CalculateDuration(
          setup_timing, BASE_STATIC_STRING(kCreateLynxEndPolyfill),
          setup_timing, BASE_STATIC_STRING(kLoadBundleStartPolyfill)));
  event.SetProps(
      kHasReload,
      lepus_timing.GetProperty(BASE_STATIC_STRING(kHasReload)).Bool());
  event.SetProps(kUseNativeTiming, 1);

  event.SetProps(
      kTemplateBundleDecode,
      CalculateDuration(
          setup_timing, BASE_STATIC_STRING(kTemplateBundleParseStartPolyfill),
          setup_timing, BASE_STATIC_STRING(kTemplateBundleParseEndPolyfill)));

  tasm::report::EventTrackerPlatformImpl::OnEvent(instance_id_,
                                                  std::move(event));
}

// OnTimingUpdate callback
void TimingMediator::TriggerUpdateClientCallback(
    const TimingInfo& timing_info, const std::string& update_flag) const {
  // Platform OnTimingUpdate Callback.
  // OnTimingUpdate callback for client returns all of the updateTimings.
  if (facade_actor_) {
    auto all_timing = timing_info.GetAllTimingInfoAsMillisecond();
    lepus::Value all_lepus_timing =
        pub::ValueUtils::ConvertValueToLepusValue(*all_timing);
    auto update_timing =
        timing_info.GetUpdateTimingInfoAsMillisecond(update_flag);
    lepus::Value update_lepus_timing =
        pub::ValueUtils::ConvertValueToLepusValue(*update_timing);

    facade_actor_->ActAsync([timing = std::move(all_lepus_timing),
                             update_timing = std::move(update_lepus_timing),
                             update_flag](auto& facade) {
      // TODO(kechenglong): set timingHandler PropBundleCreator before
      //  TimingHandler ctor, covert lepus to platform data structure
      //  in timing thread.
      TRACE_EVENT(
          LYNX_TRACE_CATEGORY, "TimingMediator::TriggerUpdateClientCallback",
          [&update_flag](lynx::perfetto::EventContext ctx) {
            ctx.event()->add_debug_annotations("timing_flag", update_flag);
          });
      facade->OnTimingUpdate(timing, update_timing, update_flag);
    });
  }
}

void TimingMediator::TriggerUpdateRuntimeCallback(
    const TimingInfo& timing_info, const std::string& update_flag) const {
  auto all_timing = timing_info.GetAllTimingInfoAsMicrosecond();
  auto update_timing =
      timing_info.GetUpdateTimingInfoAsMicrosecond(update_flag);
  lepus::Value all_lepus_timing =
      pub::ValueUtils::ConvertValueToLepusValue(*all_timing);
  lepus::Value update_lepus_timing =
      pub::ValueUtils::ConvertValueToLepusValue(*update_timing);

  all_lepus_timing.Table()->SetValue(BASE_STATIC_STRING(kUpdateTimings),
                                     std::move(update_lepus_timing));
  // Runtime OnTimingUpdate Callback.
  // OnTimingUpdate callback for runtime only returns the updateFlag related
  // updateTimings.
  if (runtime_actor_ && enable_js_runtime_) {
    runtime_actor_->ActAsync([timing = std::move(all_lepus_timing),
                              update_flag](auto& runtime) {
      TRACE_EVENT(
          LYNX_TRACE_CATEGORY, "TimingMediator::TriggerUpdateRuntimeCallback",
          [&update_flag](lynx::perfetto::EventContext ctx) {
            ctx.event()->add_debug_annotations("timing_flag", update_flag);
          });
      auto args = lepus::CArray::Create();
      args->emplace_back(BASE_STATIC_STRING(kUpdateRuntimeCallback));
      args->emplace_back(lepus_value::ShallowCopy(timing));
      runtime::MessageEvent event(runtime::kMessageEventTypeSendGlobalEvent,
                                  runtime::ContextProxy::Type::kCoreContext,
                                  runtime::ContextProxy::Type::kJSContext,
                                  lepus::Value(std::move(args)));
      runtime->OnReceiveMessageEvent(std::move(event));
    });
  } else if (engine_actor_ && timing_info.GetEnableAirStrictMode()) {
    engine_actor_->ActAsync([timing = std::move(all_lepus_timing),
                             update_flag](auto& engine) {
      TRACE_EVENT(
          LYNX_TRACE_CATEGORY, "TimingMediator::TriggerUpdateEngineCallback",
          [&update_flag](lynx::perfetto::EventContext ctx) {
            ctx.event()->add_debug_annotations("timing_flag", update_flag);
          });
      auto arguments = lepus::CArray::Create();
      arguments->emplace_back(std::move(timing));
      engine->TriggerEventBus(kUpdateRuntimeCallback,
                              lepus_value(std::move(arguments)));
    });
  }
}

void TimingMediator::ReportUpdateEvent(const TimingInfo& timing_info,
                                       const std::string& update_flag) const {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TimingMediator::ReportUpdateEvent",
              [&update_flag](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations("timing_flag", update_flag);
              });
  tasm::report::MoveOnlyEvent event;
  event.SetName(kLynxSDKUpdateTiming);
  auto timing = timing_info.GetAllTimingInfoAsMicrosecond();
  lepus::Value all_lepus_timing =
      pub::ValueUtils::ConvertValueToLepusValue(*timing);
  auto update_timing =
      timing_info.GetUpdateTimingInfoAsMicrosecond(update_flag);
  lepus::Value update_lepus_timing =
      pub::ValueUtils::ConvertValueToLepusValue(*update_timing);
  auto update_lepus_timing_dict = update_lepus_timing.GetProperty(update_flag);

  auto metrics = all_lepus_timing.GetProperty(BASE_STATIC_STRING(kMetrics));
  auto setup_timing =
      all_lepus_timing.GetProperty(BASE_STATIC_STRING(kSetupTiming));
  tasm::ForEachLepusValue(
      metrics, [&event](const lepus::Value& key, const lepus::Value& value) {
        if (key.IsString() && value.IsNumber()) {
          event.SetProps(key.ToString().c_str(), value.Number());
        }
      });
  event.SetProps(kUpdateCreateVDom,
                 CalculateDuration(update_lepus_timing_dict,
                                   BASE_STATIC_STRING(kMtsRenderStartPolyfill),
                                   update_lepus_timing_dict,
                                   BASE_STATIC_STRING(kMtsRenderEndPolyfill)));
  event.SetProps(kUpdateDispatch,
                 CalculateDuration(update_lepus_timing_dict,
                                   BASE_STATIC_STRING(kResolveStartPolyfill),
                                   update_lepus_timing_dict,
                                   BASE_STATIC_STRING(kResolveEndPolyfill)));
  event.SetProps(kUpdateLayout,
                 CalculateDuration(update_lepus_timing_dict,
                                   BASE_STATIC_STRING(kLayoutStartPolyfill),
                                   update_lepus_timing_dict,
                                   BASE_STATIC_STRING(kLayoutEndPolyfill)));
  event.SetProps(
      kUpdateUiOperationFlush,
      CalculateDuration(
          update_lepus_timing_dict,
          BASE_STATIC_STRING(kPaintingUiOperationExecuteStartPolyfill),
          update_lepus_timing_dict,
          BASE_STATIC_STRING(kLayoutUiOperationExecuteEndPolyfill)));
  event.SetProps(
      kUpdatePaintingUiOperationFlush,
      CalculateDuration(
          update_lepus_timing_dict,
          BASE_STATIC_STRING(kPaintingUiOperationExecuteStartPolyfill),
          update_lepus_timing_dict,
          BASE_STATIC_STRING(kPaintingUiOperationExecuteEndPolyfill)));
  event.SetProps(kUpdateLayoutUiOperationFlush,
                 CalculateDuration(
                     update_lepus_timing_dict,
                     BASE_STATIC_STRING(kLayoutUiOperationExecuteStartPolyfill),
                     update_lepus_timing_dict,
                     BASE_STATIC_STRING(kLayoutUiOperationExecuteEndPolyfill)));
  event.SetProps(
      kUpdateDrawWaiting,
      CalculateDuration(
          update_lepus_timing_dict,
          BASE_STATIC_STRING(kLayoutUiOperationExecuteEndPolyfill),
          update_lepus_timing_dict, BASE_STATIC_STRING(kPaintEndPolyfill)));
  std::string update_start_key = kSetStateTrigger;
  if (update_lepus_timing_dict
          .GetProperty(BASE_STATIC_STRING(kPipelineStartPolyfill))
          .Number() > 0) {
    update_start_key = kPipelineStartPolyfill;
  }
  event.SetProps(
      kUpdateTriggerWaiting,
      CalculateDuration(update_lepus_timing_dict, update_start_key,
                        update_lepus_timing_dict,
                        BASE_STATIC_STRING(kMtsRenderStartPolyfill)));
  event.SetProps(
      kUpdateWaiting,
      CalculateDuration(setup_timing, BASE_STATIC_STRING(kPaintEndPolyfill),
                        update_lepus_timing_dict, update_start_key));

  event.SetProps(kUpdateTiming,
                 CalculateDuration(update_lepus_timing_dict, update_start_key,
                                   update_lepus_timing_dict,
                                   BASE_STATIC_STRING(kPaintEndPolyfill)));

  event.SetProps(kLoadTemplateToUpdateDrawEnd,
                 CalculateDuration(setup_timing,
                                   BASE_STATIC_STRING(kLoadBundleStartPolyfill),
                                   update_lepus_timing,
                                   BASE_STATIC_STRING(kPaintEndPolyfill)));

  event.SetProps(kListRenderChildren,
                 CalculateDuration(update_lepus_timing_dict,
                                   BASE_STATIC_STRING(kListRenderChildrenStart),
                                   update_lepus_timing_dict,
                                   BASE_STATIC_STRING(kListRenderChildrenEnd)));

  event.SetProps(kListPatchChanges,
                 CalculateDuration(update_lepus_timing_dict,
                                   BASE_STATIC_STRING(kListPatchChangesStart),
                                   update_lepus_timing_dict,
                                   BASE_STATIC_STRING(kListPatchChangesEnd)));

  event.SetProps(kListDiffVdom,
                 CalculateDuration(update_lepus_timing_dict,
                                   BASE_STATIC_STRING(kListDiffVdomStart),
                                   update_lepus_timing_dict,
                                   BASE_STATIC_STRING(kListDiffVdomEnd)));

  event.SetProps(kUpdateFlag, update_flag);
  event.SetProps(
      kHasReload,
      all_lepus_timing.GetProperty(BASE_STATIC_STRING(kHasReload)).Bool());
  event.SetProps(kUseNativeTiming, 1);

  tasm::report::EventTrackerPlatformImpl::OnEvent(instance_id_,
                                                  std::move(event));
}

}  // namespace timing
}  // namespace tasm
}  // namespace lynx
