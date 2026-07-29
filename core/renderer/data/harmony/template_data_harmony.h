// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DATA_HARMONY_TEMPLATE_DATA_HARMONY_H_
#define CORE_RENDERER_DATA_HARMONY_TEMPLATE_DATA_HARMONY_H_

#include <node_api.h>

#include <memory>
#include <string>
#include <utility>

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/value/base_value.h"
#include "core/base/thread/once_task.h"
#include "core/renderer/data/template_data.h"

namespace lynx {
namespace tasm {

class TemplateData;

class NativeTemplateDataHarmony {
 public:
  static napi_value Init(napi_env env, napi_value exports);
  static napi_value New(napi_env env, napi_callback_info info);
  static NativeTemplateDataHarmony* FromNapiValue(napi_env env,
                                                  napi_value value);

  NativeTemplateDataHarmony(lepus::Value value, bool from_json,
                            std::string json_string)
      : value_(std::move(value)),
        from_json_(from_json),
        json_string_(std::move(json_string)) {}
  ~NativeTemplateDataHarmony() = default;

  const lepus::Value& GetValue() const { return value_; }
  bool IsFromJson() const { return from_json_; }
  const std::string& GetJsonString() const { return json_string_; }

 private:
  static napi_ref constructor_ref_;

  lepus::Value value_;
  bool from_json_{false};
  std::string json_string_;
};

class TemplateDataHarmony : public TemplateData {
 public:
  static std::shared_ptr<tasm::TemplateData> GenerateTemplateData(
      napi_env env, napi_value obj, napi_value raw_read_only,
      napi_value raw_processor_name);
  static lepus::Value GenerateLepusValue(napi_env env, napi_value obj);

  TemplateDataHarmony() = default;
  ~TemplateDataHarmony() override = default;

  TemplateDataHarmony(const lepus::Value& value, bool read_only,
                      std::string preprocessorName)
      : TemplateData(value, read_only, preprocessorName) {}

  TemplateDataHarmony(const lepus::Value& value, bool read_only)
      : TemplateData(value, read_only) {}

  virtual const lepus::Value& GetValue() const override;

  // Will be called when execute CopyPlatformData
  virtual std::unique_ptr<PlatformData> ObtainPlatformData() override;

  void SetAsyncTask(
      const fml::RefPtr<base::OnceTask<lepus::Value>>& async_task) {
    async_task_ = async_task;
  }

 private:
  void EnsurePlatformData();

  fml::RefPtr<base::OnceTask<lepus::Value>> async_task_{nullptr};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DATA_HARMONY_TEMPLATE_DATA_HARMONY_H_
