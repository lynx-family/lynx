// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/vm/lepus/context.h"

#include <memory>
#include <unordered_set>
#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/utils/base/base_def.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/common/js_error_reporter.h"
#include "core/runtime/common/utils.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/js_object.h"
#include "core/runtime/vm/lepus/jsvalue_helper.h"
#include "core/runtime/vm/lepus/path_parser.h"
#include "core/runtime/vm/lepus/qjs_callback.h"
#include "core/runtime/vm/lepus/quick_context.h"
#include "core/runtime/vm/lepus/ref_counted_class.h"
#include "core/runtime/vm/lepus/table.h"
#include "core/runtime/vm/lepus/tasks/lepus_callback_manager.h"
#include "core/runtime/vm/lepus/tasks/lepus_raf_manager.h"
#include "core/runtime/vm/lepus/vm_context.h"
#ifdef OS_IOS
#include "gc/trace-gc.h"
#else
#include "quickjs/include/trace-gc.h"
#endif

namespace lynx {
namespace lepus {
static LEPUSValue LepusConvertToObjectCallBack(LEPUSContext* ctx,
                                               LEPUSValue val);

// register for quickjs to free LepusRef
static LEPUSValue LepusRefFreeCallBack(LEPUSRuntime* rt, LEPUSValue val) {
  LEPUSLepusRef* pref = static_cast<LEPUSLepusRef*>(LEPUS_VALUE_GET_PTR(val));
  reinterpret_cast<fml::RefCountedThreadSafeStorage*>(pref->p)->Release();
  if (!LEPUS_IsGCModeRT(rt)) {
    LEPUS_FreeValueRT(rt, pref->lepus_val);
    lepus_free_rt(rt, pref);
  }
  return LEPUS_UNDEFINED;
}

static LEPUSValue LepusReportSetConstValueError(LEPUSContext* ctx,
                                                const LEPUSValue& obj,
                                                LEPUSValue prop) {
  lepus::QuickContext* qctx =
      QuickContext::Cast(QuickContext::GetFromJsContext(ctx));
  return qctx->ReportSetConstValueError(obj, prop);
}

// callback for quickjs setProperty for LepusRef
static LEPUSValue LepusRefSetPropertyCallBack(LEPUSContext* ctx,
                                              LEPUSValue thisObj,
                                              LEPUSValue prop, int idx,
                                              LEPUSValue val) {
  DCHECK(LEPUS_IsLepusRef(thisObj));
  LEPUSLepusRef* pref =
      static_cast<LEPUSLepusRef*>(LEPUS_VALUE_GET_PTR(thisObj));
  auto* ref_ptr = static_cast<lepus::RefCounted*>(pref->p);
  if (ref_ptr->GetRefType() != RefType::kLepusTable &&
      ref_ptr->GetRefType() != RefType::kLepusArray) {
    // only table and array can set property
    return LEPUS_UNDEFINED;
  }
  if (ref_ptr->IsConst()) {
    return LepusReportSetConstValueError(ctx, thisObj, prop);
  }

  TRACE_EVENT(LYNX_TRACE_CATEGORY, "QuickContext::LepusRefSetPropertyCallBack");
  Value lepus_val(ctx, val);
  bool gc_flag = LEPUS_IsGCMode(ctx);
  switch (pref->tag) {
    case Value_Table: {
      auto* dic = reinterpret_cast<lepus::Dictionary*>(ref_ptr);
      const char* name = LEPUS_ToCString(ctx, prop);
      HandleScope func_scope(ctx, reinterpret_cast<void*>(&name),
                             HANDLE_TYPE_CSTRING);
      dic->SetValue(name, lepus_val);
      if (!gc_flag) LEPUS_FreeCString(ctx, name);
    } break;
    case Value_Array: {
      CArray* array = reinterpret_cast<CArray*>(ref_ptr);
      uint32_t old_size = static_cast<uint32_t>(array->size());
      if (idx >= 0) {
        array->set(idx, lepus_val);
        for (auto i = old_size; i < static_cast<uint32_t>(idx); ++i) {
          const_cast<lepus::Value&>(array->get(i)).SetUndefined();
        }
      } else {
        LEPUSAtom prop_atom = LEPUS_ValueToAtom(ctx, prop);
        LEPUSAtom len_atom =
            QuickContext::GetFromJsContext(ctx)->GetLengthAtom();
        if (prop_atom == len_atom) {
          uint32_t new_array_len = 0;
          if (LEPUS_ToUint32(ctx, &new_array_len, val) == 0) {
            array->resize(new_array_len);
            for (auto i = old_size; i < new_array_len; ++i) {
              const_cast<lepus::Value&>(array->get(i)).SetUndefined();
            }
          }
        }
        if (!gc_flag) LEPUS_FreeAtom(ctx, prop_atom);
      }
    } break;
  }
  return LEPUS_UNDEFINED;
}

static void LepusRefFreeStringCache(void* old_p, void* p) {
  if (old_p) {
    base::RefCountedStringImpl* impl =
        reinterpret_cast<base::RefCountedStringImpl*>(old_p);
    impl->Release();
  }

  if (p) {
    base::RefCountedStringImpl* impl =
        reinterpret_cast<base::RefCountedStringImpl*>(p);
    impl->AddRef();
  }
}

static LEPUSValue LepusRefGetPropertyCallBack(LEPUSContext* ctx,
                                              LEPUSValue thisObj,
                                              LEPUSAtom prop, int idx) {
  DCHECK(LEPUS_IsLepusRef(thisObj));
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "QuickContext::LepusRefGetPropertyCallBack");
  LEPUSLepusRef* pref =
      static_cast<LEPUSLepusRef*>(LEPUS_VALUE_GET_PTR(thisObj));
  switch (pref->tag) {
    case Value_Table: {
      const char* name = LEPUS_AtomToCString(ctx, prop);
      HandleScope func_scope(ctx, reinterpret_cast<void*>(&name),
                             HANDLE_TYPE_CSTRING);
      auto* dic = LEPUSValueHelper::GetLepusTable(thisObj);
      auto it = dic->find(name);
      if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeCString(ctx, name);
      if (it != dic->end()) {
        return it->second.ToJSValue(ctx);
      }
    } break;
    case Value_Array: {
      auto* carray = LEPUSValueHelper::GetLepusArray(thisObj);
      if (idx >= 0) {
        if (static_cast<size_t>(idx) < carray->size()) {
          return (carray->get(idx)).ToJSValue(ctx);
        }
        return LEPUS_UNDEFINED;
      } else if (LEPUSAtomIsLengthProp(ctx, prop)) {
        return LEPUS_NewInt32(ctx, static_cast<int32_t>(carray->size()));
      }
    } break;
    case Value_JSObject:
    case Value_ByteArray:
    case Value_RefCounted: {
      return LEPUS_UNDEFINED;
    }
    default:
      assert(false);
      break;
  }

