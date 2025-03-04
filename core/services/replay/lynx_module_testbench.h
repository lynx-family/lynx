// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_REPLAY_LYNX_MODULE_TESTBENCH_H_
#define CORE_SERVICES_REPLAY_LYNX_MODULE_TESTBENCH_H_
#include <memory>
#include <string>
#include <vector>

#include "base/include/fml/thread.h"
#include "base/include/string/string_utils.h"
#include "core/runtime/bindings/jsi/modules/lynx_module.h"
#include "core/runtime/bindings/jsi/modules/module_delegate.h"
#include "core/runtime/jsi/jsi.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace piper {

typedef std::function<void(const rapidjson::Value&, Runtime*,
                           const piper::Value*, size_t)>
    SyncToPlatformHandler;

typedef std::function<void()> InvokeMethodCallback;

typedef std::function<void(const std::string&, Runtime& runtime,
                           InvokeMethodCallback callback)>
    FetchDataHandler;

class ModuleTestBench : public LynxModule {
 public:
  ModuleTestBench(const std::string& name,
                  const std::shared_ptr<ModuleDelegate>& delegate)
      : LynxModule(name, delegate), testbench_thread_("test_bench_thread") {}
  ~ModuleTestBench() override = default;
  void initModuleData(
      const rapidjson::Value& value, rapidjson::Value* value_ptr,
      rapidjson::Value* jsb_settings_ptr,
      const std::function<void(rapidjson::Value&, const rapidjson::Value&)>&
          copy_json_value,
      SyncToPlatformHandler sync_to_platform_handler);
  virtual void Destroy() override;

  void SetFetchDataHandler(FetchDataHandler handler);

  std::shared_ptr<LynxModule::MethodMetadata> GetMethodMetaData(
      const std::string method_name);

  piper::Value get(Runtime* rt, const PropNameID& prop) override;

  /**
   [
      {
      methodName:xxx,
      params:xxx,
      returnValue:xxx,
      },
      ......
   ]
   */
  rapidjson::Value moduleData = rapidjson::Value(rapidjson::kNullType);

 protected:
  base::expected<piper::Value, piper::JSINativeException> invokeMethod(
      const MethodMetadata& method, Runtime* rt, const piper::Value* args,
      size_t count) override;

  piper::Value invokeMethodKernel(const MethodMetadata& method, Runtime* rt,
                                  const piper::Value* args, size_t count);

  piper::Value getAttributeValue(Runtime* rt, std::string propName) override;

 private:
  using ValueKind = Value::ValueKind;

  bool IsStrictMode();

  bool IsJsbIgnoredParams(const std::string& param);

  rapidjson::Value* jsb_ignored_info_;

  rapidjson::Value* jsb_settings_;

  bool IsSameURL(const std::string& first, const std::string& second);

  bool isSameMethod(const MethodMetadata& method, Runtime* rt,
                    const piper::Value* args, size_t count,
                    rapidjson::Value& value);
  bool isSameArgs(Runtime* rt, const piper::Value* args, size_t count,
                  rapidjson::Value& value);
  bool sameKernel(Runtime* rt, const piper::Value* args,
                  rapidjson::Value& value);
  void syncToPlatform(const rapidjson::Value& moduleData,
                      const MethodMetadata& method, Runtime* rt,
                      const piper::Value* args, size_t count);
  // build methodMap for class LynxModule
  void buildLookupMap();

  static std::string kUndefined;
  static std::string kContainerID;
  static std::string kTimeStamp;
  static std::string kCardVersion;
  static std::string kHeader;
  static std::string kRequestTime;
  static std::string kFunction;
  static std::string kNaN;

  // record callbackFunction for every jsb call, it will be clear at end of this
  // call
  std::vector<piper::Function> callbackFunctions;

  void ActionsForJsbMatchFailed(Runtime* rt, const piper::Value* args,
                                size_t count);

  void InvokeJsbCallback(piper::Function callback_function,
                         rapidjson::Value&& value, int64_t delay = -1);

  fml::Thread testbench_thread_;

  std::function<void(rapidjson::Value&, rapidjson::Value&)> copy_json_value_;

  SyncToPlatformHandler sync_to_platform_handler_;
  FetchDataHandler fetch_data_handler_;
  std::shared_ptr<GroupInterceptor> group_interceptor_;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_SERVICES_REPLAY_LYNX_MODULE_TESTBENCH_H_
