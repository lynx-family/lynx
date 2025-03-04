// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "third_party/binding/common/object.h"

#include "third_party/binding/common/object_ref.h"

namespace lynx {
namespace binding {

ObjectImpl::~ObjectImpl() = default;

ObjectRef Object::AdoptRef() {
  if (!impl_) {
    return ObjectRef();
  }
  return ObjectRef(impl_->AdoptRef());
}

}  // namespace binding
}  // namespace lynx
