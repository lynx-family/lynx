// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/js/runtime_manager.h"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "base/include/log/logging.h"
#include "base/include/no_destructor.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/tasm/config.h"
#include "core/runtime/js/bindings/global.h"
#include "core/runtime/js/js_executor.h"
#include "core/runtime/js/jsi/jsi.h"

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
    lynx::piper::NapiRuntimeProxyJSVMFactory* factory);
#endif  // OS_HARMONY

#ifdef OS_ANDROID
#include "core/runtime/js/bindings/modules/android/lynx_proxy_runtime_helper.h"
#include "core/runtime/profile/v8/v8_runtime_profiler.h"
#endif

#if defined(OS_WIN) || defined(OS_OSX)
#if ENABLE_NAPI_BINDING
#include "core/runtime/common/napi/napi_runtime_proxy_v8.h"

extern void RegisterV8RuntimeProxyFactory(
    lynx::piper::NapiRuntimeProxyV8Factory*);
#endif  // ENABLE_NAPI_BINDING
#endif  // OS_WIN || OS_OSX

namespace lynx {
namespace runtime {

namespace {

#if JS_ENGINE_TYPE == 1 || JS_ENGINE_TYPE == 2

static constexpr int kMaxVMSize = 1;

// Currently only quickjs use this.
class VMInstancePool {
 public:
  static VMInstancePool& Instance();
  std::shared_ptr<piper::VMInstance> TakeVMInstance(
      piper::JSRuntimeType runtime_type);

