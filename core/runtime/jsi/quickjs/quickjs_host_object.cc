// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/jsi/quickjs/quickjs_host_object.h"

#include <utility>
#include <vector>

#include "base/include/compiler_specific.h"
#include "core/runtime/jsi/quickjs/quickjs_runtime.h"

#ifdef __cplusplus
extern "C" {
#include "quickjs/include/quickjs.h"
#endif
#ifdef __cplusplus
}
#endif
#ifdef OS_IOS
#include "gc/allocator.h"
#include "gc/trace-gc.h"
#else
#include "quickjs/include/allocator.h"
#include "quickjs/include/trace-gc.h"
#endif

namespace lynx {
namespace piper {
namespace detail {
QuickjsHostObjectProxy::QuickjsHostObjectProxy(
    QuickjsRuntime* rt, std::shared_ptr<piper::HostObject> sho)
    : HostObjectWrapperBase(rt, std::move(sho)){};

QuickjsHostObjectProxy::~QuickjsHostObjectProxy() {
  auto quickjs_runtime = static_cast<QuickjsRuntime*>(GetRuntime());
  if (quickjs_runtime) {
    if (LEPUS_IsGCMode(quickjs_runtime->getJSContext())) {
      p_val_.Reset(quickjs_runtime->getJSRuntime());
    }
  }
}

void QuickjsHostObjectProxy::hostFinalizer(LEPUSRuntime* rt, LEPUSValue val) {
  LEPUSClassID object_id = lynx::piper::QuickjsRuntimeInstance::getObjectId(rt);
  if (UNLIKELY(object_id == 0)) {
    LOGE("HostObject Finalizer Error! object_id is 0. LEPUSRuntime:" << rt);
    return;
  }
  QuickjsHostObjectProxy* th =
      static_cast<QuickjsHostObjectProxy*>(LEPUS_GetOpaque(val, object_id));
  if (th) {
    if (LEPUS_IsGCModeRT(rt)) th->p_val_.Reset(rt);
    delete th;
  }
}

LEPUSValue QuickjsHostObjectProxy::getProperty(LEPUSContext* ctx,
                                               LEPUSValueConst obj,
                                               LEPUSAtom atom,
                                               LEPUSValueConst receiver) {
  LEPUSClassID objectId = lynx::piper::QuickjsRuntimeInstance::getObjectId(ctx);
  if (objectId == 0) {
    LOGE(
        "QuickjsHostObjectProxy::getProperty Error! object id is 0. "
        "LEPUSContext:"
        << ctx);
    return LEPUS_UNDEFINED;
  }
  QuickjsHostObjectProxy* proxy =
      static_cast<QuickjsHostObjectProxy*>(LEPUS_GetOpaque(obj, objectId));
  LEPUSValue atom_val = LEPUS_AtomToValue(ctx, atom);
  if (LEPUS_IsException(atom_val)) {
    LOGE("Error getProperty is exception");
    LEPUS_FreeValue(ctx, atom_val);
    return LEPUS_EXCEPTION;
  }
  Runtime* rt = nullptr;
  std::shared_ptr<HostObject> lock_host_object;
  if (UNLIKELY(proxy == nullptr ||
               !proxy->GetRuntimeAndHost(rt, lock_host_object))) {
    LOGE("QuickjsHostObjectProxy::getProperty Error! LEPUSContext:" << ctx);
    return LEPUS_UNDEFINED;
  }
  auto holder =
      QuickjsHelper::createJSValue(ctx, LEPUS_DupValue(ctx, atom_val));

  piper::Value va =
      lock_host_object->get(rt, QuickjsHelper::createPropNameID(ctx, atom_val));
  LEPUSValue ret =
      LEPUS_DupValue(ctx, static_cast<QuickjsRuntime*>(rt)->valueRef(va));

  if (LEPUS_IsException(ret) || LEPUS_IsError(ctx, ret)) {
    LOGE(std::string("Exception in HostObject::getProperty(propName:") +
         QuickjsHelper::LEPUSStringToSTLString(ctx, atom_val));
  }
  return ret;
}

// return 1 is ok, 0 is false.
int QuickjsHostObjectProxy::getOwnProperty(LEPUSContext* ctx,
                                           LEPUSPropertyDescriptor* desc,
                                           LEPUSValueConst obj,
                                           LEPUSAtom prop) {
  LEPUSClassID objectId = lynx::piper::QuickjsRuntimeInstance::getObjectId(ctx);
  if (objectId == 0) {
    LOGE(
        "Error getProperty sObjectClassId is null. objectId is 0. LEPUSContext:"
        << ctx);
    return 0;
  }
  QuickjsHostObjectProxy* proxy =
      static_cast<QuickjsHostObjectProxy*>(LEPUS_GetOpaque(obj, objectId));
  LEPUSValue atom_val = LEPUS_AtomToValue(ctx, prop);
  if (LEPUS_IsException(atom_val)) {
    LOGE("Error getOwnProperty atom_val is exception");
    LEPUSValue exception_val = LEPUS_GetException(ctx);
    if (!LEPUS_IsGCMode(ctx)) {
      LOGE(QuickjsHelper::getErrorMessage(ctx, exception_val));
      LEPUS_FreeValue(ctx, exception_val);
      LEPUS_FreeValue(ctx, atom_val);
    } else {
      HandleScope block_scope(ctx, &exception_val, HANDLE_TYPE_LEPUS_VALUE);
      LOGE(QuickjsHelper::getErrorMessage(ctx, exception_val));
    }
    return 0;
  }
  Runtime* rt = nullptr;
  std::shared_ptr<HostObject> lock_host_object;
  if (UNLIKELY(proxy == nullptr ||
               !proxy->GetRuntimeAndHost(rt, lock_host_object))) {
    LOGE("QuickjsHostObjectProxy::getOwnProperty Error! LEPUSContext:" << ctx);
    return 0;
  }
  QuickjsRuntime* qjs_rt = static_cast<QuickjsRuntime*>(rt);
  HandleScope func_scope(ctx, &atom_val, HANDLE_TYPE_LEPUS_VALUE);
  piper::Value va =
      lock_host_object->get(rt, QuickjsHelper::createPropNameID(ctx, atom_val));
  LEPUSValue ret = LEPUS_DupValue(ctx, qjs_rt->valueRef(va));
  //  LOGE( "LYNX host_object_getPropertyNames jsvalueptr=" <<
  //  LEPUS_VALUE_GET_PTR(ret));
  if (desc) {
    desc->flags = LEPUS_PROP_ENUMERABLE;
    desc->value = ret;
    desc->getter = LEPUS_UNDEFINED;
    desc->setter = LEPUS_UNDEFINED;
  } else if (!LEPUS_IsGCMode(ctx)) {
    LEPUS_FreeValue(ctx, ret);
  }
  return 1;
}

int QuickjsHostObjectProxy::setProperty(LEPUSContext* ctx, LEPUSValueConst obj,
                                        LEPUSAtom atom, LEPUSValueConst value,
                                        LEPUSValueConst receiver, int flags) {
  LEPUSClassID objectId = lynx::piper::QuickjsRuntimeInstance::getObjectId(ctx);
  if (objectId == 0) {
    LOGE("Error setProperty! objectId is 0. LEPUSContext:" << ctx);
    return -1;
  }
  QuickjsHostObjectProxy* proxy =
      static_cast<QuickjsHostObjectProxy*>(LEPUS_GetOpaque(obj, objectId));
  Runtime* rt = nullptr;
  std::shared_ptr<HostObject> lock_host_object;
  if (UNLIKELY(proxy == nullptr ||
               !proxy->GetRuntimeAndHost(rt, lock_host_object))) {
    LOGE("QuickjsHostObjectProxy::setProperty Error! LEPUSContext:" << ctx);
    return -1;
  }
  LEPUSValue atom_val = LEPUS_AtomToValue(ctx, atom);
  HandleScope func_scope(ctx, &atom_val, HANDLE_TYPE_LEPUS_VALUE);
  lock_host_object->set(
      rt, QuickjsHelper::createPropNameID(ctx, atom_val),
      QuickjsHelper::createValue(LEPUS_DupValue(ctx, value),
                                 static_cast<QuickjsRuntime*>(rt)));
  return 1;
}

int QuickjsHostObjectProxy::getPropertyNames(LEPUSContext* ctx,
                                             LEPUSPropertyEnum** ptab,
                                             uint32_t* plen,
                                             LEPUSValueConst obj) {
  LEPUSClassID objectId = lynx::piper::QuickjsRuntimeInstance::getObjectId(ctx);
  if (objectId == 0) {
    LOGE("Error getProperty! objectId is 0. LEPUSContext:" << ctx);
    return -1;
  }
  QuickjsHostObjectProxy* proxy =
      static_cast<QuickjsHostObjectProxy*>(LEPUS_GetOpaque(obj, objectId));
  Runtime* rt = nullptr;
  std::shared_ptr<HostObject> lock_host_object;
  if (UNLIKELY(proxy == nullptr ||
               !proxy->GetRuntimeAndHost(rt, lock_host_object))) {
    LOGE(
        "QuickjsHostObjectProxy::getPropertyNames Error! LEPUSContext:" << ctx);
    return -1;
  }
  std::vector<PropNameID> names = lock_host_object->getPropertyNames(*rt);
  LEPUSPropertyEnum* tab = nullptr;
  uint32_t len = static_cast<uint32_t>(names.size());
  if (len > 0) {
    tab = static_cast<LEPUSPropertyEnum*>(lepus_mallocz(
        ctx, sizeof(LEPUSPropertyEnum) * len, ALLOC_TAG_LEPUSPropertyEnum));
    if (!tab) {
      LOGE("getPropertyNames alloc tab error");
      return -1;
    }
    HandleScope func_scope(ctx, (void*)tab, HANDLE_TYPE_DIR_HEAP_OBJ);
    if (LEPUS_IsGCMode(ctx)) set_heap_obj_len(tab, len);
    for (uint32_t i = 0; i < len; i++) {
      tab[i].atom = LEPUS_NewAtom(ctx, names[i].utf8(*rt).c_str());
    }
  }
  *ptab = tab;
  *plen = len;
  return 0;
}

piper::Object QuickjsHostObjectProxy::createObject(
    QuickjsRuntime* rt, std::shared_ptr<piper::HostObject> ho) {
  LEPUSContext* ctx = rt->getJSContext();
  LEPUSClassID object_id = rt->getObjectClassID();
  if (UNLIKELY(object_id == 0)) {
    LOGE("createHostObject error! object_id is 0. LEPUSContext:" << ctx);
    return QuickjsHelper::createObject(ctx, LEPUS_UNDEFINED);
  }
  QuickjsHostObjectProxy* proxy = new QuickjsHostObjectProxy(rt, std::move(ho));
  LEPUSValue obj = LEPUS_NewObjectClass(ctx, object_id);
  if (LEPUS_IsGCMode(rt->getJSContext()))
    proxy->p_val_.Reset(rt->getJSRuntime(), obj);

  LEPUS_SetOpaque(obj, proxy);
  //  LOGE( "LYNX" << "NewObjectClass ptr=" << LEPUS_VALUE_GET_PTR(obj));

  piper::Object ret = QuickjsHelper::createObject(ctx, obj);
  if (LEPUS_IsGCMode(rt->getJSContext()))
    proxy->p_val_.SetWeak(rt->getJSRuntime());
  return ret;
}

}  // namespace detail

}  // namespace piper

}  // namespace lynx
