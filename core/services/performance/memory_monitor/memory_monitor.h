// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_MEMORY_MONITOR_H_
#define CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_MEMORY_MONITOR_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "base/include/fml/time/time_point.h"
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

// @class MemoryMonitor
// @brief A class for monitoring memory usage and managing memory records.
//
// The MemoryMonitor class provides functionality to allocate, deallocate,
// and update memory usage records. It maintains a mapping of categories to
// their respective MemoryRecord instances, allowing for efficient tracking
// of memory utilization.
class MemoryMonitor {
  friend class PerformanceController;
  friend class MemoryMonitorTest;

 public:
  // Collection profiles for the query recipes in AddSamplingProps,
  // BuildPageEvent/AddSampleProps and GlobalMemoryMonitor::BuildEvent:
  //
  // Configure MEMORY_MONITOR_CONFIG before the first GetSettings()/VM creation.
  // It is a comma-separated key=value string; configure *.sample_rate, NOT
  // *.enabled. Each enabled value is drawn ONCE PER PROCESS, conditional on
  // global.enabled. "per_instance" means QuickJS slot attribution, not another
  // independent Bernoulli draw for every instance. Existing VMs are not
  // rebuilt. The default global.sample_rate=0 disables all collection.
  //
  // FULL profile, for complete page accounting plus observable slot residuals:
  //   global.sample_rate=0.01,tasm_element.sample_rate=1,lynx_ui.sample_rate=1,
  //   bts_vm_per_instance_gc.sample_rate=1,
  //   bts_vm_per_instance_rc.sample_rate=1
  // Shown wrapped; use one comma-separated string. 0.01 is an illustrative
  // rollout probability, not a new default; choose it for volume/cost. Within
  // selected processes, keeping required sources at 1 simplifies statistics.
  //
  // GLOBAL-ONLY profile: same global/Element/UI rates, both slot rates=0.
  // Shared QuickJS/V8/JSVM whole heaps do not require page slots. This supports
  // the global total/ratio when other sources are complete, but sacrifices
  // shared-QuickJS page full/core, URL byte totals and exited-slot attribution.
  // Private QuickJS pages still use their whole heap without requiring slots.
  //
  // CORE-PAGE profile: FULL profile with lynx_ui.sample_rate=0. It supports
  // a separately named core page ranking, not full page/process accounting
  // when UI is required. Core excludes ONLY UI; it still requires MTS and BTS.
  //
  // Capability limits apply to every profile, even when all rates are 1:
  // - iOS mostly JSC, with some QuickJS: JSC has no measured BTS heap/slot.
  // - Android/PC mostly QuickJS, with some V8: V8 has a shared whole heap only.
  // - Harmony mostly JSVM, with some QuickJS: JSVM has only shared-heap data.
  //
  // QuickJS can attribute shared page slots in either GC or RC mode, subject
  // to its actual tracking flag, slot capacity and callbacks. GC/RC options
  // select tracking in the respective mode; they do not switch the engine's
  // GC mode, force collection, or certify that residual bytes are leaks.
  // The only supported MTS heap here is page-private LepusNG/QuickJS.
  // Enabling a source does not implement a missing platform UI producer.
  // See the event's actual eligibility/validity flags, not the OS name alone.
  //
  // global.time_interval_sec and pss_threshold_mb choose global observations;
  // mts_threshold_mb/bts_threshold_mb configure engine cache-update thresholds.
  // They do not make missing sources eligible or request a synchronous page
  // measurement. Keep these settings fixed when comparing versions/engines:
  // most are not emitted as configuration fields, so use host experiment
  // metadata to identify the cohort instead of inventing event filters.
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

    // Entries with default sample_rate and threshold. After initialization is
    // complete, please read the `enabled` field.
    GlobalEntry global{{0.0f}, 300, 256, 1, 2};

    Entry tasm_element{1.0f};
    Entry lynx_ui{0.1f};
    Entry bts_vm_per_instance_gc{1.0f};
    Entry bts_vm_per_instance_rc{0.1f};
  };

  static const Settings& GetSettings();

  // Producer updates only refresh the cache; they do not add sampled events.
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

  // Recalculate the cache; force_report emits only the diagnostic Trace
  // counter. The two telemetry transports have their own fixed event budgets.
  void ReportMemory(bool force_report = false);

  // Called once for the first completed loadBundle pipeline. Later pipeline
  // updates do not restart the fixed plan. Reload invalidates its scope.
  void OnFirstLoadComplete();
  void OnReload();
  // Timing metadata only: bounds the due scheduled points, never triggers a
  // measurement. Shell supplies this without waiting for runtime teardown.
  void SetExitTime(int64_t at_ms) { exit_at_ms_ = at_ms; }
  static void AddSamplingProps(report::MoveOnlyEvent& event);

  // Checks if memory monitoring is enabled.
  // Modules can call this before collecting data to avoid unnecessary
  // collection.
  static bool Enable() { return GetSettings().global.enabled; }

  // Test startup only: enable monitoring and all sources before creating
  // pages/VMs. Production sampling settings remain fixed for the process.
  static void ForceEnableForTesting();

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

  const auto& GetMemoryRecords() const { return memory_records_; }

 private:
  PageMemoryUsage RefreshMemoryUsage();
  void ScheduleNext();
  void ReportScheduled(size_t index, int64_t now_ms);
  void ReportPendingSummary(int64_t end_at_ms);
  report::MoveOnlyEvent BuildIdentityEvent(const InstanceState& state);
  report::MoveOnlyEvent BuildPageEvent(const PageMemoryUsage& usage,
                                       const InstanceState& state,
                                       int64_t now_ms, size_t scheduled_index);
  void EnqueueEvent(report::MoveOnlyEvent event,
                    fml::TimePoint build_started_at);
  void ObserveEarly(const PageMemoryUsage& usage, int64_t at_ms, int index);
  void AddEarlySummary(report::MoveOnlyEvent& event, int64_t target_ms);
  void AddSampleProps(report::MoveOnlyEvent& event,
                      const EarlyMemorySamples& samples, uint32_t required_mask,
                      bool core);

  int32_t instance_id_ = report::kUninitializedInstanceId;
  base::LogContext log_context_;
  std::unordered_map<MemoryCategory, MemoryRecord> memory_records_;
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
  uint64_t event_sequence_{0};
  bool scope_changed_{false};
  // Invalid/partial summaries also consume the budget. Retrying only failed
  // summaries would change each instance's inclusion probability.
  bool summary_published_{false};
  // Freeze capability/sampling eligibility at the anchor, independently of
  // whether measurements subsequently arrive. Keep failures in the denominator.
  bool early_full_eligible_{false};
  bool early_core_eligible_{false};
  // Identity and sampling metadata frozen at the anchor. Destruction can emit
  // this even after InstanceState/VM removal, without querying either or
  // rebuilding metadata from teardown-mutated records.
  report::MoveOnlyEvent summary_identity_;
  // Bounded online state: only the first seven scheduled points, no exit point.
  EarlyMemorySamples early_full_;
  EarlyMemorySamples early_core_;
  std::string normalized_url_;
};

}  // namespace performance
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_MEMORY_MONITOR_H_
