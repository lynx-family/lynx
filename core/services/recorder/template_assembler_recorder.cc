// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/recorder/template_assembler_recorder.h"

#include <cstddef>

#include "core/renderer/data/template_data.h"
#include "core/runtime/vm/lepus/json_parser.h"
#include "third_party/modp_b64/modp_b64.h"
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/rapidjson.h"

namespace lynx {
namespace tasm {
namespace recorder {

// Template_Assembler Func

void TemplateAssemblerRecorder::RecordLoadTemplate(
    const std::string& url, const std::vector<uint8_t>& source,
    const std::shared_ptr<TemplateData> template_data, int64_t record_id,
    bool is_csr, const char* function_name) {
  if (!TestBenchBaseRecorder::GetInstance().IsRecordingProcess()) {
    return;
  }
  rapidjson::Document::AllocatorType& allocator =
      TestBenchBaseRecorder::GetInstance().GetAllocator();
  rapidjson::Value url_val(rapidjson::kStringType);
  auto url_size = static_cast<uint32_t>(url.size());
  url_val.SetString(url.c_str(), url_size, allocator);

  auto source_size = static_cast<uint32_t>(source.size());

  const char* buff = reinterpret_cast<const char*>(source.data());
  size_t encode_length = modp_b64_encode_len(source_size);
  std::unique_ptr<char[]> encode_buff = std::make_unique<char[]>(encode_length);
  encode_length = modp_b64_encode(encode_buff.get(), buff, source_size);

  rapidjson::Value source_val(rapidjson::kStringType);
  source_val.SetString(encode_buff.get(),
                       static_cast<rapidjson::SizeType>(encode_length),
                       allocator);

  // Parse template_data
  rapidjson::Value template_data_val(rapidjson::kObjectType);
  if (template_data != nullptr) {
    rapidjson::Document template_data_doc(rapidjson::kObjectType);
    template_data_doc.Parse(
        lepus::lepusValueToString(template_data.get()->GetValue()));
    template_data_val.CopyFrom(template_data_doc, allocator);

    rapidjson::Value params_preprocessor_name_val(rapidjson::kStringType);
    params_preprocessor_name_val.SetString(
        template_data.get()->PreprocessorName(), allocator);
    rapidjson::Value params_read_only_val;
    params_read_only_val.SetBool(template_data.get()->IsReadOnly());

    template_data_val.AddMember(rapidjson::StringRef(kParamPreprocessorName),
                                params_preprocessor_name_val, allocator);
    template_data_val.AddMember(rapidjson::StringRef(kParamReadOnly),
                                params_read_only_val, allocator);
  }

  rapidjson::Value params_val(rapidjson::kObjectType);
  params_val.AddMember(rapidjson::StringRef(kParamUrl), url_val, allocator);
  params_val.AddMember(rapidjson::StringRef(kParamSource), source_val,
                       allocator);
  params_val.AddMember(rapidjson::StringRef(kParamTemplateData),
                       template_data_val, allocator);

  rapidjson::Value is_csr_val;
  is_csr_val.SetBool(is_csr);
  params_val.AddMember(rapidjson::StringRef(kParamIsCSR), is_csr, allocator);

  TestBenchBaseRecorder::GetInstance().RecordAction(function_name, params_val,
                                                    record_id);
}

void TemplateAssemblerRecorder::RecordLoadTemplateBundle(
    const std::string& url, const std::vector<uint8_t>& source,
    const std::shared_ptr<TemplateData> template_data, int64_t record_id,
    bool is_csr) {
  TemplateAssemblerRecorder::RecordLoadTemplate(
      url, source, template_data, record_id, true, kFuncLoadTemplateBundle);
}

rapidjson::Value TemplateAssemblerRecorder::CreateJSONFromTemplateData(
    const std::shared_ptr<TemplateData>& template_data) {
  if (template_data == nullptr) {
    return rapidjson::Value(rapidjson::kNullType);
  }
  rapidjson::Document::AllocatorType& allocator =
      TestBenchBaseRecorder::GetInstance().GetAllocator();
  rapidjson::Document value_parser_doc;
  value_parser_doc.Parse(
      lepus::lepusValueToString(template_data.get()->GetValue()));

  rapidjson::Value params_value_val(rapidjson::kObjectType);
  params_value_val.CopyFrom(value_parser_doc, allocator);

  rapidjson::Value params_preprocessor_name_val(
      template_data.get()->PreprocessorName(), allocator);
  rapidjson::Value params_read_only_val;
  params_read_only_val.SetBool(template_data.get()->IsReadOnly());

  rapidjson::Value params_val(rapidjson::kObjectType);
  params_val.AddMember(rapidjson::StringRef(kParamValue), params_value_val,
                       allocator);
  params_val.AddMember(rapidjson::StringRef(kParamPreprocessorName),
                       params_preprocessor_name_val, allocator);
  params_val.AddMember(rapidjson::StringRef(kParamReadOnly),
                       params_read_only_val, allocator);
  return params_val;
}

rapidjson::Value TemplateAssemblerRecorder::CreateJSONFromGlobalProps(
    lepus::Value global_props) {
  rapidjson::Document::AllocatorType& allocator =
      TestBenchBaseRecorder::GetInstance().GetAllocator();
  rapidjson::Document global_props_parser_doc;
  global_props_parser_doc.Parse(lepus::lepusValueToString(global_props));

  rapidjson::Value params_global_props_val(rapidjson::kObjectType);
  params_global_props_val.CopyFrom(global_props_parser_doc, allocator);

  return params_global_props_val;
}

void TemplateAssemblerRecorder::RecordReloadTemplate(
    const std::shared_ptr<TemplateData> template_data, int64_t record_id) {
  if (!TestBenchBaseRecorder::GetInstance().IsRecordingProcess()) {
    return;
  }
  rapidjson::Value params_val = CreateJSONFromTemplateData(template_data);
  TestBenchBaseRecorder::GetInstance().RecordAction(kFuncRecordReloadTemplate,
                                                    params_val, record_id);
}

void TemplateAssemblerRecorder::RecordSetGlobalProps(lepus::Value global_props,
                                                     int64_t record_id) {
  if (!TestBenchBaseRecorder::GetInstance().IsRecordingProcess()) {
    return;
  }
  rapidjson::Document::AllocatorType& allocator =
      TestBenchBaseRecorder::GetInstance().GetAllocator();
  rapidjson::Value params_global_props_val =
      CreateJSONFromGlobalProps(global_props);
  rapidjson::Value params_val(rapidjson::kObjectType);
  params_val.AddMember(rapidjson::StringRef(kParamGlobalProps),
                       params_global_props_val, allocator);

  TestBenchBaseRecorder::GetInstance().RecordAction(kFuncSetGlobalProps,
                                                    params_val, record_id);
}

void TemplateAssemblerRecorder::RecordUpdateMetaData(
    const std::shared_ptr<TemplateData> template_data,
    lepus::Value global_props, int64_t record_id) {
  if (!TestBenchBaseRecorder::GetInstance().IsRecordingProcess()) {
    return;
  }
  rapidjson::Document::AllocatorType& allocator =
      TestBenchBaseRecorder::GetInstance().GetAllocator();
  rapidjson::Value template_data_json =
      CreateJSONFromTemplateData(template_data);
  rapidjson::Value global_props_json = CreateJSONFromGlobalProps(global_props);

  rapidjson::Value params_val(rapidjson::kObjectType);
  params_val.AddMember(rapidjson::StringRef(kParamGlobalProps),
                       global_props_json, allocator);

  params_val.AddMember(rapidjson::StringRef(kParamTemplateData),
                       template_data_json, allocator);

  TestBenchBaseRecorder::GetInstance().RecordAction(kFuncRecordUpdateMetaData,
                                                    params_val, record_id);
}

rapidjson::Value TemplateAssemblerRecorder::CreateJSONFromUpdateConfig(
    const lepus::Value& config, bool notice_delegate) {
  rapidjson::Document::AllocatorType& allocator =
      TestBenchBaseRecorder::GetInstance().GetAllocator();
  rapidjson::Document config_parser_doc;
  config_parser_doc.Parse(lepus::lepusValueToString(config));

  rapidjson::Value params_config_val(rapidjson::kObjectType);
  params_config_val.CopyFrom(config_parser_doc, allocator);
  rapidjson::Value params_notice_delegate_val(notice_delegate);

  rapidjson::Value params_val(rapidjson::kObjectType);
  params_val.AddMember(rapidjson::StringRef(kParamConfig), params_config_val,
                       allocator);
  params_val.AddMember(rapidjson::StringRef(kParamNoticeDelegate),
                       params_notice_delegate_val, allocator);
  return params_val;
}

void TemplateAssemblerRecorder::RecordUpdateConfig(const lepus::Value& config,
                                                   bool notice_delegate,
                                                   int64_t record_id) {
  if (!TestBenchBaseRecorder::GetInstance().IsRecordingProcess()) {
    return;
  }
  rapidjson::Value params_val =
      CreateJSONFromUpdateConfig(config, notice_delegate);
  TestBenchBaseRecorder::GetInstance().RecordAction(kFuncUpdateConfig,
                                                    params_val, record_id);
}

rapidjson::Value TemplateAssemblerRecorder::CreateJSONFromUpdateFontScale(
    float scale, const std::string& type) {
  rapidjson::Document::AllocatorType& allocator =
      TestBenchBaseRecorder::GetInstance().GetAllocator();

  rapidjson::Value params_val(rapidjson::kObjectType);
  params_val.AddMember(rapidjson::StringRef(kFontScale), scale, allocator);

  rapidjson::Value func_type;
  func_type.SetString(type.c_str(), static_cast<int>(type.length()), allocator);

  params_val.AddMember(rapidjson::StringRef(kFuncUpdateFontScaleType),
                       func_type, allocator);
  return params_val;
}

void TemplateAssemblerRecorder::RecordUpdateFontScale(float scale,
                                                      const std::string& type,
                                                      int64_t record_id) {
  if (!TestBenchBaseRecorder::GetInstance().IsRecordingProcess()) {
    return;
  }
  rapidjson::Value params_val = CreateJSONFromUpdateFontScale(scale, type);
  TestBenchBaseRecorder::GetInstance().RecordAction(kFuncUpdateFontScale,
                                                    params_val, record_id);
}

void TemplateAssemblerRecorder::ProcessUpdatePageOption(
    const UpdatePageOption& update_page_option, rapidjson::Value& value) {
  rapidjson::Document::AllocatorType& allocator =
      TestBenchBaseRecorder::GetInstance().GetAllocator();
  value.AddMember("from_native", update_page_option.from_native, allocator);
  value.AddMember("reset_page_data", update_page_option.reset_page_data,
                  allocator);
  value.AddMember("update_first_time", update_page_option.update_first_time,
                  allocator);
  value.AddMember("reload_template", update_page_option.reload_template,
                  allocator);
  value.AddMember("global_props_changed",
                  update_page_option.global_props_changed, allocator);
  value.AddMember("reload_from_js", update_page_option.reload_from_js,
                  allocator);
  value.AddMember("native_update_data_order_",
                  update_page_option.native_update_data_order_, allocator);
}

rapidjson::Value
TemplateAssemblerRecorder::CreateJSONFromUpdateDataByPreParsedData(
    const std::shared_ptr<TemplateData> template_data,
    const UpdatePageOption& update_page_option) {
  rapidjson::Document::AllocatorType& allocator =
      TestBenchBaseRecorder::GetInstance().GetAllocator();
  rapidjson::Document value_parser_doc;
  value_parser_doc.Parse(
      lepus::lepusValueToString(template_data.get()->GetValue()));

  rapidjson::Value params_value_val(rapidjson::kObjectType);
  params_value_val.CopyFrom(value_parser_doc, allocator);

  rapidjson::Value params_preprocessor_name_val(
      template_data.get()->PreprocessorName(), allocator);
  rapidjson::Value params_val(rapidjson::kObjectType);

  rapidjson::Value params_read_only_val;
  params_read_only_val.SetBool(template_data.get()->IsReadOnly());

  rapidjson::Value params_page_option(rapidjson::kObjectType);
  ProcessUpdatePageOption(update_page_option, params_page_option);

  params_val.AddMember(rapidjson::StringRef(kParamValue), params_value_val,
                       allocator);
  params_val.AddMember(rapidjson::StringRef(kParamPreprocessorName),
                       params_preprocessor_name_val, allocator);
  params_val.AddMember(rapidjson::StringRef(kUpdatePageOption),
                       params_page_option, allocator);
  params_val.AddMember(rapidjson::StringRef(kParamReadOnly),
                       params_read_only_val, allocator);
  return params_val;
}

void TemplateAssemblerRecorder::RecordUpdateDataByPreParsedData(
    const std::shared_ptr<TemplateData> template_data,
    const UpdatePageOption& update_page_option, int64_t record_id) {
  if (!TestBenchBaseRecorder::GetInstance().IsRecordingProcess()) {
    return;
  }
  rapidjson::Value params_val = CreateJSONFromUpdateDataByPreParsedData(
      template_data, update_page_option);
  TestBenchBaseRecorder::GetInstance().RecordAction(
      kFuncUpdateDataByPreParsedData, params_val, record_id);
}

void TemplateAssemblerRecorder::RecordTouchEvent(std::string name, int root_tag,
                                                 const EventInfo& info,
                                                 int64_t record_id) {
  if (info.is_multi_finger) {
    // TODO(jiyishen): record multi touch info
  } else {
    RecordTouchEvent(name, info.tag, root_tag, info.x, info.y, info.client_x,
                     info.client_y, info.page_x, info.page_y, record_id);
  }
}

rapidjson::Value TemplateAssemblerRecorder::CreateJSONFromTouchEvent(
    std::string name, int tag, int root_tag, float x, float y, float client_x,
    float client_y, float page_x, float page_y) {
  rapidjson::Document::AllocatorType& allocator =
      TestBenchBaseRecorder::GetInstance().GetAllocator();
  rapidjson::Value params_val(rapidjson::kObjectType);

  rapidjson::Value event_name;
  event_name.SetString(name.c_str(), static_cast<int>(name.length()),
                       allocator);
  params_val.AddMember(rapidjson::StringRef(kEventName), event_name, allocator);
  params_val.AddMember(rapidjson::StringRef(kEventTag), rapidjson::Value(tag),
                       allocator);
  params_val.AddMember(rapidjson::StringRef(kEventRootTag),
                       rapidjson::Value(root_tag), allocator);
  params_val.AddMember(rapidjson::StringRef(kEventX), rapidjson::Value(x),
                       allocator);
  params_val.AddMember(rapidjson::StringRef(kEventY), rapidjson::Value(y),
                       allocator);
  params_val.AddMember(rapidjson::StringRef(kEventClientX),
                       rapidjson::Value(client_x), allocator);
  params_val.AddMember(rapidjson::StringRef(kEventClientY),
                       rapidjson::Value(client_y), allocator);
  params_val.AddMember(rapidjson::StringRef(kEventPageX),
                       rapidjson::Value(page_x), allocator);
  params_val.AddMember(rapidjson::StringRef(kEventPageY),
                       rapidjson::Value(page_y), allocator);
  return params_val;
}

void TemplateAssemblerRecorder::RecordTouchEvent(std::string name, int tag,
                                                 int root_tag, float x, float y,
                                                 float client_x, float client_y,
                                                 float page_x, float page_y,
                                                 int64_t record_id) {
  if (!TestBenchBaseRecorder::GetInstance().IsRecordingProcess()) {
    return;
  }
  rapidjson::Value params_val = CreateJSONFromTouchEvent(
      name, tag, root_tag, x, y, client_x, client_y, page_x, page_y);
  TestBenchBaseRecorder::GetInstance().RecordAction(kFuncSendTouchEvent,
                                                    params_val, record_id);
}

rapidjson::Value TemplateAssemblerRecorder::CreateJSONFromCustomEvent(
    std::string name, int tag, int root_tag, const lepus::Value& params,
    std::string pname) {
  rapidjson::Document::AllocatorType& allocator =
      TestBenchBaseRecorder::GetInstance().GetAllocator();
  rapidjson::Value params_val(rapidjson::kObjectType);

  params_val.AddMember(rapidjson::StringRef(kEventTag), rapidjson::Value(tag),
                       allocator);
  params_val.AddMember(rapidjson::StringRef(kEventRootTag),
                       rapidjson::Value(root_tag), allocator);
  rapidjson::Value event_name;
  event_name.SetString(name.c_str(), static_cast<int>(name.length()),
                       allocator);
  params_val.AddMember(rapidjson::StringRef(kEventName), event_name, allocator);
  rapidjson::Value p_name;
  p_name.SetString(pname.c_str(), static_cast<int>(pname.length()), allocator);
  params_val.AddMember(rapidjson::StringRef(kEventParaName), p_name, allocator);

  rapidjson::Document params_parser_doc;
  params_parser_doc.Parse(lepus::lepusValueToString(params));
  rapidjson::Value key_params_val(rapidjson::kObjectType);
  key_params_val.CopyFrom(params_parser_doc, allocator);
  params_val.AddMember(rapidjson::StringRef(kEventParams), key_params_val,
                       allocator);
  return params_val;
}

void TemplateAssemblerRecorder::RecordCustomEvent(std::string name, int tag,
                                                  int root_tag,
                                                  const lepus::Value& params,
                                                  std::string pname,
                                                  int64_t record_id) {
  if (!TestBenchBaseRecorder::GetInstance().IsRecordingProcess()) {
    return;
  }
  rapidjson::Value params_val =
      CreateJSONFromCustomEvent(name, tag, root_tag, params, pname);
  TestBenchBaseRecorder::GetInstance().RecordAction(kFuncSendCustomEvent,
                                                    params_val, record_id);
}

void TemplateAssemblerRecorder::RecordBubbleEvent(std::string name, int tag,
                                                  int root_tag,
                                                  const lepus::Value& params,
                                                  int64_t record_id) {
  if (!TestBenchBaseRecorder::GetInstance().IsRecordingProcess()) {
    return;
  }
  rapidjson::Document::AllocatorType& allocator =
      TestBenchBaseRecorder::GetInstance().GetAllocator();
  rapidjson::Value params_val(rapidjson::kObjectType);

  params_val.AddMember(rapidjson::StringRef(kEventTag), rapidjson::Value(tag),
                       allocator);
  params_val.AddMember(rapidjson::StringRef(kEventRootTag),
                       rapidjson::Value(root_tag), allocator);
  rapidjson::Value event_name;
  event_name.SetString(name.c_str(), static_cast<int>(name.length()),
                       allocator);
  params_val.AddMember(rapidjson::StringRef(kEventName), event_name, allocator);

  rapidjson::Document params_parser_doc;
  params_parser_doc.Parse(lepus::lepusValueToString(params));
  rapidjson::Value key_params_val(rapidjson::kObjectType);
  key_params_val.CopyFrom(params_parser_doc, allocator);
  params_val.AddMember(rapidjson::StringRef(kEventParams), key_params_val,
                       allocator);

  TestBenchBaseRecorder::GetInstance().RecordAction(kFuncSendBubbleEvent,
                                                    params_val, record_id);
}

rapidjson::Value TemplateAssemblerRecorder::CreateJSONFromRequireTemplate(
    const std::string& url, bool sync) {
  auto& allocator = TestBenchBaseRecorder::GetInstance().GetAllocator();

  rapidjson::Value url_val;
  url_val.SetString(url.c_str(), static_cast<int>(url.length()), allocator);

  rapidjson::Value params_val(rapidjson::kObjectType);
  params_val.AddMember(rapidjson::StringRef(kParamUrl), url_val, allocator);
  params_val.AddMember(rapidjson::StringRef(kSyncTag), rapidjson::Value(sync),
                       allocator);
  return params_val;
}

void TemplateAssemblerRecorder::RecordRequireTemplate(const std::string& url,
                                                      bool sync,
                                                      int64_t record_id) {
  if (!TestBenchBaseRecorder::GetInstance().IsRecordingProcess()) {
    return;
  }
  rapidjson::Value params_val = CreateJSONFromRequireTemplate(url, sync);
  TestBenchBaseRecorder::GetInstance().RecordAction(kFuncRequireTemplate,
                                                    params_val, record_id);
}

rapidjson::Value
TemplateAssemblerRecorder::CreateJSONFromLoadComponentWithCallback(
    const std::string& url, std::vector<uint8_t>& source, bool sync,
    int32_t callback_id) {
  auto& allocator = TestBenchBaseRecorder::GetInstance().GetAllocator();
  rapidjson::Value url_val(rapidjson::kStringType);
  auto url_size = static_cast<uint32_t>(url.size());
  url_val.SetString(url.c_str(), url_size, allocator);

  auto source_size = static_cast<uint32_t>(source.size());
  const char* buff = reinterpret_cast<char*>(source.data());
  size_t encode_length = modp_b64_encode_len(source_size);
  std::unique_ptr<char[]> encode_buff = std::make_unique<char[]>(encode_length);
  encode_length = modp_b64_encode(encode_buff.get(), buff, source_size);
  rapidjson::Value source_val(rapidjson::kStringType);
  source_val.SetString(encode_buff.get(),
                       static_cast<rapidjson::SizeType>(encode_length),
                       allocator);

  rapidjson::Value params_val(rapidjson::kObjectType);
  params_val.AddMember(rapidjson::StringRef(kParamUrl), url_val, allocator);
  params_val.AddMember(rapidjson::StringRef(kParamSource), source_val,
                       allocator);
  params_val.AddMember(rapidjson::StringRef(kSyncTag), rapidjson::Value(sync),
                       allocator);
  params_val.AddMember(rapidjson::StringRef(kCallbackId),
                       rapidjson::Value(callback_id), allocator);
  return params_val;
}

void TemplateAssemblerRecorder::RecordLoadComponentWithCallback(
    const std::string& url, std::vector<uint8_t>& source, bool sync,
    int32_t callback_id, int64_t record_id) {
  if (!TestBenchBaseRecorder::GetInstance().IsRecordingProcess()) {
    return;
  }
  rapidjson::Value params_val =
      CreateJSONFromLoadComponentWithCallback(url, source, sync, callback_id);
  TestBenchBaseRecorder::GetInstance().RecordAction(
      kFuncLoadComponentWithCallback, params_val, record_id);
}

RecordRequireTemplateScope::RecordRequireTemplateScope(TemplateAssembler* tasm,
                                                       const std::string& url,
                                                       int64_t record_id)
    : tasm_(tasm), url_(url), record_id_(record_id) {
  contain_target_entry_ = tasm_->FindTemplateEntry(url) != nullptr;
}

RecordRequireTemplateScope::~RecordRequireTemplateScope() {
  // We must check the containment twice,
  // because if the requirement is sync or async is controlled by app,
  // we cannot get it directly.
  if (!contain_target_entry_) {
    // If tasm does not contain target entry, it must send a requirement.
    TemplateAssemblerRecorder::RecordRequireTemplate(
        url_, tasm_->FindTemplateEntry(url_) != nullptr, record_id_);
    // If tasm contains target entry at this time, it means the requirement is
    // sync, else the requirement is async.
  }
}

}  // namespace recorder
}  // namespace tasm
}  // namespace lynx
