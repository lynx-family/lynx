/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the current directory.
 */
// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/jsi/jsi.h"

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <stdexcept>

#include "core/resource/lynx_resource_setting.h"
#include "core/runtime/jsi/instrumentation.h"

namespace lynx {
namespace piper {

namespace {

// This is used for generating short exception strings.
std::string kindToString(const Value& v, Runtime* rt = nullptr) {
  if (v.isUndefined()) {
    return "undefined";
  } else if (v.isNull()) {
    return "null";
  } else if (v.isBool()) {
    return v.getBool() ? "true" : "false";
  } else if (v.isNumber()) {
    return "a number";
  } else if (v.isString()) {
    return "a string";
  } else {
    DCHECK(v.isObject() && "Expecting object.");
    return rt != nullptr && v.getObject(*rt).isFunction(*rt) ? "a function"
                                                             : "an object";
  }
}

}  // namespace

report_func VMInstance::trig_mem_info_event_ = nullptr;

PreparedJavaScript::~PreparedJavaScript() = default;

Value HostObject::get(Runtime*, const PropNameID&) { return Value(); }

void HostObject::set(Runtime* rt, const PropNameID& name, const Value&) {
  std::string msg("TypeError: Cannot assign to property '");
  msg += name.utf8(*rt);
  msg += "' on HostObject with default setter";
  rt->reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(msg));
}

// std::atomic<intptr_t> Runtime::g_counter_ = 0;

void Runtime::reportJSIException(const JSIException& exception) {
  if (exception_handler_) {
    exception_handler_->onJSIException(exception);
  }
}

Instrumentation& Runtime::instrumentation() {
  class NoInstrumentation : public Instrumentation {
    std::string getRecordedGCStats() override { return ""; }

    Value getHeapInfo(bool) override { return Value::undefined(); }

    void collectGarbage() override {}

    bool createSnapshotToFile(const std::string&, bool) override {
      return false;
    }

    void writeBridgeTrafficTraceToFile(const std::string&) const override {
      std::abort();
    }

    void writeBasicBlockProfileTraceToFile(const std::string&) const override {
      std::abort();
    }

    void dumpProfilerSymbolsToFile(const std::string&) const override {
      std::abort();
    }
  };

  static NoInstrumentation sharedInstance;
  return sharedInstance;
}

void Runtime::ClearHostContainers() {
  observers_.ForEachObserver();
  host_function_containers_.clear();
  host_object_containers_.clear();
}

void Runtime::AddObserver(base::Observer* obs) { observers_.AddObserver(obs); }

void Runtime::RemoveObserver(base::Observer* obs) {
  observers_.RemoveObserver(obs);
}

Pointer& Pointer::operator=(Pointer&& other) {
  if (ptr_) {
    ptr_->invalidate();
  }
  ptr_ = other.ptr_;
  other.ptr_ = nullptr;
  return *this;
}

std::optional<Object> Object::getPropertyAsObject(Runtime& runtime,
                                                  const char* name) const {
  auto v = getProperty(runtime, name);
  if (!v) {
    return std::optional<Object>();
  }

  if (!v->isObject()) {
    runtime.reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
        std::string("getPropertyAsObject: property '") + name + "' is " +
        kindToString(*v, &runtime) + ", expected an Object"));
    return std::optional<Object>();
  }

  return v->getObject(runtime);
}

std::optional<Function> Object::getPropertyAsFunction(Runtime& runtime,
                                                      const char* name) const {
  auto obj = getPropertyAsObject(runtime, name);
  if (!obj) {
    return std::optional<Function>();
  }
  if (!obj->isFunction(runtime)) {
    runtime.reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
        std::string("getPropertyAsFunction: property '") + name + "' is " +
        kindToString(std::move(*obj), &runtime) + ", expected a Function"));
    return std::optional<Function>();
  };

  Runtime::PointerValue* value = obj->ptr_;
  obj->ptr_ = nullptr;
  return Function(value);
}

std::optional<Array> Object::asArray(Runtime& runtime) const& {
  if (!isArray(runtime)) {
    runtime.reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
        "Object is " + kindToString(Value(runtime, *this), &runtime) +
        ", expected an array"));
    return std::optional<Array>();
  }
  return getArray(runtime);
}

std::optional<Array> Object::asArray(Runtime& runtime) && {
  if (!isArray(runtime)) {
    runtime.reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
        "Object is " + kindToString(Value(runtime, *this), &runtime) +
        ", expected an array"));
    return std::optional<Array>();
  }
  return std::move(*this).getArray(runtime);
}

std::optional<Function> Object::asFunction(Runtime& runtime) const& {
  if (!isFunction(runtime)) {
    runtime.reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
        "Object is " + kindToString(Value(runtime, *this), &runtime) +
        ", expected a function"));
    return std::optional<Function>();
  }
  return getFunction(runtime);
}

