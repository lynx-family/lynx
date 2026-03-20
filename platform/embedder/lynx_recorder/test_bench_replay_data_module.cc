// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_recorder/test_bench_replay_data_module.h"

#include <memory>
#include <mutex>
#include <unordered_map>

#include "platform/embedder/lynx_recorder/test_bench_utils.h"
#include "platform/embedder/public/capi/lynx_native_module_capi.h"

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_defines.h"
#endif

namespace lynx {
namespace embedder {

class ReplayDataModulesHolder {
 public:
  static ReplayDataModulesHolder& GetInstance() {
    static ReplayDataModulesHolder instance;
    return instance;
  }

  std::shared_ptr<TestBenchReplayDataModule> GetDataModule(void* context) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_modules_.find(context);
    if (it != data_modules_.end()) {
      return it->second.lock();
    }
    return nullptr;
  }

  void AddDataModule(void* context,
                     std::weak_ptr<TestBenchReplayDataModule> m) {
    std::lock_guard<std::mutex> lock(mutex_);
    data_modules_[context] = m;
  }

  void RemoveDataModule(void* context) {
    std::lock_guard<std::mutex> lock(mutex_);
    data_modules_.erase(context);
  }

  ReplayDataModulesHolder(const ReplayDataModulesHolder&) = delete;
  ReplayDataModulesHolder& operator=(const ReplayDataModulesHolder&) = delete;

 private:
  ReplayDataModulesHolder() = default;
  ~ReplayDataModulesHolder() = default;

  std::mutex mutex_;
  std::unordered_map<void*, std::weak_ptr<TestBenchReplayDataModule>>
      data_modules_;
};

napi_value GetData(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1];
  napi_value thiz = nullptr;
  napi_get_cb_info(env, info, &argc, argv, &thiz, nullptr);
  if (argc < 1) {
    return nullptr;
  }
  void* context = nullptr;
  lynx_napi_get_instance_data(env, LYNX_NAPI_ENV_LYNX_VIEW_TAG, &context);
  std::shared_ptr<TestBenchReplayDataModule> m =
      ReplayDataModulesHolder::GetInstance().GetDataModule(context);
  if (m) {
    napi_value data = nullptr;
    napi_create_string_utf8(env, m->GetData().c_str(), NAPI_AUTO_LENGTH, &data);
    napi_call_function(env, thiz, argv[0], 1, &data, nullptr);
  }
  return nullptr;
}

napi_value TestBenchReplayDataModuleCreator(napi_env env, napi_value exports,
                                            const char* module_name,
                                            void* opaque) {
  napi_value func;
  napi_create_function(env, "getData", 1, &GetData, 0, &func);
  napi_set_named_property(env, exports, "getData", func);
  return exports;
}

TestBenchReplayDataModule::~TestBenchReplayDataModule() {
  if (context_) {
    ReplayDataModulesHolder::GetInstance().RemoveDataModule(context_);
  }
}

void TestBenchReplayDataModule::RegisterJSB(
    std::function<void(const std::string&, napi_module_creator)> jsb_register) {
  jsb_register("LynxRecorderReplayDataModule",
               TestBenchReplayDataModuleCreator);
}

void TestBenchReplayDataModule::BindContext(void* context) {
  context_ = context;
  ReplayDataModulesHolder::GetInstance().AddDataModule(context_,
                                                       weak_from_this());
}

void TestBenchReplayDataModule::SetJsbIgnoredInfo(const std::string& data) {
  jsb_ignored_info_ = data;
}

void TestBenchReplayDataModule::SetJsbSettings(const std::string& data) {
  jsb_settings_ = data;
}

void TestBenchReplayDataModule::SetFunctionCall(const std::string& data) {
  function_call_ = data;
}

void TestBenchReplayDataModule::SetCallbackData(const std::string& data) {
  callback_data_ = data;
}

