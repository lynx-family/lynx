// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef DEVTOOL_BASE_DEVTOOL_LOGBOX_LOGBOX_DIALOG_WINDOWS_H_
#define DEVTOOL_BASE_DEVTOOL_LOGBOX_LOGBOX_DIALOG_WINDOWS_H_

#include <Windows.h>
#include <wrl.h>

#include <memory>
#include <string>

#include "WebView2.h"
#include "devtool/base_devtool/logbox/logbox_dialog_base.h"

namespace lynx {
namespace devtool {
class LogBoxDialogWindows : public LogBoxDialogBase {
 public:
  LogBoxDialogWindows(std::shared_ptr<LogBoxManager> manager)
      : LogBoxDialogBase(manager) {}
  virtual ~LogBoxDialogWindows();

  void Init(std::function<void()>&& onLoaded, void* native_window) override;
  void Show() override;
  void Dismiss() override;
  void GetFileContent(const std::string& url, std::string& content) override;

 protected:
  void ExecuteScript(const std::string& script) override;

 private:
  HWND window_;
  Microsoft::WRL::ComPtr<ICoreWebView2Controller> webview_controller_;

  HWND CreateLogboxWindow(HWND native_window);
  HRESULT OnCreateWebViewEnvCompleted(HRESULT result,
                                      ICoreWebView2Environment* env);
  HRESULT OnCreateWebViewControllerCompleted(
      HRESULT result, ICoreWebView2Controller* controller);
  HRESULT OnWebViewMessageReceived(
      ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* args);
  std::string GetLogboxFileUrl(const std::string& file);
  static WNDCLASS RegisterWindowClass();
  static LRESULT CALLBACK WndProc(HWND const window, UINT const message,
                                  WPARAM const wparam,
                                  LPARAM const lparam) noexcept;
};
}  // namespace devtool
}  // namespace lynx
#endif  // DEVTOOL_BASE_DEVTOOL_LOGBOX_LOGBOX_DIALOG_WINDOWS_H_
