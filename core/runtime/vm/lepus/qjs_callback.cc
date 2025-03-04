// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/vm/lepus/qjs_callback.h"

#include <string>

#include "core/runtime/vm/lepus/jsvalue_helper.h"
#include "core/runtime/vm/lepus/quick_context.h"
#ifdef OS_IOS
#include "gc/trace-gc.h"
#else
#include "quickjs/include/trace-gc.h"
#endif

namespace lynx {
namespace lepus {

base::String LEPUSGetStringFromAtom(LEPUSContext* ctx, LEPUSAtom prop,
                                    int32_t idx) {
  if (idx >= 0) {
    return base::String(std::to_string(idx));
  }
  const char* name = LEPUS_AtomToCString(ctx, prop);
  if (name) {
    base::String ret(name);
    if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeCString(ctx, name);
    return ret;
  }
  return base::String();
}

/* return TRUE(1) or FALSE(0) or EXCEPTION(-1)*/
int32_t LepusHasProperty(LEPUSContext* ctx, LEPUSValue obj, LEPUSAtom prop,
                         int32_t idx) {
  assert(LEPUS_IsLepusRef(obj));

  LEPUSLepusRef* pref = static_cast<LEPUSLepusRef*>(LEPUS_VALUE_GET_PTR(obj));
  switch (pref->tag) {
    case Value_Array: {
      auto* carray = LEPUSValueHelper::GetLepusArray(obj);
      if (idx >= 0) {
        return static_cast<size_t>(idx) < carray->size();
      } else {
        return LEPUSAtomIsLengthProp(ctx, prop);
      }
    };
    case Value_Table: {
      auto* dic = LEPUSValueHelper::GetLepusTable(obj);
      return dic->Contains(LEPUSGetStringFromAtom(ctx, prop, idx));
    };
    default: {
      assert(false);
    } break;
  }
  return -1;
}

/* return 0 or 1. return 0 if not a valid object or not configurable. */
int32_t LepusDeleteProperty(LEPUSContext* ctx, LEPUSValue this_obj,
                            LEPUSAtom prop, int32_t idx) {
  assert(LEPUS_IsLepusRef(this_obj));
  static lepus::Value undefined =
      lepus::Value(ctx, LEPUS_UNDEFINED).ToLepusValue();
  LEPUSLepusRef* pref =
      static_cast<LEPUSLepusRef*>(LEPUS_VALUE_GET_PTR(this_obj));
  int32_t ret = false;
  switch (pref->tag) {
    case Value_Table: {
      auto* dic = LEPUSValueHelper::GetLepusTable(this_obj);
      return dic->EraseKey(LEPUSGetStringFromAtom(ctx, prop, idx)) == -1 ? 0
                                                                         : 1;
    } break;
    case Value_Array: {
      auto* carray = LEPUSValueHelper::GetLepusArray(this_obj);
      if (idx >= 0) {
        if (static_cast<size_t>(idx) >= carray->size()) return true;
        ret = carray->set(idx, undefined);
      } else {
        return !LEPUSAtomIsLengthProp(ctx, prop);
      }
    } break;
    default:
      assert(false);
      return false;
      break;
  }
  return ret;
}

/* return < 0 in case if exception, 0 if OK. ptab and its atoms must
   be freed by the user. */
int32_t LEPUSValueGetOwnPropertyNames(LEPUSContext* ctx, LEPUSValue this_obj,
                                      uint32_t* prop_count,
                                      LEPUSPropertyEnum** props,
                                      int32_t flags) {
  assert(LEPUS_IsLepusRef(this_obj));
  LEPUSPropertyEnum* tab_enum = nullptr;
  HandleScope func_scope(ctx, &tab_enum, HANDLE_TYPE_HEAP_OBJ);
  *props = nullptr;
  *prop_count = 0;
  bool is_enum_only = flags & LEPUS_GPN_ENUM_ONLY;
  if (LEPUSValueHelper::IsLepusArray(this_obj)) {
    auto* array = LEPUSValueHelper::GetLepusArray(this_obj);
    auto array_size = array->size();
    if (!is_enum_only) ++array_size;
    *prop_count = static_cast<uint32_t>(array_size);
    /* avoid allocating 0 bytes */
    auto malloc_size = array_size > 0 ? array_size : 1;
    tab_enum = reinterpret_cast<LEPUSPropertyEnum*>(
        lepus_mallocz(ctx, sizeof(LEPUSPropertyEnum) * malloc_size,
                      ALLOC_TAG_LEPUSPropertyEnum));
    /* malloc failed return -1; */
    if (!tab_enum) return -1;
    for (uint32_t i = 0; i < array->size(); ++i) {
      tab_enum[i].atom = LEPUS_NewAtomUInt32(ctx, i);
      tab_enum[i].is_enumerable = 1;
    }
    if (!is_enum_only) {
      tab_enum[array_size - 1].atom = LEPUS_DupAtom(
          ctx, lepus::QuickContext::GetFromJsContext(ctx)->GetLengthAtom());
      tab_enum[array_size - 1].is_enumerable = 0;
    }
    *props = tab_enum;
    return 0;
  } else if (LEPUSValueHelper::IsLepusTable(this_obj)) {
    auto& table = *LEPUSValueHelper::GetLepusTable(this_obj);

    auto table_size = table.size();
    (*prop_count) = static_cast<uint32_t>(table_size);
    auto malloc_size = table_size > 0 ? table_size : 1;
    tab_enum = reinterpret_cast<LEPUSPropertyEnum*>(
        lepus_mallocz(ctx, sizeof(LEPUSPropertyEnum) * malloc_size,
                      ALLOC_TAG_LEPUSPropertyEnum));
    // malloc failed return -1;
    if (!tab_enum) return -1;
    uint32_t i = 0;
    for (auto& pair : table) {
      tab_enum[i].atom = LEPUS_NewAtom(ctx, pair.first.c_str());
      tab_enum[i].is_enumerable = 1;
      ++i;
    }
    *props = tab_enum;
    return 0;
  }
  return -1;
}

int32_t LEPUSValueDeepEqualCallBack(LEPUSContext* ctx, LEPUSValue val1,
                                    LEPUSValue val2) {
  return lepus::Value(ctx, val1) == lepus::Value(ctx, val2);
}

LEPUSValue LEPUSRefArrayPushCallBack(LEPUSContext* ctx, LEPUSValue this_val,
                                     int32_t argc, LEPUSValueConst* argv,
                                     int32_t unshift) {
  assert(LEPUS_IsLepusRef(this_val) &&
         LEPUS_GetLepusRefTag(this_val) == Value_Array);

  auto* array = LEPUSValueHelper::GetLepusArray(this_val);

  if (array->IsConst()) {
    QuickContext::GetFromJsContext(ctx)->ReportError(
        "The array is const of Array.prototype.push in LepusNG\n");
    return LEPUS_NewInt64(ctx, array->size());
  }

  auto old_size = array->size(), new_len = array->size() + argc;
  if (unlikely(argc <= 0)) return LEPUS_NewInt64(ctx, old_size);

  if constexpr (sizeof(new_len) > sizeof(int32_t)) {
    if (new_len > LEPUSValueHelper::MAX_SAFE_INTEGER) {
      LEPUS_ThrowTypeError(ctx, "Array.push: array is too long");
      return LEPUS_EXCEPTION;
    }
  }

  array->resize(new_len);
  auto from = old_size;
  if (unshift) {
    for (size_t i = 0; i < old_size; ++i) {
      const lepus::Value& v = array->get(old_size - 1 - i);
      array->set(old_size - 1 - i + argc, v);
    }
    from = 0;
  }

  for (int32_t i = 0; i < argc; ++i) {
    array->set(from + i, lepus::Value(ctx, argv[i]));
  }
  return LEPUS_NewInt64(ctx, new_len);
}

LEPUSValue LEPUSRefArrayPopCallBack(LEPUSContext* ctx, LEPUSValue this_val,
                                    int32_t shift) {
  assert(LEPUS_IsLepusRef(this_val) &&
         LEPUS_GetLepusRefTag(this_val) == Value_Array);

  auto* array = LEPUSValueHelper::GetLepusArray(this_val);

  if (array->IsConst()) {
    QuickContext::GetFromJsContext(ctx)->ReportError(
        "The array is const of Array.prototype.pop in LepusNG\n");
    return LEPUS_UNDEFINED;
  }

  auto old_size = array->size();
  LEPUSValue res = LEPUS_UNDEFINED;
  if (old_size > 0) {
    if (shift) {
      res = array->get(0).ToJSValue(ctx);
      array->Erase(0);
    } else {
      res = array->get(old_size - 1).ToJSValue(ctx);
      array->pop_back();
    }
  }
  return res;
}

/* return -1 if not found, otherwise return index. */
int64_t LEPUSRefArrayFindCallBack(LEPUSContext* ctx, LEPUSValue this_val,
                                  LEPUSValue value_to_find, int64_t from_index,
                                  int32_t dir) {
  assert(LEPUS_IsLepusRef(this_val) &&
         LEPUS_GetLepusRefTag(this_val) == Value_Array);
  auto* array = LEPUSValueHelper::GetLepusArray(this_val);
  int64_t array_size = array->size();
  LEPUSValue op = LEPUS_UNDEFINED;
  HandleScope block_scope(ctx, &op, HANDLE_TYPE_LEPUS_VALUE);

  if (dir > 0) {
    assert(from_index >= 0);
    for (; from_index < array_size; ++from_index) {
      op = array->get(static_cast<size_t>(from_index)).ToJSValue(ctx, false);
      if (LEPUS_SameValue(ctx, op, value_to_find)) {
        if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeValue(ctx, op);
        return from_index;
      };
      if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeValue(ctx, op);
    }
  } else {
    assert(from_index < array_size);
    for (; from_index >= 0; --from_index) {
      op = array->get(static_cast<size_t>(from_index)).ToJSValue(ctx, false);
      if (LEPUS_SameValue(ctx, op, value_to_find)) {
        if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeValue(ctx, op);
        return from_index;
      };
      if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeValue(ctx, op);
    }
  }
  return -1;
}

LEPUSValue LEPUSRefArrayReverse(LEPUSContext* ctx, LEPUSValue this_val) {
  assert(LEPUS_IsLepusRef(this_val) &&
         LEPUS_GetLepusRefTag(this_val) == Value_Array);
  auto* array = LEPUSValueHelper::GetLepusArray(this_val);
  if (array->IsConst()) {
    QuickContext::GetFromJsContext(ctx)->ReportError(
        "The array is const of Array.prototype.reverse in LepusNG\n");
    return this_val;
  }

  auto array_size = array->size();
  if (array_size > 1) {
    for (size_t l = 0, h = array_size - 1; l < h; ++l, --h) {
      const lepus::Value temp_val = array->get(l);
      array->set(l, array->get(h));
      array->set(h, temp_val);
    }
  }
  return this_val;
}

LEPUSValue LEPUSRefArraySlice(LEPUSContext* ctx, LEPUSValue this_val,
                              size_t start, size_t count, size_t item_count,
                              LEPUSValue* argv, int32_t splice) {
  assert(LEPUS_IsLepusRef(this_val) &&
         LEPUS_GetLepusRefTag(this_val) == Value_Array);
  auto* array = LEPUSValueHelper::GetLepusArray(this_val);
  LEPUSValue ret = LEPUS_NewArray(ctx);
  HandleScope func_scope(ctx, &ret, HANDLE_TYPE_LEPUS_VALUE);
  if (splice && array->IsConst()) {
    QuickContext::GetFromJsContext(ctx)->ReportError(
        "The array is const of Array.prototype.splice in LepusNG\n");
    return ret;
  }

  LEPUSValue v = LEPUS_UNDEFINED;
  func_scope.PushHandle(&v, HANDLE_TYPE_LEPUS_VALUE);
  for (size_t i = 0, k = start; i < count && k < array->size(); ++i, ++k) {
    v = array->get(k).ToJSValue(ctx);
    LEPUS_SetPropertyInt64(ctx, ret, i, v);
  }

  if (splice) {
    // first erase consecuitive n elements of array from start
    array->Erase(start, count);
    // then insert item_count elements
    array->Insert(start, item_count, ctx, argv);
  }
  return ret;
}

}  // namespace lepus
}  // namespace lynx
