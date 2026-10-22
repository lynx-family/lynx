// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/runtime/host_script_value_snapshot.h"

#include <string>
#include <utility>
#include <vector>

#include "base/include/value/array.h"
#include "base/include/value/table.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "core/value_wrapper/value_impl_piper.h"

namespace lynx::shell {
namespace {
using runtime::js::Array;
using runtime::js::Object;
using runtime::js::Runtime;
using runtime::js::String;
using runtime::js::Value;

constexpr size_t kMaxDepth = 64;

lepus::Value CopyPrimitive(Runtime& rt, const Value& value) {
  return pub::ValueUtils::ConvertValueToLepusValue(
      pub::ValueImplPiper(rt, value));
}

bool HasPlainPrototype(Runtime& rt, const Value& value) {
  auto object_class = rt.global().getPropertyAsObject(rt, "Object");
  if (!object_class) return false;
  auto get_prototype =
      object_class->getPropertyAsFunction(rt, "getPrototypeOf");
  if (!get_prototype) return false;
  auto prototype = get_prototype->call(rt, {Value(rt, value)});
  auto plain = object_class->getProperty(rt, "prototype");
  return prototype && plain &&
         (prototype->isNull() || Value::strictEquals(rt, *prototype, *plain));
}

// Any failure abandons the whole snapshot, so the ancestor stack is not
// unwound on failure.
class Snapshot {
 public:
  explicit Snapshot(Runtime& rt) : rt_(rt) {}

  std::optional<lepus::Value> Copy(const Value& value) {
    if (value.isSymbol()) return std::nullopt;
    if (!value.isObject()) return CopyPrimitive(rt_, value);
    auto object = value.getObject(rt_);
    if (object.isFunction(rt_) || object.isHostObject(rt_) ||
        object.hasProperty(rt_, "then")) {
      return std::nullopt;
    }
    if (object.isArrayBuffer(rt_)) return CopyPrimitive(rt_, value);
    return CopyContainer(value, object);
  }

 private:
  // Inspect descriptors without invoking accessors. A rejected snapshot must
  // not have already executed getters that native argument conversion will
  // execute again. Inherited array entries also stay opaque.
  std::optional<Value> DataProperty(Object& object, const String& name) {
    if (!get_descriptor_) {
      auto object_class = rt_.global().getPropertyAsObject(rt_, "Object");
      if (!object_class) return std::nullopt;
      get_descriptor_ =
          object_class->getPropertyAsFunction(rt_, "getOwnPropertyDescriptor");
      if (!get_descriptor_) return std::nullopt;
    }
    auto descriptor =
        get_descriptor_->call(rt_, {Value(rt_, object), Value(rt_, name)});
    if (!descriptor) return std::nullopt;
    if (descriptor->isUndefined()) {
      return object.hasProperty(rt_, name)
                 ? std::nullopt
                 : std::optional<Value>(Value::undefined());
    }
    if (!descriptor->isObject()) return std::nullopt;
    auto data = descriptor->getObject(rt_);
    // Accessor descriptors always own get/set. Check them before value, which
    // an accessor descriptor could otherwise inherit from Object.prototype.
    if (data.hasProperty(rt_, "get") || data.hasProperty(rt_, "set") ||
        !data.hasProperty(rt_, "value")) {
      return std::nullopt;
    }
    return data.getProperty(rt_, "value");
  }

  std::optional<lepus::Value> CopyContainer(const Value& value,
                                            Object& object) {
    const bool is_array = object.isArray(rt_);
    if (!is_array && !HasPlainPrototype(rt_, value)) return std::nullopt;
    if (ancestors_.size() >= kMaxDepth) return std::nullopt;
    for (const auto& ancestor : ancestors_) {
      if (Object::strictEquals(rt_, ancestor, object)) return std::nullopt;
    }
    // Arrays visit every index; objects visit their enumerable own keys.
    auto keys = is_array ? std::optional<Array>(object.getArray(rt_))
                         : object.getPropertyNames(rt_);
    auto size = keys ? keys->size(rt_) : std::nullopt;
    if (!size) return std::nullopt;

    auto copy = is_array ? lepus::Value(lepus::CArray::Create())
                         : lepus::Value(lepus::Dictionary::Create());
    if (!is_array) copy.Table()->reserve(*size);
    ancestors_.push_back(value.getObject(rt_));
    for (size_t i = 0; i < *size; ++i) {
      auto entry = is_array ? std::optional<Value>(Value(String::createFromUtf8(
                                  rt_, std::to_string(i))))
                            : keys->getValueAtIndex(rt_, i);
      if (!entry || !entry->isString()) return std::nullopt;
      auto name = entry->getString(rt_);
      auto property = DataProperty(object, name);
      if (!property) return std::nullopt;
      auto child = Copy(*property);
      if (!child) return std::nullopt;
      if (is_array) {
        copy.Array()->push_back(std::move(*child));
      } else {
        copy.Table()->SetValue(name.utf8(rt_), std::move(*child));
      }
    }
    ancestors_.pop_back();
    return copy;
  }

  Runtime& rt_;
  std::optional<runtime::js::Function> get_descriptor_;
  std::vector<Object> ancestors_;
};

bool CanSnapshotAt(const pub::Value& value, size_t depth) {
  if (value.IsTransfer() || value.IsLynxObject() || value.IsTemplateData() ||
      value.IsFunction()) {
    return false;
  }
  if (!value.IsArray() && !value.IsMap()) {
    return value.IsUndefined() || value.IsNil() || value.IsBool() ||
           value.IsString() || value.IsNumber() || value.IsArrayBuffer();
  }
  if (depth >= kMaxDepth) return false;
  bool copyable = true;
  if (value.IsArray()) {
    value.ForeachArray([&](int64_t, const pub::Value& child) {
      copyable = copyable && CanSnapshotAt(child, depth + 1);
    });
  } else {
    value.ForeachMap([&](const pub::Value& key, const pub::Value& child) {
      copyable = copyable && key.IsString() && CanSnapshotAt(child, depth + 1);
    });
  }
  return copyable;
}
}  // namespace

std::optional<lepus::Value> SnapshotJSValue(Runtime& rt, const Value& value) {
  return Snapshot(rt).Copy(value);
}

bool CanSnapshot(const pub::Value& value) { return CanSnapshotAt(value, 0); }

Value RestoreJSValue(Runtime& rt, const lepus::Value& value) {
  return pub::ValueUtils::ConvertValueToPiperValue(rt,
                                                   pub::ValueImplLepus(value));
}

}  // namespace lynx::shell
