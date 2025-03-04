/*
 * QuickJS Javascript Engine
 *
 * Copyright (c) 2017-2019 Fabrice Bellard
 * Copyright (c) 2017-2019 Charlie Gordon
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// Copyright 2024 The Primjs Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SRC_INTERPRETER_QUICKJS_INCLUDE_QUICKJS_H_
#define SRC_INTERPRETER_QUICKJS_INCLUDE_QUICKJS_H_

#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "base_export.h"
#include "list.h"
// #define DUMP_LEAKS 0
// for Debug
// <Primjs add>
// if want to dump atoms/shapes/strings, should open blew marco
// #define DUMP_QJS_VALUE

#ifdef DEBUG_MEMORY
#ifdef __ANDROID__
#include <android/log.h>
// #define DUMP_LEAKS 0
// log
#define printf(...) __android_log_print(ANDROID_LOG_ERROR, "LYNX", __VA_ARGS__);
#endif
#else
#define printf(...)
#endif

#if defined(__GNUC__) || defined(__clang__)
#define lepus_likely(x) __builtin_expect(!!(x), 1)
#define lepus_unlikely(x) __builtin_expect(!!(x), 0)
#define lepus_force_inline inline __attribute__((always_inline))
#define __js_printf_like(f, a) __attribute__((format(printf, f, a)))
#else
#define lepus_likely(x) (x)
#define lepus_unlikely(x) (x)
#define lepus_force_inline inline
#define __js_printf_like(a, b)
#endif

#define LEPUS_BOOL int

typedef struct LEPUSRuntime LEPUSRuntime;
typedef struct LEPUSContext LEPUSContext;
typedef struct LEPUSObject LEPUSObject;
typedef struct LEPUSClass LEPUSClass;
typedef uint32_t LEPUSClassID;
typedef uint32_t JSAtom;
typedef uint32_t LEPUSAtom;

struct LEPUSFunctionBytecode;
struct LEPUSStackFrame;
typedef struct LEPUSDebuggerInfo LEPUSDebuggerInfo;
struct qjs_queue;
struct DebuggerSuspendedState;
struct LEPUSClosureVar;
typedef struct LEPUSBreakpoint LEPUSBreakpoint;
typedef struct LEPUSScriptSource LEPUSScriptSource;

#if defined(__x86_64__) || defined(__aarch64__)
#define LEPUS_PTR64
#define LEPUS_PTR64_DEF(a) a
#else
#define LEPUS_PTR64_DEF(a)
#endif

#if !defined(LEPUS_PTR64)
#define LEPUS_NAN_BOXING
#endif

enum {
  ALLOC_TAG_WITHOUT_PTR = 1,
#define DEFTAG(name, str) ALLOC_TAG_##name,
#include "quickjs-tag.h"
#undef DEFTAG
  ALLOC_TAG_END,
};

// <Primjs begin>
typedef enum LEPUSTypedArrayType {
  LEPUS_TYPED_UNKNOW,
  LEPUS_TYPED_UINT8C_ARRAY,  /* u.array (typed_array) */
  LEPUS_TYPED_INT8_ARRAY,    /* u.array (typed_array) */
  LEPUS_TYPED_UINT8_ARRAY,   /* u.array (typed_array) */
  LEPUS_TYPED_INT16_ARRAY,   /* u.array (typed_array) */
  LEPUS_TYPED_UINT16_ARRAY,  /* u.array (typed_array) */
  LEPUS_TYPED_INT32_ARRAY,   /* u.array (typed_array) */
  LEPUS_TYPED_UINT32_ARRAY,  /* u.array (typed_array) */
  LEPUS_TYPED_FLOAT32_ARRAY, /* u.array (typed_array) */
  LEPUS_TYPED_FLOAT64_ARRAY  /* u.array (typed_array) */
} LEPUSTypedArrayType;
// <Primjs end>

typedef struct LEPUSRefCountHeader {
  int ref_count;
} LEPUSRefCountHeader;

#if defined(__aarch64__) && !defined(OS_WIN) && !defined(CONFIG_BIGNUM) && \
    !DISABLE_NANBOX

static const int64_t LEPUS_FLOAT64_NAN_BITS = 0x7ff8000000000000;

static inline double pure_nan() { return (*(double *)&LEPUS_FLOAT64_NAN_BITS); }
#define LEPUS_FLOAT64_NAN (pure_nan())

static const int DOUBLE_ENCODE_OFFSET_BIT = 49;
static const int64_t DOUBLE_ENCODE_OFFSET = (1ll << DOUBLE_ENCODE_OFFSET_BIT);
static const int64_t NUMBER_TAG = 0xfffe000000000000;
static const int64_t LEPUS_TAG_Atom = 0xfffc000000000000;
static const int64_t NOT_NUMBER_MASK = 0xffff000000000000;
static const int64_t OTHER_TAG = 0x2;

static const int64_t BOOL_TAG = 0x4;
static const int64_t BOOL_TRUE_TAG = (OTHER_TAG | BOOL_TAG | 0x1);
static const int64_t BOOL_FALSE_TAG = (OTHER_TAG | BOOL_TAG);
static const int64_t UNDEFINED_TAG = (OTHER_TAG | 0x10);
static const int64_t UNINITIALIZED_TAG = (OTHER_TAG | 0x20);
static const int64_t CATCH_OFFSET_TAG = (OTHER_TAG | 0x40);
static const int64_t EXCEPTION_TAG = (OTHER_TAG | 0x80);

#define VALUE_FALSE ((LEPUSValue){.as_int64 = BOOL_FALSE_TAG})
#define VALUE_TRUE ((LEPUSValue){.as_int64 = BOOL_TRUE_TAG})
#define VALUE_UNDEFINED ((LEPUSValue){.as_int64 = UNDEFINED_TAG})
#define VALUE_NULL ((LEPUSValue){.as_int64 = OTHER_TAG})
#define VALUE_EXCEPTION ((LEPUSValue){.as_int64 = EXCEPTION_TAG})
#define VALUE_UNINITIALIZED ((LEPUSValue){.as_int64 = UNINITIALIZED_TAG})

static const int64_t OTHER_PTR_TAG = 0x0001000000000000ll;
static const int64_t SYMBOL_TAG = (0x1 | OTHER_PTR_TAG);
static const int64_t STRING_TAG = (0x2 | OTHER_PTR_TAG);
static const int64_t MODULE_TAG = (0x3 | OTHER_PTR_TAG);
static const int64_t FUNCTION_BYTECODE_TAG = (0x0 | OTHER_PTR_TAG);
static const int64_t OTHER_PTR_MASK = 0x0000fffffffffffc;
static const int64_t NOT_OTHER_PTR_MASK = 0xffff000000000003;
static const int64_t NOT_CELL_MASK = (OTHER_TAG | NUMBER_TAG);
static const int64_t NOT_CELL_OTHER_PTR_MASK =
    (OTHER_TAG | NUMBER_TAG | OTHER_PTR_TAG);
static const int64_t LEPUS_PTR_TAG = 0xffff000000000000ll;
static const int64_t LEPUS_REF_TAG = (0x0 | LEPUS_PTR_TAG);
static const int64_t LEPUS_CPOINTER_TAG = (0x1 | LEPUS_PTR_TAG);
static const int64_t LEPUS_BIG_INT_TAG = (0x2 | LEPUS_PTR_TAG);
static const int64_t SEPARABLE_STRING_TAG = (0x3 | LEPUS_PTR_TAG);
static const int64_t LEPUS_PTR_MASK = 0x0000fffffffffffc;
static const int64_t NOT_LEPUS_PTR_MASK = 0xffff000000000003;
static const int64_t INTERNAL_GC_TAG = 0xfffd000000000000ll;
// #define NOT_OTHER_MASK (NUMBER_TAG | OTHER_PTR_TAG)

// <Primjs begin>
static const int64_t LEPUS_TAG_LEPUS_REF =
    LEPUS_REF_TAG; /* Primjs add for lepus */
static const int64_t LEPUS_TAG_LEPUS_CPOINTER = LEPUS_CPOINTER_TAG;
static const int64_t LEPUS_TAG_BIG_INT = LEPUS_BIG_INT_TAG;
static const int64_t LEPUS_TAG_BIG_FLOAT = -9;
static const int64_t LEPUS_TAG_SYMBOL = SYMBOL_TAG;
static const int64_t LEPUS_TAG_STRING = STRING_TAG;
static const int64_t LEPUS_TAG_MODULE = MODULE_TAG;
static const int64_t LEPUS_TAG_FUNCTION_BYTECODE = FUNCTION_BYTECODE_TAG;
static const int64_t LEPUS_TAG_OBJECT = 0;
static const int64_t LEPUS_TAG_INT = NUMBER_TAG;
static const int64_t LEPUS_TAG_BOOL = BOOL_TAG | OTHER_TAG;
static const int64_t LEPUS_TAG_NULL = OTHER_TAG;
static const int64_t LEPUS_TAG_UNDEFINED = UNDEFINED_TAG | OTHER_TAG;
static const int64_t LEPUS_TAG_UNINITIALIZED = UNINITIALIZED_TAG;
static const int64_t LEPUS_TAG_CATCH_OFFSET = CATCH_OFFSET_TAG;
static const int64_t LEPUS_TAG_EXCEPTION = EXCEPTION_TAG;
static const int64_t LEPUS_TAG_SHAPE =
    (0x0 | INTERNAL_GC_TAG); /* used internally during GC */
static const int64_t LEPUS_TAG_ASYNC_FUNCTION =
    (0x1 | INTERNAL_GC_TAG); /* used internally during GC */
static const int64_t LEPUS_TAG_VAR_REF =
    (0x2 | INTERNAL_GC_TAG); /* used internally during GC */
static const int64_t LEPUS_TAG_FLOAT64 = 1;
static const int64_t LEPUS_TAG_SEPARABLE_STRING = SEPARABLE_STRING_TAG;

typedef union LEPUSValue {
  int64_t as_int64;
  void *ptr;
  double as_double;
} LEPUSValue;

#define LEPUSValueConst LEPUSValue

inline int64_t LEPUS_VALUE_GET_TAG(LEPUSValue v) {
  if ((v.as_int64 & NUMBER_TAG) == 0 && (v.as_int64 & OTHER_PTR_TAG)) {
    return v.as_int64 & NOT_OTHER_PTR_MASK;
  } else if ((v.as_int64 & NOT_CELL_MASK) == 0) {
    return LEPUS_TAG_OBJECT;
  } else if ((v.as_int64 & LEPUS_PTR_TAG) == LEPUS_PTR_TAG) {
    return v.as_int64 & (~LEPUS_PTR_MASK);
  } else if ((v.as_int64 & NUMBER_TAG) == 0) {
    // true false undefined null
    return v.as_int64 & 0xfe;
  } else if ((v.as_int64 & NOT_NUMBER_MASK) == NUMBER_TAG) {
    return LEPUS_TAG_INT;
  } else if ((v.as_int64 & NOT_NUMBER_MASK) == LEPUS_TAG_Atom) {
    return LEPUS_TAG_Atom;
  } else if ((v.as_int64 & INTERNAL_GC_TAG) == INTERNAL_GC_TAG) {
    return v.as_int64 & (~LEPUS_PTR_MASK);  // shape, async_function, var_ref
  } else {
    return LEPUS_TAG_FLOAT64;
  }
}

