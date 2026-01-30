// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "devtool/base_devtool/logbox/logbox_dialog_base.h"

#include "base/include/log/logging.h"
#include "core/base/json/json_util.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/platform/clay/shell/net/net_helper.h"
#include "devtool/base_devtool/logbox/logbox_manager.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"
#include "third_party/modp_b64/modp_b64.h"

using LynxError = lynx::base::LynxError;
using LynxErrorLevel = lynx::base::LynxErrorLevel;

namespace {
std::string GetFileNameFromPath(const std::string& path) {
  size_t last_slash_pos = path.find_last_of("/\\");
  if (last_slash_pos != std::string::npos) {
    return path.substr(last_slash_pos + 1);
  }
  return path;
}
}  // namespace

namespace lynx {
namespace devtool {

LogBoxDialogBase::LogBoxDialogBase(std::shared_ptr<LogBoxManager> manager)
    : manager_(manager) {
  InitJSFunction();
}

__attribute__((weak)) std::shared_ptr<LogBoxDialogBase>
LogBoxDialogBase::Create(std::shared_ptr<LogBoxManager> manager) {
  return nullptr;
}

void LogBoxDialogBase::ShowErrorMessage(const LogBoxLogMsg& msg) {
  current_level_ = msg.error_level_;
  const std::string& log_json = WrapLogToJson(msg);
  SendJsEvent(log_json);
}

void LogBoxDialogBase::ShowErrorMessages(const std::list<LogBoxLogMsg>& msgs) {
  if (msgs.empty()) {
    return;
  }
  for (const auto& msg : msgs) {
    ShowErrorMessage(msg);
  }
}

void LogBoxDialogBase::UpdateViewInfo(int32_t index, int32_t viewCount,
                                      int32_t level,
                                      const std::string& template_url) {
  // TODO: This function is unused, index & level are incorrect
  current_level_ = level;
  rapidjson::Document dom;
  auto& allocator = dom.GetAllocator();
  rapidjson::Value event(rapidjson::kObjectType);
  rapidjson::Value data(rapidjson::kObjectType);
  event.AddMember("event", "receiveViewInfo", allocator);
  data.AddMember("index", index, allocator);
  data.AddMember("viewsCount", viewCount, allocator);
  data.AddMember("level",
                 static_cast<int32_t>(LogBoxLevel2LynxErrorLevel(level)),
                 allocator);
  data.AddMember("templateUrl", template_url, allocator);
  event.AddMember("data", data, allocator);
  const std::string& log_json = lynx::base::ToJson(event);
  SendJsEvent(log_json);
}

void LogBoxDialogBase::InitJSFunction() {
  js_api_map_["getExceptionStack"] = &LogBoxDialogBase::GetExceptionStack;
  js_api_map_["loadErrorParser"] = &LogBoxDialogBase::LoadErrorParser;
  js_api_map_["reload"] = &LogBoxDialogBase::Reload;
  js_api_map_["dismiss"] = &LogBoxDialogBase::Dismiss;
  js_api_map_["queryResource"] = &LogBoxDialogBase::GetResource;
  js_api_map_["changeView"] = &LogBoxDialogBase::ChangeView;
  js_api_map_["deleteLynxview"] = &LogBoxDialogBase::RemoveCurrentLogs;
}

void LogBoxDialogBase::ChangeView(const rapidjson::Value& params,
                                  int32_t callback_id) {
  LOGI("On ChangeView");
  // TODO: multi view support
}

void LogBoxDialogBase::RemoveCurrentLogs(const rapidjson::Value& params,
                                         int32_t callback_id) {
  LOGI("On RemoveCurrentLogs");
  auto sp_manager = manager_.lock();
  if (sp_manager) sp_manager->OnRemoveBtnClick(current_level_);
}

void LogBoxDialogBase::OnReceiveMessage(const std::string& message) {
  rapidjson::Document dom;
  dom.Parse(message);
  if (dom.HasParseError()) {
    LOGE("Webview message parse failed");
    return;
  }

  if (!dom.HasMember("bridgeName") || !dom["bridgeName"].IsString() ||
      !dom.HasMember("callbackId") || !dom["callbackId"].IsInt() ||
      !dom.HasMember("data") || !dom["data"].IsObject()) {
    LOGE("Illegal webview message: " << message);
    return;
  }

  std::string bridge_name = "";
  int32_t callback_id = -1;
  rapidjson::Value data(rapidjson::kObjectType);

  rapidjson::Value& v_bridge_name = dom["bridgeName"];
  bridge_name.append(v_bridge_name.GetString());

  rapidjson::Value& v_callback_id = dom["callbackId"];
  callback_id = v_callback_id.GetInt();

  rapidjson::Value& v_data = dom["data"];
  data.CopyFrom(v_data, dom.GetAllocator());

  auto it = js_api_map_.find(bridge_name);
  if (it == js_api_map_.end()) {
    LOGE("Unkown bridge name: " << bridge_name);
    return;
  }
  (this->*(it->second))(data, callback_id);
}

// This jsb will be called once when webview is loaded completely
void LogBoxDialogBase::GetExceptionStack(const rapidjson::Value& params,
                                         int32_t callback_id) {
  LOGI("On GetExceptionStack");
  is_loading_finished_ = true;
  LoadMappingsWasm();

  if (on_loaded_) {
    on_loaded_();
  }
}

void LogBoxDialogBase::LoadErrorParser(const rapidjson::Value& params,
                                       int32_t callback_id) {
  LOGI("On LoadErrorParser");
  if (is_parser_loaded_) {
    const std::string& result = WrapResultToJson(true, callback_id);
    SendJsResult(result);
    return;
  }
  bool res = false;
  if (!is_parser_loaded_ && params.HasMember("namespace") &&
      params["namespace"].IsString()) {
    std::string parser_name = std::string() + params["namespace"].GetString() +
                              "-error-parser.js";  // lynx-error-parser.js
    std::string content;
    GetFileContent(parser_name, content);
    if (!content.empty()) {
      ExecuteScript(content);
      is_parser_loaded_ = true;
      res = true;
    }
  }
  const std::string& result = WrapResultToJson(res, callback_id);
  SendJsResult(result);
}

void LogBoxDialogBase::Reload(const rapidjson::Value& params,
                              int32_t callback_id) {
  LOGW("on Reload.");
  if (on_loaded_) {
    on_loaded_();
  }
}

void LogBoxDialogBase::Dismiss(const rapidjson::Value& params,
                               int32_t callback_id) {
  Dismiss();
}

void LogBoxDialogBase::GetResource(const rapidjson::Value& params,
                                   int32_t callback_id) {
  LOGI("On GetResource");
  if (!params.HasMember("name") || !params["name"].IsString()) {
    SendJsResult(WrapResultToJson(std::string(), callback_id));
    return;
  }
  std::string url = params["name"].GetString();
  LOGI("Download source map with url: " << url);
  if (url.substr(0, 4) == "http") {
    Download(url, callback_id);
    return;
  }
  const auto* source = GetJsSource(url);
  if (source) {
    SendJsResult(WrapResultToJson(*source, callback_id));
  } else {
    // no source founded
    SendJsResult(WrapResultToJson(std::string(), callback_id));
  }
}

void LogBoxDialogBase::Download(const std::string& url, int32_t callback_id) {
#if !defined(DISABLE_LOGBOX_BASE)
  lynx::NetHelper* net_helper = lynx::NetHelper::GetInstance();
  net_helper->AsyncRequest(
      url, [wk = weak_from_this(), callback_id](const std::string& data) {
        if (auto sp = wk.lock()) {
          sp->OnDownloadCompleted(data, callback_id);
        }
      });
#endif
}

const std::string* LogBoxDialogBase::GetJsSource(std::string source_name) {
  std::string pure_name = GetFileNameFromPath(source_name);
  LOGI("GetJsSource with source name: " << pure_name);
  const auto& sources_map = js_source_map_;
  auto it = sources_map.find(pure_name);
  if (it == sources_map.end()) {
    LOGW("No JS source found");
    return nullptr;
  }
  return &it->second;
}

void LogBoxDialogBase::OnDownloadCompleted(const std::string& data,
                                           int32_t callback_id) {
  LOGI("On source map download completed");
#if !defined(DISABLE_LOGBOX_BASE)
  std::string result = WrapResultToJson(data, callback_id);
  lynx::base::UIThread::GetRunner()->PostTask([wk = weak_from_this(), result] {
    if (auto sp = wk.lock()) {
      sp->SendJsResult(result);
    }
  });
#endif
}

void LogBoxDialogBase::SetJsSources(
    const std::unordered_map<std::string, std::string>& sources) {
  js_source_map_.clear();
  for (const auto& pair : sources) {
    std::string pure_name = GetFileNameFromPath(pair.first);
    js_source_map_[pure_name] = pair.second;
  }
}

void LogBoxDialogBase::LoadMappingsWasm() {
  std::string mappings_wasm;
  GetFileContent("mappings.wasm", mappings_wasm);
  std::string& base64_data = modp_b64_encode(mappings_wasm);

  rapidjson::Document dom;
  auto& allocator = dom.GetAllocator();

  rapidjson::Value data_object(rapidjson::kObjectType);
  data_object.AddMember("type", "mappings.wasm", allocator);
  data_object.AddMember(
      "data", rapidjson::Value(base64_data.c_str(), base64_data.length()),
      allocator);

  rapidjson::Value root_value(rapidjson::kObjectType);
  root_value.AddMember("event", "loadFile", allocator);
  root_value.AddMember("data", data_object, allocator);

  std::string event_json = lynx::base::ToJson(root_value);
  SendJsEvent(event_json);
}

std::string LogBoxDialogBase::WrapResultToJson(bool res, int32_t callback_id) {
  rapidjson::Document dom;
  auto& allocator = dom.GetAllocator();
  rapidjson::Value v_data;
  v_data.SetBool(res);
  rapidjson::Value v_callback_id(rapidjson::kNumberType);
  v_callback_id.SetInt(callback_id);

  rapidjson::Value root_value(rapidjson::kObjectType);
  root_value.AddMember("callbackId", v_callback_id, allocator);
  root_value.AddMember("data", v_data, allocator);

  std::string result_json = lynx::base::ToJson(root_value);
  return result_json;
}

std::string LogBoxDialogBase::WrapResultToJson(const std::string& res,
                                               int32_t callback_id) {
  rapidjson::Document dom;
  auto& allocator = dom.GetAllocator();
  rapidjson::Value root_value(rapidjson::kObjectType);
  root_value.AddMember("callbackId", callback_id, allocator);
  root_value.AddMember("data", rapidjson::Value(res.c_str(), res.length()),
                       allocator);

  std::string result_json = lynx::base::ToJson(root_value);
  return result_json;
}

std::string LogBoxDialogBase::WrapLogToJson(const LogBoxLogMsg& log) {
  rapidjson::Document dom;
  auto& allocator = dom.GetAllocator();
  rapidjson::Value root_value(rapidjson::kObjectType);
  rapidjson::Value v_event(rapidjson::kStringType);
  v_event.SetString("receiveNewLog");
  rapidjson::Value v_data(rapidjson::kObjectType);
  rapidjson::Value namespace_data(rapidjson::kStringType);
  namespace_data.SetString("lynx", 4);
  v_data.AddMember("namespace", namespace_data, allocator);

  std::string logDataStr = GenerateErrorJsonString(log);
  v_data.AddMember("log",
                   rapidjson::Value(logDataStr.c_str(), logDataStr.length()),
                   allocator);
  root_value.AddMember("event", v_event, allocator);
  root_value.AddMember("data", v_data, allocator);
  std::string log_json = lynx::base::ToJson(root_value);
  return log_json;
}

std::string LogBoxDialogBase::GenerateErrorJsonString(
    const LogBoxLogMsg& error) {
  int32_t level =
      static_cast<int32_t>(LogBoxLevel2LynxErrorLevel(error.error_level_));
  std::string suggestion = error.fix_suggestion_;
  bool is_new_error_code_ = false;
#if !defined(DISABLE_LOGBOX_BASE)
  auto meta = lynx::error::GetMetaData(error.error_code_);
  if (meta) {
    if (meta->level_ != lynx::error::Level::UNDECIDED)
      level = static_cast<int32_t>(meta->level_);
    suggestion = (meta->fix_suggestion_.empty()) ? error.fix_suggestion_
                                                 : meta->fix_suggestion_;
    is_new_error_code_ = true;
  }
#endif

  rapidjson::Document dom;
  auto& allocator = dom.GetAllocator();
  rapidjson::Value log_data(rapidjson::kObjectType);
  std::string levelStr = LynxError::GetLevelString(level);
  log_data.AddMember(
      "error_code",
      is_new_error_code_ ? error.error_code_ / 100 : error.error_code_,
      allocator);
  log_data.AddMember("sub_code", error.error_code_, allocator);
  log_data.AddMember("error",
                     rapidjson::Value(error.error_message_.c_str(),
                                      error.error_message_.length()),
                     allocator);
  log_data.AddMember("level",
                     rapidjson::Value(levelStr.c_str(), levelStr.length()),
                     allocator);
  log_data.AddMember("fix_suggestion",
                     rapidjson::Value(suggestion.c_str(), suggestion.length()),
                     allocator);
  return lynx::base::ToJson(log_data);
}

void LogBoxDialogBase::SendJsEvent(const std::string& event) {
  std::string js = "window.logbox.sendEvent(" + event + ");";
  ExecuteScript(js);
}

void LogBoxDialogBase::SendJsResult(const std::string& result) {
  std::string js = "window.logbox.sendResult(" + result + ");";
  ExecuteScript(js);
}

}  // namespace devtool
}  // namespace lynx
