// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "testing/lynx/tasm/databinding/mock_replayer_component_loader.h"

#include <optional>
#include <utility>

#include "core/renderer/template_assembler.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "testing/lynx/tasm/databinding/data_update_replayer.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/modp_b64/modp_b64.h"

namespace lynx {
namespace tasm {
namespace test {

MockReplayerComponentLoader::MockReplayerComponentLoader(
    std::weak_ptr<TemplateAssembler> tasm)
    : tasm_(tasm) {
  list_header_ = require_info_list_.begin();
};

void MockReplayerComponentLoader::RequireTemplate(
    RadonLazyComponent* dynamic_component, const std::string& url,
    int instance_id) {
  // NOTIFY: DO NOT USE DYNAMIC COMPONENTS IN LIST IN DATA BINDING

  // Whenever calling RequireTemplate, header should not be the end;
  EXPECT_TRUE(list_header_ != require_info_list_.end());

  // mock each requirement by calling sequence
  if (list_header_->sync_) {
    // If the requirement is sync,
    // mock it by calling tasm->LoadComponentWithCallback().
    EXPECT_TRUE(list_header_->url_ == url);
    auto component = component_map_.find(url);
    EXPECT_TRUE(component != component_map_.end());

    auto tasm = tasm_.lock();
    EXPECT_TRUE(tasm != nullptr);
    std::vector<std::string> ids;
    PipelineOptions pipeline_options;
    auto callback_info = tasm::LazyBundleLoader::CallBackInfo{
        url,  component->second.source_,      std::nullopt, std::nullopt,
        true, component->second.callback_id_, ids};

    tasm->LoadComponentWithCallbackInfo(std::move(callback_info),
                                        pipeline_options);
  }
  ++list_header_;
}

void MockReplayerComponentLoader::InitWithActionList(
    const rapidjson::Value& action_list) {
  EXPECT_TRUE(action_list.IsArray());
  for (auto iter = action_list.Begin(); iter != action_list.End(); ++iter) {
    EXPECT_TRUE(iter->HasMember(DataUpdateReplayer::kFunctionName));

    const std::string function_name =
        (*iter)[DataUpdateReplayer::kFunctionName].GetString();
    EXPECT_TRUE(iter->HasMember(DataUpdateReplayer::kParams));

    const rapidjson::Value& params_val = (*iter)[DataUpdateReplayer::kParams];
    EXPECT_TRUE(params_val.IsObject());
    if (DataUpdateReplayer::CaseInsensitiveStringComparison(
            function_name, DataUpdateReplayer::kFuncRequireTemplate)) {
      // record every require action by a list
      EXPECT_TRUE(params_val.IsObject());

      EXPECT_TRUE(params_val.HasMember(DataUpdateReplayer::kParaUrl));
      EXPECT_TRUE(params_val[DataUpdateReplayer::kParaUrl].IsString());
      std::string url = params_val[DataUpdateReplayer::kParaUrl].GetString();

      EXPECT_TRUE(params_val.HasMember(DataUpdateReplayer::kSyncTag));
      EXPECT_TRUE(params_val[DataUpdateReplayer::kSyncTag].IsBool());
      bool sync = params_val[DataUpdateReplayer::kSyncTag].GetBool();

      require_info_list_.emplace_back(url, sync);
    } else if (DataUpdateReplayer::CaseInsensitiveStringComparison(
                   function_name,
                   DataUpdateReplayer::kFuncLoadComponentWithCallback)) {
      EXPECT_TRUE(params_val.IsObject());

      EXPECT_TRUE(params_val.HasMember(DataUpdateReplayer::kSyncTag));
      EXPECT_TRUE(params_val[DataUpdateReplayer::kSyncTag].IsBool());
      bool sync = params_val[DataUpdateReplayer::kSyncTag].GetBool();

      // If loading component sync, we must record it
      // to mock this requirement later.
      if (sync) {
        EXPECT_TRUE(params_val.HasMember(DataUpdateReplayer::kParaUrl));
        EXPECT_TRUE(params_val[DataUpdateReplayer::kParaUrl].IsString());
        const auto& url = params_val[DataUpdateReplayer::kParaUrl].GetString();

        EXPECT_TRUE(params_val[DataUpdateReplayer::kParaSource].IsString());
        std::string source =
            params_val[DataUpdateReplayer::kParaSource].GetString();
        const auto& src = modp_b64_decode(source);
        std::vector<uint8_t> input_source(src.begin(), src.end());

        EXPECT_TRUE(params_val.HasMember(DataUpdateReplayer::kCallbackId));
        EXPECT_TRUE(params_val[DataUpdateReplayer::kCallbackId].IsNumber());
        int32_t callback_id = static_cast<int32_t>(
            params_val[DataUpdateReplayer::kCallbackId].GetInt());

        component_map_.insert(
            std::make_pair(url, LoadComponentInfo(input_source, callback_id)));
      }
    }
  }

  list_header_ = require_info_list_.begin();
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
