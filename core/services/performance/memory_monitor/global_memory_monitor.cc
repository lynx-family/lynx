// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/performance/memory_monitor/global_memory_monitor.h"

#include <algorithm>
#include <limits>
#include <utility>

#include "base/include/fml/message_loop.h"
#include "base/include/memory/memory_pressure_level.h"
#include "base/include/no_destructor.h"
#include "base/include/vector.h"
#include "core/runtime/js/runtime_manager.h"
#include "core/services/event_report/event_tracker_platform_impl.h"
#include "core/services/performance/memory_monitor/memory_monitor.h"
#include "core/services/performance/memory_monitor/process_memory_usage.h"

namespace lynx {
namespace tasm {
namespace performance {
namespace {

constexpr uint32_t kPageSlotCapacity = fml::JSMemoryTrackSlotType::Count -
                                       fml::JSMemoryTrackSlotType::Overflow - 1;
constexpr int64_t kMiB = 1024 * 1024;

auto ReportRunner() {
  return report::EventTrackerPlatformImpl::GetReportTaskRunner();
}

bool SameOwner(const std::weak_ptr<runtime::js::VMInstance>& a,
               const std::weak_ptr<runtime::js::VMInstance>& b) {
  return !a.owner_before(b) && !b.owner_before(a);
}

void StoreSample(BtsVmState& state, const BtsMemorySample& sample) {
  state.heap_valid = sample.heap_valid;
  state.heap_size = sample.heap_size;
  state.measured_at_ms = sample.measured_at_ms;
  state.has_per_instance_data = sample.slots_valid;
  if (sample.slots_valid) {
    state.slot_tracking_enabled = true;
    state.memory_slots = sample.slots;
  }
}

}  // namespace

GlobalMemoryMonitor& GlobalMemoryMonitor::GetInstance() {
  static base::NoDestructor<GlobalMemoryMonitor> monitor;
  return *monitor;
}

const std::string& GlobalMemoryMonitor::ProcessSessionId() {
  static const std::string id = [] {
    std::random_device random;
    return std::to_string(fml::TimePoint::CurrentWallTime()
                              .ToEpochDelta()
                              .ToMicroseconds()) +
           "-" + std::to_string(random());
  }();
  return id;
}

const char* GlobalMemoryMonitor::RuntimeName(runtime::js::JSRuntimeType type) {
  switch (type) {
    case runtime::js::JSRuntimeType::quickjs:
      return "quickjs";
    case runtime::js::JSRuntimeType::v8:
      return "v8";
    case runtime::js::JSRuntimeType::jsvm:
      return "jsvm";
    case runtime::js::JSRuntimeType::jsc:
      return "jsc";
  }
  return "unknown";
}

void GlobalMemoryMonitor::Start() {
  if (poll_timer_) return;
  started_at_ms_ = MemoryNowMs();
  const auto interval = MemoryMonitor::GetSettings().global.time_interval_sec;
  next_timer_at_ms_ = interval ? started_at_ms_ + interval * int64_t{1000} : 0;
  poll_timer_ = std::make_unique<fml::RepeatingTimer>(ReportRunner());
  // One process-level PSS read per 8 seconds, not one per page. PSS buckets are
  // observed high-water marks; peaks shorter than this poll can be missed.
  poll_timer_->Start(fml::TimeDelta::FromSeconds(8),
                     [this] { PollProcessMemory(); });
  pressure_callback_ = std::make_unique<base::NotificationCallback>(
      base::MEMORY_PRESSURE_NOTIFICATION,
      [this](const std::string&, intptr_t level) {
        if (level <= base::MEMORY_PRESSURE_LEVEL_NONE) return;
        ReportRunner()->PostTask([this] {
          Trigger trigger;
          trigger.reasons = kWarning;
          RequestReport(std::move(trigger));
        });
      });
}

void GlobalMemoryMonitor::OnInstanceCreated(int32_t id, int64_t at_ms,
                                            WithInstanceCallback&& callback) {
  if (!MemoryMonitor::Enable() || id < 0) return;
  fml::TaskRunner::RunNowOrPostTask(
      ReportRunner(),
      [this, id, at_ms, callback = std::move(callback)]() mutable {
        Start();
        auto [it, inserted] = instance_state_.try_emplace(id);
        if (inserted) {
          it->second.created_at_ms = at_ms;
        }
        WithInstance(id, std::move(callback));
      });
}

BtsVmState* GlobalMemoryMonitor::FindVM(runtime::js::VMInstance* vm) {
  for (auto& state : bts_vm_state_) {
    if (state.vm_ptr == vm) return &state;
  }
  return nullptr;
}

void GlobalMemoryMonitor::WithInstance(int32_t id, WithInstanceCallback&& cb) {
  if (!MemoryMonitor::Enable()) return;
  fml::TaskRunner::RunNowOrPostTask(
      ReportRunner(), [this, id, callback = std::move(cb)] {
        auto it = instance_state_.find(id);
        if (it == instance_state_.end() || it->second.destroyed) return;
        auto& state = it->second;
        const auto previous = state.bts_vm;
        callback(state);
        state.url = MemoryUrl(state.url);
        // This also detects the initial nullptr -> VM binding. Unrelated
        // WithInstance updates must not count the page again or reset VM idle
        // time. BindInstance owns those effects, not the producer callback.
        if (state.bts_vm != previous) BindInstance(state, previous);
        if (auto* vm = FindVM(state.bts_vm)) EmitSlotTrigger(*vm);
      });
}

void GlobalMemoryMonitor::BindInstance(InstanceState& state,
                                       runtime::js::VMInstance* previous) {
  const auto now = MemoryNowMs();
  if (auto* old = FindVM(previous); old && old->active_page_count > 0) {
    if (--old->active_page_count == 0) old->idle_since_ms = now;
  }
  if (auto* vm = FindVM(state.bts_vm)) {
    ++vm->active_page_count;
    ++vm->total_page_count;
    vm->idle_since_ms = 0;
    RefreshPageBts(state, *vm);
  }
}

void GlobalMemoryMonitor::WithBtsVM(runtime::js::VMInstance* vm,
                                    WithBtsVMCallback&& found,
                                    base::closure&& not_found) {
  if (!MemoryMonitor::Enable()) return;
  fml::TaskRunner::RunNowOrPostTask(
      ReportRunner(),
      [this, vm, cb = std::move(found), missing = std::move(not_found)] {
        for (size_t i = 0; i < bts_vm_state_.size(); ++i) {
          if (bts_vm_state_[i].vm_ptr == vm) {
            cb(bts_vm_state_[i], i);
            return;
          }
        }
        if (missing) missing();
      });
}

void GlobalMemoryMonitor::OnBtsVMCreate(
    const std::string& group_id,
    const std::shared_ptr<runtime::js::VMInstance>& vm) {
  if (!MemoryMonitor::Enable() || !vm) return;
  auto runner = fml::MessageLoop::GetCurrent().GetTaskRunner();
  // Never capture the incoming strong reference into a report-thread task:
  // releasing its last reference there would destroy an engine on the wrong
  // thread. The raw pointer is identity-only; lock the weak_ptr on JS only.
  fml::TaskRunner::RunNowOrPostTask(
      ReportRunner(),
      [this, group_id, weak = std::weak_ptr<runtime::js::VMInstance>(vm),
       ptr = vm.get(), type = vm->GetRuntimeType(), runner,
       created = MemoryNowMs()] {
        if (weak.expired()) return;
        Start();
        if (auto* existing = FindVM(ptr)) {
          if (SameOwner(existing->vm, weak)) return;
          // An engine without a destroy callback can leave an expired entry
          // until the next snapshot. Address reuse must not bind new pages to
          // that old entry or carry its age/counters into this new object.
          OnBtsVMDestroy(ptr);
        }
        BtsVmState state;
        state.type = type;
        state.runtime_manager_scope =
            runtime::RuntimeManager::IsVMSharedAcrossGroups(type);
        state.name = state.runtime_manager_scope
                         ? std::string("runtime_manager/") + RuntimeName(type)
                         : MemoryUrl(group_id);
        state.vm = weak;
        state.vm_ptr = ptr;
        state.js_thread_runner = runner;
        state.created_at_ms = created;
        state.idle_since_ms = created;
        bts_vm_state_.push_back(std::move(state));
      });
}

void GlobalMemoryMonitor::OnInstanceDestroyed(int32_t id, int64_t at_ms) {
  if (!MemoryMonitor::Enable()) return;
  fml::TaskRunner::RunNowOrPostTask(ReportRunner(), [this, id, at_ms] {
    auto it = instance_state_.find(id);
    if (it == instance_state_.end() || it->second.destroyed) return;
    auto& state = it->second;
    auto* vm = FindVM(state.bts_vm);
    if (vm && vm->active_page_count && --vm->active_page_count == 0) {
      vm->idle_since_ms = at_ms;
    }
    state.destroyed = true;
    state.exited_at_ms = at_ms;
    // A valid slot implies tracking, but standalone VMs are not in this table.
    // Residual events read shared VM slots, never the exited page's usage.
    // MemoryMonitor owns its planned-sample summary, so cleanup need not wait
    // for its destructor or for a telemetry flush.
    if (!vm || !fml::IsValidInstanceSlot(state.slot)) {
      instance_state_.erase(it);
    }
  });
}

void GlobalMemoryMonitor::OnBtsVMDestroy(runtime::js::VMInstance* vm) {
  WithBtsVM(vm, [this, vm](auto&, size_t index) {
    bts_vm_state_.erase(bts_vm_state_.begin() + index);
    for (auto it = instance_state_.begin(); it != instance_state_.end();) {
      if (it->second.bts_vm == vm && it->second.destroyed) {
        it = instance_state_.erase(it);
      } else {
        ++it;
      }
    }
  });
}

void GlobalMemoryMonitor::OnBtsVMMemoryUpdate(runtime::js::VMInstance* vm,
                                              BtsMemorySample sample) {
  WithBtsVM(vm, [sample = std::move(sample)](auto& state, size_t) {
    StoreSample(state, sample);
  });
}

void GlobalMemoryMonitor::OnBtsVMSlotAllocate(runtime::js::VMInstance* vm,
                                              int32_t slot) {
  WithBtsVM(vm, [slot](auto& state, size_t) {
    state.slot_tracking_enabled = true;
    if (slot == -1) {
      ++state.untracked_page_count;
      if (!state.overflowed) state.pending_overflow = true;
      state.overflowed = true;
    } else if (fml::IsValidInstanceSlot(slot)) {
      ++state.allocated_slots;
      if (state.allocated_slots % 16 == 0 ||
          state.allocated_slots == kPageSlotCapacity) {
        state.pending_slot_milestone = state.allocated_slots;
      }
    }
  });
}

void GlobalMemoryMonitor::EmitSlotTrigger(BtsVmState& vm) {
  if (!vm.pending_slot_milestone && !vm.pending_overflow) return;
  Trigger trigger;
  trigger.reasons = (vm.pending_slot_milestone ? kSlot : 0u) |
                    (vm.pending_overflow ? kOverflow : 0u);
  auto& slot = trigger.slot_vm.emplace();
  slot.name = vm.name;
  slot.created_at_ms = vm.created_at_ms;
  slot.heap_size = vm.heap_size;
  slot.total_page_count = vm.total_page_count;
  slot.untracked_page_count = vm.untracked_page_count;
  slot.allocated_slots = vm.allocated_slots;
  slot.milestone = vm.pending_slot_milestone;
  slot.heap_valid = vm.heap_valid;
  vm.pending_slot_milestone = 0;
  vm.pending_overflow = false;
  // Queue after binding commits; do not collect in the middle of a
  // WithInstance callback. Each queued trigger completes in one reporter task.
  ReportRunner()->PostTask([this, trigger = std::move(trigger)]() mutable {
    RequestReport(std::move(trigger));
  });
}

const InstanceState* GlobalMemoryMonitor::GetInstanceState(int32_t id) {
  assert(ReportRunner()->RunsTasksOnCurrentThread());
  auto it = instance_state_.find(id);
  return it == instance_state_.end() ? nullptr : &it->second;
}

size_t GlobalMemoryMonitor::GetInstanceBtsMemoryUsage(int32_t id) {
  assert(ReportRunner()->RunsTasksOnCurrentThread());
  const auto* page = GetInstanceState(id);
  auto* vm = page ? FindVM(page->bts_vm) : nullptr;
  if (!vm) return std::numeric_limits<size_t>::max();
  return vm->has_per_instance_data && fml::IsValidInstanceSlot(page->slot)
             ? vm->memory_slots[page->slot]
             : 0;
}

void GlobalMemoryMonitor::RefreshPageBts(InstanceState& page,
                                         const BtsVmState& vm) {
  auto& usage = page.usage;
  usage.total_bytes -= usage.bts_bytes;
  usage.bts_bytes = 0;
  usage.bts_shared = true;
  usage.available &= ~kBtsMemory;
  usage.eligible &= ~kBtsMemory;
  if (vm.slot_tracking_enabled && fml::IsValidInstanceSlot(page.slot)) {
    usage.eligible |= kBtsMemory;
    if (vm.has_per_instance_data) {
      const auto bytes = vm.memory_slots[page.slot];
      if (bytes <= static_cast<size_t>(std::numeric_limits<int64_t>::max())) {
        usage.bts_bytes = static_cast<int64_t>(bytes);
        usage.available |= kBtsMemory;  // Includes a genuinely measured zero.
      } else {
        usage.arithmetic_valid = false;
      }
    }
  }
  if (!AddMemoryBytes(usage.bts_bytes, usage.total_bytes)) {
    usage.arithmetic_valid = false;
    // This component was not added, so it must not be subtracted next time.
    usage.bts_bytes = 0;
    usage.available &= ~kBtsMemory;
  }
  page.memory_usage = static_cast<size_t>(usage.total_bytes);
}

PageMemoryUsage GlobalMemoryMonitor::UpdatePageMemory(int32_t id,
                                                      PageMemoryUsage usage) {
  assert(ReportRunner()->RunsTasksOnCurrentThread());
  auto it = instance_state_.find(id);
  if (it == instance_state_.end()) return usage;
  auto& page = it->second;
  if (page.destroyed) return page.usage;
  page.usage = usage;
  if (auto* vm = FindVM(page.bts_vm)) RefreshPageBts(page, *vm);
  page.memory_usage = static_cast<size_t>(page.usage.total_bytes);
  return page.usage;
}

void GlobalMemoryMonitor::PollProcessMemory() {
  const auto now = MemoryNowMs();
  Trigger trigger;
  const auto interval = MemoryMonitor::GetSettings().global.time_interval_sec;
  if (interval && next_timer_at_ms_ && now >= next_timer_at_ms_) {
    trigger.reasons |= kTimer;
    // Skip missed deadlines; do not replay historical samples after suspension.
    next_timer_at_ms_ +=
        ((now - next_timer_at_ms_) / (interval * int64_t{1000}) + 1) *
        (interval * int64_t{1000});
  }
  const auto pss = ReadProcessPssBytes();
  const int64_t step =
      MemoryMonitor::GetSettings().global.pss_threshold_mb * kMiB;
  if (pss > 0 && step > 0 && pss / step > pss_high_water_bucket_) {
    trigger.reasons |= kPss;
    trigger.pss_before = pss_high_water_bucket_ * step;
    pss_high_water_bucket_ = pss / step;
    trigger.pss_after = pss_high_water_bucket_ * step;
  }
  if (trigger.reasons) {
    RequestReport(std::move(trigger));
  }
}

void GlobalMemoryMonitor::RequestReport(Trigger trigger) {
  assert(ReportRunner()->RunsTasksOnCurrentThread());
  Start();
  auto snapshot = CollectSnapshot(std::move(trigger));
  const auto pss = ReadProcessPssBytes();
  auto event = BuildEvent(snapshot, MemoryNowMs(), pss);
  previous_snapshot_at_ms_ = snapshot.started_at_ms;
  // Freeze the event before enqueue. A later flush must not read mutable state.
  report::EventTracker::OnGlobalEvent(
      [event = std::move(event)](auto& target) mutable {
        target = std::move(event);
      });
}

GlobalMemoryMonitor::Snapshot GlobalMemoryMonitor::CollectSnapshot(
    Trigger trigger) {
  assert(ReportRunner()->RunsTasksOnCurrentThread());
  // V8/JSC may have no destruction callback. Expired weak refs can be removed
  // without locking/destructing the engine on this thread.
  base::InlineVector<runtime::js::VMInstance*, 16> expired;
  for (const auto& vm : bts_vm_state_) {
    if (vm.vm.expired()) expired.push_back(vm.vm_ptr);
  }
  for (auto* vm : expired) OnBtsVMDestroy(vm);
  for (auto& item : instance_state_) {
    if (!item.second.destroyed) {
      if (auto* vm = FindVM(item.second.bts_vm))
        RefreshPageBts(item.second, *vm);
    }
  }
  Snapshot snapshot;
  snapshot.id = ++snapshot_seq_;
  snapshot.started_at_ms = MemoryNowMs();
  snapshot.trigger = std::move(trigger);
  for (auto& vm : bts_vm_state_) {
    if (vm.type != runtime::js::JSRuntimeType::v8 &&
        vm.type != runtime::js::JSRuntimeType::jsvm)
      continue;
    std::optional<size_t> heap_size;
    int64_t measured_at_ms = 0;
    if (vm.js_thread_runner) {
      vm.js_thread_runner->PostSyncTask(
          [weak = vm.vm, &heap_size, &measured_at_ms] {
            if (auto strong = weak.lock()) {
              heap_size = strong->GetHeapSize();
              measured_at_ms = MemoryNowMs();
            }
          });
    }
    if (heap_size) {
      vm.heap_size = *heap_size;
      vm.heap_valid = true;
      vm.measured_at_ms = measured_at_ms;
    } else {
      // Preserve failed candidates and any old cache rather than silently
      // shrinking the sampling population or treating a failure as zero.
      snapshot.partial = true;
    }
  }
  return snapshot;
}

report::MoveOnlyEvent GlobalMemoryMonitor::BuildEvent(const Snapshot& snapshot,
                                                      int64_t now,
                                                      int64_t pss) {
  // GLOBAL QUERY GUIDE
  //
  // Event: lynxsdk_memory_usage, schema_version=7. See AddSamplingProps for
  // units and process/source sampling. Deduplicate by
  // (process_session_id, snapshot_seq) before aggregating.
  //
  // One row contains BOTH a census of this reporter-owned snapshot and random
  // entity details. Census totals/counts require no entity weight. The active
  // instance is uniform; its URL is selected through that instance; residual
  // pages and VMs use probability proportional to bytes (PPS). Their weights
  // correct these different draws, not process selection, source availability,
  // cached-value error, event loss or the frequency of triggers.
  //
  // Default resident-memory cohort: (trigger_reason_mask & 1)!=0 (timer).
  // Hold platform, SDK/experiment, sampling configuration and, where relevant,
  // monitor-age/RAM buckets constant. This reduces pressure-trigger bias but
  // still describes OBSERVED snapshot residency, not all page loads or an
  // equally sampled population of process lifetimes. Suspended/missing timer
  // points are not backfilled. Use separate PSS/pressure/slot cohorts to
  // investigate those conditions; do not mix all reasons into a normal-use
  // percentile and assume entity weights remove the trigger bias.
  //
  // Ordinary AVG/PCT of a census total describes that value across retained
  // snapshot rows. Ordinary AVG/PCT of sampled entity values generally does
  // NOT describe an entity-equal distribution. For the latter, use the
  // supplied inverse selection weights, as documented beside each block.
  // SUM of corrected bytes over snapshots is a byte-snapshot contribution,
  // not distinct allocated bytes, current memory, or measured byte-time.
  //
  // PLATFORM / SETTINGS PLAYBOOK (typical deployments, not capability flags):
  //
  // A. Process total/ratio: use the FULL or GLOBAL-ONLY Settings profile
  //    (Element/UI rates=1 in a randomly selected process; page slots
  //    optional). Require lynx_full_memory_snapshot_valid=1 and
  //    process_pss_bytes>0. Keep process_sample_rate fixed, apply the timer
  //    or explicit PSS cohort, and group by platform and RAM bucket.
  //    This gate checks ALL active required sources and
  //    registered shared heaps, not just the sampled entity. MTS must be
  //    supported and UI producers must actually have delivered their data. No
  //    "source enabled" setting can compensate for an unsupported engine or a
  //    missing UI implementation.
  //
  //    iOS (mostly JSC, some QuickJS):
  //      A required JSC heap prevents full accounting. Even perfect source
  //      sampling cannot produce a trustworthy all-Lynx percentage for those
  //      snapshots. The full-valid population omits JSC-bearing snapshots,
  //      so label its PCT as the supported full-accounting cohort, not all iOS.
  //      The ratio field can still be present on an invalid row; such a number
  //      is a partial observed-source diagnostic, NOT the full percentage and
  //      not a mathematically guaranteed physical-memory lower bound.
  //    Android (mostly QuickJS, some V8):
  //      Both shared heaps are measurable. Full process accounting can remain
  //      valid with slots OFF or V8 pages lacking per-page BTS attribution.
  //      Do not also require active_page_full_memory_count==active_page_count,
  //      which would unnecessarily eliminate these valid global snapshots.
  //    PC (mostly QuickJS, some V8):
  //      Same engine/slot rules as Android; group by actual OS as usual.
  //    Harmony (mostly JSVM, some QuickJS):
  //      JSVM whole heaps support the global ratio even though dominant JSVM
  //      pages cannot support full/core page ranking. Do not require QuickJS
  //      or slot-enabled rows for the process metric.
  //
  // B. Shared-VM heap query: process sampling alone is enough to request these
  //    heaps; Element/UI and QuickJS slot settings are NOT required. Use
  //    vm_heap_snapshot_complete=1, then sampled_vm_present=1 for VM details
  //    with the original PPS weight. For iOS this is only the measurable
  //    QuickJS shared-VM subset; do not require global full validity just to
  //    study that subset, since JSC makes it fail. For Android/PC split
  //    sampled_vm_runtime_type="quickjs"/"v8";
  //    for Harmony split "jsvm"/"quickjs". Retain original PPS probabilities
  //    after type filtering. See the VM block for the exact weighted formulas.
  //
  // C. Page/URL BYTES and residuals: need the actual page attribution path,
  //    unlike A/B. Shared QuickJS requires valid tracked slots; JSC and shared
  //    V8/JSVM cannot become full-valid page sources by raising sample rates.
  //    FULL profile with BOTH GC/RC slot rates=1 gives the simplest supported
  //    page/URL cohort. For residuals only, Element/UI are not required; keep
  //    both slot rates=1 and the tracked-scope quality gates below. Missing
  //    mode-specific inclusion information prevents a generic correction of
  //    mixed lower-rate GC/RC cohorts. A core-only profile cannot produce a
  //    full global total or these full page/URL byte fields by ignoring UI.
  //
  // D. Page residency/counts: available independently of heap/slot/UI sources
  //    once process monitoring registered the instances. Use timer snapshots
  //    and the appropriate presence/selection weights, NOT memory-validity
  //    filters. This is the cross-engine path for studying iOS JSC and Harmony
  //    JSVM long-resident URLs without pretending their page bytes are known.
  //
  // Never classify a whole process by sampled_vm_runtime_type or
  // sampled_active_page_vm_runtime_type: these describe ONE random selection.
  // Applying such a filter to lynx_accounted_process_pss_ratio biases its
  // distribution toward processes likely to select that engine. There is no
  // all-process engine-composition field in this schema. Likewise, a platform
  // being "mostly QuickJS" is not proof all its sources are observable.
  assert(ReportRunner()->RunsTasksOnCurrentThread());
  const auto build_started_at = fml::TimePoint::Now();
  report::MoveOnlyEvent event;
  event.SetName("lynxsdk_memory_usage");
  event.SetInstanceId(report::kUnknownInstanceId);
  MemoryMonitor::AddSamplingProps(event);
  event.SetProps("snapshot_seq", snapshot.id);
  // These are monotonic monitor times, not process launch time or UTC.
  // Bucket monitor_uptime_ms directly when comparing monitor-age cohorts.
  // collection_duration_ms starts at the collector's snapshot marker, after
  // expired-entry cleanup/page-cache refresh, and includes synchronous
  // heap-query waiting and process-memory collection. It excludes trigger
  // queue delay and BuildEvent.
  event.SetProps("monitor_uptime_ms", now - started_at_ms_);
  event.SetProps("collection_duration_ms", now - snapshot.started_at_ms);
  // Reason bits: timer=1, PSS crossing=2, pressure=4, slot milestone=8,
  // first slot overflow=16. Several bits can be set on the same event.
  // Filter bits when asking "includes timer/PSS", not equality to one reason:
  // a timer event may also cross a PSS milestone.
  //
  // PSS crossing example for M=1GiB (1073741824 bytes):
  //   (trigger_reason_mask & 2)!=0
  //   trigger_pss_previous_milestone_bytes < M
  //   trigger_pss_milestone_bytes >= M
  // Use 2147483648 for 2GiB. One observation can span several milestones.
  // Boundaries are observed high-water buckets, not every allocation peak.
  // Bucket boundaries are emitted only for PSS crossings.
  // process_pss_bytes is read after heap collection and is the ratio
  // denominator, not a trigger-time read: a crossing event does not measure
  // memory at exactly M bytes.
  event.SetProps("trigger_reason_mask", snapshot.trigger.reasons);
  if (snapshot.trigger.reasons & kPss) {
    event.SetProps("trigger_pss_previous_milestone_bytes",
                   snapshot.trigger.pss_before);
    event.SetProps("trigger_pss_milestone_bytes", snapshot.trigger.pss_after);
  }
  event.SetProps("process_pss_bytes", pss);

  double accounted = 0, shared_heap = 0, retained = 0;
  double old_heap_10m = 0, old_heap_30m = 0;
  size_t active_count = 0, full_count = 0, idle_count = 0, heap_supported = 0;
  size_t heap_collected = 0, tracked_exited = 0, saturated_count = 0;
  uint64_t untracked_count = 0;
  size_t alive_10m = 0, alive_20m = 0, alive_30m = 0;
  bool full = !snapshot.partial, heap_complete = !snapshot.partial;
  bool retained_complete = !snapshot.partial;
  std::unordered_map<runtime::js::VMInstance*, const BtsVmState*> vms;
  const BtsVmState* sampled_vm = nullptr;
  const InstanceState* sampled_page = nullptr;
  const InstanceState* sampled_residual = nullptr;
  const BtsVmState* residual_vm = nullptr;
  double residual_bytes = 0;
  MemoryReservoir page_picker, vm_picker, residual_picker;
  for (const auto& vm : bts_vm_state_) {
    vms.emplace(vm.vm_ptr, &vm);
    idle_count += vm.active_page_count == 0;
    saturated_count += vm.allocated_slots >= kPageSlotCapacity;
    untracked_count += vm.untracked_page_count;
    const bool supported = vm.type != runtime::js::JSRuntimeType::jsc;
    heap_supported += supported;
    if (supported && vm.heap_valid) {
      ++heap_collected;
      shared_heap += vm.heap_size;
      accounted += vm.heap_size;
      const auto age = snapshot.started_at_ms - vm.created_at_ms;
      if (age >= 600000) old_heap_10m += vm.heap_size;
      if (age >= 1800000) old_heap_30m += vm.heap_size;
      if (vm_picker.Consider(static_cast<double>(vm.heap_size), random_))
        sampled_vm = &vm;
    } else {
      full = false;
      if (supported) heap_complete = false;
    }
    if (vm.slot_tracking_enabled && !vm.has_per_instance_data)
      retained_complete = false;
  }
  for (const auto& item : instance_state_) {
    const auto& page = item.second;
    const auto vm_it = vms.find(page.bts_vm);
    const auto* vm = vm_it == vms.end() ? nullptr : vm_it->second;
    if (page.destroyed) {
      if (vm && vm->has_per_instance_data &&
          fml::IsValidInstanceSlot(page.slot)) {
        ++tracked_exited;
        const double bytes = vm->memory_slots[page.slot];
        retained += bytes;
        if (residual_picker.Consider(bytes, random_)) {
          sampled_residual = &page;
          residual_vm = vm;
          residual_bytes = bytes;
        }
      }
      continue;
    }
    ++active_count;
    const auto& usage = page.usage;
    full_count += usage.Valid(kFullMemory);
    // PROCESS ACCOUNTING:
    //   A = SUM(active page total - its already-included shared BTS bytes)
    //       + SUM(unique supported shared-VM heaps with valid cached values).
    // Slots, including exited-page residuals, already belong to those heaps.
    // Neither URL group totals nor retained bytes may be added to A again.
    // Subtract the component in this same page total, not a slot from a
    // different observation. Example: page 100MiB includes a 40MiB slot;
    // its shared VM is 200MiB -> A=100-40+200=260MiB, not 300MiB.
    accounted += usage.total_bytes - (usage.bts_shared ? usage.bts_bytes : 0);
    const uint32_t page_required =
        usage.required & (usage.bts_shared ? ~kBtsMemory : ~0u);
    full &= usage.arithmetic_valid &&
            (usage.available & page_required) == page_required &&
            (usage.eligible & page_required) == page_required;
    if (usage.bts_shared && !vm) full = false;
    const auto age = snapshot.started_at_ms - page.created_at_ms;
    alive_10m += age >= 600000;
    alive_20m += age >= 1200000;
    alive_30m += age >= 1800000;
    if (page_picker.Consider(1, random_)) {
      sampled_page = &page;
    }
  }
  // CENSUS QUERIES: these are totals over the whole observed registry, not
  // scaled sampled_* values. For process accounted memory, require
  // lynx_full_memory_snapshot_valid=1, then AVG/PCT95/PCT99 of
  // lynx_accounted_memory_bytes (and divide by 1048576 for MiB).
  // When invalid, the number is a diagnostic aggregate and must not be
  // presented as complete memory or treated as a zero-sized process.
  //
  // active_page_full_memory_count / active_page_count describes page-byte
  // coverage. Global full alone is insufficient: shared V8/JSVM heaps can
  // make process accounting complete while page BTS attribution is unavailable.
  //
  // Long-resident count ratios need no memory-validity filter:
  //   SUM(active_page_alive_10m_count)/SUM(active_page_count)
  // estimates the fraction aged >=10min among observed instance-snapshots.
  // 20m/30m are analogous; cohorts overlap and must not be added together.
  // This is not a page-load survival probability. AVG(alive/count) instead
  // weights snapshots equally, and requires nonzero active_page_count.
  //
  // Shared-heap queries use vm_heap_snapshot_complete=1:
  //   PCT95(shared_vm_heap_bytes_sum) -> observed shared-heap pressure;
  //   SUM(shared_vm_alive_30m_heap_bytes_sum)/SUM(shared_vm_heap_bytes_sum)
  //       -> byte-weighted share owned by VMs already >=30min old.
  // Require a positive denominator. The age sums include active AND idle VMs,
  // not growth or lifetime area. Idle counts mean no bound active pages, not
  // no JavaScript work. Heap supported/collected counts include valid zero
  // heaps; JSC is counted in shared_vm_count but cannot contribute heap bytes.
  //
  // For process-level residual pressure, use retained_snapshot_complete=1 and
  // AVG/PCT95(shared_vm_tracked_exited_page_retained_bytes). These census
  // values need no PPS weight. Retained count includes observed exited slots
  // with zero bytes; PPS details below require positive bytes. Saturated
  // counts describe currently registered VMs at slot capacity.
  // shared_vm_untracked_page_count_sum accumulates allocation failures for
  // those VMs, not all unsupported/unsampled pages and not the number of
  // currently active untracked pages.
  event.SetProps("lynx_accounted_memory_bytes", accounted);
  event.SetProps("active_page_count", static_cast<uint64_t>(active_count));
  event.SetProps("active_page_full_memory_count",
                 static_cast<uint64_t>(full_count));
  event.SetProps("active_page_alive_10m_count",
                 static_cast<uint64_t>(alive_10m));
  event.SetProps("active_page_alive_20m_count",
                 static_cast<uint64_t>(alive_20m));
  event.SetProps("active_page_alive_30m_count",
                 static_cast<uint64_t>(alive_30m));
  event.SetProps("shared_vm_count",
                 static_cast<uint64_t>(bts_vm_state_.size()));
  event.SetProps("shared_vm_idle_count", static_cast<uint64_t>(idle_count));
  event.SetProps("shared_vm_heap_supported_count",
                 static_cast<uint64_t>(heap_supported));
  event.SetProps("shared_vm_heap_collected_count",
                 static_cast<uint64_t>(heap_collected));
  event.SetProps("shared_vm_heap_bytes_sum", shared_heap);
  event.SetProps("shared_vm_alive_10m_heap_bytes_sum", old_heap_10m);
  event.SetProps("shared_vm_alive_30m_heap_bytes_sum", old_heap_30m);
  event.SetProps("shared_vm_tracked_exited_page_count",
                 static_cast<uint64_t>(tracked_exited));
  event.SetProps("shared_vm_tracked_exited_page_retained_bytes", retained);
  event.SetProps("shared_vm_slot_saturated_count",
                 static_cast<uint64_t>(saturated_count));
  event.SetProps("shared_vm_untracked_page_count_sum", untracked_count);
  // Choose the quality gate for the QUESTION, not one blanket flag:
  //
  // - lynx_full_memory_snapshot_valid: all required active-page sources and
  //   unique shared heaps satisfy this accounting contract. Shared-page slot
  //   attribution is not required for the global total. Unsupported JSC heap
  //   makes this false.
  // - vm_heap_snapshot_complete: supported shared-heap candidates all have
  //   usable values and synchronous queries did not fail. JSC does not itself
  //   make this false; compare shared_vm_heap_supported_count with
  //   shared_vm_count for engine coverage. This flag does not certify current
  //   page UI or slots.
  // - retained_snapshot_complete: tracking-enabled VM slot caches are
  //   available and collection did not fail. It says nothing about disabled
  //   tracking, unsupported engines or overflow pages, even when true.
  //
  // Consequently, do not require global full validity for a heap-only
  // or retained-only query just because an unrelated page UI source is
  // missing. Conversely, a full process total does not imply URL/page full.
  // All these flags certify the implemented cached scope, never freshness,
  // all-widget physical coverage, or GC-confirmed live objects.
  //
  // PLATFORM CONFIDENCE / COVERAGE, measured BEFORE success-only filters:
  //   E = all received distinct timer snapshots in the platform/config cohort.
  //   full coverage = COUNT(E where
  //   lynx_full_memory_snapshot_valid=1)/COUNT(E). denominator coverage =
  //   COUNT(E where process_pss_bytes>0)/COUNT(E). ratio coverage = COUNT(E
  //   satisfying BOTH)/COUNT(E).
  // These denominators must include invalid iOS/JSC and source-disabled
  // rows if the question is coverage of the full received platform population.
  // Also break out source configuration. To assess supported-engine VM
  // coverage, compare the supported/collected counts with
  // shared_vm_count BEFORE requiring vm_heap_snapshot_complete. Guard zero
  // denominators: no registered VM is not a successful engine measurement.
  //
  // A trustworthy COMPLETE-SCOPE estimate needs its quality gate plus known
  // source/probability strata and sufficient independent processes. Neither
  // validity nor 100% coverage is a statistical confidence interval or proof
  // of resident-byte ownership.
  // JSC missing heap is structurally unobservable; rate changes/weights cannot
  // recover it. Untracked QuickJS and shared V8/JSVM are unobservable per-page,
  // but that does NOT make their available whole VM heaps invalid.
  // When all registered VMs are JSC, vm_heap_snapshot_complete can be true
  // with shared_vm_heap_supported_count=0 and no sampled_vm. Zero supported
  // heap/residual bytes then means "none observable", NOT "no VM memory/leak".
  event.SetProps("vm_heap_snapshot_complete", heap_complete);
  event.SetProps("retained_snapshot_complete", retained_complete);
  event.SetProps("lynx_full_memory_snapshot_valid", full);
  // BYTE-TIME uses consecutive ALL-TRIGGER collection start markers:
  //   start=max(previous_snapshot_start, current_snapshot_start-300000ms).
  // The first snapshot has no interval. Query failure invalidates this
  // interval; a later success does not fill the skipped history. There is no
  // special background subtraction, only the 5min cap and entity clipping.
  // byte_time_valid alone is not enough: heap/residual blocks also require
  // their own completeness flags before emitting their area value.
  //
  // Area queries must retain all trigger types under this policy. Keeping
  // only timer rows discards intervals ending at PSS/pressure/slot events;
  // it does not recover a continuous 5min timer-based integral. Failed,
  // dropped and capped intervals likewise remain unobserved. Age/idle filters
  // classify the endpoint: clipping below does not locate when an age/idle
  // threshold became true inside the interval.
  const bool byte_time_valid =
      previous_snapshot_at_ms_ > 0 &&
      snapshot.started_at_ms >= previous_snapshot_at_ms_ && !snapshot.partial;
  const auto interval_start = std::max(
      previous_snapshot_at_ms_, snapshot.started_at_ms - int64_t{300000});
  event.SetProps("byte_time_valid", byte_time_valid);
  // ESTIMATED PROCESS SHARE QUERY:
  //   schema_version=7; lynx_full_memory_snapshot_valid=1
  //   process_pss_bytes>0; [timer cohort OR the PSS crossing filter above]
  //   GROUP BY platform and physical-RAM bucket
  //   PCT50/PCT90/PCT99(lynx_accounted_process_pss_ratio)
  // Multiply by 100 for percent. For the two subset ratios use the matching
  // vm_heap_snapshot_complete / retained_snapshot_complete gate instead.
  //
  // Process memory is a single PSS estimate for both triggers and ratios.
  // A failed/nonpositive read emits no ratio; no metric-type filter is needed.
  //
  // Let D=process_pss_bytes. P99(A/D) != P99(A)/P99(D), and
  // AVG(A/D) != AVG(A)/AVG(D). Do not apply any entity-selection weight:
  // numerators here are census totals. shared-heap and retained ratios overlap
  // the accounted ratio (and each other); they must not be added together.
  // Cached/allocator accounting differs from OS accounting and is not
  // synchronized with it. Ratios may exceed 1; investigate source scope,
  // timing and duplication rather than clipping them or calling them physical
  // ownership percentages. Even a full-valid numerator is only an estimate.
  if (pss > 0) {
    event.SetProps("lynx_accounted_process_pss_ratio", accounted / pss);
    event.SetProps("lynx_shared_vm_process_pss_ratio", shared_heap / pss);
    event.SetProps("lynx_tracked_page_retained_process_pss_ratio",
                   retained / pss);
  }
  event.SetProps("sampled_active_page_present", sampled_page != nullptr);
  if (sampled_page) {
    // RESIDENT PAGE URL QUERY: one instance drawn uniformly from N active
    // instances, including those with incomplete memory. p=1/N, w=N.
    //
    // In the timer cohort, require sampled_active_page_present=1.
    // GROUP BY sampled_active_url (shared by the page and URL blocks):
    //   SUM(sampled_active_page_alive_10m_weighted) /
    //     SUM(active_page_count)
    // gives the >=10min fraction among observed instance-snapshots. Do not
    // filter age>=10min before computing this fraction, and do not require
    // full memory for a lifecycle-only question. 20m/30m use their own fields.
    //
    // For "large, long-resident single instances", additionally filter
    // sampled_active_page_age_ms>=600000 and
    // sampled_active_page_full_memory_valid=1, then rank URLs by
    //   SUM(sampled_active_page_memory_bytes_weighted) /
    //     SUM(active_page_count).
    // Numerator and denominator must have IDENTICAL filters. Weighted bytes
    // are emitted even for partial diagnostics; presence does not imply full.
    //
    // w corrects different N across snapshots. Ordinary PCT(memory_bytes)
    // lacks that correction. A weighted CDF is
    //   F(x)=SUM(w*I(memory_bytes<=x))/SUM(w);
    // the weighted P95 is the first x with F(x)>=0.95. Use this only if the
    // query engine supports weighted aggregation; otherwise report the
    // weighted mean or threshold fraction, not an "unbiased" ordinary PCT.
    // Weights do not remove the preference for long-lived pages to be present
    // at many snapshots. Use the page event's unique summary for load cohorts.
    //
    // Platform recipe: on iOS JSC / Harmony JSVM (also Android/PC V8), count
    // or age queries may select sampled_active_page_vm_runtime_type and use
    // the original N weight. Do not require full memory, slot sampling or UI.
    // For byte queries on the supported QuickJS cohort, add
    // sampled_active_page_vm_runtime_type="quickjs" AND
    // sampled_active_page_full_memory_valid=1; split
    // sampled_active_page_vm_shared if comparing ownership scopes.
    // There is no global sampled-page core value/validity pair: if full fails
    // because UI is missing, use the page event's core query, not this partial
    // total. A source mask alone cannot reconstruct missing component values.
    const auto& page = *sampled_page;
    const auto age = snapshot.started_at_ms - page.created_at_ms;
    const double weight = active_count;
    event.SetProps("sampled_active_page_age_ms", age);
    event.SetProps("sampled_active_page_memory_bytes", page.usage.total_bytes);
    event.SetProps("sampled_active_page_full_memory_valid",
                   page.usage.Valid(kFullMemory));
    event.SetProps("sampled_active_page_vm_shared", page.usage.bts_shared);
    event.SetProps(
        "sampled_active_page_vm_runtime_type",
        page.bts_known ? RuntimeName(page.bts_runtime_type) : "unknown");
    event.SetProps("sampled_active_page_memory_bytes_weighted",
                   page.usage.total_bytes * weight);
    event.SetProps("sampled_active_page_alive_10m_weighted",
                   age >= 600000 ? weight : 0);
    event.SetProps("sampled_active_page_alive_20m_weighted",
                   age >= 1200000 ? weight : 0);
    event.SetProps("sampled_active_page_alive_30m_weighted",
                   age >= 1800000 ? weight : 0);
    // CONCURRENT URL QUERY: the sampled instance identifies a URL, then all
    // n active instances of that URL are summed to S (across VM/JS threads).
    // Its draw probability is q=n/N, not 1/number_of_distinct_URLs, hence
    // url_weight=N/n. It can have many small instances and still be costly.
    //
    // Base filters: timer cohort, sampled_active_page_present=1,
    // sampled_active_url_full_memory_valid=1.
    // GROUP BY sampled_active_url:
    //   B=SUM(sampled_active_url_memory_bytes_weighted)
    //   W=SUM(sampled_active_url_selection_weight)
    //   C=SUM(active_page_count), since n*(N/n)=N
    //   B/W -> mean concurrent bytes when this URL is present and full-valid
    //   C/W -> mean concurrent instance count in that SAME cohort
    // Rank by B/W for concurrent cost; inspect instance_count and
    // max_instance_bytes to distinguish many small instances from one large
    // instance. B alone estimates accumulated byte-snapshot contribution and
    // depends on the observation window/exposure. B/W is a ratio estimator,
    // not an exactly unbiased finite-sample mean.
    //
    // Full requires all n members, not just the sampled instance. Keep missing
    // members in n; the full-valid member count must never replace n in q.
    // S contains page-attributed BTS only, not shared common bytes or exited
    // residuals. Do not add S to global accounted memory.
    //
    // Direct AVG/PCT(S) is biased toward URLs with larger n/N. For a threshold
    // x use SUM(url_weight*I(S>=x))/W, or the weighted CDF above for
    // percentiles. A filter such as n>=2 changes the question to multi-instance
    // snapshots; keep the ORIGINAL N/n weight after applying that filter.
    //
    // Engine-mixed URLs: sampled_active_page_vm_runtime_type is the type of
    // ONE member. The URL aggregate includes ALL n members, potentially mixing
    // QuickJS with JSC/V8/JSVM. Do not filter the selected member to QuickJS
    // and then call S a QuickJS-only URL total: the selection probability
    // becomes n_quickjs/N while S and its stored weight still use all members.
    // Use sampled_active_url_full_memory_valid=1 without that member filter.
    // A URL containing any unsupported member will not qualify; on iOS and
    // Harmony that can leave a small, selective cohort. This event has no
    // engine-specific URL member totals to recover the omitted byte ranking.
    //
    // Count-only alternative, on EVERY platform: remove the full-memory
    // condition, then C/W (as defined above) estimates concurrent instance
    // count when the URL is present. All observed members stay in n regardless
    // of whether they have memory data. This answers a different question
    // than B/W and must not be labelled a full-memory ranking.
    size_t n = 0, collected = 0;
    double sum = 0, maximum = 0;
    for (const auto& candidate : instance_state_) {
      if (!candidate.second.destroyed && candidate.second.url == page.url) {
        ++n;
        const auto& usage = candidate.second.usage;
        sum += usage.total_bytes;
        maximum = std::max(maximum, static_cast<double>(usage.total_bytes));
        collected += usage.Valid(kFullMemory);
      }
    }
    const double url_weight = weight / n;
    event.SetProps("sampled_active_url", page.url);
    event.SetProps("sampled_active_url_instance_count",
                   static_cast<uint64_t>(n));
    event.SetProps("sampled_active_url_memory_bytes", sum);
    event.SetProps("sampled_active_url_max_instance_bytes", maximum);
    event.SetProps("sampled_active_url_full_memory_valid", collected == n);
    event.SetProps("sampled_active_url_selection_weight", url_weight);
    event.SetProps("sampled_active_url_memory_bytes_weighted",
                   sum * url_weight);
    // Example: N=30, n=20, S=200MiB -> q=2/3, weight=1.5, weighted=300MiB.
    // Expected attributed contribution is (2/3)*300=200MiB. The emitted 300
    // is an estimator term, not the URL's measured concurrent size.
  }

  event.SetProps("sampled_leak_page_present", sampled_residual != nullptr);
  if (sampled_residual) {
    // EXITED-PAGE RESIDUAL QUERY ("leak" is the historical field prefix):
    // L_i is this exited page's CURRENT cached shared-VM slot, not its memory
    // at exit. Candidates need an observable valid slot and L_i>0; let
    // T=SUM(L_i), p_i=L_i/T. L_i/p_i=T, already emitted as
    // shared_vm_tracked_exited_page_retained_bytes. VM destruction
    // removes the slot from future populations; zero slots have no draw
    // probability, but still enter shared_vm_tracked_exited_page_count.
    //
    // Baseline snapshots: schema_version=7, timer cohort,
    // retained_snapshot_complete=1. For numerator rows ALSO require
    // sampled_leak_page_present=1.
    // Optionally require sampled_leak_page_exit_elapsed_ms>=600000 to study
    // residuals still observed >=10min after exit. GROUP BY
    // sampled_leak_page_url and rank
    // SUM(shared_vm_tracked_exited_page_retained_bytes) ON THESE SELECTED ROWS.
    // This estimates that URL's byte-snapshot contribution in the observable
    // tracked population. Keep the original p after exit-age/VM/saturation
    // filtering.
    //
    // For a share of ALL observed residual bytes, divide the selected URL's
    // corrected sum by SUM(shared_vm_tracked_exited_page_retained_bytes)
    // over ALL baseline snapshots, not just rows where that URL was drawn.
    // For average URL-attributed process-memory share, divide
    // SUM(lynx_tracked_page_retained_process_pss_ratio) on selected rows by K,
    // where K counts ALL baseline snapshots with process_pss_bytes>0 and the
    // SAME platform, including those with no positive residual. Non-selection
    // contributes zero estimator terms; this must not be confused with
    // imputing missing source measurements.
    //
    // Ordinary AVG/PCT(sampled_leak_page_retained_bytes) is size-biased. If a
    // positive-slot entity mean is needed, use
    //   SUM(shared_vm_tracked_exited_page_retained_bytes) on selected rows /
    //     SUM(1 / sampled_leak_page_selection_probability),
    // or inverse-probability weighted CDFs for quantiles. Neither reconstructs
    // zero-byte or untracked pages. These residuals can still be reclaimable;
    // no GC, reachability, confirmed leak or all-engine coverage is asserted.
    // retained_snapshot_complete does not repair disabled/overflow tracking.
    //
    // PLATFORM RESIDUAL RECIPE:
    // - Fix source_bts_slot_gc_sample_rate=1 and
    //   source_bts_slot_rc_sample_rate=1
    //   in a selected production process. Element/UI enablement is irrelevant.
    // - Use retained_snapshot_complete=1 for the baseline, and
    //   sampled_leak_page_present=1 for PPS detail/numerator rows.
    //   Add process_pss_bytes>0 and platform strata for ratios.
    // - This implementation's tracked residuals are shared QuickJS slots,
    //   including its GC and RC modes. That covers the main engine family on
    //   Android/PC, but only the QuickJS subset on iOS/Harmony. JSC/V8/JSVM
    //   exited-page attribution and private-VM post-exit memory are not here.
    //   No residual row is NOT evidence those other engines released memory.
    // - Do not filter sampled_vm_runtime_type="quickjs": the VM PPS draw is
    //   independent and may have selected V8/JSVM while this slot is valid.
    //
    // Capacity exclusion is not random sampling. Valid old slots remain usable
    // after saturation, while later overflow pages have no individual bytes.
    // Show sampled_leak_page_vm_tracking_saturated and
    // sampled_leak_page_vm_untracked_page_count alongside URL contribution;
    // retaining only unsaturated rows defines a narrower pre-overflow cohort,
    // not a correction for untracked pages. Even both slot rates=1 does not
    // guarantee all current/historical pages have observable residuals.
    event.SetProps("sampled_leak_page_url", sampled_residual->url);
    event.SetProps("sampled_leak_page_retained_bytes", residual_bytes);
    event.SetProps("sampled_leak_page_exit_elapsed_ms",
                   snapshot.started_at_ms - sampled_residual->exited_at_ms);
    event.SetProps("sampled_leak_page_vm_name", residual_vm->name);
    // Context for this slot's owner, not the independent VM draw. Saturation
    // means future pages may be untracked, not that old slots lose identity.
    event.SetProps("sampled_leak_page_vm_tracking_saturated",
                   residual_vm->allocated_slots >= kPageSlotCapacity);
    event.SetProps("sampled_leak_page_vm_untracked_page_count",
                   residual_vm->untracked_page_count);
    event.SetProps("sampled_leak_page_selection_probability",
                   residual_bytes / retained);
    if (byte_time_valid && retained_complete) {
      // Optional area query: all trigger types, retained_snapshot_complete=1,
      // byte_time_valid=1, and this field present. Sum by residual URL:
      //   (L_i * dt_i) / p_i = T * dt_i,
      //   dt_i=current_start-max(interval_start,page_exit), clamped at zero.
      // This is an endpoint rectangle estimate in byte*ms. Divide by
      // 1048576*1000 for MiB*seconds. It is not L_i * time_since_exit:
      // only the latest observed interval (at most 5min) is represented.
      // Within-interval allocations/GC and already-destroyed VMs are unseen.
      const auto duration = std::max<int64_t>(
          0, snapshot.started_at_ms -
                 std::max(interval_start, sampled_residual->exited_at_ms));
      event.SetProps("sampled_leak_page_attributed_byte_ms",
                     retained * duration);
    }
    // A missing sampled block can mean no positive observable residual, not
    // that every exited page in the process has released all of its memory.
  }

  event.SetProps("sampled_vm_present", sampled_vm != nullptr);
  if (sampled_vm) {
    // LARGE, LONG-RESIDENT VM QUERY:
    // H_i=current usable heap of one registered shared VM; H=SUM(H_i).
    // Draw only positive H_i with p_i=H_i/H, w_i=1/p_i.
    // H_i*w_i=H, already emitted as shared_vm_heap_bytes_sum.
    // Private page VMs are not candidates. Valid zero heaps are counted in
    // the census but cannot be drawn. Missing/unsupported heaps cannot be
    // reconstructed by weighting, which is why the completeness gate matters.
    //
    // Baseline snapshots: schema_version=7, timer cohort,
    // vm_heap_snapshot_complete=1. For numerator/weighted-mean rows ALSO
    // require sampled_vm_present=1 and sampled_vm_age_ms>=1800000 (30min).
    // GROUP BY sampled_vm_name, sampled_vm_runtime_type:
    //   B=SUM(shared_vm_heap_bytes_sum) ON THESE SELECTED ROWS
    //   W=SUM(sampled_vm_selection_weight)
    //   rank B   -> accumulated contribution from large, old shared VMs
    //   rank B/W -> mean positive-heap VM size in the selected age cohort
    // B/W is a self-normalized ratio estimate, not a finite-sample guarantee
    // of unbiasedness. B depends on the number of observed snapshots; compare
    // matched exposure or divide by a common count of ALL baseline snapshots,
    // including those with no eligible old VM. Do not use only this name's
    // selected row count as an exposure denominator.
    //
    // To focus on retained engines without users, also filter
    // sampled_vm_active_page_count=0 AND sampled_vm_idle_ms>=600000.
    // Idle means no bound active pages, not an idle JS thread. Optional
    // heap>=threshold filtering is allowed, but must keep original p and w;
    // do not renormalize after age/idle/name/heap filters.
    //
    // A 900MiB old V8 plus a 100MiB young QuickJS -> p(V8)=0.9, and either
    // draw emits 1000MiB attributed heap. Filtering old VMs leaves expected
    // contribution 0.9*1000=900MiB. The old VM need not have grown at all.
    //
    // Ordinary AVG/PCT(heap, age, load count) follows the heap-biased draw.
    // Use inverse-probability weighted CDFs for positive-heap VM percentiles;
    // weighted counts are not independent observations or distinct VM IDs.
    // Stable names are aggregation dimensions: same-name VMs on different
    // threads or recreated later are distinct objects, with no telemetry
    // generation ID. Do not join by name to invent a continuous VM lifetime.
    //
    // PLATFORM VM RECIPE: after vm_heap_snapshot_complete=1 and presence,
    // use sampled_vm_runtime_type="v8" on Android/PC or "jsvm" on Harmony
    // (both are RuntimeManager-scoped), then age/idle filters
    // and B or B/W above. No page-slot, UI or Element filter is needed.
    // On every platform, "quickjs" studies measurable shared QuickJS heaps,
    // even when slot tracking is disabled.
    // iOS "jsc" has no heap-PPS rows; its group count can exist in the census,
    // but neither heap quantiles nor per-JSC-VM age/size data can be recovered
    // from this sampled block. An empty JSC query is unsupported, not zero.
    //
    // Filtering the PPS detail by runtime selects an estimator contribution,
    // not a runtime-specific process census. For that runtime's average share
    // of process memory, SUM(lynx_shared_vm_process_pss_ratio) on selected
    // rows divided by K uses ALL baseline snapshots with valid positive PSS
    // in K, including rows where another/no VM was selected.
    // Do not instead take AVG(lynx_shared_vm_process_pss_ratio) after the
    // runtime filter: that numerator includes every measurable shared VM.
    // QuickJS uses cached callback measurements; V8/JSVM are synchronously
    // queried in this collection. Compare collection cost by runtime, and do
    // not call both equally fresh or use the engine mix as a GC confidence
    // signal. Completeness remains about the selected metric's observed scope.
    const auto& vm = *sampled_vm;
    event.SetProps("sampled_vm_name", vm.name);
    event.SetProps("sampled_vm_runtime_type", RuntimeName(vm.type));
    event.SetProps("sampled_vm_age_ms",
                   snapshot.started_at_ms - vm.created_at_ms);
    event.SetProps(
        "sampled_vm_idle_ms",
        vm.idle_since_ms > 0 ? snapshot.started_at_ms - vm.idle_since_ms : 0);
    event.SetProps("sampled_vm_heap_bytes",
                   static_cast<uint64_t>(vm.heap_size));
    // sampled_vm_total_page_count counts accepted page-to-VM bindings, not
    // active pages, JS calls or
    // reload count. Unrelated repeated updates do not increment it; leaving
    // and rebinding a VM can add another binding, so it is not a distinct-ID
    // count over the entire VM lifetime.
    event.SetProps("sampled_vm_active_page_count", vm.active_page_count);
    event.SetProps("sampled_vm_total_page_count", vm.total_page_count);
    event.SetProps("sampled_vm_selection_weight", shared_heap / vm.heap_size);
    if (byte_time_valid && heap_complete) {
      // Use ALL triggers and byte_time_valid=1, vm_heap_snapshot_complete=1,
      // plus field presence. SUM(sampled_vm_attributed_heap_byte_ms), grouped
      // by name/type, estimates endpoint heap area:
      //   (H_i * dt_i)/p_i = H*dt_i,
      //   dt_i=current_start-max(interval_start,vm_created_at).
      // Divide by 1048576*1000 for MiB*seconds. This is not heap*VM age or a
      // growth metric. Objects destroyed between endpoints are missing and
      // memory need not have stayed constant throughout dt_i. Do not divide
      // by summed durations of ONLY selected VMs to claim a process mean.
      const auto duration =
          std::max<int64_t>(0, snapshot.started_at_ms -
                                   std::max(interval_start, vm.created_at_ms));
      event.SetProps("sampled_vm_attributed_heap_byte_ms",
                     shared_heap * duration);
    }
    // Slot fields apply to QuickJS only; type does not prove tracking is
    // enabled, nor does enabled mean every page is tracked. Count is
    // cumulative; slots are not reused; allocated_count>=253 means saturated,
    // whereas untracked_page_count records allocation failures. For saturation
    // investigations use the original heap PPS weight and expose untracked
    // counts; never interpret absent V8/JSVM slot fields as zero usage.
    event.SetProps("sampled_vm_slot_tracking_enabled",
                   vm.slot_tracking_enabled);
    if (vm.slot_tracking_enabled) {
      event.SetProps("sampled_vm_slot_allocated_count", vm.allocated_slots);
      event.SetProps("sampled_vm_untracked_page_count",
                     vm.untracked_page_count);
    }
  }
  event.SetProps("trigger_vm_present", snapshot.trigger.slot_vm.has_value());
  if (snapshot.trigger.slot_vm) {
    // SLOT TRANSITION QUERY: this VM caused the event; it was NOT PPS drawn
    // and may differ from sampled_vm. Never apply sampled_vm_selection_weight
    // or combine the two blocks as equivalent VM observations.
    //
    // Capacity example:
    //   schema_version=7; trigger_vm_present=1
    //   (trigger_reason_mask & 8)!=0; trigger_vm_slot_milestone=253
    //   GROUP BY trigger_vm_name
    //   PCT50/PCT95(trigger_vm_age_ms)/60000 -> age at observed capacity
    //   AVG/PCT95(trigger_vm_heap_bytes)/1048576, only where field exists
    // For first allocation failure instead use (trigger_reason_mask & 16)!=0.
    // Do not require milestone=253 on that query: an overflow trigger may
    // carry milestone=0. These are condition-on-transition statistics, not
    // percentiles of all VMs or a PPS contribution leaderboard.
    //
    // Name, heap and counts were captured when the trigger was queued; they
    // remain valid context even if that VM has since been destroyed. Age uses
    // THIS collection start minus its saved creation time, so it includes
    // queue delay and is not an exact transition timestamp. Do not equate
    // trigger heap with current census heap or the independently sampled VM.
    //
    // 256 slots contain Unknown/Common/Overflow plus 253 page slots.
    // Successful milestones are 16,32,...,240,253. Capacity reached is
    // different from the first failed allocation; successful count never
    // decreases when a page exits. Total page loads count instance bindings,
    // not just active pages; untracked counts failed slot allocations.
    // PV(milestone=32)/PV(milestone=16) is not a lifecycle conversion rate:
    // observation windows, queued/coalesced transitions, loss and process
    // termination can omit events, and name is not a unique lifecycle ID.
    const auto& vm = *snapshot.trigger.slot_vm;
    event.SetProps("trigger_vm_name", vm.name);
    event.SetProps("trigger_vm_slot_milestone", vm.milestone);
    event.SetProps("trigger_vm_slot_allocated_count", vm.allocated_slots);
    event.SetProps("trigger_vm_age_ms",
                   snapshot.started_at_ms - vm.created_at_ms);
    event.SetProps("trigger_vm_total_page_load_count", vm.total_page_count);
    event.SetProps("trigger_vm_untracked_page_count", vm.untracked_page_count);
    if (vm.heap_valid)
      event.SetProps("trigger_vm_heap_bytes",
                     static_cast<uint64_t>(vm.heap_size));
    // A present zero heap is valid; absent heap means no valid trigger-time
    // measurement and must not be filled with zero for AVG/PCT.
  }
  // MONITOR COST QUERY: AVG/PCT95/PCT99(event_build_duration_us), stratified
  // by trigger reason and active_page_count/shared_vm_count buckets. Include
  // partial events when measuring overhead; failed collection still costs
  // work. This measures monotonic elapsed time through the final business
  // field, not CPU time. It excludes the duration property's own insertion,
  // return/local teardown, prior collection, queue waiting and flush/delivery.
  // collection_duration_ms includes heap-query waits and process-memory reads.
  // If combining them, first form duration_us/1000+collection_duration_ms PER
  // ROW, then take PCT; adding their separate P95s is not total-latency P95.
  // Even that sum excludes pre-marker cleanup and trigger queue delay.
  event.SetProps("event_build_duration_us",
                 (fml::TimePoint::Now() - build_started_at).ToMicroseconds());
  return event;
}

}  // namespace performance
}  // namespace tasm
}  // namespace lynx
