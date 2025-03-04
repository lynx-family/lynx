// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef JSBRIDGE_BINDINGS_GEN_TEST_NAPI_ASYNC_OBJECT_H_
#define JSBRIDGE_BINDINGS_GEN_TEST_NAPI_ASYNC_OBJECT_H_

#include <memory>

#include "base/include/log/logging.h"
#include "third_party/binding/napi/napi_bridge.h"

namespace lynx {
namespace gen_test {

using binding::NapiBridge;
using binding::ImplBase;

class NapiAsyncObject : public NapiBridge {
 public:
  explicit NapiAsyncObject(const Napi::CallbackInfo&);
  ~NapiAsyncObject() override;

  template <typename T>
  T* ToImplUnsafe() {
    DCHECK(impl_);
    return static_cast<T*>(impl_.get());
  }
  static bool IsInstance(Napi::ScriptWrappable*);

  void Init(std::unique_ptr<ImplBase>);

  uint32_t id() { return id_; }

  // Injection hook
  static void Install(Napi::Env, Napi::Object&, const char*);

 private:
  std::unique_ptr<ImplBase> impl_;
  uint32_t id_;
};

}  // namespace gen_test
}  // namespace lynx

#endif  // JSBRIDGE_BINDINGS_GEN_TEST_NAPI_ASYNC_OBJECT_H_