  return LEPUS_UNINITIALIZED;
}

static size_t LepusRefGetLengthCallBack(LEPUSContext* ctx, LEPUSValue val) {
  if (!LEPUS_IsLepusRef(val)) return 0;
  LEPUSLepusRef* pref = static_cast<LEPUSLepusRef*>(LEPUS_VALUE_GET_PTR(val));
  if (!LEPUS_IsUndefined(pref->lepus_val))
    return LEPUS_GetLength(ctx, pref->lepus_val);
  switch (pref->tag) {
    case Value_Table:
      return reinterpret_cast<Dictionary*>(pref->p)->size();
    case Value_Array:
      return reinterpret_cast<CArray*>(pref->p)->size();
    case Value_RefCounted:
      return 0;
    default:
      assert(false);
      break;
  }
  return 0;
}

static size_t LepusRefDeepEqualCallBack(LEPUSValue val1, LEPUSValue val2) {
  if (!LEPUS_IsLepusRef(val1) || !LEPUS_IsLepusRef(val2)) return 0;
  if (LEPUS_GetLepusRefTag(val1) != LEPUS_GetLepusRefTag(val2)) return 0;
  int tag = LEPUS_GetLepusRefTag(val1);
  void* pv1 = LEPUS_GetLepusRefPoint(val1);
  void* pv2 = LEPUS_GetLepusRefPoint(val2);
  switch (tag) {
    case Value_Table:
      return *static_cast<Dictionary*>(pv1) == *static_cast<Dictionary*>(pv2);
    case Value_Array:
      return *static_cast<CArray*>(pv1) == *static_cast<CArray*>(pv2);
    case Value_JSObject:
      return *static_cast<LEPUSObject*>(pv1) == *static_cast<LEPUSObject*>(pv2);
    default:
      return 0;
  }
}

static LEPUSValue LepusConvertToObjectCallBack(LEPUSContext* ctx,
                                               LEPUSValue val) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "QuickContext::LepusConvertToObjectCallBack");
  LEPUSLepusRef* pref = static_cast<LEPUSLepusRef*>(LEPUS_VALUE_GET_PTR(val));
  auto* ref_ptr = static_cast<lepus::RefCounted*>(pref->p);
  LEPUSValue result;
  switch (pref->tag) {
    case Value_Table: {
      result = LEPUSValueHelper::TableToJsValue(
          ctx, *static_cast<const Dictionary*>(ref_ptr), false);
    } break;
    case Value_Array: {
      result = LEPUSValueHelper::ArrayToJsValue(
          ctx, *static_cast<const CArray*>(ref_ptr), false);
    } break;
    case Value_RefCounted: {
      if (ref_ptr->js_object_cache) {
        return ref_ptr->js_object_cache->ToJSValue(ctx);
      }
      result = LEPUSValueHelper::RefCountedToJSValue(ctx, *ref_ptr);
      ref_ptr->js_object_cache = lepus::Value{ctx, result};
    } break;
    default:
      return LEPUS_UNDEFINED;
  }
  return result;
}

