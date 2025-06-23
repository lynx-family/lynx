// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DATA_HARMONY_TEMPLATE_DATA_HARMONY_H_
#define CORE_RENDERER_DATA_HARMONY_TEMPLATE_DATA_HARMONY_H_

#include <node_api.h>

#include <memory>
#include <string>

#include "base/include/fml/memory/ref_ptr.h"
#include "core/base/thread/once_task.h"
#include "core/renderer/data/template_data.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {

class TemplateData;

class TemplateDataHarmony : public TemplateData {
 public:
  static std::shared_ptr<tasm::TemplateData> GenerateTemplateData(
      napi_env env, napi_value obj, napi_value raw_read_only,
      napi_value raw_processor_name);

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
