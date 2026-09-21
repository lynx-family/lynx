// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/js/runtime_manager.h"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "base/include/fml/message_loop.h"
#include "base/include/log/logging.h"
#include "base/include/no_destructor.h"
#include "base/trace/native/trace_event.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/base/trace/trace_event_def.h"
#include "core/renderer/tasm/config.h"
#include "core/runtime/js/bindings/global.h"
#include "core/runtime/js/js_execution_control.h"
#include "core/runtime/js/js_executor.h"
#include "core/runtime/js/jsi/jsi.h"
#include "core/runtime/js/runtime_constant.h"
#include "core/runtime/trace/runtime_trace_event_def.h"

#ifndef JS_ENGINE_TYPE
// Default set JS_ENGINE_TYPE if not provided.
#if defined(OS_IOS) || defined(OS_OSX) || defined(OS_TVOS)
#define JS_ENGINE_TYPE 1
#else
#define JS_ENGINE_TYPE 2
#endif
#endif  // JS_ENGINE_TYPE

#if JS_ENGINE_TYPE == 0
#include "core/runtime/js/jsi/v8/v8_api.h"
#elif JS_ENGINE_TYPE == 1 || JS_ENGINE_TYPE == 2
#include "core/runtime/js/jsi/quickjs/quickjs_api.h"
#endif  // JS_ENGINE_TYPE
#if JS_ENGINE_TYPE == 1
#include "core/runtime/js/jsi/jsc/jsc_api.h"
#endif  // JS_ENGINE_TYPE == 1
#if OS_HARMONY
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/common/napi/napi_runtime_proxy_jsvm.h"
#include "core/runtime/common/napi/napi_runtime_proxy_jsvm_factory.h"
#include "core/runtime/js/jsi/jsvm/jsvm_api.h"

extern void RegisterJSVMRuntimeProxyFactory(
    lynx::runtime::js::NapiRuntimeProxyJSVMFactory* factory);
#endif  // OS_HARMONY

#ifdef OS_ANDROID
#include "core/runtime/js/bindings/modules/android/lynx_proxy_runtime_helper.h"
#include "core/runtime/profile/v8/v8_runtime_profiler.h"
#endif

#if defined(OS_WIN) || defined(OS_OSX) || defined(OS_LINUX)
#if ENABLE_NAPI_BINDING
#include "core/runtime/common/napi/napi_runtime_proxy_v8.h"

extern void RegisterV8RuntimeProxyFactory(
    lynx::runtime::js::NapiRuntimeProxyV8Factory*);
#endif  // ENABLE_NAPI_BINDING
#endif  // OS_WIN || OS_OSX || OS_LINUX

namespace lynx {
namespace runtime {

namespace {

#if JS_ENGINE_TYPE == 1 || JS_ENGINE_TYPE == 2

static constexpr int kMaxVMSize = 1;

// Currently only quickjs use this.
class VMInstancePool {
 public:
  static VMInstancePool& Instance();
  std::shared_ptr<runtime::js::VMInstance> TakeVMInstance(
      runtime::js::JSRuntimeType runtime_type);

#if ENABLE_TRACE_PERFETTO
  VMInstancePool()
      : report_pool_state_(
            LYNX_ON_TRACE_BEGIN_NOTIFICATION,
            [&](const std::string& tag, intptr_t data) { ReportPoolState(); }) {
  }
  void ReportPoolState() {
    TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY, BTS_VM_POOL_STATE_EVENT,
                        [&](lynx::perfetto::EventContext ctx) {
                          int index = 0;
                          for (auto&& [type, instance_vec] : vm_instances_) {
                            for (auto&& vm_inst : instance_vec) {
                              ctx.event()->add_debug_annotations(
                                  std::string("id_") + std::to_string(index++),
                                  vm_inst->GetDebugDescription());
                            }
                          }
                        });
  }
  base::NotificationCallback report_pool_state_;
#endif