#define LEPUS_VALUE_IS_STRING(v) \
  (((v).as_int64 & NOT_OTHER_PTR_MASK) == LEPUS_TAG_STRING)
#define LEPUS_VALUE_IS_SEPARABLE_STRING(v) \
  (((v).as_int64 & NOT_LEPUS_PTR_MASK) == LEPUS_TAG_SEPARABLE_STRING)
#define LEPUS_VALUE_IS_ATOM(v) \
  (((v).as_int64 & NOT_NUMBER_MASK) == LEPUS_TAG_Atom)
#define LEPUS_VALUE_IS_OBJECT(v) \
  (!((v).as_int64 & NOT_CELL_OTHER_PTR_MASK) && (v).as_int64)
#define LEPUS_VALUE_IS_NOT_OBJECT(v) \
  ((v).as_int64 & NOT_CELL_OTHER_PTR_MASK || (v).as_int64 == 0)
#define LEPUS_VALUE_IS_NULL(v) ((v).as_int64 == VALUE_NULL.as_int64)
#define LEPUS_VALUE_IS_UNDEFINED(v) ((v).as_int64 == VALUE_UNDEFINED.as_int64)
#define LEPUS_VALUE_IS_SYMBOL(v) \
  (((v).as_int64 & NOT_OTHER_PTR_MASK) == LEPUS_TAG_SYMBOL)
#define LEPUS_VALUE_IS_INT(v) (((v).as_int64 & NOT_NUMBER_MASK) == NUMBER_TAG)
// Note: This is faster than (((v).as_int64 & 0xffff0000000000f7) ==
// LEPUS_TAG_CATCH_OFFSET)
#define LEPUS_VALUE_IS_CATCH_OFFSET(v)     \
  (!((v).as_int64 & 0xffff000000000000) && \
   ((v).as_int64 & 0xf7) == LEPUS_TAG_CATCH_OFFSET)
// || ((v).as_int64 & 0xf7)!=LEPUS_TAG_CATCH_OFFSET)
#define LEPUS_VALUE_IS_FLOAT64(v) \
  (((v).as_int64 & NUMBER_TAG) && ((v).as_int64 & NUMBER_TAG) != NUMBER_TAG)
#define LEPUS_VALUE_IS_EXCEPTION(v) ((v).as_int64 == VALUE_EXCEPTION.as_int64)
#define LEPUS_VALUE_IS_BOOL(v) \
  ((v).as_int64 == VALUE_TRUE.as_int64 || (v).as_int64 == VALUE_FALSE.as_int64)
#define LEPUS_VALUE_IS_UNINITIALIZED(v) \
  ((v).as_int64 == VALUE_UNINITIALIZED.as_int64)
#define LEPUS_VALUE_IS_FUNCTION_BYTECODE(v) \
  (((v).as_int64 & NOT_OTHER_PTR_MASK) == LEPUS_TAG_FUNCTION_BYTECODE)
#define LEPUS_VALUE_IS_MODULE(v) \
  (((v).as_int64 & NOT_OTHER_PTR_MASK) == LEPUS_TAG_MODULE)

#define LEPUS_VALUE_IS_LEPUS_REF(v) \
  (((v).as_int64 & NOT_LEPUS_PTR_MASK) == LEPUS_TAG_LEPUS_REF)

#define LEPUS_VALUE_IS_LEPUS_CPOINTER(v) \
  (((v).as_int64 & NOT_LEPUS_PTR_MASK) == LEPUS_TAG_LEPUS_CPOINTER)

#define LEPUS_VALUE_IS_BIG_INT(v) \
  (((v).as_int64 & NOT_LEPUS_PTR_MASK) == LEPUS_TAG_BIG_INT)

#define LEPUS_VALUE_GET_NORM_TAG(v) (LEPUS_VALUE_GET_TAG(v))

#define LEPUS_VALUE_GET_INT(v) ((int)((v).as_int64))

#define LEPUS_VALUE_GET_CATCH_OFFSET(v) ((int)(v.as_int64 >> 16))

#define LEPUS_VALUE_GET_BOOL(v) (v.as_int64 == VALUE_TRUE.as_int64)

// <Primjs begin>
#define LEPUS_VALUE_GET_INT64(v) (v.as_int64)

// #define LEPUS_MKVAL(tag, val) \
//   ((LEPUSValue) { (((uint64_t)(tag) << 32) | (uintptr_t)(val)) }
static inline LEPUSValue LEPUS_MKVAL(int64_t tag, int32_t val) {
  if (tag != LEPUS_TAG_CATCH_OFFSET) {
    return (LEPUSValue){.as_int64 =
                            (int64_t)((tag) | (uint64_t)(uint32_t)(val))};
  } else {
    return (LEPUSValue){.as_int64 =
                            (int64_t)((tag) | (uint64_t)(uint32_t)(val) << 16)};
  }
}

#define LEPUS_MKPTR(tag, p) \
  ((LEPUSValue){.as_int64 = (int64_t)(((int64_t)(p) & LEPUS_PTR_MASK) | (tag))})
// <Primjs end>

static inline double LEPUS_VALUE_GET_FLOAT64(LEPUSValue v) {
  int64_t val = (v.as_int64 - DOUBLE_ENCODE_OFFSET);
  return *(double *)&val;
}

#if defined(ANDROID) || defined(__ANDROID__)
extern int64_t HEAP_TAG_OUTER;
extern int64_t HEAP_TAG_INNER;

#define LEPUS_VALUE_GET_CPOINTER(v) \
  ((void *)(((int64_t)v.ptr & OTHER_PTR_MASK) | HEAP_TAG_OUTER))

#define LEPUS_VALUE_GET_PTR(v) \
  ((void *)(((int64_t)(v).ptr & OTHER_PTR_MASK) | HEAP_TAG_INNER))
#else
#define LEPUS_VALUE_GET_PTR(v) ((void *)(((int64_t)(v).ptr & OTHER_PTR_MASK)))
#define LEPUS_VALUE_GET_CPOINTER(v) LEPUS_VALUE_GET_PTR(v)
#endif

static inline LEPUSValue __JS_NewFloat64(LEPUSContext *ctx, double d) {
  LEPUSValue v;
  if (isnan(d)) {
    d = pure_nan();
  }
  v.as_double = d;
  v.as_int64 = v.as_int64 + DOUBLE_ENCODE_OFFSET;
  return v;
}

#define LEPUS_NAN \
  ((LEPUSValue){.as_int64 = LEPUS_FLOAT64_NAN_BITS + DOUBLE_ENCODE_OFFSET})

// #undef isnan
// #define isnan(d) ((*(int64_t*)&d) == LEPUS_FLOAT64_NAN_BITS)

inline LEPUS_BOOL LEPUS_VALUE_IS_NAN(LEPUSValue v) {
  return v.as_int64 == LEPUS_NAN.as_int64;
}

#define LEPUS_TAG_IS_FLOAT64(tag) ((tag) == LEPUS_TAG_FLOAT64)

#define LEPUS_VALUE_HAS_REF_COUNT(v)                                   \
  (((v.as_int64 & NUMBER_TAG) == 0 && (v.as_int64 & OTHER_PTR_TAG)) || \
   ((v.as_int64 & NOT_CELL_MASK) == 0 && v.as_int64 != 0) ||           \
   (((v.as_int64 & LEPUS_PTR_TAG) == LEPUS_PTR_TAG) &&                 \
    ((v.as_int64 & 0x03) != 1)) ||                                     \
   ((v.as_int64 & LEPUS_PTR_TAG) == INTERNAL_GC_TAG))

#define LEPUS_VALUE_IS_BOTH_INT(v1, v2) \
  (LEPUS_VALUE_IS_INT((v1)) && LEPUS_VALUE_IS_INT(v2))
#define LEPUS_VALUE_IS_BOTH_FLOAT(v1, v2) \
  (LEPUS_VALUE_IS_FLOAT64(v1) && LEPUS_VALUE_IS_FLOAT64(v2))

#define LEPUS_VALUE_GET_OBJ(v) ((LEPUSObject *)LEPUS_VALUE_GET_PTR(v))
#define LEPUS_VALUE_GET_STRING(v) ((JSString *)LEPUS_VALUE_GET_PTR(v))
// #define LEPUS_VALUE_HAS_REF_COUNT(v) ((unsigned)LEPUS_VALUE_GET_TAG(v) >=
// (unsigned)LEPUS_TAG_FIRST)

/* special values */
#define LEPUS_NULL VALUE_NULL
#define LEPUS_UNDEFINED VALUE_UNDEFINED
#define LEPUS_FALSE VALUE_FALSE
#define LEPUS_TRUE VALUE_TRUE
#define LEPUS_EXCEPTION VALUE_EXCEPTION
#define LEPUS_UNINITIALIZED VALUE_UNINITIALIZED

#else /* !defined(__aarch64__) || defined(OS_WIN) || DISABLE_NANBOX */

enum {
  /* all tags with a reference count are negative */
  LEPUS_TAG_FIRST = -12,            /* first negative tag */
  LEPUS_TAG_SEPARABLE_STRING = -12, /* separable string tag */
  LEPUS_TAG_LEPUS_REF = -11,        /* Primjs add for lepus */
  LEPUS_TAG_BIG_INT = -10,
  LEPUS_TAG_BIG_FLOAT = -9,
  LEPUS_TAG_SYMBOL = -8,
  LEPUS_TAG_STRING = -7,
  /* shape, async_function, var_ref is not need for PrimJs */
  LEPUS_TAG_SHAPE = -6,             /* used internally during GC */
  LEPUS_TAG_ASYNC_FUNCTION = -5,    /* used internally during GC */
  LEPUS_TAG_VAR_REF = -4,           /* used internally during GC */
  LEPUS_TAG_MODULE = -3,            /* used internally */
  LEPUS_TAG_FUNCTION_BYTECODE = -2, /* used internally */
  LEPUS_TAG_OBJECT = -1,

  LEPUS_TAG_INT = 0,
  LEPUS_TAG_BOOL = 1,
  LEPUS_TAG_NULL = 2,
  LEPUS_TAG_UNDEFINED = 3,
  LEPUS_TAG_UNINITIALIZED = 4,
  LEPUS_TAG_CATCH_OFFSET = 5,
  LEPUS_TAG_EXCEPTION = 6,

  /* below is lepus type */
  LEPUS_TAG_LEPUS_CPOINTER = 7,  // lepus CFunction
  LEPUS_TAG_FLOAT64 = 12,
  LEPUS_TAG_Atom = 13,
  /* any larger tag is FLOAT64 if LEPUS_NAN_BOXING */
};

#define LEPUS_FLOAT64_NAN NAN

#ifdef CONFIG_CHECK_JSVALUE
/* LEPUSValue consistency : it is not possible to run the code in this
   mode, but it is useful to detect simple reference counting
   errors. It would be interesting to modify a static C analyzer to
   handle specific annotations (clang has such annotations but only
   for objective C) */
typedef struct __JSValue *LEPUSValue;
typedef const struct __JSValue *LEPUSValueConst;

