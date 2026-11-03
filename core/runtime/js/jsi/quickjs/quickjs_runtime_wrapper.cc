// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/jsi/quickjs/quickjs_runtime_wrapper.h"

#include <iterator>
#include <mutex>
#include <string_view>
#include <utility>

#ifdef __cplusplus
extern "C" {
#endif
#include "quickjs/include/cutils.h"
#ifdef __cplusplus
}
#endif

#include "base/include/fml/memory/js_memory_track_scope.h"
#include "base/trace/native/trace_controller.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/js/jsi/quickjs/quickjs_context_wrapper.h"
#include "core/runtime/js/jsi/quickjs/quickjs_helper.h"
#include "core/runtime/js/jsi/quickjs/quickjs_host_function.h"
#include "core/runtime/js/jsi/quickjs/quickjs_host_object.h"
#include "core/runtime/js/runtime_constant.h"
#include "core/runtime/trace/runtime_trace_event_def.h"
#include "core/services/performance/memory_monitor/global_memory_monitor.h"
#include "core/services/performance/memory_monitor/memory_monitor.h"

namespace lynx {

static_assert(fml::JSMemoryTrackSlotType::Count == LEPUS_MEMORY_SIZE_SLOTS);
static_assert(fml::JSMemoryTrackSlotType::Unknown ==
              LEPUS_MEMORY_CATEGORY_UNKNOWN);
static_assert(fml::JSMemoryTrackSlotType::Common ==
              LEPUS_MEMORY_CATEGORY_COMMON);
static_assert(fml::JSMemoryTrackSlotType::Overflow ==
              LEPUS_MEMORY_CATEGORY_SLOT_OVERFLOW);

namespace runtime {
namespace js {
using detail::QuickjsHostFunctionProxy;
using detail::QuickjsHostObjectProxy;

#if ENABLE_TRACE_PERFETTO
namespace {
constexpr struct {
  const char* name;
  int32_t slot;  // -1 selects the whole VM.
} kVMSnapshotTypes[] = {
    {"bts_vm", -1},
    {"bts_unknown", fml::JSMemoryTrackSlotType::Unknown},
    {"bts_common", fml::JSMemoryTrackSlotType::Common},
    {"bts_overflow", fml::JSMemoryTrackSlotType::Overflow},
};
}  // namespace
#endif

LEPUSClassID QuickjsRuntimeInstance::s_function_id_ = 0;
LEPUSClassID QuickjsRuntimeInstance::s_object_id_ = 0;

QuickjsRuntimeInstance::QuickjsRuntimeInstance()
#if ENABLE_TRACE_PERFETTO
    : creation_time_as_unique_(fml::TimePoint::Now()),
      notification_callback_(std::make_unique<base::NotificationCallback>(
          base::NotificationCallback::CallbackList{
              {LYNX_ON_TRACE_BEGIN_NOTIFICATION,
               [this](const std::string& tag, intptr_t data) {
                 if (this->rt_) {
                   LEPUS_SetGCInfoThreshold(
                       this->rt_,
                       lynx::runtime::kMemoryReportDeltaThresholdInTrace);
                 }
               }},
              {kBTSReportMemoryInfo,
               [this](const std::string& tag, intptr_t data) {
                 if (this->rt_) {
                   LEPUS_ReportGCInfo(this->rt_);
                 }
               }},
              {kBTSTakeVMSnapshot,
               [this](const std::string& tag, intptr_t data) {
                 intptr_t* payload = reinterpret_cast<intptr_t*>(data);
                 js::JSIContext* ctx =
                     reinterpret_cast<js::JSIContext*>(payload[0]);
                 if (ctx &&
                     ctx->getVM().get() == static_cast<VMInstance*>(this)) {
                   const std::string_view type =
                       reinterpret_cast<const char*>(payload[1]);
                   if (type == "bts_page") {
                     const auto slot = static_cast<int32_t>(payload[3]);
                     const auto identifier = "bts_instance/" +
                                             std::to_string(payload[2]) +
                                             "/slot_" + std::to_string(slot);
                     auto qjs_ctx = static_cast<QuickjsContextWrapper*>(ctx);
                     CaptureInstanceSnapshot(qjs_ctx->getContext(), identifier,
                                             slot);
                   } else {
                     for (uint32_t i = 0; i < std::size(kVMSnapshotTypes);
                          ++i) {
                       if (type == kVMSnapshotTypes[i].name) {
                         ScheduleVMSnapshot(*ctx, i);
                         return;
                       }
                     }
                     LOGI("Unsupported BTS snapshot type: " << type);
                   }
                 }
               }}}))
#endif
{
}

QuickjsRuntimeInstance::~QuickjsRuntimeInstance() {
  if (tasm::performance::MemoryMonitor::Enable()) {
    tasm::performance::GlobalMemoryMonitor::GetInstance().OnBtsVMDestroy(this);
  }
  if (rt_) {
    LEPUS_SetGCObserver(rt_, nullptr);
#if ENABLE_TRACE_PERFETTO
    if (snapshot_context_) {
      LEPUS_FreeContext(snapshot_context_);
    }
#endif
    LEPUS_FreeRuntime(rt_);
  }
  GetFunctionIdContainer().erase(rt_);
  GetObjectIdContainer().erase(rt_);

  rt_ = nullptr;
#if ENABLE_TRACE_PERFETTO
  ReportMemoryForTrace(false);
#endif
}

LepusIdContainer& QuickjsRuntimeInstance::GetObjectIdContainer() {
  static thread_local LepusIdContainer sObjectIdContainer;
  return sObjectIdContainer;
}

LepusIdContainer& QuickjsRuntimeInstance::GetFunctionIdContainer() {
  static thread_local LepusIdContainer sFunctionIdContainer;
  return sFunctionIdContainer;
}

void QuickjsRuntimeInstance::InitQuickjsRuntime(bool is_sync,
                                                uint32_t runtime_mode,
                                                bool force_reference_counting) {
  LEPUSRuntime* rt;
  bool gc_mode = !force_reference_counting && LEPUS_IsGCModeDefault() &&
                 !tasm::LynxEnv::GetInstance().IsDisableTracingGC();

  per_instance_memory_track_ = tasm::performance::MemoryMonitor::Enable();

  if (per_instance_memory_track_) {
    auto& alloc_slot_variable =
        fml::MessageLoop::GetCurrent().GetTaskRunner()->GetCurrentAllocSlot();
    rt = LEPUS_NewRuntimeWithModeMemoryTrackSlot(runtime_mode,
                                                 &alloc_slot_variable);
    per_instance_memory_track_ = LEPUS_IsMemorySlotTrackingEnabled(rt);
  } else {
    rt = LEPUS_NewRuntimeWithMode(runtime_mode);
  }

  if (!rt) {
    LOGE("init quickjs runtime failed!");
    return;
  }
  LEPUS_SetRuntimeInfo(rt, gc_mode ? "Lynx_JS" : "Lynx_JS_RC");
  rt_ = rt;

  LEPUS_SetGCObserver(rt_, static_cast<GCObserver*>(this));

#if ENABLE_TRACE_PERFETTO
  if (trace::TraceController::Instance()->IsTracingStarted()) {
    LEPUS_SetGCInfoThreshold(rt_,
                             lynx::runtime::kMemoryReportDeltaThresholdInTrace);
  }
#endif

  static std::once_flag s_init_id_flag;
  static LEPUSClassDef s_function_class_def;
  static LEPUSClassExoticMethods s_exotic_method;
  static LEPUSClassDef s_object_class_def;
  std::call_once(s_init_id_flag, [] {
    LEPUS_NewClassID(&s_function_id_);
    LEPUS_NewClassID(&s_object_id_);
    // init function class def
    s_function_class_def.class_name = "LynxFunctionDef";
    s_function_class_def.finalizer = QuickjsHostFunctionProxy::hostFinalizer;
    s_function_class_def.call = QuickjsHostFunctionProxy::FunctionCallback;

    // init exotic method
    s_exotic_method.get_own_property = QuickjsHostObjectProxy::getOwnProperty;
    s_exotic_method.get_own_property_names =
        QuickjsHostObjectProxy::getPropertyNames;
    s_exotic_method.get_property = QuickjsHostObjectProxy::getProperty;
    s_exotic_method.set_property = QuickjsHostObjectProxy::setProperty;

    // init object class def
    s_object_class_def.class_name = "LynxObjectClassDef";
    s_object_class_def.finalizer = QuickjsHostObjectProxy::hostFinalizer;
    s_object_class_def.exotic = &s_exotic_method;
  });

  LEPUS_NewClass(rt_, s_function_id_, &s_function_class_def);
  LEPUS_NewClass(rt_, s_object_id_, &s_object_class_def);

  if (is_sync) {
    AddToIdContainer();
  }
#if LYNX_ENABLE_FROZEN_MODE
  // Due to the fact that QuickJS’s GC traverses all objects in a Stop The World
  // fashion to try to free the circular objects, it causes extra time
  // consumption once QuickJS triggers GC. Currently, the default GC threshold
  // of QuickJS is 256 bytes, and even a slight change in the JS Framework may
  // cause changes in the GC timing. The impact is that in performance
  // degradation tests, some indicators may experience significant fluctuations
  // due to changes in the GC timing. To avoid the GC from fluctuating the
  // performance, when LYNX_ENABLE_FROZEN_MODE is enabled, set the GC of the JS
  // QuickJSRuntime to INT_MAX. In the long run, a reasonable GC threshold needs
  // to be set for QuickJS.
  LEPUS_SetGCThreshold(rt_, INT_MAX);
#endif
  LOGI("lynx InitQuickjsRuntime success");
}

void QuickjsRuntimeInstance::OnGC(std::string mem_info) {
  [[maybe_unused]] bool is_full_gc =
      mem_info.find("is_full_gc") != std::string::npos;
#if ENABLE_TRACE_PERFETTO
  ReportMemoryForTrace(is_full_gc);
#endif

  const auto heap_size = LEPUS_GetHeapSize(rt_);
  const bool monitoring = tasm::performance::MemoryMonitor::Enable();
  const bool report_large_vm =
      !large_vm_checked_ &&
      tasm::performance::MemoryMonitor::ReachesThreshold(
          heap_size, tasm::performance::MemoryMonitor::GetSettings()
                         .global.large_vm_threshold_mb);
  // Consume even when this VM has no shared group. Standalone VMs are not
  // registered solely for high-memory reporting.
  if (report_large_vm) large_vm_checked_ = true;
  if (monitoring) {
#if ENABLE_TRACE_PERFETTO
    bool capture_residual = false;
    if (is_full_gc && per_instance_memory_track_ && !capturing_slot_snapshot_) {
      capture_residual =
          trace::TraceController::Instance()->GetMemoryTraceConfig() != nullptr;
    }
#endif
    // Copy on the owner JS thread. Only a completed full GC can qualify
    // exited-page residuals; scheduled reporting never forces collection.
    tasm::performance::BtsMemorySample sample;
    // Timestamp before the dump so an exit during collection cannot qualify.
    sample.measured_at_ms = tasm::performance::MemoryNowMs();
    sample.is_full_gc = is_full_gc;
    sample.slots_valid = GetMemoryStatus(sample.heap_size, sample.slots.data());
    sample.heap_valid = true;
    sample.slots_valid &= sample.heap_valid;
#if ENABLE_TRACE_PERFETTO
    const auto measured_at_ms = sample.measured_at_ms;
    capture_residual &= sample.slots_valid;
#endif
    tasm::performance::GlobalMemoryMonitor::GetInstance().OnBtsVMMemoryUpdate(
        this, std::move(sample), report_large_vm);
#if ENABLE_TRACE_PERFETTO
    if (capture_residual) {
      tasm::performance::GlobalMemoryMonitor::GetInstance()
          .RequestBtsResidualPagesForTrace(
              this, measured_at_ms, [](auto& vm, auto pages) {
                static_cast<QuickjsRuntimeInstance&>(vm)
                    .CaptureResidualSnapshots(std::move(pages));
              });
    }
#endif
  } else if (report_large_vm && !reporting_name_.empty()) {
    tasm::performance::GlobalMemoryMonitor::GetInstance().OnLargeVM(
        reporting_name_, heap_size);
  }

  for (auto* observer : obs_set_ptr_) {
    observer->OnRuntimeGC({{kRawRuntimeHeapSize, std::to_string(heap_size)}});
  }
}

#if ENABLE_TRACE_PERFETTO
void QuickjsRuntimeInstance::ScheduleVMSnapshot(JSIContext& context,
                                                uint32_t type_index) {
  const auto pending = pending_vm_snapshot_types_;
  pending_vm_snapshot_types_ |= 1u << type_index;
  if (pending) return;

  // All matching Shells enqueue on this VM's owner thread. Defer capture to
  // coalesce their requests without retaining the requesting page context.
  auto* controller = trace::TraceController::Instance();
  auto config = controller->GetLastSessionTraceConfig();
  fml::MessageLoop::GetCurrent().GetTaskRunner()->PostDelayedTask(
      [weak = std::weak_ptr<VMInstance>(context.getVM()), config]() {
        auto strong = weak.lock();
        if (!strong) return;
        auto& vm = static_cast<QuickjsRuntimeInstance&>(*strong);
        const auto types = std::exchange(vm.pending_vm_snapshot_types_, 0);
        auto* controller = trace::TraceController::Instance();
        if (!controller->IsTracingStarted() ||
            controller->GetLastSessionTraceConfig() != config) {
          return;
        }
        vm.CaptureVMSnapshots(types);
      },
      fml::TimeDelta::FromMilliseconds(100));
}

void QuickjsRuntimeInstance::CaptureVMSnapshots(
    uint32_t types, detail::QuickjsHelper::SlotSnapshotFunction take_snapshot) {
  if (!rt_ || !types || capturing_slot_snapshot_) return;
  capturing_slot_snapshot_ = true;
  for (uint32_t i = 0; i < std::size(kVMSnapshotTypes); ++i) {
    if (!(types & (1u << i))) continue;
    const auto& type = kVMSnapshotTypes[i];
    if (type.slot >= 0 && !per_instance_memory_track_) {
      LOGI("Cannot take " << type.name << " snapshot: tracking unavailable");
      continue;
    }
    if (!snapshot_context_) {
      fml::JSMemoryTrackAsCommon scope;
      snapshot_context_ = LEPUS_NewContext(rt_);
      if (!snapshot_context_) break;
    }
    const auto identifier =
        std::string(type.name) + "/" +
        std::to_string(creation_time_as_unique_.ToEpochDelta().ToNanoseconds());
    const bool captured =
        type.slot < 0
            ? detail::QuickjsHelper::TakeHeapSnapshotAndWriteToTrace(
                  snapshot_context_, identifier)
            : detail::QuickjsHelper::TakeHeapSnapshotForSlotAndWriteToTrace(
                  snapshot_context_, identifier, type.slot, nullptr,
                  take_snapshot);
    if (captured) {
      base::NotificationCallback::Notify(kVMSnapshotCaptured, 0);
    }
  }
  capturing_slot_snapshot_ = false;
}

bool QuickjsRuntimeInstance::CaptureInstanceSnapshot(
    LEPUSContext* ctx, const std::string& identifier, int32_t slot,
    detail::QuickjsHelper::SlotSnapshotFunction take_snapshot) {
  if (!rt_ || !per_instance_memory_track_ || capturing_slot_snapshot_ ||
      !fml::IsValidInstanceSlot(slot)) {
    LOGI("Cannot take instance slot snapshot of '"
         << identifier << "': tracking unavailable or capture in progress");
    return false;
  }
  capturing_slot_snapshot_ = true;
  const bool captured =
      detail::QuickjsHelper::TakeHeapSnapshotForSlotAndWriteToTrace(
          ctx, identifier, slot, nullptr, take_snapshot);
  capturing_slot_snapshot_ = false;
  if (captured) {
    base::NotificationCallback::Notify(kVMSnapshotCaptured, 0);
  }
  return captured;
}

void QuickjsRuntimeInstance::CaptureResidualSnapshots(
    std::vector<tasm::performance::BtsResidualPage> pages,
    detail::QuickjsHelper::SlotSnapshotFunction take_snapshot) {
  if (!rt_ || pages.empty() || capturing_slot_snapshot_) return;
  // Context creation and snapshots may notify OnGC. Keep
  // telemetry updates, but never recursively schedule automatic snapshots.
  capturing_slot_snapshot_ = true;
  for (const auto& page : pages) {
    DCHECK(fml::IsValidInstanceSlot(page.slot));
    size_t heap_size = 0;
    size_t slots[LEPUS_MEMORY_SIZE_SLOTS]{};
    if (!GetMemoryStatus(heap_size, slots)) break;
    const size_t bytes = slots[page.slot];
    if (residual_snapshots_.size() <= static_cast<size_t>(page.slot)) {
      residual_snapshots_.resize(page.slot + 1);
    }
    auto& previous = residual_snapshots_[page.slot];
    const bool capture_for_change = previous.ShouldCapture(bytes);
    // Capture once for each of the first two new page bindings after exit.
    // Any successful capture consumes that page count; failures may retry.
    const bool capture_after_page_load =
        bytes && page.post_exit_page_count <= 2 &&
        page.post_exit_page_count > previous.post_exit_page_count;
    if (!capture_for_change && !capture_after_page_load) continue;
    if (!snapshot_context_) {
      fml::JSMemoryTrackAsCommon scope;
      snapshot_context_ = LEPUS_NewContext(rt_);
      if (!snapshot_context_) break;
    }
    const char* reason = !previous.sequence ? "first_post_exit_full_gc"
                         : !bytes           ? "residual_cleared"
                         : !previous.bytes  ? "residual_reappeared"
                         : capture_for_change
                             ? "residual_change_exceeds_20_percent"
                             : "post_exit_page_count_within_two";
    detail::QuickjsHelper::ResidualSnapshotInfo info{};
    info.post_exit_page_count = page.post_exit_page_count;
    info.previous_bytes = previous.bytes;
    info.trigger_bytes = bytes;
    info.reason = reason;
    const auto identifier = "bts_residual/" + std::to_string(page.instance_id) +
                            "/slot_" + std::to_string(page.slot);
    if (detail::QuickjsHelper::TakeHeapSnapshotForSlotAndWriteToTrace(
            snapshot_context_, identifier, page.slot, &info, take_snapshot)) {
      previous.bytes = info.retained_bytes;
      previous.post_exit_page_count = page.post_exit_page_count;
      ++previous.sequence;
    }
  }
  capturing_slot_snapshot_ = false;
}

void QuickjsRuntimeInstance::ReportMemoryForTrace(bool is_full_gc) {
  if (!trace::TraceController::Instance()->GetMemoryTraceConfig()) {
    return;
  }

  if (is_full_gc) {
    TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY, VM_GC_EVENT, "ptr",
                        static_cast<VMInstance*>(this));
  }

