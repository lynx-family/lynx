// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/data/harmony/template_data_harmony.h"

#include <utility>

#include "base/include/platform/harmony/napi_util.h"
#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_trace_event_def.h"
#include "core/base/harmony/napi_convert_helper.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/data/harmony/platform_data_harmony.h"
#include "core/runtime/lepus/json_parser.h"

namespace lynx {
namespace tasm {

namespace {

lepus::Value ConvertNapiValue(napi_env env, napi_value raw_value,
                              bool& from_json, std::string& json_string) {
  napi_valuetype type;
  napi_typeof(env, raw_value, &type);

  if (type == napi_valuetype::napi_string) {
    from_json = true;
    json_string = base::NapiUtil::ConvertToString(env, raw_value);
    return lepus::jsonValueTolepusValue(json_string.c_str());
  }
  if (type == napi_valuetype::napi_object) {
    return base::NapiConvertHelper::ConvertToLepusValue(env, raw_value);
  }

  LOGE("Error in TemplateDataHarmony, type should be string or object");
  return lepus::Value();
}

}  // namespace

napi_ref NativeTemplateDataHarmony::constructor_ref_ = nullptr;

napi_value NativeTemplateDataHarmony::Init(napi_env env, napi_value exports) {
  napi_value constructor = nullptr;
  napi_status status =
      napi_define_class(env, "NativeTemplateData", NAPI_AUTO_LENGTH, New,
                        nullptr, 0, nullptr, &constructor);
  if (status != napi_ok || constructor == nullptr) {
    LOGE("Failed to define NativeTemplateData class, status: " << status);
    return nullptr;
  }

  status = napi_create_reference(env, constructor, 1, &constructor_ref_);
  if (status != napi_ok) {
    LOGE("Failed to create NativeTemplateData constructor reference, status: "
         << status);
    return nullptr;
  }

  status =
      napi_set_named_property(env, exports, "NativeTemplateData", constructor);
  if (status != napi_ok) {
    LOGE("Failed to export NativeTemplateData class, status: " << status);
    return nullptr;
  }
  return exports;
}

napi_value NativeTemplateDataHarmony::New(napi_env env,
                                          napi_callback_info info) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, NATIVE_TEMPLATE_DATA_NEW);

  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_value js_this = nullptr;
  napi_status status =
      napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);
  if (status != napi_ok || js_this == nullptr) {
    LOGE("Failed to get NativeTemplateData constructor arguments, status: "
         << status);
    return nullptr;
  }

  bool from_json = false;
  std::string json_string;
  lepus::Value value =
      argc == 1 ? ConvertNapiValue(env, args[0], from_json, json_string)
                : lepus::Value();

  auto* native_data = new NativeTemplateDataHarmony(std::move(value), from_json,
                                                    std::move(json_string));
  static napi_finalize finalize = [](napi_env env, void* data, void* hint) {
    delete static_cast<NativeTemplateDataHarmony*>(data);
  };
  status = napi_wrap(env, js_this, native_data, finalize, nullptr, nullptr);
  if (status != napi_ok) {
    LOGE("Failed to wrap NativeTemplateData, status: " << status);
    delete native_data;
    return nullptr;
  }
  return js_this;
}

NativeTemplateDataHarmony* NativeTemplateDataHarmony::FromNapiValue(
    napi_env env, napi_value value) {
  if (constructor_ref_ == nullptr || value == nullptr) {
    return nullptr;
  }

  napi_valuetype type;
  if (napi_typeof(env, value, &type) != napi_ok || type != napi_object) {
    return nullptr;
  }

  napi_value constructor = nullptr;
  if (napi_get_reference_value(env, constructor_ref_, &constructor) !=
          napi_ok ||
      constructor == nullptr) {
    return nullptr;
  }

  bool is_instance = false;
  if (napi_instanceof(env, value, constructor, &is_instance) != napi_ok ||
      !is_instance) {
    return nullptr;
  }

  NativeTemplateDataHarmony* native_data = nullptr;
  if (napi_unwrap(env, value, reinterpret_cast<void**>(&native_data)) !=
      napi_ok) {
    return nullptr;
  }
  return native_data;
}

std::shared_ptr<tasm::TemplateData> TemplateDataHarmony::GenerateTemplateData(
    napi_env env, napi_value raw_value, napi_value raw_read_only,
    napi_value raw_processor_name) {
  bool from_json = false;
  std::string json_str;
  lepus::Value result;

  auto* native_data = NativeTemplateDataHarmony::FromNapiValue(env, raw_value);
  if (native_data != nullptr) {
    result = lepus::Value::ShallowCopy(native_data->GetValue());
    from_json = native_data->IsFromJson();
    if (from_json) {
      json_str = native_data->GetJsonString();
    }
  } else {
    result = ConvertNapiValue(env, raw_value, from_json, json_str);
  }

  if (result.IsNil()) {
    return nullptr;
  }

  napi_valuetype type;
  bool read_only = false;
  napi_typeof(env, raw_read_only, &type);
  if (type == napi_boolean) {
    read_only = base::NapiUtil::ConvertToBoolean(env, raw_read_only);
  }

  std::string processor_name;
  napi_typeof(env, raw_processor_name, &type);

  if (type == napi_string) {
    processor_name =
        base::NapiUtil::ConvertToShortString(env, raw_processor_name);
  }

  auto data =
      std::make_shared<TemplateDataHarmony>(result, read_only, processor_name);
  if (from_json) {
    data->SetPlatformData(std::make_unique<PlatformDataHarmony>(json_str));
  } else {
    // Clone the value for the JS thread on a background task so the load path
    // only pays for the shallow copy above. For native data the wrapper keeps
    // the source alive; for converted data `result` owns it. In both cases the
    // clone only reads const/immutable subtrees, so the concurrent read after
    // `data` is handed to the engine is safe.
    std::promise<lepus::Value> promise;
    std::future<lepus::Value> future = promise.get_future();
    auto async_task = fml::MakeRefCounted<base::OnceTask<lepus::Value>>(
        [result, promise = std::move(promise)]() mutable {
          auto cloned_result = lepus::Value::Clone(result);
          // Release the source value before making the cloned result visible.
          // Once set_value() wakes the consumer, the source dictionary may be
          // mutated and must no longer be retained by this worker task.
          result = lepus::Value();
          promise.set_value(std::move(cloned_result));
        },
        std::move(future));

    base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
        [async_task]() { async_task->Run(); },
        base::ConcurrentTaskType::HIGH_PRIORITY);

    data->SetAsyncTask(async_task);
  }

  return data;
}

lepus::Value TemplateDataHarmony::GenerateLepusValue(napi_env env,
                                                     napi_value raw_value) {
  auto* native_data = NativeTemplateDataHarmony::FromNapiValue(env, raw_value);
  if (native_data != nullptr) {
    return lepus::Value::ShallowCopy(native_data->GetValue());
  }
  return base::NapiConvertHelper::JSONToLepusValue(env, raw_value);
}

const lepus::Value& TemplateDataHarmony::GetValue() const {
  const_cast<TemplateDataHarmony*>(this)->EnsurePlatformData();
  return value_;
}

std::unique_ptr<PlatformData> TemplateDataHarmony::ObtainPlatformData() {
  EnsurePlatformData();
  return std::move(platform_data_);
}

void TemplateDataHarmony::EnsurePlatformData() {
  if (platform_data_ == nullptr && async_task_.get() != nullptr) {
    async_task_->Run();
    SetPlatformData(
        std::make_unique<PlatformDataHarmony>(async_task_->GetFuture().get()));
    async_task_ = nullptr;
  }
}

}  // namespace tasm
}  // namespace lynx
