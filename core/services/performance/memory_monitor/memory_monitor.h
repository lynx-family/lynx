// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_MEMORY_MONITOR_H_
#define CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_MEMORY_MONITOR_H_

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "base/include/fml/time/timer.h"
#include "base/include/log/log_context.h"
#include "base/include/log/logging.h"
#include "core/services/event_report/event_tracker.h"
#include "core/services/performance/memory_monitor/memory_monitor_data.h"
#include "core/services/performance/memory_monitor/memory_record.h"
#include "core/services/performance/performance_event_sender.h"

namespace lynx {
namespace tasm {
namespace performance {

class GlobalMemoryMonitor;
struct InstanceState;
inline constexpr char kRuntimeId[] = "runtimeId";
inline constexpr char kRuntimeGroupId[] = "groupId";

// Caches page memory by source and emits scheduled observations plus one
// early summary. GlobalMemoryMonitor owns process snapshots and residency.
//
// Monitoring questions and query contract, schema_version=10:
// - lynxsdk_performance_entry_memory_v2: Q1/Q2 page observations.
// - lynxsdk_memory_page_residency: Q3.1 fixed 10min lifecycle cohorts.
// - lynxsdk_memory_page: Q3.2/Q4 periodic active-page samples.
// - lynxsdk_memory_vm_stat: Q5.1/Q5.2/Q9.1 periodic all-engine VM samples.
// - lynxsdk_memory_vm_quickjs: Q5.3/Q9.2/Q10 QuickJS-only data.
// - lynxsdk_memory_page_residual: Q6 periodic positive-residual PPS samples.
// - lynxsdk_memory_persistence: Q7 all-trigger endpoint byte-time samples.
// - lynxsdk_memory_process: process accounting and Q8 PSS milestones.
// - lynxsdk_memory_mts_pool: Q11 bundle-local MTS pool samples.
// Event production owns fixed trigger and data-availability gates. Queries
// should not reconstruct those gates from quality flags.
//
// Q1 Which page keys use the most memory per instance in the first 20s?
//    Select has_early_usage_summary=1 and require the queried field to exist.
//    Q1.1: P95 of early_sample_mean_memory_bytes, one mean per instance.
//    Q1.2: P95 of early_sample_peak_memory_bytes, one sampled peak per
//    instance.
//    Q1.3: P95 of early_sample_mean_bts_attributed_bytes.
//    Q1.4: P95 of early_sample_peak_bts_attributed_bytes.
//    Q1.5: P95 of early_sample_mean_mts_heap_bytes.
//    Q1.6: P95 of early_sample_peak_mts_heap_bytes.
//    All include short-page cohorts; keep target duration,
//    lynx_ui_memory_enabled and runtime-type cohorts fixed. BTS is page
//    attribution, not the whole shared heap. Peaks cover only planned cache
//    observations, not continuous memory measurements.
// Q2 What is memory at 5s after load? Count/AVG/PCT of
//    current_page_memory_bytes at one scheduled_offset_ms. The value is the sum
//    of all available page records; filter unsupported MTS/BTS runtime types
//    as needed.
// Q3 Which page keys survive 10min after creation, conditional on the process
//    completing that window? Exact alive/evaluated counts, including pages
//    that exit early; incomplete windows contribute neither count.
//    Q3.1: A=SUM(alive_page_count), B=SUM(evaluated_page_count), ratio=A/B.
//    Q3.2: separate age>=10min snapshot query; rank resident memory
//    by A=SUM(page_memory_bytes_weighted).
//    B=SUM(active_page_count) gives mean resident instance memory A/B.
//    Both use creation age.
// Q4 Which page keys consume memory through concurrent instances? For n
//    members of a drawn key among N instances, group bytes S: A=SUM(S*N/n),
//    B=SUM(N), C=SUM(N/n); compare A/C and B/C.
// Q5 Which shared VMs are large and long-lived?
//    Q5.1: age>=30min; A=SUM(heap*V), B=SUM(heap selection weight).
//    Heap fields and their denominator exist only for usable heap samples.
//    Rank heap contribution by A and VM means by A/B.
//    The active_page_count=0, idle>=10min subset identifies unused old VMs.
//    Q5.2: P50/P90/P95/P99 of raw heap, grouping by name/type and exact V.
//    Pooling different V needs weighted quantiles; PCT(heap*V)/PCT(V) is wrong.
//    Q5.3: report_kind=sample QuickJS VM exited-slot residuals, including zero.
//    Producer qualification requires tracking/slots/full GC, no pending
//    pages in the selected VM, slots<253 and no failed/untracked exit history.
//    The dedicated retained weight is the matching denominator.
// Q6 Which exited page keys retain memory after full GC? Use tracked QuickJS
//    slots measured strictly after exit and residual PPS.
//    A=SUM(L/p), B=SUM(selection_weight=1/p), and positive-residual mean A/B.
//    Both sums require the same rows with a present, positive weight.
//    Zero, pending, reserved and untracked slots never enter the residual draw.
//    Pending pages do not invalidate confirmed samples.
// Q7 Which memory persists? The persistence event contains VM/exited-page-key
//    byte-time terms over all trigger endpoints, capped at 5min and clipped to
//    creation/exit. Field presence is the source-specific validity gate.
// Q8 What is Lynx's measured share at 1/2/3GiB? Process events require positive
//    PSS and complete synchronous VM collection. Group ratios by RAM/milestone.
//    A ratio percentile is not the ratio of separate memory percentiles.
// Q9 Which VMs repeatedly load pages?
//    Q9.1: uniform all-engine lifetime binding counts with no heap gate;
//    A=SUM(vm_total_page_count_weighted),
//    B=SUM(vm_selection_weight); mean bindings per VM snapshot=A/B.
//    Reloads are not new bindings; a snapshot repeats prior binding history.
//    Q9.2: report_kind=slot_overflow age and load count, with no entity weight.
// Q10 Which shared QuickJS VM names have high Unknown(0)/Common(1)/Overflow(2)?
//    Use report_kind=sample.
//    Group raw slot bytes by vm_name and compare P50/P90/P95/P99.
//    Unknown indicates missing scheduler attribution, Common is shared engine
//    overhead, Overflow aggregates allocations after capacity exhaustion.
//    Keep valid zeros, never impute unknown data; all three belong to the heap.
// Q11 Which bundle-local MTS pools use the most memory?
//    Timer snapshots select one physical pool uniformly, including every
//    context type and valid zero heaps. Group by page_key/mts_context_type.
//    A=SUM(mts_pool_heap_bytes_weighted),
//    B=SUM(mts_pool_heap_selection_weight), C=SUM(
//    mts_pool_runtime_count_weighted). Rank cumulative heap by A, mean heap
//    per physical pool snapshot by A/B, and mean runtime count by C/B.
//    Pool heaps are included in process accounting, but never in
//    GlobalMemoryUsage.total.mts_bytes or page memory.
//
// Detailed field gates and estimator rationale are in the memory monitor docs.
class MemoryMonitor {
  friend class GlobalMemoryMonitor;
  friend class PerformanceController;
  friend class MemoryMonitorTest;

