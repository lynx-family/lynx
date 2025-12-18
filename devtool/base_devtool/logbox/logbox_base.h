// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef DEVTOOL_BASE_DEVTOOL_LOGBOX_LOGBOX_BASE_H_
#define DEVTOOL_BASE_DEVTOOL_LOGBOX_LOGBOX_BASE_H_

#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>

#include "base/include/debug/lynx_error.h"

namespace lynx {
namespace devtool {

enum class LogBoxLogLevel : int32_t { Error = 0, Warn = 1, Info = 2, Butt };
struct LogBoxLogMsg {
  LogBoxLogMsg(const lynx::base::LynxError& error)
      : error_level_(static_cast<int32_t>(error.error_level_) - 1),
        error_code_(error.error_code_),
        error_message_(error.error_message_),
        fix_suggestion_(error.fix_suggestion_) {}
  int32_t error_level_;
  int32_t error_code_;
  std::string error_message_;
  std::string fix_suggestion_;
};
int32_t LynxErrorLevel2LogBoxLevel(lynx::base::LynxErrorLevel lynxErrorlevel);
lynx::base::LynxErrorLevel LogBoxLevel2LynxErrorLevel(int32_t logboxLevel);

class LogBoxBase {
 public:
  virtual void ShowErrorMessage(
      int level, int32_t error_code, const std::string& message,
      const std::string& fix_suggestion,
      const std::unordered_map<std::string, std::string>& custom_info,
      bool is_logbox_only) = 0;
  virtual void OnLoadTemplate() = 0;
};

class LogBoxResourceProvider {
 public:
  LogBoxResourceProvider() = default;
  virtual ~LogBoxResourceProvider() = default;

  virtual void* GetNativeWindow() = 0;
  virtual std::string GetTemplateUrl() = 0;
  virtual const std::unordered_map<std::string, std::string>&
  GetAllJsSource() = 0;
};

class LogBoxProxy : public std::enable_shared_from_this<LogBoxProxy> {
 public:
  LogBoxProxy() = delete;
  LogBoxProxy(std::shared_ptr<LogBoxResourceProvider> provider)
      : provider_(provider) {}
  void ShowErrorMessage(lynx::base::LynxError&& log);
  std::shared_ptr<LogBoxResourceProvider> GetProvider() {
    return provider_.lock();
  }
  std::list<LogBoxLogMsg> GetLogs(int32_t level = -1);
  int32_t ClearLogs(int32_t level);

 private:
  std::weak_ptr<LogBoxResourceProvider> provider_;
  std::list<lynx::base::LynxError> logs_;
};

class LogBoxManager;

class LogBoxOwner {
 public:
  static std::shared_ptr<LogBoxOwner> GetInstance();
  bool Dispatch(LogBoxLogMsg& msg, void* native_context,
                const std::shared_ptr<LogBoxProxy>& proxy);

 protected:
  LogBoxOwner() = default;
  virtual std::shared_ptr<LogBoxManager> GetOrCreateLogBoxManager(
      void* native_context) {
    return nullptr;
  }
};
}  // namespace devtool
}  // namespace lynx
#endif  // DEVTOOL_BASE_DEVTOOL_LOGBOX_LOGBOX_BASE_H_
