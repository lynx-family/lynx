// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_VALUE_REF_COUNTED_CLASS_H_
#define BASE_INCLUDE_VALUE_REF_COUNTED_CLASS_H_

#include <memory>

#include "base/include/fml/memory/ref_counted.h"
#include "base/include/value/base_value.h"
#include "base/include/value/ref_type.h"

namespace lynx {
namespace lepus {
class RefCountedBase : public fml::RefCountedThreadSafeStorage {
 public:
  void ReleaseSelf() const override { delete this; }
  virtual void visitor(void* rt, void* mark_func, uint64_t trace_tool) {}
  ~RefCountedBase() override = default;

  virtual bool IsConst() const { return false; }

  /*
   * Return RefType of this RefCountedBase.
   * ByteArray in base/include/value/byte_array.h,
   * Value_JSOBject in core/runtime/vm/lepus/js_object.h,
   * Element in core/renderer/dom/element.h,
   * AirElement in core/renderer/dom/air/air_element/air_element.h
   */
  virtual RefType GetRefType() const = 0;

 protected:
  RefCountedBase() = default;
};

class RefCounted : public RefCountedBase {
 public:
  virtual fml::RefPtr<RefCounted> Clone() {
    return fml::RefPtr<RefCounted>(nullptr);
  }

  virtual void Print(std::ostream& output) {}

  virtual bool Equals(const fml::RefPtr<RefCounted>& other) {
    return this == other.get();
  }

  std::unique_ptr<Value> js_object_cache;
};

}  // namespace lepus

namespace fml {
// Specialized for lepus::RefCounted to solve compiling issues.
template <typename D>
WeakRefPtr<D> static_ref_ptr_cast(const WeakRefPtr<lepus::RefCounted>& rhs) {
  return WeakRefPtr<D>(static_cast<D*>(rhs.get()));
}
}  // namespace fml

}  // namespace lynx

#endif  // BASE_INCLUDE_VALUE_REF_COUNTED_CLASS_H_