static LEPUSValue LepusrefToString(LEPUSContext* ctx, LEPUSValue val) {
  if (!LEPUS_IsLepusRef(val)) return LEPUS_UNDEFINED;
  LEPUSLepusRef* pref = static_cast<LEPUSLepusRef*>(LEPUS_VALUE_GET_PTR(val));
  Value lepus_val;
  std::ostringstream s;
  switch (pref->tag) {
    case Value_Table: {
      return LEPUS_NewString(ctx, "[object Object]");
    }

    case Value_Array: {
      lepus_val.SetArray(
          fml::RefPtr<lepus::CArray>(reinterpret_cast<CArray*>(pref->p)));
      s << lepus_val;
      return LEPUS_NewString(ctx, s.str().c_str());
    }

    case Value_JSObject: {
      return LEPUS_NewString(ctx, "[object JSObject]");
    }

    case Value_ByteArray: {
      return LEPUS_NewString(ctx, "[object ByteArray]");
    }

    default: {
      return LEPUS_NewString(ctx, "");
    }
  }

  return LEPUS_UNDEFINED;
}

static void PrintByALog(char* msg) { LOGE(msg); }

LEPUSLepusRefCallbacks Context::GetLepusRefCall() {
  return {&LepusRefFreeCallBack,        &LepusRefGetPropertyCallBack,
          &LepusRefGetLengthCallBack,   &LepusConvertToObjectCallBack,
          &LepusRefSetPropertyCallBack, &LepusRefFreeStringCache,
          &LepusRefDeepEqualCallBack,   &LepusrefToString};
}

