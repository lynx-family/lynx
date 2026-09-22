// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/js/bindings/modules/module_interceptor.h"

#include <utility>

namespace lynx {
namespace runtime {
namespace js {
namespace {
thread_local std::shared_ptr<GroupInterceptor> active_invocation;
}
GroupInterceptor::Scope::Scope(std::shared_ptr<GroupInterceptor> current)
    : previous_(std::move(active_invocation)) {
  active_invocation = std::move(current);
}
GroupInterceptor::Scope::~Scope() { active_invocation = std::move(previous_); }
std::shared_ptr<GroupInterceptor> GroupInterceptor::Current() {
  return active_invocation;
}
std::shared_ptr<GroupInterceptor> GroupInterceptor::CreateInvocationGroup(
    const std::string& module, const std::string& method,
    base::LynxEntityId view) const {
  std::shared_ptr<GroupInterceptor> invocation;
  for (size_t i = 0; i < interceptors_.size(); ++i) {
    auto state = interceptors_[i]->CreateInvocation(module, method, view);
    if (state && !invocation) {
      invocation = std::make_shared<GroupInterceptor>();
      for (size_t j = 0; j < i; ++j)
        invocation->interceptors_.push_back(interceptors_[j]);
    }
    if (invocation)
      invocation->interceptors_.push_back(state ? std::move(state)
                                                : interceptors_[i]);
  }
  return invocation;
}
void GroupInterceptor::RewriteArguments(Runtime& rt, const Value* args,
                                        size_t count,
                                        std::vector<Value>& replacement) {
  for (auto& interceptor : interceptors_) {
    std::vector<Value> next;
    interceptor->RewriteArguments(rt, args, count, next);
    if (!next.empty()) {
      replacement = std::move(next);
      args = replacement.data();
    }
    if (interceptor->HandlesCall()) break;
  }
}
bool GroupInterceptor::HandlesCall() const {
  for (const auto& interceptor : interceptors_)
    if (interceptor->HandlesCall()) return true;
  return false;
}
bool GroupInterceptor::RewriteResult(Runtime& rt, Value& result) {
  bool changed = false;
  for (auto& interceptor : interceptors_)
    changed = interceptor->RewriteResult(rt, result) || changed;
  return changed;
}
void GroupInterceptor::BeforeCallback(int argument_index,
                                      std::unique_ptr<pub::Value>& args) {
  for (auto& interceptor : interceptors_)
    interceptor->BeforeCallback(argument_index, args);
}
ModuleInterceptorResult GroupInterceptor::InterceptModuleMethod(
    const std::shared_ptr<LynxModule>& module,
    const LynxModule::MethodMetadata& method, Runtime* rt,
    const std::shared_ptr<ModuleDelegate>& delegate, const Value* args,
    size_t count, const std::unique_ptr<pub::Value>& pub_args,
    const CallbackMap& callbacks,
    NativeModuleInfoCollectorPtr timing_collector) const {
  for (auto& i : interceptors_) {
    auto pair =
        i->InterceptModuleMethod(module, method, rt, delegate, args, count,
                                 pub_args, callbacks, timing_collector);
    if (pair.handled) {
      return pair;
    }
  }
  return {false, Value::null()};
}

void GroupInterceptor::BeforeInvokeMethod(
    const LynxModule::MethodMetadata& method,
    const std::unique_ptr<pub::Value>& args,
    const NativeModuleInfoCollectorPtr& timing_collector) {
  for (auto& i : interceptors_) {
    i->BeforeInvokeMethod(method, args, timing_collector);
  }
}

void GroupInterceptor::OnCallbackInvoked(
    const NativeModuleInfoCollectorPtr& timing, ModuleCallback* callback) {
  for (auto& i : interceptors_) {
    i->OnCallbackInvoked(timing, callback);
  }
}

void GroupInterceptor::AddInterceptor(
    std::unique_ptr<ModuleInterceptor> interceptor, bool prepend) {
  if (prepend) {
    interceptors_.insert(interceptors_.begin(), std::move(interceptor));
  } else {
    interceptors_.push_back(std::move(interceptor));
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
