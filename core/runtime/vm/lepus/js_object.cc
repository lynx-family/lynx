// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/vm/lepus/js_object.h"

namespace lynx {
namespace lepus {

LEPUSObject::LEPUSObject(std::shared_ptr<JSIObjectProxy> lepus_obj_proxy)
    : jsi_object_proxy_(std::move(lepus_obj_proxy)) {}

LEPUSObject::JSIObjectProxy::JSIObjectProxy(int64_t id) : jsi_object_id_(id) {}

std::shared_ptr<LEPUSObject::JSIObjectProxy> LEPUSObject::jsi_object_proxy() {
  return jsi_object_proxy_;
}

void LEPUSObject::ReleaseSelf() const { delete this; }

int64_t LEPUSObject::JSIObjectID() {
  return jsi_object_proxy_ ? jsi_object_proxy_->jsi_object_id() : -1;
}

bool operator==(const LEPUSObject& left, const LEPUSObject& right) {
  if (!left.jsi_object_proxy_ || !right.jsi_object_proxy_) {
    return (!left.jsi_object_proxy_ && !right.jsi_object_proxy_) ? true : false;
  } else {
    return left.jsi_object_proxy_->jsi_object_id() ==
           right.jsi_object_proxy_->jsi_object_id();
  }
}

}  // namespace lepus
}  // namespace lynx
