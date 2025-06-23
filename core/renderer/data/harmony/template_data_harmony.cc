// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/data/harmony/template_data_harmony.h"

#include <utility>

#include "base/include/platform/harmony/napi_util.h"
#include "core/base/harmony/napi_convert_helper.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/data/harmony/platform_data_harmony.h"
#include "core/runtime/vm/lepus/json_parser.h"

namespace lynx {
namespace tasm {

std::shared_ptr<tasm::TemplateData> TemplateDataHarmony::GenerateTemplateData(
    napi_env env, napi_value raw_value, napi_value raw_read_only,
    napi_value raw_processor_name) {
  napi_valuetype type;
  napi_typeof(env, raw_value, &type);

  bool from_json = false;
  std::string json_str;
  lepus::Value result;

  if (type == napi_valuetype::napi_string) {
    from_json = true;
    json_str = base::NapiUtil::ConvertToString(env, raw_value);
    result = lepus::jsonValueTolepusValue(json_str.c_str());
  } else if (type == napi_valuetype::napi_object) {
    result = base::NapiConvertHelper::ConvertToLepusValue(env, raw_value);
  } else {
    LOGE(
        "Error in TemplateDataHarmony::GenerateTemplateData, type should be "
        "string or object");
  }

  if (result.IsNil()) {
    return nullptr;
  }

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
    std::promise<lepus::Value> promise;
    std::future<lepus::Value> future = promise.get_future();
    auto async_task = fml::MakeRefCounted<base::OnceTask<lepus::Value>>(
        [result, promise = std::move(promise)]() mutable {
          promise.set_value(lepus::Value::Clone(result));
        },
        std::move(future));

    base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
        [async_task]() { async_task->Run(); },
        base::ConcurrentTaskType::HIGH_PRIORITY);

    data->SetAsyncTask(async_task);
  }

  return data;
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
