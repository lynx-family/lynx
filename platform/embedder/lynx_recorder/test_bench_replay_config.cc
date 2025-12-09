// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_recorder/test_bench_replay_config.h"

#include "platform/embedder/lynx_recorder/test_bench_url_analyzer.h"
#include "platform/embedder/lynx_recorder/test_bench_utils.h"

namespace lynx {
namespace embedder {

void TestBenchReplayConfig::InitWithProductUrl(const std::string& url) {
  if (url.empty()) {
    return;
  }
  if (!url_analyzer_) {
    url_analyzer_ = std::make_unique<TestBenchURLAnalyzer>(url);
  }
  url_ = url_analyzer_->GetQueryStringParameter("url");
  source_url_ = url_analyzer_->GetQueryStringParameter("source");
  replay_gesture_ = url_analyzer_->GetQueryBooleanParameter("gesture", false);
  height_limit_ = url_analyzer_->GetQueryBooleanParameter("heightLimit", false);
  std::string delay_end_interval_str =
      url_analyzer_->GetQueryStringParameter("delayEndInterval");
  StringToInt(delay_end_interval_str, &delay_end_interval_, 10);
  can_mock_func_name_["setGlobalProps"] = SET_GLOBAL_PROPS;
  can_mock_func_name_["initialLynxView"] = INITIAL_LYNX_VIEW;
  can_mock_func_name_["loadTemplate"] = LOAD_TEMPLATE;
  can_mock_func_name_["updateDataByPreParsedData"] =
      UPDATE_DATA_BY_PRE_PARSED_DATA;
  can_mock_func_name_["sendGlobalEvent"] = SEND_GLOBAL_EVENT;
  can_mock_func_name_["reloadTemplate"] = RELOAD_TEMPLATE;
  can_mock_func_name_["updateConfig"] = UPDATE_CONFIG;
  can_mock_func_name_["loadTemplateBundle"] = LOAD_TEMPLATE_BUNDLE;
  can_mock_func_name_["fromTemplate"] = FROM_TEMPLATE;
  can_mock_func_name_["SendTouchEvent"] = SEND_TOUCH_EVENT;
  can_mock_func_name_["SendBubbleEvent"] = SEND_BUBBLE_EVENT;
  can_mock_func_name_["SendCustomEvent"] = SEND_CUSTOM_EVENT;
  reload_func_name_ = {"sendGlobalEvent", "updateDataByPreParsedData"};
}

int TestBenchReplayConfig::GetCanMockFuncId(const std::string& func_name) {
  auto item_find = can_mock_func_name_.find(func_name);
  if (item_find != can_mock_func_name_.end()) {
    return item_find->second;
  }
  return -1;
}

bool TestBenchReplayConfig::CheckCanMockFuncName(const std::string& func_name) {
  auto item_find = can_mock_func_name_.find(func_name);
  if (item_find != can_mock_func_name_.end()) {
    return true;
  }
  return false;
}

bool TestBenchReplayConfig::CheckReloadFuncName(const std::string& func_name) {
  auto item_find = reload_func_name_.find(func_name);
  if (item_find != reload_func_name_.end()) {
    return true;
  }
  return false;
}

}  // namespace embedder
}  // namespace lynx
