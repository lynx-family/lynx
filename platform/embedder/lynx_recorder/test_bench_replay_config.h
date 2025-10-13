// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_LYNX_RECORDER_TEST_BENCH_REPLAY_CONFIG_H_
#define PLATFORM_EMBEDDER_LYNX_RECORDER_TEST_BENCH_REPLAY_CONFIG_H_

#include <map>
#include <memory>
#include <set>
#include <string>

#include "platform/embedder/lynx_recorder/test_bench_url_analyzer.h"

namespace lynx {
namespace embedder {

const int INITIAL_LYNX_VIEW = 0;
const int UPDATE_DATA_BY_PRE_PARSED_DATA = 1;
const int SET_GLOBAL_PROPS = 2;
const int LOAD_TEMPLATE = 3;
const int SEND_GLOBAL_EVENT = 4;
const int RELOAD_TEMPLATE = 5;
const int UPDATE_CONFIG = 6;
const int LOAD_TEMPLATE_BUNDLE = 7;
const int FROM_TEMPLATE = 8;
const int SEND_TOUCH_EVENT = 9;
const int SEND_BUBBLE_EVENT = 10;
const int SEND_CUSTOM_EVENT = 11;

class TestBenchURLAnalyzer;
class TestBenchReplayConfig {
 public:
  explicit TestBenchReplayConfig() = default;
  ~TestBenchReplayConfig() = default;

  void InitWithProductUrl(const std::string& url);

  const std::string& GetUrl() const { return url_; }
  const std::string& GetSourceUrl() const { return source_url_; }
  bool GetReplayGesture() const { return replay_gesture_; }
  bool GetHeightLimit() const { return height_limit_; }
  int GetDelayEndInterval() const { return delay_end_interval_; }
  int GetCanMockFuncId(const std::string& func_name);
  bool CheckCanMockFuncName(const std::string& func_name);
  bool CheckReloadFuncName(const std::string& func_name);

 private:
  std::unique_ptr<TestBenchURLAnalyzer> url_analyzer_;
  std::string url_;
  std::string source_url_;
  bool replay_gesture_ = false;
  bool height_limit_ = false;
  int delay_end_interval_ = 3500;
  std::map<std::string, int> can_mock_func_name_;
  std::set<std::string> reload_func_name_;
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_LYNX_RECORDER_TEST_BENCH_REPLAY_CONFIG_H_
