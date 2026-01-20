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
namespace runtime {
namespace js {
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

  void valueRef(const Value& value, JSVM_Value* result);

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

  PropNameID createPropNameIDFromAscii(const char* str, size_t length) override;
  PropNameID createPropNameIDFromUtf8(const uint8_t* utf8,
                                      size_t length) override;
  PropNameID createPropNameIDFromString(const String& str) override;
  std::string utf8(const PropNameID&) override;
  bool compare(const PropNameID&, const PropNameID&) override;

  std::optional<std::string> symbolToString(const Symbol&) override;

  String createStringFromAscii(const char* str, size_t length) override;
  String createStringFromUtf8(const uint8_t* utf8, size_t length) override;
  std::string utf8(const String&) override;

  Object createObject() override;
  Object createObject(std::shared_ptr<HostObject> ho) override;
  std::weak_ptr<HostObject> getHostObject(const Object&) override;

  HostFunctionType f = [](Runtime& rt, const Value& thisVal, const Value* args,
                          size_t count) { return Value::undefined(); };
  HostFunctionType& getHostFunction(const Function&) override { return f; }

  std::optional<Value> getProperty(const Object&, const String& name) override;
  std::optional<Value> getProperty(const Object&,
                                   const PropNameID& name) override;
  bool hasProperty(const Object&, const String& name) override;
  bool hasProperty(const Object&, const PropNameID& name) override;
  bool setPropertyValue(Object&, const String& name,
                        const Value& value) override;
  bool setPropertyValue(Object&, const PropNameID& name,
                        const Value& value) override;
  bool isArray(const Object&) const override;
  bool isArrayBuffer(const Object&) const override;
  bool isFunction(const Object&) const override;
  bool isHostObject(const Object&) const override;
  bool isHostFunction(const Function&) const override;
  std::optional<Array> getPropertyNames(const Object&) override;

  std::optional<BigInt> createBigInt(const std::string& value,
                                     Runtime& rt) override;

  std::optional<Array> createArray(size_t length) override;

  ArrayBuffer createArrayBufferCopy(const uint8_t* bytes,
                                    size_t byte_length) override;

  ArrayBuffer createArrayBufferNoCopy(std::unique_ptr<const uint8_t[]> bytes,
                                      size_t byte_length) override;
  std::optional<size_t> size(const Array&) override;
  size_t size(const ArrayBuffer&) override;
  uint8_t* data(const ArrayBuffer&) override;
  size_t copyData(const ArrayBuffer&, uint8_t*, size_t) override;
  std::optional<Value> getValueAtIndex(const Array&, size_t i) override;
  bool setValueAtIndexImpl(Array&, size_t i, const Value& value) override;

  Function createFunctionFromHostFunction(const PropNameID& name,
                                          unsigned int paramCount,
                                          HostFunctionType func) override;
  std::optional<Value> call(const Function&, const Value& jsThis,
                            const Value* args, size_t count) override;
  std::optional<Value> callAsConstructor(const Function&, const Value* args,
                                         size_t count) override;
  ScopeState* pushScope() override;
  void popScope(ScopeState*) override;
  bool strictEquals(const Symbol& a, const Symbol& b) const override;
  bool strictEquals(const String& a, const String& b) const override;
  bool strictEquals(const Object& a, const Object& b) const override;
  bool instanceOf(const Object& o, const Function& f) override;

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
}  // namespace js
}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_JS_JSI_JSVM_JSVM_RUNTIME_H_
