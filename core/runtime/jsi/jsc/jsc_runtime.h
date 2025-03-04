//  Copyright (c) Facebook, Inc. and its affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSI_JSC_JSC_RUNTIME_H_
#define CORE_RUNTIME_JSI_JSC_JSC_RUNTIME_H_

#include <JavaScriptCore/JavaScript.h>

#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "core/runtime/jsi/jsc/jsc_context_group_wrapper.h"
#include "core/runtime/jsi/jsc/jsc_context_wrapper.h"
#include "core/runtime/jsi/jsc/jsc_helper.h"
#include "core/runtime/jsi/jsi.h"
#include "core/runtime/jsi/jslib.h"

namespace lynx {
namespace piper {

namespace detail {
struct JSCHostObjectProxy;
class JSCHostFunctionProxy;
}  // namespace detail

class JSCRuntime : public Runtime {
 public:
  JSCRuntime();
  ~JSCRuntime();
  JSRuntimeType type() override { return JSRuntimeType::jsc; }

  void InitRuntime(std::shared_ptr<JSIContext> sharedContext,
                   std::shared_ptr<JSIExceptionHandler> handler) override;
  std::shared_ptr<VMInstance> createVM(const StartupData*) const override;
  std::shared_ptr<VMInstance> getSharedVM() override;
  std::shared_ptr<JSIContext> createContext(
      std::shared_ptr<VMInstance> vm) const override;

  std::shared_ptr<JSIContext> getSharedContext() override;

  std::shared_ptr<const PreparedJavaScript> prepareJavaScript(
      const std::shared_ptr<const Buffer>& buffer,
      std::string sourceURL) override;

  base::expected<Value, JSINativeException> evaluatePreparedJavaScript(
      const std::shared_ptr<const PreparedJavaScript>& js) override;

  base::expected<Value, JSINativeException> evaluateJavaScript(
      const std::shared_ptr<const Buffer>& buffer,
      const std::string& sourceURL) override;

  base::expected<Value, JSINativeException> evaluateJavaScriptBytecode(
      const std::shared_ptr<const Buffer>& buffer,
      const std::string& source_url) override {
    LOGE("evaluateJavaScriptBytecode not supported in jsc");
    return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
        "evaluateJavaScriptBytecode not supported in jsc"));
  }

  Object global() override;

  std::string description() override;

  bool isInspectable() override { return false; };

  JSGlobalContextRef getContext() const { return ctx_->getContext(); }

  void setDescription(const std::string& desc) { description_ = desc; }

  const std::atomic<bool>& getCtxInvalid() {
    return ctx_ == nullptr ? temp_ctx_invalid_ : ctx_->contextInvalid();
  }

  std::atomic<intptr_t>& objectCounter() {
    return ctx_ == nullptr ? temp_obj_counter_ : ctx_->objectCounter();
  }
  bool Valid() const override;

 protected:
  PointerValue* cloneSymbol(const PointerValue* pv) override;

  PointerValue* cloneString(const PointerValue* pv) override;

  PointerValue* cloneObject(const PointerValue* pv) override;

  PointerValue* clonePropNameID(const PointerValue* pv) override;

  PropNameID createPropNameIDFromAscii(const char* str, size_t length) override;

  PropNameID createPropNameIDFromUtf8(const uint8_t* utf8,
                                      size_t length) override;

  PropNameID createPropNameIDFromString(const piper::String& str) override;

  std::string utf8(const PropNameID&) override;

  bool compare(const PropNameID&, const PropNameID&) override;

  std::optional<std::string> symbolToString(const Symbol&) override;

  piper::String createStringFromAscii(const char* str, size_t length) override;

  piper::String createStringFromUtf8(const uint8_t* utf8,
                                     size_t length) override;

  std::string utf8(const piper::String&) override;

  Object createObject() override;

  Object createObject(std::shared_ptr<HostObject> ho) override;

  virtual std::weak_ptr<HostObject> getHostObject(const Object&) override;

  virtual HostFunctionType& getHostFunction(const Function&) override;

  std::optional<Value> getProperty(const Object&,
                                   const piper::String& name) override;

  std::optional<Value> getProperty(const Object&,
                                   const PropNameID& name) override;

  bool hasProperty(const Object&, const piper::String& name) override;

  bool hasProperty(const Object&, const PropNameID& name) override;

  bool setPropertyValue(Object&, const piper::String& name,
                        const Value& value) override;

  bool setPropertyValue(Object&, const PropNameID& name,
                        const Value& value) override;

  bool isArray(const Object&) const override;

  bool isArrayBuffer(const Object&) const override;

  bool isFunction(const Object&) const override;

  bool isHostObject(const Object&) const override;

  bool isHostFunction(const Function&) const override;

  std::optional<Array> getPropertyNames(const Object&) override;

  std::optional<Array> createArray(size_t length) override;

  piper::ArrayBuffer createArrayBufferCopy(const uint8_t* bytes,
                                           size_t byte_length) override;

  piper::ArrayBuffer createArrayBufferNoCopy(
      std::unique_ptr<const uint8_t[]> bytes, size_t byte_length) override;

  std::optional<BigInt> createBigInt(const std::string& value,
                                     Runtime& rt) override;

  std::optional<size_t> size(const Array&) override;

  size_t size(const ArrayBuffer&) override;

  uint8_t* data(const ArrayBuffer&) override;

  size_t copyData(const ArrayBuffer&, uint8_t*, size_t) override;

  std::optional<Value> getValueAtIndex(const Array&, size_t i) override;

  bool setValueAtIndexImpl(Array&, size_t i, const Value& value) override;

  Function createFunctionFromHostFunction(const PropNameID& name,
                                          unsigned int paramCount,
                                          HostFunctionType func) override;

  std::optional<Value> call(const Function& f, const Value& jsThis,
                            const Value* args, size_t count) override;

  std::optional<Value> callAsConstructor(const Function& f, const Value* args,
                                         size_t count) override;

  //   ScopeState* pushScope() override;

  //   void popScope(ScopeState*) override;

  bool strictEquals(const Symbol&, const Symbol&) const override;

  bool strictEquals(const piper::String&, const piper::String&) const override;

  bool strictEquals(const Object&, const Object&) const override;

  bool instanceOf(const Object&, const Function&) override;

  void RequestGC() override;

  void RequestGCForTesting() override;

 private:
  ArrayBuffer createEmptyArrayBuffer();
  std::shared_ptr<JSCContextGroupWrapper> ctx_group_;
  std::shared_ptr<JSCContextWrapper> ctx_;
  std::string description_;
  std::atomic<bool> temp_ctx_invalid_;
  std::atomic<intptr_t> temp_obj_counter_;
};

}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_JSI_JSC_JSC_RUNTIME_H_
