// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/recorder/testbench_base_recorder.h"

#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>

#include "base/include/closure.h"
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

std::unique_ptr<Byte[]> Compress(const char* source, size_t source_size,
                                 unsigned long* compressed_size_in) {
  *compressed_size_in = compressBound(source_size);
  std::unique_ptr<Byte[]> compressed_data =
      std::make_unique<Byte[]>(*compressed_size_in);
  int z_result =
      compress(compressed_data.get(), compressed_size_in,
               reinterpret_cast<const Cr_z_Bytef*>(source), source_size);
  if (z_result == Z_OK) {
    return compressed_data;
  } else {
    return nullptr;
  }
}

std::unique_ptr<char[]> ModpB64Encode(const char* source, size_t source_size,
                                      unsigned long* base64_size_in) {
  *base64_size_in = modp_b64_encode_len(source_size);
  std::unique_ptr<char[]> base64_data =
      std::make_unique<char[]>(*base64_size_in);
  *base64_size_in = modp_b64_encode(base64_data.get(), source, source_size);
  return base64_data;
}

TestBenchBaseRecorder::TestBenchBaseRecorder() : thread_("ark_recorder") {
  is_recording_ = false;
}

void TestBenchBaseRecorder::SetRecorderPath(const std::string& path) {
  file_path_ = path;
  file_path_ += "/";
}

void TestBenchBaseRecorder::SetScreenSize(int64_t record_id, float screen_width,
                                          float screen_height) {
  InsertReplayConfig(record_id, "screenWidth", screen_width);
  InsertReplayConfig(record_id, "screenHeight", screen_height);
}

template <typename T>
void TestBenchBaseRecorder::InsertReplayConfig(int64_t record_id,
                                               const char* name, T value) {
  rapidjson::Document::AllocatorType& allocator = GetAllocator();
  if (replay_config_map_.count(record_id) == 0) {
    rapidjson::Value config_object;
    config_object.SetObject();
    // add jsb ignored info
    rapidjson::Document jsb_ignored_info;
    jsb_ignored_info.Parse(KJsbIgnoredInfo);
    config_object.AddMember(rapidjson::StringRef("jsbIgnoredInfo"),
                            jsb_ignored_info, allocator);
    rapidjson::Value jsb_settings;
    jsb_settings.SetObject();
    jsb_settings.AddMember(rapidjson::StringRef("strict"), true, allocator);
    config_object.AddMember(rapidjson::StringRef("jsbSettings"), jsb_settings,
                            allocator);
    replay_config_map_[record_id] = config_object;
  }
  rapidjson::Value& config = replay_config_map_[record_id];
  config.AddMember(rapidjson::StringRef(name), value, allocator);
}

TestBenchBaseRecorder& TestBenchBaseRecorder::GetInstance() {
  static base::NoDestructor<TestBenchBaseRecorder> instance_;
  return *instance_;
}

bool TestBenchBaseRecorder::IsRecordingProcess() { return is_recording_; }

rapidjson::Document::AllocatorType& TestBenchBaseRecorder::GetAllocator() {
  return dumped_document.GetAllocator();
};

void TestBenchBaseRecorder::StartRecord() { is_recording_ = true; }

