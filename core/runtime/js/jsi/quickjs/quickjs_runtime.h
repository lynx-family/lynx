// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JS_JSI_QUICKJS_QUICKJS_RUNTIME_H_
#define CORE_RUNTIME_JS_JSI_QUICKJS_QUICKJS_RUNTIME_H_

#include <memory>
#include <set>
#include <string>
#include <unordered_map>

#include "base/include/log/logging.h"
#include "core/base/lynx_export.h"
#include "core/base/observer/observer.h"
#include "core/base/observer/observer_list.h"
#include "core/runtime/js/bytecode/js_cache_tracker.h"
#include "core/runtime/js/jsi/jsi.h"
#include "core/runtime/js/jsi/jslib.h"
#include "core/runtime/js/jsi/quickjs/quickjs_context_wrapper.h"
#include "core/runtime/js/jsi/quickjs/quickjs_helper.h"
#include "core/runtime/js/jsi/quickjs/quickjs_host_function.h"
#include "core/runtime/js/jsi/quickjs/quickjs_host_object.h"
#include "core/runtime/js/jsi/quickjs/quickjs_inspector_manager.h"
#include "core/runtime/js/jsi/quickjs/quickjs_runtime_wrapper.h"

namespace lynx {
namespace runtime {
namespace js {
class QuickjsRuntime : public Runtime, public JSIObserver {
 public:
  QuickjsRuntime();
  ~QuickjsRuntime() override;
  JSRuntimeType type() override { return JSRuntimeType::quickjs; }

  void InitRuntime(std::shared_ptr<JSIContext> sharedContext) override;
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
      const std::string &source_url, int start_line_offset = 0) override;

  base::expected<Value, JSINativeException> evaluateJavaScriptBytecode(
      const std::shared_ptr<const Buffer> &buffer,
      const std::string &source_url) override;

  std::unique_ptr<const PreparedJavaScript> prepareJavaScript(
      const std::shared_ptr<const Buffer> &buffer, std::string source_url,
      int start_line_offset = 0) override;

  base::expected<Value, JSINativeException> evaluatePreparedJavaScript(
      const PreparedJavaScript &js) override;

  Object global() override;

  std::string description() override { return description_; };

  bool isInspectable() override { return false; };

  void SetObserver(JSIObserver *observer) override { observer_ = observer; }

  void OnRuntimeGC(
      std::unordered_map<std::string, std::string> mem_info) override;

  LEPUSContext *getJSContext() const { return context_->getContext(); };
  LEPUSRuntime *getJSRuntime() const {
    return quickjs_runtime_wrapper_->Runtime();
  };
  LEPUSValue valueRef(const Value &value);
  LEPUSClassID getFunctionClassID() const;
  LEPUSClassID getObjectClassID() const;

  void InitInspector(
      const std::shared_ptr<InspectorRuntimeObserverNG> &observer) override;
  void DestroyInspector() override;

  void RequestGC() override;
  int64_t GetCurrentHeapSizeBytes() override;

 protected:
  PointerValue *cloneSymbol(const Runtime::PointerValue *pv) override;

  PointerValue *cloneString(const Runtime::PointerValue *pv) override;

  PointerValue *cloneObject(const Runtime::PointerValue *pv) override;

  PointerValue *clonePropNameID(const Runtime::PointerValue *pv) override;

  PropNameID createPropNameIDFromAscii(const char *str, size_t length) override;

  PropNameID createPropNameIDFromUtf8(const uint8_t *utf8,
                                      size_t length) override;

  PropNameID createPropNameIDFromString(const String &str) override;

  std::string utf8(const PropNameID &id) override;

  bool compare(const PropNameID &id, const PropNameID &nameID) override;

  std::optional<std::string> symbolToString(const Symbol &symbol) override;

  String createStringFromAscii(const char *str, size_t length) override;

  String createStringFromUtf8(const uint8_t *utf8, size_t length) override;

  std::string utf8(const String &string) override;

  Object createObject() override;

  Object createObject(std::shared_ptr<HostObject> ho) override;

