// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BINDING_COMMON_ENV_H_
#define BINDING_COMMON_ENV_H_

#include <cstdint>
#include <ostream>

#include "third_party/binding/common/base.h"

namespace lynx {
namespace binding {

class Env;
class Object;
class Value;

typedef void (*EnvDataFinalizer)(Env env, void* data, void* hint);

class EnvImpl {
 public:
  virtual ~EnvImpl() = 0;
  virtual bool IsNapi() const { return false; }
  virtual bool IsRemote() const { return false; }
  virtual void* GetInstanceData(uint64_t key) = 0;
  virtual void SetInstanceData(uint64_t key, void* data, EnvDataFinalizer cb,
                               void* hint) = 0;
  virtual void SendMessage(const Object& target, const std::string& type,
                           const std::string& payload0, const Value& payload1) {
  }
};

class Env {
 public:
  explicit Env(EnvImpl* impl) : impl_(impl) { BINDING_CHECK(impl); }
  bool IsNapi() const { return impl_->IsNapi(); }
  bool IsRemote() const { return impl_->IsRemote(); }
  bool operator==(const Env& other) const { return impl_ == other.impl_; }

  template <typename T>
  T* GetInstanceData(uint64_t key) {
    return static_cast<T*>(impl_->GetInstanceData(key));
  }

  template <typename T>
  void SetInstanceData(uint64_t key, T* data) {
    impl_->SetInstanceData(
        key, data,
        [](Env env, void* data, void*) { delete static_cast<T*>(data); },
        nullptr);
  }

  void SetInstanceData(uint64_t key, void* data, EnvDataFinalizer cb,
                       void* hint) {
    impl_->SetInstanceData(key, data, cb, hint);
  }

  void SendMessage(const Object& target, const std::string& type,
                   const std::string& payload0, const Value& payload1) {
    impl_->SendMessage(target, type, payload0, payload1);
  }

 private:
  friend std::ostream& operator<<(std::ostream& os, const Env& env);
  EnvImpl* const impl_ = nullptr;
};

}  // namespace binding
}  // namespace lynx

#endif  // BINDING_COMMON_ENV_H_