std::string TestBenchReplayDataModule::GetRecordData() {
  rapidjson::Document dom;
  dom.Parse(function_call_);
  if (dom.HasParseError() || !dom.IsArray()) {
    return "{}";
  }
  rapidjson::Document callback_data_dom;
  callback_data_dom.Parse(callback_data_);
  if (callback_data_dom.HasParseError()) {
    return "{}";
  }
  rapidjson::Document new_dom;
  auto& allocator = new_dom.GetAllocator();
  new_dom.SetObject();
  for (rapidjson::SizeType i = 0; i < dom.Size(); i++) {
    const rapidjson::Value& obj = dom[i];
    std::string module_name;
    if (obj.HasMember("Module Name") && obj["Module Name"].IsString()) {
      module_name = obj["Module Name"].GetString();
    }
    if (!new_dom.HasMember(module_name)) {
      rapidjson::Value new_values(rapidjson::kArrayType);
      new_dom.AddMember(rapidjson::StringRef(module_name), new_values,
                        allocator);
    }
    std::string method_name;
    if (obj.HasMember("Method Name") && obj["Method Name"].IsString()) {
      method_name = obj["Method Name"].GetString();
    }
    int64_t record_time = 0;
    if (obj.HasMember("Record Time") && obj["Record Time"].IsString()) {
      std::string record_time_str = obj["Record Time"].GetString();
      int record_time_value = 0;
      StringToInt(record_time_str, &record_time_value, 10);
      record_time = record_time_value * 1000;
    }
    if (obj.HasMember("RecordMillisecond") &&
        obj["RecordMillisecond"].IsInt64()) {
      record_time = obj["RecordMillisecond"].GetInt64();
    }
    std::string function_invoke_label;
    rapidjson::Value callback_return_values(rapidjson::kArrayType);
    if (obj.HasMember("Params")) {
      const rapidjson::Value& params = obj["Params"];
      if (params.HasMember("callback") && params["callback"].IsArray()) {
        const rapidjson::Value& callback_ids = params["callback"];
        for (rapidjson::SizeType j = 0; j < callback_ids.Size(); j++) {
          std::string callback_id;
          if (callback_ids[j].IsString()) {
            callback_id = callback_ids[j].GetString();
          }
          if (callback_data_dom.HasMember(callback_id)) {
            const rapidjson::Value& callback_info =
                callback_data_dom[callback_id];
            int64_t response_time = 0;
            if (callback_info.HasMember("Record Time") &&
                callback_info["Record Time"].IsString()) {
              std::string response_time_str =
                  callback_info["Record Time"].GetString();
              int response_time_value = 0;
              StringToInt(response_time_str, &response_time_value, 10);
              response_time = response_time_value * 1000;
            }
            if (callback_info.HasMember("RecordMillisecond") &&
                callback_info["RecordMillisecond"].IsInt64()) {
              response_time = callback_info["RecordMillisecond"].GetInt64();
            }
            rapidjson::Value callback_kernel(rapidjson::kObjectType);
            callback_kernel.AddMember(rapidjson::StringRef("Delay"),
                                      response_time - record_time, allocator);
            rapidjson::Value callback_info_Params(callback_info["Params"],
                                                  allocator);
            callback_kernel.AddMember(rapidjson::StringRef("Value"),
                                      callback_info_Params, allocator);
            callback_return_values.PushBack(callback_kernel, allocator);
            function_invoke_label += ToJson(callback_info) + "_";
          }
        }
      }
    }
    rapidjson::Value method_look_up(rapidjson::kObjectType);
    if (obj.HasMember("SyncAttributes")) {
      rapidjson::Value SyncAttributes(obj["SyncAttributes"], allocator);
      method_look_up.AddMember(rapidjson::StringRef("SyncAttributes"),
                               SyncAttributes, allocator);
    }
    method_look_up.AddMember(rapidjson::StringRef("Method Name"), method_name,
                             allocator);
    method_look_up.AddMember(rapidjson::StringRef("Callback"),
                             callback_return_values, allocator);
    method_look_up.AddMember(rapidjson::StringRef("Label"),
                             function_invoke_label, allocator);
    rapidjson::Value Params(obj["Params"], allocator);
    method_look_up.AddMember(rapidjson::StringRef("Params"), Params, allocator);
    new_dom[module_name].PushBack(method_look_up, allocator);
  }

  return ToJson(new_dom);
}

std::string TestBenchReplayDataModule::GetJsbIgnoredInfo() {
  return jsb_ignored_info_;
}

std::string TestBenchReplayDataModule::GetJsbSettings() {
  return jsb_settings_;
}

std::string TestBenchReplayDataModule::GetData() {
  rapidjson::Document dom;
  dom.SetObject();
  auto& allocator = dom.GetAllocator();
  dom.AddMember(rapidjson::StringRef("RecordData"), GetRecordData(), allocator);
  dom.AddMember(rapidjson::StringRef("JsbIgnoredInfo"), GetJsbIgnoredInfo(),
                allocator);
  dom.AddMember(rapidjson::StringRef("JsbSettings"), GetJsbSettings(),
                allocator);
  return ToJson(dom);
}

}  // namespace embedder
}  // namespace lynx

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_undefs.h"
#endif
