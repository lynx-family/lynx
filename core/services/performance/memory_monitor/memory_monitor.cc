// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/performance/memory_monitor/memory_monitor.h"

#include <algorithm>
#include <cstdio>
#include <limits>
#include <random>
#include <string_view>
#include <utility>

#include "base/include/string/string_number_convert.h"
#include "base/trace/native/trace_controller.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/services/event_report/event_tracker_platform_impl.h"
#include "core/services/performance/memory_monitor/global_memory_monitor.h"
#if ENABLE_TRACE_PERFETTO
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"
#endif

namespace lynx {
namespace tasm {
namespace performance {
namespace {

#if ENABLE_TRACE_PERFETTO
constexpr char kForceEnableMarkerName[] = ".lynx-memory-monitor-force-enabled";

std::string ForceEnableMarkerPath() {
  auto directory =
      trace::GetTraceControllerInstance()->GenerateTracingFileDir();
  if (directory.empty()) return {};
  if (directory.back() != '/' && directory.back() != '\\') {
    directory.push_back('/');
  }
  return directory + kForceEnableMarkerName;
}

bool HasForceEnableMarker() {
  const auto path = ForceEnableMarkerPath();
  if (path.empty()) return false;
  auto* file = std::fopen(path.c_str(), "rb");
  if (!file) return false;
  std::fclose(file);
  return true;
}

void PersistForceEnableMarker() {
  const auto path = ForceEnableMarkerPath();
  if (path.empty()) return;
  auto* file = std::fopen(path.c_str(), "ab");
  if (file) std::fclose(file);
}
#else
bool HasForceEnableMarker() { return false; }

void PersistForceEnableMarker() {}
#endif

void EnableAllMemorySources(MemoryMonitor::Settings& settings) {
  for (auto* entry :
       {static_cast<MemoryMonitor::Settings::Entry*>(&settings.global),
        &settings.lynx_ui, &settings.page_scheduled}) {
    entry->enabled = 1;
    entry->sample_rate = 1.0f;
  }
}

int SourceIndex(const std::string& category) {
  if (category == kCategoryTasmElement) return 0;
  if (category == kCategoryMTSEngine) return 1;
  if (category == kCategoryBTSEngine) return 2;
  return 3;  // Platform UI producers use per-widget category names.
}

#if ENABLE_TRACE_PERFETTO
std::string MemoryRecordToJson(const MemoryRecord& record) {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  writer.StartObject();
  writer.Key(kCategory);
  writer.String(record.category_.data(),
                static_cast<rapidjson::SizeType>(record.category_.size()));
  writer.Key(kSizeBytes);
  writer.Int64(record.size_bytes_);
  writer.Key(kInstanceCount);
  writer.Int(record.instance_count_);
  if (record.detail_) {
    writer.Key(kDetail);
    writer.StartObject();
    for (const auto& [key, value] : *record.detail_) {
      writer.Key(key.data(), static_cast<rapidjson::SizeType>(key.size()));
      writer.String(value.data(),
                    static_cast<rapidjson::SizeType>(value.size()));
    }
    writer.EndObject();
  }
  writer.EndObject();
  return {buffer.GetString(), buffer.GetSize()};
}
#endif

struct InitOnceMemoryMonitorSettings : MemoryMonitor::Settings {
  InitOnceMemoryMonitorSettings() {
    auto config = LynxEnv::GetInstance().GetStringEnv(
        LynxEnv::Key::MEMORY_MONITOR_CONFIG);
    ApplyConfig(config ? std::string_view(*config) : std::string_view{});
    std::mt19937 random(std::random_device{}());
    std::uniform_real_distribution<float> distribution(0, 1);
    global.enabled = distribution(random) < global.sample_rate;
    for (auto* entry : {&lynx_ui, &page_scheduled}) {
      entry->enabled =
          global.enabled && distribution(random) < entry->sample_rate;
    }
  }
};

}  // namespace

void MemoryMonitor::Settings::ApplyConfig(std::string_view config) {
  while (!config.empty()) {
    const auto key_end = config.find(',');
    if (key_end == std::string_view::npos) break;
    const auto key = config.substr(0, key_end);
    config.remove_prefix(key_end + 1);

    const auto value_end = config.find(',');
    const auto value = config.substr(0, value_end);
    config = value_end == std::string_view::npos ? std::string_view{}
                                                 : config.substr(value_end + 1);

    if (key == "global_sample_rate" || key == "lynx_ui_sample_rate" ||
        key == "page_scheduled_sample_rate") {
      float rate;
      if (base::StringToFloat(std::string(value), rate, true)) {
        (key == "global_sample_rate"    ? global
         : key == "lynx_ui_sample_rate" ? lynx_ui
                                        : page_scheduled)
            .sample_rate = rate;
      }
      continue;
    }

    int64_t parsed;
    if (!base::StringToInt(std::string(value), parsed)) {
      continue;
    }
    const auto number = static_cast<uint32_t>(parsed);
    if (key == "time_interval_sec") {
      global.time_interval_sec = number;
    } else if (key == "pss_threshold_mb") {
      global.pss_threshold_mb = number;
    } else if (key == "mts_threshold_mb") {
      global.mts_threshold_mb = number;
    } else if (key == "bts_threshold_mb") {
      global.bts_threshold_mb = number;
    }
  }
  if (HasForceEnableMarker()) EnableAllMemorySources(*this);
}

const MemoryMonitor::Settings& MemoryMonitor::GetSettings() {
  static InitOnceMemoryMonitorSettings settings;
  return settings;
}

void MemoryMonitor::ForceEnableForTesting(ForceEnableMode mode) {
  auto& settings = const_cast<Settings&>(GetSettings());
  EnableAllMemorySources(settings);
  if (mode == ForceEnableMode::kPersistent) {
    PersistForceEnableMarker();
  }
}

uint32_t MemoryMonitor::ScriptingEngineMode(bool is_mts) {
  const auto& settings = GetSettings();
  if (!settings.global.enabled) return 0;
  // Engine callback thresholds configure cache updates, not event
  // triggers. Bits [31:24] encode MiB; this does not request or force a GC.
  return std::min<uint32_t>(255, is_mts ? settings.global.mts_threshold_mb
                                        : settings.global.bts_threshold_mb)
         << 24;
}

MemoryMonitor::MemoryMonitor(PerformanceEventSender* /*observer*/,
                             const base::LogContext& context, int32_t id)
    : instance_id_(id),
      log_context_(context),
      global_(&GlobalMemoryMonitor::GetInstance()) {}

MemoryMonitor::~MemoryMonitor() {
  if (sample_timer_) sample_timer_->Stop();
  if (anchor_at_ms_ >= 0) {
    ReportPendingSummary(exit_at_ms_ >= 0 ? exit_at_ms_ : MemoryNowMs());
  }
  ReportMemoryForTrace(true, true);
}

void MemoryMonitor::AllocateMemory(MemoryRecord&& record) {
  if (!Enable()) {
    return;
  }
  auto it = memory_records_.find(record.category_);
  if (it == memory_records_.end()) {
    memory_records_.emplace(record.category_, std::move(record));
  } else {
    it->second += record;
  }
  ReportMemoryForTrace();
}

void MemoryMonitor::DeallocateMemory(MemoryRecord&& record) {
  if (!Enable()) {
    return;
  }
  auto it = memory_records_.find(record.category_);
  if (it == memory_records_.end()) {
    return;
  }
  it->second -= record;
  ReportMemoryForTrace();
}

void MemoryMonitor::UpdateMemoryUsage(MemoryRecord&& record,
                                      bool force_report) {
  if (!Enable()) {
    return;
  }
  auto category = record.category_;
  memory_records_.insert_or_assign(category, std::move(record));
  ReportMemoryForTrace(force_report);
}

PageMemoryUsage MemoryMonitor::RefreshMemoryUsage() {
  const auto* state = global_->GetInstanceState(instance_id_);
  if (!state || state->destroyed) return last_usage_;
  PageMemoryUsage usage;
  for (const auto& item : memory_records_) {
    const auto index = SourceIndex(item.first);
    if (index == 2) {
      continue;  // This value is heap size of VM.
    }
    const auto bytes = item.second.size_bytes_;
    usage.total_bytes += bytes;
    int64_t* part = index == 0   ? &usage.element_bytes
                    : index == 1 ? &usage.mts_bytes
                                 : &usage.ui_bytes;
    *part += bytes;
    if (index == 0) {
      usage.element_count += item.second.instance_count_;
    }
  }
  if (state->bts_expected) {
    const auto bts = global_->GetInstanceBtsMemoryUsage(instance_id_);
    usage.bts_shared = bts != std::numeric_limits<size_t>::max();
    const auto it = memory_records_.find(kCategoryBTSEngine);
    if (it != memory_records_.end()) {
      if (usage.bts_shared) {
        // Shared, replace bts usage in memory_records_ with instance attributed
        // value and usage.bts_bytes will be assigned later by UpdatePageMemory
        it->second.size_bytes_ = bts;
      } else {
        // Not shared, use heap size as instance bts usage.
        usage.bts_bytes = it->second.size_bytes_;
      }
    }
  }
  usage.total_bytes += usage.bts_bytes;
  last_usage_ = global_->UpdatePageMemory(instance_id_, usage);
  return last_usage_;
}

void MemoryMonitor::ReportMemoryForTrace(bool force_report, bool is_exit) {
#if ENABLE_TRACE_PERFETTO
  if (!trace::TraceController::Instance()->IsTracingStarted()) {
    return;
  }

  auto now = fml::TimePoint::Now();
  if (!force_report && (now - last_trace_event_time_).ToMilliseconds() < 16) {
    return;
  }
  last_trace_event_time_ = now;

  PageMemoryUsage usage;
  if (!is_exit) {
    usage = RefreshMemoryUsage();
  } else {
    memory_records_.clear();
  }
  TRACE_COUNTER(LYNX_TRACE_CATEGORY,
                (std::string("memory_") + std::to_string(instance_id_)).c_str(),
                usage.total_bytes,
                [this, instance_id = instance_id_,
                 size_bytes = usage.total_bytes](perfetto::EventContext ctx) {
                  ctx.event()->add_debug_annotations(
                      kSizeBytes, std::to_string(size_bytes));
                  for (const auto& [category, record] : memory_records_) {
                    ctx.event()->add_debug_annotations(
                        category, MemoryRecordToJson(record));
                  }
                  ctx.event()->add_debug_annotations(
                      INSTANCE_ID, std::to_string(instance_id));
                });
#endif
}

void MemoryMonitor::AddCommonProps(report::MoveOnlyEvent& event) {
  // Common page-event configuration:
  // - global_sample_rate selects the process. Every non-UI producer follows
  //   that decision; lynx_ui_sample_rate is the only additional source draw.
  // - Keep lynx_ui_memory_enabled fixed within a query cohort. A value of zero
  //   means page totals intentionally exclude UI records.
  // - MTS and page-attributed BTS can still be absent because an engine does
  //   not expose them. Use mts_context_type and bts_runtime_type for those
  //   cohorts instead of treating absent records as a separate validity state.
  event.SetProps("schema_version", 10);
  event.SetProps("lynx_ui_memory_enabled", GetSettings().lynx_ui.enabled != 0);
}

void MemoryMonitor::OnFirstLoadComplete() {
  // Establish one page-memory anchor. Creation-based residency was already
  // enrolled by GlobalMemoryMonitor and is unaffected by load/reload timing.
  if (!Enable() || instance_id_ < 0 || anchor_at_ms_ >= 0) return;
  const auto anchor = MemoryNowMs();
  const auto* state = global_->ResolveInstancePageKey(instance_id_);
  if (!state || state->destroyed) return;
  anchor_at_ms_ = anchor;
  const auto usage = RefreshMemoryUsage();
  summary_identity_ = BuildIdentityEvent(*state);
  summary_identity_.SetProps("bts_shared", usage.bts_shared);
  sample_timer_ = std::make_unique<fml::OneshotTimer>(
      report::EventTrackerPlatformImpl::GetReportTaskRunner());
  ReportScheduled(0, MemoryNowMs());
  next_sample_ = 1;
  ScheduleNext();
}

void MemoryMonitor::ScheduleNext() {
  if (!sample_timer_ || next_sample_ >= kPageMemoryOffsetsMs.size()) return;
  const auto target = anchor_at_ms_ + kPageMemoryOffsetsMs[next_sample_];
  sample_timer_->Start(
      fml::TimeDelta::FromMilliseconds(
          std::max<int64_t>(1, target - MemoryNowMs())),
      [this] {
        const auto now = MemoryNowMs();
        const auto index = next_sample_++;
        // Report the pending point at its actual callback time, then skip
        // further elapsed deadlines. Never backfill.
        // Visibility/app state do not pause the plan or extend its window.
        ReportScheduled(index, now);
        while (next_sample_ < kPageMemoryOffsetsMs.size() &&
               anchor_at_ms_ + kPageMemoryOffsetsMs[next_sample_] <= now) {
          ++next_sample_;
        }
        ScheduleNext();
      });
}

void MemoryMonitor::ObserveEarly(const PageMemoryUsage& usage, int index) {
  early_.Observe(usage.total_bytes, index);
  early_bts_.Observe(usage.bts_bytes, index);
  early_mts_.Observe(usage.mts_bytes, index);
}

report::MoveOnlyEvent MemoryMonitor::BuildIdentityEvent(
    const InstanceState& state) {
  // Identity is shared by scheduled observations and saved early summaries.
  // Group the single-page leaderboard by page_key. Use reporter-resolved host
  // page_id when nonempty, otherwise the platform-defined URL fallback.
  // The output deliberately uses a different key from host common page_id.
  //
  // Offsets use reporter receipt of the first loadBundle completion, not
  // actualFMP or page creation. The monotonic anchor itself is not uploaded.
  // Use telemetry wall-clock metadata for date cohorts, allowing for delay.
  //
  // Engine types explain capability differences; unknown/-1 must not be
  // interpreted as "no engine". End summaries reuse the identity saved at the
  // anchor, so teardown cannot change which page key/runtime owns the samples.
  report::MoveOnlyEvent event;
  event.Reserve(64);
  event.SetName("lynxsdk_performance_entry_memory_v2");
  event.SetInstanceId(instance_id_);
  AddCommonProps(event);
  event.SetProps("page_key", state.page_key);
  event.SetProps(
      "mts_context_type",
      state.mts_known ? static_cast<int32_t>(state.mts_context_type) : -1);
  event.SetProps("bts_runtime_type",
                 state.bts_known
                     ? GlobalMemoryMonitor::RuntimeName(state.bts_runtime_type)
                     : "unknown");
  return event;
}

report::MoveOnlyEvent MemoryMonitor::BuildPageEvent(
    const PageMemoryUsage& usage, const InstanceState& state, size_t index) {
  // Fixed-age queries select one scheduled_offset_ms and aggregate
  // current_page_memory_bytes by page_key. The total is the sum of records
  // available to this page; missing MTS/BTS contributions are intentionally
  // left as a lower value and can be filtered using the runtime fields.
  auto event = BuildIdentityEvent(state);
  event.SetProps("report_kind", "scheduled");
  event.SetProps("scheduled_offset_ms", kPageMemoryOffsetsMs[index]);
  // BTS is either a private heap or this page's tracked slot in a shared
  // QuickJS VM, never a copy of the whole shared heap.
  event.SetProps("current_element_bytes", usage.element_bytes);
  event.SetProps("current_mts_heap_bytes", usage.mts_bytes);
  event.SetProps("current_bts_attributed_bytes", usage.bts_bytes);
  event.SetProps("current_platform_ui_bytes", usage.ui_bytes);
  event.SetProps("current_page_memory_bytes", usage.total_bytes);
  event.SetProps("bts_shared", usage.bts_shared);
  if (GetSettings().lynx_ui.enabled) {
    for (const auto& item : memory_records_) {
      if (SourceIndex(item.first) == 3) {
        auto key = item.first + "SizeBytes";
        std::replace(key.begin(), key.end(), '-', '_');
        event.SetProps(key.c_str(), item.second.size_bytes_);
      }
    }
  }
  return event;
}

void MemoryMonitor::AddSampleProps(report::MoveOnlyEvent& event,
                                   const EarlyMemorySamples& samples,
                                   std::string_view memory_kind,
                                   uint32_t required) {
  // One summary gives each instance one vote. Mean/peak are emitted only when
  // every due point was observed. Field presence carries that gate.
  if (samples.Complete(required)) {
    std::string field_name("early_sample_mean_");
    field_name.append(memory_kind.data(), memory_kind.size()).append("_bytes");
    event.SetProps(field_name.c_str(), samples.Mean());
    field_name.replace(sizeof("early_sample_") - 1, 4, "peak");
    event.SetProps(field_name.c_str(), samples.peak_bytes);
  }
  // Peaks are maxima of these scheduled cache observations, not continuous
  // lifetime peaks. Q1.2/Q1.4/Q1.6 use the three peaks on the same summary
  // cohort to screen for early memory spikes.
  // Invalid means/peaks are omitted, not zero-filled. Long-lived pages have
  // more chances to hit a sampled peak, so compare duration/count strata.
}

void MemoryMonitor::AddEarlySummary(report::MoveOnlyEvent& event,
                                    int64_t target) {
  if (summary_published_) return;
  // target=min(logical page end-anchor, kEarlyMemoryWindowMs), clamped at zero.
  // Normal publication uses the full window. target determines which plan
  // points are due. It is neither a weight nor a denominator for the mean.
  // A full-window target says the window was capped, not that page lifetime
  // was exactly that duration; it can have lived for much longer.
  //
  // Compare rows with an emitted early_sample_mean_memory_bytes against all
  // received unique summaries to measure planned-point coverage. Runtime and
  // lynx_ui_memory_enabled remain independent query dimensions.
  summary_published_ = true;  // Failure also consumes the one-summary budget.
  event.SetProps("has_early_usage_summary", true);
  event.SetProps("early_window_target_duration_ms", target);
  uint32_t required = 0;
  for (size_t i = 0; i < kEarlyMemorySampleCount; ++i) {
    if (kPageMemoryOffsetsMs[i] <= target) {
      required |= 1u << i;
    }
  }
  AddSampleProps(event, early_, "memory", required);
  AddSampleProps(event, early_bts_, "bts_attributed", required);
  AddSampleProps(event, early_mts_, "mts_heap", required);
  summary_identity_ = {};  // No end-of-page event is needed after publication.
}

void MemoryMonitor::ReportScheduled(size_t index, int64_t now_ms) {
  const auto* state = global_->GetInstanceState(instance_id_);
  if (!state || state->destroyed) {
    // Untracked pages can already be erased by the single exit notification.
    // Stop rather than treating a missing page as eligible for more samples.
    if (sample_timer_) sample_timer_->Stop();
    next_sample_ = kPageMemoryOffsetsMs.size();
    return;
  }
  const auto usage = RefreshMemoryUsage();
  if (index < kEarlyMemorySampleCount && !summary_published_) {
    ObserveEarly(usage, static_cast<int>(index));
  }
  const bool publish_summary =
      !summary_published_ && (index == kEarlyMemorySampleCount - 1 ||
                              now_ms - anchor_at_ms_ >= kEarlyMemoryWindowMs);
  if (!GetSettings().page_scheduled.enabled) {
    // Sample event delivery only: every early observation still contributes.
    // Preserve the summary even when its scheduled carrier is suppressed.
    if (publish_summary) {
      ReportPendingSummary(anchor_at_ms_ + kEarlyMemoryWindowMs);
    }
    return;
  }
  auto event = BuildPageEvent(usage, *state, index);
  if (publish_summary) {
    // Publish even when a delayed runner skipped the window's last point:
    // missing points still invalidate the summary, never backfill them.
    AddEarlySummary(event, kEarlyMemoryWindowMs);
  } else {
    event.SetProps("has_early_usage_summary", false);
  }
  EnqueueEvent(std::move(event));
}

void MemoryMonitor::ReportPendingSummary(int64_t end_at_ms) {
  if (anchor_at_ms_ < 0 || summary_published_) return;
  // Do not query GlobalMemoryMonitor, records, slots or engine state here.
  // The two actors can tear down in either order: these values were frozen
  // during scheduled sampling, before any runtime teardown callbacks.
  auto event = std::move(summary_identity_);
  event.SetProps("report_kind", "early_summary");
  // No exit memory measurement is made. End time only bounds the due plan.
  // Crashes or forced termination may never run this path.
  AddEarlySummary(event, std::clamp<int64_t>(end_at_ms - anchor_at_ms_, 0,
                                             kEarlyMemoryWindowMs));
  EnqueueEvent(std::move(event));
}

void MemoryMonitor::EnqueueEvent(report::MoveOnlyEvent event) {
  // Event construction precedes queueing/flush/platform delivery, so arrival
  // time is not the sampling timestamp.
  report::EventTracker::OnEvent(
      [event = std::move(event)](auto& target) mutable {
        target = std::move(event);
      });
}

}  // namespace performance
}  // namespace tasm
}  // namespace lynx
