// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JS_JSI_JSVM_JSVM_RUNTIME_H_
#define CORE_RUNTIME_JS_JSI_JSVM_JSVM_RUNTIME_H_

#include <ark_runtime/jsvm_types.h>

#include <memory>
#include <string>

#include "core/runtime/js/jsi/jsi.h"
#include "core/runtime/js/jsi/jsvm/jsvm_context_wrapper.h"
#include "core/runtime/js/jsi/jsvm/jsvm_runtime_wrapper.h"

namespace lynx {
namespace piper {

class JSVMRuntime : public Runtime {
 public:
  JSVMRuntime() = default;
  ~JSVMRuntime() override;

  JSRuntimeType type() override { return JSRuntimeType::jsvm; };
  void InitRuntime(std::shared_ptr<JSIContext> sharedContext,
                   std::shared_ptr<JSIExceptionHandler> handler) override;
  std::shared_ptr<VMInstance> createVM(const StartupData*) const override;
  std::shared_ptr<VMInstance> getSharedVM() override;
  std::shared_ptr<JSIContext> createContext(
      std::shared_ptr<VMInstance>) const override;
  std::shared_ptr<JSIContext> getSharedContext() override;

  std::shared_ptr<const PreparedJavaScript> prepareJavaScript(
      const std::shared_ptr<const Buffer>& buffer, std::string source_url,
      int start_line_offset = 0) override;

  base::expected<Value, JSINativeException> evaluatePreparedJavaScript(
      const std::shared_ptr<const PreparedJavaScript>& js) override;

  base::expected<Value, JSINativeException> evaluateJavaScript(
      const std::shared_ptr<const Buffer>& buffer,
      const std::string& source_url, int start_line_offset = 0) override;

  base::expected<Value, JSINativeException> evaluateJavaScriptBytecode(
      const std::shared_ptr<const Buffer>& buffer,
      const std::string& source_url) override;
  Object global() override;

  std::string description() override { return description_; };

  bool isInspectable() override { return true; };

  void setDescription(const std::string& desc) { description_ = desc; };

  JSVM_Env getEnv() const;

  void valueRef(const piper::Value& value, JSVM_Value* result);

  JSVM_Ref GetHostObjectTemplate() { return host_object_template_; };
  void SetHostObjectTemplate(JSVM_Ref object_template) {
    host_object_template_ = object_template;
  };
  JSVM_Ref GetHostFunctionTemplate() { return host_function_template_; };
  void SetHostFunctionTemplate(JSVM_Ref function_template) {
    host_function_template_ = function_template;
  };

 protected:
  PointerValue* cloneSymbol(const Runtime::PointerValue* pv) override;
  PointerValue* cloneString(const Runtime::PointerValue* pv) override;
  PointerValue* cloneObject(const Runtime::PointerValue* pv) override;
  PointerValue* clonePropNameID(const Runtime::PointerValue* pv) override;

  piper::PropNameID createPropNameIDFromAscii(const char* str,
                                              size_t length) override;
  piper::PropNameID createPropNameIDFromUtf8(const uint8_t* utf8,
                                             size_t length) override;
  piper::PropNameID createPropNameIDFromString(
      const piper::String& str) override;
  std::string utf8(const piper::PropNameID&) override;
  bool compare(const piper::PropNameID&, const piper::PropNameID&) override;

  std::optional<std::string> symbolToString(const piper::Symbol&) override;

  piper::String createStringFromAscii(const char* str, size_t length) override;
  piper::String createStringFromUtf8(const uint8_t* utf8,
                                     size_t length) override;
  std::string utf8(const piper::String&) override;

  piper::Object createObject() override;
  piper::Object createObject(std::shared_ptr<piper::HostObject> ho) override;
  std::weak_ptr<piper::HostObject> getHostObject(const piper::Object&) override;

  piper::HostFunctionType f = [](Runtime& rt, const piper::Value& thisVal,
                                 const piper::Value* args, size_t count) {
    return piper::Value::undefined();
  };
  piper::HostFunctionType& getHostFunction(const piper::Function&) override {
    return f;
  }

  std::optional<Value> getProperty(const piper::Object&,
                                   const piper::String& name) override;
  std::optional<Value> getProperty(const piper::Object&,
                                   const piper::PropNameID& name) override;
  bool hasProperty(const piper::Object&, const piper::String& name) override;
  bool hasProperty(const piper::Object&,
                   const piper::PropNameID& name) override;
  bool setPropertyValue(piper::Object&, const piper::String& name,
                        const piper::Value& value) override;
  bool setPropertyValue(piper::Object&, const piper::PropNameID& name,
                        const piper::Value& value) override;
  bool isArray(const piper::Object&) const override;
  bool isArrayBuffer(const piper::Object&) const override;
  bool isFunction(const piper::Object&) const override;
  bool isHostObject(const piper::Object&) const override;
  bool isHostFunction(const piper::Function&) const override;
  std::optional<Array> getPropertyNames(const piper::Object&) override;

  std::optional<BigInt> createBigInt(const std::string& value,
                                     Runtime& rt) override;

  std::optional<Array> createArray(size_t length) override;

  piper::ArrayBuffer createArrayBufferCopy(const uint8_t* bytes,
                                           size_t byte_length) override;

  piper::ArrayBuffer createArrayBufferNoCopy(
      std::unique_ptr<const uint8_t[]> bytes, size_t byte_length) override;
  std::optional<size_t> size(const piper::Array&) override;
  size_t size(const piper::ArrayBuffer&) override;
  uint8_t* data(const piper::ArrayBuffer&) override;
  size_t copyData(const ArrayBuffer&, uint8_t*, size_t) override;
  std::optional<Value> getValueAtIndex(const piper::Array&, size_t i) override;
  bool setValueAtIndexImpl(piper::Array&, size_t i,
                           const piper::Value& value) override;

  piper::Function createFunctionFromHostFunction(
      const piper::PropNameID& name, unsigned int paramCount,
      piper::HostFunctionType func) override;
  std::optional<Value> call(const piper::Function&, const piper::Value& jsThis,
                            const piper::Value* args, size_t count) override;
  std::optional<Value> callAsConstructor(const piper::Function&,
                                         const piper::Value* args,
                                         size_t count) override;
  ScopeState* pushScope() override;
  void popScope(ScopeState*) override;
  bool strictEquals(const piper::Symbol& a,
                    const piper::Symbol& b) const override;
  bool strictEquals(const piper::String& a,
                    const piper::String& b) const override;
  bool strictEquals(const piper::Object& a,
                    const piper::Object& b) const override;
  bool instanceOf(const piper::Object& o, const piper::Function& f) override;

  void RequestGC() override;

  void InitInspector(
      const std::shared_ptr<InspectorRuntimeObserverNG>& observer) override;
  void DestroyInspector() override;

 private:
  std::shared_ptr<JSVMRuntimeInstance> runtime_wrapper_;
  std::shared_ptr<JSVMContextWrapper> context_;
  std::string description_;
  JSVM_Ref host_object_template_ = nullptr;
  JSVM_Ref host_function_template_ = nullptr;
};
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JS_JSI_JSVM_JSVM_RUNTIME_H_
