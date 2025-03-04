// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BINDING_NAPI_OBJECT_REF_H_
#define BINDING_NAPI_OBJECT_REF_H_

#include "third_party/binding/common/object_ref.h"
#include "third_party/binding/napi/shim/shim_napi.h"

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace binding {

class NapiObjectRefImpl : public ObjectRefImpl {
 public:
  ~NapiObjectRefImpl() override = default;
  Object Get() const override;
  std::unique_ptr<ObjectRefImpl> Dup() override;

 private:
  friend class NapiObjectImpl;
  explicit NapiObjectRefImpl(Napi::Object object)
    : ref_(Napi::Persistent(object)) {}

  Napi::ObjectReference ref_;
};

}  // namespace binding
}  // namespace lynx

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_undefs.h"
#endif

#endif  // BINDING_NAPI_OBJECT_REF_H_
