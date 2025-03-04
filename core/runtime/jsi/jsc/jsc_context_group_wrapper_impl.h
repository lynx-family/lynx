// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JSI_JSC_JSC_CONTEXT_GROUP_WRAPPER_IMPL_H_
#define CORE_RUNTIME_JSI_JSC_JSC_CONTEXT_GROUP_WRAPPER_IMPL_H_

#include "core/runtime/jsi/jsc/jsc_context_group_wrapper.h"

namespace lynx {
namespace piper {
class JSCContextGroupWrapperImpl : public JSCContextGroupWrapper {
 public:
  JSCContextGroupWrapperImpl();
  ~JSCContextGroupWrapperImpl() override;
  void InitContextGroup() override;
  inline JSContextGroupRef GetContextGroup() { return group_; }

 private:
  JSContextGroupRef group_;
};
}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_JSI_JSC_JSC_CONTEXT_GROUP_WRAPPER_IMPL_H_
