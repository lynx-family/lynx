// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JSI_QUICKJS_QUICKJS_RUNTIME_H_
#define CORE_RUNTIME_JSI_QUICKJS_QUICKJS_RUNTIME_H_

#include <memory>
#include <set>
#include <string>

#include "base/include/base_export.h"
#include "base/include/log/logging.h"
#include "core/base/observer/observer.h"
#include "core/base/observer/observer_list.h"
#include "core/runtime/jscache/js_cache_tracker.h"
#include "core/runtime/jsi/jsi.h"
#include "core/runtime/jsi/jslib.h"
#include "core/runtime/jsi/quickjs/quickjs_context_wrapper.h"
#include "core/runtime/jsi/quickjs/quickjs_helper.h"
#include "core/runtime/jsi/quickjs/quickjs_host_function.h"
#include "core/runtime/jsi/quickjs/quickjs_host_object.h"
#include "core/runtime/jsi/quickjs/quickjs_inspector_manager.h"
#include "core/runtime/jsi/quickjs/quickjs_runtime_wrapper.h"

namespace lynx {
namespace piper {
class QuickjsRuntime : public Runtime {
 public:
  QuickjsRuntime();
  ~QuickjsRuntime() override;
  JSRuntimeType type() override { return JSRuntimeType::quickjs; }

  void InitRuntime(std::shared_ptr<JSIContext> sharedContext,
                   std::shared_ptr<JSIExceptionHandler> handler) override;
  // Supress gc pause when the mode is true.
  void SetGCPauseSuppressionMode(bool mode) override;
  bool GetGCPauseSuppressionMode() override;
  std::shared_ptr<VMInstance> createVM(const StartupData *) const override;
  // For CreateVM at vm_pool or createVM
  //  sync means in same use thread.
  static std::shared_ptr<VMInstance> CreateVM(const StartupData *, bool sync);
  std::shared_ptr<VMInstance> getSharedVM() override;
  std::shared_ptr<JSIContext> createContext(
      std::shared_ptr<VMInstance>) const override;
  std::shared_ptr<JSIContext> getSharedContext() override;

  base::expected<Value, JSINativeException> evaluateJavaScript(
      const std::shared_ptr<const Buffer> &buffer,
      const std::string &source_url) override;

  base::expected<Value, JSINativeException> evaluateJavaScriptBytecode(
      const std::shared_ptr<const Buffer> &buffer,
      const std::string &source_url) override;

  std::shared_ptr<const PreparedJavaScript> prepareJavaScript(
      const std::shared_ptr<const Buffer> &buffer,
      std::string source_url) override;

  base::expected<Value, JSINativeException> evaluatePreparedJavaScript(
      const std::shared_ptr<const PreparedJavaScript> &js) override;

  Object global() override;

  std::string description() override { return description_; };

  bool isInspectable() override { return false; };

  LEPUSContext *getJSContext() const { return context_->getContext(); };
  LEPUSRuntime *getJSRuntime() const {
    return quickjs_runtime_wrapper_->Runtime();
  };
  LEPUSValue valueRef(const piper::Value &value);
  LEPUSClassID getFunctionClassID() const;
  LEPUSClassID getObjectClassID() const;

  void InitInspector(
      const std::shared_ptr<InspectorRuntimeObserverNG> &observer) override;
  void DestroyInspector() override;

  void RequestGC() override;

 protected:
  PointerValue *cloneSymbol(const Runtime::PointerValue *pv) override;

  PointerValue *cloneString(const Runtime::PointerValue *pv) override;

  PointerValue *cloneObject(const Runtime::PointerValue *pv) override;

  PointerValue *clonePropNameID(const Runtime::PointerValue *pv) override;

  PropNameID createPropNameIDFromAscii(const char *str, size_t length) override;

  PropNameID createPropNameIDFromUtf8(const uint8_t *utf8,
                                      size_t length) override;

  PropNameID createPropNameIDFromString(const piper::String &str) override;

  std::string utf8(const PropNameID &id) override;

  bool compare(const PropNameID &id, const PropNameID &nameID) override;

  std::optional<std::string> symbolToString(const Symbol &symbol) override;

  piper::String createStringFromAscii(const char *str, size_t length) override;

  piper::String createStringFromUtf8(const uint8_t *utf8,
                                     size_t length) override;