#define LEPUS_VALUE_GET_TAG(v) (int)((uintptr_t)(v) & 0xf)
/* same as LEPUS_VALUE_GET_TAG, but return LEPUS_TAG_FLOAT64 with NaN boxing */
#define LEPUS_VALUE_GET_NORM_TAG(v) LEPUS_VALUE_GET_TAG(v)
#define LEPUS_VALUE_GET_INT(v) (int)((intptr_t)(v) >> 4)
#define LEPUS_VALUE_GET_BOOL(v) LEPUS_VALUE_GET_INT(v)
#define LEPUS_VALUE_GET_FLOAT64(v) (double)LEPUS_VALUE_GET_INT(v)
#define LEPUS_VALUE_GET_PTR(v) (void *)((intptr_t)(v) & ~0xf)
#define LEPUS_VALUE_GET_CPOINTER(v) LEPUS_VALUE_GET_PTR(v)

#define LEPUS_MKVAL(tag, val) (LEPUSValue)(intptr_t)(((val) << 4) | (tag))
#define LEPUS_MKPTR(tag, p) (LEPUSValue)((intptr_t)(p) | (tag))

#define LEPUS_TAG_IS_FLOAT64(tag) ((unsigned)(tag) == LEPUS_TAG_FLOAT64)

#define LEPUS_NAN LEPUS_MKVAL(LEPUS_TAG_FLOAT64, 1)

static inline LEPUSValue __JS_NewFloat64(LEPUSContext *ctx, double d) {
  return LEPUS_MKVAL(LEPUS_TAG_FLOAT64, (int)d);
}

#elif defined(LEPUS_NAN_BOXING)
typedef uint64_t LEPUSValue;

#define LEPUSValueConst LEPUSValue

#define LEPUS_VALUE_GET_TAG(v) ((int)((v) >> 32))
#define LEPUS_VALUE_GET_INT(v) ((int)(v))
#define LEPUS_VALUE_GET_BOOL(v) ((int)(v))
#define LEPUS_VALUE_GET_PTR(v) ((void *)(intptr_t)(v))
#define LEPUS_VALUE_GET_CPOINTER(v) LEPUS_VALUE_GET_PTR(v)

#define LEPUS_MKVAL(tag, val) (((uint64_t)(tag) << 32) | (uint32_t)(val))
#define LEPUS_MKPTR(tag, ptr) (((uint64_t)(tag) << 32) | (uintptr_t)(ptr))

// <Primjs begin>
#define LEPUS_VALUE_GET_INT64(v) ((uint64_t)v)
// <Primjs end>

#define LEPUS_FLOAT64_TAG_ADDEND \
  (0x7ff80000 - LEPUS_TAG_FIRST + 1) /* quiet NaN encoding */

static inline double LEPUS_VALUE_GET_FLOAT64(LEPUSValue v) {
  union {
    LEPUSValue v;
    double d;
  } u;
  u.v = v;
  u.v += (uint64_t)LEPUS_FLOAT64_TAG_ADDEND << 32;
  return u.d;
}

#define LEPUS_NAN \
  (0x7ff8000000000000 - ((uint64_t)LEPUS_FLOAT64_TAG_ADDEND << 32))

static inline LEPUSValue __JS_NewFloat64(LEPUSContext *ctx, double d) {
  union {
    double d;
    uint64_t u64;
  } u;
  LEPUSValue v;
  u.d = d;
  /* normalize NaN */
  if (lepus_unlikely((u.u64 & 0x7fffffffffffffff) > 0x7ff0000000000000))
    v = LEPUS_NAN;
  else
    v = u.u64 - ((uint64_t)LEPUS_FLOAT64_TAG_ADDEND << 32);
  return v;
}

#define LEPUS_TAG_IS_FLOAT64(tag) \
  ((unsigned)((tag)-LEPUS_TAG_FIRST) >= (LEPUS_TAG_FLOAT64 - LEPUS_TAG_FIRST))

/* same as LEPUS_VALUE_GET_TAG, but return LEPUS_TAG_FLOAT64 with NaN boxing */
static inline int LEPUS_VALUE_GET_NORM_TAG(LEPUSValue v) {
  uint32_t tag;
  tag = LEPUS_VALUE_GET_TAG(v);
  if (LEPUS_TAG_IS_FLOAT64(tag))
    return LEPUS_TAG_FLOAT64;
  else
    return tag;
}

#else /* !LEPUS_NAN_BOXING */

typedef union LEPUSValueUnion {
  int32_t int32;
  double float64;
  int64_t int64;
  void *ptr;
} LEPUSValueUnion;

typedef struct LEPUSValue {
  LEPUSValueUnion u;
  int64_t tag;
} LEPUSValue;

#define LEPUSValueConst LEPUSValue

#define LEPUS_VALUE_GET_TAG(v) ((int32_t)(v).tag)
/* same as LEPUS_VALUE_GET_TAG, but return LEPUS_TAG_FLOAT64 with NaN boxing */
#define LEPUS_VALUE_GET_NORM_TAG(v) LEPUS_VALUE_GET_TAG(v)
#define LEPUS_VALUE_GET_INT(v) ((v).u.int32)
#define LEPUS_VALUE_GET_BOOL(v) ((v).u.int32)
#define LEPUS_VALUE_GET_FLOAT64(v) ((v).u.float64)
#define LEPUS_VALUE_GET_PTR(v) ((v).u.ptr)
#define LEPUS_VALUE_GET_CPOINTER(v) LEPUS_VALUE_GET_PTR(v)

// <Primjs begin>
#define LEPUS_VALUE_GET_INT64(v) ((v).u.int64)
// <Primjs end>

#define LEPUS_MKVAL(tag, val) \
  (LEPUSValue) { .u.int32 = val, tag }
#define LEPUS_MKPTR(tag, p) \
  (LEPUSValue) { .u.ptr = p, tag }

#define LEPUS_TAG_IS_FLOAT64(tag) ((unsigned)(tag) == LEPUS_TAG_FLOAT64)

#define LEPUS_NAN \
  (LEPUSValue) { .u.float64 = LEPUS_FLOAT64_NAN, LEPUS_TAG_FLOAT64 }

static inline LEPUSValue __JS_NewFloat64(LEPUSContext *ctx, double d) {
  LEPUSValue v;
  v.tag = LEPUS_TAG_FLOAT64;
  v.u.float64 = d;
  return v;
}

#endif /* !LEPUS_NAN_BOXING */

#define LEPUS_VALUE_IS_BOTH_INT(v1, v2) \
  ((LEPUS_VALUE_GET_TAG(v1) | LEPUS_VALUE_GET_TAG(v2)) == 0)
#define LEPUS_VALUE_IS_BOTH_FLOAT(v1, v2)           \
  (LEPUS_TAG_IS_FLOAT64(LEPUS_VALUE_GET_TAG(v1)) && \
   LEPUS_TAG_IS_FLOAT64(LEPUS_VALUE_GET_TAG(v2)))

// add macro to avoid accessing LEPUS_TAG directly
#define LEPUS_VALUE_IS_STRING(v) (LEPUS_VALUE_GET_TAG(v) == LEPUS_TAG_STRING)
#define LEPUS_VALUE_IS_SEPARABLE_STRING(v) \
  (LEPUS_VALUE_GET_TAG(v) == LEPUS_TAG_SEPARABLE_STRING)
#define LEPUS_VALUE_IS_OBJECT(v) (LEPUS_VALUE_GET_TAG(v) == LEPUS_TAG_OBJECT)
#define LEPUS_VALUE_IS_NOT_OBJECT(v) \
  (LEPUS_VALUE_GET_TAG(v) != LEPUS_TAG_OBJECT)
#define LEPUS_VALUE_IS_NULL(v) (LEPUS_VALUE_GET_TAG(v) == LEPUS_TAG_NULL)
#define LEPUS_VALUE_IS_UNDEFINED(v) \
  (LEPUS_VALUE_GET_TAG(v) == LEPUS_TAG_UNDEFINED)
#define LEPUS_VALUE_IS_SYMBOL(v) (LEPUS_VALUE_GET_TAG(v) == LEPUS_TAG_SYMBOL)
#define LEPUS_VALUE_IS_INT(v) (LEPUS_VALUE_GET_TAG(v) == LEPUS_TAG_INT)
#define LEPUS_VALUE_IS_CATCH_OFFSET(v) \
  (LEPUS_VALUE_GET_TAG(v) == LEPUS_TAG_CATCH_OFFSET)
#define LEPUS_VALUE_IS_FLOAT64(v) (LEPUS_TAG_IS_FLOAT64(LEPUS_VALUE_GET_TAG(v)))
#define LEPUS_VALUE_IS_EXCEPTION(v) \
  (LEPUS_VALUE_GET_TAG(v) == LEPUS_TAG_EXCEPTION)
#define LEPUS_VALUE_IS_BOOL(v) (LEPUS_VALUE_GET_TAG(v) == LEPUS_TAG_BOOL)
#define LEPUS_VALUE_IS_UNINITIALIZED(v) \
  (LEPUS_VALUE_GET_TAG(v) == LEPUS_TAG_UNINITIALIZED)
#define LEPUS_VALUE_IS_FUNCTION_BYTECODE(v) \
  (LEPUS_VALUE_GET_TAG(v) == LEPUS_TAG_FUNCTION_BYTECODE)
#define LEPUS_VALUE_IS_MODULE(v) (LEPUS_VALUE_GET_TAG(v) == LEPUS_TAG_MODULE)
#define LEPUS_VALUE_IS_LEPUS_REF(v) \
  (LEPUS_VALUE_GET_TAG(v) == LEPUS_TAG_LEPUS_REF)
#define LEPUS_VALUE_IS_LEPUS_CPOINTER(v) \
  (LEPUS_VALUE_GET_TAG(v) == LEPUS_TAG_LEPUS_CPOINTER)
#define LEPUS_VALUE_IS_BIG_INT(v) (LEPUS_VALUE_GET_TAG(v) == LEPUS_TAG_BIG_INT)

#define LEPUS_VALUE_GET_OBJ(v) ((LEPUSObject *)LEPUS_VALUE_GET_PTR(v))
#define LEPUS_VALUE_GET_STRING(v) ((JSString *)LEPUS_VALUE_GET_PTR(v))
#define LEPUS_VALUE_GET_CATCH_OFFSET(v) LEPUS_VALUE_GET_INT((v))
#define LEPUS_VALUE_HAS_REF_COUNT(v) \
  ((unsigned)LEPUS_VALUE_GET_TAG(v) >= (unsigned)LEPUS_TAG_FIRST)

/* special values */
#define LEPUS_NULL LEPUS_MKVAL(LEPUS_TAG_NULL, 0)
#define LEPUS_UNDEFINED LEPUS_MKVAL(LEPUS_TAG_UNDEFINED, 0)
#define LEPUS_FALSE LEPUS_MKVAL(LEPUS_TAG_BOOL, 0)
#define LEPUS_TRUE LEPUS_MKVAL(LEPUS_TAG_BOOL, 1)
#define LEPUS_EXCEPTION LEPUS_MKVAL(LEPUS_TAG_EXCEPTION, 0)
#define LEPUS_UNINITIALIZED LEPUS_MKVAL(LEPUS_TAG_UNINITIALIZED, 0)

#endif /* !defined(__aarch64__) || defined(OS_WIN) || DISABLE_NANBOX */

/* flags for object properties */
#define LEPUS_PROP_CONFIGURABLE (1 << 0)
#define LEPUS_PROP_WRITABLE (1 << 1)
#define LEPUS_PROP_ENUMERABLE (1 << 2)
#define LEPUS_PROP_C_W_E \
  (LEPUS_PROP_CONFIGURABLE | LEPUS_PROP_WRITABLE | LEPUS_PROP_ENUMERABLE)
