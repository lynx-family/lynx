// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/clay_value.h"

namespace lynx {

bool ClayValueWrapper::IsNumber() const {
  switch (backend_value_.type()) {
    case clay::Value::kInt:
    case clay::Value::kLong:
    case clay::Value::kUInt:
    case clay::Value::kFloat:
    case clay::Value::kDouble:
      return true;
    default:
      return false;
  }
}

double ClayValueWrapper::Number() const {
  switch (backend_value_.type()) {
    case clay::Value::kInt:
      return backend_value_.GetInt();
    case clay::Value::kLong:
      return backend_value_.GetLong();
    case clay::Value::kUInt:
      return backend_value_.GetUint();
    case clay::Value::kFloat:
      return backend_value_.GetFloat();
    case clay::Value::kDouble:
      return backend_value_.GetDouble();
    default:
      return 0;
  }
}

const std::string& ClayValueWrapper::str() const {
  if (backend_value_.IsString()) {
    return backend_value_.GetString();
  } else if (backend_value_.IsBool()) {
    static std::string true_str("true");
    static std::string false_str("false");
    return backend_value_.GetBool() ? true_str : false_str;
  }
  // When backend_value_ is not a string type, the value returned by String()
  // method will be free after leaving this function scope. Returning
  // String()->str() directly will result in heap-use-after-free error.
  // So we return a static string here.
  static std::string empty("");
  return empty;
}
int ClayValueWrapper::Length() const {
  if (backend_value_.IsMap()) {
    return backend_value_.GetMap().size();
  } else if (backend_value_.IsArray()) {
    return backend_value_.GetArray().size();
  } else if (backend_value_.IsString()) {
    return backend_value_.GetString().size();
  }
  return 0;
}

bool ClayValueWrapper::IsEqual(const pub::Value& value) const {
  if (value.backend_type() != pub::ValueBackendType::ValueBackendTypeCustom) {
    return false;
  }
  auto that = reinterpret_cast<const ClayValueWrapper&>(value);
  if (backend_value_.type() != that.backend_value_.type()) {
    return false;
  }
  switch (backend_value_.type()) {
    case clay::Value::kNone:
      return true;
    case clay::Value::kPointer:
      return backend_value_.GetPointerType() ==
                 that.backend_value_.GetPointerType() &&
             backend_value_.GetPointer() == that.backend_value_.GetPointer();
    case clay::Value::kBool:
      return backend_value_.GetBool() == that.backend_value_.GetBool();
    case clay::Value::kInt:
      return backend_value_.GetInt() == that.backend_value_.GetInt();
    case clay::Value::kLong:
      return backend_value_.GetLong() == that.backend_value_.GetLong();
    case clay::Value::kUInt:
      return backend_value_.GetUint() == that.backend_value_.GetUint();
    case clay::Value::kFloat:
      return backend_value_.GetFloat() == that.backend_value_.GetFloat();
    case clay::Value::kDouble:
      return backend_value_.GetDouble() == that.backend_value_.GetDouble();
    case clay::Value::kString:
      return backend_value_.GetString() == that.backend_value_.GetString();
    case clay::Value::kArray:
      return &backend_value_.GetArray() == &that.backend_value_.GetArray();
    case clay::Value::kArrayBuffer:
      return &backend_value_.GetArrayBuffer() ==
             &that.backend_value_.GetArrayBuffer();
    case clay::Value::kMap:
      return &backend_value_.GetMap() == &that.backend_value_.GetMap();
    default:
      return false;
  }
}

void ClayValueWrapper::ForeachArray(pub::ForeachArrayFunc func) const {
  if (!backend_value_.IsArray()) {
    return;
  }
  for (size_t i = 0; i < backend_value_.GetArray().size(); ++i) {
    ClayValueWrapper impl_value(backend_value_.GetArray()[i]);
    func(i, impl_value);
  }
}

void ClayValueWrapper::ForeachMap(pub::ForeachMapFunc func) const {
  if (!backend_value_.IsMap()) {
    return;
  }
  for (const auto& [key, value] : backend_value_.GetMap()) {
    clay::Value name{key.c_str()};
    ClayValueWrapper impl_key(name);
    ClayValueWrapper impl_value(value);
    func(impl_key, impl_value);
  }
}

std::unique_ptr<pub::Value> ClayValueWrapper::GetValueAtIndex(
    uint32_t idx) const {
  if (backend_value_.IsArray()) {
    const auto& array = backend_value_.GetArray();
    if (idx < array.size()) {
      return wrap(array[idx]);
    }
  }
  // Returns an empty Value if it's not a array to keep consistent with
  // clay::Value
  return std::make_unique<ClayValueWrapper>(clay::Value{});
}

bool ClayValueWrapper::Erase(uint32_t idx) const {
  if (backend_value_.IsArray()) {
    auto& array = const_cast<clay::Value::Array&>(backend_value_.GetArray());
    if (idx < array.size()) {
      array.erase(array.begin() + idx);
      return true;
    }
  }
  return false;
}

std::unique_ptr<pub::Value> ClayValueWrapper::GetValueForKey(
    const std::string& key) const {
  if (backend_value_.IsMap()) {
    auto it = backend_value_.GetMap().find(key);
    if (it != backend_value_.GetMap().end()) {
      return wrap(it->second);
    }
  }
  // Returns an empty Value if it's not a map to keep consistent with
  // clay::Value
  return std::make_unique<ClayValueWrapper>(clay::Value{});
}

bool ClayValueWrapper::Erase(const std::string& key) const {
  if (backend_value_.IsMap()) {
    auto& map = const_cast<clay::Value::Map&>(backend_value_.GetMap());
    auto it = map.find(key);
    if (it != map.end()) {
      map.erase(it);
      return true;
    }
  }
  return false;
}

bool ClayValueWrapper::Contains(const std::string& key) const {
  if (backend_value_.IsMap()) {
    auto it = backend_value_.GetMap().find(key);
    if (it != backend_value_.GetMap().end()) {
      return true;
    }
  }
  return false;
}

bool ClayValueWrapper::PushValueToArray(const Value& value) {
  if (!backend_value_.IsArray()) {
    return false;
  }
  auto& array = const_cast<clay::Value::Array&>(backend_value_.GetArray());
  array.push_back(ValueConverter::CreateClayValue(value));
  return true;
}

bool ClayValueWrapper::PushValueToArray(std::unique_ptr<Value> value) {
  return PushValueToArray(*value);
}

bool ClayValueWrapper::PushNullToArray() {
  if (!backend_value_.IsArray()) {
    return false;
  }
  auto& array = const_cast<clay::Value::Array&>(backend_value_.GetArray());
  array.push_back(clay::Value::Null());
  return true;
}

bool ClayValueWrapper::PushArrayBufferToArray(std::unique_ptr<uint8_t[]> value,
                                              size_t length) {
  if (!backend_value_.IsArray()) {
    return false;
  }
  auto& array = const_cast<clay::Value::Array&>(backend_value_.GetArray());
  uint8_t* mem = value.get();
  array.push_back(clay::Value(clay::Value::ArrayBuffer(mem, mem + length)));
  return true;
}

bool ClayValueWrapper::PushBigIntToArray(const std::string& value) {
  if (!IsArray()) {
    return false;
  }
  auto& array = const_cast<clay::Value::Array&>(backend_value_.GetArray());
  auto int_value =
      static_cast<int64_t>(std::strtoll(value.c_str(), nullptr, 0));
  array.push_back(clay::Value(int_value));
  return true;
}

bool ClayValueWrapper::PushUInt64ToArray(uint64_t value) {
  return PushInt64ToArray(static_cast<int64_t>(value));
}

#define NormalTypePushArrayImpl(name, type)                                   \
  bool ClayValueWrapper::Push##name##ToArray(type value) {                    \
    if (!backend_value_.IsArray()) {                                          \
      return false;                                                           \
    }                                                                         \
    auto& array = const_cast<clay::Value::Array&>(backend_value_.GetArray()); \
    array.push_back(clay::Value(value));                                      \
    return true;                                                              \
  }