static void SetFuncsAndRegisterPrimJSCallbacks(LEPUSRuntime* rt) {
  static void* funcs[] = {
      reinterpret_cast<void*>(PrintByALog),
      reinterpret_cast<void*>(LepusHasProperty),
      reinterpret_cast<void*>(LepusDeleteProperty),
      reinterpret_cast<void*>(LEPUSValueGetOwnPropertyNames),
      reinterpret_cast<void*>(LEPUSValueDeepEqualCallBack),
      reinterpret_cast<void*>(LEPUSRefArrayPushCallBack),
      reinterpret_cast<void*>(LEPUSRefArrayPopCallBack),
      reinterpret_cast<void*>(LEPUSRefArrayFindCallBack),
      reinterpret_cast<void*>(LEPUSRefArrayReverse),
      reinterpret_cast<void*>(LEPUSRefArraySlice)};

  static constexpr int32_t kCountFuncs = sizeof(funcs) / sizeof(funcs[0]);
  auto registered_count = 1;  // only PrintByALog
  if (!tasm::LynxEnv::GetInstance().IsDisabledLepusngOptimize()) {
    registered_count = kCountFuncs;
  }
  RegisterPrimJSCallbacks(rt, reinterpret_cast<void**>(funcs),
                          registered_count);
}

LEPUSRuntimeData::LEPUSRuntimeData(bool disable_tracing_gc) {
  runtime_ = LEPUS_NewRuntimeWithMode(0);
  if (disable_tracing_gc || tasm::LynxEnv::GetInstance().IsDisableTracingGC()) {
    LEPUS_SetRuntimeInfo(runtime_, "Lynx_LepusNG_RC");
  } else {
    LEPUS_SetRuntimeInfo(runtime_, "Lynx_LepusNG");
  }

  SetFuncsAndRegisterPrimJSCallbacks(runtime_);
  lepus_context_ = LEPUS_NewContext(runtime_);
  length_atom_ = LEPUS_NewAtom(lepus_context_, "length");
}

LEPUSRuntimeData::~LEPUSRuntimeData() {
  ContextCell* cell = Context::GetContextCellFromCtx(lepus_context_);
  LEPUS_FreeContext(lepus_context_);
  cell->ctx_ = nullptr;
  cell->qctx_ = nullptr;
  LEPUS_FreeRuntime(runtime_);
  cell->rt_ = nullptr;
}

Context::Context(ContextType type) : type_(type) {}

void Context::EnsureDelegate() {
  if (delegate_ != nullptr) {
    return;
  }
  BASE_STATIC_STRING_DECL(kTemplateAssembler, "$kTemplateAssembler");
  Value delegate_point = GetGlobalData(kTemplateAssembler);
  if (delegate_point.IsCPointer()) {
    delegate_ = reinterpret_cast<Context::Delegate*>(delegate_point.CPoint());
  } else {
    LOGE("Not Found TemplateAssembler Instance");
  }
}

Context::Delegate* Context::GetDelegate() {
  EnsureDelegate();
  return delegate_;
}

std::shared_ptr<Context> Context::CreateContext(bool use_lepusng,
                                                bool disable_tracing_gc) {
  if (use_lepusng) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "Context::CreateQuickContext");
    return std::make_shared<QuickContext>(disable_tracing_gc);
  } else {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "Context::CreateVMContext");
#if !ENABLE_JUST_LEPUSNG
    return std::make_shared<VMContext>();
#else
    LOGE("lepusng sdk do not support vm context");
    assert(false);
    return NULL;
#endif
  }
}

void Context::EnsureLynx() {
  // Initialize and inject Lynx if it has not been set before.
  if (lynx_.IsEmpty()) {
    lynx_ = lepus::Value::CreateObject(this);
    RegisterMethodToLynx();
    SetGlobalData(BASE_STATIC_STRING(tasm::kGlobalLynx), lynx_);
  }
}

void Context::SetPropertyToLynx(const base::String& key,
                                const lepus::Value& value) {
  EnsureLynx();
  lynx_.SetProperty(key, value);
}

