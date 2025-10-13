// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_LYNX_RECORDER_TEST_BENCH_URL_ANALYZER_H_
#define PLATFORM_EMBEDDER_LYNX_RECORDER_TEST_BENCH_URL_ANALYZER_H_

#include <map>
#include <string>

namespace lynx {
namespace embedder {

class TestBenchURLAnalyzer {
 public:
  explicit TestBenchURLAnalyzer(const std::string& url);
  ~TestBenchURLAnalyzer() = default;

  std::string GetQueryStringParameter(const std::string& key) const;
  bool GetQueryBooleanParameter(const std::string& key,
                                bool default_value) const;

 private:
  void ParseUrl(const std::string& url);

 private:
  std::map<std::string, std::string> query_parameters_;
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_LYNX_RECORDER_TEST_BENCH_URL_ANALYZER_H_
