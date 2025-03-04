// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DATA_TEMPLATE_DATA_H_
#define CORE_RENDERER_DATA_TEMPLATE_DATA_H_

#include <memory>
#include <string>
#include <utility>

#include "core/renderer/data/platform_data.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/table.h"

namespace lynx {

namespace lepus {
class Value;
}

namespace tasm {

class TemplateData {
 public:
  // Return a new Template Data. If other has platform data, obtain the platform
  // data from other and set value_ to empty. If other does not have platform
  // data, deep copy other's value_ and assign it to its own value_.
  static TemplateData CopyPlatformData(
      const std::shared_ptr<TemplateData>& other);
  static TemplateData CopyPlatformData(const TemplateData& other);
  static TemplateData ShallowCopy(const TemplateData& other);
  static TemplateData DeepClone(const TemplateData& other);

  TemplateData() = default;
  virtual ~TemplateData() = default;

  // make TemplateData move only
  TemplateData(const TemplateData&) = delete;
  TemplateData& operator=(const TemplateData&) = delete;
  TemplateData(TemplateData&&) = default;
  TemplateData& operator=(TemplateData&&) = default;

  TemplateData(const lepus::Value& value, bool read_only,
               std::string preprocessorName);
  TemplateData(const lepus::Value& value, bool read_only);

  void SetValue(const lepus::Value& value);

  // value() only used in ProcessInitData & ProcessTemplateDataForRadon now, it
  // does not invole platform_data_.
  lepus::Value& value() { return value_; }

  // When calling GetValue, if value_ is empty, it attempts to obtain the value
  // from platform data.
  virtual const lepus::Value& GetValue() const;

  void SetPreprocessorName(const std::string& name);
  const std::string& PreprocessorName() const;

  void SetReadOnly(bool flag) { read_only_ = flag; }
  bool IsReadOnly() const;

  void CloneValue();

  void SetPlatformData(std::unique_ptr<PlatformData> data) {
    platform_data_ = std::move(data);
  }

  // Will be called when execute CopyPlatformData
  virtual std::unique_ptr<PlatformData> ObtainPlatformData() {
    return std::move(platform_data_);
  }

 protected:
  lepus::Value value_;
  std::string processor_name_;
  bool read_only_{false};

  std::unique_ptr<PlatformData> platform_data_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DATA_TEMPLATE_DATA_H_