#define LEPUS_PROP_LENGTH (1 << 3) /* used internally in Arrays */
#define LEPUS_PROP_TMASK \
  (3 << 4) /* mask for NORMAL, GETSET, VARREF, AUTOINIT */
#define LEPUS_PROP_NORMAL (0 << 4)
#define LEPUS_PROP_GETSET (1 << 4)
#define LEPUS_PROP_VARREF (2 << 4)   /* used internally */
#define LEPUS_PROP_AUTOINIT (3 << 4) /* used internally */

/* flags for LEPUS_DefineProperty */
#define LEPUS_PROP_HAS_SHIFT 8
#define LEPUS_PROP_HAS_CONFIGURABLE (1 << 8)
#define LEPUS_PROP_HAS_WRITABLE (1 << 9)
#define LEPUS_PROP_HAS_ENUMERABLE (1 << 10)
#define LEPUS_PROP_HAS_GET (1 << 11)
#define LEPUS_PROP_HAS_SET (1 << 12)
#define LEPUS_PROP_HAS_VALUE (1 << 13)

/* throw an exception if false would be returned
   (LEPUS_DefineProperty/LEPUS_SetProperty) */
#define LEPUS_PROP_THROW (1 << 14)
/* throw an exception if false would be returned in strict mode
   (LEPUS_SetProperty) */
#define LEPUS_PROP_THROW_STRICT (1 << 15)

#define LEPUS_PROP_NO_ADD (1 << 16)    /* internal use */
#define LEPUS_PROP_NO_EXOTIC (1 << 17) /* internal use */

#define LEPUS_DEFAULT_STACK_SIZE (256 * 1024)

/* LEPUS_Eval() flags */
#define LEPUS_EVAL_TYPE_GLOBAL (0 << 0)   /* global code (default) */
#define LEPUS_EVAL_TYPE_MODULE (1 << 0)   /* module code */
#define LEPUS_EVAL_TYPE_DIRECT (2 << 0)   /* direct call (internal use) */
#define LEPUS_EVAL_TYPE_INDIRECT (3 << 0) /* indirect call (internal use) */
#define LEPUS_EVAL_TYPE_MASK (3 << 0)

#define LEPUS_EVAL_FLAG_STRICT (1 << 3)       /* force 'strict' mode */
#define LEPUS_EVAL_FLAG_STRIP (1 << 4)        /* force 'strip' mode */
#define LEPUS_EVAL_FLAG_COMPILE_ONLY (1 << 5) /* internal use */
/* use for runtime.compileScript */
#define LEPUS_DEBUGGER_NO_PERSIST_SCRIPT (1 << 6)

typedef LEPUSValue LEPUSCFunction(LEPUSContext *ctx, LEPUSValueConst this_val,
                                  int argc, LEPUSValueConst *argv);
typedef LEPUSValue LEPUSCFunctionMagic(LEPUSContext *ctx,
                                       LEPUSValueConst this_val, int argc,
                                       LEPUSValueConst *argv, int magic);
typedef LEPUSValue LEPUSCFunctionData(LEPUSContext *ctx,
                                      LEPUSValueConst this_val, int argc,
                                      LEPUSValueConst *argv, int magic,
                                      LEPUSValue *func_data);

struct LEPUSMallocFunctions;
// <Primjs begin>
typedef struct LEPUSDebuggerCallbacks {
  // callbacks for lepusNG debugger
  void (*run_message_loop_on_pause)(LEPUSContext *ctx);
  void (*quit_message_loop_on_pause)(LEPUSContext *ctx);
  void (*get_messages)(LEPUSContext *ctx);
  void (*send_response)(LEPUSContext *ctx, int32_t message_id,
                        const char *message);
  void (*send_notification)(LEPUSContext *ctx, const char *message);
  void (*free_messages)(LEPUSContext *ctx, char **messages, int32_t size);

  void (*inspector_check)(LEPUSContext *ctx);
  void (*debugger_exception)(LEPUSContext *ctx);
  uint8_t (*is_devtool_on)(LEPUSContext *ctx);
} LEPUSDebuggerCallbacks;

typedef struct LEPUSLepusRefCallbacks {
  LEPUSValue (*free_value)(LEPUSRuntime *rt, LEPUSValue val);
  LEPUSValue (*get_property)(LEPUSContext *ctx, LEPUSValue thisObj, JSAtom prop,
                             int idx);
  size_t (*get_length)(LEPUSContext *ctx, LEPUSValue val);
  LEPUSValue (*convert_to_object)(LEPUSContext *ctx, LEPUSValue val);
  LEPUSValue (*set_property)(LEPUSContext *ctx, LEPUSValue thisObj,
                             LEPUSValue prop, int idx, LEPUSValue val);
  void (*free_str_cache)(void *old_ptr, void *new_ptr);
  size_t (*lepus_ref_equal)(LEPUSValue val1, LEPUSValue val2);
  LEPUSValue (*lepus_ref_tostring)(LEPUSContext *ctx, LEPUSValue val);
  // void (*free_string_cache)();
} LEPUSLepusRefCallbacks;

typedef struct LEPUSLepusRef {
  LEPUSRefCountHeader header;
  int tag;               // lepus value tag
  void *p;               // lepus value reference
  LEPUSValue lepus_val;  // convert to lepusvalue cache, default is undefined
} LEPUSLepusRef;

void RegisterLepusType(LEPUSRuntime *rt, int32_t array_typeid,
                       int32_t table_typeid);

void RegisterGCInfoCallback(LEPUSRuntime *rt, void (*func)(const char *, int));

void RegisterLepusRefCallbacks(LEPUSRuntime *rt, LEPUSLepusRefCallbacks *funcs);

void RegisterPrimJSCallbacks(LEPUSRuntime *rt, void **funcs,
                             int32_t callback_size);
void RegisterQJSDebuggerCallbacks(LEPUSRuntime *rt, void **funcs,
                                  int32_t callback_size);
void PrepareQJSDebuggerDefer(LEPUSContext *ctx, void **funcs,
                             int32_t callback_size);
// for shared context
void PrepareQJSDebuggerForSharedContext(LEPUSContext *ctx, void **funcs,
                                        int32_t callback_size,
                                        bool devtool_connect);

// <Primjs end>

LEPUSRuntime *LEPUS_NewRuntime(void);
LEPUSRuntime *LEPUS_NewRuntimeWithMode(uint32_t mode);
/* info lifetime must exceed that of rt */
void LEPUS_SetRuntimeInfo(LEPUSRuntime *rt, const char *info);
void LEPUS_SetMemoryLimit(LEPUSRuntime *rt, size_t limit);
void LEPUS_SetGCThreshold(LEPUSRuntime *rt, size_t gc_threshold);
LEPUSRuntime *LEPUS_NewRuntime2(const struct LEPUSMallocFunctions *mf,
                                void *opaque, uint32_t mode);
void LEPUS_FreeRuntime(LEPUSRuntime *rt);
typedef void LEPUS_MarkFunc(LEPUSRuntime *rt, LEPUSValueConst val,
                            int local_idx);
void LEPUS_MarkValue(LEPUSRuntime *rt, LEPUSValueConst val,
                     LEPUS_MarkFunc *mark_func, int local_idx);
void LEPUS_RunGC(LEPUSRuntime *rt);
LEPUS_BOOL LEPUS_IsInGCSweep(LEPUSRuntime *rt);

LEPUSContext *LEPUS_NewContext(LEPUSRuntime *rt);
void LEPUS_FreeContext(LEPUSContext *s);
void *LEPUS_GetContextOpaque(LEPUSContext *ctx);
void LEPUS_SetContextOpaque(LEPUSContext *ctx, void *opaque);
LEPUSRuntime *LEPUS_GetRuntime(LEPUSContext *ctx);
void LEPUS_SetMaxStackSize(LEPUSContext *ctx, size_t stack_size);
void LEPUS_SetClassProto(LEPUSContext *ctx, LEPUSClassID class_id,
                         LEPUSValue obj);
LEPUSValue LEPUS_GetClassProto(LEPUSContext *ctx, LEPUSClassID class_id);
int LEPUS_MoveUnhandledRejectionToException(LEPUSContext *ctx);
size_t LEPUS_GetHeapSize(LEPUSRuntime *rt);
/* the following functions are used to select the intrinsic object to
   save memory */
LEPUSContext *LEPUS_NewContextRaw(LEPUSRuntime *rt);
QJS_HIDE void LEPUS_AddIntrinsicBaseObjects(LEPUSContext *ctx);
QJS_HIDE void LEPUS_AddIntrinsicDate(LEPUSContext *ctx);
QJS_HIDE void LEPUS_AddIntrinsicEval(LEPUSContext *ctx);
QJS_HIDE void LEPUS_AddIntrinsicStringNormalize(LEPUSContext *ctx);
QJS_HIDE void LEPUS_AddIntrinsicRegExpCompiler(LEPUSContext *ctx);
QJS_HIDE void LEPUS_AddIntrinsicRegExp(LEPUSContext *ctx);
QJS_HIDE void LEPUS_AddIntrinsicJSON(LEPUSContext *ctx);
QJS_HIDE void LEPUS_AddIntrinsicProxy(LEPUSContext *ctx);
QJS_HIDE void LEPUS_AddIntrinsicMapSet(LEPUSContext *ctx);
QJS_HIDE void LEPUS_AddIntrinsicTypedArrays(LEPUSContext *ctx);
QJS_HIDE void LEPUS_AddIntrinsicPromise(LEPUSContext *ctx);
#ifdef QJS_UNITTEST
QJS_HIDE LEPUSValue lepus_string_codePointRange(LEPUSContext *ctx,
                                                LEPUSValueConst this_val,
                                                int argc,
                                                LEPUSValueConst *argv);

QJS_HIDE LEPUSValue lepus_gc(LEPUSContext *ctx, LEPUSValueConst this_val,
                             int argc, LEPUSValueConst *argv);
#endif
QJS_HIDE void *lepus_malloc_rt(LEPUSRuntime *rt, size_t size, int alloc_tag);
void lepus_free_rt(LEPUSRuntime *rt, void *ptr);
QJS_HIDE void *lepus_realloc_rt(LEPUSRuntime *rt, void *ptr, size_t size,
                                int alloc_tag);
QJS_HIDE size_t lepus_malloc_usable_size_rt(LEPUSRuntime *rt, const void *ptr);
QJS_HIDE void *lepus_mallocz_rt(LEPUSRuntime *rt, size_t size, int alloc_tag);

void *lepus_malloc(LEPUSContext *ctx, size_t size, int alloc_tag);
void lepus_free(LEPUSContext *ctx, void *ptr);
void *lepus_mallocz(LEPUSContext *ctx, size_t size, int alloc_tag);
QJS_HIDE void *lepus_realloc(LEPUSContext *ctx, void *ptr, size_t size,
                             int alloc_tag);
QJS_HIDE size_t lepus_malloc_usable_size(LEPUSContext *ctx, const void *ptr);
QJS_HIDE void *lepus_realloc2(LEPUSContext *ctx, void *ptr, size_t size,
                              size_t *pslack, int alloc_tag);
QJS_HIDE char *lepus_strdup(LEPUSContext *ctx, const char *str, int alloc_tag);
QJS_HIDE char *lepus_strndup(LEPUSContext *ctx, const char *s, size_t n,
                             int alloc_tag);

