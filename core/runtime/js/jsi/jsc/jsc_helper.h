//  Copyright (c) Facebook, Inc. and its affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_JSI_JSC_JSC_HELPER_H_
#define CORE_RUNTIME_JS_JSI_JSC_JSC_HELPER_H_

#include <JavaScriptCore/JavaScript.h>

#include <memory>
#include <string>

#include "base/include/string/string_utils.h"
#include "core/base/observer/observer.h"
#include "core/runtime/js/jsi/jsi.h"

namespace lynx {
namespace piper {
class JSCRuntime;
namespace detail {

template <typename T, typename RuntimeType>
class JSCObjectBase : public Runtime::PointerValue, public base::Observer {
 public:
  JSCObjectBase(JSGlobalContextRef ctx, RuntimeType* jsc_runtime, T sym,
                std::atomic<intptr_t>& counter)
      : ctx_(ctx),
        sym_(sym),
        jsc_runtime_(jsc_runtime)
#if defined(DEBUG) || (defined(LYNX_UNIT_TEST) && LYNX_UNIT_TEST)
        ,
        counter_(counter)
#endif
  {
    if (ctx_) {
      JSValueProtect(ctx_, sym_);
    }
#if defined(DEBUG) || (defined(LYNX_UNIT_TEST) && LYNX_UNIT_TEST)
    counter_ += 1;
#endif
    key_ = jsc_runtime_->AddObjectObserver(this);
  }
  BASE_DISALLOW_COPY_AND_ASSIGN(JSCObjectBase);

  ~JSCObjectBase() {
    if (jsc_runtime_) {
      jsc_runtime_->RemoveObjectObserver(key_);
      jsc_runtime_ = nullptr;
    }
  }

  // delete by jsc_runtime destroy
  void Update() {
    if (ctx_) {
      JSValueUnprotect(ctx_, sym_);
      ctx_ = nullptr;
      jsc_runtime_ = nullptr;
    }
  }

  // delete by user code
  void invalidate() {
#if defined(DEBUG) || (defined(LYNX_UNIT_TEST) && LYNX_UNIT_TEST)
    counter_ -= 1;
#endif
    if (ctx_) {
      JSValueUnprotect(ctx_, sym_);
      ctx_ = nullptr;
    }
    delete this;
  }

  T Get() const { return ctx_ == nullptr ? nullptr : sym_; }

 protected:
  JSGlobalContextRef ctx_;
  T sym_;
  uint64_t key_;
  friend class JSCRuntime;
  friend class JSCHelper;

 private:
  RuntimeType* jsc_runtime_;
#if defined(DEBUG) || (defined(LYNX_UNIT_TEST) && LYNX_UNIT_TEST)
  std::atomic<intptr_t>& counter_;
#endif
};

class JSCSymbolValue final : public JSCObjectBase<JSValueRef, JSCRuntime> {
 public:
  JSCSymbolValue(JSGlobalContextRef ctx, JSCRuntime* jsc_runtime,
                 std::atomic<intptr_t>& counter, JSValueRef sym);
  BASE_DISALLOW_COPY_AND_ASSIGN(JSCSymbolValue);
  virtual std::string Name() override { return "JSCSymbolValue"; }
};

class JSCStringValue final : public Runtime::PointerValue {
 public:
  JSCStringValue(std::atomic<intptr_t>& counter, JSStringRef str);
  BASE_DISALLOW_COPY_AND_ASSIGN(JSCStringValue);
  void invalidate() override;
  virtual std::string Name() override { return "JSCStringValue"; }
  JSStringRef str_;
#if defined(DEBUG) || (defined(LYNX_UNIT_TEST) && LYNX_UNIT_TEST)
  std::atomic<intptr_t>& counter_;
#endif
};

class JSCObjectValue final : public JSCObjectBase<JSObjectRef, JSCRuntime> {
 public:
  JSCObjectValue(JSGlobalContextRef ctx, JSCRuntime* jsc_runtime,
                 std::atomic<intptr_t>& counter, JSObjectRef obj);
  BASE_DISALLOW_COPY_AND_ASSIGN(JSCObjectValue);
  virtual std::string Name() override { return "JSCObjectValue"; }
};

class JSCHelper {
 public:
  static Runtime::PointerValue* makeSymbolValue(JSGlobalContextRef,
                                                JSCRuntime* jsc_runtime,
                                                std::atomic<intptr_t>& counter,
                                                JSValueRef);
  static Runtime::PointerValue* makeStringValue(std::atomic<intptr_t>& counter,
                                                JSStringRef);
  static Runtime::PointerValue* makeObjectValue(JSGlobalContextRef,
                                                JSCRuntime* jsc_runtime,
                                                std::atomic<intptr_t>& counter,
                                                JSObjectRef);

  static Value createValue(JSCRuntime&, JSValueRef);
  static Symbol createSymbol(JSGlobalContextRef, JSCRuntime* jsc_runtime,
                             std::atomic<intptr_t>& counter, JSValueRef);
  static piper::String createString(std::atomic<intptr_t>& counter,
                                    JSStringRef);
  static PropNameID createPropNameID(std::atomic<intptr_t>& counter,
                                     JSStringRef);
  static Object createObject(JSGlobalContextRef, JSCRuntime* jsc_runtime,
                             std::atomic<intptr_t>& counter, JSObjectRef);

  static JSObjectRef createArrayBufferFromJS(JSCRuntime&, JSGlobalContextRef,
                                             const uint8_t*, size_t);

  static JSValueRef valueRef(JSGlobalContextRef, JSCRuntime&, const Value&);
  static JSValueRef symbolRef(const Symbol&);
  static JSStringRef stringRef(const piper::String&);
  static JSStringRef stringRef(const PropNameID&);
  static JSObjectRef objectRef(const Object&);

  static std::string JSStringToSTLString(JSStringRef);
  static JSStringRef getJSStringFromPool(std::string);
  static JSObjectRef getJSObject(JSCRuntime&, JSGlobalContextRef, JSObjectRef,
                                 JSStringRef);

  static bool smellsLikeES6Symbol(JSGlobalContextRef, JSValueRef);

  static std::optional<Value> call(JSGlobalContextRef ctx, JSCRuntime& rt,
                                   const Function& f, const Value& jsThis,
                                   const Value* args, size_t nArgs);

  static std::optional<Value> callAsConstructor(JSGlobalContextRef ctx,
                                                JSCRuntime& rt,
                                                const Function& f,
                                                const Value* args,
                                                size_t nArgs);
  static void ThrowJsException(JSContextRef ctx,
                               const JSINativeException& jsi_exception,
                               JSValueRef* exception);
};
}  // namespace detail
}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_JS_JSI_JSC_JSC_HELPER_H_
