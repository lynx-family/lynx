// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_RECORDER_TEMPLATE_ASSEMBLER_RECORDER_H_
#define CORE_SERVICES_RECORDER_TEMPLATE_ASSEMBLER_RECORDER_H_

#include <memory>
#include <string>
#include <vector>

#include "core/renderer/template_assembler.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/services/recorder/recorder_constants.h"
#include "core/services/recorder/testbench_base_recorder.h"
namespace lynx {
namespace tasm {

class TemplateData;
class TemplateAssembler;

namespace recorder {

class TemplateAssemblerRecorder {
 public:
  static void RecordLoadTemplateBundle(
      const std::string& url, const std::vector<uint8_t>& source,
      const std::shared_ptr<TemplateData> template_data, int64_t record_id,
      bool is_csr = true);
  static void RecordLoadTemplate(
      const std::string& url, const std::vector<uint8_t>& source,
      const std::shared_ptr<TemplateData> template_data, int64_t record_id,
      bool is_csr = true, const char* function_name = kFuncLoadTemplate);
  static void RecordReloadTemplate(
      const std::shared_ptr<TemplateData> template_data, int64_t record_id);
  static void RecordSetGlobalProps(lepus::Value global_props,
                                   int64_t record_id);
  static void RecordUpdateMetaData(
      const std::shared_ptr<TemplateData> template_data,
      lepus::Value global_props, int64_t record_id);
  static void RecordUpdateConfig(const lepus::Value& config,
                                 const bool notice_delegate, int64_t record_id);
  static void RecordUpdateFontScale(float scale, const std::string& type,
                                    int64_t record_id);
  static void RecordUpdateDataByPreParsedData(
      const std::shared_ptr<TemplateData> template_data,
      const UpdatePageOption& update_page_option, int64_t record_id);

  static void RecordTouchEvent(std::string name, int root_tag,
                               const EventInfo& info, int64_t record_id);
  static void RecordTouchEvent(std::string name, int tag, int root_tag, float x,
                               float y, float client_x, float client_y,
                               float page_x, float page_y, int64_t record_id);
  static void RecordCustomEvent(std::string name, int tag, int root_tag,
                                const lepus::Value& params, std::string pname,
                                int64_t record_id);
  static void RecordBubbleEvent(std::string name, int tag, int root_tag,
                                const lepus::Value& params, int64_t record_id);

  static void RecordRequireTemplate(const std::string& url, bool sync,
                                    int64_t record_id);

  static void RecordLoadComponentWithCallback(const std::string& url,
                                              std::vector<uint8_t>& source,
                                              bool sync, int32_t callback_id,
                                              int64_t record_id);

 private:
  static void ProcessUpdatePageOption(
      const UpdatePageOption& update_page_option, rapidjson::Value& value);
  static rapidjson::Value CreateJSONFromTemplateData(
      const std::shared_ptr<TemplateData>& template_data);
  static rapidjson::Value CreateJSONFromGlobalProps(lepus::Value global_props);

  static rapidjson::Value CreateJSONFromUpdateConfig(const lepus::Value& config,
                                                     bool notice_delegate);
  static rapidjson::Value CreateJSONFromUpdateFontScale(
      float scale, const std::string& type);
  static rapidjson::Value CreateJSONFromUpdateDataByPreParsedData(
      const std::shared_ptr<TemplateData> template_data,
      const UpdatePageOption& update_page_option);
  static rapidjson::Value CreateJSONFromTouchEvent(std::string name, int tag,
                                                   int root_tag, float x,
                                                   float y, float client_x,
                                                   float client_y, float page_x,
                                                   float page_y);
  static rapidjson::Value CreateJSONFromCustomEvent(std::string name, int tag,
                                                    int root_tag,
                                                    const lepus::Value& params,
                                                    std::string pname);
  static rapidjson::Value CreateJSONFromRequireTemplate(const std::string& url,
                                                        bool sync);
  static rapidjson::Value CreateJSONFromLoadComponentWithCallback(
      const std::string& url, std::vector<uint8_t>& source, bool sync,
      int32_t callback_id);
};

class RecordRequireTemplateScope {
 public:
  RecordRequireTemplateScope(TemplateAssembler* tasm, const std::string& url,
                             int64_t record_id);
  ~RecordRequireTemplateScope();

 private:
  TemplateAssembler* tasm_;
  std::string url_;
  int64_t record_id_;
  bool contain_target_entry_{false};
};

}  // namespace recorder
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_RECORDER_TEMPLATE_ASSEMBLER_RECORDER_H_