 private:
  void CreateVMInstanceAsync(runtime::js::JSRuntimeType runtime_type);
  std::shared_ptr<runtime::js::VMInstance> DoCreateVMInstance(
      runtime::js::JSRuntimeType runtime_type);
  std::mutex mtx_;
  std::unordered_map<runtime::js::JSRuntimeType,
                     std::vector<std::shared_ptr<runtime::js::VMInstance>>>
      vm_instances_;
};

VMInstancePool& VMInstancePool::Instance() {
  static base::NoDestructor<VMInstancePool> pool_;
  return *pool_;
}

// Currently only support quickjs.
// Maybe null
std::shared_ptr<runtime::js::VMInstance> VMInstancePool::TakeVMInstance(
    runtime::js::JSRuntimeType runtime_type) {
  std::shared_ptr<runtime::js::VMInstance> vm_instance = nullptr;
  if (runtime_type != runtime::js::JSRuntimeType::quickjs) {
    return vm_instance;
  }
  {
    std::unique_lock<std::mutex> lock(mtx_, std::try_to_lock);
    if (lock.owns_lock()) {
      auto it = vm_instances_.find(runtime_type);
      if (it != vm_instances_.end() && !it->second.empty()) {
        vm_instance.swap(it->second.back());
        it->second.pop_back();
      }
    }
  }

  runtime::js::BindQuickjsVMToCurrentThread(vm_instance);

  // pre create next vm instance.
  CreateVMInstanceAsync(runtime_type);
  return vm_instance;
}

void VMInstancePool::CreateVMInstanceAsync(
    runtime::js::JSRuntimeType runtime_type) {
  base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
      [runtime_type, this]() mutable {
        std::lock_guard<std::mutex> lock{mtx_};
        if (vm_instances_[runtime_type].size() >= kMaxVMSize) {
          return;
        }
        for (auto i = vm_instances_[runtime_type].size(); i < kMaxVMSize; i++) {
          auto ret = DoCreateVMInstance(runtime_type);
          vm_instances_[runtime_type].emplace_back(std::move(ret));
        }

#if ENABLE_TRACE_PERFETTO
        ReportPoolState();
#endif
      },
      base::ConcurrentTaskType::NORMAL_PRIORITY);
}

std::shared_ptr<runtime::js::VMInstance> VMInstancePool::DoCreateVMInstance(
    runtime::js::JSRuntimeType runtime_type) {
  if (runtime_type == runtime::js::JSRuntimeType::quickjs) {
    return runtime::js::CreateQuickJsVM(nullptr, false);
  }
  // Current don't support other engine.
  return nullptr;
}

#endif  // JS_ENGINE_TYPE

// Log tag for the new "shared Isolate/VM + per-page isolated Context" scheme.
constexpr const char* kNewShareGroupTag = "new_share_group:";

void AlignRuntimeEngineWithVM(
    const std::shared_ptr<runtime::js::VMInstance>& vm,
    bool& force_use_lightweight_js_engine, const char* log_prefix) {
  if (!vm) {
    return;
  }

  const auto type = vm->GetRuntimeType();
  const bool use_non_quickjs_engine = type == runtime::js::JSRuntimeType::v8 ||
                                      type == runtime::js::JSRuntimeType::jsc ||
                                      type == runtime::js::JSRuntimeType::jsvm;
  if (use_non_quickjs_engine) {
    if (force_use_lightweight_js_engine) {
      LOGI(log_prefix << " with v8, jsc or jsvm, change "
                      << "force_use_lightweight_js_engine to false");
      force_use_lightweight_js_engine = false;
    } else {
      LOGI(log_prefix << " with v8, jsc or jsvm");
    }
    return;
  }

  if (!force_use_lightweight_js_engine) {
    LOGI(log_prefix << " with none-v8, none-jsc and none-jsvm, change "
                    << "force_use_lightweight_js_engine to true");
    force_use_lightweight_js_engine = true;
  } else {
    LOGI(log_prefix << " with none-v8, none-jsc and none-jsvm");
  }
}

void RegisterVMForTraceAndMonitor(
    const std::shared_ptr<runtime::js::VMInstance>& vm,
    const std::string& group_id, int32_t instance_id) {
  TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY, LYNX_PAGE_USES_BTS_VM, "group_id",
                      group_id, "instance_id", instance_id, "desc",
                      vm->GetDebugDescription(), "ptr", vm.get());
}

}  // namespace

RuntimeManager* RuntimeManager::Instance() {
  static thread_local RuntimeManager instance_;
  return &instance_;
}

RuntimeManager::NewShareGroupPageReleaseObserver::
    NewShareGroupPageReleaseObserver(RuntimeManager* manager)
    : manager_(manager) {}

void RuntimeManager::NewShareGroupPageReleaseObserver::OnRelease(
    const std::string& group_id) {
  manager_->OnNewShareGroupPageRelease(group_id);
}

RuntimeManager::RuntimeManager()
    : memory_pressure_callback_(base::NotificationCallback::CallbackList{
          {base::MEMORY_PRESSURE_NOTIFICATION,
           [this](const std::string& tag, intptr_t data) {
             OnMemoryPressure(static_cast<base::MemoryPressureLevel>(data));
           }}
#if ENABLE_TRACE_PERFETTO
          ,
          {kScheduleVMSnapshot,
           [this](const std::string& tag, intptr_t data) {
             const char* group_id = reinterpret_cast<const char*>(data);
             if (group_id != nullptr) {
               ScheduleVMSnapshot(group_id);
             }
           }}
#endif
      }) {
#if ENABLE_TRACE_PERFETTO
  pending_vm_snapshot_tasks_ =
      std::make_unique<std::unordered_map<std::string, uint64_t>>();
#endif
}

RuntimeManager::~RuntimeManager() {
  for (const auto& [type, vm] : mVMContainer_) {
    if (type == runtime::js::JSRuntimeType::v8) {
      UnregisterVMInstance(vm.get());
    }
  }
  // Should destroy runtime_manager_delegate_ before mVMContainer_
  runtime_manager_delegate_.reset();
}

