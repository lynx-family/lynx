// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "testing/lynx/tasm/databinding/data_update_replayer.h"

#include <utility>
#include <vector>

#include "core/renderer/data/template_data.h"
#include "core/renderer/dom/vdom/radon/radon_list_base.h"
#include "core/renderer/utils/base/base_def.h"
#include "core/runtime/vm/lepus/json_parser.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "testing/lynx/tasm/databinding/databinding_test.h"
#include "testing/lynx/tasm/databinding/mock_replayer_component_loader.h"
#include "third_party/modp_b64/modp_b64.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace tasm {
namespace test {

DataUpdateReplayer::DataUpdateReplayer(std::shared_ptr<TemplateAssembler> tasm)
    : weak_tasm_(tasm) {}

bool DataUpdateReplayer::CaseInsensitiveStringComparison(
    const std::string& left, const char* right) {
  if (left.size() != strlen(right)) {
    return false;
  }

  bool res = true;
  const char* right_c = right;
  uint32_t left_index = 0;
  const char* left_c = &left[left_index];

  while (*right_c++ != '\0' && left_index++ < left.size()) {
    left_c = &left[left_index];
    res = res && CharComparison(left_c, right_c);
  }

  return res;
}

bool DataUpdateReplayer::CharComparison(const char* c1, const char* c2) {
  if (*c1 == *c2) {
    return true;
  } else if (std::toupper(*c1) == std::toupper(*c2)) {
    return true;
  }
  return false;
}

void DataUpdateReplayer::DataUpdateReplay(const std::string& replay_data,
                                          bool use_ark_source) {
  auto tasm = weak_tasm_.lock();
  if (tasm == nullptr) {
    return;
  }
  rapidjson::Document replay_doc;
  replay_doc.Parse(replay_data.c_str());
  rapidjson::Value func_list;
  if (replay_doc.HasMember(kFunctionCallArray)) {
    func_list = replay_doc[kFunctionCallArray];
  } else if (replay_doc.HasMember(kActionList)) {
    func_list = replay_doc[kActionList];
  }
  EXPECT_TRUE(func_list.IsArray());

  auto loader = std::make_shared<MockReplayerComponentLoader>(weak_tasm_);
  loader->InitWithActionList(func_list);
  auto engine_actor_ = std::make_shared<shell::LynxActor<shell::LynxEngine>>(
      std::make_unique<shell::LynxEngine>(tasm, nullptr, nullptr,
                                          shell::kUnknownInstanceId),
      fml::MakeRefCounted<MockTasmRunner>());
  loader->SetEngineActor(engine_actor_);

  tasm->SetLazyBundleLoader(loader);

  for (auto iter = func_list.Begin(); iter != func_list.End(); ++iter) {
    EXPECT_TRUE(iter->HasMember(kFunctionName));

    const std::string function_name = (*iter)[kFunctionName].GetString();
    EXPECT_TRUE(iter->HasMember(kParams));

    const rapidjson::Value& params_val = (*iter)[kParams];
    EXPECT_TRUE(params_val.IsObject());

    if (CaseInsensitiveStringComparison(function_name, kFuncLoadTemplate) &&
        params_val.HasMember(kParaUrl) && params_val.HasMember(kParaSource) &&
        params_val.HasMember(kParaTemplateData) && use_ark_source) {
      EXPECT_TRUE(params_val[kParaTemplateData].IsObject());
      lepus::Value data_val =
          lepus::jsonValueTolepusValue(params_val[kParaTemplateData]);

      EXPECT_TRUE(
          data_val.IsTable() &&
          data_val.Table()->Contains(kParamPreprocessorName) &&
          data_val.Table()->GetValue(kParamPreprocessorName).IsString());
      std::string preprocessorName_val =
          data_val.Table()->GetValue(kParamPreprocessorName).StdString();

      std::shared_ptr<tasm::TemplateData> template_data_ptr =
          std::make_shared<tasm::TemplateData>(
              TemplateData(data_val, false, preprocessorName_val));

      EXPECT_TRUE(params_val[kParaSource].IsString());
      std::string source = params_val[kParaSource].GetString();
      const auto& src = modp_b64_decode(source);
      std::vector<uint8_t> input_source(src.begin(), src.end());

      EXPECT_TRUE(params_val[kParaUrl].IsString());
      const auto& url = params_val[kParaUrl].GetString();
      PipelineOptions pipeline_options;
      tasm->LoadTemplate(url, input_source, template_data_ptr,
                         pipeline_options);

    } else if (CaseInsensitiveStringComparison(function_name,
                                               kFuncUpdateDataByJS) &&
               params_val.HasMember(kParamData)) {
      EXPECT_TRUE(params_val[kParamData].IsObject());
      lepus::Value data_val =
          lepus::jsonValueTolepusValue(params_val[kParamData]);
      const std::string PAGE_GROUP_ID = "-1";
      PipelineOptions pipeline_options;
      runtime::UpdateDataTask task(
          true, PAGE_GROUP_ID, std::move(data_val), piper::ApiCallBack(),
          runtime::UpdateDataType(), std::move(pipeline_options));
      tasm->UpdateDataByJS(task, task.pipeline_options_);
    } else if (CaseInsensitiveStringComparison(function_name,
                                               kFuncUpdateConfig) &&
               params_val.HasMember(kParamConfig) &&
               params_val.HasMember(kParamNoticeDelegate)) {
      EXPECT_TRUE(params_val[kParamConfig].IsObject());
      lepus::Value config_val =
          lepus::jsonValueTolepusValue(params_val[kParamConfig]);

      EXPECT_TRUE(params_val[kParamNoticeDelegate].IsBool());
      bool noticeDelegate_val = params_val[kParamNoticeDelegate].GetBool();
      PipelineOptions pipeline_options;
      tasm->UpdateConfig(config_val, noticeDelegate_val, pipeline_options);

    } else if (CaseInsensitiveStringComparison(function_name,
                                               kFuncSetGlobalProps) &&
               params_val.HasMember(kParamGlobalProps)) {
      EXPECT_TRUE(params_val[kParamGlobalProps].IsObject());
      lepus::Value global_props_val =
          lepus::jsonValueTolepusValue(params_val[kParamGlobalProps]);
      PipelineOptions pipeline_options;
      tasm->UpdateGlobalProps(global_props_val, true, pipeline_options);
    } else if (CaseInsensitiveStringComparison(function_name,
                                               kFuncUpdateComponentData) &&
               params_val.HasMember(kParamComponentId) &&
               params_val.HasMember(kParamData)) {
      EXPECT_TRUE(params_val[kParamData].IsObject());
      lepus::Value data_val =
          lepus::jsonValueTolepusValue(params_val[kParamData]);

      EXPECT_TRUE(params_val[kParamComponentId].IsString());
      std::string component_id = params_val[kParamComponentId].GetString();
      PipelineOptions pipeline_options;
      tasm->page_proxy()->UpdateComponentData(component_id, data_val,
                                              pipeline_options);

    } else if (CaseInsensitiveStringComparison(
                   function_name, kFuncUpdateDataByPreParsedData) &&
               params_val.HasMember(kParamValue) &&
               params_val.HasMember(kParamPreprocessorName)) {
      EXPECT_TRUE(params_val[kParamValue].IsObject());
      lepus::Value value_val =
          lepus::jsonValueTolepusValue(params_val[kParamValue]);

      EXPECT_TRUE(params_val[kParamPreprocessorName].IsString());
      std::string preprocessorName_val =
          params_val[kParamPreprocessorName].GetString();

      std::shared_ptr<tasm::TemplateData> template_data_ptr =
          std::make_shared<tasm::TemplateData>(
              TemplateData(value_val, false, preprocessorName_val));
      UpdatePageOption update_page_option;
      update_page_option.from_native = true;
      PipelineOptions pipeline_options;
      tasm->UpdateDataByPreParsedData(template_data_ptr, update_page_option,
                                      pipeline_options);

    } else if (CaseInsensitiveStringComparison(function_name,
                                               kFuncUpdateComponent) &&
               params_val.HasMember(kParamSign) &&
               params_val.HasMember(kParamRow) &&
               params_val.HasMember(kParamOperationId) &&
               params_val.HasMember(kParamImplId) &&
               params_val.HasMember(kParamBaseId)) {
      EXPECT_TRUE(params_val[kParamBaseId].IsUint());
      uint32_t base_id_val = params_val[kParamBaseId].GetUint();

      EXPECT_TRUE(params_val[kParamSign].IsUint());
      uint32_t sign_val =
          params_val[kParamSign].GetUint() + kInitialImplId - base_id_val;

      EXPECT_TRUE(params_val[kParamRow].IsUint());
      uint32_t row_val = params_val[kParamRow].GetUint();

      EXPECT_TRUE(params_val[kParamOperationId].IsInt64());
      int64_t operation_id_val = params_val[kParamOperationId].GetInt64();

      EXPECT_TRUE(params_val[kParamImplId].IsInt());
      int32_t id_val =
          params_val[kParamImplId].GetInt() + kInitialImplId - base_id_val;

      ListNode* node = GetListNode(id_val);
      if (node) {
        node->UpdateComponent(sign_val, row_val, operation_id_val);
      }

    } else if (CaseInsensitiveStringComparison(function_name,
                                               kFuncRenderComponentAtIndex) &&
               params_val.HasMember(kParamIndex) &&
               params_val.HasMember(kParamOperationId) &&
               params_val.HasMember(kParamImplId) &&
               params_val.HasMember(kParamBaseId)) {
      EXPECT_TRUE(params_val[kParamBaseId].IsUint());
      uint32_t base_id_val = params_val[kParamBaseId].GetUint();

      EXPECT_TRUE(params_val[kParamIndex].IsUint());
      uint32_t index_val = params_val[kParamIndex].GetUint();

      EXPECT_TRUE(params_val[kParamOperationId].IsInt64());
      int64_t operation_id_val = params_val[kParamOperationId].GetInt64();

      EXPECT_TRUE(params_val[kParamImplId].IsInt());
      int32_t id_val =
          params_val[kParamImplId].GetInt() + kInitialImplId - base_id_val;

      ListNode* node = GetListNode(id_val);
      if (node) {
        node->RenderComponentAtIndex(index_val, operation_id_val);
      }

    } else if (CaseInsensitiveStringComparison(function_name,
                                               kFuncRemoveComponent) &&
               params_val.HasMember(kParamSign) &&
               params_val.HasMember(kParamImplId) &&
               params_val.HasMember(kParamBaseId)) {
      EXPECT_TRUE(params_val[kParamBaseId].IsUint());
      uint32_t base_id_val = params_val[kParamBaseId].GetUint();

      EXPECT_TRUE(params_val[kParamSign].IsUint());
      uint32_t sign_val =
          params_val[kParamSign].GetUint() + kInitialImplId - base_id_val;

      EXPECT_TRUE(params_val[kParamImplId].IsInt());
      int32_t id_val =
          params_val[kParamImplId].GetInt() + kInitialImplId - base_id_val;

      ListNode* node = GetListNode(id_val);
      if (node) {
        node->RenderComponentAtIndex(sign_val);
      }
    } else if (CaseInsensitiveStringComparison(function_name,
                                               kFuncSendTouchEvent)) {
      EXPECT_TRUE(params_val.IsObject());
      EXPECT_TRUE(params_val.HasMember(kParaName));
      EXPECT_TRUE(params_val[kParaName].IsString());
      std::string name = params_val[kParaName].GetString();

      EXPECT_TRUE(params_val.HasMember(kParaTag));
      EXPECT_TRUE(params_val[kParaTag].IsNumber());
      int tag = params_val[kParaTag].GetInt();

      EXPECT_TRUE(params_val.HasMember(kParaRootTag));
      EXPECT_TRUE(params_val[kParaRootTag].IsNumber());
      int root_tag = params_val[kParaRootTag].GetInt();
      int new_tag = tasm->page_proxy()->element_manager()->root()->impl_id() +
                    tag - root_tag;

      EXPECT_TRUE(params_val.HasMember(kParaX));
      EXPECT_TRUE(params_val[kParaX].IsNumber());
      double x = params_val[kParaX].GetDouble();

      EXPECT_TRUE(params_val.HasMember(kParaY));
      EXPECT_TRUE(params_val[kParaY].IsNumber());
      double y = params_val[kParaY].GetDouble();

      EXPECT_TRUE(params_val.HasMember(kParaClientX));
      EXPECT_TRUE(params_val[kParaClientX].IsNumber());
      double client_x = params_val[kParaClientX].GetDouble();

      EXPECT_TRUE(params_val.HasMember(kParaClientY));
      EXPECT_TRUE(params_val[kParaClientY].IsNumber());
      double client_y = params_val[kParaClientY].GetDouble();

      EXPECT_TRUE(params_val.HasMember(kParaPageX));
      EXPECT_TRUE(params_val[kParaPageX].IsNumber());
      double page_x = params_val[kParaPageX].GetDouble();

      EXPECT_TRUE(params_val.HasMember(kParaPageY));
      EXPECT_TRUE(params_val[kParaPageY].IsNumber());
      double page_y = params_val[kParaPageY].GetDouble();

      EventInfo info(new_tag, x, y, client_x, client_y, page_x, page_y);
      tasm->SendTouchEvent(name, info);
    } else if (CaseInsensitiveStringComparison(function_name,
                                               kFuncSendCustomEvent)) {
      EXPECT_TRUE(params_val.IsObject());
      EXPECT_TRUE(params_val.HasMember(kParaName));
      EXPECT_TRUE(params_val[kParaName].IsString());
      std::string name = params_val[kParaName].GetString();

      EXPECT_TRUE(params_val.HasMember(kParaTag));
      EXPECT_TRUE(params_val[kParaTag].IsNumber());
      int tag = params_val[kParaTag].GetInt();

      EXPECT_TRUE(params_val.HasMember(kParaRootTag));
      EXPECT_TRUE(params_val[kParaRootTag].IsNumber());
      int root_tag = params_val[kParaRootTag].GetInt();
      int new_tag = tasm->page_proxy()->element_manager()->root()->impl_id() +
                    tag - root_tag;

      EXPECT_TRUE(params_val.HasMember(kParaPname));
      EXPECT_TRUE(params_val[kParaPname].IsString());
      std::string pname = params_val[kParaPname].GetString();

      EXPECT_TRUE(params_val.HasMember(kParaParams));
      EXPECT_TRUE(params_val[kParaParams].IsObject());
      lepus::Value params =
          lepus::jsonValueTolepusValue(params_val[kParaParams]);

      tasm->SendCustomEvent(name, new_tag, params, pname);
    } else if (CaseInsensitiveStringComparison(
                   function_name, kFuncLoadComponentWithCallback)) {
      EXPECT_TRUE(params_val.IsObject());

      EXPECT_TRUE(params_val.HasMember(kSyncTag));
      EXPECT_TRUE(params_val[kSyncTag].IsBool());
      bool sync = params_val[kSyncTag].GetBool();

      // Sync dynamic component loading will be called by component loader,
      // so, just call async loading.
      if (!sync) {
        EXPECT_TRUE(params_val.HasMember(kParaUrl));
        EXPECT_TRUE(params_val[kParaUrl].IsString());
        const auto& url = params_val[kParaUrl].GetString();

        EXPECT_TRUE(params_val[kParaSource].IsString());
        std::string source = params_val[kParaSource].GetString();
        const auto& src = modp_b64_decode(source);
        std::vector<uint8_t> input_source(src.begin(), src.end());

        EXPECT_TRUE(params_val.HasMember(DataUpdateReplayer::kCallbackId));
        EXPECT_TRUE(params_val[DataUpdateReplayer::kCallbackId].IsNumber());
        auto callback_info = LazyBundleLoader::CallBackInfo(
            url, input_source, std::nullopt, std::nullopt, nullptr, -1);
        callback_info.sync = false;
        PipelineOptions pipeline_options;
        tasm->DidLoadComponent(std::move(callback_info), pipeline_options);
      }
    }
  }
}

ListNode* DataUpdateReplayer::GetListNode(int32_t tag) {
  auto tasm = weak_tasm_.lock();
  if (tasm == nullptr) {
    return nullptr;
  }
  // client maybe nullptr
  auto& client = tasm->page_proxy()->element_manager();
  if (client == nullptr || tasm == nullptr) {
    return nullptr;
  }
  lynx::tasm::Element* element = client->node_manager()->Get(tag);
  if (element == nullptr) {
    return nullptr;
  }
  if (tasm->page_proxy()->HasRadonPage()) {
    // we are in radon page mode.
    if (element->data_model() == nullptr) {
      return nullptr;
    }
    auto* node = static_cast<lynx::tasm::RadonNode*>(
        element->data_model()->radon_node_ptr());
    if (node && node->NodeType() == lynx::tasm::RadonNodeType::kRadonListNode) {
      return static_cast<lynx::tasm::ListNode*>(
          static_cast<lynx::tasm::RadonListBase*>(node));
    }
  }
  return nullptr;
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
