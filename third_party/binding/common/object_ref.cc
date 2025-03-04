// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "third_party/binding/common/object_ref.h"

namespace lynx {
namespace binding {

ObjectRefImpl::~ObjectRefImpl() = default;

ObjectRef::ObjectRef(ObjectRef&& other) : impl_(std::move(other.impl_)) {}

ObjectRef& ObjectRef::operator=(ObjectRef&& other) {
  impl_ = std::move(other.impl_);
  return *this;
}

Object ObjectRef::Get() const {
  if (!impl_) {
    return Object::CreateEmpty();
  }
  return impl_->Get();
}

ObjectRef ObjectRef::Clone() const {
  if (!impl_) {
    return ObjectRef();
  }
  return ObjectRef(impl_->Dup());
}

}  // namespace binding
}  // namespace lynx
