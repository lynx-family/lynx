/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the current directory.
 */
// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSI_JSI_INL_H_
#define CORE_RUNTIME_JSI_JSI_INL_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace lynx {
namespace piper {
namespace detail {

inline Value toValue(Runtime&, std::nullptr_t) { return Value::null(); }
inline Value toValue(Runtime&, bool b) { return Value(b); }
inline Value toValue(Runtime&, double d) { return Value(d); }
inline Value toValue(Runtime&, float f) {
  return Value(static_cast<double>(f));
}
inline Value toValue(Runtime&, int i) { return Value(i); }
inline Value toValue(Runtime& runtime, const char* str) {
  return String::createFromAscii(runtime, str);
}
inline Value toValue(Runtime& runtime, const std::string& str) {
  return String::createFromAscii(runtime, str);
}
template <typename T>
inline Value toValue(Runtime& runtime, const T& other) {
  static_assert(std::is_base_of<Pointer, T>::value,
                "This type cannot be converted to Value");
  return Value(runtime, other);
}
inline Value toValue(Runtime& runtime, const Value& value) {
  return Value(runtime, value);
}
inline Value&& toValue(Runtime&, Value&& value) { return std::move(value); }

inline PropNameID toPropNameID(Runtime& runtime, const char* name) {
  return PropNameID::forAscii(runtime, name);
}
inline PropNameID toPropNameID(Runtime& runtime, const std::string& name) {
  return PropNameID::forUtf8(runtime, name);
}
inline PropNameID&& toPropNameID(Runtime&, PropNameID&& name) {
  return std::move(name);
}

}  // namespace detail

template <typename T>
inline T Runtime::make(Runtime::PointerValue* pv) {
  return T(pv);
}

inline const Runtime::PointerValue* Runtime::getPointerValue(
    const piper::Pointer& pointer) {
  return pointer.ptr_;
}

inline const Runtime::PointerValue* Runtime::getPointerValue(
    const piper::Value& value) {
  return value.data_.pointer.ptr_;
}

inline std::optional<Value> Object::getProperty(Runtime& runtime,
                                                const char* name) const {
  return getProperty(runtime, String::createFromAscii(runtime, name));
}

inline std::optional<Value> Object::getProperty(Runtime& runtime,
                                                const String& name) const {
  return runtime.getProperty(*this, name);
}

inline std::optional<Value> Object::getProperty(Runtime& runtime,
                                                const PropNameID& name) const {
  return runtime.getProperty(*this, name);
}

inline bool Object::hasProperty(Runtime& runtime, const char* name) const {
  return hasProperty(runtime, String::createFromAscii(runtime, name));
}

inline bool Object::hasProperty(Runtime& runtime, const String& name) const {
  return runtime.hasProperty(*this, name);
}

inline bool Object::hasProperty(Runtime& runtime,
                                const PropNameID& name) const {
  return runtime.hasProperty(*this, name);
}

template <typename T>
bool Object::setProperty(Runtime& runtime, const char* name, T&& value) {
  return setProperty(runtime, String::createFromAscii(runtime, name),
                     std::forward<T>(value));
}

template <typename T>
bool Object::setProperty(Runtime& runtime, const String& name, T&& value) {
  return setPropertyValue(runtime, name,
                          detail::toValue(runtime, std::forward<T>(value)));
}

template <typename T>
bool Object::setProperty(Runtime& runtime, const PropNameID& name, T&& value) {
  return setPropertyValue(runtime, name,
                          detail::toValue(runtime, std::forward<T>(value)));
}

inline Array Object::getArray(Runtime& runtime) const& {
  DCHECK(runtime.isArray(*this));
  (void)runtime;  // when assert is disabled we need to mark this as used
  return Array(runtime.cloneObject(ptr_));
}

inline Array Object::getArray(Runtime& runtime) && {
  DCHECK(runtime.isArray(*this));
  (void)runtime;  // when assert is disabled we need to mark this as used
  Runtime::PointerValue* value = ptr_;
  ptr_ = nullptr;
  return Array(value);
}

inline BigInt Object::getBigInt(Runtime& runtime) && {
  (void)runtime;  // when assert is disabled we need to mark this as used
  Runtime::PointerValue* value = ptr_;
  ptr_ = nullptr;
  return BigInt(value);
}

inline ArrayBuffer Object::getArrayBuffer(Runtime& runtime) const& {
  DCHECK(runtime.isArrayBuffer(*this));
  (void)runtime;  // when assert is disabled we need to mark this as used
  return ArrayBuffer(runtime.cloneObject(ptr_));
}

inline ArrayBuffer Object::getArrayBuffer(Runtime& runtime) && {
  DCHECK(runtime.isArrayBuffer(*this));
  (void)runtime;  // when assert is disabled we need to mark this as used
  Runtime::PointerValue* value = ptr_;
  ptr_ = nullptr;
  return ArrayBuffer(value);
}

inline Function Object::getFunction(Runtime& runtime) const& {
  DCHECK(runtime.isFunction(*this));
  return Function(runtime.cloneObject(ptr_));
}

inline Function Object::getFunction(Runtime& runtime) && {
  DCHECK(runtime.isFunction(*this));
  (void)runtime;  // when assert is disabled we need to mark this as used
  Runtime::PointerValue* value = ptr_;
  ptr_ = nullptr;
  return Function(value);
}

template <typename T>
inline bool Object::isHostObject(Runtime& runtime) const {
  return runtime.isHostObject(*this) &&
         std::dynamic_pointer_cast<T>(runtime.getHostObject(*this));
}

template <>
inline bool Object::isHostObject<HostObject>(Runtime& runtime) const {
  return runtime.isHostObject(*this);
}

template <typename T>
inline std::weak_ptr<T> Object::getHostObject(Runtime& runtime) const {
  DCHECK(isHostObject<T>(runtime));
  return std::static_pointer_cast<T>(runtime.getHostObject(*this));
}

template <>
inline std::weak_ptr<HostObject> Object::getHostObject<HostObject>(
    Runtime& runtime) const {
  DCHECK(runtime.isHostObject(*this));
  return runtime.getHostObject(*this);
}

inline std::optional<Array> Object::getPropertyNames(Runtime& runtime) const {
  return runtime.getPropertyNames(*this);
}

template <typename T>
bool Array::setValueAtIndex(Runtime& runtime, size_t i, T&& value) {
  return setValueAtIndexImpl(runtime, i,
                             detail::toValue(runtime, std::forward<T>(value)));
}

inline std::optional<Value> Array::getValueAtIndex(Runtime& runtime,
                                                   size_t i) const {
  return runtime.getValueAtIndex(*this, i);
}

inline Function Function::createFromHostFunction(Runtime& runtime,
                                                 const piper::PropNameID& name,
                                                 unsigned int paramCount,
                                                 piper::HostFunctionType func) {
  return runtime.createFunctionFromHostFunction(name, paramCount,
                                                std::move(func));
}

inline std::optional<Value> Function::call(Runtime& runtime, const Value* args,
                                           size_t count) const {
  return runtime.call(*this, Value::undefined(), args, count);
}

inline std::optional<Value> Function::call(
    Runtime& runtime, std::initializer_list<Value> args) const {
  return call(runtime, args.begin(), args.size());
}

template <typename... Args>
inline std::optional<Value> Function::call(Runtime& runtime,
                                           Args&&... args) const {
  // A more awesome version of this would be able to create raw values
  // which can be used directly without wrapping and unwrapping, but
  // this will do for now.
  return call(runtime, {detail::toValue(runtime, std::forward<Args>(args))...});
}

inline std::optional<Value> Function::callWithThis(Runtime& runtime,
                                                   const Object& jsThis,
                                                   const Value* args,
                                                   size_t count) const {
  return runtime.call(*this, Value(runtime, jsThis), args, count);
}

inline std::optional<Value> Function::callWithThis(
    Runtime& runtime, const Object& jsThis,
    std::initializer_list<Value> args) const {
  return callWithThis(runtime, jsThis, args.begin(), args.size());
}

template <typename... Args>
inline std::vector<PropNameID> PropNameID::names(Runtime& runtime,
                                                 Args&&... args) {
  return names({detail::toPropNameID(runtime, std::forward<Args>(args))...});
}

template <size_t N>
inline std::vector<PropNameID> PropNameID::names(
    PropNameID (&&propertyNames)[N]) {
  std::vector<PropNameID> result;
  result.reserve(N);
  for (auto& name : propertyNames) {
    result.push_back(std::move(name));
  }
  return result;
}

inline std::optional<Value> Function::callAsConstructor(Runtime& runtime,
                                                        const Value* args,
                                                        size_t count) const {
  return runtime.callAsConstructor(*this, args, count);
}

inline std::optional<Value> Function::callAsConstructor(
    Runtime& runtime, std::initializer_list<Value> args) const {
  return callAsConstructor(runtime, args.begin(), args.size());
}

template <typename... Args>
inline std::optional<Value> Function::callAsConstructor(Runtime& runtime,
                                                        Args&&... args) const {
  return callAsConstructor(
      runtime, {detail::toValue(runtime, std::forward<Args>(args))...});
}

}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_JSI_JSI_INL_H_
