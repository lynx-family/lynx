// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "explorer/windows/lynx_explorer/lynx_window.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

#include "explorer/windows/lynx_explorer/dpi_utils_win32.h"
#include "explorer/windows/lynx_explorer/fetcher/example_generic_resource_fetcher.h"
#include "explorer/windows/lynx_explorer/httplib/httplib_client.h"
#include "explorer/windows/lynx_explorer/lynx_window_manager.h"
#include "explorer/windows/lynx_explorer/module/lynx_node_api_module.h"
#include "explorer/windows/lynx_explorer/runtime/example_lynx_runtime_lifecycle_observer.h"
#include "platform/embedder/public/lynx_native_view.h"

namespace lynx {

namespace {

constexpr const wchar_t kWindowClassName[] = L"Lynx Explorer";
constexpr unsigned int kDefaultWindowWidth = 800;
constexpr unsigned int kDefaultWindowHeight = 600;
constexpr unsigned int kMinWindowSize = 200;

const wchar_t *RegisterWindowClass() {
  static bool class_registered_ = false;
  if (!class_registered_) {
    WNDCLASS window_class{};
    window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
    window_class.lpszClassName = kWindowClassName;
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;
    window_class.hInstance = GetModuleHandle(nullptr);
    window_class.hbrBackground = 0;
    window_class.lpszMenuName = nullptr;
    window_class.lpfnWndProc = LynxWindow::WndProc;
    RegisterClass(&window_class);
    class_registered_ = true;
  }
  return kWindowClassName;
}

// Scale helper to convert logical scaler values to physical using passed in
// scale factor
int Scale(int source, double scale_factor) {
  return static_cast<int>(source * scale_factor);
}

std::string GetExecutableDirectoryPath() {
  char path[MAX_PATH] = {0};
  DWORD length = GetModuleFileNameA(GetModuleHandle(nullptr), path, MAX_PATH);
  if (length == 0 || length == MAX_PATH) {
    return "";
  }
  std::string full_path(path);
  size_t pos = full_path.find_last_of("\\/");
  if (pos == std::string::npos) {
    return "";
  }
  return full_path.substr(0, pos);
}

void LoadHomePage(std::vector<uint8_t> &data) {
  std::string path = GetExecutableDirectoryPath();
  if (!path.empty()) {
    path.append("\\");
  }
  std::ifstream file_rstream(path + "resources\\homepage\\main.lynx.bundle",
                             std::ios::ate | std::ios::binary);
  if (file_rstream) {
    std::streamsize size = file_rstream.tellg();
    file_rstream.seekg(0, std::ios::beg);
    data.resize(size);
    file_rstream.read(reinterpret_cast<char *>(data.data()), size);
  }
}

std::pair<unsigned int, unsigned int> GetInitialWindowSizeForUrl(
    const std::string &url) {
  auto width = kDefaultWindowWidth;
  auto height = kDefaultWindowHeight;
  const std::string bundle_query_separator = ".lynx.bundle?";
  auto query_pos = url.find(bundle_query_separator);
  if (query_pos == std::string::npos) {
    return {width, height};
  }

  std::string query =
      url.substr(query_pos + bundle_query_separator.length(), url.size());
  auto parse_dimension = [&](const std::string &key,
                             unsigned int fallback) -> unsigned int {
    const std::string key_with_equal = key + "=";
    auto key_pos = query.find(key_with_equal);
    if (key_pos == std::string::npos) {
      return fallback;
    }
    auto value_start = key_pos + key_with_equal.length();
    auto value_end = query.find('&', value_start);
    auto value = query.substr(value_start, value_end - value_start);
    if (value.empty()) {
      return fallback;
    }
    auto parsed = std::stoul(value);
    return std::max(kMinWindowSize, static_cast<unsigned int>(parsed));
  };

  width = parse_dimension("width", width);
  height = parse_dimension("height", height);
  return {width, height};
}

bool HasTruthyQueryParam(const std::string &url, const std::string &key) {
  const std::string bundle_query_separator = ".lynx.bundle?";
  auto query_pos = url.find(bundle_query_separator);
  if (query_pos == std::string::npos) {
    return false;
  }

  std::string query =
      url.substr(query_pos + bundle_query_separator.length(), url.size());
  const std::string key_with_equal = key + "=";
  auto key_pos = query.find(key_with_equal);
  while (key_pos != std::string::npos && key_pos != 0 &&
         query[key_pos - 1] != '&') {
    key_pos = query.find(key_with_equal, key_pos + 1);
  }
  if (key_pos == std::string::npos) {
    return false;
  }
  auto value_start = key_pos + key_with_equal.length();
  auto value_end = query.find('&', value_start);
  auto value = query.substr(value_start, value_end - value_start);
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char ch) { return std::tolower(ch); });
  return value == "1" || value == "true" || value == "yes";
}
}  // namespace