bool RuntimeManager::IsSingleJSContext(const std::string& group_id) {
  return group_id == "-1";
}

base::UnsafeOwningPtr<runtime::js::Runtime>
RuntimeManager::CreateNewShareGroupJSRuntime(
    base::MoveOnlyClosure<std::vector<
        std::pair<std::string, std::shared_ptr<runtime::js::Buffer>>>>&
        js_pre_sources_getter,
    bool force_use_lightweight_js_engine, bool ensure_console,
    runtime::js::JSExecutor& executor,
    const runtime::js::JSRuntimeExternalParams& create_params,
    const tasm::PageOptions& page_options) {
  const auto& group_id = create_params.group_id;
  auto* global_wrapper = EnsureNewShareGroupGlobalContext(
      force_use_lightweight_js_engine, create_params, page_options,
      js_pre_sources_getter, executor);

  auto vm = global_wrapper->GetVM();
  // The page runtime must use the same engine type as the shared VM, matching
  // the legacy shared-context reuse rule.
  AlignRuntimeEngineWithVM(vm, force_use_lightweight_js_engine,
                           "use new share group");

  auto page_runtime = CreateRuntime(force_use_lightweight_js_engine, true,
                                    create_params, page_options);
  page_runtime->setCreatedType(runtime::js::JSRuntimeCreatedType::context);
  auto page_context = page_runtime->createContext(vm);

  RegisterVMForTraceAndMonitor(page_context->getVM(), group_id,
                               page_options.GetInstanceID());

  EnsureConsolePostMan(page_context, executor, force_use_lightweight_js_engine,
                       page_options);
  page_runtime->InitRuntime(page_context);

  auto page_wrapper = std::make_shared<NewShareGroupPageContextWrapper>(
      page_context, group_id, &new_share_group_page_release_observer_);
  page_context->SetReleaseObserver(page_wrapper);
  global_wrapper->IncLivePageCount();
  std::shared_ptr<runtime::js::ConsoleMessagePostMan> page_post_man =
      page_context->GetPostMan();
  page_wrapper->initGlobal(page_runtime, page_post_man, page_options);
  if (ensure_console) {
    page_wrapper->EnsureConsole(page_post_man, page_options);
  }

  global_wrapper->CopyGlobalsTo(*page_runtime);

  // Each page has its own runtime, so the devtool delegate needs to be
  // notified per page just like the legacy shared-context reuse path.
  if (IsInspectEnabled(force_use_lightweight_js_engine, page_options)) {
    runtime_manager_delegate_->OnRuntimeReady(executor, *page_runtime,
                                              group_id);
  }

#if ENABLE_TRACE_PERFETTO
  CheckAutotakeSnapshot(group_id);
#endif

  return page_runtime;
}

