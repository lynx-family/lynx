// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "third_party/binding/napi/napi_object_ref.h"

#include "third_party/binding/napi/napi_object.h"

namespace lynx {
namespace binding {

ObjectRef NapiObjectRefImpl::Elevate(Napi::ObjectReference& weak_reference) {
  weak_reference.Ref();
  Napi::ObjectReference strong_reference = std::move(weak_reference);
  weak_reference.Reset(strong_reference.Value());
  return ObjectRef(std::unique_ptr<ObjectRefImpl>(
      new NapiObjectRefImpl(std::move(strong_reference))));
}

Object NapiObjectRefImpl::Get() const { return FromNAPI(ref_.Value()); }

std::unique_ptr<ObjectRefImpl> NapiObjectRefImpl::Dup() {
  return std::unique_ptr<ObjectRefImpl>(new NapiObjectRefImpl(ref_.Value()));
}

}  // namespace binding
}  // namespace lynx
