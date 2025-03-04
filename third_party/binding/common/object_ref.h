// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BINDING_COMMON_OBJECT_REF_H_
#define BINDING_COMMON_OBJECT_REF_H_

#include "third_party/binding/common/object.h"

namespace lynx {
namespace binding {

class ObjectRefImpl {
 public:
  virtual ~ObjectRefImpl() = 0;
  virtual Object Get() const = 0;
  virtual std::unique_ptr<ObjectRefImpl> Dup() = 0;
};

// An empty or strong reference (ref count == 1) to an object.
class ObjectRef {
 public:
  ObjectRef() = default;
  explicit ObjectRef(std::unique_ptr<ObjectRefImpl> impl)
      : impl_(std::move(impl)) {}
  ObjectRef(ObjectRef&& other);
  ObjectRef& operator=(ObjectRef&& other);

  bool IsEmpty() const { return !impl_; }
  Object Get() const;
  ObjectRef Clone() const;
  void Unref() { impl_.reset(); }

 private:
  std::unique_ptr<ObjectRefImpl> impl_;
};

}  // namespace binding
}  // namespace lynx

#endif  // BINDING_COMMON_OBJECT_REF_H_
