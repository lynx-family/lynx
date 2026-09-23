// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/performance/memory_monitor/global_memory_monitor.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <optional>
#include <utility>

#include "base/include/fml/message_loop.h"
#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/memory/memory_pressure_level.h"
#include "base/include/memory/process_memory_info.h"
#include "base/include/no_destructor.h"
#include "base/include/vector.h"
#include "base/trace/native/trace_defines.h"
#include "core/runtime/js/js_realm_manager.h"
#include "core/services/event_report/event_tracker_platform_impl.h"
#include "core/services/performance/memory_monitor/memory_monitor.h"
#include "core/services/performance/memory_page_key.h"

namespace lynx {
namespace tasm {
namespace performance {
namespace {

constexpr uint32_t kPageSlotCapacity = fml::JSMemoryTrackSlotType::Count -
                                       fml::JSMemoryTrackSlotType::Overflow - 1;
constexpr int64_t kMiB = 1024 * 1024;
constexpr int64_t kResidencyThresholdMs = 600000;

struct VmHeapQueryResult {
  fml::AutoResetWaitableEvent event;
  std::optional<size_t> heap_size;
  int64_t measured_at_ms{0};
};

auto ReportRunner() {
  return report::EventTrackerPlatformImpl::GetReportTaskRunner();
}

bool SameOwner(const std::weak_ptr<runtime::js::VMInstance>& a,
               const std::weak_ptr<runtime::js::VMInstance>& b) {
  return !a.owner_before(b) && !b.owner_before(a);
}

void StoreSample(BtsVmState& state, const BtsMemorySample& sample) {
  // Ordinary callbacks refresh current heap/slots. Only a valid, newer full
  // GC replaces residual evidence; notification arrival order is irrelevant.
  state.heap_valid = sample.heap_valid;
  state.heap_size = sample.heap_size;
  state.measured_at_ms = sample.measured_at_ms;
  state.has_per_instance_data = sample.slots_valid;
  if (sample.slots_valid) {
    state.slot_tracking_enabled = true;
    state.memory_slots = sample.slots;
    if (sample.is_full_gc && sample.heap_valid &&
        sample.measured_at_ms > state.full_gc_at_ms) {
      state.full_gc_slots = sample.slots;
      state.full_gc_at_ms = sample.measured_at_ms;
    }
  }
}

void AddQuickJSVMProps(report::MoveOnlyEvent& event, const BtsVmState& vm,
                       int64_t now_ms) {
  event.SetProps("vm_name", vm.vm_name());
  event.SetProps("vm_age_ms", now_ms - vm.created_at_ms);
  event.SetProps("vm_total_page_count", vm.total_page_count);
  event.SetProps("vm_slot_allocated_count", vm.allocated_slots);
  event.SetProps("vm_untracked_page_count", vm.untracked_page_count);
  event.SetProps("vm_heap_bytes", static_cast<uint64_t>(vm.heap_size));
}

}  // namespace

void InstanceState::SetUrl(const std::string& url) {
  if (url_ == url) return;
  url_ = url;
  if (!page_key_resolved) page_key = NormalizeMemoryPageUrl(url_);
}

GlobalMemoryMonitor& GlobalMemoryMonitor::GetInstance() {
  static base::NoDestructor<GlobalMemoryMonitor> monitor;
  return *monitor;
}

GlobalMemoryMonitor::GlobalMemoryMonitor()
    : devtool_connected_callback_(
          LYNX_ON_DEVTOOL_CONNECTED_NOTIFICATION,
          [](const std::string& tag, intptr_t data) {
            // Production release enables memory monitoring by sampling.
            // Force-enable it when DevTool connects so local debugging always
            // has memory data.
            MemoryMonitor::ForceEnableForTesting(
                MemoryMonitor::ForceEnableMode::kPersistent);
          }) {}

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
  // Registration starts polling without an immediate snapshot. Every 8s,
  // check residency, the timer deadline (default 300s), and PSS high water.
  // An 8s poll is not an 8s event cadence. Peaks between polls can be missed.
  poll_timer_->Start(fml::TimeDelta::FromSeconds(8),
                     [this] { PollProcessMemory(); });
  pressure_callback_ = std::make_unique<base::NotificationCallback>(
      base::MEMORY_PRESSURE_NOTIFICATION,
      [this](const std::string&, intptr_t level) {
        if (level <= base::MEMORY_PRESSURE_LEVEL_NONE) return;
        // Each positive-pressure notification requests its own snapshot;
        // there is no monitor-level cooldown or timer/PSS coalescing here.
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
          // Begin at creation, including pages that never complete loading.
          // Only the first registration enrolls this instance; reload and
          // repeated updates cannot extend its observation window.
          it->second.created_at_ms = at_ms;
          auto observation = std::make_unique<ResidencyObservation>();
          observation->instance_id = id;
          observation->created_at_ms = at_ms;
          it->second.residency_observation = observation.get();
          residency_observations_.push_back(std::move(observation));
          const auto deadline = at_ms + kResidencyThresholdMs;
          if (!next_residency_at_ms_ || deadline < next_residency_at_ms_) {
            next_residency_at_ms_ = deadline;
          }
        }
        WithInstance(id, std::move(callback));
      });
}

BtsVmState* GlobalMemoryMonitor::FindVM(runtime::js::VMInstance* vm) {
  for (auto& state : bts_vm_state_) {
    if (state->vm_ptr == vm) return state.get();
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
        if (state.bts_vm != previous) BindInstance(state, previous);
        if (auto* vm = FindVM(state.bts_vm)) EmitSlotOverflowTrigger(*vm);
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

void GlobalMemoryMonitor::OnBtsVMCreate(
    const std::string& group_id, const std::string& monitoring_group_label,
    const std::shared_ptr<runtime::js::VMInstance>& vm) {
  if (!MemoryMonitor::Enable()) return;
  auto runner = fml::MessageLoop::GetCurrent().GetTaskRunner();
  // Never capture the incoming strong reference into a report-thread task:
  // releasing its last reference there would destroy an engine on the wrong
  // thread. The raw pointer is identity-only; lock the weak_ptr on JS only.
  fml::TaskRunner::RunNowOrPostTask(
      ReportRunner(),
      [this, group_id, monitoring_group_label,
       weak = std::weak_ptr<runtime::js::VMInstance>(vm), ptr = vm.get(),
       type = vm->GetRuntimeType(), runner, created = MemoryNowMs()] {
        if (weak.expired()) return;
        Start();
        if (auto* existing = FindVM(ptr)) {
          if (SameOwner(existing->vm, weak)) return;
          // An engine without a destroy callback can leave an expired entry
          // until the next snapshot. Address reuse must not bind new pages to
          // that old entry or carry its age/counters into this new object.
          OnBtsVMDestroy(ptr);
        }
        auto state = std::make_unique<BtsVmState>();
        state->type = type;
        state->runtime_manager_scope =
            runtime::JSRealmManager::IsVMSharedAcrossGroups(type);
        state->SetName(state->runtime_manager_scope
                           ? std::string("runtime_manager/") + RuntimeName(type)
                           : group_id);
        state->monitoring_group_label = monitoring_group_label;
        state->vm = weak;
        state->vm_ptr = ptr;
        state->js_thread_runner = runner;
        state->created_at_ms = created;
        state->idle_since_ms = created;
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
    state.memory_monitor = nullptr;
    state.destroyed = true;
    state.exited_at_ms = at_ms;
    if (state.residency_observation) {
      // Keep early exits until the original creation+10min deadline. Counting
      // them now would bias the denominator toward short-lived processes.
      state.residency_observation->exited_at_ms = at_ms;
      state.residency_observation->page_key = state.page_key;
      state.residency_observation = nullptr;
    }
    if (vm && state.bts_expected && !fml::IsValidInstanceSlot(state.slot)) {
      vm->has_untracked_exited_pages = true;
    }
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
  fml::TaskRunner::RunNowOrPostTask(ReportRunner(), [this, vm] {
    const auto state = std::find_if(
        bts_vm_state_.begin(), bts_vm_state_.end(),
        [vm](const auto& candidate) { return candidate->vm_ptr == vm; });
    if (state == bts_vm_state_.end()) return;
    bts_vm_state_.erase(state);
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
  fml::TaskRunner::RunNowOrPostTask(ReportRunner(),
                                    [this, vm, sample = std::move(sample)] {
                                      if (auto* state = FindVM(vm))
                                        StoreSample(*state, sample);
                                    });
}

void GlobalMemoryMonitor::OnBtsVMSlotAllocate(runtime::js::VMInstance* vm,
                                              int32_t slot) {
  if (!MemoryMonitor::Enable()) return;
  fml::TaskRunner::RunNowOrPostTask(ReportRunner(), [this, vm, slot] {
    auto* state = FindVM(vm);
    if (!state) return;
    state->slot_tracking_enabled = true;
    if (slot == -1) {
      // Full capacity alone is not overflow: the first failed allocation is
      // a separate transition. Later failures only increase the count.
      ++state->untracked_page_count;
      if (!state->overflowed) state->pending_overflow = true;
      state->overflowed = true;
    } else if (fml::IsValidInstanceSlot(slot)) {
      ++state->allocated_slots;
    }
  });
}

void GlobalMemoryMonitor::OnMTSRuntimePoolUpdate(
    int32_t pool_instance_id, const std::string& template_url,
    runtime::ContextType context_type, int64_t created_at_ms,
    uint64_t runtime_count, int64_t heap_bytes) {
  if (!MemoryMonitor::Enable() || pool_instance_id < 0) {
    return;
  }
  fml::TaskRunner::RunNowOrPostTask(
      ReportRunner(), [this, pool_instance_id, template_url, context_type,
                       created_at_ms, runtime_count, heap_bytes] {
        Start();
        auto [it, inserted] =
            mts_runtime_pool_state_.try_emplace(pool_instance_id);
        auto& state = it->second;
        if (inserted) {
          state.pool_instance_id = pool_instance_id;
          state.template_url = template_url;
          state.page_key = NormalizeMemoryPageUrl(template_url);
          state.context_type = static_cast<int32_t>(context_type);
          state.created_at_ms = created_at_ms;
        }
        state.runtime_count = runtime_count;
        state.heap_bytes = std::max<int64_t>(0, heap_bytes);
      });
}

void GlobalMemoryMonitor::OnMTSRuntimePoolDestroy(int32_t pool_instance_id) {
  if (!MemoryMonitor::Enable() || pool_instance_id < 0) return;
  fml::TaskRunner::RunNowOrPostTask(ReportRunner(), [this, pool_instance_id] {
    mts_runtime_pool_state_.erase(pool_instance_id);
  });
}

void GlobalMemoryMonitor::EmitSlotOverflowTrigger(BtsVmState& vm) {
  if (!vm.pending_overflow) return;
  Trigger trigger;
  trigger.reasons = kOverflow;
  trigger.overflow_vm = std::make_unique<BtsVmState>(vm);
  vm.pending_overflow = false;
  // Queue after binding commits; do not collect in the middle of a
  // WithInstance callback. It does not merge with a timer/PSS snapshot or
  // reset the periodic deadline.
  ReportRunner()->PostTask([this, trigger = std::move(trigger)]() mutable {
    RequestReport(std::move(trigger));
  });
}

const InstanceState* GlobalMemoryMonitor::ResolveInstancePageKey(int32_t id) {
  assert(ReportRunner()->RunsTasksOnCurrentThread());
  const auto it = instance_state_.find(id);
  if (it == instance_state_.end()) return nullptr;
  auto& state = it->second;
  if (!state.destroyed && !state.page_key_resolved) {
    const auto page_id =
        report::EventTracker::GetGenericInfoOrExtraParam(id, "page_id");
    if (!page_id.empty() && page_id != "unknown") state.page_key = page_id;
    state.page_key_resolved = true;
  }
  return &state;
}

const InstanceState* GlobalMemoryMonitor::GetInstanceState(int32_t id) {
  assert(ReportRunner()->RunsTasksOnCurrentThread());
  auto it = instance_state_.find(id);
  return it == instance_state_.end() ? nullptr : &it->second;
}

void GlobalMemoryMonitor::ResolveMTSRuntimePoolPageKeys() {
  assert(ReportRunner()->RunsTasksOnCurrentThread());
  for (auto& item : mts_runtime_pool_state_) {
    auto& pool = item.second;
    if (pool.page_key_resolved) continue;
    int32_t matching_id = std::numeric_limits<int32_t>::max();
    for (const auto& [instance_id, page] : instance_state_) {
      if (!page.destroyed && page.url() == pool.template_url) {
        matching_id = std::min(matching_id, instance_id);
      }
    }
    if (matching_id == std::numeric_limits<int32_t>::max()) continue;
    if (const auto* page = ResolveInstancePageKey(matching_id)) {
      pool.page_key = page->page_key;
      pool.page_key_resolved = true;
    }
  }
}

std::vector<report::MoveOnlyEvent> GlobalMemoryMonitor::TakeResidencyReports(
    int64_t now_ms) {
  // Q3: exact cohort counts within selected processes, grouped by page_key.
  // A=SUM(alive_page_count), B=SUM(evaluated_page_count), survival=A/B.
  // Both counts require observation through creation+10min. A process that
  // ends sooner contributes neither; process age alone is insufficient.
  // If every evaluated instance survives, this ratio is 100% regardless of
  // how many unevaluated short processes existed. It is not a user UV ratio
  // or the fraction of active snapshot samples whose age exceeds 10min.
  assert(ReportRunner()->RunsTasksOnCurrentThread());
  if (!next_residency_at_ms_ || now_ms < next_residency_at_ms_) return {};
  struct Counts {
    uint64_t evaluated{0};
    uint64_t alive{0};
  };
  std::unordered_map<std::string, Counts> counts;
  // Move pending unique_ptrs into a replacement vector. Pointee addresses
  // stay stable for InstanceState, and no repeated vector erases are needed.
  // Creation timestamps need not follow notification order.
  std::vector<std::unique_ptr<ResidencyObservation>> pending_observations;
  pending_observations.reserve(residency_observations_.size());
  next_residency_at_ms_ = 0;
  for (auto& observation_ptr : residency_observations_) {
    auto& observation = *observation_ptr;
    const auto deadline = observation.created_at_ms + kResidencyThresholdMs;
    if (deadline > now_ms) {
      if (!next_residency_at_ms_ || deadline < next_residency_at_ms_) {
        next_residency_at_ms_ = deadline;
      }
      pending_observations.push_back(std::move(observation_ptr));
      continue;
    }
    auto it = instance_state_.find(observation.instance_id);
    // Observation.page_key is saved only on exit, before instance cleanup.
    // Live pages read InstanceState directly without duplicating the string:
    // first load may have resolved its fallback to a host page_id since
    // creation. This lookup never queries platform storage.
    const std::string* key = &observation.page_key;
    // A reused instance ID must not supply the new page's key.
    if (it != instance_state_.end() &&
        it->second.residency_observation == &observation) {
      key = &it->second.page_key;
      it->second.residency_observation = nullptr;
    }
    auto& count = counts[*key];
    ++count.evaluated;
    // A late poll still evaluates the fixed deadline. Exiting exactly at
    // that deadline means not alive; visibility and memory validity do not
    // participate. Suspension or termination before delivery can lose events.
    count.alive +=
        observation.exited_at_ms < 0 || observation.exited_at_ms > deadline;
  }
  residency_observations_ = std::move(pending_observations);
  std::vector<report::MoveOnlyEvent> events;
  events.reserve(counts.size());
  // One row per due key, no memory snapshot or entity-selection fields.
  for (const auto& [key, count] : counts) {
    report::MoveOnlyEvent event;
    event.SetName("lynxsdk_memory_page_residency");
    event.SetInstanceId(report::kUnknownInstanceId);
    event.SetProps("schema_version", 10);
    event.SetProps("page_key", key);
    event.SetProps("residency_threshold_ms", kResidencyThresholdMs);
    event.SetProps("evaluated_page_count", count.evaluated);
    event.SetProps("alive_page_count", count.alive);
    events.push_back(std::move(event));
  }
  return events;
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
  assert(usage.total_bytes >= usage.bts_bytes);
  usage.total_bytes -= usage.bts_bytes;
  usage.bts_bytes = 0;
  usage.bts_shared = true;
  if (vm.slot_tracking_enabled && vm.has_per_instance_data &&
      fml::IsValidInstanceSlot(page.slot)) {
    const auto bytes = vm.memory_slots[page.slot];
    if (bytes <= static_cast<size_t>(std::numeric_limits<int64_t>::max())) {
      usage.bts_bytes = static_cast<int64_t>(bytes);
    }
  }
  usage.total_bytes += usage.bts_bytes;
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

PageMemoryUsage GlobalMemoryMonitor::RefreshInstanceMemoryUsage(
    InstanceState& page) {
  if (page.memory_monitor) {
    return page.memory_monitor->RefreshMemoryUsage();
  }
  if (auto* vm = FindVM(page.bts_vm)) {
    RefreshPageBts(page, *vm);
  }
  return page.usage;
}

void GlobalMemoryMonitor::GetInstanceMemoryUsage(
    int32_t id, InstanceMemoryUsageCallback&& callback) {
  fml::TaskRunner::RunNowOrPostTask(
      ReportRunner(), [this, id, callback = std::move(callback)]() mutable {
        InstanceMemoryUsageResult result;
        if (!MemoryMonitor::Enable()) {
          result.status = MemoryUsageQueryStatus::kMonitoringDisabled;
          callback(std::move(result));
          return;
        }
        auto it = instance_state_.find(id);
        if (it == instance_state_.end() || it->second.destroyed) {
          result.status = MemoryUsageQueryStatus::kInvalidInstance;
          callback(std::move(result));
          return;
        }
        Snapshot snapshot;
        RefreshInstanceMemoryUsage(it->second);
        if (auto* vm = FindVM(it->second.bts_vm)) {
          RefreshVmHeap(*vm, snapshot, true);
        }
        result.usage =
            BuildInstanceMemoryUsage(id, it->second, snapshot.failed_heap_vms);
        callback(std::move(result));
      });
}

void GlobalMemoryMonitor::GetGlobalMemoryUsage(
    GlobalMemoryUsageCallback&& callback) {
  fml::TaskRunner::RunNowOrPostTask(
      ReportRunner(), [this, callback = std::move(callback)]() mutable {
        callback(BuildGlobalMemoryUsage(CollectSnapshot({}, true)));
      });
}

bool GlobalMemoryMonitor::IsHeapAvailable(
    const BtsVmState& vm,
    const std::vector<runtime::js::VMInstance*>& failed_heap_vms) const {
  return vm.type != runtime::js::JSRuntimeType::jsc && vm.heap_valid &&
         std::find(failed_heap_vms.begin(), failed_heap_vms.end(), vm.vm_ptr) ==
             failed_heap_vms.end();
}

InstanceMemoryUsage GlobalMemoryMonitor::BuildInstanceMemoryUsage(
    int32_t id, const InstanceState& page,
    const std::vector<runtime::js::VMInstance*>& failed_heap_vms) {
  InstanceMemoryUsage result;
  result.instance_id = id;
  result.page_id = page.page_key;
  result.url = page.url();
  result.page = page.usage;
  result.bts_runtime_group_id = page.group_id;
  if (auto* vm = FindVM(page.bts_vm)) {
    if (IsHeapAvailable(*vm, failed_heap_vms) &&
        vm->heap_size <=
            static_cast<size_t>(std::numeric_limits<int64_t>::max())) {
      result.bts_heap_bytes = static_cast<int64_t>(vm->heap_size);
    }
  } else {
    result.bts_heap_bytes = result.page.bts_bytes;
  }
  return result;
}

GlobalMemoryUsage GlobalMemoryMonitor::BuildGlobalMemoryUsage(
    const Snapshot& snapshot) {
  GlobalMemoryUsage result;
  base::InlineVector<runtime::js::VMInstance*, 16> active_vms;
  result.instances.reserve(instance_state_.size());
  for (const auto& item : instance_state_) {
    const auto& page = item.second;
    if (page.destroyed) continue;
    result.instances.push_back(
        BuildInstanceMemoryUsage(item.first, page, snapshot.failed_heap_vms));
    const auto& usage = page.usage;
    result.total.element_bytes += usage.element_bytes;
    result.total.element_count += usage.element_count;
    result.total.mts_bytes += usage.mts_bytes;
    result.total.ui_bytes += usage.ui_bytes;
    result.total.total_bytes +=
        usage.total_bytes - (usage.bts_shared ? usage.bts_bytes : 0);
    if (usage.bts_shared) {
      if (page.bts_vm && std::find(active_vms.begin(), active_vms.end(),
                                   page.bts_vm) == active_vms.end()) {
        active_vms.push_back(page.bts_vm);
      }
    } else {
      result.total.bts_bytes += usage.bts_bytes;
    }
  }
  for (const auto& entry : bts_vm_state_) {
    const auto& vm = *entry;
    if (std::find(active_vms.begin(), active_vms.end(), vm.vm_ptr) ==
            active_vms.end() ||
        !IsHeapAvailable(vm, snapshot.failed_heap_vms) ||
        vm.heap_size >
            static_cast<size_t>(std::numeric_limits<int64_t>::max())) {
      continue;
    }
    const auto heap = static_cast<int64_t>(vm.heap_size);
    result.total.bts_bytes += heap;
    result.total.total_bytes += heap;
  }
  return result;
}

void GlobalMemoryMonitor::PollProcessMemory() {
  const auto now = MemoryNowMs();
  // Residency remains enabled even if timer/PSS triggers are configured off.
  // Its earliest-deadline check avoids scanning pending pages on every poll.
  for (auto& event : TakeResidencyReports(now)) {
    report::EventTracker::OnGlobalEvent(
        [event = std::move(event)](auto& target) mutable {
          target = std::move(event);
        });
  }
  Trigger trigger;
  const auto interval = MemoryMonitor::GetSettings().global.time_interval_sec;
  if (interval && next_timer_at_ms_ && now >= next_timer_at_ms_) {
    trigger.reasons |= kTimer;
    // Skip missed deadlines; do not replay historical samples after suspension.
    next_timer_at_ms_ +=
        ((now - next_timer_at_ms_) / (interval * int64_t{1000}) + 1) *
        (interval * int64_t{1000});
  }
  const auto pss = base::GetProcessPssBytes();
  const int64_t step =
      MemoryMonitor::GetSettings().global.pss_threshold_mb * kMiB;
  // The initial bucket is zero. One upward jump emits one crossing even if
  // several buckets were skipped; falling PSS never resets the high water.
  if (pss > 0 && step > 0 && pss / step > pss_high_water_bucket_) {
    trigger.reasons |= kPss;
    trigger.pss_before = pss_high_water_bucket_ * step;
    pss_high_water_bucket_ = pss / step;
    trigger.pss_after = pss_high_water_bucket_ * step;
  }
  if (trigger.reasons) {
    // Timer and PSS from this poll share one snapshot with both reason tokens.
    RequestReport(std::move(trigger));
  }
}

void GlobalMemoryMonitor::RequestReport(Trigger trigger) {
  assert(ReportRunner()->RunsTasksOnCurrentThread());
  Start();
  auto snapshot = CollectSnapshot(std::move(trigger));
  const auto pss = base::GetProcessPssBytes();
  auto events = BuildEvents(snapshot, MemoryNowMs(), pss);
  previous_snapshot_at_ms_ = snapshot.started_at_ms;
  // Freeze before enqueue: flush/delivery must not read mutable state.
  // Platform arrival time can therefore be later than this observation.
  for (auto& event : events) {
    report::EventTracker::OnGlobalEvent(
        [event = std::move(event)](auto& target) mutable {
          target = std::move(event);
        });
  }
}

GlobalMemoryMonitor::Snapshot GlobalMemoryMonitor::CollectSnapshot(
    Trigger trigger, bool refresh_trackable_heaps) {
  assert(ReportRunner()->RunsTasksOnCurrentThread());
  // Engine wrappers do not depend on this monitor. Expired weak refs can be
  // removed without locking or destroying the engine on this thread.
  base::InlineVector<runtime::js::VMInstance*, 16> expired;
  for (const auto& entry : bts_vm_state_) {
    const auto& vm = *entry;
    if (vm.vm.expired()) expired.push_back(vm.vm_ptr);
  }
  for (auto* vm : expired) OnBtsVMDestroy(vm);
  for (auto& item : instance_state_) {
    auto& page = item.second;
    if (page.destroyed) continue;
    RefreshInstanceMemoryUsage(page);
  }
  ResolveMTSRuntimePoolPageKeys();
  Snapshot snapshot;
  snapshot.started_at_ms = MemoryNowMs();
  snapshot.trigger = std::move(trigger);
  for (auto& entry : bts_vm_state_) {
    RefreshVmHeap(*entry, snapshot, refresh_trackable_heaps);
  }
  return snapshot;
}

void GlobalMemoryMonitor::RefreshVmHeap(BtsVmState& vm, Snapshot& snapshot,
                                        bool refresh_trackable_heaps) {
  const bool heap_only = vm.type == runtime::js::JSRuntimeType::v8 ||
                         vm.type == runtime::js::JSRuntimeType::jsvm;
  if (!heap_only && !(refresh_trackable_heaps &&
                      vm.type == runtime::js::JSRuntimeType::quickjs)) {
    return;
  }
  std::optional<size_t> heap_size;
  int64_t measured_at_ms = 0;
  if (vm.js_thread_runner) {
    // The posted task may outlive this call after a timeout.
    auto result = std::make_shared<VmHeapQueryResult>();
    auto query = [weak = vm.vm, result] {
      if (auto strong = weak.lock()) {
        result->heap_size = strong->GetHeapSize();
        result->measured_at_ms = MemoryNowMs();
      }
      result->event.Signal();
    };
    vm.js_thread_runner->PostTask(std::move(query));
    if (result->event.WaitWithTimeout(fml::TimeDelta::FromSeconds(1))) {
      snapshot.partial = true;
      snapshot.failed_heap_vms.push_back(vm.vm_ptr);
      return;
    }
    heap_size = result->heap_size;
    measured_at_ms = result->measured_at_ms;
  }
  if (heap_size) {
    vm.heap_size = *heap_size;
    vm.heap_valid = true;
    vm.measured_at_ms = measured_at_ms;
    return;
  }
  // Preserve the candidate and old cache for later recovery. Builders
  // suppress this VM's heap fields for the current snapshot.
  snapshot.partial = true;
  snapshot.failed_heap_vms.push_back(vm.vm_ptr);
}

std::vector<report::MoveOnlyEvent> GlobalMemoryMonitor::BuildEvents(
    const Snapshot& snapshot, int64_t now, int64_t pss) {
  assert(ReportRunner()->RunsTasksOnCurrentThread());
  auto make_event = [&](const char* name, bool include_ui_scope = false) {
    report::MoveOnlyEvent event;
    event.Reserve(32);
    event.SetName(name);
    event.SetInstanceId(report::kUnknownInstanceId);
    event.SetProps("schema_version", 10);
    if (include_ui_scope) {
      event.SetProps("lynx_ui_memory_enabled",
                     MemoryMonitor::GetSettings().lynx_ui.enabled != 0);
    }
    return event;
  };
  double accounted = 0, shared_heap = 0, retained = 0, mts_pool_heap = 0;
  size_t active_count = 0;
  size_t sampled_quickjs_pending_gc = 0, quickjs_count = 0;
  uint64_t mts_pool_runtime_count = 0;
  double sampled_quickjs_retained = 0;
  bool retained_complete = true;
  const BtsVmState* sampled_vm = nullptr;
  const BtsVmState* sampled_quickjs = nullptr;
  const MTSRuntimePoolState* sampled_mts_pool = nullptr;
  const InstanceState* sampled_page = nullptr;
  const InstanceState* sampled_residual = nullptr;
  double residual_bytes = 0;
  MemoryReservoir page_picker, vm_picker, quickjs_picker, residual_picker,
      mts_pool_picker;
  for (const auto& entry : bts_vm_state_) {
    const auto& vm = *entry;
    if (vm_picker.Consider(1, random_)) sampled_vm = &vm;
    if (vm.type == runtime::js::JSRuntimeType::quickjs) {
      ++quickjs_count;
      if (quickjs_picker.Consider(1, random_)) sampled_quickjs = &vm;
    }
    if (IsHeapAvailable(vm, snapshot.failed_heap_vms)) {
      shared_heap += vm.heap_size;
      accounted += vm.heap_size;
    }
    if (vm.slot_tracking_enabled && !vm.has_per_instance_data)
      retained_complete = false;
  }
  for (const auto& item : instance_state_) {
    const auto& page = item.second;
    // The VM registry is normally tiny; avoid a per-report hash allocation.
    const auto* vm = FindVM(page.bts_vm);
    if (page.destroyed) {
      if (vm && fml::IsValidInstanceSlot(page.slot)) {
        // Compare capture time, not reporter delivery order. A pre-exit or
        // same-ms GC cannot establish post-exit residuals.
        if (vm->full_gc_at_ms <= page.exited_at_ms) {
          sampled_quickjs_pending_gc += vm == sampled_quickjs;
          retained_complete = false;
          continue;
        }
        const double bytes = vm->full_gc_slots[page.slot];
        retained += bytes;
        if (vm == sampled_quickjs) sampled_quickjs_retained += bytes;
        if (residual_picker.Consider(bytes, random_)) {
          sampled_residual = &page;
          residual_bytes = bytes;
        }
      }
      continue;
    }
    ++active_count;
    const auto& usage = page.usage;
    // PROCESS ACCOUNTING:
    //   A = SUM(active page total - its already-included shared BTS bytes)
    //       + SUM(unique supported shared-VM heaps with valid cached values)
    //       + SUM(bundle-local MTS pool heaps).
    // Slots, including exited-page residuals, already belong to those heaps.
    // Neither page-group totals nor retained bytes may be added to A again.
    // Pool heaps are process overhead, not GlobalMemoryUsage page totals.
    // Subtract the component in this same page total, not a slot from a
    // different observation. Example: page 100MiB includes a 40MiB slot;
    // its shared VM is 200MiB -> A=100-40+200=260MiB, not 300MiB.
    accounted += usage.total_bytes - (usage.bts_shared ? usage.bts_bytes : 0);
    if (page_picker.Consider(1, random_)) {
      sampled_page = &page;
    }
  }
  for (const auto& item : mts_runtime_pool_state_) {
    const auto& pool = item.second;
    mts_pool_heap += pool.heap_bytes;
    mts_pool_runtime_count += pool.runtime_count;
    if (mts_pool_picker.Consider(1, random_)) sampled_mts_pool = &pool;
  }
  accounted += mts_pool_heap;
  std::vector<report::MoveOnlyEvent> events;
  events.reserve(8);

  // Process rows are self-qualified: a positive PSS and complete synchronous
  // heap collection are required before the event exists.
  if (pss > 0 && !snapshot.partial) {
    auto event = make_event("lynxsdk_memory_process", true);
    event.SetProps("process_pss_bytes", pss);
    event.SetProps("lynx_accounted_memory_bytes", accounted);
    event.SetProps("active_page_count", static_cast<uint64_t>(active_count));
    event.SetProps("shared_vm_count",
                   static_cast<uint64_t>(bts_vm_state_.size()));
    event.SetProps("shared_vm_heap_bytes_sum", shared_heap);
    event.SetProps("mts_pool_count",
                   static_cast<uint64_t>(mts_runtime_pool_state_.size()));
    event.SetProps("mts_pool_runtime_count", mts_pool_runtime_count);
    event.SetProps("mts_pool_heap_bytes_sum", mts_pool_heap);
    event.SetProps("shared_vm_tracked_exited_page_retained_bytes", retained);
    event.SetProps("lynx_accounted_process_pss_ratio", accounted / pss);
    event.SetProps("lynx_shared_vm_process_pss_ratio", shared_heap / pss);
    event.SetProps("lynx_mts_pool_process_pss_ratio", mts_pool_heap / pss);
    event.SetProps("lynx_tracked_page_retained_process_pss_ratio",
                   retained / pss);
    if (snapshot.trigger.reasons & kPss) {
      event.SetProps("trigger_pss_previous_milestone_bytes",
                     snapshot.trigger.pss_before);
      event.SetProps("trigger_pss_milestone_bytes", snapshot.trigger.pss_after);
    }
    events.push_back(std::move(event));
  }

  if ((snapshot.trigger.reasons & kTimer) && sampled_page) {
    auto event = make_event("lynxsdk_memory_page", true);
    const auto& page = *sampled_page;
    const auto age = snapshot.started_at_ms - page.created_at_ms;
    const double weight = active_count;
    event.SetProps("active_page_count", static_cast<uint64_t>(active_count));
    event.SetProps("page_age_ms", age);
    event.SetProps("page_memory_bytes", page.usage.total_bytes);
    event.SetProps("page_vm_shared", page.usage.bts_shared);
    event.SetProps(
        "page_vm_runtime_type",
        page.bts_known ? RuntimeName(page.bts_runtime_type) : "unknown");
    event.SetProps("page_memory_bytes_weighted",
                   page.usage.total_bytes * weight);
    // The selected instance chooses a key with probability n/N. N/n is the
    // inverse weight for the same-key aggregate.
    size_t n = 0;
    double sum = 0, maximum = 0;
    for (const auto& candidate : instance_state_) {
      if (!candidate.second.destroyed &&
          candidate.second.page_key == page.page_key) {
        ++n;
        const auto& usage = candidate.second.usage;
        sum += usage.total_bytes;
        maximum = std::max(maximum, static_cast<double>(usage.total_bytes));
      }
    }
    const double group_weight = weight / n;
    event.SetProps("page_key", page.page_key);
    event.SetProps("page_group_instance_count", static_cast<uint64_t>(n));
    event.SetProps("page_group_memory_bytes", sum);
    event.SetProps("page_group_max_instance_bytes", maximum);
    event.SetProps("page_group_selection_weight", group_weight);
    event.SetProps("page_group_memory_bytes_weighted", sum * group_weight);
    events.push_back(std::move(event));
  }

  if ((snapshot.trigger.reasons & kTimer) && sampled_mts_pool) {
    auto event = make_event("lynxsdk_memory_mts_pool");
    const auto& pool = *sampled_mts_pool;
    const double weight = mts_runtime_pool_state_.size();
    event.SetProps("page_key", pool.page_key);
    event.SetProps("mts_context_type", pool.context_type);
    event.SetProps(
        "mts_pool_age_ms",
        std::max<int64_t>(0, snapshot.started_at_ms - pool.created_at_ms));
    event.SetProps("mts_pool_runtime_count", pool.runtime_count);
    event.SetProps("mts_pool_runtime_count_weighted",
                   pool.runtime_count * weight);
    event.SetProps("mts_pool_selection_weight", weight);
    event.SetProps("mts_pool_heap_bytes", pool.heap_bytes);
    event.SetProps("mts_pool_heap_bytes_weighted", pool.heap_bytes * weight);
    event.SetProps("mts_pool_heap_selection_weight", weight);
    events.push_back(std::move(event));
  }

  if ((snapshot.trigger.reasons & kTimer) && sampled_residual) {
    auto event = make_event("lynxsdk_memory_page_residual");
    event.SetProps("shared_vm_tracked_exited_page_retained_bytes", retained);
    event.SetProps("leak_page_key", sampled_residual->page_key);
    event.SetProps("leak_page_selection_weight", retained / residual_bytes);
    events.push_back(std::move(event));
  }

  if ((snapshot.trigger.reasons & kTimer) && sampled_vm) {
    auto event = make_event("lynxsdk_memory_vm_stat");
    // One VM is drawn uniformly from all registered engines. Reuse fields are
    // always emitted. Heap fields and their matching denominator are emitted
    // only when this VM has usable data for the current snapshot.
    const auto& vm = *sampled_vm;
    event.SetProps("vm_name", vm.vm_name());
    event.SetProps("vm_runtime_type", RuntimeName(vm.type));
    event.SetProps("vm_age_ms", snapshot.started_at_ms - vm.created_at_ms);
    event.SetProps("vm_idle_ms", vm.idle_since_ms > 0
                                     ? snapshot.started_at_ms - vm.idle_since_ms
                                     : 0);
    const double vm_weight = bts_vm_state_.size();
    const bool heap_valid = IsHeapAvailable(vm, snapshot.failed_heap_vms);
    if (heap_valid) {
      event.SetProps("vm_heap_bytes", static_cast<uint64_t>(vm.heap_size));
      event.SetProps("vm_heap_bytes_weighted", vm.heap_size * vm_weight);
      event.SetProps("vm_heap_selection_weight", vm_weight);
      if (vm.measured_at_ms > 0) {
        event.SetProps("vm_heap_source_age_ms",
                       std::max<int64_t>(0, now - vm.measured_at_ms));
      }
    }
    // vm_total_page_count counts accepted page-to-VM bindings, not
    // successful loads, active pages, JS calls or reloads. Unrelated updates
    // do not increment it; leaving
    // and rebinding a VM can add another binding, so it is not a distinct-ID
    // count over the entire VM lifetime. Summing across snapshots repeats
    // historical bindings; it is not the number of new loads in the period.
    event.SetProps("vm_active_page_count", vm.active_page_count);
    event.SetProps("vm_total_page_count", vm.total_page_count);
    event.SetProps("vm_total_page_count_weighted",
                   vm.total_page_count * vm_weight);
    event.SetProps("vm_selection_weight", vm_weight);
    events.push_back(std::move(event));
  }
  // Q7 uses every snapshot as an endpoint. The presence of each area field is
  // its source-specific validity gate.
  const bool interval_valid =
      previous_snapshot_at_ms_ > 0 &&
      snapshot.started_at_ms >= previous_snapshot_at_ms_;
  if (interval_valid) {
    const auto interval_start = std::max(
        previous_snapshot_at_ms_, snapshot.started_at_ms - int64_t{300000});
    const bool vm_area_valid =
        sampled_vm && IsHeapAvailable(*sampled_vm, snapshot.failed_heap_vms);
    const bool residual_area_valid = sampled_residual && retained_complete;
    if (vm_area_valid || residual_area_valid) {
      auto event = make_event("lynxsdk_memory_persistence");
      if (vm_area_valid) {
        const auto& vm = *sampled_vm;
        const double weight = bts_vm_state_.size();
        const auto duration = std::max<int64_t>(
            0, snapshot.started_at_ms -
                   std::max(interval_start, vm.created_at_ms));
        event.SetProps("vm_name", vm.vm_name());
        event.SetProps("vm_runtime_type", RuntimeName(vm.type));
        event.SetProps("vm_attributed_heap_byte_ms",
                       vm.heap_size * weight * duration);
      }
      if (residual_area_valid) {
        const auto duration = std::max<int64_t>(
            0, snapshot.started_at_ms -
                   std::max(interval_start, sampled_residual->exited_at_ms));
        event.SetProps("leak_page_key", sampled_residual->page_key);
        event.SetProps("leak_page_attributed_byte_ms", retained * duration);
      }
      events.push_back(std::move(event));
    }
  }

  // Q5.3/Q10 use a QuickJS-only uniform draw. Q9.2 reuses the event name but
  // emits an independent row for the causative VM.
  const bool sample_quickjs = (snapshot.trigger.reasons & kTimer) &&
                              sampled_quickjs &&
                              sampled_quickjs->slot_tracking_enabled &&
                              sampled_quickjs->has_per_instance_data;
  if (sample_quickjs) {
    auto event = make_event("lynxsdk_memory_vm_quickjs");
    event.SetProps("report_kind", "sample");
    const auto& vm = *sampled_quickjs;
    AddQuickJSVMProps(event, vm, snapshot.started_at_ms);
    event.SetProps("vm_unknown_bytes",
                   static_cast<uint64_t>(
                       vm.memory_slots[fml::JSMemoryTrackSlotType::Unknown]));
    event.SetProps("vm_common_bytes",
                   static_cast<uint64_t>(
                       vm.memory_slots[fml::JSMemoryTrackSlotType::Common]));
    event.SetProps("vm_overflow_bytes",
                   static_cast<uint64_t>(
                       vm.memory_slots[fml::JSMemoryTrackSlotType::Overflow]));
    if (vm.full_gc_at_ms > 0 && !sampled_quickjs_pending_gc &&
        vm.allocated_slots < kPageSlotCapacity && !vm.untracked_page_count &&
        !vm.has_untracked_exited_pages) {
      const double weight = quickjs_count;
      event.SetProps("vm_retained_bytes", sampled_quickjs_retained);
      event.SetProps("vm_retained_bytes_weighted",
                     sampled_quickjs_retained * weight);
      event.SetProps("vm_retained_selection_weight", weight);
    }
    events.push_back(std::move(event));
  }
  if (snapshot.trigger.overflow_vm) {
    auto event = make_event("lynxsdk_memory_vm_quickjs");
    event.SetProps("report_kind", "slot_overflow");
    const auto& vm = *snapshot.trigger.overflow_vm;
    AddQuickJSVMProps(event, vm, snapshot.started_at_ms);
    events.push_back(std::move(event));
  }
  return events;
}

}  // namespace performance
}  // namespace tasm
}  // namespace lynx