class FakeView : public lynx::pub::LynxNativeView {
 public:
  FakeView(void *opaque) : lynx_win_(static_cast<LynxWindow *>(opaque)) {}
  ~FakeView() override {}
  bool OnCreate() override { return true; }
  void OnDestroy() override {}
  void OnAttach() override {}
  void OnDetach() override {}
  void OnLayoutChanged(float left, float top, float width, float height,
                       float pixel_ratio) override {
    lynx::pub::LynxValue detail(lynx::pub::LynxValue::kCreateAsMapTag);
    detail.SetProperty("pixelRatio", lynx::pub::LynxValue(pixel_ratio));
    detail.SetProperty("width", lynx::pub::LynxValue(width));
    detail.SetProperty("height", lynx::pub::LynxValue(height));
    TriggerEvent("resize", std::move(detail));
  }
  void OnPropertiesChanged(const lynx::pub::LynxValue &attrs,
                           const lynx::pub::LynxValue &events) override {}
  void OnMethodInvoked(
      const char *method, const lynx::pub::LynxValue &attrs,
      std::function<void(int, lynx::pub::LynxValue &&)> callback) override {
    callback(kSuccess,
             lynx::pub::LynxValue(lynx::pub::LynxValue::kCreateAsNullTag));
  }
  bool IsScrollEnabled() override { return true; }
  bool IsSurfaceEnabled() override { return true; }

 private:
  LynxWindow *lynx_win_;
};

LynxWindow::LynxWindow(unsigned int origin_x, unsigned int origin_y,
                       unsigned int width, unsigned int height,
                       bool test_bench_replay, bool enable_napi_addon)
    : is_test_bench_replay_(test_bench_replay),
      enable_napi_addon_(enable_napi_addon) {
  auto *window_class = RegisterWindowClass();
  const POINT target_point = {static_cast<LONG>(origin_x),
                              static_cast<LONG>(origin_y)};
  HMONITOR monitor = MonitorFromPoint(target_point, MONITOR_DEFAULTTONEAREST);
  double dpi = GetDpiForMonitor(monitor) / 96.0;
  int x = Scale(origin_x, dpi);
  int y = Scale(origin_y, dpi);
  RECT rect = {0, 0, Scale(width, dpi), Scale(height, dpi)};
  AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, TRUE);
  window_handle_ = CreateWindowEx(
      WS_EX_APPWINDOW, window_class, kWindowClassName,
      WS_VISIBLE | WS_OVERLAPPEDWINDOW, x, y, rect.right - rect.left,
      rect.bottom - rect.top, nullptr, nullptr, GetModuleHandle(nullptr), this);

  RECT frame = GetClientArea();
  frame.left /= dpi;
  frame.right /= dpi;
  frame.top /= dpi;
  frame.bottom /= dpi;
  lynx::pub::LynxView::Builder builder;
  builder.SetScreenSize(frame.right - frame.left, frame.bottom - frame.top, dpi)
      .SetFrame(0, 0, frame.right - frame.left, frame.bottom - frame.top)
      .SetParent(window_handle_)
      .SetGenericResourceFetcher(
          std::make_shared<lynx::example::ExampleGenericResourceFetcher>())
      .RegisterNativeView<FakeView>("x-fake-view", this);
  if (enable_napi_addon_) {
    builder.RegisterNativeModule("LynxNodeAPI", LynxNodeAPIModuleCreator, this);
  }
#if ENABLE_TESTBENCH_REPLAY
  if (is_test_bench_replay_) {
    lynx::embedder::TestBenchReplayDataModule::RegisterJSB(
        [&builder](const std::string &name, napi_module_creator creator) {
          lynx_view_builder_register_native_module(builder.Impl(), name.c_str(),
                                                   creator, nullptr);
        });
  }
#endif
  lynx_view_ = builder.Build();
  // Example of runtime lifecycle observer.
  lynx_view_->RegisterRuntimeLifecycleObserver(
      std::make_shared<lynx::example::ExampleLynxRuntimeLifecycleObserver>(
          enable_napi_addon_ ? reinterpret_cast<uint64_t>(this) : 0));
  child_content_ = reinterpret_cast<HWND>(lynx_view_->GetNativeWindow());
  SetFocus(child_content_);

  LynxWindowManager::GetInstance().PushWindow(this);

#if ENABLE_TESTBENCH_REPLAY
  if (is_test_bench_replay_) {
    auto dpi = GetDpiForHWND(window_handle_) / 96.f;
    test_bench_action_manager_ =
        std::make_shared<lynx::embedder::TestBenchActionManager>(
            lynx_view_, [this, dpi](int width, int height) {
              MoveWindow(window_handle_, 10, 10, std::round(width * dpi),
                         std::round(height * dpi), TRUE);
            });
    test_bench_action_manager_->SetFetchCallback(
        [](const std::string &url_str,
           std::function<void(const std::string &result)> callback) {
          std::string response = example::HttplibClient::Get(url_str);
          callback(response);
        });
  }
