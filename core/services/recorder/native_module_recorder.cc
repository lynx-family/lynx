// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/recorder/native_module_recorder.h"

#include <cmath>

#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace tasm {
namespace recorder {

void NativeModuleRecorder::RecordFunctionCall(
    const char* module_name, const char* js_method_name, uint32_t argc,
    const piper::Value* args, const int64_t* callbacks, uint32_t count,
    piper::Value& res, piper::Runtime* rt, int64_t record_id) {
  if (!TestBenchBaseRecorder::GetInstance().IsRecordingProcess()) {
    return;
  }
  piper::Scope scope(*rt);

  rapidjson::Document::AllocatorType& allocator =
      TestBenchBaseRecorder::GetInstance().GetAllocator();

  rapidjson::Value params_val(rapidjson::kObjectType);

  params_val.AddMember(rapidjson::StringRef(kParamArgc), argc, allocator);

  rapidjson::Value args_val(rapidjson::kArrayType);
  for (uint32_t index = 0; index < argc; index++) {
    args_val.PushBack(ParsePiperValueToJsonValue(args[index], rt), allocator);
  }
  params_val.AddMember(rapidjson::StringRef(kParamArgs), args_val, allocator);

  rapidjson::Value return_val = ParsePiperValueToJsonValue(res, rt);
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
                                          const piper::Value& args,
                                          piper::Runtime* rt,
                                          int64_t callback_id,
                                          int64_t record_id) {
  if (!TestBenchBaseRecorder::GetInstance().IsRecordingProcess()) {
    return;
  }
  rapidjson::Document::AllocatorType& allocator =
      TestBenchBaseRecorder::GetInstance().GetAllocator();

  rapidjson::Value params_val(rapidjson::kObjectType);
  rapidjson::Value return_value = ParsePiperValueToJsonValue(args, rt);
  params_val.AddMember(rapidjson::StringRef(kParamReturnValue), return_value,
                       allocator);

  TestBenchBaseRecorder::GetInstance().RecordCallback(
      module_name, js_method_name, params_val, callback_id, record_id);
}

void NativeModuleRecorder::RecordCallback(const char* module_name,
                                          const char* js_method_name,
                                          const piper::Value* args,
                                          uint32_t count, piper::Runtime* rt,
                                          int64_t callback_id,
                                          int64_t record_id) {
  if (!TestBenchBaseRecorder::GetInstance().IsRecordingProcess()) {
    return;
  }
  rapidjson::Document::AllocatorType& allocator =
      TestBenchBaseRecorder::GetInstance().GetAllocator();

  rapidjson::Value params_val(rapidjson::kObjectType);
  rapidjson::Value return_val(rapidjson::kArrayType);

  for (uint32_t index = 0; index < count; ++index) {
    rapidjson::Value value = ParsePiperValueToJsonValue(args[index], rt);
    return_val.PushBack(value, allocator);
  }
  params_val.AddMember(rapidjson::StringRef(kParamReturnValue), return_val,
                       allocator);

  TestBenchBaseRecorder::GetInstance().RecordCallback(
      module_name, js_method_name, params_val, callback_id, record_id);
}

void NativeModuleRecorder::RecordGlobalEvent(std::string module_id,
                                             std::string method_id,
                                             const piper::Value* args,
                                             uint64_t count, piper::Runtime* rt,
                                             int64_t record_id) {
  if (!TestBenchBaseRecorder::GetInstance().IsRecordingProcess()) {
    return;
  }
  rapidjson::Document::AllocatorType& allocator =
      TestBenchBaseRecorder::GetInstance().GetAllocator();
  rapidjson::Value params_val(rapidjson::kObjectType);

  params_val.AddMember("module_id", module_id, allocator);
  params_val.AddMember("method_id", method_id, allocator);

  rapidjson::Value return_val(rapidjson::kArrayType);

  for (uint32_t index = 0; index < count; ++index) {
    rapidjson::Value value = ParsePiperValueToJsonValue(args[index], rt);
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
    const piper::Value& res, piper::Runtime* rt) {
  piper::Scope scope(*rt);
  rapidjson::Document::AllocatorType& allocator =
      TestBenchBaseRecorder::GetInstance().GetAllocator();

  rapidjson::Value return_val(rapidjson::kNullType);

  if (res.isBool()) {
    return_val.SetBool(res.getBool());
  } else if (res.isNumber()) {
    double number = res.getNumber();
    if (std::isnan(number)) {
      return_val.SetString("NaN", allocator);
    } else {
      return_val.SetDouble(number);
    }
  } else if (res.isString()) {
    return_val.SetString(res.getString(*rt).utf8(*rt).c_str(), allocator);
  } else if (res.isSymbol()) {
    return_val.SetString(res.getSymbol(*rt).toString(*rt)->c_str(), allocator);
  } else if (res.isObject()) {
    piper::Object piper_obj = res.getObject(*rt);

    if (piper_obj.isArray(*rt)) {
      // Parse Array
      return_val.SetArray();
      piper::Array piper_array = piper_obj.getArray(*rt);
      auto array_size = piper_array.size(*rt);
      if (array_size) {
        for (size_t index = 0; index != *array_size; index++) {
          auto val_opt = piper_array.getValueAtIndex(*rt, index);
          if (!val_opt) {
            return return_val;
          }
          return_val.PushBack(ParsePiperValueToJsonValue(*val_opt, rt),
                              allocator);
        }
      }
    } else if (piper_obj.isArrayBuffer(*rt)) {
      // Parse Array Buffer
      auto buffer = piper_obj.getArrayBuffer(*rt);
      return_val.SetUint(static_cast<uint32_t>(*buffer.data(*rt)));

    } else if (piper_obj.isFunction(*rt)) {
      // TODO(kechenglong): parse function if needed
      return_val.SetString(kParamFunction, allocator);

    } else if (piper_obj.isHostObject(*rt)) {
      // Parse HostObject
      return_val.SetObject();
      auto weak_host_obj = piper_obj.getHostObject(*rt);
      auto host_obj = weak_host_obj.lock();
      if (!host_obj) {
        return return_val;
      }
      auto property_names = host_obj->getPropertyNames(*rt);
      for (auto& property_name : property_names) {
        rapidjson::Value key(rapidjson::kStringType);
        key.SetString(property_name.utf8(*rt).c_str(), allocator);
        piper::Value piper_val = host_obj->get(rt, property_name);
        rapidjson::Value val = ParsePiperValueToJsonValue(piper_val, rt);
        return_val.AddMember(key, val, allocator);
      }

    } else {
      // Parse Object
      auto property_names_array_opt = piper_obj.getPropertyNames(*rt);
      if (!property_names_array_opt) {
        return return_val;
      }
      auto array_size_opt = property_names_array_opt->size(*rt);
      if (!array_size_opt) {
        return return_val;
      }
      return_val.SetObject();
      for (size_t index = 0; index != *array_size_opt; index++) {
        auto property_name =
            property_names_array_opt->getValueAtIndex(*rt, index);
        if (!property_name) {
          return return_val;
        }
        rapidjson::Value key = ParsePiperValueToJsonValue(*property_name, rt);
        auto piper_val = piper_obj.getProperty(*rt, key.GetString());
        if (!piper_val) {
          return return_val;
        }
        rapidjson::Value val = ParsePiperValueToJsonValue(*piper_val, rt);
        return_val.AddMember(key, val, allocator);
      }
    }
  } else {
    if (res.isNull()) {
      return_val.SetNull();
    } else {
      return_val.SetString(res.toString(*rt)->utf8(*rt).c_str(), allocator);
    }
  }

  return return_val;
}

}  // namespace recorder
}  // namespace tasm
}  // namespace lynx
