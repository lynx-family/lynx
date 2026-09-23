// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_GLOBAL_MEMORY_MONITOR_H_
#define CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_GLOBAL_MEMORY_MONITOR_H_

#include <array>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/closure.h"
#include "base/include/fml/memory/js_memory_track_define.h"
#include "base/include/fml/time/timer.h"
#include "base/include/notification_center.h"
#include "core/runtime/js/jsi/jsi.h"
#include "core/runtime/mts_context.h"
#include "core/services/event_report/event_tracker.h"
#include "core/services/performance/memory_monitor/memory_monitor_data.h"

namespace lynx {
namespace tasm {
namespace performance {

class MemoryMonitor;

// One observation per creation, including pages that never finish loading.
// Early-exit metadata survives until a poll evaluates the fixed 10min window.
// No page or VM is kept alive to complete the observation.
struct ResidencyObservation {
  int32_t instance_id;
  int64_t created_at_ms;
  int64_t exited_at_ms{-1};
  // Saved on exit; live observations read the instance's current key.
  std::string page_key;
};

struct InstanceState {
  void SetUrl(const std::string& url);
  const std::string& url() const { return url_; }

  // Reporter-thread-only observer, cleared before PerformanceController
  // teardown. Global snapshots use it to aggregate current producer records.
  MemoryMonitor* memory_monitor{nullptr};

  // Resolved once at first-load completion: nonempty host page_id other than
  // "unknown", otherwise the platform-specific URL fallback. Before that,
  // SetUrl updates the fallback; residency does not wait for resolution.
  std::string page_key;
  // Page's business group, not necessarily the actual VM's name (V8/JSVM).
  std::string group_id;
  bool page_key_resolved{false};
  // Logical shell exit excludes this page from active sampling. Only tracked
  // shared-slot metadata survives for residual attribution; page summaries
  // own their saved observations and do not hold up cleanup.
  bool destroyed{false};
  // Default enum values below are not evidence of an initialized engine.
  bool mts_known{false};
  bool bts_known{false};
  // False only when the shell explicitly disables BTS, not when it is late.
  bool bts_expected{true};
  runtime::ContextType mts_context_type{runtime::VMContextType};
  runtime::js::JSRuntimeType bts_runtime_type{runtime::js::JSRuntimeType::jsc};
  int32_t slot{fml::JSMemoryTrackSlotType::Unknown};
  // Lookup identity only. Never dereferenced on the reporter thread.
  runtime::js::VMInstance* bts_vm{nullptr};
  // Monotonic times: page age uses creation, not the first-load sample anchor.
  int64_t created_at_ms{0};
  int64_t exited_at_ms{0};
  ResidencyObservation* residency_observation{nullptr};
  // Total is duplicated explicitly for the public sampling contract. `usage`
  // retains the BTS component used in that same total, allowing exact dedup.
  size_t memory_usage{0};
  PageMemoryUsage usage;

 private:
  // Preserve the host URL verbatim, including nested routes in query/fragment.
  std::string url_;
};

struct BtsVmState {
  void SetName(const std::string& name) { name_ = name; }
  const std::string& vm_name() const {
    return !monitoring_group_label.empty() ? monitoring_group_label : name_;
  }

  // Stable analytics dimension (group or "runtime_manager/v8"), not a unique
  // object ID. Same-name VMs remain separate records, identified by vm_ptr/vm.
  std::string monitoring_group_label;
  runtime::js::JSRuntimeType type{runtime::js::JSRuntimeType::jsc};
  bool runtime_manager_scope{false};
  // Valid zero heap differs from unknown JSC/uninitialized/missing data.
  bool heap_valid{false};
  // Tracking may be enabled before its first callback has delivered slots.
  bool has_per_instance_data{false};
  bool slot_tracking_enabled{false};
  // First failed allocation; capacity reached alone does not imply overflow.
  bool overflowed{false};
  // An erased untracked exit must not turn an incomplete residual into zero.
  bool has_untracked_exited_pages{false};
  fml::RefPtr<fml::TaskRunner> js_thread_runner;
  std::weak_ptr<runtime::js::VMInstance> vm;
  runtime::js::VMInstance* vm_ptr{nullptr};
  size_t heap_size{0};
  // Slots survive page exit and are never reused. This current cache serves
  // active pages and reserved-slot diagnostics.
  // Reserved slots 0/1/2 are Unknown/Common/Overflow, already included in
  // heap_size. They are VM-level categories, never individual page slots.
  std::array<size_t, fml::JSMemoryTrackSlotType::Count> memory_slots{};
  // Preserve the last valid full-GC snapshot across ordinary updates. A page
  // qualifies only if exited_at_ms < full_gc_at_ms (equal-ms order is unknown).
  // Keeping all slots also handles a delayed exit notification without a
  // per-slot lifecycle map or a scan of the instance registry on every GC.
  std::array<size_t, fml::JSMemoryTrackSlotType::Count> full_gc_slots{};
  int64_t full_gc_at_ms{0};
  int64_t created_at_ms{0};
  int64_t measured_at_ms{0};
  // Time with zero active pages, not "no JS task has executed". Reset on bind.
  int64_t idle_since_ms{0};
  // Counts accepted page-to-VM bindings, including untracked pages. Repeated
  // property updates/reload do not increment it; leaving and rebinding can.
  // It counts neither successful loads nor lifetime distinct instances.
  uint64_t total_page_count{0};
  uint32_t active_page_count{0};
  uint32_t allocated_slots{0};
  uint64_t untracked_page_count{0};
  // Emission is deferred until RuntimeManager commits the failed page binding.
  bool pending_overflow{false};

