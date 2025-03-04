// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "third_party/binding/common/base.h"

#include "third_party/binding/common/env.h"
#include "third_party/binding/common/object_ref.h"
#include "third_party/binding/common/object.h"
#include "third_party/binding/napi/shim/shim_napi.h"

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace binding {

Napi::Object BridgeBase::NapiObject() { BINDING_NOTREACHED(); }

Napi::Env BridgeBase::NapiEnv() { BINDING_NOTREACHED(); }

ImplBase::ImplBase() = default;

ImplBase::~ImplBase() {
  if (IsNapiWrapped()) {
    OnExternalMemoryChanged(0);
  }
}

Object ImplBase::GetBaseObject() { return bridge_->GetBaseObject(); }

Napi::Object ImplBase::NapiObject() { return bridge_->NapiObject(); }

ObjectRef ImplBase::ObtainStrongRef() { return bridge_->GetStrongRef(); }

Env ImplBase::GetEnv() { return bridge_->GetEnv(); }

Napi::Env ImplBase::NapiEnv() { return bridge_->NapiEnv(); }

void ImplBase::AssociateWithWrapper(BridgeBase* bridge) {
  if (bridge_ == bridge) {
    return;
  }
  if (bridge_) {
    OnOrphaned();
    OnExternalMemoryChanged(0);
  }
  bridge_ = bridge;
  if (bridge_) {
    // Assuming that no extra memory is allocated before wrapping.
    OnExternalMemoryChanged(kInitialMemoryInBytes);
    OnWrapped();
  }
}

void ImplBase::OnExternalMemoryChanged(int64_t current_memory_in_bytes) {
  if (current_memory_in_bytes == resident_memory_in_bytes_) {
    return;
  }
  if (IsNapiWrapped() && static_cast<napi_env>(NapiEnv())->rt) {
    Napi::MemoryManagement::AdjustExternalMemory(
        NapiEnv(), current_memory_in_bytes - last_reported_memory_in_bytes_);
    last_reported_memory_in_bytes_ = current_memory_in_bytes;
  }
  resident_memory_in_bytes_ = current_memory_in_bytes;
}

}  // namespace binding
}  // namespace lynx
