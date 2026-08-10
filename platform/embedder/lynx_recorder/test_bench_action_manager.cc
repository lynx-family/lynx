// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_recorder/test_bench_action_manager.h"

#include <algorithm>
#include <future>
#include <memory>
#include <utility>

#include "platform/embedder/lynx_recorder/test_bench_utils.h"

namespace lynx {
namespace embedder {

TestBenchActionManager::TestBenchActionManager(
    std::shared_ptr<lynx::pub::LynxView> view, ResizeCallback resize_callback)
    : lynx_view_(view), resize_callback_(resize_callback) {
  data_module_ = std::make_shared<TestBenchReplayDataModule>();
  data_module_->BindContext(lynx_view_->Impl());
}

void TestBenchActionManager::SetFetchCallback(FetchCallback fetch_callback) {
  // For fetch record file.
  fetch_callback_ = fetch_callback;
}

void TestBenchActionManager::SetTaskScheduler(
    ReplayTaskScheduler task_scheduler) {
  task_scheduler_ = std::move(task_scheduler);
}

void TestBenchActionManager::SetReplayCompleteCallback(
    ReplayCompleteCallback complete_callback) {
  replay_complete_callback_ = std::move(complete_callback);
}

void TestBenchActionManager::StartWithUrl(
    const std::string& url,
    std::shared_ptr<lynx::pub::LynxTemplateData> default_global_props) {
  if (url.empty()) {
    return;
  }
  replay_generation_.fetch_add(1, std::memory_order_relaxed);
  data_module_->Reset();
  preloaded_source_.clear();
  component_list_.clear();
  template_bundle_param_.clear();
  global_props_ = std::move(default_global_props);
  template_bundle_.reset();
  replay_config_ = std::make_unique<embedder::TestBenchReplayConfig>();
  replay_config_->InitWithProductUrl(url);
  if (replay_config_->GetSourceUrl().empty()) {
    FetchRecordFile(replay_config_->GetUrl());
  } else {
    FetchPreloadedSource(replay_config_->GetSourceUrl());
  }
}

void TestBenchActionManager::FetchPreloadedSource(const std::string& url) {
  if (url.empty()) {
    return;
  }
  // Currently, synchronous fetching is adopted. We hope to optimize it in the
  // future.
  std::promise<std::string> fetch_promise;
  auto fetch_future = fetch_promise.get_future();
  fetch_callback_(url, [&fetch_promise](const std::string& data) {
    fetch_promise.set_value(data);
  });
  preloaded_source_ = fetch_future.get();
  FetchRecordFile(replay_config_->GetUrl());
}

void TestBenchActionManager::FetchRecordFile(const std::string& url) {
  if (url.empty()) {
    return;
  }
  // Currently, synchronous fetching is adopted. We hope to optimize it in the
  // future.
  std::promise<std::string> fetch_promise;
  auto fetch_future = fetch_promise.get_future();
  fetch_callback_(url, [&fetch_promise](const std::string& data) {
    fetch_promise.set_value(data);
  });
  std::string record_file = fetch_future.get();
  if (record_file.empty()) {
    // Handle error.
    return;
  }
  size_t raw_json_start = FindRawJsonStart(record_file);
  if (raw_json_start != std::string::npos) {
    HandleRecordFileData(record_file.substr(raw_json_start));
    return;
  }
  std::string decode_result = TestBenchDecode(record_file);
  std::vector<uint8_t> compressed_data(decode_result.begin(),
                                       decode_result.end());
  std::vector<uint8_t> decompressed_data = TestBenchDecompress(compressed_data);
  std::string result_str(decompressed_data.begin(), decompressed_data.end());
  HandleRecordFileData(result_str);
}

void TestBenchActionManager::HandleRecordFileData(const std::string& result) {
  rapidjson::Document dom;
  dom.Parse(result);
  if (dom.HasParseError()) {
    return;
  }
  std::string result_json;
  if (dom.HasMember("Config")) {
    rapidjson::Value& config = dom["Config"];
    if (config.HasMember("jsbIgnoredInfo")) {
      result_json = ToJson(config["jsbIgnoredInfo"]);
      data_module_->SetJsbIgnoredInfo(result_json);
    }
    if (config.HasMember("jsbSettings")) {
      result_json = ToJson(config["jsbSettings"]);
      data_module_->SetJsbSettings(result_json);
    }
  }
  if (dom.HasMember("Invoked Method Data")) {
    result_json = ToJson(dom["Invoked Method Data"]);
    data_module_->SetFunctionCall(result_json);
  }
  if (dom.HasMember("Callback")) {
    result_json = ToJson(dom["Callback"]);
    data_module_->SetCallbackData(result_json);
  }
  if (dom.HasMember("Component List")) {
    component_list_ = ToJson(dom["Component List"]);
  }
  data_module_->MarkReady();

  if (dom.HasMember("Action List")) {
    rapidjson::Value& action_list = dom["Action List"];
    if (CheckFile(action_list)) {
      HandleActionList(action_list);
    }
  }
}

bool TestBenchActionManager::CheckFile(const rapidjson::Value& action_list) {
  if (action_list.GetType() == rapidjson::kArrayType) {
    for (rapidjson::SizeType i = 0; i < action_list.Size(); i++) {
      const rapidjson::Value& obj = action_list[i];
      std::string function_name_str;
      if (obj.HasMember("Function Name") && obj["Function Name"].IsString()) {
        function_name_str = obj["Function Name"].GetString();
      }
      if (function_name_str.compare("loadTemplate") == 0 ||
          function_name_str.compare("loadTemplateBundle") == 0) {
        if (function_name_str.compare("loadTemplateBundle") == 0) {
          if (obj.HasMember("Params")) {
            const rapidjson::Value& params = obj["Params"];
            template_bundle_param_ = ToJson(params);
          }
        }
        return true;
      }
    }
  }
  return false;
}

void TestBenchActionManager::HandleActionList(
    const rapidjson::Value& action_list) {
  if (action_list.GetType() == rapidjson::kArrayType) {
    bool has_replay_start_time = false;
    int64_t replay_start_time = 0;
    int64_t replay_end_interval = 0;
    const uint64_t replay_generation =
        replay_generation_.load(std::memory_order_relaxed);
    for (rapidjson::SizeType i = 0; i < action_list.Size(); i++) {
      const rapidjson::Value& obj = action_list[i];
      std::string function_name_str;
      if (obj.HasMember("Function Name") && obj["Function Name"].IsString()) {
        function_name_str = obj["Function Name"].GetString();
      }
      if (function_name_str.compare("updateViewPort") == 0 ||
          function_name_str.compare("setThreadStrategy") == 0) {
        function_name_str = "initialLynxView";
      }
      if (!replay_config_->CheckCanMockFuncName(function_name_str)) {
        continue;
      }
      if (function_name_str.compare("SendBubbleEvent") == 0 &&
          !replay_config_->GetReplayGesture()) {
        continue;
      }
      int64_t record_time = 0;
      if (obj.HasMember("Record Time") && obj["Record Time"].IsString()) {
        std::string record_time_str = obj["Record Time"].GetString();
        int record_time_value = 0;
        StringToInt(record_time_str, &record_time_value, 10);
        record_time = static_cast<int64_t>(record_time_value) * 1000;
      }
      if (obj.HasMember("RecordMillisecond") &&
          obj["RecordMillisecond"].IsInt64()) {
        record_time = obj["RecordMillisecond"].GetInt64();
      }
      std::string param;
      if (obj.HasMember("Params")) {
        param = ToJson(obj["Params"]);
      }
      if (function_name_str.compare("fromTemplate") == 0) {
        param = template_bundle_param_;
      }
      if (!has_replay_start_time) {
        has_replay_start_time = true;
        replay_start_time = record_time;
      }
      ReplayAction replay_action;
      replay_action.interval =
          std::max<int64_t>(record_time - replay_start_time, 0);
      replay_action.function_id =
          replay_config_->GetCanMockFuncId(function_name_str);
      replay_action.params = param;
      replay_end_interval =
          std::max(replay_end_interval, replay_action.interval);
      DispatchAction(replay_action, replay_generation);
    }
    DispatchReplayComplete(
        replay_end_interval +
            std::max<int64_t>(replay_config_->GetDelayEndInterval(), 0),
        replay_generation);
  }
}

void TestBenchActionManager::DispatchAction(const ReplayAction& action,
                                            uint64_t replay_generation) {
  if (action.interval <= 0 || !task_scheduler_) {
    DoAction(action);
    return;
  }

  std::weak_ptr<TestBenchActionManager> weak_self = weak_from_this();
  task_scheduler_(
      [weak_self, replay_generation, action]() {
        std::shared_ptr<TestBenchActionManager> self = weak_self.lock();
        if (!self || self->replay_generation_.load(std::memory_order_relaxed) !=
                         replay_generation) {
          return;
        }
        self->DoAction(action);
      },
      action.interval);
}

void TestBenchActionManager::DispatchReplayComplete(
    int64_t delay_ms, uint64_t replay_generation) {
  if (!replay_complete_callback_) {
    return;
  }
  std::weak_ptr<TestBenchActionManager> weak_self = weak_from_this();
  auto complete = [weak_self, replay_generation]() {
    std::shared_ptr<TestBenchActionManager> self = weak_self.lock();
    if (!self || self->replay_generation_.load(std::memory_order_relaxed) !=
                     replay_generation) {
      return;
    }
    ReplayCompleteCallback callback = self->replay_complete_callback_;
    if (callback) {
      callback();
    }
  };
  if (delay_ms <= 0 || !task_scheduler_) {
    complete();
    return;
  }
  task_scheduler_(std::move(complete), delay_ms);
}

void TestBenchActionManager::DoAction(const ReplayAction& action) {
  std::string params = action.params;
  switch (action.function_id) {
    case INITIAL_LYNX_VIEW:
      InitialLynxView(params);
      break;
    case SET_GLOBAL_PROPS:
      SetGlobalProps(params);
      break;
    case LOAD_TEMPLATE:
      LoadTemplate(params);
      break;
    case LOAD_TEMPLATE_BUNDLE:
      LoadTemplateBundle(params);
      break;
    case UPDATE_DATA_BY_PRE_PARSED_DATA:
      UpdatePreData(params);
      break;
    case SEND_GLOBAL_EVENT:
      SendGlobalEvent(params);
      break;
    case RELOAD_TEMPLATE:
      ReloadTemplate(params);
      break;
    case UPDATE_CONFIG:
      UpdateConfig(params);
      break;
    case FROM_TEMPLATE:
      FromTemplate(params);
      break;
    case SEND_TOUCH_EVENT:
      SendTouchEvent(params);
      break;
    case SEND_BUBBLE_EVENT:
      SendBubbleEvent(params);
      break;
    case SEND_CUSTOM_EVENT:
      SendCustomEvent(params);
      break;
      break;
    default:
      break;
  }
}

void TestBenchActionManager::InitialLynxView(const std::string& param) {
  // init lynx view with size
  rapidjson::Document dom;
  dom.Parse(param);
  if (dom.HasParseError()) {
    return;
  }
  double preferred_layout_height = 0;
  double preferred_layout_width = 0;
  if (dom.HasMember("preferredLayoutHeight") &&
      dom["preferredLayoutHeight"].IsNumber()) {
    preferred_layout_height = dom["preferredLayoutHeight"].GetDouble();
  }
  if (dom.HasMember("preferredLayoutWidth") &&
      dom["preferredLayoutWidth"].IsNumber()) {
    preferred_layout_width = dom["preferredLayoutWidth"].GetDouble();
  }
  if (preferred_layout_height == 0 || preferred_layout_width == 0) {
    return;
  }
  if (resize_callback_) {
    resize_callback_(preferred_layout_width, preferred_layout_height);
  }
}

void TestBenchActionManager::SetGlobalProps(const std::string& param) {
  rapidjson::Document dom;
  dom.Parse(param);
  if (dom.HasParseError()) {
    return;
  }
  if (dom.HasMember("global_props")) {
    global_props_ = std::make_shared<lynx::pub::LynxTemplateData>(
        ToJson(dom["global_props"]));
    auto update_meta = std::make_shared<lynx::pub::LynxUpdateMeta>();
    update_meta->SetGlobalProps(global_props_);
    lynx_view_->UpdateData(update_meta);
  }
}

void TestBenchActionManager::LoadTemplate(const std::string& param) {
  rapidjson::Document dom;
  dom.Parse(param);
  if (dom.HasParseError()) {
    return;
  }
  std::string source = preloaded_source_;
  if (preloaded_source_.empty() && dom.HasMember("source") &&
      dom["source"].IsString()) {
    source = TestBenchDecode(dom["source"].GetString());
  }
  std::shared_ptr<lynx::pub::LynxTemplateData> template_data;
  if (dom.HasMember("templateData")) {
    const rapidjson::Value& init_data = dom["templateData"];
    template_data =
        std::make_shared<lynx::pub::LynxTemplateData>(ToJson(init_data));
    if (init_data.HasMember("readOnly") && init_data["readOnly"].IsBool() &&
        init_data["readOnly"].GetBool()) {
      template_data->MarkReadOnly();
    }
    if (init_data.HasMember("preprocessorName") &&
        init_data["preprocessorName"].IsString()) {
      template_data->MarkState(init_data["preprocessorName"].GetString());
    }
  }
  std::vector<uint8_t> vec(source.begin(), source.end());
  auto load_meta = std::make_shared<lynx::pub::LynxLoadMeta>();
  load_meta->SetBinaryData(vec);
  if (template_data) {
    load_meta->SetInitialData(template_data);
  }
  if (global_props_) {
    load_meta->SetGlobalProps(global_props_);
  }
  lynx_view_->LoadTemplate(load_meta);
}

void TestBenchActionManager::LoadTemplateBundle(const std::string& param) {
  rapidjson::Document dom;
  dom.Parse(param);
  if (dom.HasParseError()) {
    return;
  }
  std::shared_ptr<lynx::pub::LynxTemplateData> template_data;
  if (dom.HasMember("templateData")) {
    const rapidjson::Value& init_data = dom["templateData"];
    template_data =
        std::make_shared<lynx::pub::LynxTemplateData>(ToJson(init_data));
    if (init_data.HasMember("readOnly") && init_data["readOnly"].IsBool() &&
        init_data["readOnly"].GetBool()) {
      template_data->MarkReadOnly();
    }
    if (init_data.HasMember("preprocessorName") &&
        init_data["preprocessorName"].IsString()) {
      template_data->MarkState(init_data["preprocessorName"].GetString());
    }
  }
  if (!template_bundle_) {
    std::string source = preloaded_source_;
    if (preloaded_source_.empty() && dom.HasMember("source") &&
        dom["source"].IsString()) {
      source = TestBenchDecode(dom["source"].GetString());
    }
    template_bundle_ = std::make_shared<lynx::pub::LynxTemplateBundle>(
        (uint8_t*)source.c_str(), source.size());
  }
  auto load_meta = std::make_shared<lynx::pub::LynxLoadMeta>();
  load_meta->SetTemplateBundle(template_bundle_);
  if (template_data) {
    load_meta->SetInitialData(template_data);
  }
  if (global_props_) {
    load_meta->SetGlobalProps(global_props_);
  }
  lynx_view_->LoadTemplate(load_meta);
}

void TestBenchActionManager::UpdatePreData(const std::string& param) {
  rapidjson::Document dom;
  dom.Parse(param);
  if (dom.HasParseError()) {
    return;
  }
  std::string value;
  std::string process_name;
  if (dom.HasMember("value")) {
    value = ToJson(dom["value"]);
  }
  if (dom.HasMember("preprocessorName") && dom["preprocessorName"].IsString()) {
    process_name = dom["preprocessorName"].GetString();
  }
  auto template_data = std::make_shared<lynx::pub::LynxTemplateData>(value);
  if (!process_name.empty()) {
    template_data->MarkState(process_name);
  }

  auto update_meta = std::make_shared<lynx::pub::LynxUpdateMeta>();
  update_meta->SetUpdateData(template_data);
  lynx_view_->UpdateData(update_meta);
}

void TestBenchActionManager::SendGlobalEvent(const std::string& param) {
  rapidjson::Document dom;
  dom.Parse(param);
  if (dom.HasParseError()) {
    return;
  }
  const rapidjson::Value* arguments = nullptr;
  if (dom.IsObject() && dom.HasMember("arguments") &&
      dom["arguments"].IsArray()) {
    arguments = &dom["arguments"];
  } else if (dom.IsArray()) {
    arguments = &dom;
  }

  if (!arguments || arguments->Size() != 2 || !(*arguments)[0].IsString()) {
    return;
  }

  const std::string event_name = (*arguments)[0].GetString();
  if (event_name.compare("exposure") == 0 ||
      event_name.compare("disexposure") == 0) {
    return;
  }
  lynx_view_->SendGlobalEvent(event_name, ToJson((*arguments)[1]));
}

void TestBenchActionManager::ReloadTemplate(const std::string& param) {
  rapidjson::Document dom;
  dom.Parse(param);
  if (dom.HasParseError()) {
    return;
  }
  std::shared_ptr<lynx::pub::LynxTemplateData> data;
  if (dom.HasMember("value")) {
    std::string value = ToJson(dom["value"]);
    data = std::make_shared<lynx::pub::LynxTemplateData>(value);
    if (dom.HasMember("preprocessorName") &&
        dom["preprocessorName"].IsString()) {
      data->MarkState(dom["preprocessorName"].GetString());
      data->MarkReadOnly();
    }
  }

  if (data) {
    lynx_view_->ReloadTemplate(data, nullptr);
  }
}

void TestBenchActionManager::UpdateConfig(const std::string& param) {}

void TestBenchActionManager::FromTemplate(const std::string& param) {
  rapidjson::Document dom;
  dom.Parse(param);
  if (dom.HasParseError()) {
    return;
  }
  std::string source = preloaded_source_;
  if (preloaded_source_.empty() && dom.HasMember("source") &&
      dom["source"].IsString()) {
    source = TestBenchDecode(dom["source"].GetString());
  }
  template_bundle_ = std::make_unique<lynx::pub::LynxTemplateBundle>(
      (uint8_t*)source.c_str(), source.size());
}

void TestBenchActionManager::SendTouchEvent(const std::string& param) {
  // NOOP
}

void TestBenchActionManager::SendBubbleEvent(const std::string& param) {
  lynx_view_->InjectBubbleEvent(param);
}

void TestBenchActionManager::SendCustomEvent(const std::string& param) {
  // NOOP
}

}  // namespace embedder
}  // namespace lynx