const std::shared_ptr<tasm::LepusCallbackManager>& Context::GetCallbackManager()
    const {
  if (!callback_manager_) {
    callback_manager_ = std::make_shared<tasm::LepusCallbackManager>();
  }
  return callback_manager_;
}

const std::shared_ptr<tasm::AnimationFrameManager>&
Context::GetAnimationFrameManager() const {
  if (!animate_frame_manager_) {
    animate_frame_manager_ = std::make_shared<tasm::AnimationFrameManager>();
  }
  return animate_frame_manager_;
}

void Context::ReportError(const std::string& exception_info, int32_t err_code,
                          base::LynxErrorLevel error_level) {
#ifndef LEPUS_PC
  EnsureDelegate();

  if (!delegate_) {
    return;
  }

  base::LynxError error{err_code, exception_info, "", error_level};
  error.custom_info_ = {{"name", name_},
                        {"type", std::to_string(static_cast<int>(type_))}};
  if (name_.compare(LEPUS_DEFAULT_CONTEXT_NAME) != 0) {
    common::FormatErrorUrl(error, name_);
  }
  BeforeReportError(error);
  delegate_->ReportError(std::move(error));
#endif
}

void Context::PrintMsgToJS(const std::string& level, const std::string& msg) {
  EnsureDelegate();

  if (!delegate_) {
    return;
  }

  delegate_->PrintMsgToJS(level, msg);
}

void Context::InitInspector(
    const std::shared_ptr<InspectorLepusObserver>& observer) {
  if (observer != nullptr) {
    // Context may be reused, so do not recreate inspector_manager_ if it is not
    // null.
    if (inspector_manager_ == nullptr) {
      inspector_manager_ = observer->CreateLepusInspectorManager();
    }
    if (inspector_manager_ != nullptr) {
      inspector_manager_->InitInspector(this, observer);
    }
  }
}

void Context::DestroyInspector() {
  if (inspector_manager_ != nullptr) {
    inspector_manager_->DestroyInspector();
  }
}

CellManager::~CellManager() {
  for (auto* itr : cells_) {
    delete itr;
  }
}

ContextCell* CellManager::AddCell(lepus::QuickContext* qctx) {
  LEPUSContext* ctx = qctx->context();
  ContextCell* ret = new ContextCell(qctx, ctx, LEPUS_GetRuntime(ctx));
  cells_.emplace_back(ret);
  return ret;
}

CellManager& Context::GetContextCells() {
  thread_local CellManager cells_;
  return cells_;
}

ContextCell* Context::RegisterContextCell(lepus::QuickContext* qctx) {
  return GetContextCells().AddCell(qctx);
}

bool Context::UpdateTopLevelVariable(const std::string& name,
                                     const Value& val) {
  auto path = ParseValuePath(name);
  return UpdateTopLevelVariableByPath(path, val);
}

Value Context::CallArgs(const base::String& name,
                        const std::vector<Value>& args,
                        bool pause_suppression_mode) {
  const Value* p_args[args.size()];
  for (size_t i = 0; i < args.size(); ++i) {
    p_args[i] = &args[i];
  }
  return CallArgs(name, p_args, args.size(), pause_suppression_mode);
}

Value Context::CallClosureArgs(const Value& closure,
                               const std::vector<Value>& args) {
  const Value* p_args[args.size()];
  for (size_t i = 0; i < args.size(); ++i) {
    p_args[i] = &args[i];
  }
  return CallClosureArgs(closure, p_args, args.size());
}

std::unique_ptr<ContextBundle> ContextBundle::Create(bool is_lepusng_binary) {
  if (is_lepusng_binary) {
    return std::make_unique<QuickContextBundle>();
  }
#if !ENABLE_JUST_LEPUSNG
  return std::make_unique<VMContextBundle>();
#else
  return nullptr;
#endif
}

}  // namespace lepus
}  // namespace lynx
