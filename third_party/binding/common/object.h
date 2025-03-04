// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BINDING_COMMON_OBJECT_H_
#define BINDING_COMMON_OBJECT_H_

#include <memory>

namespace lynx {
namespace binding {

class ObjectRef;
class ObjectRefImpl;

class ObjectImpl {
 public:
  virtual ~ObjectImpl() = 0;
  virtual std::unique_ptr<ObjectRefImpl> AdoptRef() const = 0;
  virtual std::unique_ptr<ObjectImpl> ShallowCopy() const = 0;
  virtual bool IsNapi() const { return false; }
  virtual bool IsRemote() const { return false; }
};

class Object {
 public:
  explicit Object(std::unique_ptr<ObjectImpl> impl) : impl_(std::move(impl)) {}
  Object(const Object& other) {
    if (other.impl_) {
      impl_ = other.impl_->ShallowCopy();
    }
  }
  Object& operator=(const Object& other) {
    if (!other.impl_) {
      impl_.reset();
      return *this;
    }
    impl_ = other.impl_->ShallowCopy();
    return *this;
  }
  Object(Object&& other) : impl_(std::move(other.impl_)) {}
  Object& operator=(Object&& other) {
    impl_ = std::move(other.impl_);
    return *this;
  }

  static Object CreateEmpty() { return Object(); }

  ObjectRef AdoptRef();

  bool IsNapi() const { return impl_ && impl_->IsNapi(); }
  bool IsRemote() const { return impl_ && impl_->IsRemote();  }

  // If object is empty, |impl_| would not be created in the first place.
  bool IsEmpty() const { return !impl_; }
  ObjectImpl* GetImpl() const { return impl_.get(); }

 private:
  Object() = default;

  std::unique_ptr<ObjectImpl> impl_;
};

}  // namespace binding
}  // namespace lynx

#endif  // BINDING_COMMON_OBJECT_H_