base::UnsafeOwningPtr<runtime::js::Runtime> RuntimeManager::CreateJSRuntime(
    base::MoveOnlyClosure<std::vector<
        std::pair<std::string, std::shared_ptr<runtime::js::Buffer>>>>
        js_pre_sources_getter,
    bool force_use_lightweight_js_engine, bool ensure_console,
    runtime::js::JSExecutor& executor,
    const runtime::js::JSRuntimeExternalParams& create_params,
    const tasm::PageOptions& page_options) {
  const auto& group_id = create_params.group_id;
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, RUNTIME_MANAGER_CREATE_JS_RUNTIME,
              "group_id", group_id);
  // call inspect's prepare
  if (IsInspectEnabled(force_use_lightweight_js_engine, page_options)) {
    runtime_manager_delegate_->BeforeRuntimeCreate(
        force_use_lightweight_js_engine);
  }
  const bool is_single_context = IsSingleJSContext(group_id);
  // New "shared Isolate/VM + per-page isolated Context" scheme. Only reachable
  // when the LynxGroup opts in AND this is a shared (non "-1") group. Falls
  // through to the legacy path otherwise so existing behavior is untouched.
  if (create_params.enable_new_share_group && !is_single_context) {
    return CreateNewShareGroupJSRuntime(
        js_pre_sources_getter, force_use_lightweight_js_engine, ensure_console,
        executor, create_params, page_options);
  }
  base::UnsafeOwningPtr<runtime::js::Runtime> js_runtime;
  std::shared_ptr<runtime::js::JSIContext> js_context;
  // This variable indicates 'false' only when it has been created previously
  // and the context is being shared.
  bool need_create_context_wrapper = true;
  if (is_single_context) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS,
                RUNTIME_MANAGER_CREATE_SINGLE_CONTEXT_RUNTIME);
    js_runtime = CreateRuntime(force_use_lightweight_js_engine, false,
                               create_params, page_options);
    js_context = CreateJSIContext(*js_runtime, create_params);
    LOGI("create single_context:" << js_context.get());
  } else {
    TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS,
                RUNTIME_MANAGER_GET_SHARED_JS_CONTEXT);
    js_context = GetSharedJSContext(group_id);
    if (js_context) {
      auto vm = js_context->getVM();
      // A page that joins an existing shared context must use the engine type
      // of that shared VM. Different runtime types on the same VM can crash.
      AlignRuntimeEngineWithVM(vm, force_use_lightweight_js_engine,
                               "use shared jscontext");
      TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS,
                  RUNTIME_MANAGER_SHARED_CONTEXT_REUSED);
      need_create_context_wrapper = false;
      js_runtime = CreateRuntime(force_use_lightweight_js_engine, true,
                                 create_params, page_options);
      js_runtime->setCreatedType(
          runtime::js::JSRuntimeCreatedType::none_vm_none_context);
      LOGI("get shared_context success, context:" << js_context.get()
                                                  << ", group:" << group_id);
    } else {
      TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS,
                  RUNTIME_MANAGER_CREATE_SHARED_CONTEXT_FIRST_TIME);
      // share context first create.
      js_runtime = CreateRuntime(force_use_lightweight_js_engine, false,
                                 create_params, page_options);
      js_context = CreateJSIContext(*js_runtime, create_params);
      LOGI("get shared_context failed, create context:"
           << js_context.get() << ", group:" << group_id);
    }
  }

  RegisterVMForTraceAndMonitor(js_context->getVM(), group_id,
                               page_options.GetInstanceID());

  EnsureConsolePostMan(js_context, executor, force_use_lightweight_js_engine,
                       page_options);
  js_runtime->InitRuntime(js_context);

  // none share context and first create share context.
  if (need_create_context_wrapper) {
    std::shared_ptr<JSContextWrapper> context_wrapper;
    base::UnsafeOwningPtr<runtime::js::Runtime> owned_global_runtime;
    if (is_single_context) {
      context_wrapper = std::make_shared<NoneSharedJSContextWrapper>(
          js_context, runtime_manager_delegate_ == nullptr ? this : nullptr);
    } else {
      context_wrapper =
          std::make_shared<SharedJSContextWrapper>(js_context, group_id, this);
      shared_context_map_.insert(std::make_pair(group_id, context_wrapper));
      if (IsInspectEnabled(force_use_lightweight_js_engine, page_options)) {
        runtime_manager_delegate_->AfterSharedContextCreate(group_id,
                                                            js_runtime->type());
      }
      // In shared-context mode the global runtime is a SEPARATE runtime
      // instance owned by SharedJSContextWrapper.
      auto unique_global =
          MakeRuntime(js_runtime->type() == runtime::js::JSRuntimeType::quickjs,
                      false, page_options);
      owned_global_runtime =
          base::UnsafeOwningPtr<runtime::js::Runtime>(unique_global.release());
      runtime::js::JSRuntimeExternalParams global_external_params{};
      global_external_params.group_id = group_id;
      owned_global_runtime->SetExternalParams(
          std::move(global_external_params));
      owned_global_runtime->InitRuntime(js_context);
    }
    auto& global_runtime =
        is_single_context ? js_runtime : owned_global_runtime;
#if ENABLE_TRACE_PERFETTO
    auto runtime_profiler = MakeRuntimeProfiler(
        js_context, force_use_lightweight_js_engine, page_options);
    context_wrapper->SetRuntimeProfiler(runtime_profiler);
#endif
    js_context->SetReleaseObserver(context_wrapper);
    std::shared_ptr<runtime::js::ConsoleMessagePostMan> post_man = nullptr;
    if (!IsInspectEnabled(force_use_lightweight_js_engine, page_options)) {
      post_man = js_context->GetPostMan();
    }
    TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS,
                RUNTIME_MANAGER_CONTEXT_WRAPPER_INIT_GLOBAL);
    context_wrapper->initGlobal(global_runtime, post_man, page_options);
    if (ensure_console) {
      context_wrapper->EnsureConsole(post_man, page_options);
    }

    // should call brefore loadPreJS.
    if (IsInspectEnabled(force_use_lightweight_js_engine, page_options)) {
      runtime_manager_delegate_->OnRuntimeReady(executor, *js_runtime,
                                                group_id);
    }

    runtime::js::GCPauseSuppressionMode mode(js_runtime.get());
    auto js_pre_sources = js_pre_sources_getter();
    TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS,
                RUNTIME_MANAGER_CONTEXT_WRAPPER_PREPARE_JS_ENV);
    context_wrapper->prepareJSEnv(js_runtime.GetWeakPtr(), js_pre_sources);
  } else {
    // Shared context reused. If corejs was deferred on the first creation,
    // we need to try to load it here to ensure the current runtime runs with
    // a fully-initialized shared JSIContext.
    auto* context_wrapper = GetContextWrapper(group_id);
    if (context_wrapper != nullptr && !context_wrapper->isJSCoreLoaded()) {
      auto js_pre_sources = js_pre_sources_getter();
      context_wrapper->EnsureCoreJSLoaded(*js_runtime, js_pre_sources);
    }
    // share context also need call this, because lynx_runtime is different.
    if (IsInspectEnabled(force_use_lightweight_js_engine, page_options)) {
      runtime_manager_delegate_->OnRuntimeReady(executor, *js_runtime,
                                                group_id);
    }
  }