  std::weak_ptr<HostObject> getHostObject(const Object &object) override;

  //  HostFunctionType &getHostFunction(const Function
  //  &function) override;

  HostFunctionType f = [](Runtime &rt, const Value &thisVal, const Value *args,
                          size_t count) { return Value::undefined(); };
  HostFunctionType &getHostFunction(const Function &) override { return f; }

  std::optional<Value> getProperty(const Object &object,
                                   const PropNameID &name) override;

  std::optional<Value> getProperty(const Object &object,
                                   const String &name) override;

  bool hasProperty(const Object &object, const PropNameID &name) override;

  bool hasProperty(const Object &object, const String &name) override;

  bool setPropertyValue(Object &object, const PropNameID &name,
                        const Value &value) override;
  bool setPropertyValueGC(Object &object, const char *name,
                          const Value &value) override;
  bool setPropertyValue(Object &object, const String &name,
                        const Value &value) override;

  bool isArray(const Object &object) const override;

  bool isArrayBuffer(const Object &object) const override;

  bool isFunction(const Object &object) const override;

  bool isHostObject(const Object &object) const override;

  bool isHostFunction(const Function &function) const override;

  std::optional<Array> getPropertyNames(const Object &object) override;

  std::optional<Array> createArray(size_t length) override;

  std::optional<BigInt> createBigInt(const std::string &value,
                                     Runtime &rt) override;

  ArrayBuffer createArrayBufferCopy(const uint8_t *bytes,
                                    size_t byte_length) override;

  ArrayBuffer createArrayBufferNoCopy(std::unique_ptr<const uint8_t[]> bytes,
                                      size_t byte_length) override;

  std::optional<size_t> size(const Array &array) override;

  size_t size(const ArrayBuffer &buffer) override;

  uint8_t *data(const ArrayBuffer &buffer) override;

  size_t copyData(const ArrayBuffer &, uint8_t *, size_t) override;

  std::optional<Value> getValueAtIndex(const Array &array, size_t i) override;

  bool setValueAtIndexImpl(Array &array, size_t i, const Value &value) override;

  Function createFunctionFromHostFunction(const PropNameID &name,
                                          unsigned int paramCount,
                                          HostFunctionType func) override;

  std::optional<Value> call(const Function &function, const Value &jsThis,
                            const Value *args, size_t count) override;

  std::optional<Value> callAsConstructor(const Function &function,
                                         const Value *args,
                                         size_t count) override;

  ScopeState *pushScope() override;

  void popScope(ScopeState *state) override;

  bool strictEquals(const Symbol &a, const Symbol &b) const override;

  bool strictEquals(const String &a, const String &b) const override;

  bool strictEquals(const Object &a, const Object &b) const override;

  bool instanceOf(const Object &o, const Function &f) override;

  std::shared_ptr<Buffer> GetBytecode(
      const std::shared_ptr<const Buffer> &buffer,
      const std::string &source_url) const;

  std::string AddPrefixToUrlIfNeeded(const std::string &url) override;

 private:
  std::shared_ptr<QuickjsContextWrapper> CreateContext_(
      std::shared_ptr<VMInstance> vm) const;

  std::unique_ptr<const PreparedJavaScript> PrepareJavaScriptBytecode(
      const std::shared_ptr<const Buffer> &buffer, std::string source_url,
      cache::JsCacheErrorCode &error_code);

  bool IsJavaScriptBytecode(const std::shared_ptr<const Buffer> &buffer);

 private:
  std::shared_ptr<QuickjsRuntimeInstance> quickjs_runtime_wrapper_;
  std::shared_ptr<QuickjsContextWrapper> context_;
  std::string description_;
  std::unique_ptr<QuickjsInspectorManager> inspector_manager_;
  JSIObserver *observer_ = nullptr;
};

}  // namespace js

}  // namespace runtime
}  // namespace lynx

// #ifdef __cplusplus
// }
// #endif

#endif  // CORE_RUNTIME_JS_JSI_QUICKJS_QUICKJS_RUNTIME_H_
