// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_SERVICES_LONG_TASK_TIMING_LONG_TASK_MONITOR_H_
#define CORE_SERVICES_LONG_TASK_TIMING_LONG_TASK_MONITOR_H_

#include <stdlib.h>

#include <atomic>
#include <memory>
#include <optional>
#include <string>

#include "base/include/vector.h"
#include "core/services/long_task_timing/long_batched_tasks_monitor.h"
#include "core/services/long_task_timing/long_task_timing.h"

namespace lynx {
namespace tasm {
namespace timing {
/**
 const var of task type.
 */
static constexpr char kLoadTemplateTask[] = "load_template_task";
static constexpr char kUIOperationFlushTask[] = "ui_operation_flush_task";
static constexpr char kAnimationTask[] = "animation_task";
static constexpr char kListNodeTask[] = "list_node_task";
static constexpr char kUpdateDataByNativeTask[] = "update_data_by_native_task";
static constexpr char kUpdateDataByJSTask[] = "update_data_by_js_task";
static constexpr char kNativeFuncTask[] = "native_func_task";
static constexpr char kLoadJSTask[] = "load_js_task";
static constexpr char kJSFuncTask[] = "js_func_task";
static constexpr char kTimerTask[] = "timer_task";
/**
 const var of task name.
 */
static constexpr char kTaskNameLynxEngineLoadTemplate[] =
    "LynxEngine::LoadTemplate";
static constexpr char kTaskNameLynxEngineLoadTemplateBundle[] =
    "LynxEngine::LoadTemplateBundle";
static constexpr char kTaskNameLynxEngineReloadTemplate[] =
    "LynxEngine::ReloadTemplate";
static constexpr char kTaskNameLynxEngineUpdateGlobalProps[] =
    "LynxEngine::UpdateGlobalProps";
static constexpr char kTaskNameLynxEngineUpdateDataByParsedData[] =
    "LynxEngine::UpdateDataByParsedData";
static constexpr char kTaskNameLynxEngineResetDataByParsedData[] =
    "LynxEngine::ResetDataByParsedData";
static constexpr char kTaskNameLynxEngineUpdateDataByJS[] =
    "LynxEngine::UpdateDataByJS";
static constexpr char kTaskNameLynxEngineUpdateBatchedDataByJS[] =
    "LynxEngine::UpdateBatchedDataByJS";
static constexpr char kTaskNameLynxEngineUpdateComponentData[] =
    "LynxEngine::UpdateComponentData";
static constexpr char kTaskNameLynxEngineCallLepusMethod[] =
    "LynxEngine::CallLepusMethod";
static constexpr char kTaskNameLynxEngineDidLoadComponent[] =
    "LynxEngine::DidLoadComponent";
static constexpr char kTaskNameJSEventListenerInvoke[] =
    "JSClosureEventListener::Invoke";
static constexpr char kTaskNameJSAppUpdateData[] = "JSApp::Get::UpdateData";
static constexpr char kTaskNameJSAppBatchedUpdateData[] =
    "JSApp::Get::BatchedUpdateData";
static constexpr char kTaskNameJSAppUpdateComponentData[] =
    "JSApp::Get::UpdateComponentData";
static constexpr char kTaskNameJSAppCallLepusMethod[] =
    "JSApp::Get::CallLepusMethod";
static constexpr char kTaskNameJSAppSendPageEvent[] = "App::SendPageEvent";
static constexpr char kTaskNameJSAppPublishComponentEvent[] =
    "App::PublishComponentEvent";
static constexpr char kTaskNameJSProxyCallJSFunction[] =
    "JSProxy::CallJSFunction";
static constexpr char kTaskNameAnimationVSyncTickAllElement[] =
    "ElementVsyncProxy::TickAllElement";
static constexpr char kTaskNameListElementComponentAtIndex[] =
    "ListElement::ComponentAtIndex";
static constexpr char kTaskNameRadonDiffListNode2ComponentAtIndex[] =
    "RadonDiffListNode2::ComponentAtIndex";
static constexpr char kTaskNameRadonListBaseRenderAtIndex[] =
    "RadonListBase::RenderComponentAtIndex";
static constexpr char kTaskNameRadonListBaseUpdateComponent[] =
    "RadonListBase::UpdateComponent";
static constexpr char kTaskNameHandlePendingLayoutTask[] =
    "LayoutMediator::HandlePendingLayoutTask";
static constexpr char kTaskNameLepusLynxSetTimeout[] = "LepusLynx::SetTimeout";
static constexpr char kTaskNameLepusLynxSetInterval[] =
    "LepusLynx::SetInterval";
static constexpr char kTaskNameJsTaskAdapterSetTimeout[] =
    "JsTaskAdapter::SetTimeout";
static constexpr char kTaskNameJsTaskAdapterSetInterval[] =
    "JsTaskAdapter::SetInterval";
static constexpr char kTaskNameJsTaskAdapterQueueMicrotask[] =
    "JsTaskAdapter::QueueMicrotask";
static constexpr char kTaskNameLynxUIOperationAsyncQueueFlush[] =
    "LynxUIOperationAsyncQueue::FlushInterval";
static constexpr char kTaskNameLynxUIOperationQueueConsumeOperations[] =
    "LynxUIOperationQueue::ConsumeOperations";

/**
 * @brief Monitors and tracks the execution of long-running tasks.
 *
 * The `LongTaskMonitor` class is responsible for tracking the start and end
 * times of long-running tasks, as well as recording metadata about those tasks.
 * It maintains a stack of `LongTaskTiming` objects that store the timing
 * information for each task.
 */
class LongTaskMonitor {
 public:
  class Scope {
   public:
    explicit Scope(int32_t instance_id, const std::string& type = "",
                   const std::string& name = "",
                   const std::string& task_info = "") {
      LongTaskMonitor::Instance()->WillProcessTask(type, name, task_info,
                                                   instance_id);
    }