 public:
  // Configure MEMORY_MONITOR_CONFIG before the first GetSettings()/VM creation
  // as comma-separated key/value pairs, for example:
  // global_sample_rate,1,lynx_ui_sample_rate,1,pss_threshold_mb,256
  // global_sample_rate selects processes and enables Element, MTS, BTS and
  // QuickJS slot collection together. lynx_ui_sample_rate is the only
  // additional source draw and is conditional on global selection. Events
  // whose totals include UI memory emit lynx_ui_memory_enabled. Existing VMs
  // are not rebuilt. The default global_sample_rate is 0, disabling collection.
  // page_scheduled_sample_rate independently selects processes for scheduled
  // page-event delivery, also conditional on global selection. It does not
  // change planned observations, early summaries, or global reporting.
  //
  // time_interval_sec and pss_threshold_mb choose global observations;
  // mts_threshold_mb/bts_threshold_mb configure engine cache-update thresholds.
  // They do not request a synchronous page measurement.
  struct Settings {
    struct Entry {
      float sample_rate;
      uint32_t enabled{0};
    };

    struct GlobalEntry : public Entry {
      uint32_t time_interval_sec;
      uint32_t pss_threshold_mb;
      uint32_t mts_threshold_mb;
      uint32_t bts_threshold_mb;
    };

    // After initialization is complete, read the `enabled` field.
    GlobalEntry global{{0.0f}, 300, 256, 1, 1};
    Entry lynx_ui{0.1f};
    Entry page_scheduled{0.1f};

    void ApplyConfig(std::string_view config);
  };

  static const Settings& GetSettings();

  // Producer updates only mutate category records; page observations and
  // active trace emissions aggregate them.
  // This interface will increase the total memory usage for the category found
  // in the record.
  void AllocateMemory(MemoryRecord&& record);

  // Decrements cached memory usage.
  // This interface will decrease the total memory usage for the category found
  // in the record.
  void DeallocateMemory(MemoryRecord&& record);

  // Overwrites cached usage.
  // This interface will overwrite the record corresponding to the category in
  // the record, effectively updating the memory usage information.
  void UpdateMemoryUsage(MemoryRecord&& record, bool force_report = false);

  // Aggregate current category records and report page memory to trace.
  void ReportMemoryForTrace(bool force_report = false, bool is_exit = false);

