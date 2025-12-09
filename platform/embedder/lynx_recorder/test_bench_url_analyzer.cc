// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_recorder/test_bench_url_analyzer.h"

#include "platform/embedder/lynx_recorder/test_bench_utils.h"

namespace lynx {
namespace embedder {

TestBenchURLAnalyzer::TestBenchURLAnalyzer(const std::string& url) {
  ParseUrl(url);
}

std::string TestBenchURLAnalyzer::GetQueryStringParameter(
    const std::string& key) const {
  auto item_find = query_parameters_.find(key);
  if (item_find != query_parameters_.end()) {
    return item_find->second;
  }
  return std::string();
}

bool TestBenchURLAnalyzer::GetQueryBooleanParameter(const std::string& key,
                                                    bool default_value) const {
  std::string val = GetQueryStringParameter(key);
  return val.empty() ? default_value : true;
}

void TestBenchURLAnalyzer::ParseUrl(const std::string& url) {
  size_t query_index = url.find(TEST_BENCH_URL_PREFIX);
  if (query_index == std::string::npos) {
    return;
  }
  std::string query =
      url.substr(query_index + TEST_BENCH_URL_PREFIX.size(), url.size());
  bool break_flag = true;
  while (break_flag) {
    auto param_index = query.find('&');
    if (param_index == std::string::npos) {
      param_index = query.find('#');
      if (param_index == std::string::npos) {
        param_index = query.size();
      } else {
        param_index = 0;
      }
      break_flag = false;
    }
    std::string param = query.substr(0, param_index);
    auto key_index = param.find('=');
    if (key_index != std::string::npos) {
      std::string key = param.substr(0, key_index);
      std::string value =
          param.substr(key_index + 1, param.size() - key_index - 1);
      query_parameters_[key] = value;
    }
    if (param_index + 1 < query.size()) {
      query = query.substr(param_index + 1, query.size() - param_index - 1);
    }
  }
}

}  // namespace embedder
}  // namespace lynx