#if ENABLE_TRACE_PERFETTO
  CheckAutotakeSnapshot(group_id);
#endif

  return js_runtime;
}

NewShareGroupGlobalContextWrapper*
RuntimeManager::EnsureNewShareGroupGlobalContext(
    bool force_use_lightweight_js_engine,
    const runtime::js::JSRuntimeExternalParams& create_params,
    const tasm::PageOptions& page_options,
    base::MoveOnlyClosure<std::vector<
        std::pair<std::string, std::shared_ptr<runtime::js::Buffer>>>>&
        js_pre_sources_getter,
    runtime::js::JSExecutor& executor) {
  const auto& group_id = create_params.group_id;
  auto it = new_share_group_map_.find(group_id);
  if (it != new_share_group_map_.end()) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS,
                RUNTIME_MANAGER_SHARED_CONTEXT_REUSED);
    auto* wrapper = it->second.get();
    auto* global_runtime = wrapper ? wrapper->GetGlobalRuntime() : nullptr;
    if (wrapper != nullptr && global_runtime != nullptr &&
        !wrapper->isJSCoreLoaded()) {
      auto js_pre_sources = js_pre_sources_getter();
      wrapper->JSContextWrapper::EnsureCoreJSLoaded(*global_runtime,
                                                    js_pre_sources);
    }
    return it->second.get();
  }

  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS,
              RUNTIME_MANAGER_CREATE_SHARED_CONTEXT_FIRST_TIME);

  // Create the group's global runtime + shared VM/context. Ownership of this
  // runtime is handed to the global-context wrapper (via initGlobal) so it
  // outlives every page in the group. It carries only the group id as external
  // params; napi is intentionally NOT installed on the global context (per-page
  // contexts own their own napi hooks).
  auto unique_global =
      MakeRuntime(force_use_lightweight_js_engine, false, page_options);
  base::UnsafeOwningPtr<runtime::js::Runtime> global_runtime(
      unique_global.release());
  runtime::js::JSRuntimeExternalParams global_params{};
  global_params.group_id = group_id;
  global_params.enable_new_share_group = true;
  global_params.is_shared_global_context = true;
  global_runtime->SetExternalParams(std::move(global_params));
  auto global_context = CreateJSIContext(*global_runtime, create_params);
  global_runtime->InitRuntime(global_context);
  runtime::js::Runtime* global_rt_ptr = global_runtime.get();
  // Capture a weak handle before initGlobal moves the owning pointer into the
  // wrapper; used to drive prepareJSEnv below.
  base::UnsafeWeakPtr<runtime::js::Runtime> global_runtime_weak =
      global_runtime.GetWeakPtr();

  auto wrapper = base::MakeUnsafeOwning<NewShareGroupGlobalContextWrapper>(
      global_context, group_id);
  // Register the engine type with the devtool delegate so the matching release
  // callback fires when the group's global context is torn down, mirroring the
  // legacy shared-context first-create path.
  if (IsInspectEnabled(force_use_lightweight_js_engine, page_options)) {
    runtime_manager_delegate_->AfterSharedContextCreate(group_id,
                                                        global_rt_ptr->type());
  }
#if ENABLE_TRACE_PERFETTO
  auto runtime_profiler = MakeRuntimeProfiler(
      global_context, force_use_lightweight_js_engine, page_options);
  wrapper->SetRuntimeProfiler(runtime_profiler);
#endif
  EnsureConsolePostMan(global_context, executor,
                       force_use_lightweight_js_engine, page_options);
  std::shared_ptr<runtime::js::ConsoleMessagePostMan> post_man = nullptr;
  if (!IsInspectEnabled(force_use_lightweight_js_engine, page_options)) {
    post_man = global_context->GetPostMan();
  }

  // initGlobal moves ownership of `global_runtime` into the wrapper and
  // installs the full set of shared host objects so page contexts can copy
  // them.
  wrapper->initGlobal(global_runtime, post_man, page_options);
  wrapper->EnsureConsole(post_man, page_options);

  // Register corejs before evaluation without attaching a page session to
  // this group-owned runtime.
  if (IsInspectEnabled(force_use_lightweight_js_engine, page_options)) {
    runtime_manager_delegate_->OnRuntimeReady(executor, *global_rt_ptr,
                                              group_id);
  }

  runtime::js::GCPauseSuppressionMode mode(global_rt_ptr);
  auto js_pre_sources = js_pre_sources_getter();
  wrapper->prepareJSEnv(global_runtime_weak, js_pre_sources);

  auto emplaced =
      new_share_group_map_.insert_or_assign(group_id, std::move(wrapper));
  return emplaced.first->second.get();
}

