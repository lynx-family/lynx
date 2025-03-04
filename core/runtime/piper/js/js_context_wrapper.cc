// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/piper/js/js_context_wrapper.h"

#include "core/inspector/console_message_postman.h"
#include "core/runtime/bindings/jsi/global.h"
#include "core/runtime/profile/runtime_profiler_manager.h"

namespace lynx {
namespace runtime {

JSContextWrapper::JSContextWrapper(std::shared_ptr<piper::JSIContext> context)
    : js_context_(context), js_core_loaded_(false), global_inited_(false) {}

void JSContextWrapper::loadPreJS(
    std::weak_ptr<piper::Runtime> js_runtime,
    std::vector<std::pair<std::string, std::string>>& js_preload) {
  if (js_core_loaded_) {
    return;
  }

  std::shared_ptr<piper::Runtime> rt = js_runtime.lock();
  if (!rt) {
    return;
  }

  // load the lynx_core.js
  piper::Scope scope(*rt);
  for (auto& [url, source] : js_preload) {
    auto buffer = std::make_shared<piper::StringBuffer>(source);
    auto prep = rt->prepareJavaScript(buffer, url);
    auto ret = rt->evaluatePreparedJavaScript(prep);
    if (!ret.has_value()) {
      rt->reportJSIException(ret.error());
    }
  }
  js_core_loaded_ = true;
}

#if ENABLE_TRACE_PERFETTO
void JSContextWrapper::SetRuntimeProfiler(
    std::shared_ptr<profile::RuntimeProfiler> runtime_profiler) {
  runtime_profiler_ = runtime_profiler;
  profile::RuntimeProfilerManager::GetInstance()->AddRuntimeProfiler(
      runtime_profiler_);
}
#endif

//////////////////////
SharedJSContextWrapper::SharedJSContextWrapper(
    std::shared_ptr<piper::JSIContext> context, const std::string& group_id,
    ReleaseListener* listener)
    : JSContextWrapper(context), group_id_(group_id), listener_(listener) {}

void SharedJSContextWrapper::Def() {
  // global has owner the js context, when only global own the js context, can
  // release now
  if (js_context_.use_count() == 2) {  // TODO : be trick, global has one, and
                                       // the Runtime call this has one...
    // TODO : release of global_ will trigger another Def() call
    if (global_ != nullptr) {
      global_.reset();
      if (listener_ != nullptr) {
        listener_->OnRelease(group_id_);
      }
    }
#if ENABLE_TRACE_PERFETTO
    profile::RuntimeProfilerManager::GetInstance()->RemoveRuntimeProfiler(
        runtime_profiler_);
    runtime_profiler_ = nullptr;
#endif
  }
}

void SharedJSContextWrapper::EnsureConsole(
    std::shared_ptr<piper::ConsoleMessagePostMan> post_man) {
  if (isGlobalInited() && global_) {
    global_->EnsureConsole(post_man);
  }
}

void SharedJSContextWrapper::initGlobal(
    std::shared_ptr<piper::Runtime>& rt,
    std::shared_ptr<piper::ConsoleMessagePostMan> post_man) {
  if (global_inited_) {
    return;
  }
  std::shared_ptr<piper::SharedContextGlobal> global =
      std::make_shared<piper::SharedContextGlobal>();
  global->Init(rt, post_man);
  global_inited_ = true;
  global_ = global;
}

NoneSharedJSContextWrapper::NoneSharedJSContextWrapper(
    std::shared_ptr<piper::JSIContext> context)
    : JSContextWrapper(context) {}

NoneSharedJSContextWrapper::NoneSharedJSContextWrapper(
    std::shared_ptr<piper::JSIContext> context,
    SharedJSContextWrapper::ReleaseListener* listener)
    : JSContextWrapper(context), listener_(listener) {}

void NoneSharedJSContextWrapper::Def() {
  if (js_context_.use_count() == 1) {
    global_.reset();
#if ENABLE_TRACE_PERFETTO
    profile::RuntimeProfilerManager::GetInstance()->RemoveRuntimeProfiler(
        runtime_profiler_);
    runtime_profiler_ = nullptr;
#endif
  }
}

void NoneSharedJSContextWrapper::EnsureConsole(
    std::shared_ptr<piper::ConsoleMessagePostMan> post_man) {
  if (isGlobalInited() && global_) {
    global_->EnsureConsole(post_man);
  }
}

void NoneSharedJSContextWrapper::initGlobal(
    std::shared_ptr<piper::Runtime>& js_runtime,
    std::shared_ptr<piper::ConsoleMessagePostMan> post_man) {
  if (global_inited_) {
    return;
  }
  std::shared_ptr<piper::SingleGlobal> global =
      std::make_shared<piper::SingleGlobal>();
  global->Init(js_runtime, post_man);
  global_inited_ = true;
  global_ = global;
}

}  // namespace runtime
}  // namespace lynx
