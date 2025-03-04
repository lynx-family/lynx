//  Copyright (c) Facebook, Inc. and its affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSI_JSC_JSC_HELPER_H_
#define CORE_RUNTIME_JSI_JSC_JSC_HELPER_H_

#include <JavaScriptCore/JavaScript.h>

#include <memory>
#include <string>

#include "base/include/string/string_utils.h"
#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {
class JSCRuntime;
namespace detail {

class JSCSymbolValue final : public Runtime::PointerValue {
 public:
  JSCSymbolValue(JSGlobalContextRef ctx, const std::atomic<bool>& ctxInvalid,
                 std::atomic<intptr_t>& counter, JSValueRef sym);
  void invalidate() override;
  virtual std::string Name() override { return "JSCSymbolValue"; }
  JSGlobalContextRef ctx_;
  const std::atomic<bool>& ctxInvalid_;
  JSValueRef sym_;
#ifdef DEBUG
  std::atomic<intptr_t>& counter_;
#endif
 protected:
  friend class JSCRuntime;
  friend class JSCHelper;
};

class JSCStringValue final : public Runtime::PointerValue {
 public:
  JSCStringValue(std::atomic<intptr_t>& counter, JSStringRef str);
  void invalidate() override;
  virtual std::string Name() override { return "JSCStringValue"; }
  JSStringRef str_;
#ifdef DEBUG
  std::atomic<intptr_t>& counter_;
#endif
 protected:
  friend class JSCRuntime;
  friend class JSCHelper;
};

class JSCObjectValue final : public Runtime::PointerValue {
 public:
  JSCObjectValue(JSGlobalContextRef ctx, const std::atomic<bool>& ctxInvalid,
                 std::atomic<intptr_t>& counter, JSObjectRef obj);
  void invalidate() override;
  virtual std::string Name() override { return "JSCObjectValue"; }
  JSObjectRef Get() const;
  JSGlobalContextRef ctx_;
  const std::atomic<bool>& ctxInvalid_;
  JSObjectRef obj_;
#ifdef DEBUG
  std::atomic<intptr_t>& counter_;
#endif
 protected:
  friend class JSCRuntime;
  friend class JSCHelper;
};

class JSCHelper {
 public:
  static Runtime::PointerValue* makeSymbolValue(
      JSGlobalContextRef, const std::atomic<bool>& ctxInvalid,
      std::atomic<intptr_t>& counter, JSValueRef);
  static Runtime::PointerValue* makeStringValue(std::atomic<intptr_t>& counter,
                                                JSStringRef);
  static Runtime::PointerValue* makeObjectValue(
      JSGlobalContextRef, const std::atomic<bool>& ctxInvalid,
      std::atomic<intptr_t>& counter, JSObjectRef);

  static Value createValue(JSCRuntime&, JSValueRef);
  static Symbol createSymbol(JSGlobalContextRef, const std::atomic<bool>&,
                             std::atomic<intptr_t>& counter, JSValueRef);
  static piper::String createString(std::atomic<intptr_t>& counter,
                                    JSStringRef);
  static PropNameID createPropNameID(std::atomic<intptr_t>& counter,
                                     JSStringRef);
  static Object createObject(JSGlobalContextRef,
                             const std::atomic<bool>& ctxInvalid,
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
#endif  // CORE_RUNTIME_JSI_JSC_JSC_HELPER_H_
