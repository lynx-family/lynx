// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/runtime/host_script_thread_bindings.h"

#include <utility>

#include "base/include/log/logging.h"

namespace lynx {
namespace shell {
namespace {
using runtime::js::Function;
using runtime::js::JSINativeExceptionCollector;
using runtime::js::Object;
using runtime::js::PropNameID;
using runtime::js::Runtime;
using runtime::js::Scope;
using runtime::js::String;
using runtime::js::Value;

// Catch in JavaScript: some JSI backends report user errors through a delegate
// while returning a populated value from property access.
constexpr char kInvokeSynchronous[] = R"JS(((Error, String) => function(fn) {
  try {
    const value = fn();
    if (value != null && typeof value.then === 'function')
      throw new Error('ASYNC_RESULT_UNSUPPORTED');
    return [true, value];
  } catch (error) { return [false, String(error)]; }
})(Error, String))JS";

bool IsFunction(Runtime& runtime, const Value& value) {
  return value.isObject() && value.getObject(runtime).isFunction(runtime);
}
}  // namespace

std::shared_ptr<HostScriptThreadBindings> HostScriptThreadBindings::Create(
    Runtime& runtime, fml::RefPtr<fml::TaskRunner> runner, Executor executor) {
  if (!runner || !runner->RunsTasksOnCurrentThread() || !executor) return {};
  return std::shared_ptr<HostScriptThreadBindings>(new HostScriptThreadBindings(
      runtime, std::move(runner), std::move(executor)));
}

HostScriptThreadBindings::HostScriptThreadBindings(
    Runtime& runtime, fml::RefPtr<fml::TaskRunner> runner, Executor executor)
    : runtime_(runtime),
      runner_(std::move(runner)),
      executor_(std::move(executor)) {}

bool HostScriptThreadBindings::Install(const std::string& domain) {
  if (domain != "bts" && domain != "mts" && domain != "ui") return false;
  if (attached_ || !executor_ || !runner_->RunsTasksOnCurrentThread())
    return false;
  promise_ = runtime_.global().getPropertyAsFunction(runtime_, "Promise");
  error_ = runtime_.global().getPropertyAsFunction(runtime_, "Error");
  auto object = runtime_.global().getPropertyAsObject(runtime_, "Object");
  auto json = runtime_.global().getPropertyAsObject(runtime_, "JSON");
  if (!promise_ || !error_ || !object || !json) return false;
  parse_json_ = json->getPropertyAsFunction(runtime_, "parse");
  if (!parse_json_) return false;
  define_property_ = object->getPropertyAsFunction(runtime_, "defineProperty");
  create_object_ = object->getPropertyAsFunction(runtime_, "create");
  get_prototype_ = object->getPropertyAsFunction(runtime_, "getPrototypeOf");
  auto function = runtime_.global().getPropertyAsObject(runtime_, "Function");
  if (!define_property_ || !create_object_ || !get_prototype_ || !function)
    return false;
  function_prototype_ = function->getPropertyAsObject(runtime_, "prototype");
  if (!function_prototype_) return false;
  function_source_ =
      function_prototype_->getPropertyAsFunction(runtime_, "toString");
  if (!function_source_) return false;
  auto invoke = runtime_.evaluateJavaScript(
      std::make_shared<runtime::js::StringBuffer>(kInvokeSynchronous),
      "host-script://synchronous-function");
  if (!invoke.has_value() || !IsFunction(runtime_, *invoke)) return false;
  invoke_sync_ = invoke->getObject(runtime_).getFunction(runtime_);

  Object target(runtime_);
  if (!DefineProperty(target, "thread",
                      String::createFromUtf8(runtime_, domain), false))
    return false;
  const std::pair<const char*, Domain> methods[] = {
      {"runOnMainThread", Domain::kMTS},
      {"runOnBackgroundThread", Domain::kBTS},
      {"runOnUIThread", Domain::kUI}};
  for (const auto& method : methods) {
    auto function = Function::createFromHostFunction(
        runtime_, PropNameID::forAscii(runtime_, method.first), 1,
        [weak = weak_from_this(), domain = method.second](
            Runtime&, const Value&, const Value* args,
            size_t count) -> CallResult {
          auto self = weak.lock();
          if (!self || !self->attached_) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "The Host Script runtime was detached"));
          }
          return self->Run(domain, args, count);
        });
    auto global = runtime_.global();
    if (!DefineProperty(global, method.first, Value(runtime_, function)) ||
        !DefineProperty(target, method.first, Value(std::move(function))))
      return false;
  }
  auto global = runtime_.global();
  if (!DefineProperty(global, "__hostScriptThread", Value(std::move(target))))
    return false;
  domain_ = domain;
  attached_ = true;
  return true;
}

