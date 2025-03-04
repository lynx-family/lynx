// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_JS_OBJECT_H_
#define CORE_RUNTIME_VM_LEPUS_JS_OBJECT_H_

#include <memory>
#include <unordered_map>
#include <utility>

#include "core/runtime/vm/lepus/ref_counted_class.h"
#include "core/runtime/vm/lepus/ref_type.h"

namespace lynx {
namespace lepus {

class LEPUSObject : public lepus::RefCounted {
 public:
  class JSIObjectProxy {
   public:
    JSIObjectProxy(int64_t id);
    virtual ~JSIObjectProxy() = default;
    int64_t jsi_object_id() { return jsi_object_id_; }

   protected:
    int64_t jsi_object_id_;
  };
  static fml::RefPtr<LEPUSObject> Create() {
    return fml::AdoptRef<LEPUSObject>(new LEPUSObject());
  }
  static fml::RefPtr<LEPUSObject> Create(
      std::shared_ptr<JSIObjectProxy> proxy) {
    return fml::AdoptRef<LEPUSObject>(new LEPUSObject(std::move(proxy)));
  }

  RefType GetRefType() const override { return RefType::kJSIObject; };

  std::shared_ptr<LEPUSObject::JSIObjectProxy> jsi_object_proxy();

  void ReleaseSelf() const override;

  // return -1 if no JSIObjectProxy
  int64_t JSIObjectID();

  friend bool operator==(const LEPUSObject& left, const LEPUSObject& right);

  friend bool operator!=(const LEPUSObject& left, const LEPUSObject& right) {
    return !(left == right);
  }

 protected:
  LEPUSObject() = default;
  LEPUSObject(std::shared_ptr<JSIObjectProxy> lepus_obj_proxy);

 private:
  std::shared_ptr<JSIObjectProxy> jsi_object_proxy_;
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_JS_OBJECT_H_