    ~Scope() { LongTaskMonitor::Instance()->DidProcessTask(); }

    Scope(const Scope& s) = delete;
    Scope& operator=(const Scope&) = delete;
    Scope(Scope&&) = delete;
    Scope& operator=(Scope&&) = delete;
  };

  /**
   * @brief Called when a task is about to be processed.
   *
   * This method should be called immediately before a long-running task is
   * executed. It records the start time and other metadata about the task.
   *
   * @param type The type or category of the task (e.g.,
   * "update_data_by_js_task", "update_data_by_native_task").
   * @param name The name or description of the specific task (e.g.,
   * "xxx/MyComponent").
   * @param task_info The detailed information of the task. This is an
   * optional field used to locate additional information about the task. For
   * example, for the "update_data_by_js_task" type, the "updated component
   * name" and "updated keys" can basically locate the specific business
   * function, so "updated keys" is a suitable detail information for the task.
   * @param instance_id The instance ID of template. If `instance_id_` is equal
   * to -2, it means the instance ID has not been set yet. In this case, the
   * event can be automatically associated by calling `EventTracker::OnEvent`.
   *   If `instance_id_` is greater than or equal to 0, it means the instance ID
   * has been set. In this case, the event can be directly reported using the
   * `instance_id_`.
   */
  void WillProcessTask(const std::string& type = "",
                       const std::string& name = "",
                       const std::string& task_info = "",
                       int32_t instance_id = report::kUninitializedInstanceId);
  /**
   * @brief Called when a task has completed processing.
   *
   * This method should be called immediately after a long-running task has
   * finished executing. It records the end time of the task and updates any
   * relevant metrics or statistics.
   */
  void DidProcessTask();
  /**
   * @brief Get a pointer to the top element of the timing stack.
   * If the stack is empty, returns nullptr.
   * @return LongTaskTiming* A pointer to the top element of the timing stack,
   * or nullptr if the stack is empty.
   */
  LongTaskTiming* GetTopTimingPtr();

  static LongTaskMonitor* Instance();

 private:
  LongTaskMonitor();
  LongTaskMonitor(const LongTaskMonitor& timing) = delete;
  LongTaskMonitor& operator=(const LongTaskMonitor&) = delete;
  LongTaskMonitor(LongTaskMonitor&&) = delete;
  LongTaskMonitor& operator=(LongTaskMonitor&&) = delete;

  base::InlineStack<LongTaskTiming, 16> timing_stack_;
  bool enable_;
  // TODO(limeng.amer): get value from LynxEnv;
  double duration_threshold_ms_;
  std::string thread_name_;
  LongBatchedTasksMonitor long_batched_tasks_monitor_;
};

}  // namespace timing
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_LONG_TASK_TIMING_LONG_TASK_MONITOR_H_