bool HostScriptThreadBindings::DefineProperty(Object& target, const char* name,
                                              Value value,
                                              bool mutable_export) {
  // Define exports explicitly instead of assigning through user-defined
  // setters. Mutable exports allow assignment and redefinition; the thread
  // identity allows neither. JSI reports descriptor failures uniformly.
  auto created = create_object_->call(runtime_, Value::null());
  if (!created || !created->isObject()) return false;
  auto descriptor = created->getObject(runtime_);
  descriptor.setProperty(runtime_, "value", std::move(value));
  descriptor.setProperty(runtime_, "writable", mutable_export);
  descriptor.setProperty(runtime_, "enumerable", true);
  descriptor.setProperty(runtime_, "configurable", mutable_export);
  return define_property_
      ->call(runtime_, target, String::createFromUtf8(runtime_, name),
             descriptor)
      .has_value();
}

HostScriptThreadBindings::CallResult HostScriptThreadBindings::Run(
    Domain domain, const Value* args, size_t count) {
  const uint64_t request = ++next_request_;
  pending_.emplace(request, Pending{});
  auto executor = Function::createFromHostFunction(
      runtime_, PropNameID::forAscii(runtime_, "hostScriptPromise"), 2,
      [weak = weak_from_this(), request](Runtime& runtime, const Value&,
                                         const Value* args,
                                         size_t count) -> CallResult {
        auto self = weak.lock();
        if (!self || !self->attached_) return Value::undefined();
        auto found = self->pending_.find(request);
        if (found == self->pending_.end()) return Value::undefined();
        if (count != 2 || !IsFunction(runtime, args[0]) ||
            !IsFunction(runtime, args[1]) || found->second.resolve) {
          return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
              "Invalid Host Script Promise executor"));
        }
        found->second.resolve = args[0].getObject(runtime).getFunction(runtime);
        found->second.reject = args[1].getObject(runtime).getFunction(runtime);
        return Value::undefined();
      });
  auto promise = promise_->callAsConstructor(runtime_, std::move(executor));
  auto found = pending_.find(request);
  if (!promise || found == pending_.end() || !found->second.resolve) {
    pending_.erase(request);
    return base::unexpected(
        BUILD_JSI_NATIVE_EXCEPTION("Cannot create Host Script Promise"));
  }
  auto reject = [&](const char* message,
                    const char* code = "INVALID_ARGUMENT") -> CallResult {
    auto current = pending_.find(request);
    if (current != pending_.end()) {
      auto pending = std::move(current->second);
      pending_.erase(current);
      Reject(std::move(pending), code, message);
    }
    return std::move(*promise);
  };
  if (count < 1 || count > 2 ||
      (!args[0].isString() && !IsFunction(runtime_, args[0])) ||
      (count == 2 && !args[1].isUndefined() && !args[1].isString())) {
    return reject(
        "Thread execution expects a function or source and optional source "
        "URL");
  }
  auto url =
      count < 2 || args[1].isUndefined()
          ? std::string("host-script://") + ProcessRuntime::DomainName(domain)
          : args[1].getString(runtime_).utf8(runtime_);
  if (url.empty()) return reject("Source URL must be non-empty");
  if (!IsHostScriptRuntimeEnabled())
    return reject("HSR_DEBUG_DISABLED", "INVALID_STATE");
  std::string source;
  if (args[0].isString()) {
    source = args[0].getString(runtime_).utf8(runtime_);
  } else {
    const bool local = domain_ == ProcessRuntime::DomainName(domain);
    std::optional<Value> value;
    std::string failure;
    {
      // User functions and prototype traps may reenter or throw. Isolate the
      // exception collector before settling the source Promise.
      JSINativeExceptionCollector::Scope exceptions;
      auto prototype = get_prototype_->call(runtime_, &args[0], size_t{1});
      if (!prototype || !prototype->isObject() ||
          !Object::strictEquals(runtime_, prototype->getObject(runtime_),
                                *function_prototype_)) {
        failure = "Thread functions must be ordinary synchronous functions";
      } else if (local) {
        auto outcome = invoke_sync_->call(runtime_, &args[0], size_t{1});
        if (outcome && outcome->isObject() &&
            outcome->getObject(runtime_).isArray(runtime_)) {
          auto result = outcome->getObject(runtime_).getArray(runtime_);
          auto success = result.getValueAtIndex(runtime_, 0);
          value = result.getValueAtIndex(runtime_, 1);
          if (!success || !success->isBool() || !success->getBool()) {
            failure = value && value->isString()
                          ? value->getString(runtime_).utf8(runtime_)
                          : "JAVASCRIPT_CALL_FAILED";
            if (failure.empty()) failure = "JAVASCRIPT_CALL_FAILED";
          }
        }
        if (!value && failure.empty()) failure = "JAVASCRIPT_CALL_FAILED";
      } else {
        auto text = function_source_->callWithThis(
            runtime_, args[0].getObject(runtime_), {});
        if (!text || !text->isString()) {
          failure = "Cannot read function source";
        } else {
          // No function handle or lexical environment leaves this runtime.
          source = "\"use strict\";((Error)=>{const result=" +
                   std::string(kInvokeSynchronous) + "(" +
                   text->getString(runtime_).utf8(runtime_) +
                   "\n);if(!result[0])throw new Error(result[1]);"
                   "return result[1];})(Error)";
        }
      }
      const auto& error =
          JSINativeExceptionCollector::Instance()->GetException();
      if (error)
        failure = error->message().empty() ? "JAVASCRIPT_CALL_FAILED"
                                           : error->message();
    }
    if (!failure.empty()) return reject(failure.c_str(), "PLATFORM_ERROR");
    if (local) {
      // Nested runOn* may rehash pending_; never reuse the old iterator.
      auto current = pending_.find(request);
      if (current != pending_.end() && attached_) {
        auto pending = std::move(current->second);
        pending_.erase(current);
        JSINativeExceptionCollector::Scope exceptions;
        pending.resolve->call(runtime_, static_cast<const Value*>(&*value),
                              size_t{1});
      }
      return std::move(*promise);
    }
  }
  const auto weak = weak_from_this();
  // This callback can run on any target thread. It never retains or accesses
  // an engine handle, including when shutdown drops a queued completion.
  executor_(
      domain, std::move(source), std::move(url),
      [weak, runner = runner_, request](Result result) {
        runner->PostTask([weak, request, result = std::move(result)] {
          if (auto self = weak.lock(); self && self->attached_) {
            self->Complete(request, result);
          }
        });
      },
      [weak] {
        auto self = weak.lock();
        return self && self->attached_.load();
      });
  return std::move(*promise);
}

