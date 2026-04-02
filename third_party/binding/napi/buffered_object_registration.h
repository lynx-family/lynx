// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef THIRD_PARTY_BINDING_NAPI_BUFFERED_OBJECT_REGISTRATION_H_
#define THIRD_PARTY_BINDING_NAPI_BUFFERED_OBJECT_REGISTRATION_H_

#include <cstdint>

#include "third_party/binding/common/base.h"
#include "third_party/binding/napi/napi_bridge.h"

namespace lynx {
namespace binding {

template <typename CommandBuffer>
class NapiBufferedObjectRegistration {
 public:
  NapiBufferedObjectRegistration() = default;
  NapiBufferedObjectRegistration(const NapiBufferedObjectRegistration&) =
      delete;
  NapiBufferedObjectRegistration& operator=(
      const NapiBufferedObjectRegistration&) = delete;
  NapiBufferedObjectRegistration(NapiBufferedObjectRegistration&&) = delete;
  NapiBufferedObjectRegistration& operator=(NapiBufferedObjectRegistration&&) =
      delete;
  ~NapiBufferedObjectRegistration() { Reset(); }

  void Register(NapiBridge* wrapped) {
    BINDING_DCHECK(wrapped);
    BINDING_DCHECK(!wrapped_);
    wrapped_ = wrapped;
    id_ = CommandBuffer::RegisterBufferedObject(wrapped);
    wrapped->NapiObject().DefineProperty(Napi::PropertyDescriptor::Value(
        "__id", Napi::Number::New(wrapped->Env(), id_)));
  }

  void Reset() {
    if (!wrapped_) {
      return;
    }
    CommandBuffer::UnregisterBufferedObject(wrapped_, id_);
    wrapped_ = nullptr;
    id_ = 0;
  }

  uint32_t id() const { return id_; }

 private:
  Napi::ScriptWrappable* wrapped_ = nullptr;
  uint32_t id_ = 0;
};

}  // namespace binding
}  // namespace lynx

#endif  // THIRD_PARTY_BINDING_NAPI_BUFFERED_OBJECT_REGISTRATION_H_
