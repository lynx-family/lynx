// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_JSI_V8_V8_RUNTIME_H_
#define CORE_RUNTIME_JS_JSI_V8_V8_RUNTIME_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "core/base/observer/observer_list.h"
#include "core/runtime/js/jsi/jsi.h"
#include "core/runtime/js/jsi/jslib.h"
#include "core/runtime/js/jsi/v8/v8_context_wrapper.h"
#include "core/runtime/js/jsi/v8/v8_helper.h"
#include "core/runtime/js/jsi/v8/v8_inspector_manager.h"
#include "core/runtime/js/jsi/v8/v8_isolate_wrapper.h"
#include "v8.h"

namespace lynx {
namespace runtime {
namespace js {
class V8Runtime : public Runtime {
 public:
  // Creates new context in new isolate
  V8Runtime();

  // Retains ctx
  ~V8Runtime();

  JSRuntimeType type() override { return JSRuntimeType::v8; }

  void InitRuntime(std::shared_ptr<JSIContext> sharedContext,
                   std::shared_ptr<JSIExceptionHandler> handler) override;
  std::shared_ptr<VMInstance> createVM(const StartupData*) const override;
  std::shared_ptr<VMInstance> getSharedVM() override;
  std::shared_ptr<JSIContext> createContext(
      std::shared_ptr<VMInstance>) const override;
  std::shared_ptr<JSIContext> getSharedContext() override;
  // StartupData createDefaultStartupData() override;

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
      const std::string& source_url) override {
    LOGE("evaluateJavaScriptBytecode not supported in v8");
    return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
        "evaluateJavaScriptBytecode not supported in v8"));
  }

  Object global() override;

  std::string description() override { return description_; };

  bool isInspectable() override { return true; };

  void setDescription(const std::string& desc) { description_ = desc; };

  v8::Local<v8::Context> getContext() const { return context_->getContext(); }

  v8::Isolate* getIsolate() const { return isolate_wrapper_->Isolate(); }

  // Value->JSValueRef (similar to above)
  v8::Local<v8::Value> valueRef(const Value& value);

  v8::Local<v8::ObjectTemplate> GetHostObjectTemplate();
  void SetHostObjectTemplate(v8::Local<v8::ObjectTemplate> object_template);
  v8::Local<v8::ObjectTemplate> GetHostFunctionTemplate();
  void SetHostFunctionTemplate(v8::Local<v8::ObjectTemplate> object_template);

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
  virtual std::weak_ptr<HostObject> getHostObject(const Object&) override;

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
  class V8ScopeState : public Runtime::ScopeState {
   public:
    V8ScopeState(v8::Isolate* iso) : handle_scope_(iso) {}

    virtual ~V8ScopeState(){};

   private:
    v8::HandleScope handle_scope_;
  };
  virtual ScopeState* pushScope() override;
  virtual void popScope(ScopeState*) override;
  bool strictEquals(const Symbol& a, const Symbol& b) const override;
  bool strictEquals(const String& a, const String& b) const override;
  bool strictEquals(const Object& a, const Object& b) const override;
  bool instanceOf(const Object& o, const Function& f) override;

  void RequestGC() override;

  void InitInspector(
      const std::shared_ptr<InspectorRuntimeObserverNG>& observer) override;
  void DestroyInspector() override;
  std::string AddPrefixToUrlIfNeeded(const std::string& url) override;

 private:
  std::shared_ptr<V8IsolateInstance> CreateVM_(const char* arg,
                                               bool useSnapshot) const;
  std::shared_ptr<V8ContextWrapper> CreateContext_(
      std::shared_ptr<VMInstance> vm) const;
  void Finalize();

 private:
  std::shared_ptr<V8IsolateInstance> isolate_wrapper_;
  std::shared_ptr<V8ContextWrapper> context_;
  // v8::Persistent<v8::Context> ctx_; // TODO remove
  std::string description_;
  std::atomic<bool> ctxInvalid_;

  std::unique_ptr<V8InspectorManager> inspector_manager_;

  v8::Persistent<v8::ObjectTemplate> host_object_template_;
  v8::Persistent<v8::ObjectTemplate> host_function_template_;

#ifndef NDEBUG
  mutable std::atomic<intptr_t> objectCounter_;
  mutable std::atomic<intptr_t> symbolCounter_;
  mutable std::atomic<intptr_t> stringCounter_;
#endif
};

class V8StartupData : public StartupData {
 public:
  V8StartupData(const char* args, v8::StartupData data) {
    args_ = args;
    data_ = data;
  }

  std::string args() const { return args_; }
  v8::StartupData data() const { return data_; }

 protected:
  std::string args_;
  v8::StartupData data_;
};

class V8DefaultStartupData : public V8StartupData {
 public:
  V8DefaultStartupData() : V8StartupData("", v8::StartupData()) {
    data_.raw_size = 0;
  }
};

}  // namespace js

}  // namespace runtime
}  // namespace lynx
#endif  // CORE_RUNTIME_JS_JSI_V8_V8_RUNTIME_H_
