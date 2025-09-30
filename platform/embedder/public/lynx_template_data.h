// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_LYNX_TEMPLATE_DATA_H_
#define PLATFORM_EMBEDDER_PUBLIC_LYNX_TEMPLATE_DATA_H_

#include <string>

#include "capi/lynx_template_data_capi.h"

namespace lynx {
namespace pub {

/**
 * @apidoc
 * @brief TemplateData is a data structure provided by Lynx. It is used to store
 * the data types accepted by Lynx at runtime and can be constructed using
 * either a string or a map type.
 */
class LynxTemplateData {
 public:
  /**
   * @apidoc
   * @brief Constructs a new LynxTemplateData object.
   * @param json The JSON string used to create the template data.
   */
  explicit LynxTemplateData(const std::string& json)
      : template_data_(lynx_template_data_create_from_json(json.c_str())) {}
  ~LynxTemplateData() { lynx_template_data_release(template_data_); }

  /**
   * @apidoc
   * @brief Update the current TemplateData with the JSON string.
   * @param json The JSON string used to update the template data.
   */
  void UpdateWithJson(const std::string& json) {
    if (read_only_) {
      // If the data is read only, we can't update it.
      return;
    }
    if (template_data_) {
      lynx_template_data_release(template_data_);
    }
    template_data_ = lynx_template_data_create_from_json(json.c_str());
    if (!processor_name_.empty()) {
      lynx_template_data_mark_state(template_data_, processor_name_.c_str());
    }
  }

  /**
   * @apidoc
   * @brief Mark the current TemplateData with the associated dataProcessor
   * name.
   * @param name Marked name.
   */
  void MarkState(const std::string& name) {
    processor_name_ = name;
    lynx_template_data_mark_state(template_data_, name.c_str());
  }

  /**
   * @apidoc
   * @brief Mark the current TemplateData as read-only. TemplateData will be
   * sync to Native. For Thread-Safety, we will clone the value in Native Side.
   * In some case, this may result in performance-loss, If your data won't
   * change any more, Please call this method to mark value Read-Only, so we'll
   * no longer clone the value any more to improve performance.
   */
  void MarkReadOnly() {
    read_only_ = true;
    lynx_template_data_set_read_only(template_data_, read_only_);
  }

  /**
   * @apidoc
   * @brief Check if the current TemplateData is read-only.
   * @return True if the current TemplateData is read-only, false otherwise.
   */
  bool IsReadOnly() const { return read_only_; }

  /**
   * @apidoc
   * @brief Get the name of the dataProcessor associated with the current
   * TemplateData.
   * @return The name of the dataProcessor associated with the current
   * TemplateData.
   */
  std::string GetProcessorName() const { return processor_name_; }

  lynx_template_data_t* Impl() { return template_data_; }

  LynxTemplateData(const LynxTemplateData&) = delete;
  LynxTemplateData& operator=(const LynxTemplateData&) = delete;

 private:
  lynx_template_data_t* template_data_;
  std::string processor_name_;
  bool read_only_ = false;
};

}  // namespace pub
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_PUBLIC_LYNX_TEMPLATE_DATA_H_