  // First completed loadBundle starts the fixed plan and attempts 0s now.
  // Later completions and reload do not restart it.
  // Pages without this anchor still participate in global residency.
  void OnFirstLoadComplete();
  // Timing metadata only: bounds the due scheduled points, never triggers a
  // measurement. Shell supplies this without waiting for runtime teardown.
  void SetExitTime(int64_t at_ms) { exit_at_ms_ = at_ms; }
  static void AddCommonProps(report::MoveOnlyEvent& event);

  // Checks if memory monitoring is enabled.
  // Modules can call this before collecting data to avoid unnecessary
  // collection.
  static bool Enable() { return GetSettings().global.enabled; }

  enum class ForceEnableMode {
    // Enable only this process. Used by tests and explicit host switches.
    kCurrentProcess,
    // In Perfetto builds, also persist a marker for subsequent app launches.
    // Allows users to enable monitoring without overriding sampling settings.
    kPersistent,
  };

  // Enable monitoring and all sources before creating pages/VMs. In Perfetto
  // builds, persistent mode records the choice in the platform trace directory.
  static void ForceEnableForTesting(ForceEnableMode mode);

  /// @brief Generates a bitmask for scripting engine memory monitoring
  /// configuration This method combines memory monitoring status and memory
  /// increment threshold into a uint32_t bitmask:
  /// - Bits 24-31 : Memory increment threshold in MiB (capped at 255MiB)
  /// @param is_mts MTS and BTS use different threshold value
  /// @return uint32_t Combined configuration bitmask
  static uint32_t ScriptingEngineMode(bool is_mts);

  explicit MemoryMonitor(
      PerformanceEventSender* observer, const base::LogContext& log_context,
      int32_t instance_id = report::kUninitializedInstanceId);
  ~MemoryMonitor();
  MemoryMonitor(const MemoryMonitor& timing) = delete;
  MemoryMonitor& operator=(const MemoryMonitor&) = delete;
  MemoryMonitor(MemoryMonitor&& other) = delete;
  MemoryMonitor& operator=(MemoryMonitor&& other) = delete;

  void SetLogContext(const base::LogContext& log_context) {
    log_context_ = log_context;
  }

 private:
  PageMemoryUsage RefreshMemoryUsage();
  void ScheduleNext();
  void ReportScheduled(size_t index, int64_t now_ms);
  // Publish a pending summary from saved observations at destruction or when
  // scheduled event delivery is disabled.
  // A summary already attached to a scheduled event consumes this budget,
  // even if invalid. No exit-time memory read or second summary is needed.
  void ReportPendingSummary(int64_t end_at_ms);
  report::MoveOnlyEvent BuildIdentityEvent(const InstanceState& state);
  report::MoveOnlyEvent BuildPageEvent(const PageMemoryUsage& usage,
                                       const InstanceState& state,
                                       size_t scheduled_index);
  void EnqueueEvent(report::MoveOnlyEvent event);
  void ObserveEarly(const PageMemoryUsage& usage, int index);
  void AddEarlySummary(report::MoveOnlyEvent& event, int64_t target_ms);
  void AddSampleProps(report::MoveOnlyEvent& event,
                      const EarlyMemorySamples& samples,
                      std::string_view memory_kind, uint32_t required_mask);

  int32_t instance_id_ = report::kUninitializedInstanceId;
  base::LogContext log_context_;
  std::unordered_map<MemoryCategory, MemoryRecord> memory_records_;
#if ENABLE_TRACE_PERFETTO
  // When Lynx Trace captures memory data, the threshold for VM to report memory
  // changes is relatively small. Adding time control helps prevent excessive
  // events from being written to the trace file.
  fml::TimePoint last_trace_event_time_;
#endif

  GlobalMemoryMonitor* global_;
  // Latest ownership-correct cache for normal updates. Never read to generate
  // an end-of-page summary; only scheduled observations enter that summary.
  PageMemoryUsage last_usage_;
  std::unique_ptr<fml::OneshotTimer> sample_timer_;
  // First completed loadBundle receipt on report thread, not paintEnd.
  int64_t anchor_at_ms_{-1};
  // Logical shell teardown timestamp; excludes reporter queue waiting time.
  int64_t exit_at_ms_{-1};
  size_t next_sample_{0};
  // Invalid/partial summaries also consume the budget. Retrying only failed
  // summaries would change each instance's inclusion probability.
  bool summary_published_{false};
  // Identity and schema metadata frozen at the anchor. Destruction can emit
  // this even after InstanceState/VM removal, without querying either or
  // rebuilding metadata from teardown-mutated records.
  report::MoveOnlyEvent summary_identity_;
  // Independent bounded state for kEarlyMemorySampleCount points, no exit
  // point.
  EarlyMemorySamples early_;
  EarlyMemorySamples early_bts_;
  EarlyMemorySamples early_mts_;
};

}  // namespace performance
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_MEMORY_MONITOR_H_