 private:
  std::string name_;
};

struct BtsMemorySample {
  size_t heap_size{0};
  bool heap_valid{false};
  bool slots_valid{false};
  // The engine, not the reporter, identifies a completed full GC.
  bool is_full_gc{false};
  int64_t measured_at_ms{0};
  std::array<size_t, fml::JSMemoryTrackSlotType::Count> slots{};
};

class GlobalMemoryMonitor {
 public:
  GlobalMemoryMonitor();
  ~GlobalMemoryMonitor() = default;

  // Monitoring starts on the first enabled page/VM/local-pool registration.
  static GlobalMemoryMonitor& GetInstance();
  static const char* RuntimeName(runtime::js::JSRuntimeType type);

  using WithInstanceCallback = base::MoveOnlyClosure<void, InstanceState&>;
  using InstanceMemoryUsageCallback =
      base::MoveOnlyClosure<void, InstanceMemoryUsageResult>;
  using GlobalMemoryUsageCallback =
      base::MoveOnlyClosure<void, GlobalMemoryUsage>;

  // Register, enroll in residency, and apply initial fields in one reporter
  // task. Duplicate registration does not restart the observation window.
  void OnInstanceCreated(int32_t id, int64_t at_ms,
                         WithInstanceCallback&& callback);
  // Shell-owned logical exit. Idempotently update VM counts and retain only
  // attributable shared-slot metadata, without taking an exit memory sample.
  void OnInstanceDestroyed(int32_t id, int64_t at_ms);
  // Only explicit creation can add an instance; late updates cannot resurrect
  // an exited/erased page. These entry points accept calls from any thread.
  void WithInstance(int32_t id, WithInstanceCallback&& callback);
  // group_id names QuickJS/JSC shared groups. V8/JSVM callers pass an empty
  // group; their stable reporting name is derived here from the runtime type.
  void OnBtsVMCreate(const std::string& group_id,
                     const std::string& monitoring_group_label,
                     const std::shared_ptr<runtime::js::VMInstance>& vm);
  // The engine copies values on its owning JS thread; no engine-specific
  // headers, pointer reads or GC interpretation belong in this monitor.
  void OnBtsVMMemoryUpdate(runtime::js::VMInstance* vm, BtsMemorySample sample);
  void OnBtsVMSlotAllocate(runtime::js::VMInstance* vm, int32_t slot);
  // Bundle-local pools publish owner-thread heap snapshots. LynxGlobalPool
  // never calls these entry points.
  void OnMTSRuntimePoolUpdate(int32_t pool_instance_id,
                              const std::string& template_url,
                              runtime::ContextType context_type,
                              int64_t created_at_ms, uint64_t runtime_count,
                              int64_t heap_bytes);
  void OnMTSRuntimePoolDestroy(int32_t pool_instance_id);
  // Runs the query and callback on the reporter thread. Invalid, missing, and
  // destroyed instances share one status. Unsupported or failed source reads
  // still return kOk; callers determine field validity from positive values.
  void GetInstanceMemoryUsage(int32_t id,
                              InstanceMemoryUsageCallback&& callback);
  // Refreshes every active page and returns a process-wide snapshot on the
  // reporter thread. Shared BTS heaps are counted once by VM identity.
  void GetGlobalMemoryUsage(GlobalMemoryUsageCallback&& callback);

