// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/event_report/event_tracker.h"

#include <map>

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/services/event_report/event_tracker_platform_impl.h"

namespace lynx {
namespace tasm {
namespace report {
// targetSdkVersion set by FE.
constexpr const static char* kLynxTargetSDKVersion = "lynx_target_sdk_version";
// lynx_dsl could be ttml, react, react_nodiff or unkown.
constexpr const static char* kLynxDSL = "lynx_dsl";
// lepus_type could be lepus or lepusNG.
constexpr const static char* kLynxLepusType = "lynx_lepus_type";
// template's page version set by FE.
constexpr const static char* kLynxPageVersion = "lynx_page_version";
// Property value of lepusNG.
constexpr const static char* kLynxLepusNG = "lepusNG";
// Property value of lepus.
constexpr const static char* kLynxLepus = "lepus";
// Is lynx air enable.
constexpr const static char* kLynxEnableAir = "enable_air";
// Is lynx nodiff enable.
constexpr const static char* kLynxEnableFiberArch = "enable_no_diff";

namespace {
std::string GetDSLName(const std::shared_ptr<tasm::PageConfig>& config) {
  switch (config->GetLynxAirMode()) {
    case lynx::tasm::CompileOptionAirMode::AIR_MODE_OFF:
      break;
    case lynx::tasm::CompileOptionAirMode::AIR_MODE_FIBER:
      return "ttml_air_fiber";
    case lynx::tasm::CompileOptionAirMode::AIR_MODE_STRICT:
      return "ttml_air_strict";
    case lynx::tasm::CompileOptionAirMode::AIR_MODE_TTML_WITHOUT_JS:
      return "ttml_air_without_js";
    case lynx::tasm::CompileOptionAirMode::AIR_MODE_NATIVE_SCRIPT:
      return "ttml_air_native_script";
  }
  if (config->GetEnableFiberArch()) {
    if (config->GetDSL() == PackageInstanceDSL::TT) {
      return "ttml_nodiff";
    } else {
      return "reactlynx3";
    }
  } else {
    if (config->GetDSL() == PackageInstanceDSL::TT) {
      return "ttml_radondiff";
    } else {
      return "reactlynx2";
    }
  }
}
}  // namespace

EventTracker* EventTracker::Instance() {
  static thread_local EventTracker instance_;
  return &instance_;
}

void EventTracker::OnEvent(EventBuilder builder) {
  EventTracker* instance = EventTracker::Instance();
  instance->tracker_event_builder_stack_.push_back(std::move(builder));
}

void EventTracker::UpdateGenericInfoByPageConfig(
    int32_t instance_id, const std::shared_ptr<tasm::PageConfig>& config) {
  // the unique id of template instance.
  // instance_id is a value greater than or equal to 0.
  // If not actively initialized when LynxActor<T> is created, the default value
  // is -1.
  if (instance_id < 0) {
    return;
  }
  EventTrackerPlatformImpl::GetReportTaskRunner()->PostTask([instance_id,
                                                             config]() {
    std::unordered_map<std::string, std::string> info;
    info.insert({kLynxEnableAir, std::to_string(config->GetEnableLynxAir())});
    info.insert(
        {kLynxEnableFiberArch, std::to_string(config->GetEnableFiberArch())});
    info.insert({kLynxTargetSDKVersion, config->GetTargetSDKVersion()});
    auto dsl = GetDSLName(config);
    info.insert({kLynxDSL, dsl});
    info.insert({kLynxLepusType,
                 config->GetEnableLepusNG() ? kLynxLepusNG : kLynxLepus});
    info.insert({kLynxPageVersion, config->GetVersion()});
    EventTrackerPlatformImpl::UpdateGenericInfo(instance_id, std::move(info));
  });
}

void EventTracker::UpdateGenericInfo(int32_t instance_id, std::string key,
                                     std::string value) {
  // the unique id of template instance.
  // instance_id is a value greater than or equal to 0.
  // If not actively initialized when LynxActor<T> is created, the default value
  // is -1.
  if (instance_id < 0) {
    return;
  }
  EventTrackerPlatformImpl::GetReportTaskRunner()->PostTask(
      [instance_id, key = std::move(key), value = std::move(value)]() mutable {
        EventTrackerPlatformImpl::UpdateGenericInfo(instance_id, std::move(key),
                                                    std::move(value));
      });
}

void EventTracker::UpdateGenericInfo(int32_t instance_id, std::string key,
                                     int64_t value) {
  // the unique id of template instance.
  // instance_id is a value greater than or equal to 0.
  // If not actively initialized when LynxActor<T> is created, the default value
  // is -1.
  if (instance_id < 0) {
    return;
  }
  EventTrackerPlatformImpl::GetReportTaskRunner()->PostTask(
      [instance_id, key = std::move(key), value]() mutable {
        EventTrackerPlatformImpl::UpdateGenericInfo(instance_id, std::move(key),
                                                    value);
      });
}

void EventTracker::UpdateGenericInfo(int32_t instance_id, std::string key,
                                     float value) {
  // the unique id of template instance.
  // instance_id is a value greater than or equal to 0.
  // If not actively initialized when LynxActor<T> is created, the default value
  // is -1.
  if (instance_id < 0) {
    return;
  }
  EventTrackerPlatformImpl::GetReportTaskRunner()->PostTask(
      [instance_id, key = std::move(key), value = value]() mutable {
        EventTrackerPlatformImpl::UpdateGenericInfo(instance_id, std::move(key),
                                                    value);
      });
}

void EventTracker::ClearCache(int32_t instance_id) {
  // the unique id of template instance.
  // instance_id is a value greater than or equal to 0.
  // If not actively initialized when LynxActor<T> is created, the default value
  // is -1.
  if (instance_id < 0) {
    return;
  }
  EventTrackerPlatformImpl::GetReportTaskRunner()->PostTask(
      [instance_id]() { EventTrackerPlatformImpl::ClearCache(instance_id); });
}

void EventTracker::Flush(int32_t instance_id) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "EventTracker::Flush",
              [&](perfetto::EventContext ctx) {
                auto* debug = ctx.event()->add_debug_annotations();
                debug->set_name("instance_id");
                debug->set_string_value(std::to_string(instance_id));
              });
  EventTracker* instance = EventTracker::Instance();
  // the unique id of template instance.
  // instance_id is a value greater than or equal to 0.
  // If not actively initialized when LynxActor<T> is created, the default value
  // is -1.
  if (instance->tracker_event_builder_stack_.empty() || instance_id < 0) {
    return;
  }

  // Most cases tracker_event_builder_stack_ contains single builder.
  // If so, move the single builder only so that tracker_event_builder_stack_'s
  // allocated buffer is not affected.
  if (instance->tracker_event_builder_stack_.size() == 1) {
    EventTrackerPlatformImpl::GetReportTaskRunner()->PostTask(
        [instance_id,
         builder = std::move(
             instance->tracker_event_builder_stack_.front())]() mutable {
          MoveOnlyEvent event;
          builder(event);
          EventTrackerPlatformImpl::OnEvent(instance_id, std::move(event));
        });
    // tracker_event_builder_stack_'s buffer and capacity is not affected.
    instance->tracker_event_builder_stack_.clear();
  } else {
    EventTrackerPlatformImpl::GetReportTaskRunner()->PostTask(
        [instance_id, builder_stack = std::move(
                          instance->tracker_event_builder_stack_)]() mutable {
          std::vector<MoveOnlyEvent> stack;
          stack.reserve(builder_stack.size());
          for (const auto& builder : builder_stack) {
            builder(stack.emplace_back());
          }
          EventTrackerPlatformImpl::OnEvents(instance_id, std::move(stack));
        });
  }
}
}  // namespace report
}  // namespace tasm
}  // namespace lynx
