// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/recorder/testbench_base_recorder.h"

#include <cstdio>
#include <ctime>
#include <fstream>
#include <sstream>
#include <utility>

#include "base/include/closure.h"
#include "base/include/log/logging.h"
#include "third_party/modp_b64/modp_b64.h"
#include "third_party/rapidjson/filewritestream.h"
#include "third_party/rapidjson/prettywriter.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/zlib/zlib.h"

#if OS_IOS
#include <TargetConditionals.h>
#endif

namespace lynx {
namespace tasm {
namespace recorder {

thread_local rapidjson::Document dumped_document;

namespace {

std::string Base64Encode(const char* data, size_t size) {
  std::string encoded(lynx_modp_b64_encode_len(size), '\0');
  const size_t encoded_size = lynx_modp_b64_encode(&encoded[0], data, size);
  encoded.resize(encoded_size);
  return encoded;
}

std::string CompressAndBase64Encode(const char* data, size_t size) {
  // zlib-compress, then base64-encode. Both stages write directly into sized
  // string storage so no extra copy of the payload is made.
  unsigned long compressed_size = compressBound(size);
  std::string compressed(compressed_size, '\0');
  const int z_result =
      compress(reinterpret_cast<Bytef*>(&compressed[0]), &compressed_size,
               reinterpret_cast<const Bytef*>(data), size);
  if (z_result != Z_OK) {
    return "";
  }
  compressed.resize(compressed_size);
  return Base64Encode(compressed.data(), compressed.size());
}

// Single time source for the record timestamps so the seconds member and the
// milliseconds member of RecordTime (and later the fixture view) cannot drift
// apart.
int64_t CurrentRecordMillis() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

}  // namespace

std::string TestBenchBaseRecorder::CompressToBase64String(const char* data,
                                                          size_t size) {
  return CompressAndBase64Encode(data, size);
}

bool TestBenchBaseRecorder::WriteRecordJson(const std::string& filename,
                                            rapidjson::Value& doc) {
  std::ofstream ifs;
  ifs.open(filename, std::ios::binary | std::ios::out);
  if (!ifs.is_open()) {
    return false;
  }
  rapidjson::StringBuffer os;
  rapidjson::Writer<rapidjson::StringBuffer, rapidjson::UTF8<>,
                    rapidjson::UTF8<>, rapidjson::CrtAllocator,
                    rapidjson::kWriteNanAndInfFlag>
      writer(os);
  doc.Accept(writer);
  const std::string encoded =
      CompressToBase64String(os.GetString(), os.GetSize());
  // CompressToBase64String returns "" only when compression fails; never
  // leave an empty (or partially written) artifact behind in that case.
  bool ok = false;
  if (!encoded.empty()) {
    ifs.write(encoded.data(), encoded.size());
    ifs.flush();
    ok = ifs.good();
  }
  ifs.close();
  if (!ok) {
    std::remove(filename.c_str());
  }
  return ok;
}

void SetScriptValue(rapidjson::Value& scripts, const std::string& url,
                    const std::string& encoded_source,
                    rapidjson::Document::AllocatorType& allocator) {
  auto script = scripts.FindMember(url.c_str());
  if (script != scripts.MemberEnd()) {
    script->value.SetString(
        encoded_source.c_str(),
        static_cast<rapidjson::SizeType>(encoded_source.length()), allocator);
    return;
  }

  rapidjson::Value url_value;
  url_value.SetString(
      url.c_str(), static_cast<rapidjson::SizeType>(url.length()), allocator);
  rapidjson::Value source_value;
  source_value.SetString(
      encoded_source.c_str(),
      static_cast<rapidjson::SizeType>(encoded_source.length()), allocator);
  scripts.AddMember(url_value, source_value, allocator);
}

void AppendUniqueString(rapidjson::Value& values, const std::string& value,
                        rapidjson::Document::AllocatorType& allocator) {
  for (const auto& existing : values.GetArray()) {
    if (existing.IsString() && value == existing.GetString()) {
      return;
    }
  }
  rapidjson::Value item;
  item.SetString(value.c_str(),
                 static_cast<rapidjson::SizeType>(value.length()), allocator);
  values.PushBack(item, allocator);
}

TestBenchBaseRecorder::TestBenchBaseRecorder() : thread_("ark_recorder") {}

void TestBenchBaseRecorder::SetRecorderPath(const std::string& path) {
  file_path_ = path;
  file_path_ += "/";
}

void TestBenchBaseRecorder::SetScreenSize(int64_t record_id, float screen_width,
                                          float screen_height) {
  auto set_screen_size_task = [this, record_id, screen_width, screen_height]() {
    InsertReplayConfig(record_id, "screenWidth", screen_width);
    InsertReplayConfig(record_id, "screenHeight", screen_height);
  };
  thread_.GetTaskRunner()->PostTask(std::move(set_screen_size_task));
}

namespace {

[[maybe_unused]] inline void SetJsonValue(rapidjson::Value* target,
                                          float value) {
  target->SetFloat(value);
}

[[maybe_unused]] inline void SetJsonValue(rapidjson::Value* target,
                                          double value) {
  target->SetDouble(value);
}

[[maybe_unused]] inline void SetJsonValue(rapidjson::Value* target,
                                          int64_t value) {
  target->SetInt64(value);
}

[[maybe_unused]] inline void SetJsonValue(rapidjson::Value* target, int value) {
  target->SetInt(value);
}

[[maybe_unused]] inline void SetJsonValue(rapidjson::Value* target,
                                          bool value) {
  target->SetBool(value);
}

}  // namespace

template <typename T>
void TestBenchBaseRecorder::InsertReplayConfig(int64_t record_id,
                                               const char* name, T value) {
  rapidjson::Document& config = replay_config_map_[record_id];
  rapidjson::Document::AllocatorType& allocator = config.GetAllocator();
  if (!config.IsObject()) {
    config.SetObject();

    // add jsb ignored info
    rapidjson::Document jsb_ignored_info;
    jsb_ignored_info.Parse(KJsbIgnoredInfo);
    rapidjson::Value jsb_ignored_info_value;
    jsb_ignored_info_value.CopyFrom(jsb_ignored_info, allocator);
    config.AddMember(rapidjson::StringRef("jsbIgnoredInfo"),
                     jsb_ignored_info_value, allocator);

    rapidjson::Value jsb_settings(rapidjson::kObjectType);
    jsb_settings.AddMember(rapidjson::StringRef("strict"), true, allocator);
    config.AddMember(rapidjson::StringRef("jsbSettings"), jsb_settings,
                     allocator);
  }

  auto member_it = config.FindMember(name);
  if (member_it != config.MemberEnd()) {
    SetJsonValue(&member_it->value, value);
    return;
  }

  // StringRef creates dangling pointer when original string is destroyed, we
  // create a deep copy of the string key to avoid heap-buffer-overflow.
  rapidjson::Value key;
  key.SetString(name, allocator);
  config.AddMember(key, value, allocator);
}

TestBenchBaseRecorder& TestBenchBaseRecorder::GetInstance() {
  static base::NoDestructor<TestBenchBaseRecorder> instance_;
  return *instance_;
}

void TestBenchBaseRecorder::InitConfig(const std::string& path,
                                       int64_t session_id, float screen_width,
                                       float screen_height, int64_t record_id) {
  auto init_config_task = [this, path, session_id, screen_width, screen_height,
                           record_id]() {
    SetRecorderPath(path);
    AddLynxViewSessionID(record_id, session_id);
    InsertReplayConfig(record_id, "screenWidth", screen_width);
    InsertReplayConfig(record_id, "screenHeight", screen_height);
    if (is_recording_.load(std::memory_order_acquire)) {
      GetRecordedFile(record_id);
    }
  };
  // StartRecord stays synchronous so events cannot be dropped immediately
  // after Recording.start. Config and record mutations run FIFO on this runner,
  // including when recording starts before a LynxView is created.
  thread_.GetTaskRunner()->PostTask(std::move(init_config_task));
}

bool TestBenchBaseRecorder::IsRecordingProcess() {
  return is_recording_.load(std::memory_order_acquire);
}

uint64_t TestBenchBaseRecorder::RecordingGeneration() {
  return recording_generation_.load(std::memory_order_acquire);
}

rapidjson::Document::AllocatorType& TestBenchBaseRecorder::GetAllocator() {
  return dumped_document.GetAllocator();
};

void TestBenchBaseRecorder::StartRecord() {
  bool expected = false;
  if (is_recording_.compare_exchange_strong(expected, true,
                                            std::memory_order_acq_rel)) {
    const uint64_t recording_generation =
        recording_generation_.fetch_add(1, std::memory_order_acq_rel) + 1;
    auto initialize_records_task = [this, recording_generation]() {
      if (!is_recording_.load(std::memory_order_acquire) ||
          RecordingGeneration() != recording_generation) {
        return;
      }
      for (const auto& config : replay_config_map_) {
        GetRecordedFile(config.first);
      }
    };
    thread_.GetTaskRunner()->PostTask(std::move(initialize_records_task));
  }
}

void TestBenchBaseRecorder::EndRecord(
    base::MoveOnlyClosure<void, std::vector<std::string>&,
                          std::vector<int64_t>&>
        send_complete) {
  auto writer_task = [this,
                      complete_func = std::move(send_complete)]() mutable {
    if (!is_recording_.load(std::memory_order_acquire)) {
      return;
    }
    is_recording_.store(false, std::memory_order_release);
    std::vector<std::string> filenames;
    std::vector<int64_t> sessions;
    for (auto& lynx_view_pair : lynx_view_table_) {
      int64_t shell_id = lynx_view_pair.first;
      std::string filename = file_path_ + std::to_string(shell_id) + ".json";
      {
        rapidjson::Value& doc = lynx_view_pair.second;
        rapidjson::Document::AllocatorType& allocator = GetAllocator();
        rapidjson::Value config;
        auto config_it = replay_config_map_.find(shell_id);
        if (config_it == replay_config_map_.end()) {
          config.SetNull();
        } else {
          config.CopyFrom(config_it->second, allocator);
        }
        doc.AddMember(rapidjson::StringRef(kConfig), config, allocator);
        // Only report the artifact when it was actually written, so the
        // filenames/sessions arrays stay index-aligned with real files.
        if (WriteRecordJson(filename, doc)) {
          filenames.push_back(filename);
          if (this->session_ids_.find(shell_id) != this->session_ids_.end()) {
            sessions.push_back(this->session_ids_[shell_id]);
          } else {
            sessions.push_back(-1);
          }
        } else {
          LOGE("[TestBench] failed to write record json: " << filename);
        }
      }
    }
    this->ClearRecordingSessionData();
    // send a recordingComplete event after the previous session is fully
    // cleared so the receiver can safely start another recording.
    complete_func(filenames, sessions);
  };

  thread_.GetTaskRunner()->PostTask(std::move(writer_task));
}

void TestBenchBaseRecorder::AddLynxViewSessionID(int64_t record_id,
                                                 int64_t session) {
  session_ids_[record_id] = session;
}

void TestBenchBaseRecorder::RemoveRecord(int64_t record_id) {
  auto remove_record_task = [this, record_id]() {
    lynx_view_table_.erase(record_id);
    replay_config_map_.erase(record_id);
    url_map_.erase(record_id);
    session_ids_.erase(record_id);
    script_cache_.erase(record_id);
    preload_script_cache_.erase(record_id);
    preload_script_paths_cache_.erase(record_id);
  };
  thread_.GetTaskRunner()->PostTask(std::move(remove_record_task));
}

void TestBenchBaseRecorder::RecordAction(const char* function_name,
                                         rapidjson::Value& params,
                                         int64_t record_id) {
  if (record_id == 0) {
    return;
  }
  const uint64_t recording_generation = RecordingGeneration();
  rapidjson::Document owned_params;
  owned_params.CopyFrom(params, owned_params.GetAllocator());
  RecordActionOwned(function_name, std::move(owned_params), record_id,
                    recording_generation);
}

void TestBenchBaseRecorder::RecordActionWithGeneration(
    std::string function_name, rapidjson::Document params, int64_t record_id,
    uint64_t recording_generation) {
  if (record_id == 0 || recording_generation == 0) {
    return;
  }
  RecordActionOwned(std::move(function_name), std::move(params), record_id,
                    recording_generation);
}

void TestBenchBaseRecorder::RecordActionOwned(std::string function_name,
                                              rapidjson::Document params,
                                              int64_t record_id,
                                              uint64_t recording_generation) {
  auto record_action_task = [this, function_name = std::move(function_name),
                             record_id, params = std::move(params),
                             recording_generation]() mutable {
    if (!is_recording_.load(std::memory_order_acquire) ||
        RecordingGeneration() != recording_generation) {
      return;
    }
    rapidjson::Document::AllocatorType& allocator = GetAllocator();
    RecordActionKernel(function_name.c_str(), params, record_id, allocator);
  };

  thread_.GetTaskRunner()->PostTask(std::move(record_action_task));
}

void TestBenchBaseRecorder::RecordActionKernel(
    const char* function_name, const rapidjson::Value& params,
    int64_t record_id, rapidjson::Document::AllocatorType& allocator) {
  rapidjson::Value& action_list_value =
      GetRecordedFileField(record_id, kActionList);
  rapidjson::Value func_name;
  func_name.SetString(function_name, static_cast<int>(strlen(function_name)),
                      allocator);

  rapidjson::Value params_val;
  params_val.CopyFrom(params, allocator);

  rapidjson::Value val;
  val.SetObject();
  val.AddMember(rapidjson::StringRef(kFunctionName), func_name, allocator);

  // Record Time
  RecordTime(val);

  val.AddMember(rapidjson::StringRef(kParams), params_val, allocator);

  action_list_value.PushBack(val, allocator);
}

void TestBenchBaseRecorder::AppendInvokedMethodData(
    rapidjson::Value& recorded_file, const std::string& module_name,
    const std::string& method_name, const rapidjson::Value& params) {
  rapidjson::Value& invoked_method_data_value =
      recorded_file[kInvokedMethodData];
  rapidjson::Document::AllocatorType& allocator = GetAllocator();

  rapidjson::Value module_name_val(rapidjson::kStringType);
  module_name_val.SetString(module_name.c_str(), allocator);

  rapidjson::Value method_name_val(rapidjson::kStringType);
  method_name_val.SetString(method_name.c_str(), allocator);

  rapidjson::Value val;
  val.SetObject();
  val.AddMember(rapidjson::StringRef(kModuleName), module_name_val, allocator);
  val.AddMember(rapidjson::StringRef(kMethodName), method_name_val, allocator);

  rapidjson::Value params_val;
  params_val.CopyFrom(params, allocator);

  RecordTime(val);

  val.AddMember(rapidjson::StringRef(kParams), params_val, allocator);
  invoked_method_data_value.PushBack(val, allocator);
}

void TestBenchBaseRecorder::AppendCallbackData(rapidjson::Value& recorded_file,
                                               const std::string& module_name,
                                               const std::string& method_name,
                                               const rapidjson::Value& params,
                                               int64_t callback_id) {
  rapidjson::Value& callback_value = recorded_file[kCallback];
  rapidjson::Document::AllocatorType& allocator = GetAllocator();

  rapidjson::Value callback;
  callback.SetString(std::to_string(callback_id).c_str(), allocator);

  rapidjson::Value module_name_val(rapidjson::kStringType);
  module_name_val.SetString(module_name.c_str(), allocator);

  rapidjson::Value method_name_val(rapidjson::kStringType);
  method_name_val.SetString(method_name.c_str(), allocator);

  rapidjson::Value val;
  val.SetObject();
  val.AddMember(rapidjson::StringRef(kModuleName), module_name_val, allocator);
  val.AddMember(rapidjson::StringRef(kMethodName), method_name_val, allocator);

  RecordTime(val);

  rapidjson::Value local_params;
  local_params.CopyFrom(params, allocator);
  val.AddMember(rapidjson::StringRef(kParams), local_params, allocator);
  auto existing = callback_value.FindMember(callback.GetString());
  if (existing == callback_value.MemberEnd()) {
    callback_value.AddMember(callback, val, allocator);
    return;
  }
  if (existing->value.IsArray()) {
    existing->value.PushBack(val, allocator);
    return;
  }

  // Keep the legacy object shape for one callback. Promote only reused IDs so
  // every response is preserved without changing existing recording files.
  rapidjson::Value candidates(rapidjson::kArrayType);
  rapidjson::Value previous;
  previous.CopyFrom(existing->value, allocator);
  candidates.PushBack(previous, allocator);
  candidates.PushBack(val, allocator);
  existing->value.Swap(candidates);
}

void TestBenchBaseRecorder::RecordInvokedMethodData(const char* module_name,
                                                    const char* method_name,
                                                    rapidjson::Value& params,
                                                    int64_t record_id) {
  if (record_id == 0) {
    return;
  }
  const uint64_t recording_generation = RecordingGeneration();
  rapidjson::Document owned_params;
  owned_params.CopyFrom(params, owned_params.GetAllocator());
  RecordInvokedMethodDataOwned(module_name, method_name,
                               std::move(owned_params), record_id,
                               recording_generation);
}

void TestBenchBaseRecorder::RecordInvokedMethodDataWithGeneration(
    std::string module_name, std::string method_name,
    rapidjson::Document params, int64_t record_id,
    uint64_t recording_generation) {
  if (record_id == 0 || recording_generation == 0) {
    return;
  }
  RecordInvokedMethodDataOwned(std::move(module_name), std::move(method_name),
                               std::move(params), record_id,
                               recording_generation);
}

void TestBenchBaseRecorder::RecordInvokedMethodDataOwned(
    std::string module_name, std::string method_name,
    rapidjson::Document params, int64_t record_id,
    uint64_t recording_generation) {
  auto record_invoked_method_task = [this, module_name = std::move(module_name),
                                     method_name = std::move(method_name),
                                     params = std::move(params), record_id,
                                     recording_generation]() mutable {
    if (!is_recording_.load(std::memory_order_acquire) ||
        RecordingGeneration() != recording_generation) {
      return;
    }
    rapidjson::Value& recorded_file = GetRecordedFile(record_id);
    AppendInvokedMethodData(recorded_file, module_name, method_name, params);
  };

  thread_.GetTaskRunner()->PostTask(std::move(record_invoked_method_task));
}

void TestBenchBaseRecorder::RecordCallback(const char* module_name,
                                           const char* method_name,
                                           rapidjson::Value& params,
                                           int64_t callback_id,
                                           int64_t record_id) {
  if (record_id == 0) {
    return;
  }
  const uint64_t recording_generation = RecordingGeneration();
  rapidjson::Document owned_params;
  owned_params.CopyFrom(params, owned_params.GetAllocator());
  RecordCallbackOwned(module_name, method_name, std::move(owned_params),
                      callback_id, record_id, recording_generation);
}

void TestBenchBaseRecorder::RecordCallbackWithGeneration(
    std::string module_name, std::string method_name,
    rapidjson::Document params, int64_t callback_id, int64_t record_id,
    uint64_t recording_generation) {
  if (record_id == 0 || recording_generation == 0) {
    return;
  }
  RecordCallbackOwned(std::move(module_name), std::move(method_name),
                      std::move(params), callback_id, record_id,
                      recording_generation);
}

void TestBenchBaseRecorder::RecordCallbackOwned(std::string module_name,
                                                std::string method_name,
                                                rapidjson::Document params,
                                                int64_t callback_id,
                                                int64_t record_id,
                                                uint64_t recording_generation) {
  auto record_callback_task = [this, module_name = std::move(module_name),
                               method_name = std::move(method_name),
                               params = std::move(params), callback_id,
                               record_id, recording_generation]() mutable {
    if (!is_recording_.load(std::memory_order_acquire) ||
        RecordingGeneration() != recording_generation) {
      return;
    }
    rapidjson::Value& recorded_file = GetRecordedFile(record_id);
    AppendCallbackData(recorded_file, module_name, method_name, params,
                       callback_id);
  };
  thread_.GetTaskRunner()->PostTask(std::move(record_callback_task));
}

void TestBenchBaseRecorder::RecordComponent(const char* component_name,
                                            int type, int64_t record_id) {
  auto record_component_task =
      [this, component_name = std::string(component_name), type, record_id]() {
        if (!is_recording_.load(std::memory_order_acquire)) {
          return;
        }
        if (lynx_view_table_.count(record_id) == 0) {
          return;
        }
        rapidjson::Value& component_list_value =
            GetRecordedFileField(record_id, kComponentList);
        rapidjson::Document::AllocatorType& allocator = GetAllocator();

        rapidjson::Value component_name_val(rapidjson::kStringType);
        component_name_val.SetString(component_name.c_str(), allocator);

        rapidjson::Value component_type_val(rapidjson::kNumberType);
        component_type_val.SetInt(type);

        rapidjson::Value val;
        val.SetObject();

        val.AddMember(rapidjson::StringRef(kComponentName), component_name_val,
                      allocator);
        val.AddMember(rapidjson::StringRef(kComponentType), component_type_val,
                      allocator);

        component_list_value.PushBack(val, allocator);
      };
  thread_.GetTaskRunner()->PostTask(std::move(record_component_task));
}

void TestBenchBaseRecorder::RecordDebugInfo(int64_t record_id,
                                            const std::string& url,
                                            const std::string& content) {
  auto record_debug_info_task = [this, record_id, url, content]() {
    if (!is_recording_.load(std::memory_order_acquire)) {
      return;
    }
    if (lynx_view_table_.count(record_id) == 0) {
      return;
    }
    rapidjson::Value& debug_info_value =
        GetRecordedFileField(record_id, kDebugInfo);
    rapidjson::Document::AllocatorType& allocator = GetAllocator();

    rapidjson::Value url_val(rapidjson::kStringType);
    url_val.SetString(url.c_str(), allocator);

    rapidjson::Value content_val(rapidjson::kStringType);
    // compress content for large data
    const std::string encoded_content =
        CompressToBase64String(content.data(), content.length());
    if (!encoded_content.empty()) {
      content_val.SetString(encoded_content.c_str(), allocator);
    }

    rapidjson::Value val;
    val.SetObject();

    val.AddMember(rapidjson::StringRef(kParamDebugInfoUrl), url_val, allocator);
    val.AddMember(rapidjson::StringRef(kParamContent), content_val, allocator);

    debug_info_value.PushBack(val, allocator);
  };
  thread_.GetTaskRunner()->PostTask(std::move(record_debug_info_task));
}

bool TestBenchBaseRecorder::TryRecordExternalScriptUrl(int64_t record_id,
                                                       const std::string& url) {
  std::lock_guard<std::mutex> lock(recorded_external_script_urls_mutex_);
  return recorded_external_script_urls_[record_id].insert(url).second;
}

void TestBenchBaseRecorder::RecordScripts(const std::string& url,
                                          const std::string& source,
                                          int64_t record_id) {
  if (record_id == 0) {
    return;
  }
  auto record_scripts_task = [this, url, source, record_id]() {
    std::string encoded_source =
        CompressAndBase64Encode(source.data(), source.size());
    if (encoded_source.empty()) {
      return;
    }
    script_cache_[record_id][url] = encoded_source;

    if (!is_recording_.load(std::memory_order_acquire)) {
      return;
    }
    rapidjson::Value& scripts = GetRecordedFileField(record_id, kScripts);
    rapidjson::Document::AllocatorType& allocator = GetAllocator();
    SetScriptValue(scripts, url, encoded_source, allocator);
  };
  thread_.GetTaskRunner()->PostTask(std::move(record_scripts_task));
}

void TestBenchBaseRecorder::RecordExternalTemplate(const std::string& url,
                                                   const std::string& source,
                                                   int64_t record_id) {
  uint64_t recording_generation = RecordingGeneration();
  if (record_id == 0 || recording_generation == 0 || !IsRecordingProcess() ||
      RecordingGeneration() != recording_generation) {
    return;
  }

  auto record_template_task = [this, url, source, record_id,
                               recording_generation]() {
    if (!is_recording_.load(std::memory_order_acquire) ||
        RecordingGeneration() != recording_generation) {
      return;
    }
    rapidjson::Document::AllocatorType& allocator = GetAllocator();
    rapidjson::Value params(rapidjson::kObjectType);
    params.AddMember(
        rapidjson::StringRef("url"),
        rapidjson::Value(url.c_str(),
                         static_cast<rapidjson::SizeType>(url.size()),
                         allocator),
        allocator);

    const std::string encoded_source =
        Base64Encode(source.data(), source.size());
    params.AddMember(
        rapidjson::StringRef("source"),
        rapidjson::Value(
            encoded_source.data(),
            static_cast<rapidjson::SizeType>(encoded_source.size()), allocator),
        allocator);
    params.AddMember(rapidjson::StringRef("templateData"),
                     rapidjson::Value(rapidjson::kObjectType), allocator);
    params.AddMember(rapidjson::StringRef("isCSR"), true, allocator);
    RecordActionKernel("loadTemplate", std::move(params), record_id, allocator);
  };
  thread_.GetTaskRunner()->PostTask(std::move(record_template_task));
}

void TestBenchBaseRecorder::RecordExternalScript(const std::string& url,
                                                 const std::string& source) {
  uint64_t recording_generation = RecordingGeneration();
  if (recording_generation == 0 || !IsRecordingProcess() ||
      RecordingGeneration() != recording_generation) {
    return;
  }
  auto record_script_task = [this, url, source, recording_generation]() {
    if (!is_recording_.load(std::memory_order_acquire) ||
        RecordingGeneration() != recording_generation) {
      return;
    }
    std::string encoded_source =
        CompressAndBase64Encode(source.data(), source.size());
    if (encoded_source.empty()) {
      return;
    }
    external_script_cache_[url] = encoded_source;

    rapidjson::Document::AllocatorType& allocator = GetAllocator();
    for (auto& entry : lynx_view_table_) {
      SetScriptValue(entry.second[kScripts], url, encoded_source, allocator);
    }
  };
  thread_.GetTaskRunner()->PostTask(std::move(record_script_task));
}

void TestBenchBaseRecorder::RecordPreloadScript(const std::string& url,
                                                const std::string& source,
                                                int64_t record_id) {
  if (record_id == 0) {
    return;
  }
  auto record_preload_script_task = [this, url, source, record_id]() {
    std::string encoded_source =
        CompressAndBase64Encode(source.data(), source.size());
    if (encoded_source.empty()) {
      return;
    }

    auto& scripts = preload_script_cache_[record_id];
    const bool is_new_script = scripts.find(url) == scripts.end();
    scripts[url] = encoded_source;
    if (is_new_script) {
      preload_script_paths_cache_[record_id].push_back(url);
    }

    if (!is_recording_.load(std::memory_order_acquire)) {
      return;
    }
    if (lynx_view_table_.count(record_id) == 0) {
      return;
    }
    rapidjson::Document::AllocatorType& allocator = GetAllocator();
    rapidjson::Value& preload_scripts =
        GetRecordedFileField(record_id, kPreloadScripts);
    SetScriptValue(preload_scripts, url, encoded_source, allocator);
    rapidjson::Value& preload_script_paths =
        GetRecordedFileField(record_id, kPreloadScriptPaths);
    AppendUniqueString(preload_script_paths, url, allocator);
  };
  thread_.GetTaskRunner()->PostTask(std::move(record_preload_script_task));
}

void TestBenchBaseRecorder::RecordSharedData(const std::string& key,
                                             rapidjson::Value& value,
                                             int64_t record_id) {
  rapidjson::Document owned_value;
  owned_value.CopyFrom(value, owned_value.GetAllocator());
  auto record_shared_data_task = [this, key, value = std::move(owned_value),
                                  record_id]() {
    if (!is_recording_.load(std::memory_order_acquire)) {
      return;
    }
    if (lynx_view_table_.count(record_id) == 0) {
      return;
    }
    rapidjson::Value& shared_data_map =
        GetRecordedFileField(record_id, kSharedData);

    rapidjson::Document::AllocatorType& allocator = GetAllocator();

    rapidjson::Value local_value(rapidjson::kObjectType);
    local_value.CopyFrom(value, allocator);

    rapidjson::Value json_key(rapidjson::kStringType);
    json_key.SetString(key.c_str(), allocator);

    shared_data_map.AddMember(json_key, local_value, allocator);
  };
  thread_.GetTaskRunner()->PostTask(std::move(record_shared_data_task));
}

void TestBenchBaseRecorder::RecordTime(rapidjson::Value& val) {
  rapidjson::Document::AllocatorType& allocator = GetAllocator();
  // Sample the clock once: the seconds member derives from the same
  // millisecond sample, so the two fields can never disagree.
  const int64_t millis = CurrentRecordMillis();
  rapidjson::Value time_val;
  time_val.SetString(std::to_string(millis / 1000).c_str(), allocator);
  val.AddMember(rapidjson::StringRef(kParamRecordTime), time_val, allocator);

  // record Millisecond
  rapidjson::Value m_time_val;
  m_time_val.SetInt64(millis);
  val.AddMember(rapidjson::StringRef(kParamRecordMillisecond), m_time_val,
                allocator);
}

rapidjson::Value& TestBenchBaseRecorder::GetRecordedFileField(
    int64_t record_id, const std::string& filed_name) {
  rapidjson::Value& tmp_value = GetRecordedFile(record_id);
  rapidjson::Value& action_list_value = tmp_value[filed_name];
  return action_list_value;
}

rapidjson::Value& TestBenchBaseRecorder::GetRecordedFile(int64_t record_id) {
  if (lynx_view_table_.find(record_id) == lynx_view_table_.end()) {
    CreateRecordedFile(record_id);
  }
  return lynx_view_table_[record_id];
}

void TestBenchBaseRecorder::CreateRecordedFile(int64_t record_id) {
  rapidjson::Document::AllocatorType& allocator = GetAllocator();
  // document
  rapidjson::Value dump_document;
  dump_document.SetObject();

  // action list
  rapidjson::Value action_list_value;
  action_list_value.SetArray();
  dump_document.AddMember(rapidjson::StringRef(kActionList), action_list_value,
                          allocator);

  // Invoked Method Data
  rapidjson::Value invoked_method_data_value;
  invoked_method_data_value.SetArray();
  dump_document.AddMember(rapidjson::StringRef(kInvokedMethodData),
                          invoked_method_data_value, allocator);

  // callback
  rapidjson::Value callback_value;
  callback_value.SetObject();
  dump_document.AddMember(rapidjson::StringRef(kCallback), callback_value,
                          allocator);

  // component
  rapidjson::Value component_list_value;
  component_list_value.SetArray();
  dump_document.AddMember(rapidjson::StringRef(kComponentList),
                          component_list_value, allocator);

  // debug_info
  rapidjson::Value debug_info_value;
  debug_info_value.SetArray();
  dump_document.AddMember(rapidjson::StringRef(kDebugInfo), debug_info_value,
                          allocator);
  // sharedData
  rapidjson::Value shared_data;
  shared_data.SetObject();
  dump_document.AddMember(rapidjson::StringRef(kSharedData), shared_data,
                          allocator);
  // scripts
  rapidjson::Value scripts;
  scripts.SetObject();
  for (const auto& script : external_script_cache_) {
    SetScriptValue(scripts, script.first, script.second, allocator);
  }
  auto script_cache = script_cache_.find(record_id);
  if (script_cache != script_cache_.end()) {
    for (const auto& script : script_cache->second) {
      SetScriptValue(scripts, script.first, script.second, allocator);
    }
  }
  dump_document.AddMember(rapidjson::StringRef(kScripts), scripts, allocator);

  rapidjson::Value preload_scripts;
  preload_scripts.SetObject();
  auto preload_script_cache = preload_script_cache_.find(record_id);
  if (preload_script_cache != preload_script_cache_.end()) {
    for (const auto& script : preload_script_cache->second) {
      SetScriptValue(preload_scripts, script.first, script.second, allocator);
    }
  }
  dump_document.AddMember(rapidjson::StringRef(kPreloadScripts),
                          preload_scripts, allocator);

  rapidjson::Value preload_script_paths;
  preload_script_paths.SetArray();
  auto preload_script_paths_cache = preload_script_paths_cache_.find(record_id);
  if (preload_script_paths_cache != preload_script_paths_cache_.end()) {
    for (const auto& path : preload_script_paths_cache->second) {
      AppendUniqueString(preload_script_paths, path, allocator);
    }
  }
  dump_document.AddMember(rapidjson::StringRef(kPreloadScriptPaths),
                          preload_script_paths, allocator);

  lynx_view_table_[record_id] = dump_document;
}

void TestBenchBaseRecorder::ClearRecordingSessionData() {
  lynx_view_table_.clear();
  resource_table_.SetNull();
  {
    std::lock_guard<std::mutex> lock(recorded_external_script_urls_mutex_);
    recorded_external_script_urls_.clear();
  }
  external_script_cache_.clear();
  GetAllocator().Clear();
}

void TestBenchBaseRecorder::ClearRecordedData() {
  ClearRecordingSessionData();
  replay_config_map_.clear();
  url_map_.clear();
  session_ids_.clear();
}

void TestBenchBaseRecorder::ResetForTesting() {
  is_recording_.store(false, std::memory_order_release);
  ClearRecordedData();
  script_cache_.clear();
  preload_script_cache_.clear();
  preload_script_paths_cache_.clear();
}

}  // namespace recorder
}  // namespace tasm
}  // namespace lynx

