// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/lynx_trace_categories.h"

#ifndef CORE_SERVICES_TRACE_SERVICE_TRACE_EVENT_DEF_H_
#define CORE_SERVICES_TRACE_SERVICE_TRACE_EVENT_DEF_H_
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE

/**
 * @trace_description: Send reporting tasks, such as Timing reports and
 * LongTask detection reports, to the asynchronous thread.
 */
static constexpr const char* const EVENT_TRACKER_FLUSH = "EventTracker::Flush";

static constexpr const char* const FEATURE_COUNT_REPORTER_START_TIMER =
    "FeatureCountReporter::StartTimer";
static constexpr const char* const LONG_TASK_TIMING_REPORT_LONG_TASK =
    "LongTaskTiming::ReportLongTask";
static constexpr const char* const TIMING_SET_NEED_MARK_DRAW_END =
    "Timing::SetNeedMarkDrawEndTiming";
static constexpr const char* const TIMING_MARK = "Timing::Mark.";
/**
 * @trace_description: The setup phase callback of timing-api for native
 * developer
 */
static constexpr const char* const
    TIMING_MEDIATOR_TRIGGER_SETUP_CLIENT_CALLBACK =
        "TimingMediator::TriggerSetupClientCallback";
static constexpr const char* const
    TIMING_MEDIATOR_TRIGGER_SETUP_RUNTIME_CALLBACK =
        "TimingMediator::TriggerSetupRuntimeCallback";
static constexpr const char* const
    TIMING_MEDIATOR_TRIGGER_SETUP_ENGINE_CALLBACK =
        "TimingMediator::TriggerSetupEngineCallback";
static constexpr const char* const TIMING_MEDIATOR_REPORT_SETUP_EVENT =
    "TimingMediator::ReportSetupEvent";
static constexpr const char* const TIMING_MEDIATOR_REPORT_UPDATE_EVENT =
    "TimingMediator::ReportUpdateEvent";
static constexpr const char* const
    TIMING_MEDIATOR_REPORT_UPDATE_CLIENT_CALLBACK =
        "TimingMediator::TriggerUpdateClientCallback";
static constexpr const char* const
    TIMING_MEDIATOR_TRIGGER_UPDATE_RUNTIME_CALLBACK =
        "TimingMediator::TriggerUpdateRuntimeCallback";
static constexpr const char* const
    TIMING_MEDIATOR_TRIGGER_UPDATE_ENGINE_CALLBACK =
        "TimingMediator::TriggerUpdateEngineCallback";
static constexpr const char* const TIMING_MARK_FRAME_WORK_TIMING =
    "Timing::MarkFrameWorkTiming.";

static constexpr const char* const PERFORMANCE_MEDIATOR_ON_PERFORMANCE_EVENT =
    "PerformanceMediator::OnPerformanceEvent";
static constexpr const char* const
    PERFORMANCE_MEDIATOR_ON_PERFORMANCE_EVENT_MTS_ENGINE =
        "PerformanceMediator::OnPerformanceEventMTSEngine";
static constexpr const char* const
    PERFORMANCE_MEDIATOR_ON_PERFORMANCE_EVENT_BTS_ENGINE =
        "PerformanceMediator::OnPerformanceEventBTSEngine";
/// FSP
static constexpr const char* const FSP_TIMING_MARK_FSP_END =
    "Timing::Mark.fspEnd";
static constexpr const char* const FSP_SNAPSHOT_BASE_64_BITMAP =
    "FSPSnapshotBase64Bitmap";
static constexpr const char* const FSP_SNAPSHOT_BASE_64_BITMAP_PROP_DATA =
    "data";
static constexpr const char* const FSP_TRACER_START = "FSPTracer::Start";
static constexpr const char* const FSP_TRACER_STOP = "FSPTracer::Stop";
static constexpr const char* const FSP_TRACER_ON_VALUABLE_SNAPSHOT =
    "FSPTracer::OnValuableSnapshot";
static constexpr const char* const FSP_TRACER_SNAPSHOT_STABLE =
    "FSPTracer::SnapshotStable";
static constexpr const char* const FSP_TRACER_UPDATE_PREVIOUS_SNAPSHOT_STABLE =
    "FSPTracer::UpdatePreviousSnapshotStable";
static constexpr const char* const FSP_CONTAINER_SIZE = "containerSize";
static constexpr const char* const FSP_TOTAL_PRESENTED_CONTENT_AREA =
    "totalPresentedContentArea";
static constexpr const char* const FSP_LAST_CHANGE_TIMESTAMP_US =
    "lastChangeTimestampUs";
#endif  // #if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE

#endif  // CORE_SERVICES_TRACE_SERVICE_TRACE_EVENT_DEF_H_