base::UnsafeOwningPtr<runtime::js::Runtime> RuntimeManager::CreateRuntime(
    bool force_use_lightweight_js_engine, bool use_shared_context,
    const runtime::js::JSRuntimeExternalParams& create_params,
    const tasm::PageOptions& page_options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, RUNTIME_MANAGER_CREATE_RUNTIME);
  auto unique_runtime = MakeRuntime(force_use_lightweight_js_engine,
                                    use_shared_context, page_options);
  base::UnsafeOwningPtr<runtime::js::Runtime> js_runtime(
      unique_runtime.release());
  if (!memory_task_runner_) {
    memory_task_runner_ = fml::MessageLoop::GetCurrent().GetTaskRunner();
  }
  TrackRuntimeForMemoryPressure(js_runtime.GetWeakPtr());
  js_runtime->SetPageOptions(page_options);
  js_runtime->SetExternalParams(create_params);
  return js_runtime;
}

void RuntimeManager::TrackRuntimeForMemoryPressure(
    base::UnsafeWeakPtr<runtime::js::Runtime> runtime) {
  if (!memory_task_runner_) {
    return;
  }
  memory_task_runner_->PostTask([this, w = std::move(runtime)]() mutable {
    weak_runtimes_.emplace_back(std::move(w));
    CompactWeakRuntimes();
  });
}

void RuntimeManager::CompactWeakRuntimes() {
  std::vector<base::UnsafeWeakPtr<runtime::js::Runtime>> alive;
  alive.reserve(weak_runtimes_.size());
  for (auto& w : weak_runtimes_) {
    if (!w.Expired()) {
      alive.emplace_back(w);
    }
  }
  weak_runtimes_.swap(alive);
}

void RuntimeManager::OnRelease(const std::string& group_id) {
  auto it = shared_context_map_.find(group_id);
  if (it != shared_context_map_.end()) {
    if (runtime_manager_delegate_) {
      runtime_manager_delegate_->OnRelease(group_id);
    }
    LOGI("RuntimeManager remove context:" << group_id);
    shared_context_map_.erase(it);
  } else {
    LOGI("RuntimeManager::OnRelease : not find shared jscontext in group:"
         << group_id << " It may has been released in global runtime.");
  }
}

void RuntimeManager::OnNewShareGroupPageRelease(const std::string& group_id) {
  // Dispatched only from NewShareGroupPageContextWrapper through
  // new_share_group_page_release_observer_, so this never collides with a
  // legacy shared context that happens to reuse the same group id. Decrement
  // the group's live page count; when the last page is gone drop the group's
  // global-context wrapper, which owns the global runtime and thus tears down
  // the shared VM + global context.
  auto ng_it = new_share_group_map_.find(group_id);
  if (ng_it == new_share_group_map_.end()) {
    return;
  }
  if (ng_it->second->DecLivePageCount() > 0) {
    return;
  }
  if (runtime_manager_delegate_) {
    runtime_manager_delegate_->OnRelease(group_id);
  }
  LOGI(kNewShareGroupTag << " release global context group:" << group_id);
  new_share_group_map_.erase(ng_it);
}

JSContextWrapper* RuntimeManager::GetContextWrapper(
    const std::string& group_id, bool enable_new_share_group) {
  if (enable_new_share_group) {
    auto ng_it = new_share_group_map_.find(group_id);
    return ng_it == new_share_group_map_.end() ? nullptr : ng_it->second.get();
  }
  auto it = shared_context_map_.find(group_id);
  return it == shared_context_map_.end() ? nullptr : it->second.get();
}

std::shared_ptr<runtime::js::JSIContext> RuntimeManager::GetSharedJSContext(
    const std::string& group_id) {
  auto it = shared_context_map_.find(group_id);
  return it == shared_context_map_.end() ? nullptr : it->second->getJSContext();
}

std::shared_ptr<runtime::js::JSIContext> RuntimeManager::CreateJSIContext(
    runtime::js::Runtime& rt,
    const runtime::js::JSRuntimeExternalParams& create_params) {
  std::shared_ptr<runtime::js::JSIContext> js_context;
  bool need_create_vm = false;
  if (!IsVMSharedAcrossGroups(rt.type())) {
    need_create_vm = true;
#if JS_ENGINE_TYPE == 1 || JS_ENGINE_TYPE == 2
    auto vm_instance = VMInstancePool::Instance().TakeVMInstance(rt.type());
    return rt.createContext(vm_instance == nullptr ? rt.createVM(nullptr)
                                                   : vm_instance);
#else
    return rt.createContext(rt.createVM(nullptr));
#endif
  } else {
    need_create_vm = EnsureVM(rt);
    js_context = rt.createContext(mVMContainer_[rt.type()]);
  }
  InitJSRuntimeCreatedType(need_create_vm, rt);
  return js_context;
}

