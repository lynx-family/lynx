// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_RECORDER_TESTBENCH_BASE_RECORDER_H_
#define CORE_SERVICES_RECORDER_TESTBENCH_BASE_RECORDER_H_

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/closure.h"
#include "base/include/fml/thread.h"
#include "base/include/no_destructor.h"
#include "core/services/recorder/recorder_constants.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace tasm {
namespace recorder {

class TestBenchBaseRecorder {
 public:
  static TestBenchBaseRecorder& GetInstance();

  bool IsRecordingProcess();

  void RecordAction(const char* function_name, rapidjson::Value& params,
                    int64_t record_id);
  void RecordInvokedMethodData(const char* module_name, const char* method_name,
                               rapidjson::Value& params, int64_t record_id);

  void RecordCallback(const char* module_name, const char* method_name,
                      rapidjson::Value& params, int64_t callback_id,
                      int64_t record_id);

  void RecordComponent(const char* name, int type, int64_t record_id);

  void RecordScripts(const char* url, const char* source);

  rapidjson::Document::AllocatorType& GetAllocator();
  void SetRecorderPath(const std::string& path);
  void SetScreenSize(int64_t record_id, float screen_width,
                     float screen_height);
  void AddLynxViewSessionID(int64_t record_id, int64_t session);
  void StartRecord();
  void EndRecord(base::MoveOnlyClosure<void, std::vector<std::string>&,
                                       std::vector<int64_t>&>
                     send_complete);

 private:
  friend base::NoDestructor<TestBenchBaseRecorder>;
  TestBenchBaseRecorder();
  ~TestBenchBaseRecorder() = default;
  TestBenchBaseRecorder(const TestBenchBaseRecorder&) = delete;
  TestBenchBaseRecorder& operator=(const TestBenchBaseRecorder&) = delete;

  void RecordTime(rapidjson::Value& val);
  rapidjson::Value& GetRecordedFileField(int64_t record_id,
                                         const std::string& filed_name);
  rapidjson::Value& GetRecordedFile(int64_t record_id);
  void CreateRecordedFile(int64_t record_id);

  template <typename T>
  void InsertReplayConfig(int64_t record_id, const char* name, T value);

  std::unordered_map<int64_t, rapidjson::Value> lynx_view_table_;
  rapidjson::Value resource_table_;
  rapidjson::Value scripts_table_;
  bool is_recording_;
  std::string file_path_;
  std::unordered_map<int64_t, rapidjson::Value> replay_config_map_;
  std::unordered_map<int64_t, std::string> url_map_;
  std::unordered_map<int64_t, int64_t> session_ids_;
  fml::Thread thread_;
  void RecordActionKernel(const char* function_name, rapidjson::Value params,
                          int64_t record_id,
                          rapidjson::Document::AllocatorType& allocator);
  void Clear();
};

}  // namespace recorder
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_RECORDER_TESTBENCH_BASE_RECORDER_H_
