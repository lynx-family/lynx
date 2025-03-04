// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JSI_QUICKJS_QUICKJS_HOST_FUNCTION_H_
#define CORE_RUNTIME_JSI_QUICKJS_QUICKJS_HOST_FUNCTION_H_

#include <memory>
#include <unordered_map>

#include "core/runtime/jsi/quickjs/quickjs_helper.h"

namespace lynx {
namespace piper {
class QuickjsRuntime;
namespace detail {
std::weak_ptr<piper::HostFunctionType> getHostFunction(
    QuickjsRuntime* rt, const piper::Function& obj);

class QuickjsHostFunctionProxy
    : public HostObjectWrapperBase<HostFunctionType> {
  friend class QuickjsRuntimeInstance;

 public:
  QuickjsHostFunctionProxy(piper::HostFunctionType hostFunction,
                           QuickjsRuntime* rt);
  ~QuickjsHostFunctionProxy() override;

  static LEPUSValue createFunctionFromHostFunction(
      QuickjsRuntime* rt, LEPUSContext* ctx, const piper::PropNameID& name,
      unsigned int paramCount, piper::HostFunctionType func);

  static LEPUSValue FunctionCallback(LEPUSContext* ctx,
                                     LEPUSValueConst func_obj,
                                     LEPUSValueConst this_obj, int argc,
                                     LEPUSValueConst* argv, int flags);

  static void hostFinalizer(LEPUSRuntime* rt, LEPUSValue val);

 protected:
  GCPersistent p_val_;
};

}  // namespace detail

}  // namespace piper

}  // namespace lynx

// #ifdef __cplusplus
// }
// #endif
#endif  // CORE_RUNTIME_JSI_QUICKJS_QUICKJS_HOST_FUNCTION_H_