extern "C" void LynxTestBenchRecordExternalScript(const char* url,
                                                  const char* source) {
#if ENABLE_TESTBENCH_RECORDER
  if (url == nullptr || url[0] == '\0' || source == nullptr ||
      source[0] == '\0') {
    return;
  }
  lynx::tasm::recorder::TestBenchBaseRecorder::GetInstance()
      .RecordExternalScript(url, source);
#endif
}

extern "C" void LynxTestBenchRecordExternalScriptWithSize(const char* url,
                                                          const char* source,
                                                          size_t source_size) {
#if ENABLE_TESTBENCH_RECORDER
  if (url == nullptr || url[0] == '\0' || source == nullptr ||
      source_size == 0) {
    return;
  }
  lynx::tasm::recorder::TestBenchBaseRecorder::GetInstance()
      .RecordExternalScript(url, std::string(source, source_size));
#endif
}

extern "C" void LynxTestBenchRecordExternalTemplateWithSize(
    int64_t record_id, const char* url, const char* source,
    size_t source_size) {
#if ENABLE_TESTBENCH_RECORDER
  if (record_id == 0 || url == nullptr || url[0] == '\0' || source == nullptr ||
      source_size == 0) {
    return;
  }
  lynx::tasm::recorder::TestBenchBaseRecorder::GetInstance()
      .RecordExternalTemplate(url, std::string(source, source_size), record_id);
#endif
}
