// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BINDING_NAPI_OBJECT_H_
#define BINDING_NAPI_OBJECT_H_

#include "third_party/binding/common/object.h"
#include "third_party/binding/common/object_ref.h"
#include "third_party/binding/napi/shim/shim_napi.h"

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace binding {

class NapiObjectImpl : public ObjectImpl {
 public:
  std::unique_ptr<ObjectRefImpl> AdoptRef() const override;
  std::unique_ptr<ObjectImpl> ShallowCopy() const override;
  bool IsNapi() const override { return true; }

 private:
  friend Object FromNAPI(Napi::Object object);
  friend Napi::Object ToNAPI(Object object);
  explicit NapiObjectImpl(Napi::Object obj) : obj_(obj) {}

  Napi::Object obj_;
};

Object FromNAPI(Napi::Object object);
Napi::Object ToNAPI(Object object);

}  // namespace binding
}  // namespace lynx

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_undefs.h"
#endif

#endif  // BINDING_NAPI_OBJECT_H_