#if LYNX_SIMPLIFY
typedef struct LEPUSMemoryUsage {
  int64_t malloc_size, malloc_limit, memory_used_size;
  int64_t malloc_count;
  int64_t memory_used_count;
  int64_t atom_count, atom_size;
  int64_t str_count, str_size;
  int64_t obj_count, obj_size;
  int64_t prop_count, prop_size;
  int64_t shape_count, shape_size;
  int64_t lepus_func_count, lepus_func_size, lepus_func_code_size;
  int64_t lepus_func_pc2line_count, lepus_func_pc2line_size;
  int64_t c_func_count, array_count;
  int64_t fast_array_count, fast_array_elements;
  int64_t binary_object_count, binary_object_size;
} LEPUSMemoryUsage;

void LEPUS_ComputeMemoryUsage(LEPUSRuntime *rt, LEPUSMemoryUsage *s);
void LEPUS_DumpMemoryUsage(FILE *fp, const LEPUSMemoryUsage *s,
                           LEPUSRuntime *rt);

#endif

/* atom support */
JSAtom LEPUS_NewAtomLen(LEPUSContext *ctx, const char *str, size_t len);
JSAtom LEPUS_NewAtom(LEPUSContext *ctx, const char *str);
JSAtom LEPUS_NewAtomUInt32(LEPUSContext *ctx, uint32_t n);
JSAtom LEPUS_DupAtom(LEPUSContext *ctx, JSAtom v);
void LEPUS_FreeAtom(LEPUSContext *ctx, JSAtom v);
void LEPUS_FreeAtomRT(LEPUSRuntime *rt, JSAtom v);
LEPUSValue LEPUS_AtomToValue(LEPUSContext *ctx, JSAtom atom);
LEPUSValue LEPUS_AtomToString(LEPUSContext *ctx, JSAtom atom);
const char *LEPUS_AtomToCString(LEPUSContext *ctx, JSAtom atom);

/* object class support */

typedef struct LEPUSPropertyEnum {
  LEPUS_BOOL is_enumerable;
  JSAtom atom;
} LEPUSPropertyEnum;

typedef struct LEPUSPropertyDescriptor {
  int flags;
  LEPUSValue value;
  LEPUSValue getter;
  LEPUSValue setter;
} LEPUSPropertyDescriptor;

typedef struct LEPUSClassExoticMethods {
  /* Return -1 if exception (can only happen in case of Proxy object),
     FALSE if the property does not exists, TRUE if it exists. If 1 is
     returned, the property descriptor 'desc' is filled if != NULL. */
  int (*get_own_property)(LEPUSContext *ctx, LEPUSPropertyDescriptor *desc,
                          LEPUSValueConst obj, JSAtom prop);
  /* '*ptab' should hold the '*plen' property keys. Return 0 if OK,
     -1 if exception. The 'is_enumerable' field is ignored.
  */
  int (*get_own_property_names)(LEPUSContext *ctx, LEPUSPropertyEnum **ptab,
                                uint32_t *plen, LEPUSValueConst obj);
  /* return < 0 if exception, or TRUE/FALSE */
  int (*delete_property)(LEPUSContext *ctx, LEPUSValueConst obj, JSAtom prop);
  /* return < 0 if exception or TRUE/FALSE */
  int (*define_own_property)(LEPUSContext *ctx, LEPUSValueConst this_obj,
                             JSAtom prop, LEPUSValueConst val,
                             LEPUSValueConst getter, LEPUSValueConst setter,
                             int flags);
  /* The following methods can be emulated with the previous ones,
     so they are usually not needed */
  /* return < 0 if exception or TRUE/FALSE */
  int (*has_property)(LEPUSContext *ctx, LEPUSValueConst obj, JSAtom atom);
  LEPUSValue (*get_property)(LEPUSContext *ctx, LEPUSValueConst obj,
                             JSAtom atom, LEPUSValueConst receiver);
  /* return < 0 if exception or TRUE/FALSE */
  int (*set_property)(LEPUSContext *ctx, LEPUSValueConst obj, JSAtom atom,
                      LEPUSValueConst value, LEPUSValueConst receiver,
                      int flags);
} LEPUSClassExoticMethods;

typedef void LEPUSClassFinalizer(LEPUSRuntime *rt, LEPUSValue val);
typedef void LEPUSClassGCMark(LEPUSRuntime *rt, LEPUSValueConst val,
                              LEPUS_MarkFunc *mark_func, int local_idx);
#define LEPUS_CALL_FLAG_CONSTRUCTOR (1 << 0)
typedef LEPUSValue LEPUSClassCall(LEPUSContext *ctx, LEPUSValueConst func_obj,
                                  LEPUSValueConst this_val, int argc,
                                  LEPUSValueConst *argv, int flags);
typedef struct LEPUSClassDef {
  const char *class_name;
  LEPUSClassFinalizer *finalizer;
  LEPUSClassGCMark *gc_mark;
  /* if call != NULL, the object is a function. If (flags &
       JS_CALL_FLAG_CONSTRUCTOR) != 0, the function is called as a
       constructor. In this case, 'this_val' is new.target. A
       constructor call only happens if the object constructor bit is
       set (see JS_SetConstructorBit()). */
  LEPUSClassCall *call;
  /* XXX: suppress this indirection ? It is here only to save memory
     because only a few classes need these methods */
  LEPUSClassExoticMethods *exotic;
} LEPUSClassDef;

LEPUSClassID LEPUS_NewClassID(LEPUSClassID *pclass_id);
int LEPUS_NewClass(LEPUSRuntime *rt, LEPUSClassID class_id,
                   const LEPUSClassDef *class_def);
int LEPUS_IsRegisteredClass(LEPUSRuntime *rt, LEPUSClassID class_id);

/* value handling */

static lepus_force_inline LEPUSValue LEPUS_NewBool(LEPUSContext *ctx,
                                                   LEPUS_BOOL val) {
#if defined(__aarch64__) && !defined(OS_WIN) && !defined(CONFIG_BIGNUM) && \
    !DISABLE_NANBOX
  return LEPUS_MKVAL(LEPUS_TAG_BOOL, val != 0);
#else
  return LEPUS_MKVAL(LEPUS_TAG_BOOL, val);
#endif
}

static lepus_force_inline LEPUSValue LEPUS_NewInt32(LEPUSContext *ctx,
                                                    int32_t val) {
  return LEPUS_MKVAL(LEPUS_TAG_INT, val);
}

static lepus_force_inline LEPUSValue LEPUS_NewCatchOffset(LEPUSContext *ctx,
                                                          int32_t val) {
  return LEPUS_MKVAL(LEPUS_TAG_CATCH_OFFSET, val);
}

LEPUSValue LEPUS_NewInt64(LEPUSContext *ctx, int64_t v);
LEPUSValue LEPUS_NewBigInt64(LEPUSContext *ctx, int64_t v);
LEPUSValue LEPUS_NewBigUint64(LEPUSContext *ctx, uint64_t v);

static lepus_force_inline LEPUSValue LEPUS_NewFloat64(LEPUSContext *ctx,
                                                      double d) {
  LEPUSValue v;
  int32_t val;
  union {
    double d;
    uint64_t u;
  } u, t;
  u.d = d;
  val = (int32_t)d;
  t.d = val;
  /* -0 cannot be represented as integer, so we compare the bit
      representation */
  if (u.u == t.u) {
    v = LEPUS_MKVAL(LEPUS_TAG_INT, val);
  } else {
    v = __JS_NewFloat64(ctx, d);
  }
  return v;
}

LEPUS_BOOL LEPUS_IsNumber(LEPUSValueConst v);

static inline LEPUS_BOOL LEPUS_IsInteger(LEPUSValueConst v) {
  return LEPUS_VALUE_IS_INT(v) || LEPUS_VALUE_IS_BIG_INT(v);
}

#ifdef CONFIG_BIGNUM
static inline LEPUS_BOOL LEPUS_IsBigFloat(LEPUSValueConst v) {
  int tag = LEPUS_VALUE_GET_TAG(v);
  return tag == LEPUS_TAG_BIG_FLOAT;
}
#endif

static inline LEPUS_BOOL LEPUS_IsBool(LEPUSValueConst v) {
  return LEPUS_VALUE_IS_BOOL(v);
}

static inline LEPUS_BOOL LEPUS_IsNull(LEPUSValueConst v) {
  return LEPUS_VALUE_IS_NULL(v);
}

static inline LEPUS_BOOL LEPUS_IsUndefined(LEPUSValueConst v) {
  return LEPUS_VALUE_IS_UNDEFINED(v);
}

static inline LEPUS_BOOL LEPUS_IsException(LEPUSValueConst v) {
  return (LEPUS_BOOL)(lepus_unlikely(LEPUS_VALUE_IS_EXCEPTION(v)));
}

static inline LEPUS_BOOL LEPUS_IsUninitialized(LEPUSValueConst v) {
  return (LEPUS_BOOL)(lepus_unlikely(LEPUS_VALUE_IS_UNINITIALIZED(v)));
}

static inline LEPUS_BOOL LEPUS_IsString(LEPUSValueConst v) {
  return LEPUS_VALUE_IS_STRING(v) || LEPUS_VALUE_IS_SEPARABLE_STRING(v);
}

static inline LEPUS_BOOL LEPUS_IsSymbol(LEPUSValueConst v) {
  return LEPUS_VALUE_IS_SYMBOL(v);
}

static inline LEPUS_BOOL LEPUS_IsObject(LEPUSValueConst v) {
  return LEPUS_VALUE_IS_OBJECT(v);
}

LEPUSValue LEPUS_Throw(LEPUSContext *ctx, LEPUSValue obj);
LEPUSValue LEPUS_GetException(LEPUSContext *ctx);
LEPUS_BOOL LEPUS_IsError(LEPUSContext *ctx, LEPUSValueConst val);
#ifdef QJS_UNITTEST
void LEPUS_EnableIsErrorProperty(LEPUSContext *ctx, LEPUS_BOOL enable);
#endif
void LEPUS_ResetUncatchableError(LEPUSContext *ctx);
LEPUSValue LEPUS_NewError(LEPUSContext *ctx);
LEPUSValue __js_printf_like(2, 3)
    LEPUS_ThrowSyntaxError(LEPUSContext *ctx, const char *fmt, ...);
LEPUSValue __js_printf_like(2, 3)
    LEPUS_ThrowTypeError(LEPUSContext *ctx, const char *fmt, ...);
LEPUSValue __js_printf_like(2, 3)
    LEPUS_ThrowReferenceError(LEPUSContext *ctx, const char *fmt, ...);
LEPUSValue __js_printf_like(2, 3)
    LEPUS_ThrowRangeError(LEPUSContext *ctx, const char *fmt, ...);
LEPUSValue __js_printf_like(2, 3)
    LEPUS_ThrowInternalError(LEPUSContext *ctx, const char *fmt, ...);
LEPUSValue LEPUS_ThrowOutOfMemory(LEPUSContext *ctx);

void __JS_FreeValue(LEPUSContext *ctx, LEPUSValue v);
void LEPUS_FreeValue(LEPUSContext *ctx, LEPUSValue v);
QJS_HIDE void __JS_FreeValueRT(LEPUSRuntime *rt, LEPUSValue v);
void LEPUS_FreeValueRT(LEPUSRuntime *rt, LEPUSValue v);