  // When Lynx Trace is enabled, memory data is reported directly on the JS
  // thread. Otherwise, multithreading may cause the time sequence of events to
  // be disordered, hindering automated analysis.
  auto now = fml::TimePoint::Now();
  if (rt_ && ((now - last_trace_event_time_).ToMilliseconds() < 16)) {
    return;
  }
  last_trace_event_time_ = now;
  std::string track_name =
      "bts_vm_acc_" +
      std::to_string(creation_time_as_unique_.ToEpochDelta().ToNanoseconds());

  auto usage = detail::QuickjsHelper::GetMemoryUsage(rt_);
  TRACE_COUNTER(
      LYNX_TRACE_CATEGORY, track_name.c_str(), usage.heap_size,
      [&](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations(kRawRuntimeBaseMemoryInfo,
                                           std::to_string(usage.base_size));
        ctx.event()->add_debug_annotations(kRawRuntimePageRssMemoryInfo,
                                           std::to_string(usage.page_rss_size));
        ctx.event()->add_debug_annotations("ptr",
                                           static_cast<VMInstance*>(this));

        if (rt_ && per_instance_memory_track_) {
          size_t memory_size_slots[LEPUS_MEMORY_SIZE_SLOTS];
          auto max_slot = LEPUS_DumpMemorySlots(rt_, memory_size_slots);
          for (int i = 0; i <= max_slot; i++) {
            ctx.event()->add_debug_annotations(
                "slot_" + std::to_string(i),
                std::to_string(memory_size_slots[i]));
          }
        }
      });
}
#endif

void QuickjsRuntimeInstance::AddObserver(JSIObserver* obs) {
  if (!obs) {
    return;
  }
  obs_set_ptr_.emplace(obs);
}

void QuickjsRuntimeInstance::RemoveObserver(JSIObserver* obs) {
  if (!obs) {
    return;
  }
  obs_set_ptr_.erase(obs);
#ifdef DEBUG
  // In debug mode, when the page exits and its BTS runtime is destroyed,
  // check if any memory allocations with unknown ownership were generated.
  // All unclaimed memory should be eliminated during the development and
  // debugging phase.
  if (per_instance_memory_track_) {
    size_t memory_size_slots[LEPUS_MEMORY_SIZE_SLOTS];
    LEPUS_DumpMemorySlots(rt_, memory_size_slots);
    assert(memory_size_slots[LEPUS_MEMORY_CATEGORY_UNKNOWN] == 0);
  }
#endif
}

bool QuickjsRuntimeInstance::GetMemoryStatus(size_t& heap_size,
                                             size_t* memory_size_slots) {
  heap_size = 0;
  memset(memory_size_slots, 0,
         sizeof(memory_size_slots[0]) * LEPUS_MEMORY_SIZE_SLOTS);
  if (rt_) {
    if (per_instance_memory_track_) {
      LEPUS_DumpMemorySlots(rt_, memory_size_slots);
    }
    heap_size = LEPUS_GetHeapSize(rt_);
  }
  return per_instance_memory_track_;
}

void QuickjsRuntimeInstance::RebindMemoryTrackSlot() {
  // Invoked on target JS thread and rebind.
  auto& alloc_slot_variable =
      fml::MessageLoop::GetCurrent().GetTaskRunner()->GetCurrentAllocSlot();
  LEPUS_RebindRuntimeMemoryTrackSlot(rt_, &alloc_slot_variable);
}

int32_t QuickjsRuntimeInstance::AllocatePageMemorySlot() {
  if (per_instance_memory_track_) {
    auto slot = LEPUS_AllocateMemorySlot(rt_);
    tasm::performance::GlobalMemoryMonitor::GetInstance().OnBtsVMSlotAllocate(
        this, slot);
    if (slot == -1) {
      // All slots are occupied. Previous pages continue to be counted, but new
      // pages are no longer counted. Memory usage is uniformly calculated on
      // Overflow-Slot.
      slot = fml::JSMemoryTrackSlotType::Overflow;
    }
    return slot;
  }
  return fml::JSMemoryTrackSlotType::Unknown;
}

std::string QuickjsRuntimeInstance::GetDebugDescription() const {
  return detail::QuickjsHelper::GetDebugDescription(rt_);
}

size_t QuickjsRuntimeInstance::GetHeapSize() const {
  if (rt_) {
    return LEPUS_GetHeapSize(rt_);
  }
  return 0;
}

void QuickjsRuntimeInstance::AddToIdContainer() {
  GetFunctionIdContainer().insert({rt_, s_function_id_});
  GetObjectIdContainer().insert({rt_, s_object_id_});
}

}  // namespace js

}  // namespace runtime
}  // namespace lynx