#endif
}

LynxWindow::~LynxWindow() {
  LynxWindowManager::GetInstance().RemoveWindow(this);
}

HWND LynxWindow::GetWindowHandle() const { return window_handle_; }

void LynxWindow::LoadTemplate(const std::string &url) {
#if ENABLE_TESTBENCH_REPLAY
  if (is_test_bench_replay_) {
    test_bench_action_manager_->StartWithUrl(url);
    return;
  }

#endif
  auto load_meta = std::make_shared<lynx::pub::LynxLoadMeta>();
  load_meta->SetGlobalProps(std::make_shared<lynx::pub::LynxTemplateData>(
      "{\"theme\":\"light\",\"platform\":\"windows\"}"));
  if (url.empty()) {
    load_meta->SetUrl("assets://homepage");
    std::vector<uint8_t> template_data;
    LoadHomePage(template_data);
    load_meta->SetBinaryData(std::move(template_data));
  } else {
    load_meta->SetUrl(url);
  }
  lynx_view_->LoadTemplate(load_meta);
}

void LynxWindow::Destroy() {
  lynx_view_.reset();
  if (window_handle_) {
    DestroyWindow(window_handle_);
    window_handle_ = nullptr;
  }
}

void LynxWindow::SetQuitOnClose(bool quit_on_close) {
  quit_on_close_ = quit_on_close;
}

RECT LynxWindow::GetClientArea() {
  RECT rect;
  GetClientRect(window_handle_, &rect);
  return rect;
}

// static
LRESULT CALLBACK LynxWindow::WndProc(HWND const window, UINT const message,
                                     WPARAM const wparam,
                                     LPARAM const lparam) noexcept {
  if (message == WM_NCCREATE) {
    auto window_struct = reinterpret_cast<CREATESTRUCT *>(lparam);
    SetWindowLongPtr(window, GWLP_USERDATA,
                     reinterpret_cast<LONG_PTR>(window_struct->lpCreateParams));

    auto that = static_cast<LynxWindow *>(window_struct->lpCreateParams);
    that->window_handle_ = window;
  } else if (LynxWindow *that = GetThisFromHandle(window)) {
    return that->MessageHandler(window, message, wparam, lparam);
  }

  return DefWindowProc(window, message, wparam, lparam);
}

LRESULT
LynxWindow::MessageHandler(HWND hwnd, UINT const message, WPARAM const wparam,
                           LPARAM const lparam) noexcept {
  switch (message) {
    case WM_DESTROY:
      window_handle_ = nullptr;
      Destroy();
      if (quit_on_close_) {
        LynxWindowManager::GetInstance().CloseAllWindow();
        PostQuitMessage(0);
      }
      delete this;
      return 0;

    case WM_DPICHANGED: {
      auto newRectSize = reinterpret_cast<RECT *>(lparam);
      LONG newWidth = newRectSize->right - newRectSize->left;
      LONG newHeight = newRectSize->bottom - newRectSize->top;

      SetWindowPos(hwnd, nullptr, newRectSize->left, newRectSize->top, newWidth,
                   newHeight, SWP_NOZORDER | SWP_NOACTIVATE);

      return 0;
    }
    case WM_SIZE: {
      RECT rect = GetClientArea();
      if (lynx_view_) {
        auto dpi = GetDpiForHWND(window_handle_) / 96.f;
        lynx_view_->UpdateScreenMetrics((rect.right - rect.left) / dpi,
                                        (rect.bottom - rect.top) / dpi, dpi);
        lynx_view_->SetFrame(0, 0, (rect.right - rect.left) / dpi,
                             (rect.bottom - rect.top) / dpi);
      }
      return 0;
    }

    case WM_ACTIVATE:
      if (child_content_ != nullptr) {
        SetFocus(child_content_);
      }
      return 0;

    case LYNX_WINDOW_OPEN:
      std::string *url_ptr = (std::string *)lparam;
      bool is_test_bench_replay = false;
#if ENABLE_TESTBENCH_REPLAY
      if (url_ptr->find("sslocal://arkview?") != std::string::npos) {
        is_test_bench_replay = true;
      }
#endif
      auto [width, height] = GetInitialWindowSizeForUrl(*url_ptr);
      bool enable_napi_addon =
          HasTruthyQueryParam(*url_ptr, "enable_napi_addon");
      auto *window = new lynx::LynxWindow(
          0, 0, width, height, is_test_bench_replay, enable_napi_addon);
      window->SetQuitOnClose(false);
      window->LoadTemplate(*url_ptr);
      delete url_ptr;
      return 0;
  }

  return DefWindowProc(window_handle_, message, wparam, lparam);
}

LynxWindow *LynxWindow::GetThisFromHandle(HWND const window) noexcept {
  return reinterpret_cast<LynxWindow *>(
      GetWindowLongPtr(window, GWLP_USERDATA));
}

}  // namespace lynx