ClayDeclarationTypeList(NormalTypePushArrayImpl)
#undef NormalTypePushArrayImpl

    bool ClayValueWrapper::PushValueToMap(const std::string& key,
                                          const Value& value) {
  if (!backend_value_.IsMap()) {
    return false;
  }
  auto& map = const_cast<clay::Value::Map&>(backend_value_.GetMap());
  map[key] = ValueConverter::CreateClayValue(value);
  return true;
}

bool ClayValueWrapper::PushValueToMap(const std::string& key,
                                      std::unique_ptr<Value> value) {
  return PushValueToMap(key, *value);
}

bool ClayValueWrapper::PushNullToMap(const std::string& key) {
  if (!backend_value_.IsMap()) {
    return false;
  }
  auto& map = const_cast<clay::Value::Map&>(backend_value_.GetMap());
  map[key] = clay::Value::Null();
  return true;
}

bool ClayValueWrapper::PushArrayBufferToMap(const std::string& key,
                                            std::unique_ptr<uint8_t[]> value,
                                            size_t length) {
  if (!backend_value_.IsMap()) {
    return false;
  }
  auto& map = const_cast<clay::Value::Map&>(backend_value_.GetMap());
  uint8_t* mem = value.get();
  map[key] = clay::Value(clay::Value::ArrayBuffer(mem, mem + length));
  return true;
}