  // Reporter-thread-only reads/updates. Missing shared VM returns SIZE_MAX:
  // use the standalone page BTS record. Zero alone does not prove validity.
  size_t GetInstanceBtsMemoryUsage(int32_t id);
  const InstanceState* GetInstanceState(int32_t id);
  PageMemoryUsage UpdatePageMemory(int32_t id, PageMemoryUsage usage);

 private:
  friend class MemoryMonitor;
  friend class MemoryMonitorTest;
  GlobalMemoryMonitor(const GlobalMemoryMonitor&) = delete;
  GlobalMemoryMonitor& operator=(const GlobalMemoryMonitor&) = delete;

  enum Reason : uint32_t {
    kTimer = 1,
    kPss = 2,
    kWarning = 4,
    kOverflow = 8,
  };
  struct Trigger {
    uint32_t reasons{0};
    int64_t pss_before{0};
    int64_t pss_after{0};
    std::unique_ptr<BtsVmState> overflow_vm;
  };
  // Collection and BuildEvents run in one reporter task. overflow_vm is the
  // frozen trigger-time copy; other state remains reporter-owned.
  struct Snapshot {
    int64_t started_at_ms{0};
    Trigger trigger;
    // A missing runner or expired VM keeps its cache for later recovery, but
    // the failed VM cannot publish heap fields in this snapshot.
    bool partial{false};
    std::vector<runtime::js::VMInstance*> failed_heap_vms;
  };

  void Start();
  void PollProcessMemory();
  void RequestReport(Trigger trigger);
  // Completes within one reporter task, waiting at most one second for each
  // heap-only engine on its JS runner. Queued reporter updates cannot
  // interleave.
  Snapshot CollectSnapshot(Trigger trigger,
                           bool refresh_trackable_heaps = false);
  void RefreshVmHeap(BtsVmState& vm, Snapshot& snapshot,
                     bool refresh_trackable_heaps);
  std::vector<report::MoveOnlyEvent> BuildEvents(const Snapshot& snapshot,
                                                 int64_t now_ms, int64_t pss);
  bool IsHeapAvailable(
      const BtsVmState& vm,
      const std::vector<runtime::js::VMInstance*>& failed_heap_vms) const;
  InstanceMemoryUsage BuildInstanceMemoryUsage(
      int32_t id, const InstanceState& page,
      const std::vector<runtime::js::VMInstance*>& failed_heap_vms);
  GlobalMemoryUsage BuildGlobalMemoryUsage(const Snapshot& snapshot);
  BtsVmState* FindVM(runtime::js::VMInstance* vm);
  PageMemoryUsage RefreshInstanceMemoryUsage(InstanceState& page);
  void OnBtsVMDestroy(runtime::js::VMInstance* vm);
  void BindInstance(InstanceState& state, runtime::js::VMInstance* previous);
  // Called only by MemoryMonitor::OnFirstLoadComplete on the reporter thread.
  const InstanceState* ResolveInstancePageKey(int32_t id);
  void ResolveMTSRuntimePoolPageKeys();
  std::vector<report::MoveOnlyEvent> TakeResidencyReports(int64_t now_ms);
  void RefreshPageBts(InstanceState& page, const BtsVmState& vm);
  void EmitSlotOverflowTrigger(BtsVmState& vm);

  // All containers and timers below are reporter-thread confined. The singleton
  // outlives queued work; query tasks hold VM strong refs only on the JS
  // thread.
  std::unordered_map<int32_t, InstanceState> instance_state_;
  std::vector<std::unique_ptr<BtsVmState>> bts_vm_state_;
  std::unordered_map<int32_t, MTSRuntimePoolState> mts_runtime_pool_state_;
  // Stable pointees while pending unique_ptrs move into a replacement vector.
  // Creation notifications can arrive out of timestamp order, so scan all
  // entries when due; do not stop at the first future deadline.
  std::vector<std::unique_ptr<ResidencyObservation>> residency_observations_;
  // Earliest pending deadline, or 0. The 8s process poll skips the scan until
  // due; no per-page or dedicated residency timer is needed.
  int64_t next_residency_at_ms_{0};
  std::unique_ptr<fml::RepeatingTimer> poll_timer_;
  std::unique_ptr<base::NotificationCallback> pressure_callback_;
  base::NotificationCallback devtool_connected_callback_;
  std::mt19937_64 random_{std::random_device{}()};
  int64_t started_at_ms_{0};
  // Consecutive snapshot endpoint rectangle estimate, not heap * VM lifetime.
  int64_t previous_snapshot_at_ms_{0};
  int64_t next_timer_at_ms_{0};
  int64_t pss_high_water_bucket_{0};
};

}  // namespace performance
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_GLOBAL_MEMORY_MONITOR_H_
