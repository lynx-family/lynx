// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_QJS_CALLBACK_H_
#define CORE_RUNTIME_VM_LEPUS_QJS_CALLBACK_H_

#include <string>

#include "core/runtime/vm/lepus/context.h"
#include "core/runtime/vm/lepus/quick_context.h"
namespace lynx {
namespace lepus {

/* return TRUE(1) or FALSE(0) or EXCEPTION(-1)*/
int32_t LepusHasProperty(LEPUSContext* ctx, LEPUSValue obj, LEPUSAtom prop,
                         int32_t idx);

/* return 0 or 1. return 0 if not a valid object or not configurable. */
int32_t LepusDeleteProperty(LEPUSContext* ctx, LEPUSValue this_obj,
                            LEPUSAtom prop, int32_t idx);

/* return < 0 in case if exception, 0 if OK. ptab and its atoms must
   be freed by the user.
*/
int32_t LEPUSValueGetOwnPropertyNames(LEPUSContext* ctx, LEPUSValue this_obj,
                                      uint32_t* prop_count,
                                      LEPUSPropertyEnum** props, int32_t flags);

/* return 0 (FALSE) or 1 (TRUE) */
int32_t LEPUSValueDeepEqualCallBack(LEPUSContext* ctx, LEPUSValue val1,
                                    LEPUSValue val2);

// The fllowing functions are called back in array.prototype.funcs;
// used for array.prototype.push (unshift = 0) and array.prototype.unshift
// (unshift = 1)
LEPUSValue LEPUSRefArrayPushCallBack(LEPUSContext* ctx, LEPUSValue this_val,
                                     int32_t argc, LEPUSValueConst* argv,
                                     int32_t unshift);
// used for array.prototype.pop (shift = 0) and array.prototype.shift
// (unshift = 1)
LEPUSValue LEPUSRefArrayPopCallBack(LEPUSContext* ctx, LEPUSValue this_val,
                                    int32_t shift);

/* return -1 if not found, otherwise return index. */
int64_t LEPUSRefArrayFindCallBack(LEPUSContext* ctx, LEPUSValue this_val,
                                  LEPUSValue value_to_find, int64_t from_index,
                                  int32_t dir);
// reverse array for array.prototype.reverse
LEPUSValue LEPUSRefArrayReverse(LEPUSContext* ctx, LEPUSValue this_val);

base::String LEPUSGetStringFromAtom(LEPUSContext* ctx, LEPUSAtom prop,
                                    int32_t idx);

static inline bool LEPUSAtomIsLengthProp(LEPUSContext* ctx, LEPUSAtom prop) {
  return lepus::QuickContext::GetFromJsContext(ctx)->GetLengthAtom() == prop;
}

/*
  used for Array.prototype.slice and Array.prototype.splice is splice == 1;
*/
LEPUSValue LEPUSRefArraySlice(LEPUSContext* ctx, LEPUSValue this_val,
                              size_t start, size_t count, size_t item_count,
                              LEPUSValue* argv, int32_t splice);
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_QJS_CALLBACK_H_
