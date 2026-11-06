// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/js/bindings/modules/module_interceptor.h"

#include <utility>
#include <vector>

#include "base/include/log/logging.h"
#include "core/runtime/js/bindings/modules/lynx_jsi_module_callback.h"

namespace lynx {
namespace runtime {
namespace js {
ModuleInterceptorResult GroupInterceptor::InterceptModuleMethod(
    const std::shared_ptr<LynxModule>& module,
    const LynxModule::MethodMetadata& method, Runtime* rt,
    const std::shared_ptr<ModuleDelegate>& delegate, const Value* args,
    size_t count, const std::unique_ptr<pub::Value>& pub_args,
    const CallbackMap& callbacks, NativeModuleInfoCollectorPtr timing_collector,
    ModuleCallContext* context) const {
  for (auto& i : interceptors_) {
    // Callbacks may run inside InterceptModuleMethod. Bound their hooks before
    // entering it, then clear the bound if this interceptor declines the call.
    if (context) context->resolver_ = i.get();
    auto pair = i->InterceptModuleMethod(module, method, rt, delegate, args,
                                         count, pub_args, callbacks,
                                         timing_collector, context);
    if (pair.handled) {
      return pair;
    }
    if (context) context->resolver_ = nullptr;
  }
  return {false, Value::null()};
}

template <typename Function>
void GroupInterceptor::ForEachTarget(const ModuleCallContext* context,
                                     Function function) {
  // Resolving stops propagation for the rest of the invocation.
  const ModuleInterceptor* resolver = context ? context->resolver() : nullptr;
  for (auto& i : interceptors_) {
    function(*i);
    if (i.get() == resolver) {
      break;
    }
  }
}

std::shared_ptr<ModuleCallContext> GroupInterceptor::BeforeConvertArgs(
    const std::shared_ptr<LynxModule>& module,
    const LynxModule::MethodMetadata& method, Runtime* rt, const Value* args,
    size_t count, std::vector<Value>& rewritten) {
  if (!has_invocation_hooks_) {
    return nullptr;
  }
  std::shared_ptr<ModuleCallContext> context;
  for (auto& i : interceptors_) {
    std::vector<Value> replacement;
    auto supplied =
        i->BeforeConvertArgs(module, method, rt, args, count, replacement);
    if (supplied) {
      // A chain has at most one context provider.
      CHECK(!context);
      context.swap(supplied);
    }
    if (!replacement.empty() && replacement.size() == count) {
      rewritten.swap(replacement);
      args = rewritten.data();
    }
  }
  // Stateless chains still need a per-call callback propagation boundary.
  if (context) {
    return context;
  }
  return std::make_shared<ModuleCallContext>();
}

void GroupInterceptor::BeforeInvokeMethod(
    const LynxModule::MethodMetadata& method,
    const std::unique_ptr<pub::Value>& args,
    const NativeModuleInfoCollectorPtr& timing_collector) {
  for (auto& i : interceptors_) {
    i->BeforeInvokeMethod(method, args, timing_collector);
  }
}

bool GroupInterceptor::AfterInvokeMethod(Runtime* rt,
                                         ModuleCallContext* context,
                                         Value& result) {
  bool replaced = false;
  ForEachTarget(context, [&](ModuleInterceptor& i) {
    replaced = i.AfterInvokeMethod(rt, context, result) || replaced;
  });
  return replaced;
}

void GroupInterceptor::OnInvokeMethodEnd(
    ModuleCallContext* context, const std::shared_ptr<ModuleDelegate>& delegate,
    const CallbackMap& callbacks, bool args_converted) {
  for (auto& i : interceptors_) {
    i->OnInvokeMethodEnd(context, delegate, callbacks, args_converted);
  }
}

void GroupInterceptor::WillInvokeCallback(ModuleCallback& callback,
                                          std::unique_ptr<pub::Value>& args) {
  ForEachTarget(callback.call_context().get(), [&](ModuleInterceptor& i) {
    if (args) {
      i.WillInvokeCallback(callback, args);
    }
  });
}

void GroupInterceptor::OnCallbackInvoked(
    const NativeModuleInfoCollectorPtr& timing, ModuleCallback* callback) {
  ForEachTarget(callback->call_context().get(), [&](ModuleInterceptor& i) {
    i.OnCallbackInvoked(timing, callback);
  });
}

void GroupInterceptor::AddInterceptor(
    std::unique_ptr<ModuleInterceptor> interceptor,
    bool enable_invocation_hooks) {
  has_invocation_hooks_ |= enable_invocation_hooks;
  interceptors_.push_back(std::move(interceptor));
}

void GroupInterceptor::AddLeadingInterceptor(
    std::unique_ptr<ModuleInterceptor> interceptor) {
  AddInterceptor(std::move(interceptor), true);
  for (size_t i = interceptors_.size() - 1; i > 0; --i) {
    interceptors_[i].swap(interceptors_[i - 1]);
  }
}

void GroupInterceptor::SetTemplateUrl(const std::string& url) {
  for (const auto& interceptor : interceptors_) {
    interceptor->SetTemplateUrl(url);
  }
}

}  // namespace js

}  // namespace runtime
}  // namespace lynx
