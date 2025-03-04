// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/tracing/frame_trace_service.h"

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"

namespace lynx {
namespace trace {

FrameTraceService::FrameTraceService() : thread_("CrBrowserMain") {}

void FrameTraceService::Initialize() {
  thread_.GetTaskRunner()->PostTask([] {
    TRACE_EVENT(
        LYNX_TRACE_CATEGORY_DEVTOOL_TIMELINE, "TracingStartedInBrowser",
        [](lynx::perfetto::EventContext ctx) {
          auto* legacy_event = ctx.event()->set_legacy_event();
          legacy_event->set_phase('I');
          legacy_event->set_unscoped_id(1);
          auto* debug = ctx.event()->add_debug_annotations();
          debug->set_name("data");
          std::string data =
              R"({"frameTreeNodeId":"", "frames":[{"frame":"","name":"","processId":)";
          // just a placeholder for protocol
          data += "0";
          data += R"(,"url":""}],"persistentIds":true})";
          debug->set_legacy_json_value(data);
        });
    TRACE_EVENT(LYNX_TRACE_CATEGORY_DEVTOOL_TIMELINE, "SetLayerTreeId",
                [](lynx::perfetto::EventContext ctx) {
                  auto* legacy_event = ctx.event()->set_legacy_event();
                  legacy_event->set_phase('I');
                  legacy_event->set_unscoped_id(1);
                  auto* debug = ctx.event()->add_debug_annotations();
                  debug->set_name("data");
                  const std::string data = R"({"frame":"", "layerTreeId":1})";
                  debug->set_legacy_json_value(data);
                });
  });
}

void FrameTraceService::SendScreenshots(const std::string& snapshot) {
  thread_.GetTaskRunner()->PostTask(
      [self = shared_from_this(), snapshot]() { self->Screenshots(snapshot); });
}

void FrameTraceService::Screenshots(const std::string& snapshot) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY_SCREENSHOTS, "Screenshot",
              [&snapshot](lynx::perfetto::EventContext ctx) {
                auto* legacy_event = ctx.event()->set_legacy_event();
                legacy_event->set_phase('O');
                legacy_event->set_unscoped_id(1);
                auto* debug = ctx.event()->add_debug_annotations();
                debug->set_name("snapshot");
                debug->set_string_value(snapshot);
              });
}

void FrameTraceService::SendFPSData(const uint64_t& startTime,
                                    const uint64_t& endTime) {
  thread_.GetTaskRunner()->PostTask(
      [self = shared_from_this(), startTime, endTime]() {
        self->FPSTrace(startTime, endTime);
      });
}

void FrameTraceService::FPSTrace(const uint64_t& startTime,
                                 const uint64_t& endTime) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY_FPS, "NeedsBeginFrameChanged", startTime,
              [](lynx::perfetto::EventContext ctx) {
                auto* legacy_event = ctx.event()->set_legacy_event();
                legacy_event->set_phase('I');
                legacy_event->set_unscoped_id(1);
                auto* dataDebug = ctx.event()->add_debug_annotations();
                const std::string data = R"({"needsBeginFrame":1})";
                dataDebug->set_name("data");
                dataDebug->set_legacy_json_value(data);
                auto* idDebug = ctx.event()->add_debug_annotations();
                idDebug->set_name("layerTreeId");
                idDebug->set_int_value(1);
              });
  TRACE_EVENT(LYNX_TRACE_CATEGORY_FPS, "BeginFrame", startTime,
              [](lynx::perfetto::EventContext ctx) {
                auto* legacy_event = ctx.event()->set_legacy_event();
                legacy_event->set_phase('I');
                legacy_event->set_unscoped_id(1);
                auto* idDebug = ctx.event()->add_debug_annotations();
                idDebug->set_name("layerTreeId");
                idDebug->set_int_value(1);
              });
  TRACE_EVENT(LYNX_TRACE_CATEGORY_FPS, "DrawFrame", endTime,
              [endTime](lynx::perfetto::EventContext ctx) {
                auto* legacy_event = ctx.event()->set_legacy_event();
                legacy_event->set_phase('b');
                legacy_event->set_unscoped_id(1);
                auto* dataDebug = ctx.event()->add_debug_annotations();
                dataDebug->set_name("presentationTimestamp");
                dataDebug->set_int_value(endTime / 1000);
                auto* idDebug = ctx.event()->add_debug_annotations();
                idDebug->set_name("layerTreeId");
                idDebug->set_int_value(1);
              });
  TRACE_EVENT(LYNX_TRACE_CATEGORY_FPS, "DrawFrame", endTime,
              [](lynx::perfetto::EventContext ctx) {
                auto* legacy_event = ctx.event()->set_legacy_event();
                legacy_event->set_phase('e');
                legacy_event->set_unscoped_id(1);
              });
}

}  // namespace trace
}  // namespace lynx
#endif
