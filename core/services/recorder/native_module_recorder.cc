// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/recorder/native_module_recorder.h"

#include <utility>
#include <vector>

#include "core/runtime/js/jsi/jsi.h"
#include "core/services/recorder/native_module_json.h"

namespace lynx {
namespace tasm {
namespace recorder {

void NativeModuleRecorder::RecordSharedData(const runtime::js::Value* args,
                                            runtime::js::Runtime* rt,
                                            int64_t record_id) {
  if (!TestBenchBaseRecorder::GetInstance().IsRecordingProcess()) {
    return;
  }

  runtime::js::Scope scope(*rt);

  std::vector<const runtime::js::Object*> visited_objs;
  rapidjson::Value value =
      ParsePiperValueToJsonValue(args[1], rt, &visited_objs);

  std::string key = args[0].getString(*rt).utf8(*rt);
  TestBenchBaseRecorder::GetInstance().RecordSharedData(key, value, record_id);
}

void NativeModuleRecorder::RecordFunctionCall(
    const char* module_name, const char* js_method_name, uint32_t argc,
    const runtime::js::Value* args, const int64_t* callbacks, uint32_t count,
    runtime::js::Value& res, runtime::js::Runtime* rt, int64_t record_id) {
  if (!TestBenchBaseRecorder::GetInstance().IsRecordingProcess()) {
    return;
  }
  runtime::js::Scope scope(*rt);

  rapidjson::Document::AllocatorType& allocator =
      TestBenchBaseRecorder::GetInstance().GetAllocator();

  rapidjson::Value params_val(rapidjson::kObjectType);

  params_val.AddMember(rapidjson::StringRef(kParamArgc), argc, allocator);

  std::vector<const runtime::js::Object*> visited_objs;
  rapidjson::Value args_val(rapidjson::kArrayType);
  for (uint32_t index = 0; index < argc; index++) {
    args_val.PushBack(
        ParsePiperValueToJsonValue(args[index], rt, &visited_objs), allocator);
  }
  params_val.AddMember(rapidjson::StringRef(kParamArgs), args_val, allocator);

  rapidjson::Value return_val =
      ParsePiperValueToJsonValue(res, rt, &visited_objs);
  params_val.AddMember(rapidjson::StringRef(kParamReturnValue), return_val,
                       allocator);

  if (count != 0) {
    rapidjson::Value callback_val(rapidjson::kArrayType);
    for (uint32_t i = 0; i < count; ++i) {
      rapidjson::Value tmp_callback;
      tmp_callback.SetString(std::to_string(callbacks[i]), allocator);
      callback_val.PushBack(tmp_callback, allocator);
    }
    params_val.AddMember(rapidjson::StringRef(kCallBack), callback_val,
                         allocator);
  }

  TestBenchBaseRecorder::GetInstance().RecordInvokedMethodData(
      module_name, js_method_name, params_val, record_id);
}

void NativeModuleRecorder::RecordCallback(const char* module_name,
                                          const char* js_method_name,
                                          const runtime::js::Value& args,
                                          runtime::js::Runtime* rt,
                                          int64_t callback_id,
                                          int64_t record_id) {
  if (!TestBenchBaseRecorder::GetInstance().IsRecordingProcess()) {
    return;
  }
  rapidjson::Document::AllocatorType& allocator =
      TestBenchBaseRecorder::GetInstance().GetAllocator();

  std::vector<const runtime::js::Object*> visited_objs;
  rapidjson::Value params_val(rapidjson::kObjectType);
  rapidjson::Value return_value =
      ParsePiperValueToJsonValue(args, rt, &visited_objs);
  params_val.AddMember(rapidjson::StringRef(kParamReturnValue), return_value,
                       allocator);

  TestBenchBaseRecorder::GetInstance().RecordCallback(
      module_name, js_method_name, params_val, callback_id, record_id);
}

void NativeModuleRecorder::RecordCallback(
    const char* module_name, const char* js_method_name,
    const runtime::js::Value* args, uint32_t count, runtime::js::Runtime* rt,
    int64_t callback_id, int64_t record_id) {
  if (!TestBenchBaseRecorder::GetInstance().IsRecordingProcess()) {
    return;
  }
  rapidjson::Document::AllocatorType& allocator =
      TestBenchBaseRecorder::GetInstance().GetAllocator();

  rapidjson::Value params_val(rapidjson::kObjectType);
  rapidjson::Value return_val(rapidjson::kArrayType);

  std::vector<const runtime::js::Object*> visited_objs;
  for (uint32_t index = 0; index < count; ++index) {
    rapidjson::Value value =
        ParsePiperValueToJsonValue(args[index], rt, &visited_objs);
    return_val.PushBack(value, allocator);
  }
  params_val.AddMember(rapidjson::StringRef(kParamReturnValue), return_val,
                       allocator);

  TestBenchBaseRecorder::GetInstance().RecordCallback(
      module_name, js_method_name, params_val, callback_id, record_id);
}

void NativeModuleRecorder::RecordGlobalEvent(
    std::string module_id, std::string method_id,
    const runtime::js::Array& arguments, runtime::js::Runtime* rt,
    int64_t record_id) {
  if (!TestBenchBaseRecorder::GetInstance().IsRecordingProcess()) {
    return;
  }
  auto size = arguments.length(*rt);
  if (!size) {
    return;
  }
  std::vector<runtime::js::Value> values(*size);
  for (size_t index = 0; index < *size; ++index) {
    auto item = arguments.getValueAtIndex(*rt, index);
    if (item) {
      values[index] = std::move(*item);
    }
  }
  rapidjson::Document::AllocatorType& allocator =
      TestBenchBaseRecorder::GetInstance().GetAllocator();
  rapidjson::Value params_val(rapidjson::kObjectType);

  params_val.AddMember("module_id", module_id, allocator);
  params_val.AddMember("method_id", method_id, allocator);

  rapidjson::Value return_val(rapidjson::kArrayType);

  std::vector<const runtime::js::Object*> visited_objs;
  for (uint32_t index = 0; index < *size; ++index) {
    rapidjson::Value value =
        ParsePiperValueToJsonValue(values[index], rt, &visited_objs);
    return_val.PushBack(value, allocator);
  }

  params_val.AddMember("arguments", return_val, allocator);
  TestBenchBaseRecorder::GetInstance().RecordAction(kFuncSendGlobalEvent,
                                                    params_val, record_id);
}

void NativeModuleRecorder::RecordEventAndroid(
    const std::vector<std::string>& args, int32_t event_type, int64_t record_id,
    TestBenchBaseRecorder* instance) {
  if (!(instance->IsRecordingProcess())) {
    return;
  }
  rapidjson::Document::AllocatorType& allocator =
      TestBenchBaseRecorder::GetInstance().GetAllocator();
  rapidjson::Value params_val(rapidjson::kObjectType);
  rapidjson::Value type_val(rapidjson::kStringType);
  if (event_type == 0 &&
      args.size() == sizeof(kEventAndroidArgs) / sizeof(char*)) {
    for (size_t index = 0; index < args.size(); index++) {
      rapidjson::Value key_val(rapidjson::kStringType);
      key_val.Set(args[index], allocator);
      params_val.AddMember(
          rapidjson::Value::StringRefType(kEventAndroidArgs[index]), key_val,
          allocator);
    }
    type_val.Set(std::string("touch"), allocator);
    params_val.AddMember("type", type_val, allocator);
  } else if (event_type == 1 &&
             args.size() == sizeof(kKeyEventAndroidArgs) / sizeof(char*)) {
    for (size_t index = 0; index < args.size(); index++) {
      rapidjson::Value key_val(rapidjson::kStringType);
      key_val.Set(args[index], allocator);
      params_val.AddMember(
          rapidjson::Value::StringRefType(kKeyEventAndroidArgs[index]), key_val,
          allocator);
    }
    type_val.Set(std::string("key"), allocator);
    params_val.AddMember("type", type_val, allocator);
  }
  instance->RecordAction(kFuncSendEventAndroid, params_val, record_id);
}

rapidjson::Value NativeModuleRecorder::ParsePiperValueToJsonValue(
    const runtime::js::Value& res, runtime::js::Runtime* rt,
    std::vector<const runtime::js::Object*>* visited_objs) {
  return NativeModuleValueToJson(
      res, rt, TestBenchBaseRecorder::GetInstance().GetAllocator(),
      visited_objs);
}

}  // namespace recorder
}  // namespace tasm
}  // namespace lynx
