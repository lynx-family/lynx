// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_JSI_QUICKJS_QUICKJS_RUNTIME_WRAPPER_H_
#define CORE_RUNTIME_JS_JSI_QUICKJS_QUICKJS_RUNTIME_WRAPPER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "base/include/fml/time/time_point.h"
#include "base/include/log/logging.h"
#include "base/include/no_destructor.h"
#if ENABLE_TRACE_PERFETTO
#include <vector>

#include "base/include/notification_center.h"
#include "core/runtime/js/jsi/quickjs/quickjs_helper.h"
#endif
#include "base/include/vector.h"
#include "core/runtime/js/jsi/jsi.h"
#include "quickjs/include/quickjs.h"
#ifdef OS_IOS
#include "gc/trace-gc.h"
#else
#include "quickjs/include/trace-gc.h"
#endif

namespace lynx {
namespace tasm {
namespace performance {
struct BtsResidualPage;
}
}  // namespace tasm
namespace runtime {
namespace js {
using LepusIdContainer = std::unordered_map<LEPUSRuntime*, LEPUSClassID>;

class QuickjsRuntimeInstance : public VMInstance, public GCObserver {
 public:
  QuickjsRuntimeInstance();
  virtual ~QuickjsRuntimeInstance();

  void InitQuickjsRuntime(bool is_sync = true, uint32_t runtime_mode = 0,
                          bool force_reference_counting = false);
#ifdef OS_ANDROID
  // Returns a LEPUSPageOutResult value.
  int PageOutGCMemory();
  static const char* DescribePageOutResult(int result);
#endif
  // Runtimes sharing this VM share one suppression flag, so the scopes are
  // counted here rather than per runtime. Must be called on the VM thread.
  void EnterGCPauseSuppressionMode();
  void LeaveGCPauseSuppressionMode();
  inline LEPUSRuntime* Runtime() { return rt_; }
  LEPUSClassID getFunctionId() { return s_function_id_; }
  LEPUSClassID getObjectId() { return s_object_id_; }

  static LEPUSClassID getFunctionId(LEPUSContext* ctx) {
    LEPUSRuntime* rt = LEPUS_GetRuntime(ctx);
    return getFunctionId(rt);
  }

  static LEPUSClassID getFunctionId(LEPUSRuntime* rt) {
    auto it = GetFunctionIdContainer().find(rt);
    DCHECK(it != GetFunctionIdContainer().end());
    if (it != GetFunctionIdContainer().end()) {
      return it->second;
    }
    return 0;
  }

  static LEPUSClassID getObjectId(LEPUSContext* ctx) {
    LEPUSRuntime* rt = LEPUS_GetRuntime(ctx);
    return getObjectId(rt);
  }

  static LEPUSClassID getObjectId(LEPUSRuntime* rt) {
    auto it = GetObjectIdContainer().find(rt);
    if (it != GetObjectIdContainer().end()) {
      return it->second;
    }
    return 0;
  }

  static LepusIdContainer& GetObjectIdContainer();
  static LepusIdContainer& GetFunctionIdContainer();

  /**
   * @brief Called when the garbage collection (GC) operation is completed to
   * handle the post-GC memory information.
   *
   * This method is a concrete implementation of the `GCObserver` interface. It
   * will receive a string containing memory-related information after the
   * garbage collection operation ends.
   *
   * @param mem_info A string containing post-GC memory information. Its format
   * could be JSON or a specific custom format.
   */
  void OnGC(std::string mem_info) override;
  void AddObserver(JSIObserver* obs);
  void RemoveObserver(JSIObserver* obs);

  JSRuntimeType GetRuntimeType() const override {
    return JSRuntimeType::quickjs;
  }
  std::string GetDebugDescription() const override;
  size_t GetHeapSize() const override;
  std::unique_ptr<Buffer> TakeHeapSnapshot(int32_t slot,
                                           const char*& error) override;

  // Must exec in use thread.
  void AddToIdContainer();

  // Caller supplies JSMemoryTrackSlotType::Count entries. The return value
  // describes slot tracking, not heap-query success.
  bool GetMemoryStatus(size_t& heap_size, size_t* memory_size_slots);

  void RebindMemoryTrackSlot();
  int32_t AllocatePageMemorySlot();

  void SetReportingName(const std::string& group_id,
                        const std::string& monitoring_group_label) {
    reporting_name_ =
        monitoring_group_label.empty() ? group_id : monitoring_group_label;
  }

 private:
  LEPUSRuntime* rt_{nullptr};
  uint32_t gc_pause_suppression_count_{0};
  base::LinearFlatSet<JSIObserver*> obs_set_ptr_;
  static LEPUSClassID s_function_id_;
  static LEPUSClassID s_object_id_;

  // Empty for standalone VMs; resolved on the owner JS thread. Match the
  // monitored VM name by preferring the label over the group lookup identity.
  std::string reporting_name_;
  bool per_instance_memory_track_{false};
  bool large_vm_checked_{false};
  // Shared by explicit and trace captures; released before the runtime.
  LEPUSContext* snapshot_context_{nullptr};
  bool capturing_slot_snapshot_{false};
#if ENABLE_TRACE_PERFETTO
  friend class QuickjsResidualSnapshotTest;
  struct ResidualSnapshotState {
    size_t bytes{0};
    uint32_t sequence{0};
    // New page bindings observed by the last successful capture.
    uint32_t post_exit_page_count{0};

    bool ShouldCapture(size_t current_bytes) const {
      if (!sequence) return current_bytes != 0;
      const size_t delta =
          current_bytes > bytes ? current_bytes - bytes : bytes - current_bytes;
      // Exact strict 20% comparison, without overflow or floating-point loss.
      return delta > bytes / 5;
    }
  };
  // Indexed by slot, allocated only when automatic capture is enabled.
  std::vector<ResidualSnapshotState> residual_snapshots_;
  // VM-level types requested during the current coalescing window.
  uint32_t pending_vm_snapshot_types_{0};
  void ScheduleVMSnapshot(JSIContext& context, uint32_t type_index);
  void CaptureVMSnapshots(
      uint32_t types,
      detail::QuickjsHelper::SlotSnapshotFunction take_snapshot =
          js_profile_take_heap_snapshot_for_slot);
  bool CaptureInstanceSnapshot(
      LEPUSContext* ctx, const std::string& identifier, int32_t slot,
      detail::QuickjsHelper::SlotSnapshotFunction take_snapshot =
          js_profile_take_heap_snapshot_for_slot);
  void CaptureResidualSnapshots(
      std::vector<tasm::performance::BtsResidualPage> pages,
      detail::QuickjsHelper::SlotSnapshotFunction take_snapshot =
          js_profile_take_heap_snapshot_for_slot);
  fml::TimePoint last_trace_event_time_;
  fml::TimePoint creation_time_as_unique_;
  std::unique_ptr<base::NotificationCallback> notification_callback_;

  void ReportMemoryForTrace(bool is_full_gc);
#endif
};

}  // namespace js

}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_JS_JSI_QUICKJS_QUICKJS_RUNTIME_WRAPPER_H_
