// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/jsi/quickjs/quickjs_helper.h"

#include <string>

#include "core/runtime/jsi/jsi.h"
#include "core/runtime/jsi/quickjs/quickjs_exception.h"
// #include "quickjs_runtime.h"
#include <fstream>
#include <iostream>
#include <string>
#include <utility>

#include "core/build/gen/lynx_sub_error_code.h"
#include "core/runtime/jsi/quickjs/quickjs_runtime.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "quickjs/include/quickjs-libc.h"
#include "quickjs/include/quickjs.h"
#ifdef __cplusplus
}
#endif
#ifdef OS_IOS
#include "gc/trace-gc.h"
#else
#include "quickjs/include/trace-gc.h"
#endif

namespace lynx {
namespace piper {
namespace detail {
QuickjsJSValueValue::QuickjsJSValueValue(LEPUSContext *ctx, LEPUSValue val)
    : val_(val), rt_(LEPUS_GetRuntime(ctx)) {
  //  static int calc = 0;
  //  time = calc;
  //  LOGE( "create ptr=" << JS_VALUE_GET_PTR(val_) << " create time="
  //  << calc); calc++;
  // DCHECK((uintptr_t)(&((QuickjsJSValueValue *)(0x0))->val_) == 8);
}

void QuickjsJSValueValue::invalidate() {
  //  LOGE( "invalidate ptr=" << LEPUS_VALUE_GET_PTR(val_) << "
  //  invalidate time=" << time);
  if (!LEPUS_IsGCModeRT(rt_)) LEPUS_FreeValueRT(rt_, val_);
  QJSValueValueAllocator::Delete(rt_, (void *)(this));
}

LEPUSValue QuickjsJSValueValue::Get() const { return val_; }

piper::Runtime::PointerValue *QuickjsHelper::makeStringValue(LEPUSContext *ctx,
                                                             LEPUSValue str) {
  void *ret = QJSValueValueAllocator::New(LEPUS_GetRuntime(ctx));
  return new (ret) QuickjsJSValueValue(ctx, str);
}

piper::Runtime::PointerValue *QuickjsHelper::makeObjectValue(LEPUSContext *ctx,
                                                             LEPUSValue obj) {
  void *ret = QJSValueValueAllocator::New(LEPUS_GetRuntime(ctx));
  return new (ret) QuickjsJSValueValue(ctx, obj);
}

piper::Runtime::PointerValue *QuickjsHelper::makeJSValueValue(LEPUSContext *ctx,
                                                              LEPUSValue obj) {
  void *ret = QJSValueValueAllocator::New(LEPUS_GetRuntime(ctx));
  return new (ret) QuickjsJSValueValue(ctx, obj);
}

piper::Object QuickjsHelper::createJSValue(LEPUSContext *ctx, LEPUSValue obj) {
  return Runtime::make<piper::Object>(makeJSValueValue(ctx, obj));
}

piper::PropNameID QuickjsHelper::createPropNameID(LEPUSContext *ctx,
                                                  LEPUSValue propName) {
  return Runtime::make<piper::PropNameID>(makeStringValue(ctx, propName));
}

piper::String QuickjsHelper::createString(LEPUSContext *ctx, LEPUSValue str) {
  return Runtime::make<piper::String>(makeStringValue(ctx, str));
}

piper::Symbol QuickjsHelper::createSymbol(LEPUSContext *ctx, LEPUSValue sym) {
  return Runtime::make<piper::Symbol>(makeJSValueValue(ctx, sym));
}

piper::Object QuickjsHelper::createObject(LEPUSContext *ctx, LEPUSValue obj) {
  return Runtime::make<piper::Object>(makeObjectValue(ctx, obj));
}

piper::Value QuickjsHelper::createValue(LEPUSValue value, QuickjsRuntime *rt) {
  if (LEPUS_IsInteger(value)) {
    return piper::Value(LEPUS_VALUE_GET_INT(value));
  } else if (LEPUS_IsNumber(value)) {
    return piper::Value(LEPUS_VALUE_GET_FLOAT64(value));
  } else if (LEPUS_IsBool(value)) {
    bool temp = static_cast<bool>(LEPUS_ToBool(rt->getJSContext(), value));
    return piper::Value(temp);
  } else if (LEPUS_IsNull(value)) {
    return piper::Value(nullptr);
  } else if (LEPUS_IsUndefined(value)) {
    return piper::Value();
  } else if (LEPUS_IsSymbol(value)) {
    return piper::Value(createSymbol(rt->getJSContext(), value));
  } else if (LEPUS_IsString(value)) {
    return piper::Value(createString(rt->getJSContext(), value));
  } else if (LEPUS_IsObject(value) || LEPUS_IsException(value)) {
    return piper::Value(createObject(rt->getJSContext(), value));
  } else {
    int64_t tag = LEPUS_VALUE_GET_TAG(value);
    LOGE("createValue failed type is unknown:" << tag);
    std::string msg =
        "createValue failed type is unknown:" + std::to_string(tag);
    rt->reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(msg));
    return piper::Value();
  }
}

LEPUSValue QuickjsHelper::symbolRef(const piper::Symbol &sym) {
  const QuickjsJSValueValue *quickjs_sym =
      static_cast<const QuickjsJSValueValue *>(Runtime::getPointerValue(sym));
  return quickjs_sym->Get();
}

LEPUSValue QuickjsHelper::valueRef(const piper::PropNameID &sym) {
  return static_cast<const QuickjsJSValueValue *>(Runtime::getPointerValue(sym))
      ->Get();
}

LEPUSValue QuickjsHelper::stringRef(const piper::String &sym) {
  return static_cast<const QuickjsJSValueValue *>(Runtime::getPointerValue(sym))
      ->Get();
}

LEPUSValue QuickjsHelper::objectRef(const piper::Object &sym) {
  return static_cast<const QuickjsJSValueValue *>(Runtime::getPointerValue(sym))
      ->Get();
}

std::string QuickjsHelper::LEPUSStringToSTLString(LEPUSContext *ctx,
                                                  LEPUSValue s) {
  const char *c = LEPUS_ToCString(ctx, s);
  if (c == nullptr) {
    if (LEPUS_IsGCMode(ctx))
      LEPUS_GetException(ctx);  // just disconnect the reference-relation from
                                // ctx to exception_val
    else
      LEPUS_FreeValue(ctx, LEPUS_GetException(ctx));

    return "Error!";
  }
  std::string ret(c);
  if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeCString(ctx, c);
  return ret;
}

std::optional<piper::Value> QuickjsHelper::call(QuickjsRuntime *rt,
                                                const piper::Function &f,
                                                const piper::Object &jsThis,
                                                LEPUSValue *arguments,
                                                size_t nArgs) {
  LEPUSValue thisObj = QuickjsHelper::objectRef(jsThis);
  LEPUSValue target_object = LEPUS_IsUninitialized(thisObj)
                                 ? LEPUS_GetGlobalObject(rt->getJSContext())
                                 : thisObj;
  LEPUSValue func = objectRef(f);
  LEPUSValue result = LEPUS_Call(rt->getJSContext(), func, target_object,
                                 static_cast<int>(nArgs), arguments);

  if (!LEPUS_IsGCMode(rt->getJSContext()) && LEPUS_IsUninitialized(thisObj)) {
    LEPUS_FreeValue(rt->getJSContext(), target_object);
  }

  bool has_exception = !QuickjsException::ReportExceptionIfNeeded(*rt, result);
  lepus_std_loop(rt->getJSContext());
  while (LEPUS_MoveUnhandledRejectionToException((rt->getJSContext()))) {
    LEPUSValue exception_val = LEPUS_GetException(rt->getJSContext());
    rt->reportJSIException(QuickjsException(*rt, exception_val));
  }
  // Before, If function inside quickjs triggered an exception, it return an
  // object as `Exception` type. This type is invisible to jsi, thus cannot be
  // identified. Now, return `undefined` here, the same result as V8.
  if (has_exception) {
    return std::optional<piper::Value>();
  }
  return createValue(result, rt);
}

std::optional<piper::Value> QuickjsHelper::callAsConstructor(QuickjsRuntime *rt,
                                                             LEPUSValue obj,
                                                             LEPUSValue *args,
                                                             int nArgs) {
  LEPUSValue result =
      LEPUS_CallConstructor(rt->getJSContext(), obj, nArgs, args);
  auto result_holder = createValue(result, rt);

  bool has_exception = !QuickjsException::ReportExceptionIfNeeded(*rt, result);
  lepus_std_loop(rt->getJSContext());
  while (LEPUS_MoveUnhandledRejectionToException(rt->getJSContext())) {
    LEPUSValue exception_val = LEPUS_GetException(rt->getJSContext());
    rt->reportJSIException(QuickjsException(*rt, exception_val));
  }
  // Same raison as `QuickjsHelper::call`
  if (has_exception) {
    return std::optional<piper::Value>();
  }
  return result_holder;
}

std::string QuickjsHelper::getErrorMessage(LEPUSContext *ctx,
                                           LEPUSValue &exception_value) {
  std::string error_msg;
  // Even if most of the caller make a check to exception_value before calling
  // getErrorMessage, here we double check if exception_value is an exception or
  // an error to make sure we will not get a message like: [object Object].
  if (LEPUS_IsException(exception_value) ||
      LEPUS_IsError(ctx, exception_value)) {
    auto str = LEPUS_ToCString(ctx, exception_value);
    if (str) {
      error_msg.append(str);
    }
    if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeCString(ctx, str);
  }
  return error_msg;
}

base::expected<Value, JSINativeException> QuickjsHelper::evalBuf(
    QuickjsRuntime *rt, LEPUSContext *ctx, const char *buf, size_t buf_len,
    const char *filename, int eval_flags) {
  LEPUSValue val = LEPUS_Eval(ctx, buf, buf_len, filename, eval_flags);
  auto maybe_error = QuickjsException::TryCatch(*rt, val);
  if (maybe_error.has_value()) {
    LOGE("evalBuf failed:" << filename);
    return base::unexpected(JSINativeException(
        maybe_error->name(), maybe_error->message(), maybe_error->stack(), true,
        error::E_BTS_RUNTIME_ERROR_SCRIPT_ERROR));
  }
  piper::Value evalRes = createValue(val, rt);
  // createValue did not add ref count to val;
  // so don't use LEPUS_FreeValue
  //   LEPUS_FreeValue(ctx, val);
  lepus_std_loop(ctx);
  return evalRes;
}

base::expected<Value, JSINativeException> QuickjsHelper::evalBin(
    QuickjsRuntime *rt, LEPUSContext *ctx, const char *buf, size_t buf_len,
    const char *filename, int eval_flags) {
  LEPUSValue val = LEPUS_EvalBinary(ctx, reinterpret_cast<const uint8_t *>(buf),
                                    buf_len, eval_flags);
  auto maybe_error = QuickjsException::TryCatch(*rt, val);
  if (maybe_error.has_value()) {
    LOGE("evalBin failed:" << filename);
    return base::unexpected(JSINativeException(
        maybe_error->name(), maybe_error->message(), maybe_error->stack(), true,
        error::E_BTS_RUNTIME_ERROR_BYTECODE_SCRIPT_ERROR));
  }
  piper::Value evalRes = createValue(val, rt);
  // createValue did not add ref count to val;
  // so don't use LEPUS_FreeValue
  // LEPUS_FreeValue(ctx, val);
  lepus_std_loop(ctx);
  return evalRes;
}

LEPUSValue QuickjsHelper::ThrowJsException(
    LEPUSContext *ctx, const JSINativeException &exception) {
  DCHECK(!exception.message().empty());
  auto err = LEPUS_NewError(ctx);
  HandleScope func_scope(ctx, &err, HANDLE_TYPE_LEPUS_VALUE);
  LEPUSValue error_value = LEPUS_NewStringLen(
      ctx, exception.message().empty() ? "" : exception.message().c_str(),
      exception.message().length());
  func_scope.PushHandle(&error_value, HANDLE_TYPE_LEPUS_VALUE);
  LEPUSValue stack_value = LEPUS_NewStringLen(
      ctx, exception.stack().empty() ? "" : exception.stack().c_str(),
      exception.stack().length());
  func_scope.PushHandle(&stack_value, HANDLE_TYPE_LEPUS_VALUE);
  if (LEPUS_IsError(ctx, err)) {
    LEPUS_DefinePropertyValueStr(ctx, err, "message", error_value,
                                 LEPUS_PROP_CONFIGURABLE | LEPUS_PROP_WRITABLE);
    if (exception.IsJSError()) {
      LEPUS_DefinePropertyValueStr(
          ctx, err, "stack", stack_value,
          LEPUS_PROP_CONFIGURABLE | LEPUS_PROP_WRITABLE);
      LEPUSValue name_value = LEPUS_NewStringLen(ctx, exception.name(),
                                                 std::strlen(exception.name()));
      func_scope.PushHandle(&name_value, HANDLE_TYPE_LEPUS_VALUE);
      LEPUS_DefinePropertyValueStr(
          ctx, err, "name", name_value,
          LEPUS_PROP_CONFIGURABLE | LEPUS_PROP_WRITABLE);
    } else {
      LEPUS_DefinePropertyValueStr(
          ctx, err, "cause", stack_value,
          LEPUS_PROP_CONFIGURABLE | LEPUS_PROP_WRITABLE);
    }
  }
  return LEPUS_Throw(ctx, err);
}

}  // namespace detail
}  // namespace piper
}  // namespace lynx
