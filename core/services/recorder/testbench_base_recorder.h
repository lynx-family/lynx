// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_RECORDER_TESTBENCH_BASE_RECORDER_H_
#define CORE_SERVICES_RECORDER_TESTBENCH_BASE_RECORDER_H_

#include <atomic>
#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "base/include/closure.h"
#include "base/include/fml/thread.h"
#include "base/include/no_destructor.h"
#include "core/base/lynx_export.h"
#include "core/services/recorder/recorder_constants.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace tasm {
namespace recorder {

class TestBenchBaseRecorder {
 public:
  static TestBenchBaseRecorder& GetInstance();

  bool IsRecordingProcess();
  uint64_t RecordingGeneration();

  void RecordDebugInfo(int64_t record_id, const std::string& url,
                       const std::string& content);

  void RecordAction(const char* function_name, rapidjson::Value& params,
                    int64_t record_id);
  void RecordActionWithGeneration(std::string function_name,
                                  rapidjson::Document params, int64_t record_id,
                                  uint64_t recording_generation);
  void RecordInvokedMethodData(const char* module_name, const char* method_name,
                               rapidjson::Value& params, int64_t record_id);
  void RecordInvokedMethodDataWithGeneration(std::string module_name,
                                             std::string method_name,
                                             rapidjson::Document params,
                                             int64_t record_id,
                                             uint64_t recording_generation);

  void RecordCallback(const char* module_name, const char* method_name,
                      rapidjson::Value& params, int64_t callback_id,
                      int64_t record_id);
  void RecordCallbackWithGeneration(std::string module_name,
                                    std::string method_name,
                                    rapidjson::Document params,
                                    int64_t callback_id, int64_t record_id,
                                    uint64_t recording_generation);
  void RecordSharedData(const std::string& key, rapidjson::Value& value,
                        int64_t record_id);

  void RecordComponent(const char* name, int type, int64_t record_id);

  void RecordScripts(const std::string& url, const std::string& source,
                     int64_t record_id);
  void RecordExternalScript(const std::string& url, const std::string& source);
  void RecordExternalTemplate(const std::string& url, const std::string& source,
                              int64_t record_id);
  void RecordPreloadScript(const std::string& url, const std::string& source,
                           int64_t record_id);

  // Returns false if this external script url was already recorded for
  // record_id in the current recording session.
  bool TryRecordExternalScriptUrl(int64_t record_id, const std::string& url);

  rapidjson::Document::AllocatorType& GetAllocator();
  void InitConfig(const std::string& path, int64_t session_id,
                  float screen_width, float screen_height, int64_t record_id);
  void SetRecorderPath(const std::string& path);
  void SetScreenSize(int64_t record_id, float screen_width,
                     float screen_height);
  void AddLynxViewSessionID(int64_t record_id, int64_t session);
  void RemoveRecord(int64_t record_id);
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
  void AppendInvokedMethodData(rapidjson::Value& recorded_file,
                               const std::string& module_name,
                               const std::string& method_name,
                               const rapidjson::Value& params);
  void AppendCallbackData(rapidjson::Value& recorded_file,
                          const std::string& module_name,
                          const std::string& method_name,
                          const rapidjson::Value& params, int64_t callback_id);
  template <typename T>
  void InsertReplayConfig(int64_t record_id, const char* name, T value);

  std::unordered_map<int64_t, rapidjson::Value> lynx_view_table_;
  rapidjson::Value resource_table_;
  std::atomic<bool> is_recording_{false};
  std::atomic<uint64_t> recording_generation_{0};
  std::string file_path_;
  std::unordered_map<int64_t, rapidjson::Document> replay_config_map_;
  std::unordered_map<int64_t, std::string> url_map_;
  std::unordered_map<int64_t, int64_t> session_ids_;
  std::mutex recorded_external_script_urls_mutex_;
  std::unordered_map<int64_t, std::unordered_set<std::string>>
      recorded_external_script_urls_;
  std::unordered_map<int64_t, std::unordered_map<std::string, std::string>>
      script_cache_;
  std::unordered_map<std::string, std::string> external_script_cache_;
  std::unordered_map<int64_t, std::unordered_map<std::string, std::string>>
      preload_script_cache_;
  std::unordered_map<int64_t, std::vector<std::string>>
      preload_script_paths_cache_;
  fml::Thread thread_;
  void RecordActionOwned(std::string function_name, rapidjson::Document params,
                         int64_t record_id, uint64_t recording_generation);
  void RecordInvokedMethodDataOwned(std::string module_name,
                                    std::string method_name,
                                    rapidjson::Document params,
                                    int64_t record_id,
                                    uint64_t recording_generation);
  void RecordCallbackOwned(std::string module_name, std::string method_name,
                           rapidjson::Document params, int64_t callback_id,
                           int64_t record_id, uint64_t recording_generation);
  void RecordActionKernel(const char* function_name,
                          const rapidjson::Value& params, int64_t record_id,
                          rapidjson::Document::AllocatorType& allocator);
  void ClearRecordingSessionData();
  void ClearRecordedData();
  // Resets singleton state between tests. Production sessions retain per-view
  // script caches until RemoveRecord so a reused LynxView can record again.
  void ResetForTesting();

  // Compresses size bytes with zlib and base64-encodes the compressed bytes.
  // Returns the encoded string, or an empty string when compression fails.
  // Shared by WriteRecordJson, RecordScripts and RecordDebugInfo.
  static std::string CompressToBase64String(const char* data, size_t size);

  // Writes one shell's record json (serialize + zlib compress + base64) to
  // recorder{shell_id}.json. Returns true on success. Extracted from
  // EndRecord so it is independently unit-testable; the doc must already
  // contain Config.
  static bool WriteRecordJson(const std::string& filename,
                              rapidjson::Value& doc);
};

}  // namespace recorder
}  // namespace tasm
}  // namespace lynx

extern "C" {
LYNX_EXPORT_FOR_DEVTOOL void LynxTestBenchRecordExternalScript(
    const char* url, const char* source);
LYNX_EXPORT_FOR_DEVTOOL void LynxTestBenchRecordExternalScriptWithSize(
    const char* url, const char* source, size_t source_size);
LYNX_EXPORT_FOR_DEVTOOL void LynxTestBenchRecordExternalTemplateWithSize(
    int64_t record_id, const char* url, const char* source, size_t source_size);
}

#endif  // CORE_SERVICES_RECORDER_TESTBENCH_BASE_RECORDER_H_
