// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_LYNX_RECORDER_TEST_BENCH_ACTION_MANAGER_H_
#define PLATFORM_EMBEDDER_LYNX_RECORDER_TEST_BENCH_ACTION_MANAGER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "platform/embedder/lynx_recorder/test_bench_replay_config.h"
#include "platform/embedder/lynx_recorder/test_bench_replay_data_module.h"
#include "platform/embedder/lynx_recorder/test_bench_utils.h"
#include "platform/embedder/public/lynx_view.h"

namespace lynx {
namespace embedder {

typedef struct ReplayAction {
  int function_id;
  std::string params;
  int64_t interval;
} ReplayAction;

using ResizeCallback = std::function<void(int width, int height)>;
using FetchCallback = std::function<void(
    const std::string& url, std::function<void(const std::string& result)>)>;

class TestBenchActionManager {
 public:
  TestBenchActionManager(std::shared_ptr<lynx::pub::LynxView> view,
                         ResizeCallback resize_callback);
  ~TestBenchActionManager() = default;

  void SetFetchCallback(FetchCallback fetch_callback);

  void StartWithUrl(const std::string& url);

 private:
  void FetchPreloadedSource(const std::string& url);
  void FetchRecordFile(const std::string& url);

  void HandleRecordFileData(const std::string& result);
  bool CheckFile(const rapidjson::Value& action_list);
  void HandleActionList(const rapidjson::Value& action_list);
  void DoAction(const ReplayAction& action);

  // action function
  void InitialLynxView(const std::string& param);
  void SetGlobalProps(const std::string& param);
  void LoadTemplate(const std::string& param);
  void LoadTemplateBundle(const std::string& param);
  void UpdatePreData(const std::string& param);
  void SendGlobalEvent(const std::string& param);
  void ReloadTemplate(const std::string& param);
  void UpdateConfig(const std::string& param);
  void FromTemplate(const std::string& param);
  void SendTouchEvent(const std::string& param);
  void SendBubbleEvent(const std::string& param);
  void SendCustomEvent(const std::string& param);

 private:
  std::shared_ptr<lynx::pub::LynxView> lynx_view_;
  ResizeCallback resize_callback_;
  FetchCallback fetch_callback_;
  std::unique_ptr<embedder::TestBenchReplayConfig> replay_config_;
  std::shared_ptr<TestBenchReplayDataModule> data_module_;

  std::string preloaded_source_;
  std::string component_list_;
  std::string template_bundle_param_;

  std::shared_ptr<lynx::pub::LynxTemplateData> global_props_;
  std::shared_ptr<lynx::pub::LynxTemplateBundle> template_bundle_;
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_LYNX_RECORDER_TEST_BENCH_ACTION_MANAGER_H_