bool LEPUS_IsGCMode(LEPUSContext *ctx);
bool LEPUS_IsGCModeRT(LEPUSRuntime *rt);

char *LEPUS_GetGCTimingInfo(LEPUSContext *ctx, bool is_start);

void LEPUS_PushHandle(LEPUSContext *ctx, void *ptr, int type);
void LEPUS_ResetHandle(LEPUSContext *ctx, void *ptr, int type);

static inline LEPUSValue LEPUS_DupValue(LEPUSContext *ctx, LEPUSValueConst v) {
  if (LEPUS_VALUE_HAS_REF_COUNT(v)) {
    LEPUSRefCountHeader *p = (LEPUSRefCountHeader *)LEPUS_VALUE_GET_PTR(v);
    p->ref_count++;
  }
  return (LEPUSValue)v;
}

// <Primjs begin>
// if no_lepus_strict_mode is set to true, these conditions will handle
// differently: if the object is null or undefined, read properties will return
// null if the object is null or undefined, write properties will not throw
// exception more info:
void LEPUS_SetNoStrictMode(LEPUSContext *ctx);
// <Primjs end>

// <Primjs begin>
void LEPUS_SetVirtualStackSize(LEPUSContext *ctx, uint32_t stack_size);
// <Primjs end>

static inline LEPUSValue LEPUS_DupValueRT(LEPUSRuntime *rt, LEPUSValueConst v) {
  if (LEPUS_VALUE_HAS_REF_COUNT(v)) {
    LEPUSRefCountHeader *p = (LEPUSRefCountHeader *)LEPUS_VALUE_GET_PTR(v);
    p->ref_count++;
  }
  return (LEPUSValue)v;
}

int LEPUS_ToBool(LEPUSContext *ctx,
                 LEPUSValueConst val); /* return -1 for LEPUS_EXCEPTION */
int LEPUS_ToInt32(LEPUSContext *ctx, int32_t *pres, LEPUSValueConst val);
static int inline LEPUS_ToUint32(LEPUSContext *ctx, uint32_t *pres,
                                 LEPUSValueConst val) {
  return LEPUS_ToInt32(ctx, (int32_t *)pres, val);
}
int LEPUS_ToInt64(LEPUSContext *ctx, int64_t *pres, LEPUSValueConst val);
int LEPUS_ToIndex(LEPUSContext *ctx, uint64_t *plen, LEPUSValueConst val);
int LEPUS_ToFloat64(LEPUSContext *ctx, double *pres, LEPUSValueConst val);
int LEPUS_ToBigInt64(LEPUSContext *ctx, int64_t *pres, LEPUSValueConst val);

LEPUSValue LEPUS_NewStringLen(LEPUSContext *ctx, const char *str1, size_t len1);
LEPUSValue LEPUS_NewString(LEPUSContext *ctx, const char *str);
LEPUSValue LEPUS_NewAtomString(LEPUSContext *ctx, const char *str);
LEPUSValue LEPUS_ToString(LEPUSContext *ctx, LEPUSValueConst val);
LEPUSValue LEPUS_ToPropertyKey(LEPUSContext *ctx, LEPUSValueConst val);
LEPUSValue LEPUS_ToObject(LEPUSContext *ctx, LEPUSValueConst val);
const char *LEPUS_ToCStringLen2(LEPUSContext *ctx, size_t *plen,
                                LEPUSValueConst val1, LEPUS_BOOL cesu8);
static inline const char *LEPUS_ToCStringLen(LEPUSContext *ctx, size_t *plen,
                                             LEPUSValueConst val1) {
  return LEPUS_ToCStringLen2(ctx, plen, val1, 0);
}
static inline const char *LEPUS_ToCString(LEPUSContext *ctx,
                                          LEPUSValueConst val1) {
  return LEPUS_ToCStringLen2(ctx, NULL, val1, 0);
}
void LEPUS_FreeCString(LEPUSContext *ctx, const char *ptr);

LEPUSValue LEPUS_NewObjectProtoClass(LEPUSContext *ctx, LEPUSValueConst proto,
                                     LEPUSClassID class_id);
LEPUSValue LEPUS_NewObjectClass(LEPUSContext *ctx, int class_id);
LEPUSValue LEPUS_NewObjectProto(LEPUSContext *ctx, LEPUSValueConst proto);
LEPUSValue LEPUS_NewObject(LEPUSContext *ctx);

LEPUS_BOOL LEPUS_IsFunction(LEPUSContext *ctx, LEPUSValueConst val);
LEPUS_BOOL LEPUS_IsConstructor(LEPUSContext *ctx, LEPUSValueConst val);
LEPUS_BOOL LEPUS_SetConstructorBit(LEPUSContext *ctx, LEPUSValueConst func_obj,
                                   LEPUS_BOOL val);

LEPUSValue LEPUS_NewArray(LEPUSContext *ctx);
int LEPUS_IsArray(LEPUSContext *ctx, LEPUSValueConst val);

LEPUSValue LEPUS_GetPropertyInternal(LEPUSContext *ctx, LEPUSValueConst obj,
                                     JSAtom prop, LEPUSValueConst receiver,
                                     LEPUS_BOOL throw_ref_error);
static lepus_force_inline LEPUSValue LEPUS_GetProperty(LEPUSContext *ctx,
                                                       LEPUSValueConst this_obj,
                                                       JSAtom prop) {
  return LEPUS_GetPropertyInternal(ctx, this_obj, prop, this_obj, 0);
}
LEPUSValue LEPUS_GetPropertyStr(LEPUSContext *ctx, LEPUSValueConst this_obj,
                                const char *prop);
LEPUSValue LEPUS_GetPropertyUint32(LEPUSContext *ctx, LEPUSValueConst this_obj,
                                   uint32_t idx);

int LEPUS_SetPropertyInternal(LEPUSContext *ctx, LEPUSValueConst this_obj,
                              JSAtom prop, LEPUSValue val, int flags);
static inline int LEPUS_SetProperty(LEPUSContext *ctx, LEPUSValueConst this_obj,
                                    JSAtom prop, LEPUSValue val) {
  return LEPUS_SetPropertyInternal(ctx, this_obj, prop, val, LEPUS_PROP_THROW);
}
int LEPUS_SetPropertyUint32(LEPUSContext *ctx, LEPUSValueConst this_obj,
                            uint32_t idx, LEPUSValue val);
int LEPUS_SetPropertyInt64(LEPUSContext *ctx, LEPUSValueConst this_obj,
                           int64_t idx, LEPUSValue val);
int LEPUS_SetPropertyStr(LEPUSContext *ctx, LEPUSValueConst this_obj,
                         const char *prop, LEPUSValue val);
int LEPUS_HasProperty(LEPUSContext *ctx, LEPUSValueConst this_obj, JSAtom prop);
int LEPUS_IsExtensible(LEPUSContext *ctx, LEPUSValueConst obj);
int LEPUS_PreventExtensions(LEPUSContext *ctx, LEPUSValueConst obj);
int LEPUS_DeleteProperty(LEPUSContext *ctx, LEPUSValueConst obj, JSAtom prop,
                         int flags);
int LEPUS_SetPrototype(LEPUSContext *ctx, LEPUSValueConst obj,
                       LEPUSValueConst proto_val);
LEPUSValueConst LEPUS_GetPrototype(LEPUSContext *ctx, LEPUSValueConst val);

#define LEPUS_GPN_STRING_MASK (1 << 0)
#define LEPUS_GPN_SYMBOL_MASK (1 << 1)
#define LEPUS_GPN_PRIVATE_MASK (1 << 2)
/* only include the enumerable properties */
#define LEPUS_GPN_ENUM_ONLY (1 << 4)
/* set theJSPropertyEnum.is_enumerable field */
#define LEPUS_GPN_SET_ENUM (1 << 5)

int LEPUS_GetOwnPropertyNames(LEPUSContext *ctx, LEPUSPropertyEnum **ptab,
                              uint32_t *plen, LEPUSValueConst obj, int flags);
int LEPUS_GetOwnProperty(LEPUSContext *ctx, LEPUSPropertyDescriptor *desc,
                         LEPUSValueConst obj, JSAtom prop);

LEPUSValue LEPUS_ParseJSON(LEPUSContext *ctx, const char *buf, size_t buf_len,
                           const char *filename);
LEPUSValue LEPUS_Call(LEPUSContext *ctx, LEPUSValueConst func_obj,
                      LEPUSValueConst this_obj, int argc,
                      LEPUSValueConst *argv);
LEPUSValue LEPUS_Invoke(LEPUSContext *ctx, LEPUSValueConst this_val,
                        JSAtom atom, int argc, LEPUSValueConst *argv);
LEPUSValue LEPUS_CallConstructor(LEPUSContext *ctx, LEPUSValueConst func_obj,
                                 int argc, LEPUSValueConst *argv);
LEPUSValue LEPUS_CallConstructor2(LEPUSContext *ctx, LEPUSValueConst func_obj,
                                  LEPUSValueConst new_target, int argc,
                                  LEPUSValueConst *argv);
LEPUSValue LEPUS_Eval(LEPUSContext *ctx, const char *input, size_t input_len,
                      const char *filename, int eval_flags);
#define LEPUS_EVAL_BINARY_LOAD_ONLY (1 << 0) /* only load the module */
LEPUSValue LEPUS_EvalBinary(LEPUSContext *ctx, const uint8_t *buf,
                            size_t buf_len, int flags);
LEPUSValue LEPUS_GetGlobalObject(LEPUSContext *ctx);

/* trace gc begin */
void LEPUS_SetGCPauseSuppressionMode(LEPUSRuntime *rt, bool mode);
bool LEPUS_GetGCPauseSuppressionMode(LEPUSRuntime *rt);

void DisposeGlobal(LEPUSRuntime *runtime, LEPUSValue *global_handle);
LEPUSValue *GlobalizeReference(LEPUSRuntime *runtime, LEPUSValue val,
                               bool is_weak);

void *AllocateQJSValueValue(LEPUSRuntime *runtime);
void FreeQJSValueValue(LEPUSRuntime *runtime, void *instance);

void SetGlobalWeak(LEPUSRuntime *runtime, LEPUSValue *global_handle, void *data,
                   void (*cb)(void *));
void ClearGlobalWeak(LEPUSRuntime *runtime, LEPUSValue *global_handle);
void SetWeakState(LEPUSRuntime *runtime, LEPUSValue *global_handle);

void *GetNapiScope(LEPUSContext *ctx);
void SetNapiScope(LEPUSContext *ctx, void *scope);
void InitNapiScope(LEPUSContext *ctx);
void FreeNapiScope(LEPUSContext *ctx);

void LEPUS_VisitLEPUSValue(LEPUSRuntime *rt, LEPUSValue *val, int local_idx);

void AddCurNode(LEPUSRuntime *rt, void *node, int type);

bool CheckValidPtr(void *runtime, void *ptr);

void LEPUS_TrigGC(LEPUSRuntime *rt);
/* trace gc end*/

LEPUSValue LEPUS_GetGlobalVar(LEPUSContext *ctx, JSAtom prop,
                              LEPUS_BOOL throw_ref_error);
// <Primjs change>
// #ifdef ENABLE_LEPUSNG
/* flag = 0: normal variable write
   flag = 1: initialize lexical variable
   flag = 2: normal variable write, strict check was done before
*/
typedef void (*IterateObject)(LEPUSContext *ctx, LEPUSValue key,
                              LEPUSValue value, void *p, void *raw_data);