std::optional<Function> Object::asFunction(Runtime& runtime) && {
  if (!isFunction(runtime)) {
    runtime.reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
        "Object is " + kindToString(Value(runtime, *this), &runtime) +
        ", expected a function"));
    return std::optional<Function>();
  }
  return std::move(*this).getFunction(runtime);
}

Value::Value(Value&& other) : Value(other.kind_) {
  if (kind_ == ValueKind::BooleanKind) {
    data_.boolean = other.data_.boolean;
  } else if (kind_ == ValueKind::NumberKind) {
    data_.number = other.data_.number;
  } else if (kind_ >= ValueKind::PointerKind) {
    new (&data_.pointer) Pointer(std::move(other.data_.pointer));
  }
  // when the other's dtor runs, nothing will happen.
  other.kind_ = ValueKind::UndefinedKind;
}

Value::Value(Runtime& runtime, const Value& other) : Value(other.kind_) {
  // data_ is uninitialized, so use placement new to create non-POD
  // types in it.  Any other kind of initialization will call a dtor
  // first, which is incorrect.
  if (kind_ == ValueKind::BooleanKind) {
    data_.boolean = other.data_.boolean;
  } else if (kind_ == ValueKind::NumberKind) {
    data_.number = other.data_.number;
  } else if (kind_ == ValueKind::SymbolKind) {
    new (&data_.pointer) Pointer(runtime.cloneSymbol(other.data_.pointer.ptr_));
  } else if (kind_ == ValueKind::StringKind) {
    new (&data_.pointer) Pointer(runtime.cloneString(other.data_.pointer.ptr_));
  } else if (kind_ >= ValueKind::ObjectKind) {
    new (&data_.pointer) Pointer(runtime.cloneObject(other.data_.pointer.ptr_));
  }
}

Value::~Value() {
  if (kind_ >= ValueKind::PointerKind) {
    data_.pointer.~Pointer();
  }
}

std::optional<Value> Value::createFromJsonString(Runtime& runtime,
                                                 String& string) {
  auto json_obj_opt = runtime.global().getPropertyAsObject(runtime, "JSON");
  if (!json_obj_opt) {
    runtime.reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
        "createFromJsonUtf8 error: try to get JSON object from JS global "
        "fail!"));
    return std::optional<Value>();
  }
  auto parse_func_opt = json_obj_opt->getPropertyAsFunction(runtime, "parse");
  if (!parse_func_opt) {
    runtime.reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
        "createFromJsonUtf8 error: try to get parse function from JSON object "
        "fail!"));
    return std::optional<Value>();
  }
  return parse_func_opt->call(runtime, string);
}

std::optional<Value> Value::createFromJsonUtf8(Runtime& runtime,
                                               const uint8_t* json,
                                               size_t length) {
  String string = String::createFromUtf8(runtime, json, length);
  return Value::createFromJsonString(runtime, string);
}

bool Value::strictEquals(Runtime& runtime, const Value& a, const Value& b) {
  if (a.kind_ != b.kind_) {
    return false;
  }
  switch (a.kind_) {
    case ValueKind::UndefinedKind:
    case ValueKind::NullKind:
      return true;
    case ValueKind::BooleanKind:
      return a.data_.boolean == b.data_.boolean;
    case ValueKind::NumberKind:
      return a.data_.number == b.data_.number;
    case ValueKind::SymbolKind:
      return runtime.strictEquals(static_cast<const Symbol&>(a.data_.pointer),
                                  static_cast<const Symbol&>(b.data_.pointer));
    case ValueKind::StringKind:
      return runtime.strictEquals(static_cast<const String&>(a.data_.pointer),
                                  static_cast<const String&>(b.data_.pointer));
    case ValueKind::ObjectKind:
      return runtime.strictEquals(static_cast<const Object&>(a.data_.pointer),
                                  static_cast<const Object&>(b.data_.pointer));
  }
}

std::optional<double> Value::asNumber(Runtime& rt) const {
  // TODO(wangqingyu): use base::unexpected here
  if (!isNumber()) {
    rt.reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
        "Value is " + kindToString(*this) + ", expected a number"));
    return std::optional<double>();
  }

  return getNumber();
}

std::optional<Object> Value::asObject(Runtime& rt) const& {
  // TODO(wangqingyu): use base::unexpected here
  if (!isObject()) {
    rt.reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
        "Value is " + kindToString(*this, &rt) + ", expected an Object"));
    return std::optional<Object>();
  }

  return getObject(rt);
}

std::optional<Object> Value::asObject(Runtime& rt) && {
  // TODO(wangqingyu): use base::unexpected here
  if (!isObject()) {
    rt.reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
        "Value is " + kindToString(*this, &rt) + ", expected an Object"));
    return std::optional<Object>();
  }
  auto ptr = data_.pointer.ptr_;
  data_.pointer.ptr_ = nullptr;
  return static_cast<Object>(ptr);
}

std::optional<Symbol> Value::asSymbol(Runtime& rt) const& {
  // TODO(wangqingyu): use base::unexpected here
  if (!isSymbol()) {
    rt.reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
        "Value is " + kindToString(*this, &rt) + ", expected a Symbol"));
    return std::optional<Symbol>();
  }

  return getSymbol(rt);
}