void RuntimeManager::InitJSRuntimeCreatedType(bool need_create_vm,
                                              runtime::js::Runtime& rt) {
  runtime::js::JSRuntimeCreatedType type =
      need_create_vm ? runtime::js::JSRuntimeCreatedType::vm_context
                     : runtime::js::JSRuntimeCreatedType::context;
  rt.setCreatedType(type);
}

bool RuntimeManager::EnsureVM(runtime::js::Runtime& rt) {
  if (mVMContainer_.find(rt.type()) == mVMContainer_.end()) {
    runtime::js::StartupData* data = nullptr;

    mVMContainer_.insert(std::make_pair(rt.type(), rt.createVM(data)));
    if (rt.type() == runtime::js::JSRuntimeType::v8) {
      RegisterVMInstance(mVMContainer_[rt.type()]);
    }
    return true;
  }
  return false;
}

void RuntimeManager::EnsureConsolePostMan(
    std::shared_ptr<runtime::js::JSIContext>& context,
    runtime::js::JSExecutor& executor, bool force_use_lightweight_js_engine,
    const tasm::PageOptions& page_options) {
  if (IsInspectEnabled(force_use_lightweight_js_engine, page_options)) {
    return;
  }
  if (context != nullptr) {
    if (context->GetPostMan() == nullptr) {
      context->SetPostMan(executor.CreateConsoleMessagePostMan());
    }
    auto postman = context->GetPostMan();
    if (postman != nullptr) {
      postman->InsertRuntimeObserver(executor.GetRuntimeObserver());
    }
  }
}

std::unique_ptr<runtime::js::Runtime> RuntimeManager::MakeRuntime(
    bool force_use_lightweight_js_engine, bool use_shared_context,
    const tasm::PageOptions& page_options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, RUNTIME_MANAGER_MAKE_RUNTIME);
  if (IsInspectEnabled(force_use_lightweight_js_engine, page_options)) {
    return runtime_manager_delegate_->MakeRuntime(
        force_use_lightweight_js_engine, use_shared_context, page_options);
  }

#ifdef __APPLE__
#if defined(OS_IOS)
  if (force_use_lightweight_js_engine) {
    LOGI("make runtime with force_use_lightweight_js_engine = true");
    return runtime::js::makeQuickJsRuntime();
  }
#endif  // defined(OS_IOS)
#if JS_ENGINE_TYPE == 0
#if ENABLE_NAPI_BINDING
  static runtime::js::NapiRuntimeProxyV8FactoryImpl factory;
  LOGI("Setting napi proxy factory: " << &factory);
  RegisterV8RuntimeProxyFactory(&factory);
#endif
  return runtime::js::makeV8Runtime();
#elif JS_ENGINE_TYPE == 1
  LOGI("make JSC runtime");
  return runtime::js::makeJSCRuntime();
#endif
#endif  // __APPLE__

#ifdef OS_ANDROID
  if (!force_use_lightweight_js_engine) {
    auto ret = LynxProxyRuntimeHelper::Instance().MakeRuntime();
    if (ret) {
      LOGI("make runtime with proxy runtime helper.");
      return ret;
    } else {
      LOGI("make runtime LynxProxyRuntimeHelper return null");
    }
  } else {
    LOGI("make runtime with force_use_lightweight_js_engine = true");
  }

#if JS_ENGINE_TYPE == 1
  LOGI("make JSC runtime");
  return runtime::js::makeJSCRuntime();
#elif JS_ENGINE_TYPE == 2
  LOGI("make QuickJS runtime");
  return runtime::js::makeQuickJsRuntime();
#endif  // JS_ENGINE_TYPE

#endif  // OS_ANDROID

#if defined(OS_WIN) || defined(OS_LINUX)
#if JS_ENGINE_TYPE == 0

#if ENABLE_NAPI_BINDING
  static runtime::js::NapiRuntimeProxyV8FactoryImpl factory;
  LOGI("Setting napi proxy factory from none inspector: " << &factory);
  RegisterV8RuntimeProxyFactory(&factory);
#endif

  return runtime::js::makeV8Runtime();
#elif JS_ENGINE_TYPE == 2
  LOGI("make quickjs runtime");
  return runtime::js::makeQuickJsRuntime();
#endif

#endif  // OS_WIN || OS_LINUX

#if OS_HARMONY
#if JS_ENGINE_TYPE != 3
  if ((!force_use_lightweight_js_engine ||
       tasm::LynxEnv::GetInstance().EnableJSVMRuntime()) &&
      runtime::js::IsJSVMRuntimeAvailable()) {
#endif  // JS_ENGINE_TYPE != 3
#if ENABLE_NAPI_BINDING
    static runtime::js::NapiRuntimeProxyJSVMFactoryImpl factory;
    RegisterJSVMRuntimeProxyFactory(&factory);
#endif  // ENABLE_NAPI_BINDING
    LOGI("make jsvm runtime");
    return runtime::js::makeJSVMRuntime();
#if JS_ENGINE_TYPE != 3
  }