void LEPUS_SetStringCache(LEPUSContext *ctx, LEPUSValue val, void *p);
void *LEPUS_GetStringCache(LEPUSValue val);
void *LEPUS_GetStringCache_GC(LEPUSValue val);
int LEPUS_SetGlobalVar(LEPUSContext *ctx, JSAtom prop, LEPUSValue val,
                       int flag);
LEPUSValue LEPUS_DeepEqual(LEPUSContext *ctx, LEPUSValueConst obj1,
                           LEPUSValueConst obj2);
LEPUSValue LEPUS_DeepCopy(LEPUSContext *ctx, LEPUSValueConst val);
void LEPUS_IterateObject(LEPUSContext *ctx, LEPUSValue obj,
                         IterateObject callback, void *p, void *raw_data);
int LEPUS_GetLength(LEPUSContext *ctx, LEPUSValue val);

LEPUSValue LEPUS_NewLepusWrap(LEPUSContext *ctx, void *p, int tag);
// check if has lepus refs count header
static inline LEPUS_BOOL LEPUS_IsLepusRef(LEPUSValue val) {
  return LEPUS_VALUE_IS_LEPUS_REF(val);
}

static inline int LEPUS_GetLepusRefTag(LEPUSValue val) {
  if (!LEPUS_IsLepusRef(val)) return -1;
  LEPUSLepusRef *pref = (LEPUSLepusRef *)(LEPUS_VALUE_GET_PTR(val));
  return pref->tag;
}

static inline void *LEPUS_GetLepusRefPoint(LEPUSValue val) {
  if (!LEPUS_IsLepusRef(val)) return NULL;
  LEPUSLepusRef *pref = (LEPUSLepusRef *)(LEPUS_VALUE_GET_PTR(val));
  return pref->p;
}

// #define CONFIG_BIGNUM
// #endif
// <Primjs end>
int LEPUS_IsInstanceOf(LEPUSContext *ctx, LEPUSValueConst val,
                       LEPUSValueConst obj);
int LEPUS_DefineProperty(LEPUSContext *ctx, LEPUSValueConst this_obj,
                         JSAtom prop, LEPUSValueConst val,
                         LEPUSValueConst getter, LEPUSValueConst setter,
                         int flags);
// It is recommended to call LEPUS_DupValue to refer the original LEPUSValue val
// before calling the following define methods, otherwise it may crash
int LEPUS_DefinePropertyValue(LEPUSContext *ctx, LEPUSValueConst this_obj,
                              JSAtom prop, LEPUSValue val, int flags);
int LEPUS_DefinePropertyValueUint32(LEPUSContext *ctx, LEPUSValueConst this_obj,
                                    uint32_t idx, LEPUSValue val, int flags);
int LEPUS_DefinePropertyValueStr(LEPUSContext *ctx, LEPUSValueConst this_obj,
                                 const char *prop, LEPUSValue val, int flags);
int LEPUS_DefinePropertyGetSet(LEPUSContext *ctx, LEPUSValueConst this_obj,
                               JSAtom prop, LEPUSValue getter,
                               LEPUSValue setter, int flags);
void LEPUS_SetOpaque(LEPUSValue obj, void *opaque);
void *LEPUS_GetOpaque(LEPUSValueConst obj, LEPUSClassID class_id);
void *LEPUS_GetOpaque2(LEPUSContext *ctx, LEPUSValueConst obj,
                       LEPUSClassID class_id);

typedef void LEPUSFreeArrayBufferDataFunc(LEPUSRuntime *rt, void *opaque,
                                          void *ptr);
LEPUSValue LEPUS_NewArrayBuffer(LEPUSContext *ctx, uint8_t *buf, size_t len,
                                LEPUSFreeArrayBufferDataFunc *free_func,
                                void *opaque, LEPUS_BOOL is_shared);
LEPUSValue LEPUS_NewArrayBufferCopy(LEPUSContext *ctx, const uint8_t *buf,
                                    size_t len);
void LEPUS_DetachArrayBuffer(LEPUSContext *ctx, LEPUSValueConst obj);
uint8_t *LEPUS_GetArrayBuffer(LEPUSContext *ctx, size_t *psize,
                              LEPUSValueConst obj);
LEPUSValue LEPUS_GetTypedArrayBuffer(LEPUSContext *ctx, LEPUSValueConst obj,
                                     size_t *pbyte_offset, size_t *pbyte_length,
                                     size_t *pbytes_per_element);

// <Primjs begin>
LEPUS_BOOL LEPUS_IsArrayBuffer(LEPUSValueConst v);
LEPUSTypedArrayType LEPUS_GetTypedArrayType(LEPUSContext *ctx,
                                            LEPUSValueConst val);
LEPUS_BOOL LEPUS_IsDataView(LEPUSContext *ctx, LEPUSValueConst val);
LEPUS_BOOL LEPUS_IsTypedArray(LEPUSContext *ctx, LEPUSValueConst val);

LEPUSClassID LEPUS_GetTypedArrayClassID(LEPUSTypedArrayType type);

// must be freed by lepus_free() / lepus_free_rt()
uint8_t *LEPUS_MoveArrayBuffer(LEPUSContext *, size_t *, LEPUSValue);
LEPUS_BOOL LEPUS_StrictEq(LEPUSContext *ctx, LEPUSValueConst op1,
                          LEPUSValueConst op2);
LEPUS_BOOL LEPUS_SameValue(LEPUSContext *ctx, LEPUSValueConst op1,
                           LEPUSValueConst op2);
// <Primjs end>

LEPUSValue LEPUS_NewPromiseCapability(LEPUSContext *ctx,
                                      LEPUSValue *resolving_funcs);

/* return != 0 if the LEPUS code needs to be interrupted */
typedef int LEPUSInterruptHandler(LEPUSRuntime *rt, void *opaque);
void LEPUS_SetInterruptHandler(LEPUSRuntime *rt, LEPUSInterruptHandler *cb,
                               void *opaque);
/* if can_block is TRUE, Atomics.wait() can be used */
void LEPUS_SetCanBlock(LEPUSRuntime *rt, LEPUS_BOOL can_block);

typedef struct LEPUSModuleDef LEPUSModuleDef;

/* return the module specifier (allocated with lepus_malloc()) or NULL if
   exception */
typedef char *LEPUSModuleNormalizeFunc(LEPUSContext *ctx,
                                       const char *module_base_name,
                                       const char *module_name, void *opaque);
typedef LEPUSModuleDef *LEPUSModuleLoaderFunc(LEPUSContext *ctx,
                                              const char *module_name,
                                              void *opaque);

/* module_normalize = NULL is allowed and invokes the default module
   filename normalizer */
void LEPUS_SetModuleLoaderFunc(LEPUSRuntime *rt,
                               LEPUSModuleNormalizeFunc *module_normalize,
                               LEPUSModuleLoaderFunc *module_loader,
                               void *opaque);

/* LEPUS Job support */

typedef LEPUSValue LEPUSJobFunc(LEPUSContext *ctx, int argc,
                                LEPUSValueConst *argv);
int LEPUS_EnqueueJob(LEPUSContext *ctx, LEPUSJobFunc *job_func, int argc,
                     LEPUSValueConst *argv);

LEPUS_BOOL LEPUS_IsJobPending(LEPUSRuntime *rt);
int LEPUS_ExecutePendingJob(LEPUSRuntime *rt, LEPUSContext **pctx);

uint64_t LEPUS_GetPrimjsVersion();

/* Object Writer/Reader (currently only used to handle precompiled code) */
#define LEPUS_WRITE_OBJ_BYTECODE (1 << 0) /* allow function/module */
#define LEPUS_WRITE_OBJ_BSWAP (1 << 1)    /* byte swapped output */
uint8_t *LEPUS_WriteObject(LEPUSContext *ctx, size_t *psize,
                           LEPUSValueConst obj, int flags);
#define LEPUS_READ_OBJ_BYTECODE (1 << 0) /* allow function/module */
#define LEPUS_READ_OBJ_ROM_DATA (1 << 1) /* avoid duplicating 'buf' data */
LEPUSValue LEPUS_ReadObject(LEPUSContext *ctx, const uint8_t *buf,
                            size_t buf_len, int flags);
LEPUSValue LEPUS_EvalFunction(LEPUSContext *ctx, LEPUSValue fun_obj,
                              LEPUSValueConst this_obj);

LEPUSValue LEPUS_NewWString(LEPUSContext *, const uint16_t *, size_t length);
LEPUSValue LEPUS_FromJSON(LEPUSContext *, const char *);
LEPUSValue LEPUS_ToJSON(LEPUSContext *, LEPUSValueConst, int indent);
LEPUSValue LEPUS_ToWString(LEPUSContext *, LEPUSValueConst);
JSAtom LEPUS_ValueToAtom(LEPUSContext *ctx, LEPUSValueConst val);

const uint16_t *LEPUS_GetStringChars(LEPUSContext *, LEPUSValueConst);
uint32_t LEPUS_GetStringLength(LEPUSContext *, LEPUSValueConst);
LEPUSClassID LEPUS_GetClassID(LEPUSContext *, LEPUSValueConst);
LEPUSValue LEPUS_NewArrayWithValue(LEPUSContext *, uint32_t length,
                                   LEPUSValueConst *values);
LEPUSValue LEPUS_NewTypedArray(LEPUSContext *, uint32_t length, LEPUSClassID);
LEPUSValue LEPUS_NewTypedArrayWithBuffer(LEPUSContext *, LEPUSValueConst buffer,
                                         uint32_t byteOffset, uint32_t length,
                                         LEPUSClassID class_id);
LEPUSValue LEPUS_CallV(LEPUSContext *ctx, LEPUSValueConst func_obj,
                       LEPUSValueConst this_obj, int argc, LEPUSValue *argv);
LEPUSValue LEPUS_CallConstructorV(LEPUSContext *ctx, LEPUSValueConst func_obj,
                                  int argc, LEPUSValue *argv);

/* C function definition */
typedef enum LEPUSCFunctionEnum { /* XXX: should rename for namespace isolation
                                   */
                                  LEPUS_CFUNC_generic,
                                  LEPUS_CFUNC_generic_magic,
                                  LEPUS_CFUNC_constructor,
                                  LEPUS_CFUNC_constructor_magic,
                                  LEPUS_CFUNC_constructor_or_func,
                                  LEPUS_CFUNC_constructor_or_func_magic,
                                  LEPUS_CFUNC_f_f,
                                  LEPUS_CFUNC_f_f_f,
                                  LEPUS_CFUNC_getter,
                                  LEPUS_CFUNC_setter,
                                  LEPUS_CFUNC_getter_magic,
                                  LEPUS_CFUNC_setter_magic,
                                  LEPUS_CFUNC_iterator_next,
} LEPUSCFunctionEnum;