std::optional<Symbol> Value::asSymbol(Runtime& rt) && {
  // TODO(wangqingyu): use base::unexpected here
  if (!isSymbol()) {
    rt.reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
        "Value is " + kindToString(*this, &rt) + ", expected a Symbol"));
    return std::optional<Symbol>();
  }

  return std::move(*this).getSymbol(rt);
}

std::optional<String> Value::asString(Runtime& rt) const& {
  // TODO(wangqingyu): use base::unexpected here
  if (!isString()) {
    rt.reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
        "Value is " + kindToString(*this, &rt) + ", expected a String"));
    return std::optional<String>();
  }

  return getString(rt);
}

std::optional<String> Value::asString(Runtime& rt) && {
  // TODO(wangqingyu): use base::unexpected here
  if (!isString()) {
    rt.reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
        "Value is " + kindToString(*this, &rt) + ", expected a String"));
    return std::optional<String>();
  }

  return std::move(*this).getString(rt);
}

std::optional<String> Value::toString(Runtime& runtime) const {
  auto toString = runtime.global().getPropertyAsFunction(runtime, "String");
  if (!toString) {
    return std::optional<String>();
  }
  auto ret = toString->call(runtime, *this);
  if (!ret) {
    return std::optional<String>();
  }
  return ret->getString(runtime);
}

std::string Value::typeToString() const {
  switch (kind()) {
    case ValueKind::UndefinedKind:
      return "Undefined";
    case ValueKind::NullKind:
      return "Null";
    case ValueKind::NumberKind:
      return "Number";
    case ValueKind::SymbolKind:
      return "Symbol";
    case ValueKind::StringKind:
      return "String";
    case ValueKind::ObjectKind:
      return "Object";
    default:
      return "Unknown";
  }
}

std::optional<Value> Value::toJsonString(Runtime& runtime) const {
  auto json_obj_opt = runtime.global().getPropertyAsObject(runtime, "JSON");
  if (!json_obj_opt) {
    runtime.reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
        "Value::toJsonString error : try to get JSON object from js global "
        "fail!"));
    return std::optional<Value>();
  }
  auto stringify_func_opt =
      json_obj_opt->getPropertyAsFunction(runtime, "stringify");
  if (!stringify_func_opt) {
    runtime.reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
        "Value::toJsonString error : try to get stringify function from JSON "
        "object fail!"));
    return std::optional<Value>();
  }
  return stringify_func_opt->call(runtime, static_cast<const Value*>(this),
                                  static_cast<size_t>(1));
}

std::optional<BigInt> BigInt::createWithString(Runtime& rt,
                                               const std::string& value) {
  return rt.createBigInt(value, rt);
}

std::vector<PropNameID> HostObject::getPropertyNames(Runtime&) { return {}; }

Runtime::ScopeState* Runtime::pushScope() { return nullptr; }

void Runtime::popScope(ScopeState*) {}

JSError::JSError(Runtime& rt, Value&& value) { setValue(rt, std::move(value)); }

JSError::JSError(Runtime& rt, std::string message)
    : JSIException(std::move(message)) {}

JSError::JSError(Runtime& rt, std::string message, std::string stack)
    : JSError(std::move(message), std::move(stack)) {}

JSError::JSError(std::string message, std::string stack)
    : JSIException(std::move(message), std::move(stack),
                   error::E_BTS_RUNTIME_ERROR) {}

JSError::JSError(Runtime& rt, Value&& value, std::string message)
    : JSIException(std::move(message)) {
  setValue(rt, std::move(value));
}

void JSError::setValue(Runtime& rt, Value&& value) {
  value_ = std::make_shared<piper::Value>(std::move(value));
  // If another JSError happen when construct a JSError, it may enter a dead
  // loop. Return directly to avoid dead loop.
  if (rt.IsInJSErrorConstructionProcessing()) {
    message_ = "Another JS Error happened when construct a JS Error!";
    return;
  }
  JSError::Scope scope(rt);

  if ((message_.empty() || stack_.empty()) && value_->isObject()) {
    auto obj = value_->getObject(rt);

    auto name = obj.getProperty(rt, "name");
    if (name && name->isString()) {
      name_ = name->getString(rt).utf8(rt);
    }

    if (message_.empty()) {
      auto message = obj.getProperty(rt, "message");
      if (message && !message->isUndefined()) {
        auto msg = message->toString(rt);
        if (msg) {
          message_ = msg->utf8(rt);
        }
      }
    }

    if (stack_.empty()) {
      auto stack = obj.getProperty(rt, "stack");
      if (stack && !stack->isUndefined()) {
        auto stk = stack->toString(rt);
        if (stk) {
          stack_ = stk->utf8(rt);
        }
      }
    }
  }

  if (message_.empty()) {
    auto str = value_->toString(rt);
    if (str) {
      message_ = str->utf8(rt);
    }
  }
}

}  // namespace piper
}  // namespace lynx