  std::string utf8(const piper::String &string) override;

  Object createObject() override;

  Object createObject(std::shared_ptr<HostObject> ho) override;

  std::weak_ptr<HostObject> getHostObject(const piper::Object &object) override;

  //  piper::HostFunctionType &getHostFunction(const piper::Function
  //  &function) override;

  piper::HostFunctionType f = [](Runtime &rt, const piper::Value &thisVal,
                                 const piper::Value *args, size_t count) {
    return piper::Value::undefined();
  };
  piper::HostFunctionType &getHostFunction(const piper::Function &) override {
    return f;
  }

  std::optional<Value> getProperty(const Object &object,
                                   const PropNameID &name) override;

  std::optional<Value> getProperty(const Object &object,
                                   const piper::String &name) override;

  bool hasProperty(const Object &object, const PropNameID &name) override;

  bool hasProperty(const Object &object, const piper::String &name) override;

  bool setPropertyValue(Object &object, const PropNameID &name,
                        const piper::Value &value) override;
  bool setPropertyValueGC(Object &object, const char *name,
                          const piper::Value &value) override;
  bool setPropertyValue(Object &object, const piper::String &name,
                        const piper::Value &value) override;

  bool isArray(const Object &object) const override;

  bool isArrayBuffer(const Object &object) const override;

  bool isFunction(const Object &object) const override;

  bool isHostObject(const piper::Object &object) const override;

  bool isHostFunction(const piper::Function &function) const override;

  std::optional<piper::Array> getPropertyNames(const Object &object) override;

  std::optional<Array> createArray(size_t length) override;

  std::optional<BigInt> createBigInt(const std::string &value,
                                     Runtime &rt) override;

  piper::ArrayBuffer createArrayBufferCopy(const uint8_t *bytes,
                                           size_t byte_length) override;

  piper::ArrayBuffer createArrayBufferNoCopy(
      std::unique_ptr<const uint8_t[]> bytes, size_t byte_length) override;

  std::optional<size_t> size(const Array &array) override;

  size_t size(const ArrayBuffer &buffer) override;

  uint8_t *data(const ArrayBuffer &buffer) override;

  size_t copyData(const ArrayBuffer &, uint8_t *, size_t) override;

  std::optional<Value> getValueAtIndex(const Array &array, size_t i) override;

  bool setValueAtIndexImpl(Array &array, size_t i,
                           const piper::Value &value) override;

  piper::Function createFunctionFromHostFunction(
      const PropNameID &name, unsigned int paramCount,
      piper::HostFunctionType func) override;

  std::optional<Value> call(const piper::Function &function,
                            const piper::Value &jsThis,
                            const piper::Value *args, size_t count) override;

  std::optional<Value> callAsConstructor(const piper::Function &function,
                                         const piper::Value *args,
                                         size_t count) override;

  ScopeState *pushScope() override;

  void popScope(ScopeState *state) override;

  bool strictEquals(const Symbol &a, const Symbol &b) const override;

  bool strictEquals(const piper::String &a,
                    const piper::String &b) const override;

  bool strictEquals(const Object &a, const Object &b) const override;

  bool instanceOf(const Object &o, const piper::Function &f) override;

  std::shared_ptr<Buffer> GetBytecode(
      const std::shared_ptr<const Buffer> &buffer,
      const std::string &source_url) const;

  std::string BuildFilenameForDevTool(const std::string &source_url);

 private:
  std::shared_ptr<QuickjsContextWrapper> CreateContext_(
      std::shared_ptr<VMInstance> vm) const;

  std::shared_ptr<const PreparedJavaScript> PrepareJavaScriptBytecode(
      const std::shared_ptr<const Buffer> &buffer, std::string source_url,
      cache::JsCacheErrorCode &error_code);

  bool IsJavaScriptBytecode(const std::shared_ptr<const Buffer> &buffer);

 private:
  std::shared_ptr<QuickjsRuntimeInstance> quickjs_runtime_wrapper_;
  std::shared_ptr<QuickjsContextWrapper> context_;
  std::string description_;
  std::unique_ptr<QuickjsInspectorManager> inspector_manager_;
};

}  // namespace piper
}  // namespace lynx

// #ifdef __cplusplus
// }
// #endif

#endif  // CORE_RUNTIME_JSI_QUICKJS_QUICKJS_RUNTIME_H_