void HostScriptThreadBindings::Complete(uint64_t request,
                                        const Result& result) {
  auto found = pending_.find(request);
  if (found == pending_.end()) return;
  Scope scope(runtime_);
  JSINativeExceptionCollector::Scope exceptions;
  auto pending = std::move(found->second);
  pending_.erase(found);
  if (!result.success) {
    Reject(std::move(pending), "PLATFORM_ERROR", result.error.c_str());
    return;
  }
  Value value = Value::undefined();
  if (result.has_value) {
    // Recreate the copied value on the source runtime, never share VM handles.
    auto decoded = parse_json_->call(
        runtime_, String::createFromUtf8(runtime_, result.value_json));
    if (!decoded) {
      Reject(std::move(pending), "PLATFORM_ERROR",
             "Cannot decode thread result");
      return;
    }
    value = std::move(*decoded);
  }
  pending.resolve->call(runtime_, static_cast<const Value*>(&value), size_t{1});
  const auto& error = JSINativeExceptionCollector::Instance()->GetException();
  if (error) LOGE("Host Script completion failed: " << error->message());
}

void HostScriptThreadBindings::Reject(Pending pending, const char* code,
                                      const char* message) {
  if (!pending.reject) return;
  auto error = error_->callAsConstructor(
      runtime_, String::createFromUtf8(runtime_, message));
  if (error && error->isObject()) {
    error->getObject(runtime_).setProperty(
        runtime_, "code", String::createFromUtf8(runtime_, code));
    pending.reject->call(runtime_, static_cast<const Value*>(&*error),
                         size_t{1});
  }
}

void HostScriptThreadBindings::Detach() {
  attached_ = false;
  Scope scope(runtime_);
  JSINativeExceptionCollector::Scope exceptions;
  auto pending = std::move(pending_);
  pending_.clear();
  for (auto& request : pending) {
    Reject(std::move(request.second), "INVALID_STATE",
           "The Host Script runtime was detached");
  }
  promise_.reset();
  error_.reset();
  parse_json_.reset();
  define_property_.reset();
  create_object_.reset();
  get_prototype_.reset();
  function_source_.reset();
  invoke_sync_.reset();
  function_prototype_.reset();
  domain_.clear();
  executor_ = {};
}

}  // namespace shell
}  // namespace lynx