 private:
  void CreateVMInstanceAsync(piper::JSRuntimeType runtime_type);
  std::shared_ptr<piper::VMInstance> DoCreateVMInstance(
      piper::JSRuntimeType runtime_type);
  std::mutex mtx_;
  std::unordered_map<piper::JSRuntimeType,
                     std::vector<std::shared_ptr<piper::VMInstance>>>
      vm_instances_;
};

VMInstancePool& VMInstancePool::Instance() {
  static base::NoDestructor<VMInstancePool> pool_;
  return *pool_;
}

// Currently only support quickjs.
// Maybe null
std::shared_ptr<piper::VMInstance> VMInstancePool::TakeVMInstance(
    piper::JSRuntimeType runtime_type) {
  std::shared_ptr<piper::VMInstance> vm_instance = nullptr;
  if (runtime_type != piper::JSRuntimeType::quickjs) {
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

  piper::BindQuickjsVMToCurrentThread(vm_instance);

  // pre create next vm instance.
  CreateVMInstanceAsync(runtime_type);
  return vm_instance;
}

void VMInstancePool::CreateVMInstanceAsync(piper::JSRuntimeType runtime_type) {
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
      },
      base::ConcurrentTaskType::NORMAL_PRIORITY);
}

std::shared_ptr<piper::VMInstance> VMInstancePool::DoCreateVMInstance(
    piper::JSRuntimeType runtime_type) {
  if (runtime_type == piper::JSRuntimeType::quickjs) {
    return piper::CreateQuickJsVM(nullptr, false);
  }
  // Current don't support other engine.
  return nullptr;
}

#endif  // JS_ENGINE_TYPE

}  // namespace

RuntimeManager* RuntimeManager::Instance() {
  static thread_local RuntimeManager instance_;
  return &instance_;
}

RuntimeManager::~RuntimeManager() {
  // Should destroy runtime_manager_delegate_ before mVMContainer_
  runtime_manager_delegate_.reset();
}

bool RuntimeManager::IsSingleJSContext(const std::string& group_id) {
  return group_id == "-1";
}

std::shared_ptr<piper::Runtime> RuntimeManager::CreateJSRuntime(
    const std::string& group_id,
    std::shared_ptr<piper::JSIExceptionHandler> exception_handler,
    base::MoveOnlyClosure<
        std::vector<std::pair<std::string, std::shared_ptr<piper::Buffer>>>>
        js_pre_sources_getter,
    bool force_use_lightweight_js_engine, piper::JSExecutor& executor,
    int64_t rt_id, bool ensure_console, bool enable_bytecode,
    const std::string& bytecode_source_url,
    piper::BytecodeGetter bytecode_getter,
    const tasm::PageOptions& page_options) {
  // call inspect's prepare
  if (IsInspectEnabled(force_use_lightweight_js_engine, page_options)) {
    runtime_manager_delegate_->BeforeRuntimeCreate(
        force_use_lightweight_js_engine);
  }
  bool is_single_context = IsSingleJSContext(group_id);
  std::shared_ptr<piper::Runtime> js_runtime;
  std::shared_ptr<piper::JSIContext> js_context;
  // This variable indicates 'false' only when it has been created previously
  // and the context is being shared.
  bool need_create_context_wrapper = true;
  if (is_single_context) {
    js_runtime = CreateRuntime(group_id, exception_handler,
                               force_use_lightweight_js_engine, rt_id,
                               enable_bytecode, bytecode_source_url,
                               std::move(bytecode_getter), page_options);
    js_context = CreateJSIContext(js_runtime, group_id);
    LOGI("create single_context:" << js_context.get());
  } else {
    js_context = GetSharedJSContext(group_id);
    if (js_context) {
      // Decide whether the engine type is determined by the
      // share context created previously.
      auto vm = js_context->getVM();
      if (vm) {
        // page need shared context with same lynx group id, js runtime must be
        // create with the type from js context different js runtime type using
        // shared context will cause crash. here we change the
        // force_use_lightweight_js_engine param for MakeRuntime to control the
        // runtime type.
        if (vm->GetRuntimeType() == piper::JSRuntimeType::v8 ||
            vm->GetRuntimeType() == piper::JSRuntimeType::jsc ||
            vm->GetRuntimeType() == piper::JSRuntimeType::jsvm) {
          if (force_use_lightweight_js_engine) {
            LOGI(
                "use shared jscontext with v8, jsc or jsvm, change "
                "force_use_lightweight_js_engine to false");
            force_use_lightweight_js_engine = false;
          } else {
            LOGI("use shared jscontext");
          }
        } else {
          if (!force_use_lightweight_js_engine) {
            LOGI(
                "use shared jscontext with none-v8, none-jsc and none-jsvm, "
                "change "
                "force_use_lightweight_js_engine to true");
            force_use_lightweight_js_engine = true;
          } else {
            LOGI("use shared jscontext");
          }
        }
      }
      need_create_context_wrapper = false;
      js_runtime = CreateRuntime(
          group_id, exception_handler, force_use_lightweight_js_engine, rt_id,
          enable_bytecode, bytecode_source_url, std::move(bytecode_getter),
          page_options, true);
      js_runtime->setCreatedType(
          piper::JSRuntimeCreatedType::none_vm_none_context);
      LOGI("get shared_context success, context:" << js_context.get()
                                                  << ", group:" << group_id);
    } else {
      // share context first create.
      js_runtime = CreateRuntime(group_id, exception_handler,
                                 force_use_lightweight_js_engine, rt_id,
                                 enable_bytecode, bytecode_source_url,
                                 std::move(bytecode_getter), page_options);
      js_context = CreateJSIContext(js_runtime, group_id);
      LOGI("get shared_context failed, create context:"
           << js_context.get() << ", group:" << group_id);
    }
  }
  EnsureConsolePostMan(js_context, executor, force_use_lightweight_js_engine,
                       page_options);
  js_runtime->InitRuntime(js_context, exception_handler);
  js_runtime->setGroupId(group_id);

  // none share context and first create share context.
  if (need_create_context_wrapper) {
    std::shared_ptr<JSContextWrapper> context_wrapper;
    std::shared_ptr<piper::Runtime> global_runtime;
    if (is_single_context) {
      context_wrapper = std::make_shared<NoneSharedJSContextWrapper>(
          js_context, runtime_manager_delegate_ == nullptr ? this : nullptr);
      global_runtime = js_runtime;
    } else {
      context_wrapper =
          std::make_shared<SharedJSContextWrapper>(js_context, group_id, this);
      shared_context_map_.insert(std::make_pair(group_id, context_wrapper));
      if (IsInspectEnabled(force_use_lightweight_js_engine, page_options)) {
        runtime_manager_delegate_->AfterSharedContextCreate(group_id,
                                                            js_runtime->type());
      }
      global_runtime =
          MakeRuntime(js_runtime->type() == piper::JSRuntimeType::quickjs,
                      false, page_options);
      // FIXME(heshan):now set exception_handler to global runtime, not
      // correct...
      global_runtime->InitRuntime(js_context, exception_handler);
      global_runtime->setGroupId(group_id);
    }
#if ENABLE_TRACE_PERFETTO
    auto runtime_profiler = MakeRuntimeProfiler(
        js_context, force_use_lightweight_js_engine, page_options);
    context_wrapper->SetRuntimeProfiler(runtime_profiler);
#endif
    js_context->SetReleaseObserver(context_wrapper);
    std::shared_ptr<piper::ConsoleMessagePostMan> post_man = nullptr;
    if (!IsInspectEnabled(force_use_lightweight_js_engine, page_options)) {
      post_man = js_context->GetPostMan();
    }
    context_wrapper->initGlobal(global_runtime, post_man, page_options);
    if (ensure_console) {
      context_wrapper->EnsureConsole(post_man, page_options);
    }

    // should call brefore loadPreJS.
    if (IsInspectEnabled(force_use_lightweight_js_engine, page_options)) {
      runtime_manager_delegate_->OnRuntimeReady(executor, js_runtime, group_id);
    }

    piper::GCPauseSuppressionMode mode(global_runtime.get());
    auto js_pre_sources = js_pre_sources_getter();
    context_wrapper->prepareJSEnv(js_runtime, js_pre_sources);
  } else {
    // share context also need call this, because lynx_runtime is different.
    if (IsInspectEnabled(force_use_lightweight_js_engine, page_options)) {
      runtime_manager_delegate_->OnRuntimeReady(executor, js_runtime, group_id);
    }
  }

  return js_runtime;
}

std::shared_ptr<piper::Runtime> RuntimeManager::CreateRuntime(
    const std::string& group_id,
    std::shared_ptr<piper::JSIExceptionHandler> exception_handler,
    bool force_use_lightweight_js_engine, int64_t rt_id, bool enable_bytecode,
    const std::string& bytecode_source_url,
    piper::BytecodeGetter bytecode_getter,
    const tasm::PageOptions& page_options, bool use_shared_context) {
  auto js_runtime = MakeRuntime(force_use_lightweight_js_engine,
                                use_shared_context, page_options);
  js_runtime->setRuntimeId(rt_id);
  js_runtime->SetPageOptions(page_options);
  js_runtime->SetEnableUserBytecode(enable_bytecode);
  js_runtime->SetBytecodeSourceUrl(bytecode_source_url);
  js_runtime->SetBytecodeGetter(std::move(bytecode_getter));
  return js_runtime;
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

JSContextWrapper* RuntimeManager::GetContextWrapper(
    const std::string& group_id) {
  auto it = shared_context_map_.find(group_id);
  return it == shared_context_map_.end() ? nullptr : it->second.get();
}

std::shared_ptr<piper::JSIContext> RuntimeManager::GetSharedJSContext(
    const std::string& group_id) {
  auto it = shared_context_map_.find(group_id);
  return it == shared_context_map_.end() ? nullptr : it->second->getJSContext();
}

std::shared_ptr<piper::JSIContext> RuntimeManager::CreateJSIContext(
    std::shared_ptr<piper::Runtime>& rt, const std::string& group_id) {
  std::shared_ptr<piper::JSIContext> js_context;
  bool need_create_vm = false;
  if (rt->type() == piper::JSRuntimeType::jsc ||
      rt->type() == piper::JSRuntimeType::quickjs) {
    need_create_vm = true;
#if JS_ENGINE_TYPE == 1 || JS_ENGINE_TYPE == 2
    auto vm_instance = VMInstancePool::Instance().TakeVMInstance(rt->type());
    return rt->createContext(vm_instance == nullptr ? rt->createVM(nullptr)
                                                    : vm_instance);
#else
    return rt->createContext(rt->createVM(nullptr));
#endif
  } else {
    need_create_vm = EnsureVM(rt);
    js_context = rt->createContext(mVMContainer_[rt->type()]);
  }
  InitJSRuntimeCreatedType(need_create_vm, rt);
  return js_context;
}

void RuntimeManager::InitJSRuntimeCreatedType(
    bool need_create_vm, std::shared_ptr<piper::Runtime>& rt) {
  piper::JSRuntimeCreatedType type =
      need_create_vm ? piper::JSRuntimeCreatedType::vm_context
                     : piper::JSRuntimeCreatedType::context;
  rt->setCreatedType(type);
}

bool RuntimeManager::EnsureVM(std::shared_ptr<piper::Runtime>& rt) {
  if (mVMContainer_.find(rt->type()) == mVMContainer_.end()) {
    piper::StartupData* data = nullptr;

    mVMContainer_.insert(std::make_pair(rt->type(), rt->createVM(data)));
    return true;
  }
  return false;
}

void RuntimeManager::EnsureConsolePostMan(
    std::shared_ptr<piper::JSIContext>& context, piper::JSExecutor& executor,
    bool force_use_lightweight_js_engine,
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

std::shared_ptr<piper::Runtime> RuntimeManager::MakeRuntime(
    bool force_use_lightweight_js_engine, bool use_shared_context,
    const tasm::PageOptions& page_options) {
  if (IsInspectEnabled(force_use_lightweight_js_engine, page_options)) {
    return runtime_manager_delegate_->MakeRuntime(
        force_use_lightweight_js_engine, use_shared_context, page_options);
  }

#ifdef __APPLE__
#if defined(OS_IOS)
  if (force_use_lightweight_js_engine) {
    LOGI("make runtime with force_use_lightweight_js_engine = true");
    return piper::makeQuickJsRuntime();
  }
#endif  // defined(OS_IOS)
#if JS_ENGINE_TYPE == 0
#if ENABLE_NAPI_BINDING
  static piper::NapiRuntimeProxyV8FactoryImpl factory;
  LOGI("Setting napi proxy factory: " << &factory);
  RegisterV8RuntimeProxyFactory(&factory);
#endif
  return piper::makeV8Runtime();
#elif JS_ENGINE_TYPE == 1
  LOGI("make JSC runtime");
  return piper::makeJSCRuntime();
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
  return piper::makeJSCRuntime();
#elif JS_ENGINE_TYPE == 2
  LOGI("make QuickJS runtime");
  return piper::makeQuickJsRuntime();
#endif  // JS_ENGINE_TYPE

#endif  // OS_ANDROID

#if defined(OS_WIN)
#if JS_ENGINE_TYPE == 0

#if ENABLE_NAPI_BINDING
  static piper::NapiRuntimeProxyV8FactoryImpl factory;
  LOGI("Setting napi proxy factory from none inspector: " << &factory);
  RegisterV8RuntimeProxyFactory(&factory);
#endif

  return piper::makeV8Runtime();
#elif JS_ENGINE_TYPE == 2
  LOGI("make quickjs runtime");
  return piper::makeQuickJsRuntime();
#endif

#endif  // OS_WIN

#if OS_HARMONY
#if JS_ENGINE_TYPE != 3
  if ((!force_use_lightweight_js_engine ||
       tasm::LynxEnv::GetInstance().EnableJSVMRuntime()) &&
      piper::IsJSVMRuntimeAvailable()) {
#endif  // JS_ENGINE_TYPE != 3
#if ENABLE_NAPI_BINDING
    static piper::NapiRuntimeProxyJSVMFactoryImpl factory;
    RegisterJSVMRuntimeProxyFactory(&factory);
#endif  // ENABLE_NAPI_BINDING
    LOGI("make jsvm runtime");
    return piper::makeJSVMRuntime();
#if JS_ENGINE_TYPE != 3
  }
#endif  // JS_ENGINE_TYPE != 3
#endif  // OS_HARMONY

// Fit compile on other unknown platforms such as Linux.
#if JS_ENGINE_TYPE == 2
  // desktop tests may run on Linux.
  LOGI("make quickjs runtime");
  return piper::makeQuickJsRuntime();
#endif

  LOGW("No runtime made");
  return nullptr;
}

#if ENABLE_TRACE_PERFETTO
std::shared_ptr<profile::RuntimeProfiler> RuntimeManager::MakeRuntimeProfiler(
    std::shared_ptr<piper::JSIContext> js_context,
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
    return piper::makeQuickJsRuntimeProfiler(js_context);
  }
#endif  // OS_ANDROID
#if OS_IOS
  if (force_use_lightweight_js_engine) {
    return piper::makeQuickJsRuntimeProfiler(js_context);
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

}  // namespace runtime
}  // namespace lynx