bool ClayValueWrapper::PushUInt64ToMap(const std::string& key, uint64_t value) {
  if (!backend_value_.IsMap()) {
    return false;
  }
  auto& map = const_cast<clay::Value::Map&>(backend_value_.GetMap());
  map[key] = clay::Value(static_cast<int64_t>(value));
  return true;
}

#define NormalTypePushMapImpl(name, type)                               \
  bool ClayValueWrapper::Push##name##ToMap(const std::string& key,      \
                                           type value) {                \
    if (!backend_value_.IsMap()) {                                      \
      return false;                                                     \
    }                                                                   \
    auto& map = const_cast<clay::Value::Map&>(backend_value_.GetMap()); \
    map[key] = clay::Value(value);                                      \
    return true;                                                        \
  }
ClayDeclarationTypeList(NormalTypePushMapImpl)
#undef NormalTypePushMapImpl

    // static
    std::unique_ptr<pub::Value> ClayValueWrapper::wrap(const clay::Value& val) {
  switch (val.type()) {
    case clay::Value::kNone:
    case clay::Value::kPointer:
      return std::make_unique<ClayValue>(clay::Value::Null());
    case clay::Value::kBool:
      return std::make_unique<ClayValue>(clay::Value(val.GetBool()));
    case clay::Value::kInt:
      return std::make_unique<ClayValue>(clay::Value(val.GetInt()));
    case clay::Value::kLong:
      return std::make_unique<ClayValue>(clay::Value(val.GetLong()));
    case clay::Value::kUInt:
      return std::make_unique<ClayValue>(clay::Value(val.GetUint()));
    case clay::Value::kFloat:
      return std::make_unique<ClayValue>(clay::Value(val.GetFloat()));
    case clay::Value::kDouble:
      return std::make_unique<ClayValue>(clay::Value(val.GetDouble()));
    case clay::Value::kString:
      return std::make_unique<ClayValue>(clay::Value(val.GetString()));
    case clay::Value::kArray:
      return std::make_unique<ClayValue>(
          clay::Value(val.value<std::shared_ptr<clay::Value::Array>>()));
    case clay::Value::kArrayBuffer:
      return std::make_unique<ClayValue>(
          clay::Value(val.value<std::shared_ptr<clay::Value::ArrayBuffer>>()));
    case clay::Value::kMap:
      return std::make_unique<ClayValue>(
          clay::Value(val.value<std::shared_ptr<clay::Value::Map>>()));
  }
  return nullptr;
}

}  // namespace lynx