#endif  // JS_ENGINE_TYPE != 3
#endif  // OS_HARMONY

// Fit compile on other unknown platforms such as Linux.
#if JS_ENGINE_TYPE == 2
  // desktop tests may run on Linux.
  LOGI("make quickjs runtime");
  return runtime::js::makeQuickJsRuntime();
#endif

  LOGW("No runtime made");
  return nullptr;
}

#if ENABLE_TRACE_PERFETTO
void RuntimeManager::CheckAutotakeSnapshot(const std::string& group_id) {
  if (!IsSingleJSContext(group_id)) {
    if (auto config =
            trace::TraceController::Instance()->GetLastSessionTraceConfig();
        config && config->enable_memory_trace && config->auto_take_snapshot) {
      if (config->auto_take_snapshot_group_id.empty() ||
          config->auto_take_snapshot_group_id == group_id) {
        TakeVMSnapshot(group_id, true);
      }
    }
  }
}

void RuntimeManager::TakeVMSnapshot(const std::string& group_id, bool initial) {
  auto* ctx_wrap = GetContextWrapper(group_id);
  if (ctx_wrap) {
    auto ctx = ctx_wrap->getJSContext();
    if (ctx) {
      std::string identifier = group_id + "(shared bts)";
      intptr_t payload[3] = {reinterpret_cast<intptr_t>(ctx.get()),
                             reinterpret_cast<intptr_t>(identifier.c_str()),
                             static_cast<intptr_t>(initial)};
      base::NotificationCallback::Notify(kBTSTakeVMSnapshot,
                                         reinterpret_cast<intptr_t>(payload));
    }
  }
}

void RuntimeManager::ScheduleVMSnapshot(const std::string& group_id) {
  if (!memory_task_runner_) {
    return;
  }
  uint64_t task_id = 0;
  {
    std::lock_guard<std::mutex> lock(pending_vm_snapshot_mutex_);
    task_id = ++((*pending_vm_snapshot_tasks_)[group_id]);
  }
  memory_task_runner_->PostDelayedTask(
      [this, group_id, task_id]() {
        {
          std::lock_guard<std::mutex> lock(pending_vm_snapshot_mutex_);
          auto it = pending_vm_snapshot_tasks_->find(group_id);
          if (it == pending_vm_snapshot_tasks_->end() ||
              it->second != task_id) {
            return;
          }
          pending_vm_snapshot_tasks_->erase(it);
        }
        TakeVMSnapshot(group_id, false);
      },
      fml::TimeDelta::FromMilliseconds(500));
}

std::shared_ptr<profile::RuntimeProfiler> RuntimeManager::MakeRuntimeProfiler(
    std::shared_ptr<runtime::js::JSIContext> js_context,
    bool force_use_lightweight_js_engine,
    const tasm::PageOptions& page_options) {
  if (runtime_manager_delegate_) {
    return runtime_manager_delegate_->MakeRuntimeProfiler(
        js_context, force_use_lightweight_js_engine, page_options);
  }
#if OS_ANDROID
  if (!force_use_lightweight_js_engine) {
    auto v8_profiler =
        LynxProxyRuntimeHelper::Instance().MakeRuntimeProfiler(js_context);
    return std::make_shared<profile::V8RuntimeProfiler>(std::move(v8_profiler));
  } else {
    return runtime::js::makeQuickJsRuntimeProfiler(js_context);
  }
#endif  // OS_ANDROID
#if OS_IOS
  if (force_use_lightweight_js_engine) {
    return runtime::js::makeQuickJsRuntimeProfiler(js_context);
  }
#endif  // defined(OS_IOS)
  return nullptr;
}
#endif  // ENABLE_TRACE_PERFETTO

bool RuntimeManager::IsInspectEnabled(bool force_use_lightweight_js_engine,
                                      const tasm::PageOptions& page_options) {
  bool debuggable = page_options.GetDebuggable();
  return runtime_manager_delegate_ &&
         tasm::LynxEnv::GetInstance().IsJsDebugEnabled(
             force_use_lightweight_js_engine, debuggable);
}

void RuntimeManager::OnMemoryPressure(base::MemoryPressureLevel level) {
  if (!memory_task_runner_) {
    return;
  }
  memory_task_runner_->PostTask([this]() {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, RUN_GC_EVENT);
    std::vector<base::UnsafeWeakPtr<runtime::js::Runtime>> alive;
    std::unordered_set<std::string> seen_groups;
    for (auto& w : weak_runtimes_) {
      auto* rt = w.Lock();
      if (!rt) {
        continue;
      }
      const auto& gid = rt->getGroupId();
      if (IsSingleJSContext(gid)) {
        rt->RequestGC();
      } else {
        if (seen_groups.insert(gid).second) {
          rt->RequestGC();
        }
      }
      alive.emplace_back(w);
    }
    weak_runtimes_.swap(alive);
  });
}

}  // namespace runtime
}  // namespace lynx
