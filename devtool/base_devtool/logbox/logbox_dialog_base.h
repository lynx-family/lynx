// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef DEVTOOL_BASE_DEVTOOL_LOGBOX_LOGBOX_DIALOG_BASE_H_
#define DEVTOOL_BASE_DEVTOOL_LOGBOX_LOGBOX_DIALOG_BASE_H_

#include <functional>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>

#include "devtool/base_devtool/logbox/logbox_base.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace devtool {
class LogBoxManager;
class LogBoxDialogBase : public std::enable_shared_from_this<LogBoxDialogBase> {
 public:
  virtual ~LogBoxDialogBase() = default;

  virtual void Init(std::function<void()>&& onLoaded, void* native_context) = 0;
  virtual void Dismiss() = 0;
  virtual void Show() = 0;

  void ShowErrorMessage(const LogBoxLogMsg& msg);
  void ShowErrorMessages(const std::list<LogBoxLogMsg>& msgs);
  bool IsLoadingFinished() { return is_loading_finished_; }
  bool IsShowing() { return is_showing_; }
  int32_t GetLevel() { return current_level_; }
  void SetJsSources(
      const std::unordered_map<std::string, std::string>& sources);
  void UpdateViewInfo(int32_t index, int32_t viewCount, int32_t level,
                      const std::string& template_url);

  static std::string GenerateErrorJsonString(const LogBoxLogMsg& error);
  static std::shared_ptr<LogBoxDialogBase> Create(
      std::shared_ptr<LogBoxManager> manager);

 protected:
  using BridgeFunction = void (LogBoxDialogBase::*)(const rapidjson::Value&,
                                                    int32_t);

  bool is_parser_loaded_ = false;
  bool is_loading_finished_ = false;
  bool is_showing_;
  int32_t current_level_ = -1;
  std::function<void()> on_loaded_;
  std::unordered_map<std::string, BridgeFunction> js_api_map_;
  std::string template_url_;
  std::unordered_map<std::string, std::string> js_source_map_;
  std::weak_ptr<LogBoxManager> manager_;

  LogBoxDialogBase(std::shared_ptr<LogBoxManager> manager);

  void InitJSFunction();
  void OnReceiveMessage(const std::string& message);

  virtual void ExecuteScript(const std::string& script) = 0;
  virtual void GetFileContent(const std::string& url, std::string& content) = 0;

  // jsb method
  void GetExceptionStack(const rapidjson::Value& params, int32_t callback_id);
  void LoadErrorParser(const rapidjson::Value& params, int32_t callback_id);
  void Reload(const rapidjson::Value& params, int32_t callback_id);
  void Dismiss(const rapidjson::Value& params, int32_t callback_id);
  void GetResource(const rapidjson::Value& params, int32_t callback_id);
  void ChangeView(const rapidjson::Value& params, int32_t callback_id);
  void RemoveCurrentLogs(const rapidjson::Value& params, int32_t callback_id);

  void RequestLogOfViewIndex(int32_t index);
  void Download(const std::string& name, int32_t callback_id);
  const std::string* GetJsSource(std::string source_name);
  void OnDownloadCompleted(const std::string& data, int32_t callback_id);
  void LoadMappingsWasm();

  static std::string WrapResultToJson(bool res, int32_t callback_id);
  static std::string WrapResultToJson(const std::string& res,
                                      int32_t callback_id);
  static std::string WrapLogToJson(const LogBoxLogMsg& log);
  void SendJsEvent(const std::string& event);
  void SendJsResult(const std::string& result);
};

}  // namespace devtool
}  // namespace lynx
#endif  // DEVTOOL_BASE_DEVTOOL_LOGBOX_LOGBOX_DIALOG_BASE_H_
