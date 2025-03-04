// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JSI_QUICKJS_QUICKJS_HOST_OBJECT_H_
#define CORE_RUNTIME_JSI_QUICKJS_QUICKJS_HOST_OBJECT_H_
#include <memory>
#include <string>
#include <unordered_map>

#include "core/base/observer/observer.h"
#include "core/runtime/jsi/quickjs/quickjs_helper.h"

namespace lynx {
namespace piper {
class QuickjsRuntime;
namespace detail {

struct QuickjsHostObjectProxy : public HostObjectWrapperBase<HostObject> {
 public:
  QuickjsHostObjectProxy(QuickjsRuntime* rt,
                         std::shared_ptr<piper::HostObject> sho);
  ~QuickjsHostObjectProxy() override;

  static void hostFinalizer(LEPUSRuntime* rt, LEPUSValue val);
  static LEPUSValue getProperty(LEPUSContext* ctx, LEPUSValueConst obj,
                                LEPUSAtom atom, LEPUSValueConst receiver);

  static int getOwnProperty(LEPUSContext* ctx, LEPUSPropertyDescriptor* desc,
                            LEPUSValueConst obj, LEPUSAtom prop);

  static int setProperty(LEPUSContext* ctx, LEPUSValueConst obj, LEPUSAtom atom,
                         LEPUSValueConst value, LEPUSValueConst receiver,
                         int flags);

  static int getPropertyNames(LEPUSContext* ctx, LEPUSPropertyEnum** ptab,
                              uint32_t* plen, LEPUSValueConst obj);

  static piper::Object createObject(lynx::piper::QuickjsRuntime* ctx,
                                    std::shared_ptr<piper::HostObject> ho);

  friend class QuickJsRuntime;

  GCPersistent p_val_;
};

}  // namespace detail

}  // namespace piper

}  // namespace lynx

#endif  // CORE_RUNTIME_JSI_QUICKJS_QUICKJS_HOST_OBJECT_H_
