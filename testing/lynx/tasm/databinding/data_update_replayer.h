// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef TESTING_LYNX_TASM_DATABINDING_DATA_UPDATE_REPLAYER_H_
#define TESTING_LYNX_TASM_DATABINDING_DATA_UPDATE_REPLAYER_H_

#include <memory>
#include <string>

namespace lynx {
namespace tasm {

class ListNode;
class TemplateAssembler;

namespace test {
class DataUpdateReplayer {
 public:
  // base recorder
  static constexpr const char* kActionList = "Action List";
  static constexpr const char* kFunctionCallArray = "Function Call Array";
  static constexpr const char* kFunctionName = "Function Name";
  static constexpr const char* kParams = "Params";

  // list node recorder
  static constexpr const char* kParamBaseId = "base_id";
  static constexpr const char* kParamImplId = "impl_id";
  static constexpr const char* kParamIndex = "index";
  static constexpr const char* kParamOperationId = "operation_id";
  static constexpr const char* kParamRow = "row";
  static constexpr const char* kParamSign = "sign";

  static constexpr const char* kFuncRemoveComponent = "RemoveComponent";
  static constexpr const char* kFuncRenderComponentAtIndex =
      "RenderComponentAtIndex";
  static constexpr const char* kFuncUpdateComponent = "UpdateComponent";

  // template assembler recorder
  static constexpr const char* kParamComponentId = "component_id";
  static constexpr const char* kParamConfig = "config";
  static constexpr const char* kParamData = "data";
  static constexpr const char* kParamGlobalProps = "global_props";
  static constexpr const char* kParamNoticeDelegate = "noticeDelegate";
  static constexpr const char* kParamPreprocessorName = "preprocessorName";
  static constexpr const char* kParamValue = "value";
  static constexpr const char* kParaUrl = "url";
  static constexpr const char* kParaSource = "source";
  static constexpr const char* kParaTemplateData = "templateData";
  static constexpr const char* kParaName = "name";
  static constexpr const char* kParaPname = "pname";
  static constexpr const char* kParaParams = "params";
  static constexpr const char* kParaTag = "tag";
  static constexpr const char* kParaRootTag = "root_tag";
  static constexpr const char* kParaX = "x";
  static constexpr const char* kParaY = "y";
  static constexpr const char* kParaClientX = "client_x";
  static constexpr const char* kParaClientY = "client_y";
  static constexpr const char* kParaPageX = "page_x";
  static constexpr const char* kParaPageY = "page_y";

  // func name
  static constexpr const char* kFuncSetGlobalProps = "SetGlobalProps";
  static constexpr const char* kFuncUpdateComponentData = "UpdateComponentData";
  static constexpr const char* kFuncUpdateConfig = "UpdateConfig";
  static constexpr const char* kFuncUpdateDataByJS = "UpdateDataByJS";
  static constexpr const char* kFuncUpdateDataByPreParsedData =
      "UpdateDataByPreParsedData";
  static constexpr const char* kFuncLoadTemplate = "LoadTemplate";
  static constexpr const char* kFuncSendTouchEvent = "SendTouchEvent";
  static constexpr const char* kFuncSendCustomEvent = "SendCustomEvent";

  // dynamic component recoder
  static constexpr const char* kSyncTag = "sync_tag";
  static constexpr const char* kCallbackId = "callback_id";
  static constexpr const char* kFuncRequireTemplate = "RequireTemplate";
  static constexpr const char* kFuncLoadComponentWithCallback =
      "LoadComponentWithCallback";

  DataUpdateReplayer(std::shared_ptr<TemplateAssembler> tasm);
  void DataUpdateReplay(const std::string& replay_data, bool use_ark_source);

  static bool CaseInsensitiveStringComparison(const std::string& left,
                                              const char* right);
  static bool CharComparison(const char* c1, const char* c2);

 private:
  std::weak_ptr<TemplateAssembler> weak_tasm_;
  ListNode* GetListNode(int32_t tag);
};

}  // namespace test
}  // namespace tasm
}  // namespace lynx

#endif  // TESTING_LYNX_TASM_DATABINDING_DATA_UPDATE_REPLAYER_H_
