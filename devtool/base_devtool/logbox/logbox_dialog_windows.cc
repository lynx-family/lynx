// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "devtool/base_devtool/logbox/logbox_dialog_windows.h"

#include <atlstr.h>
#include <tchar.h>
#include <winuser.h>

#include <fstream>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnonportable-include-path"
#include "WebView2EnvironmentOptions.h"
#pragma clang diagnostic pop
#include "base/include/log/logging.h"
#include "base/include/string/string_conversion_win.h"
#include "core/base/utils/paths_win.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"

namespace base_utils = lynx::base;

namespace {
std::wstring GetWebViewTempPath() {
  wchar_t tempPath[MAX_PATH];
  DWORD pathLen = GetTempPath(MAX_PATH, tempPath);
  if (pathLen == 0 || pathLen > MAX_PATH) {
    return L"";
  }
  std::wstring userDataPath = tempPath;
  if (userDataPath.back() != L'\\') {
    userDataPath += L'\\';
  }
  userDataPath += L"LynxLogBoxWebView";
  return userDataPath;
}
}  // namespace

namespace lynx {
namespace devtool {
constexpr static const TCHAR kLogboxWindowClass[] = _T("DevtoolLogbox");
constexpr static const TCHAR kLogboxWindowName[] = _T("DevtoolLogbox");
constexpr static const double kWebviewDefaultHeight = 0.7;
constexpr static std::string_view kBridgeJs =
    "(function () {"
    "var id = 0, callbacks = {}, eventListeners = {};"
    "var nativeBridge = window.chrome.webview;"
    "window.logbox = {"
    "  call: function(bridgeName, callback, data) {"
    "    var thisId = id++;"
    "    callbacks[thisId] = callback;"
    "    nativeBridge.postMessage({"
    "      bridgeName: bridgeName,"
    "      data: data || {},"
    "      callbackId: thisId"
    "    });"
    "  },"
    "  on: function(event, handler) {"
    "    eventListeners[event] = handler;"
    "  },"
    "  sendResult: function(msg) {"
    "    var callbackId = msg.callbackId;"
    "    if (callbacks[callbackId]) {"
    "      callbacks[callbackId](msg.data);"
    "    }"
    "  },"
    "  sendEvent: function(msg) {"
    "    if (eventListeners[msg.event]) {"
    "      eventListeners[msg.event](msg.data);"
    "    }"
    "  }"
    "};"
    "})();"
    "document.dispatchEvent(new Event('LogBoxReady'));";

LogBoxDialogWindows::~LogBoxDialogWindows() {
  webview_controller_->Close();
  webview_controller_ = nullptr;
  DestroyWindow(window_);
}

void LogBoxDialogWindows::Init(std::function<void()>&& onLoaded,
                               void* native_window) {
  HWND root_wnd = (HWND)native_window;
  if (!CreateLogboxWindow(root_wnd)) {
    return;
  }
  on_loaded_.swap(onLoaded);
  auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
#if defined(DEBUG_FRONTEND)
  options->put_AdditionalBrowserArguments(L"--remote-debugging-port=8080");
#endif
  auto tempDataPath = GetWebViewTempPath();
  CreateCoreWebView2EnvironmentWithOptions(
      nullptr, tempDataPath.empty() ? nullptr : tempDataPath.c_str(),
      options.Get(),
      Microsoft::WRL::Callback<
          ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
          this, &LogBoxDialogWindows::OnCreateWebViewEnvCompleted)
          .Get());
}

void LogBoxDialogWindows::Show() {
  if (!is_showing_ && window_ && webview_controller_) {
    is_showing_ = true;
    webview_controller_->put_IsVisible(true);
    ShowWindow(window_, SW_SHOW);
    SetWindowPos(window_, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    SetFocus(window_);
  }
}

void LogBoxDialogWindows::Dismiss() {
  if (is_showing_ && window_) {
    is_showing_ = false;
    ShowWindow(window_, SW_HIDE);
  }
  SendJsEvent("{\"event\":\"reset\"}");
}

void LogBoxDialogWindows::GetFileContent(const std::string& url,
                                         std::string& content) {
  std::string filePath = GetLogboxFileUrl(url);
  std::wstring w_filePath = base_utils::Utf16FromUtf8(filePath);

  std::ifstream file_stream(w_filePath, std::ios::binary);
  if (!file_stream.is_open()) {
    LOGE("Failed to open file: " << filePath);
    return;
  }

  file_stream.seekg(0, std::ios::end);
  std::streamsize file_size = file_stream.tellg();
  file_stream.seekg(0, std::ios::beg);

  if (file_size <= 0) {
    LOGE("File is empty or invalid size: " << file_size);
    return;
  }

  content.resize(static_cast<size_t>(file_size));
  if (!file_stream.read(&content[0], file_size)) {
    LOGE("Failed to read file content, expected: " << file_size << " bytes");
    content.clear();
    return;
  }
}

void LogBoxDialogWindows::ExecuteScript(const std::string& script) {
  if (!webview_controller_) return;

  Microsoft::WRL::ComPtr<ICoreWebView2> webview;
  webview_controller_->get_CoreWebView2(&webview);
  if (!webview) {
    LOGE("Webview is null");
    return;
  }
  std::wstring w_script = base_utils::Utf16FromUtf8(script);
  webview->ExecuteScript(w_script.c_str(), nullptr);
}

HWND LogBoxDialogWindows::CreateLogboxWindow(HWND parent) {
  CHECK_NULL_RETURN_VALUE(parent, nullptr);

  WNDCLASS window_class = RegisterWindowClass();
  window_ = CreateWindowEx(0, kLogboxWindowClass, kLogboxWindowName,
                           WS_CHILD | WS_MAXIMIZE | WS_CLIPCHILDREN, 0, 0, 0, 0,
                           parent, nullptr, window_class.hInstance, nullptr);
  CHECK_NULL_AND_LOG_RETURN_VALUE(window_, "Create window failed", nullptr);

  // set this pointer for wnd proc
  SetWindowLongPtr(window_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
  ShowWindow(window_, SW_SHOW);
  return window_;
}

HRESULT LogBoxDialogWindows::OnCreateWebViewEnvCompleted(
    HRESULT result, ICoreWebView2Environment* env) {
  if (!env) {
    // if user doesnt install webview2 runtime,
    // creation of webview env will fail
    LOGE("Create webview env failed");
    return E_FAIL;
  }

  env->CreateCoreWebView2Controller(
      window_,
      Microsoft::WRL::Callback<
          ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
          this, &LogBoxDialogWindows::OnCreateWebViewControllerCompleted)
          .Get());
  return S_OK;
}

HRESULT LogBoxDialogWindows::OnCreateWebViewControllerCompleted(
    HRESULT result, ICoreWebView2Controller* controller) {
  if (controller == nullptr) {
    LOGE("Create webview controller failed");
    return E_FAIL;
  }

  webview_controller_ = controller;
  Microsoft::WRL::ComPtr<ICoreWebView2> webview;
  controller->get_CoreWebView2(&webview);

  // resize webview to fit the bounds of the parent window
  RECT bounds;
  GetClientRect(window_, &bounds);
  bounds.top =
      bounds.top + (bounds.bottom - bounds.top) * (1 - kWebviewDefaultHeight);
  webview_controller_->put_Bounds(bounds);

  webview->AddScriptToExecuteOnDocumentCreated(
      base_utils::Utf16FromUtf8(kBridgeJs).c_str(), nullptr);

  EventRegistrationToken token;
  webview->add_WebMessageReceived(
      Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>(
          this, &LogBoxDialogWindows::OnWebViewMessageReceived)
          .Get(),
      &token);

  std::string url = GetLogboxFileUrl("index.html");
  if (url.empty()) {
    LOGE("GetLogboxFileUrl failed");
    return E_FAIL;
  }
  url.append("?url=" + template_url_);
  url.append("&downloadapi=true");

  std::wstring w_url = base_utils::Utf16FromUtf8("file:///" + url);
  webview->Navigate(w_url.c_str());
  // trigger redbox show when webview initialize completed
  Show();
  return S_OK;
}

HRESULT LogBoxDialogWindows::OnWebViewMessageReceived(
    ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* args) {
  PWSTR message;
  args->get_WebMessageAsJson(&message);
  std::string message_utf8 =
      base_utils::Utf8FromUtf16(std::wstring_view(message));
  CoTaskMemFree(message);
  if (message_utf8.empty()) return E_FAIL;

  OnReceiveMessage(message_utf8);
  return S_OK;
}

std::string LogBoxDialogWindows::GetLogboxFileUrl(const std::string& file) {
  auto [result, exe_dir] = base_utils::GetExecutableDirectoryPath();
  if (!result) {
    LOGE("GetExecutableDirectoryPath failed");
    return "";
  }
  if (exe_dir.back() != '\\') {
    exe_dir.append("\\");
  }
  return exe_dir + "js_assets\\logbox\\" + file;
}

WNDCLASS LogBoxDialogWindows::RegisterWindowClass() {
  WNDCLASS window_class{};
  window_class.lpszClassName = kLogboxWindowClass;
  window_class.hInstance = GetModuleHandle(nullptr);
  window_class.lpfnWndProc = WndProc;
  RegisterClass(&window_class);
  return window_class;
}

// a window's wndproc is called in the context of
// the thread that created the window
LRESULT LogBoxDialogWindows::WndProc(HWND const window, UINT const message,
                                     WPARAM const wparam,
                                     LPARAM const lparam) noexcept {
  switch (message) {
    case WM_PAINT:
    case WM_SIZE: {
      LOGI("On size changed");
      auto* that = reinterpret_cast<LogBoxDialogWindows*>(
          GetWindowLongPtr(window, GWLP_USERDATA));
      if (!that || !that->webview_controller_) break;
      HWND parent = GetParent(window);
      if (!parent) break;

      RECT old_bounds;
      RECT new_bounds;
      GetClientRect(window, &old_bounds);
      GetClientRect(parent, &new_bounds);
      if (EqualRect(&old_bounds, &new_bounds)) break;

      MoveWindow(window, new_bounds.left, new_bounds.top,
                 new_bounds.right - new_bounds.left,
                 new_bounds.bottom - new_bounds.top, true);
      new_bounds.top = new_bounds.top + (new_bounds.bottom - new_bounds.top) *
                                            (1 - kWebviewDefaultHeight);
      that->webview_controller_->put_Bounds(new_bounds);
      return 0;
    }
    case WM_LBUTTONDOWN: {
      LOGI("On left button down");
      auto* that = reinterpret_cast<LogBoxDialogWindows*>(
          GetWindowLongPtr(window, GWLP_USERDATA));
      if (that) {
        that->Dismiss();
        return 0;
      }
      break;
    }
    default:
      break;
  }
  return DefWindowProc(window, message, wparam, lparam);
}

std::shared_ptr<LogBoxDialogBase> LogBoxDialogBase::Create(
    std::shared_ptr<LogBoxManager> manager) {
  return std::make_shared<LogBoxDialogWindows>(manager);
}
}  // namespace devtool
}  // namespace lynx
