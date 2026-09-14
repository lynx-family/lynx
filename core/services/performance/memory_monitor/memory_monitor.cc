// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/performance/memory_monitor/memory_monitor.h"

#include <algorithm>
#include <limits>
#include <random>
#include <string_view>
#include <utility>
#include <vector>

#include "base/include/string/string_number_convert.h"
#include "base/include/string/string_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/services/event_report/event_tracker_platform_impl.h"
#include "core/services/performance/memory_monitor/global_memory_monitor.h"

namespace lynx {
namespace tasm {
namespace performance {
namespace {

int SourceIndex(const std::string& category) {
  if (category == kCategoryTasmElement) return 0;
  if (category == kCategoryMTSEngine) return 1;
  if (category == kCategoryBTSEngine) return 2;
  return 3;  // Platform UI producers use per-widget category names.
}

struct InitOnceMemoryMonitorSettings : MemoryMonitor::Settings {
  InitOnceMemoryMonitorSettings() {
    std::pair<std::string_view, Entry*> entries[] = {
        {"global", &global},
        {"tasm_element", &tasm_element},
        {"lynx_ui", &lynx_ui},
        {"bts_vm_per_instance_gc", &bts_vm_per_instance_gc},
        {"bts_vm_per_instance_rc", &bts_vm_per_instance_rc},
    };
    auto config = LynxEnv::GetInstance().GetStringEnv(
        LynxEnv::Key::MEMORY_MONITOR_CONFIG);
    if (config) {
      std::vector<std::string> items;
      base::SplitString(*config, ',', items);
      for (const auto& item : items) {
        std::vector<std::string> pair;
        if (!base::SplitString(item, '=', pair) || pair.size() != 2) continue;
        const auto key = base::TrimString(pair[0]);
        const auto dot = key.rfind('.');
        if (dot == std::string::npos) continue;
        Entry* entry = nullptr;
        for (const auto& candidate : entries) {
          if (candidate.first == std::string_view(key).substr(0, dot)) {
            entry = candidate.second;
            break;
          }
        }
        if (!entry) continue;
        const auto property = std::string_view(key).substr(dot + 1);
        const auto value = base::TrimString(pair[1]);
        if (property == "sample_rate") {
          float rate;
          if (base::StringToFloat(value, rate, true) && rate >= 0 &&
              rate <= 1) {
            entry->sample_rate = rate;
          }
        } else if (entry == &global) {
          int64_t parsed;
          if (!base::StringToInt(value, parsed) || parsed < 0 ||
              parsed > std::numeric_limits<uint32_t>::max())
            continue;
          const auto number = static_cast<uint32_t>(parsed);
          if (property == "time_interval_sec")
            global.time_interval_sec = number;
          if (property == "pss_threshold_mb") global.pss_threshold_mb = number;
          if (property == "mts_threshold_mb") global.mts_threshold_mb = number;
          if (property == "bts_threshold_mb") global.bts_threshold_mb = number;
        }
      }
    }
    std::mt19937 random(std::random_device{}());
    std::uniform_real_distribution<float> distribution(0, 1);
    global.enabled = distribution(random) < global.sample_rate;
    for (const auto& item : entries) {
      if (item.second != &global) {
        item.second->enabled =
            global.enabled && distribution(random) < item.second->sample_rate;
      }
    }
  }
};

}  // namespace

const MemoryMonitor::Settings& MemoryMonitor::GetSettings() {
  static InitOnceMemoryMonitorSettings settings;
  return settings;
}

void MemoryMonitor::ForceEnableForTesting() {
  auto& settings = const_cast<Settings&>(GetSettings());
  for (auto* entry :
       {static_cast<Settings::Entry*>(&settings.global), &settings.tasm_element,
        &settings.lynx_ui, &settings.bts_vm_per_instance_gc,
        &settings.bts_vm_per_instance_rc}) {
    entry->enabled = 1;
    entry->sample_rate = 1.0f;
  }
}

uint32_t MemoryMonitor::ScriptingEngineMode(bool is_mts) {
  const auto& settings = GetSettings();
  if (!settings.global.enabled) return 0;
  // Engine callback thresholds remain enabled for cache updates, not as event
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
  // No exit measurement or global lifecycle notification here. Runtime/VM
  // teardown may already have changed every cache; the summary uses only
  // saved scheduled observations. Shell flushes after this destructor.
}

void MemoryMonitor::AllocateMemory(MemoryRecord&& record) {
  if (!Enable()) return;
  auto it = memory_records_.find(record.category_);
  if (it == memory_records_.end()) {
    UpdateMemoryUsage(std::move(record));
    return;
  }
  int64_t total = it->second.size_bytes_;
  const int64_t count =
      static_cast<int64_t>(it->second.instance_count_) + record.instance_count_;
  if (!AddMemoryBytes(record.size_bytes_, total) ||
      count > std::numeric_limits<int32_t>::max() ||
      count < std::numeric_limits<int32_t>::min()) {
    it->second.size_bytes_ = -1;
  } else {
    // Keep existing count/detail merging for cache consumers. Only the new
    // flat telemetry aggregation ignores detail_; do not erase it here.
    it->second += record;
  }
  ReportMemory();
}

void MemoryMonitor::DeallocateMemory(MemoryRecord&& record) {
  if (!Enable()) return;
  auto it = memory_records_.find(record.category_);
  if (it == memory_records_.end()) return;
  const int64_t count =
      static_cast<int64_t>(it->second.instance_count_) - record.instance_count_;
  if (record.size_bytes_ < 0 || it->second.size_bytes_ < record.size_bytes_ ||
      count > std::numeric_limits<int32_t>::max() ||
      count < std::numeric_limits<int32_t>::min()) {
    it->second.size_bytes_ = -1;
  } else {
    it->second -= record;
  }
  ReportMemory();
}

void MemoryMonitor::UpdateMemoryUsage(MemoryRecord&& record,
                                      bool force_report) {
  if (!Enable()) return;
  auto category = record.category_;
  memory_records_.insert_or_assign(category, std::move(record));
  ReportMemory(force_report);
}

PageMemoryUsage MemoryMonitor::RefreshMemoryUsage() {
  const auto* state = global_->GetInstanceState(instance_id_);
  // Global cleanup can precede this monitor's destruction. Late source
  // notifications must not reclassify a former shared heap as standalone.
  if (!state || state->destroyed) return last_usage_;
  if (anchor_at_ms_ >= 0 && !normalized_url_.empty() &&
      normalized_url_ != MemoryUrl(state->url)) {
    scope_changed_ = true;
  }
  normalized_url_ = MemoryUrl(state->url);
  PageMemoryUsage usage;
  const auto& settings = GetSettings();
  usage.eligible = (settings.tasm_element.enabled ? kElementMemory : 0u) |
                   (settings.lynx_ui.enabled ? kUiMemory : 0u);
  if (state && state->mts_known &&
      state->mts_context_type == runtime::LepusNGContextType) {
    usage.eligible |= kMtsMemory;
  }
  if (state && !state->bts_expected) usage.required &= ~kBtsMemory;
  for (const auto& item : memory_records_) {
    const auto index = SourceIndex(item.first);
    if (index == 2) continue;
    const auto bytes = item.second.size_bytes_;
    int64_t* part = index == 0   ? &usage.element_bytes
                    : index == 1 ? &usage.mts_bytes
                                 : &usage.ui_bytes;
    if (!AddMemoryBytes(bytes, *part) ||
        !AddMemoryBytes(bytes, usage.total_bytes)) {
      usage.arithmetic_valid = false;
    } else {
      usage.available |= 1u << index;
    }
  }
  if (usage.required & kBtsMemory) {
    const auto bts = global_->GetInstanceBtsMemoryUsage(instance_id_);
    usage.bts_shared = bts != std::numeric_limits<size_t>::max();
    if (!usage.bts_shared) {
      const auto it = memory_records_.find(kCategoryBTSEngine);
      if (state && state->bts_known &&
          state->bts_runtime_type != runtime::js::JSRuntimeType::jsc) {
        usage.eligible |= kBtsMemory;
        if (it != memory_records_.end() && it->second.size_bytes_ >= 0) {
          usage.bts_bytes = it->second.size_bytes_;
          usage.available |= kBtsMemory;
        }
      }
    }
    // Shared-slot availability is filled by UpdatePageMemory below. A returned
    // zero alone cannot distinguish a valid zero slot from unsupported data.
  }
  if (!AddMemoryBytes(usage.bts_bytes, usage.total_bytes)) {
    usage.arithmetic_valid = false;
    usage.bts_bytes = 0;
    usage.available &= ~kBtsMemory;
  }
  if (anchor_at_ms_ >= 0 && last_usage_.required != usage.required) {
    scope_changed_ = true;
  }
  last_usage_ = global_->UpdatePageMemory(instance_id_, usage);
  return last_usage_;
}

void MemoryMonitor::ReportMemory(bool force_report) {
  if (!Enable()) return;
  [[maybe_unused]] const auto usage = RefreshMemoryUsage();
  // Preserve the diagnostic counter, but never revive the old threshold-based
  // PerformanceEntry transport: that would duplicate scheduled memory events.
  if (force_report) {
    TRACE_COUNTER(
        LYNX_TRACE_CATEGORY,
        (std::string("memory_") + std::to_string(instance_id_)).c_str(),
        usage.total_bytes);
  }
}

void MemoryMonitor::AddSamplingProps(report::MoveOnlyEvent& event) {
  const auto& settings = GetSettings();
  // Query conventions shared by both memory events:
  //
  // - Start every query with the event name and schema_version=7. Also hold
  //   platform, SDK/experiment and source-sampling configuration constant.
  //   Schema 7 removes redundant/diagnostic properties, retaining the page
  //   sample means and single PSS estimate for triggers and ratios. Do not mix
  //   trigger cohorts across older versions, or page means with older exit
  //   measurements/time-weighted means.
  // - PCT95(x) below means the 95th percentile of x, not a 95% confidence
  //   interval. AVG/PCT operate on the rows remaining after ALL stated filters.
  //   Convert bytes to MiB with /1048576; ratios are fractions, not
  //   percentages. Boolean properties are emitted as 0/1.
  // - process_session_id identifies one process run. It is useful for joins,
  //   distinct-process counts and clustered uncertainty estimates, not as a
  //   grouping key for a URL leaderboard. A large weighted count is not a
  //   large number of independent observations: report actual rows/processes.
  // - All memory is SDK-accounted memory. Complete sources do not establish
  //   simultaneous measurements, resident physical bytes or GC-confirmed
  //   liveness. Missing optional metrics must not be replaced with zero.
  // - Use a documented minimum row/process count for tail rankings, especially
  //   P99, and inspect day-to-day stability. An event-weighted percentile is
  //   not user-equal; use existing user metadata for user-level cohorts if
  //   available. Confidence intervals need correlated instances from the
  //   same user/process handled together, not treated as independent samples.
  //
  // Sampling has separate layers. process_sample_rate is the process
  // inclusion probability; the source rates below are CONDITIONAL on that
  // decision. For one required source with rate q and process rate p, the
  // random inclusion probability is p*q. More complex source combinations
  // require their actual joint design: the QuickJS GC/RC rates describe
  // different modes and must not blindly be multiplied together.
  //
  // Prefer separate cohorts with fixed source-sampling rates.
  // If combining different probabilities for population estimates, use the
  // appropriate inverse inclusion probability in both numerator and
  // denominator (and a weighted percentile, not ordinary PCT). This cannot
  // repair unsupported engines, missing callbacks or lost events. The global
  // event's N, N/n and heap-total/heap weights correct only the subsequent
  // entity draw, not source inclusion or the frequency of snapshot triggers.
  //
  // CONFIGURATION -> QUERY DIMENSIONS (profiles are described in Settings):
  //   global.sample_rate                 -> process_sample_rate
  //   tasm_element.sample_rate           -> source_tasm_element_sample_rate
  //   lynx_ui.sample_rate                -> source_platform_ui_sample_rate
  //   bts_vm_per_instance_gc.sample_rate -> source_bts_slot_gc_sample_rate
  //   bts_vm_per_instance_rc.sample_rate -> source_bts_slot_rc_sample_rate
  // Rates identify the configured cohort, not whether a measurement arrived.
  // Process-level enabled results are not emitted: the validity gates already
  // require eligible, available sources for the scope actually being queried.
  // Page eligibility/validity and the global question-specific complete flags
  // remain mandatory. No separate mts/bts-heap rate exists: global enables
  // collection requests, while engine capability determines what is available.
  //
  // For a production FULL-profile cohort, require all four emitted source
  // rates=1; hold process_sample_rate fixed and >0.
  // These are cohort filters, not substitutes for validity. If using already
  // collected lower-rate data, retain the original rate strata and the same
  // validity gates; do not pretend the rates were 1.
  //
  // Actual GC vs RC mode and each mode's enabled result are NOT emitted.
  // An OR of those results could not prove the current VM has tracking.
  // Trace capture can also enable QuickJS tracking outside the sampling
  // decision. Never infer mode or a per-VM inclusion probability from type.
  // Prefer BOTH slot rates=1 for QuickJS page/residual analytics and exclude
  // externally forced diagnostic cohorts using host experiment/trace metadata
  // if such data is mixed into production telemetry.
  //
  // Why this matters with the current defaults, conditional on process
  // selection: UI rate=.1, GC-slot rate=1, RC-slot rate=.1, Element rate=1.
  // The random qualification for a shared QuickJS full page is .1 in GC mode
  // but .01 in RC mode; core is 1 vs .1. This is BEFORE slot exhaustion,
  // missing sources or invalid scheduled points. A global total needs no slot
  // draw, so its UI/Element qualification is .1 for either mode. Private
  // QuickJS full pages also need no slot draw. Dividing all page counts by
  // .1, or merging GC/RC populations with ordinary PCT, is therefore wrong.
  // Because mode is absent, a generic exact correction of that mixed page
  // population cannot be reconstructed from this schema alone. Even known
  // inclusion probabilities cannot correct non-random missing callbacks.
  //
  // TRUST LEVELS (not numeric confidence scores):
  // 1. Valid full/core/query-scope rows: usable for that named cached metric
  //    within the supported, sampled, successfully observed population.
  // 2. Valid individual sources but invalid full rows: usable only for those
  //    source diagnostics; never silently label their sum "page/process total".
  // 3. Unsupported, unsampled or unreceived sources: unknown, not measured
  // zero. No flag proves freshness, GC reachability, all-widget coverage, or
  // that the observed cohort represents unsupported engines. Random process
  // sampling reduces volume without fixing those capability/measurement gaps.
  // Source enablement must be paired with a working platform producer.
  event.SetProps("schema_version", 7);
  event.SetProps("process_session_id", GlobalMemoryMonitor::ProcessSessionId());
  event.SetProps("process_sample_rate",
                 static_cast<double>(settings.global.sample_rate));
  event.SetProps("source_tasm_element_sample_rate",
                 static_cast<double>(settings.tasm_element.sample_rate));
  event.SetProps("source_platform_ui_sample_rate",
                 static_cast<double>(settings.lynx_ui.sample_rate));
  event.SetProps(
      "source_bts_slot_gc_sample_rate",
      static_cast<double>(settings.bts_vm_per_instance_gc.sample_rate));
  event.SetProps(
      "source_bts_slot_rc_sample_rate",
      static_cast<double>(settings.bts_vm_per_instance_rc.sample_rate));
}

void MemoryMonitor::OnFirstLoadComplete() {
  if (!Enable() || instance_id_ < 0 || anchor_at_ms_ >= 0) return;
  const auto* state = global_->GetInstanceState(instance_id_);
  if (!state || state->destroyed) return;
  anchor_at_ms_ = MemoryNowMs();
  const auto usage = RefreshMemoryUsage();
  early_full_eligible_ = (usage.eligible & usage.required) == usage.required;
  const auto core = usage.required & kCoreMemory;
  early_core_eligible_ = (usage.eligible & core) == core;
  summary_identity_ = BuildIdentityEvent(*state);
  summary_identity_.SetProps("bts_shared", usage.bts_shared);
  sample_timer_ = std::make_unique<fml::OneshotTimer>(
      report::EventTrackerPlatformImpl::GetReportTaskRunner());
  ReportScheduled(0, anchor_at_ms_);
  next_sample_ = 1;
  ScheduleNext();
}

void MemoryMonitor::OnReload() {
  if (anchor_at_ms_ >= 0) scope_changed_ = true;
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
        // Sampling follows elapsed time, independent of view visibility or
        // app state. Delays affect validity; never backfill missed points.
        ReportScheduled(index, now);
        while (next_sample_ < kPageMemoryOffsetsMs.size() &&
               anchor_at_ms_ + kPageMemoryOffsetsMs[next_sample_] <= now) {
          ++next_sample_;
        }
        ScheduleNext();
      });
}

void MemoryMonitor::ObserveEarly(const PageMemoryUsage& usage, int64_t at_ms,
                                 int index) {
  early_full_.Observe(at_ms, usage.total_bytes,
                      !scope_changed_ && usage.Valid(kFullMemory), index);
  early_core_.Observe(at_ms, usage.CoreBytes(),
                      !scope_changed_ && usage.Valid(kCoreMemory), index);
}

report::MoveOnlyEvent MemoryMonitor::BuildIdentityEvent(
    const InstanceState& state) {
  // Identity is shared by scheduled observations and saved early summaries.
  // Group the single-page leaderboard by normalized_url, but deduplicate
  // instances by (process_session_id, instance_id), never by URL alone.
  // Several instances of the same URL are separate observations. Query and
  // fragment are stripped; dynamic path-to-route normalization is not done
  // here, so a host must supply stable routes if its paths contain IDs.
  //
  // Offsets use reporter receipt of the first loadBundle completion, not
  // actualFMP or page creation. The monotonic anchor itself is not uploaded.
  // Use telemetry wall-clock metadata for date cohorts, allowing for delay.
  //
  // Engine types explain capability differences; unknown/-1 must not be
  // interpreted as "no engine". End summaries reuse the identity saved at the
  // anchor, so teardown cannot change which URL/runtime owns the samples.
  report::MoveOnlyEvent event;
  event.SetName("lynxsdk_performance_entry_memory");
  event.SetInstanceId(instance_id_);
  AddSamplingProps(event);
  event.SetProps("instance_id", instance_id_);
  event.SetProps("normalized_url", MemoryUrl(state.url));
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
    const PageMemoryUsage& usage, const InstanceState& state, int64_t now_ms,
    size_t index) {
  // FIXED-AGE QUERY: "How much memory does one instance of this URL account
  // for approximately five seconds after its first load completes?"
  //
  //   event = lynxsdk_performance_entry_memory; schema_version = 7
  //   report_kind = "scheduled"; scheduled_offset_ms = 5000
  //   ABS(sample_lateness_ms) <= 1000
  //   accounting_scope_changed = 0; page_full_memory_valid = 1
  //   GROUP BY normalized_url
  //   AVG / PCT50 / PCT95 / PCT99(current_page_full_bytes) / 1048576
  //
  // Use one scheduled offset per query. With fixed sampling probabilities,
  // every surviving instance has at most one observation for that point, so
  // these ordinary percentiles have a clear per-instance interpretation.
  // Do not require a later valid 30s summary: that would exclude short pages
  // which did reach this point. Do not mix early_summary rows into this query.
  // Later offsets condition on longer survival; P95(at 30s)-P95(at 5s) is NOT
  // a same-instance growth statistic. Such a query needs paired instance IDs.
  //
  // Actual aggregation offset = scheduled_offset_ms + sample_lateness_ms.
  // Late callbacks are not backdated or replayed. For a CORE query use
  // page_core_memory_valid and current_page_core_bytes with the same
  // timing/scope filters, without requiring full validity.
  //
  // PLATFORM SLICES FOR THIS FIXED-AGE QUERY:
  // Use platform/OS from common telemetry metadata (not a new property here).
  // In schema 7, mts_context_type=1 means LepusNG/QuickJS; -1 is unknown,
  // and 0/2/3 are unmeasured private interpreters. Full/core validity already
  // enforces supported required MTS. Never infer MTS capability from BTS type.
  // The FULL/CORE profiles in Settings change sampling, not these capabilities.
  //
  // - iOS: separate bts_runtime_type="quickjs" and "jsc". The QuickJS slice
  //   can use the full query above (or its core variant), with bts_shared
  //   stratification. This represents the QuickJS subset, not all iOS URLs.
  //   JSC with required BTS cannot qualify for full OR core, even with every
  //   source enabled. An empty JSC total leaderboard is expected, not zero
  //   memory and not a reason to weaken the validity filter.
  // - Android and PC: use bts_runtime_type="quickjs" for the normal page
  //   full/core ranking. Keep bts_runtime_type="v8" separate: RuntimeManager
  //   shares its whole heap across groups; even one active page cannot make
  //   it an attributable private heap. V8 page full/core is unavailable.
  // - Harmony: the dominant bts_runtime_type="jsvm" has the same page limit
  //   as shared V8. Use global VM/total queries for that cohort, and the
  //   bts_runtime_type="quickjs" slice for supported page full/core rankings.
  //   That minority slice is not a representative Harmony-wide page total.
  //
  // For shared QuickJS (bts_shared=1), the valid BTS source requires a tracked
  // page slot and received slot data. For private QuickJS (bts_shared=0), the
  // whole page-owned heap is enough; a slot-enabled filter is unnecessary
  // unless intentionally restricting to the same FULL configuration cohort.
  // Changing the mixture of private heaps vs attributed shared slots can
  // change a URL's score without a regression: keep sharing mode as a slice.
  // Pages explicitly created without BTS have no required BTS bit; they can
  // still be valid. Identify that case from (page_required_source_mask & 4)=0
  // on scheduled rows, NOT bts_runtime_type="unknown". The early summary has
  // no source-mask block; its frozen eligibility/validity gates define scope.
  //
  // FALLBACK DIAGNOSTIC FOR JSC / SHARED V8 / SHARED JSVM:
  // Keep the common scheduled/offset/timing/scope filters above, but replace
  // the full-valid condition with validity of the explicitly named sources.
  // Example Element+MTS:
  //   (page_required_source_mask & 3)=3
  //   source_tasm_element_complete=1; source_mts_vm_heap_complete=1
  //   PCT95(current_element_bytes + current_mts_heap_bytes)
  // Group by normalized_url AND bts_runtime_type. Call this "Element+MTS",
  // not core (which includes BTS). For UI require its applicable bit 8 and
  // source_platform_ui_complete=1 before adding its bytes; single-source
  // queries use only that source's bit and completeness flag. Do not require
  // page_full_memory_valid=1: unavailable BTS would reject this source cohort.
  // No early Element+MTS mean is emitted; early_sample_mean_core_bytes cannot
  // be repurposed to supply it. Never compare a source-only score to full.
  auto event = BuildIdentityEvent(state);
  event.SetProps("report_kind", "scheduled");
  event.SetProps("scheduled_offset_ms", kPageMemoryOffsetsMs[index]);
  event.SetProps("sample_lateness_ms",
                 now_ms - anchor_at_ms_ - kPageMemoryOffsetsMs[index]);
  event.SetProps("accounting_scope_changed", scope_changed_);
  // Ownership-correct decomposition:
  //   accounted = element + MTS heap + attributed BTS + platform UI
  //   core      = accounted - platform UI
  // MTS is page-private. BTS is either a private heap or this page's tracked
  // slot in a shared QuickJS VM, never a copy of the entire shared heap.
  // Shared V8/JSVM cannot provide a page BTS value; their whole heap is
  // represented by the global event. Thus core does NOT fix missing BTS
  // attribution: it only removes UI from the scope.
  //
  // Component fields are also present on partial rows. Do not rank their sum
  // as complete memory even when nonzero. Full/core value fields below
  // are emitted only when valid. For component attribution, use the SAME
  // full-valid cohort for SUM(component)/SUM(current_page_full_bytes); that
  // is a byte-weighted share, not AVG(per-page share). Component P95s cannot
  // be added to obtain the total P95.
  event.SetProps("current_element_bytes", usage.element_bytes);
  event.SetProps("current_mts_heap_bytes", usage.mts_bytes);
  event.SetProps("current_bts_attributed_bytes", usage.bts_bytes);
  event.SetProps("current_platform_ui_bytes", usage.ui_bytes);
  // Mask bits: Element=1, MTS=2, BTS=4, UI=8.
  // required: applicable sources. Validity checks support, sampling,
  // availability and arithmetic internally; those intermediate masks are not
  // uploaded. A disabled-by-architecture BTS is removed from required;
  // unsupported/unsampled/missing BTS is NOT removed merely to claim full.
  //
  // source_X_complete is Valid(X): it can be true when X is not required.
  // To examine actual X measurements, also require the bit in required.
  // Never infer applicability or fresh measurement from complete alone.
  // For fixed-age collection success, count full-valid timely rows
  // over ALL received scheduled rows at that offset, including partial/late
  // rows; this measures received-row quality, not end-to-end delivery.
  event.SetProps("page_required_source_mask", usage.required);
  const char* source_names[] = {"tasm_element", "mts_vm_heap", "bts_vm_heap",
                                "platform_ui"};
  for (size_t i = 0; i < 4; ++i) {
    const auto prefix = std::string("source_") + source_names[i];
    event.SetProps((prefix + "_complete").c_str(), usage.Valid(1u << i));
  }
  event.SetProps("page_full_memory_valid", usage.Valid(kFullMemory));
  event.SetProps("page_core_memory_valid", usage.Valid(kCoreMemory));
  if (usage.Valid(kFullMemory))
    event.SetProps("current_page_full_bytes", usage.total_bytes);
  if (usage.Valid(kCoreMemory))
    event.SetProps("current_page_core_bytes", usage.CoreBytes());
  event.SetProps("bts_shared", usage.bts_shared);
  return event;
}

void MemoryMonitor::AddSampleProps(report::MoveOnlyEvent& event,
                                   const EarlyMemorySamples& samples,
                                   uint32_t required, bool core) {
  // PRIMARY URL LEADERBOARD: one early-life score per instance.
  //
  //   event = lynxsdk_performance_entry_memory; schema_version = 7
  //   has_early_usage_summary = 1; early_usage_summary_full_valid = 1
  //   GROUP BY normalized_url
  //   PCT95(early_sample_mean_full_bytes) / 1048576  -> high-memory URL score
  //   AVG(early_sample_mean_full_bytes) / 1048576    -> typical instance mean
  //
  // Keep both summary carriers: report_kind="scheduled" and "early_summary".
  // Filtering only the latter drops long pages; filtering only scheduled
  // drops most short pages. Do not qualify a saved summary using current_*
  // fields or the carrier's lateness: these describe a different, single
  // observation and are absent on early_summary.
  //
  // For instance j, M_j = SUM(valid scheduled bytes_j) / valid_count_j.
  // Valid requires ALL due early points, not just a convenient subset.
  // The leaderboard takes P95 over M_j, not P95 over all raw samples, not the
  // average of per-offset P95s, and not SUM(sample sums)/SUM(sample counts).
  // Each instance has equal weight regardless of its lifetime/sample count.
  // Example: a page ending at 8s with 100/200 MiB at 0/5s contributes 150 MiB;
  // neither an exit value nor zeros for the unvisited 10..30s points enter.
  //
  // A separate CORE leaderboard substitutes the core validity/value fields.
  // Do not fall back to core only for URLs where full fails: that silently
  // compares different scopes. A full-invalid URL is "insufficient full
  // data", not a low-memory URL. Keep engine/source configuration strata.
  //
  // Valid also requires anchor eligibility, unchanged accounting scope,
  // attempted_mask == valid_mask == required_mask, timely unique observations
  // (within 1000ms), and maximum adjacent valid gap <=6000ms. A ratio of 1
  // alone is not a replacement for early_usage_summary_*_valid.
  //
  // PLATFORM SUMMARY RECIPE: use the FULL-profile sampling cohort from
  // AddSamplingProps, schema_version=7, bts_runtime_type="quickjs",
  // has_early_usage_summary=1, early_full_profile_eligible=1 and
  // early_usage_summary_full_valid=1; group by platform, normalized_url and
  // bts_shared, then take PCT95(early_sample_mean_full_bytes). This is the
  // normal supported BTS cohort on Android/PC and only the QuickJS minority
  // on iOS/Harmony. The JSC/V8/JSVM majority/minority weights cannot be filled
  // in with zero or corrected by process_sample_rate.
  //
  // For the intentional CORE-PAGE profile, replace BOTH eligibility/validity
  // fields with their core versions and use early_sample_mean_core_bytes.
  // Fix the UI sampling profile rather than selecting full for some URLs and
  // core for others. Core can recover QuickJS pages missing only UI, not JSC
  // heap, shared V8/JSVM attribution or unsupported MTS.
  //
  // These summary gates supersede the scheduled row's source masks/current
  // fields; do not join on a future full-valid snapshot to "repair" an invalid
  // summary. Existing lower-rate FULL cohorts may be used within their own
  // documented sampling strata, with the GC/RC limitations above.
  const bool valid = samples.Complete(required) && !scope_changed_ &&
                     (core ? early_core_eligible_ : early_full_eligible_);
  event.SetProps(core ? "early_usage_summary_core_valid"
                      : "early_usage_summary_full_valid",
                 valid);
  const std::string prefix = core ? "early_sample_core_" : "early_sample_full_";
  event.SetProps((prefix + "count").c_str(), samples.count);
  // Count is valid observed points, not attempted/due points. Coverage can be
  // calculated as count/early_required_scheduled_count; it is NOT time
  // coverage or a replacement for validity. Internal masks/gaps still enforce
  // all due, timely, unique points, but are not needed as uploaded filters.
  // Stratify by early_window_target_duration_ms and count to detect changing
  // short/long visit mixtures. Keep failures when calculating success rates.
  if (valid) {
    event.SetProps(
        core ? "early_sample_mean_core_bytes" : "early_sample_mean_full_bytes",
        samples.Mean());
    event.SetProps(
        core ? "early_sample_peak_core_bytes" : "early_sample_peak_full_bytes",
        samples.peak_bytes);
  }
  // peak is MAX of these scheduled cache observations, not a continuous
  // lifetime peak. PCT95(early_sample_peak_full_bytes) on the same valid
  // summary cohort is a separate spike-screening metric, not the main score.
  // Invalid means/peaks are omitted, not zero-filled. Long-lived pages have
  // more chances to hit a sampled peak, so compare duration/count strata.
}

void MemoryMonitor::AddEarlySummary(report::MoveOnlyEvent& event,
                                    int64_t target) {
  if (summary_published_) return;
  // target=min(logical page end-anchor, 30000ms), clamped at zero; a normal
  // 30s publication uses 30000 directly. target only determines which plan
  // points are due. It is neither a weight nor a denominator for the mean.
  // A 30000ms target says the window was capped, not that page lifetime was
  // exactly 30s; it can have lived for much longer.
  //
  // QUALITY QUERY (before filtering on summary validity):
  //   R = received rows with has_early_usage_summary=1 and
  //       early_full_profile_eligible=1, after transport deduplication.
  //   V = rows in R with early_usage_summary_full_valid=1.
  //   full summary success rate = COUNT(V)/COUNT(R).
  // Repeat for core and by early_window_target_duration_ms buckets. Include
  // scope changes, lateness and missing-point failures in R; filtering them out
  // hides collection problems. Profile eligibility was frozen at the anchor
  // from capabilities/sampling, not chosen later based on memory or success.
  // This is success among eligible RECEIVED summaries, not all page loads:
  // no-anchor pages, unreported crashes/OOMs and transport loss need a
  // separate lifecycle denominator. Also show raw rows and distinct processes.
  //
  // For PLATFORM COVERAGE, start one step earlier:
  //   A = all received unique summaries for that platform/configuration,
  //       before engine, profile eligibility or full-valid filters;
  //   E = those with early_full_profile_eligible=1;
  //   V = those with early_usage_summary_full_valid=1.
  // Show COUNT(E)/COUNT(A), COUNT(V)/COUNT(E), and COUNT(V)/COUNT(A), with
  // zero-denominator guards and a bts_runtime_type breakdown. E/A includes
  // capabilities AND source sampling (and readiness at the anchor), not just
  // missing measurements. V/E evaluates collection after that qualification.
  // iOS JSC and Harmony JSVM normally depress E/A for full/core; hiding them
  // before computing it would present a minority-engine score as universal.
  // These are received-summary ratios, not independent per-instance source
  // inclusion probabilities. With fixed process probability and unequal GC/RC
  // slot rates, neither ratio can reconstruct unmeasurable engine memory.
  summary_published_ = true;  // Failure also consumes the one-summary budget.
  event.SetProps("has_early_usage_summary", true);
  event.SetProps("early_full_profile_eligible", early_full_eligible_);
  event.SetProps("early_core_profile_eligible", early_core_eligible_);
  event.SetProps("early_window_target_duration_ms", target);
  uint32_t required = 0;
  int32_t count = 0;
  for (size_t i = 0; i < 7; ++i) {
    if (kPageMemoryOffsetsMs[i] <= target) {
      required |= 1u << i;
      ++count;
    }
  }
  event.SetProps("early_required_scheduled_count", count);
  AddSampleProps(event, early_full_, required, false);
  AddSampleProps(event, early_core_, required, true);
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
  const auto build_started_at = fml::TimePoint::Now();
  auto event = BuildPageEvent(usage, *state, now_ms, index);
  if (index < 7 && !summary_published_) {
    ObserveEarly(usage, now_ms - anchor_at_ms_, static_cast<int>(index));
  }
  if (!summary_published_ &&
      (index == 6 || now_ms - anchor_at_ms_ >= kEarlyMemoryWindowMs)) {
    // Publish even when a delayed runner skipped 30s: missing points still
    // invalidate the summary, never backfill them.
    //
    // COMPLETE-WINDOW CONTROL uses the primary summary itself:
    //   has_early_usage_summary=1; early_usage_summary_full_valid=1
    //   early_window_target_duration_ms=30000; early_required_scheduled_count=7
    //   -> AVG/PCT95(early_sample_mean_full_bytes) by normalized_url.
    // This excludes short windows without uploading a duplicate mean/flag
    // block. Use the core validity/mean fields for the independent core slice.
    AddEarlySummary(event, kEarlyMemoryWindowMs);
  } else {
    event.SetProps("has_early_usage_summary", false);
  }
  EnqueueEvent(std::move(event), build_started_at);
}

void MemoryMonitor::ReportPendingSummary(int64_t end_at_ms) {
  if (anchor_at_ms_ < 0 || summary_published_) return;
  const auto build_started_at = fml::TimePoint::Now();
  // Do not query GlobalMemoryMonitor, records, slots or engine state here.
  // The two actors can tear down in either order: these values were frozen
  // during scheduled sampling, before any runtime teardown callbacks.
  auto event = std::move(summary_identity_);
  if (end_at_ms < anchor_at_ms_) scope_changed_ = true;
  event.SetProps("report_kind", "early_summary");
  event.SetProps("accounting_scope_changed", scope_changed_);
  // No exit memory measurement is made. End time only bounds the due plan.
  // summary_end_time_known=1 means Shell supplied logical destruction time;
  // 0 means monitor destruction time was used and may include teardown delay.
  // For a strict logical-end cohort use:
  //   report_kind!="early_summary" OR summary_end_time_known=1.
  // Do NOT require summary_end_time_known=1 on every main-summary row:
  // scheduled carriers legitimately omit this field. The default leaderboard
  // can include both, while exposing unknown-end share as a quality slice.
  // Crashes/forced termination may never run this path; received short-page
  // summaries are not proof of coverage of every short high-memory page.
  event.SetProps("summary_end_time_known", exit_at_ms_ >= 0);
  AddEarlySummary(event, std::clamp<int64_t>(end_at_ms - anchor_at_ms_, 0,
                                             kEarlyMemoryWindowMs));
  EnqueueEvent(std::move(event), build_started_at);
}

void MemoryMonitor::EnqueueEvent(report::MoveOnlyEvent event,
                                 fml::TimePoint build_started_at) {
  // Deduplicate transport retries by event_id before any count/SUM/PCT. It
  // distinguishes scheduled points and the optional final summary; it is not
  // a new instance ID. has_early_usage_summary then selects at most one row
  // per (process_session_id, instance_id) for the primary leaderboard.
  event.SetProps("event_id", GlobalMemoryMonitor::ProcessSessionId() + "/" +
                                 std::to_string(instance_id_) + "/" +
                                 std::to_string(++event_sequence_));
  // MONITOR COST QUERY: AVG/PCT95/PCT99(event_build_duration_us), grouped by
  // report_kind and has_early_usage_summary (optionally scheduled_offset_ms).
  // Do not filter to successful memory samples: failed summaries cost work
  // too. This is monotonic elapsed wall time, not CPU time; sub-microsecond
  // builds can round to zero. It includes fields, sample/summary work and ID,
  // but not prior cache refresh, earlier identity saving, this property's own
  // insertion, queue waiting, flush or platform delivery. SUM gives aggregate
  // build time over observed events, not process CPU utilization.
  event.SetProps("event_build_duration_us",
                 (fml::TimePoint::Now() - build_started_at).ToMicroseconds());
  report::EventTracker::OnEvent(
      [event = std::move(event)](auto& target) mutable {
        target = std::move(event);
      });
}

}  // namespace performance
}  // namespace tasm
}  // namespace lynx