void TestBenchBaseRecorder::EndRecord(
    base::MoveOnlyClosure<void, std::vector<std::string>&,
                          std::vector<int64_t>&>
        send_complete) {
  auto writer_task = [this,
                      complete_func = std::move(send_complete)]() mutable {
    if (!is_recording_) {
      return;
    }
    is_recording_ = false;
    std::vector<std::string> filenames;
    std::vector<int64_t> sessions;
    for (auto& lynx_view_pair : lynx_view_table_) {
      int64_t shell_id = lynx_view_pair.first;
      rapidjson::Value& config = replay_config_map_[shell_id];
      std::string filename = file_path_ + std::to_string(shell_id) + ".json";
      {
        std::ofstream ifs;
        ifs.open(filename, std::ios::binary | std::ios::out);
        if (ifs.is_open()) {
          rapidjson::Value& doc = lynx_view_pair.second;
          rapidjson::Document::AllocatorType& allocator = GetAllocator();
          doc.AddMember(rapidjson::StringRef(kScripts), scripts_table_,
                        allocator);
          doc.AddMember(rapidjson::StringRef(kConfig), config, allocator);
          rapidjson::StringBuffer os;
          rapidjson::Writer<rapidjson::StringBuffer, rapidjson::UTF8<>,
                            rapidjson::UTF8<>, rapidjson::CrtAllocator,
                            rapidjson::kWriteNanAndInfFlag>
              writer(os);
          doc.Accept(writer);
          unsigned long compressed_size;
          std::unique_ptr<Byte[]> compressed_data =
              Compress(os.GetString(), os.GetSize(), &compressed_size);
          if (compressed_data != nullptr) {
            unsigned long base64_size;
            std::unique_ptr<char[]> base64_data = ModpB64Encode(
                reinterpret_cast<const char*>(compressed_data.get()),
                compressed_size, &base64_size);
            ifs.write(base64_data.get(), base64_size);
            ifs.flush();
          }
          ifs.close();
        }
      }
      if (this->session_ids_.find(shell_id) != this->session_ids_.end()) {
        sessions.push_back(this->session_ids_[shell_id]);
      } else {
        sessions.push_back(-1);
      }
      filenames.push_back(filename);
    }
    // send a recordingComplete event
    complete_func(filenames, sessions);
    this->Clear();
  };

  thread_.GetTaskRunner()->PostTask(std::move(writer_task));
}

void TestBenchBaseRecorder::AddLynxViewSessionID(int64_t record_id,
                                                 int64_t session) {
  session_ids_[record_id] = session;
}

void TestBenchBaseRecorder::RecordAction(const char* function_name,
                                         rapidjson::Value& params,
                                         int64_t record_id) {
  auto record_action_task =
      [this, function_name = std::string(function_name), record_id,
       params = rapidjson::Value(params, GetAllocator())]() {
        if (!is_recording_) {
          return;
        }
        rapidjson::Document::AllocatorType& allocator = GetAllocator();
        RecordActionKernel(function_name.c_str(),
                           rapidjson::Value(params, GetAllocator()), record_id,
                           allocator);
      };

  thread_.GetTaskRunner()->PostTask(std::move(record_action_task));
}

void TestBenchBaseRecorder::RecordActionKernel(
    const char* function_name, rapidjson::Value params, int64_t record_id,
    rapidjson::Document::AllocatorType& allocator) {
  rapidjson::Value& action_list_value =
      GetRecordedFileField(record_id, kActionList);
  rapidjson::Value func_name;
  func_name.SetString(function_name, static_cast<int>(strlen(function_name)),
                      allocator);

  rapidjson::Value params_val;
  params_val.SetObject();
  params_val.Swap(params);

  rapidjson::Value val;
  val.SetObject();
  val.AddMember(rapidjson::StringRef(kFunctionName), func_name, allocator);

  // Record Time
  RecordTime(val);

  val.AddMember(rapidjson::StringRef(kParams), params_val, allocator);

  action_list_value.PushBack(val, allocator);
}

void TestBenchBaseRecorder::RecordInvokedMethodData(const char* module_name,
                                                    const char* method_name,
                                                    rapidjson::Value& params,
                                                    int64_t record_id) {
  auto record_invoked_method_task =
      [this, module_name = std::string(module_name),
       method_name = std::string(method_name),
       params = rapidjson::Value(params, GetAllocator()), record_id]() {
        if (!is_recording_) {
          return;
        }
        if (lynx_view_table_.count(record_id) == 0) {
          return;
        }

        rapidjson::Value& tmp_value = lynx_view_table_[record_id];
        rapidjson::Value& invoked_method_data_value =
            tmp_value[kInvokedMethodData];

        rapidjson::Document::AllocatorType& allocator = GetAllocator();

        rapidjson::Value module_name_val(rapidjson::kStringType);
        module_name_val.SetString(module_name.c_str(), allocator);

        rapidjson::Value method_name_val(rapidjson::kStringType);
        method_name_val.SetString(method_name.c_str(), allocator);

        rapidjson::Value val;
        val.SetObject();
        val.AddMember(rapidjson::StringRef(kModuleName), module_name_val,
                      allocator);
        val.AddMember(rapidjson::StringRef(kMethodName), method_name_val,
                      allocator);

        rapidjson::Value params_val;
        params_val.SetObject();
        params_val.CopyFrom(params, allocator);

        // Record Time
        RecordTime(val);

        val.AddMember(rapidjson::StringRef(kParams), params_val, allocator);
        invoked_method_data_value.PushBack(val, allocator);
      };

  thread_.GetTaskRunner()->PostTask(std::move(record_invoked_method_task));
}

