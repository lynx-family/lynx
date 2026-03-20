// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_LYNX_RECORDER_TEST_BENCH_REPLAY_DATA_MODULE_H_
#define PLATFORM_EMBEDDER_LYNX_RECORDER_TEST_BENCH_REPLAY_DATA_MODULE_H_

#include <functional>
#include <memory>
#include <string>

#include "platform/embedder/public/capi/lynx_native_module_capi.h"

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_defines.h"
#endif

namespace lynx {
namespace embedder {

class TestBenchReplayDataModule
    : public std::enable_shared_from_this<TestBenchReplayDataModule> {
 public:
  TestBenchReplayDataModule() = default;
  virtual ~TestBenchReplayDataModule();

  static void RegisterJSB(
      std::function<void(const std::string&, napi_module_creator)>);

  void BindContext(void* context);

  void SetJsbIgnoredInfo(const std::string& data);
  void SetJsbSettings(const std::string& data);
  void SetFunctionCall(const std::string& data);
  void SetCallbackData(const std::string& data);

  std::string GetRecordData();
  std::string GetJsbIgnoredInfo();
  std::string GetJsbSettings();

  std::string GetData();

 private:
  void* context_ = nullptr;

  std::string jsb_ignored_info_;
  std::string jsb_settings_;
  std::string function_call_;
  std::string callback_data_;
};

}  // namespace embedder
}  // namespace lynx

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_undefs.h"
#endif

#endif  // PLATFORM_EMBEDDER_LYNX_RECORDER_TEST_BENCH_REPLAY_DATA_MODULE_H_