typedef union LEPUSCFunctionType {
  LEPUSCFunction *generic;
  LEPUSValue (*generic_magic)(LEPUSContext *ctx, LEPUSValueConst this_val,
                              int argc, LEPUSValueConst *argv, int magic);
  LEPUSCFunction *constructor;
  LEPUSValue (*constructor_magic)(LEPUSContext *ctx, LEPUSValueConst new_target,
                                  int argc, LEPUSValueConst *argv, int magic);
  LEPUSCFunction *constructor_or_func;
  double (*f_f)(double);
  double (*f_f_f)(double, double);
  LEPUSValue (*getter)(LEPUSContext *ctx, LEPUSValueConst this_val);
  LEPUSValue (*setter)(LEPUSContext *ctx, LEPUSValueConst this_val,
                       LEPUSValueConst val);
  LEPUSValue (*getter_magic)(LEPUSContext *ctx, LEPUSValueConst this_val,
                             int magic);
  LEPUSValue (*setter_magic)(LEPUSContext *ctx, LEPUSValueConst this_val,
                             LEPUSValueConst val, int magic);
  LEPUSValue (*iterator_next)(LEPUSContext *ctx, LEPUSValueConst this_val,
                              int argc, LEPUSValueConst *argv, int *pdone,
                              int magic);
} LEPUSCFunctionType;

LEPUSValue LEPUS_NewCFunction2(LEPUSContext *ctx, LEPUSCFunction *func,
                               const char *name, int length,
                               LEPUSCFunctionEnum cproto, int magic);
LEPUSValue LEPUS_NewCFunctionData(LEPUSContext *ctx, LEPUSCFunctionData *func,
                                  int length, int magic, int data_len,
                                  LEPUSValueConst *data);

static inline LEPUSValue LEPUS_NewCFunction(LEPUSContext *ctx,
                                            LEPUSCFunction *func,
                                            const char *name, int length) {
  return LEPUS_NewCFunction2(ctx, func, name, length, LEPUS_CFUNC_generic, 0);
}

static inline LEPUSValue LEPUS_NewCFunctionMagic(LEPUSContext *ctx,
                                                 LEPUSCFunctionMagic *func,
                                                 const char *name, int length,
                                                 LEPUSCFunctionEnum cproto,
                                                 int magic) {
  return LEPUS_NewCFunction2(ctx, (LEPUSCFunction *)func, name, length, cproto,
                             magic);
}

/* C property definition */

typedef struct LEPUSCFunctionListEntry {
  const char *name;
  uint8_t prop_flags;
  uint8_t def_type;
  int16_t magic;
  union {
    struct {
      uint8_t length; /* XXX: should move outside union */
      uint8_t cproto; /* XXX: should move outside union */
      LEPUSCFunctionType cfunc;
    } func;
    struct {
      LEPUSCFunctionType get;
      LEPUSCFunctionType set;
    } getset;
    struct {
      const char *name;
      int base;
    } alias;
    struct {
      const struct LEPUSCFunctionListEntry *tab;
      int len;
    } prop_list;
    const char *str;
    int32_t i32;
    int64_t i64;
    double f64;
  } u;
} LEPUSCFunctionListEntry;

#define LEPUS_DEF_CFUNC 0
#define LEPUS_DEF_CGETSET 1
#define LEPUS_DEF_CGETSET_MAGIC 2
#define LEPUS_DEF_PROP_STRING 3
#define LEPUS_DEF_PROP_INT32 4
#define LEPUS_DEF_PROP_INT64 5
#define LEPUS_DEF_PROP_DOUBLE 6
#define LEPUS_DEF_PROP_UNDEFINED 7
#define LEPUS_DEF_OBJECT 8
#define LEPUS_DEF_ALIAS 9

#define LEPUS_CFUNC_DEF(name, length, func1)                                 \
  {                                                                          \
    name, LEPUS_PROP_WRITABLE | LEPUS_PROP_CONFIGURABLE, LEPUS_DEF_CFUNC, 0, \
        .u.func = {                                                          \
          length,                                                            \
          LEPUS_CFUNC_generic,                                               \
          {.generic = func1}                                                 \
        }                                                                    \
  }
#define LEPUS_CFUNC_MAGIC_DEF(name, length, func1, magic)                 \
  {                                                                       \
    name, LEPUS_PROP_WRITABLE | LEPUS_PROP_CONFIGURABLE, LEPUS_DEF_CFUNC, \
        magic, .u.func = {                                                \
          length,                                                         \
          LEPUS_CFUNC_generic_magic,                                      \
          {.generic_magic = func1}                                        \
        }                                                                 \
  }
#define LEPUS_CFUNC_SPECIAL_DEF(name, length, cproto, func1)                 \
  {                                                                          \
    name, LEPUS_PROP_WRITABLE | LEPUS_PROP_CONFIGURABLE, LEPUS_DEF_CFUNC, 0, \
        .u.func = {                                                          \
          length,                                                            \
          LEPUS_CFUNC_##cproto,                                              \
          {.cproto = func1}                                                  \
        }                                                                    \
  }
#define LEPUS_ITERATOR_NEXT_DEF(name, length, func1, magic)               \
  {                                                                       \
    name, LEPUS_PROP_WRITABLE | LEPUS_PROP_CONFIGURABLE, LEPUS_DEF_CFUNC, \
        magic, .u.func = {                                                \
          length,                                                         \
          LEPUS_CFUNC_iterator_next,                                      \
          {.iterator_next = func1}                                        \
        }                                                                 \
  }
#define LEPUS_CGETSET_DEF(name, fgetter, fsetter)                      \
  {                                                                    \
    name, LEPUS_PROP_CONFIGURABLE, LEPUS_DEF_CGETSET, 0,               \
        .u.getset.get.getter = fgetter, .u.getset.set.setter = fsetter \
  }
#define LEPUS_CGETSET_MAGIC_DEF(name, fgetter, fsetter, magic)     \
  {                                                                \
    name, LEPUS_PROP_CONFIGURABLE, LEPUS_DEF_CGETSET_MAGIC, magic, \
        .u.getset.get.getter_magic = fgetter,                      \
        .u.getset.set.setter_magic = fsetter                       \
  }
#define LEPUS_PROP_STRING_DEF(name, cstr, prop_flags) \
  { name, prop_flags, LEPUS_DEF_PROP_STRING, 0, .u.str = cstr }
#define LEPUS_PROP_INT32_DEF(name, val, prop_flags) \
  { name, prop_flags, LEPUS_DEF_PROP_INT32, 0, .u.i32 = val }
#define LEPUS_PROP_INT64_DEF(name, val, prop_flags) \
  { name, prop_flags, LEPUS_DEF_PROP_INT64, 0, .u.i64 = val }
#define LEPUS_PROP_DOUBLE_DEF(name, val, prop_flags) \
  { name, prop_flags, LEPUS_DEF_PROP_DOUBLE, 0, .u.f64 = val }
#define LEPUS_PROP_UNDEFINED_DEF(name, prop_flags) \
  { name, prop_flags, LEPUS_DEF_PROP_UNDEFINED, 0, .u.i32 = 0 }
#define LEPUS_OBJECT_DEF(name, tab, len, prop_flags)                   \
  {                                                                    \
    name, prop_flags, LEPUS_DEF_OBJECT, 0, .u.prop_list = { tab, len } \
  }
#define LEPUS_ALIAS_DEF(name, from)                                          \
  {                                                                          \
    name, LEPUS_PROP_WRITABLE | LEPUS_PROP_CONFIGURABLE, LEPUS_DEF_ALIAS, 0, \
        .u.alias = {                                                         \
          from,                                                              \
          -1                                                                 \
        }                                                                    \
  }
#define LEPUS_ALIAS_BASE_DEF(name, from, base)                               \
  {                                                                          \
    name, LEPUS_PROP_WRITABLE | LEPUS_PROP_CONFIGURABLE, LEPUS_DEF_ALIAS, 0, \
        .u.alias = {                                                         \
          from,                                                              \
          base                                                               \
        }                                                                    \
  }

void LEPUS_SetPropertyFunctionList(LEPUSContext *ctx, LEPUSValueConst obj,
                                   const LEPUSCFunctionListEntry *tab, int len);

/* C module definition */

typedef int LEPUSModuleInitFunc(LEPUSContext *ctx, LEPUSModuleDef *m);

LEPUSModuleDef *LEPUS_NewCModule(LEPUSContext *ctx, const char *name_str,
                                 LEPUSModuleInitFunc *func);
/* can only be called before the module is instantiated */
QJS_HIDE int LEPUS_AddModuleExport(LEPUSContext *ctx, LEPUSModuleDef *m,
                                   const char *name_str);
QJS_HIDE int LEPUS_AddModuleExportList(LEPUSContext *ctx, LEPUSModuleDef *m,
                                       const LEPUSCFunctionListEntry *tab,
                                       int len);
/* can only be called after the module is instantiated */
int LEPUS_SetModuleExport(LEPUSContext *ctx, LEPUSModuleDef *m,
                          const char *export_name, LEPUSValue val);
int LEPUS_SetModuleExportList(LEPUSContext *ctx, LEPUSModuleDef *m,
                              const LEPUSCFunctionListEntry *tab, int len);

/* expose for napi use */
LEPUSValue lepus_object_getOwnPropertyDescriptor(LEPUSContext *ctx,
                                                 LEPUSValueConst this_val,
                                                 int argc,
                                                 LEPUSValueConst *argv,
                                                 int magic);

// <Primjs begin>
QJS_EXPORT_FOR_DEVTOOL int64_t find_line_num(LEPUSContext *ctx,
                                             struct LEPUSFunctionBytecode *b,
                                             uint32_t pc_value);

QJS_EXPORT_FOR_DEVTOOL int lepus_class_has_bytecode(LEPUSClassID class_id);

QJS_EXPORT_FOR_DEVTOOL const char *get_func_name(LEPUSContext *ctx,
                                                 LEPUSValueConst func);

QJS_EXPORT_FOR_DEVTOOL int get_leb128_u64(uint64_t *pval, const uint8_t *buf,
                                          const uint8_t *buf_end);

QJS_EXPORT_FOR_DEVTOOL int get_sleb128_u64(int64_t *pval, const uint8_t *buf,
                                           const uint8_t *buf_end);
QJS_EXPORT_FOR_DEVTOOL __attribute__((warn_unused_result)) int
lepus_get_length32(LEPUSContext *ctx, uint32_t *pres, LEPUSValueConst obj);
void SetLynxTargetSdkVersion(LEPUSContext *ctx, const char *version);

/*
 register lepus::Value::ValueType's value in qjs runtime for lepusng,
 because these type value are used in qjs
*/
void LEPUS_RegisterNgType(LEPUSRuntime *, int32_t *, uint32_t size);

/*
when an object of type lepus::RefCounted is destructed,
   call this function to free all MapRecords
*/
void LEPUS_FreeRefCountedWeakRef(LEPUSRuntime *, struct JSMapRecord *);

LEPUS_BOOL LEPUS_IsPrimjsEnabled(LEPUSRuntime *rt);

/*
  This function doesn't change the rc of value;
*/
LEPUSValue LEPUS_NewObjectWithArgs(LEPUSContext *, int32_t size, const char **,
                                   LEPUSValue *);
/*
  This function doesn't change the rc of value;
*/

LEPUSValue LEPUS_NewArrayWithArgs(LEPUSContext *, int32_t, LEPUSValue *);

/*
  If input is an ASCII string, return pointer to string, otherwise
  return nullptr;
*/
const char *LEPUS_GetStringUtf8(LEPUSContext *, const struct JSString *);
void LEPUS_SetFuncFileName(LEPUSContext *, LEPUSValue, const char *);

void InitLynxTraceEnv(void *(*)(const char *), void (*)(void *));
// <Primjs end>

#undef lepus_unlikely
#undef lepus_force_inline

#endif  // SRC_INTERPRETER_QUICKJS_INCLUDE_QUICKJS_H_