void TestBenchBaseRecorder::RecordCallback(const char* module_name,
                                           const char* method_name,
                                           rapidjson::Value& params,
                                           int64_t callback_id,
                                           int64_t record_id) {
  auto record_callback_task = [this, module_name = std::string(module_name),
                               method_name = std::string(method_name),
                               params =
                                   rapidjson::Value(params, GetAllocator()),
                               callback_id, record_id]() {
    if (!is_recording_) {
      return;
    }
    if (lynx_view_table_.count(record_id) == 0) {
      return;
    }
    rapidjson::Value& callback_value =
        GetRecordedFileField(record_id, kCallback);
    rapidjson::Document::AllocatorType& allocator = GetAllocator();

    rapidjson::Value callback(rapidjson::kObjectType);
    callback.SetString(std::to_string(callback_id).c_str(), allocator);

    rapidjson::Value module_name_val(rapidjson::kStringType);
    module_name_val.SetString(module_name.c_str(), allocator);

    rapidjson::Value method_name_val(rapidjson::kStringType);
    method_name_val.SetString(method_name.c_str(), allocator);

    rapidjson::Value val;
    val.SetObject();

    val.AddMember(rapidjson::StringRef(kModuleName), module_name_val,
                  allocator);
    val.AddMember(rapidjson::StringRef(kMethodName), method_name_val,
                  allocator);

    // Record Time
    RecordTime(val);

    rapidjson::Value local_params(rapidjson::kObjectType);
    local_params.CopyFrom(params, allocator);
    val.AddMember(rapidjson::StringRef(kParams), local_params, allocator);
    callback_value.AddMember(callback, val, allocator);
  };
  thread_.GetTaskRunner()->PostTask(std::move(record_callback_task));
}

void TestBenchBaseRecorder::RecordComponent(const char* component_name,
                                            int type, int64_t record_id) {
  auto record_component_task =
      [this, component_name = std::string(component_name), type, record_id]() {
        if (!is_recording_) {
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

void TestBenchBaseRecorder::RecordScripts(const char* url, const char* source) {
  auto record_scripts_task = [this, url = std::string(url),
                              source = std::string(source)]() {
    if (!is_recording_) {
      return;
    }
    rapidjson::Document::AllocatorType& allocator = GetAllocator();

    rapidjson::Value url_val(rapidjson::kStringType);
    url_val.SetString(url.c_str(), allocator);

    rapidjson::Value source_val(rapidjson::kStringType);

    unsigned long compressed_size;
    std::unique_ptr<Byte[]> compressed_data =
        Compress(source.c_str(), source.length(), &compressed_size);
    if (compressed_data != nullptr) {
      unsigned long base64_size;
      std::unique_ptr<char[]> base64_data =
          ModpB64Encode(reinterpret_cast<const char*>(compressed_data.get()),
                        compressed_size, &base64_size);
      source_val.SetString(base64_data.get(), allocator);
    }
    // scripts_table_ will be reset to null each time recording ends,
    // So it needs to be judged to avoid crash
    if (!scripts_table_.IsObject()) {
      scripts_table_.SetObject();
    }
    scripts_table_.AddMember(url_val, source_val, allocator);
  };
  thread_.GetTaskRunner()->PostTask(std::move(record_scripts_task));
}

void TestBenchBaseRecorder::RecordTime(rapidjson::Value& val) {
  rapidjson::Document::AllocatorType& allocator = GetAllocator();
  std::time_t now_time = std::time(nullptr);
  rapidjson::Value time_val;
  time_val.SetString(std::to_string(now_time).c_str(), allocator);
  val.AddMember(rapidjson::StringRef(kParamRecordTime), time_val, allocator);

  // record Millisecond
  int64_t m_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();
  rapidjson::Value m_time_val;
  time_val.SetInt64(m_time);
  val.AddMember(rapidjson::StringRef(kParamRecordMillisecond), time_val,
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

  lynx_view_table_[record_id] = dump_document;
}

void TestBenchBaseRecorder::Clear() {
  std::unordered_map<int64_t, rapidjson::Value> empty_table;
  this->lynx_view_table_.swap(empty_table);
  GetAllocator().Clear();
}

}  // namespace recorder
}  // namespace tasm
}  // namespace lynx
