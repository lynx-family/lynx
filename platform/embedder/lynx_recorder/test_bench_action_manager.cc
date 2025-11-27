// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_recorder/test_bench_action_manager.h"

#include <cmath>
#include <future>
#include <memory>

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

void TestBenchActionManager::StartWithUrl(const std::string& url) {
  if (url.empty()) {
    return;
  }
  if (!replay_config_) {
    replay_config_ = std::make_unique<embedder::TestBenchReplayConfig>();
  }
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
    } else if (config.HasMember("jsbSettings")) {
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
      int64_t record_time = 0;
      if (obj.HasMember("Record Time") && obj["Record Time"].IsString()) {
        std::string record_time_str = obj["Record Time"].GetString();
        int record_time_value = 0;
        StringToInt(record_time_str, &record_time_value, 10);
        record_time = record_time_value * 1000;
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
      ReplayAction replay_action;
      replay_action.function_id =
          replay_config_->GetCanMockFuncId(function_name_str);
      replay_action.params = param;
      DoAction(replay_action);
    }
  }
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
  int preferred_layout_height = 0;
  int preferred_layout_width = 0;
  if (dom.HasMember("preferredLayoutHeight") &&
      dom["preferredLayoutHeight"].IsFloat()) {
    preferred_layout_height =
        std::round(dom["preferredLayoutHeight"].GetFloat());
  }
  if (dom.HasMember("preferredLayoutWidth") &&
      dom["preferredLayoutWidth"].IsFloat()) {
    preferred_layout_width = std::round(dom["preferredLayoutWidth"].GetFloat());
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
  std::string event_name;
  if (dom.IsArray() && dom.Size() == 2) {
    if (dom[0].IsString()) {
      event_name = dom[0].GetString();
    }
    if (event_name.compare("exposure") == 0 ||
        event_name.compare("disexposure") == 0) {
      return;
    }
    std::string params = ToJson(dom[1]);
    lynx_view_->SendGlobalEvent(event_name, params);
  }
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
