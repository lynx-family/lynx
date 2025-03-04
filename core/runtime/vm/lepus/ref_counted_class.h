// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_VM_LEPUS_REF_COUNTED_CLASS_H_
#define CORE_RUNTIME_VM_LEPUS_REF_COUNTED_CLASS_H_

#include <optional>

#include "base/include/fml/memory/ref_counted.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/ref_type.h"

extern "C" {
#include "quickjs/include/quickjs.h"
}

namespace lynx {
namespace lepus {
class RefCounted : public fml::RefCountedThreadSafeStorage {
 public:
  static LEPUSClassExoticMethods exotic_methods_;
  constexpr static const char* class_name = "__lepus_RefCounted__";
  static LEPUSClassDef ref_counted_class_def_;

  static void InitRefCountedClass(LEPUSRuntime*);
  static LEPUSClassID GetClassID() { return class_id; }

  void ReleaseSelf() const override { delete this; }
  ~RefCounted() override = default;

  std::optional<lepus::Value> js_object_cache = std::nullopt;
  virtual bool IsConst() const { return false; }

  /*
   * Return RefType of this RefCounted.
   * ByteArray in core/runtime/vm/lepus/byte_array.h,
   * Value_JSOBject in core/runtime/vm/lepus/js_object.h,
   * Element in core/renderer/dom/element.h,
   * AirElement in core/renderer/dom/air/air_element/air_element.h
   */
  virtual RefType GetRefType() const = 0;

 protected:
  RefCounted() = default;
  static inline LEPUSClassID class_id = 0;
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_REF_COUNTED_CLASS_H_
