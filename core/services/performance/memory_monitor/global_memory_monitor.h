// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_GLOBAL_MEMORY_MONITOR_H_
#define CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_GLOBAL_MEMORY_MONITOR_H_

#include <array>
#include <memory>
#include <optional>
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

struct InstanceState {
  // URL is an analytics grouping key, never an instance identity. Multiple
  // instances with the same URL remain separate uniform-sampling candidates.
  std::string url;
  // Page's business group, not necessarily the actual VM's name (V8/JSVM).
  std::string group_id;
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
  // Total is duplicated explicitly for the public sampling contract. `usage`
  // retains the BTS component used in that same total, allowing exact dedup.
  size_t memory_usage{0};
  PageMemoryUsage usage;
};

struct BtsVmState {
  // Stable analytics dimension (group or "runtime_manager/v8"), not a unique
  // object ID. Same-name VMs remain separate records, identified by vm_ptr/vm.
  std::string name;
  runtime::js::JSRuntimeType type{runtime::js::JSRuntimeType::jsc};
  bool runtime_manager_scope{false};
  // Valid zero heap differs from unknown JSC/uninitialized/missing data.
  bool heap_valid{false};
  // Tracking may be enabled before its first callback has delivered slots.
  bool has_per_instance_data{false};
  bool slot_tracking_enabled{false};
  // First failed allocation; capacity reached alone does not imply overflow.
  bool overflowed{false};
  fml::RefPtr<fml::TaskRunner> js_thread_runner;
  std::weak_ptr<runtime::js::VMInstance> vm;
  runtime::js::VMInstance* vm_ptr{nullptr};
  size_t heap_size{0};
  // Slots survive page exit and are never reused. Exited slot bytes are
  // current residuals, not proof of a GC-confirmed leak.
  std::array<size_t, fml::JSMemoryTrackSlotType::Count> memory_slots{};
  int64_t created_at_ms{0};
  int64_t measured_at_ms{0};
  // Time with zero active pages, not "no JS task has executed". Reset on bind.
  int64_t idle_since_ms{0};
  // Counts unique instance bindings, including pages without a tracked slot.
  // Used to diagnose heavily reused V8/JSVM as well as QuickJS.
  uint64_t total_page_count{0};
  uint32_t active_page_count{0};
  uint32_t allocated_slots{0};
  uint64_t untracked_page_count{0};
  // Emission is deferred until RuntimeManager commits the new page binding.
  uint32_t pending_slot_milestone{0};
  bool pending_overflow{false};
};

struct BtsMemorySample {
  size_t heap_size{0};
  bool heap_valid{false};
  bool slots_valid{false};
  int64_t measured_at_ms{0};
  std::array<size_t, fml::JSMemoryTrackSlotType::Count> slots{};
};

class GlobalMemoryMonitor {
 public:
  GlobalMemoryMonitor() = default;
  ~GlobalMemoryMonitor() = default;

  // Monitoring starts on the first enabled page/VM registration.
  static GlobalMemoryMonitor& GetInstance();
  static const std::string& ProcessSessionId();
  static const char* RuntimeName(runtime::js::JSRuntimeType type);

  using WithInstanceCallback = base::MoveOnlyClosure<void, InstanceState&>;
  using WithBtsVMCallback = base::MoveOnlyClosure<void, BtsVmState&, size_t>;

  // Register and apply initial fields in one reporter task.
  void OnInstanceCreated(int32_t id, int64_t at_ms,
                         WithInstanceCallback&& callback);
  // Shell-owned logical exit. Idempotently update VM counts and retain only
  // attributable shared-slot metadata, without taking an exit memory sample.
  void OnInstanceDestroyed(int32_t id, int64_t at_ms);
  // Only explicit creation can add an instance; late updates cannot resurrect
  // an exited/erased page. These entry points accept calls from any thread.
  void WithInstance(int32_t id, WithInstanceCallback&& callback);
  void WithBtsVM(runtime::js::VMInstance* vm, WithBtsVMCallback&& found,
                 base::closure&& not_found = nullptr);
  // group_id names QuickJS/JSC shared groups. V8/JSVM callers pass an empty
  // group; their stable reporting name is derived here from the runtime type.
  void OnBtsVMCreate(const std::string& group_id,
                     const std::shared_ptr<runtime::js::VMInstance>& vm);
  void OnBtsVMDestroy(runtime::js::VMInstance* vm);
  // The engine copies values on its owning JS thread; no engine-specific
  // headers, pointer reads or GC interpretation belong in this monitor.
  void OnBtsVMMemoryUpdate(runtime::js::VMInstance* vm, BtsMemorySample sample);
  void OnBtsVMSlotAllocate(runtime::js::VMInstance* vm, int32_t slot);

  // Reporter-thread-only reads/updates. The existing sentinel contract is
  // intentional: missing shared VM => standalone => use the page BTS record.
  size_t GetInstanceBtsMemoryUsage(int32_t id);
  const InstanceState* GetInstanceState(int32_t id);
  PageMemoryUsage UpdatePageMemory(int32_t id, PageMemoryUsage usage);

 private:
  friend class MemoryMonitorTest;
  GlobalMemoryMonitor(const GlobalMemoryMonitor&) = delete;
  GlobalMemoryMonitor& operator=(const GlobalMemoryMonitor&) = delete;

  enum Reason : uint32_t {
    kTimer = 1,
    kPss = 2,
    kWarning = 4,
    kSlot = 8,
    kOverflow = 16,
  };
  // Only trigger-time telemetry survives the queued report, not the VM cache.
  struct SlotTrigger {
    std::string name;
    int64_t created_at_ms{0};
    size_t heap_size{0};
    uint64_t total_page_count{0};
    uint64_t untracked_page_count{0};
    uint32_t allocated_slots{0};
    uint32_t milestone{0};
    bool heap_valid{false};
  };
  struct Trigger {
    uint32_t reasons{0};
    int64_t pss_before{0};
    int64_t pss_after{0};
    std::optional<SlotTrigger> slot_vm;
  };
  // Metadata only. Collection and BuildEvent read the reporter-owned state in
  // one task; this object must not be used as a cross-task state snapshot.
  struct Snapshot {
    uint64_t id{0};
    int64_t started_at_ms{0};
    Trigger trigger;
    // A missing runner or expired VM keeps the cached candidate but marks the
    // snapshot partial. A valid zero heap is still a successful measurement.
    bool partial{false};
  };

  void Start();
  void PollProcessMemory();
  void RequestReport(Trigger trigger);
  // Completes within one reporter task, synchronously waiting for heap-only
  // engines on their JS runners. Queued reporter updates cannot interleave.
  Snapshot CollectSnapshot(Trigger trigger);
  report::MoveOnlyEvent BuildEvent(const Snapshot& snapshot, int64_t now_ms,
                                   int64_t pss);
  BtsVmState* FindVM(runtime::js::VMInstance* vm);
  void BindInstance(InstanceState& state, runtime::js::VMInstance* previous);
  void RefreshPageBts(InstanceState& page, const BtsVmState& vm);
  void EmitSlotTrigger(BtsVmState& vm);

  // All containers and timers below are reporter-thread confined. The singleton
  // outlives queued work; query tasks hold VM strong refs only on the JS
  // thread.
  std::unordered_map<int32_t, InstanceState> instance_state_;
  std::vector<BtsVmState> bts_vm_state_;
  std::unique_ptr<fml::RepeatingTimer> poll_timer_;
  std::unique_ptr<base::NotificationCallback> pressure_callback_;
  std::mt19937_64 random_{std::random_device{}()};
  uint64_t snapshot_seq_{0};
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
