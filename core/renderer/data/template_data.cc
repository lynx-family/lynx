// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/data/template_data.h"

#include <utility>

namespace lynx {
namespace tasm {

TemplateData TemplateData::CopyPlatformData(
    const std::shared_ptr<TemplateData>& other) {
  if (other == nullptr) {
    return TemplateData();
  }
  return CopyPlatformData(*other);
}

TemplateData TemplateData::CopyPlatformData(const TemplateData& other) {
  TemplateData data(lepus::Value(), other.IsReadOnly(),
                    other.PreprocessorName());
  // When ReloadFromJS or lepus component execute updateData, there will be no
  // platform data. Therefore, if other.platform_data_ != nullptr, obtain
  // platform_data_ from other. Otherwise, deep clone other's value.
  if (other.platform_data_ != nullptr) {
    data.SetPlatformData(const_cast<TemplateData&>(other).ObtainPlatformData());
  } else {
    data.SetValue(lepus::Value::Clone(other.GetValue()));
  }
  return data;
}

TemplateData TemplateData::ShallowCopy(const TemplateData& other) {
  return TemplateData(lepus::Value::ShallowCopy(other.GetValue()),
                      other.IsReadOnly(), other.PreprocessorName());
}

TemplateData TemplateData::DeepClone(const TemplateData& other) {
  return TemplateData(lepus::Value::Clone(other.GetValue()), other.IsReadOnly(),
                      other.PreprocessorName());
}

TemplateData::TemplateData(const lepus::Value& value, bool read_only,
                           std::string name)
    : value_(value), processor_name_(std::move(name)), read_only_(read_only) {}

TemplateData::TemplateData(const lepus::Value& value, bool read_only)
    : value_(value), read_only_(read_only) {}

void TemplateData::SetValue(const lepus::Value& value) { value_ = value; }

const lepus::Value& TemplateData::GetValue() const {
  if (value_.IsEmpty() && platform_data_ != nullptr) {
    return platform_data_->GetValue();
  }
  return value_;
}

void TemplateData::SetPreprocessorName(const std::string& name) {
  processor_name_ = name;
}

const std::string& TemplateData::PreprocessorName() const {
  return processor_name_;
}

bool TemplateData::IsReadOnly() const { return read_only_; }

void TemplateData::CloneValue() { value_ = lynx::lepus::Value::Clone(value_); }

}  // namespace tasm
}  // namespace lynx
