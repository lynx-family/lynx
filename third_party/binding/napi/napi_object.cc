// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "third_party/binding/napi/napi_object.h"

#include "third_party/binding/napi/napi_object_ref.h"

namespace lynx {
namespace binding {

std::unique_ptr<ObjectRefImpl> NapiObjectImpl::AdoptRef() const {
  return std::unique_ptr<ObjectRefImpl>(new NapiObjectRefImpl(obj_));
}

std::unique_ptr<ObjectImpl> NapiObjectImpl::ShallowCopy() const {
  return std::unique_ptr<ObjectImpl>(new NapiObjectImpl(obj_));
}

Object FromNAPI(Napi::Object object) {
  if (object.IsEmpty()) {
    return Object::CreateEmpty();
  }
  return Object(std::unique_ptr<ObjectImpl>(new NapiObjectImpl(object)));
}

Napi::Object ToNAPI(Object object) {
  if (!object.IsNapi()) {
    return Napi::Object();
  }
  return static_cast<NapiObjectImpl*>(object.GetImpl())->obj_;
}

}  // namespace binding
}  // namespace lynx
