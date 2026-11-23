// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/windows/egl/child_window_win.h"

#include <dxgi1_6.h>
#include <wrl/client.h>

#include <atomic>
#include <utility>
#include <vector>

#include "clay/fml/logging.h"
#include "clay/ui/common/isolate.h"

namespace clay {
namespace egl {

namespace {

ATOM g_window_class;
constexpr UINT_PTR kWhiteLevelTimer = 1;

bool FindWhiteLevelTarget(const WCHAR* device_name,
                          DISPLAYCONFIG_SDR_WHITE_LEVEL* white) {
  UINT32 path_count, mode_count;
  if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &path_count,
                                  &mode_count) != ERROR_SUCCESS) {
    return false;
  }
  std::vector<DISPLAYCONFIG_PATH_INFO> paths(path_count);
  std::vector<DISPLAYCONFIG_MODE_INFO> modes(mode_count);
  if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &path_count, paths.data(),
                         &mode_count, modes.data(), nullptr) != ERROR_SUCCESS) {
    return false;
  }
  for (UINT32 i = 0; i < path_count; ++i) {
    const auto& path = paths[i];
    DISPLAYCONFIG_SOURCE_DEVICE_NAME source = {};
    source.header = {DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME, sizeof(source),
                     path.sourceInfo.adapterId, path.sourceInfo.id};
    if (DisplayConfigGetDeviceInfo(&source.header) != ERROR_SUCCESS ||
        _wcsicmp(source.viewGdiDeviceName, device_name) != 0) {
      continue;
    }
    white->header = {DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL,
                     sizeof(*white), path.targetInfo.adapterId,
                     path.targetInfo.id};
    return true;
  }
  return false;
}

void InitializeWindowClass(WNDPROC window_proc) {
  if (g_window_class) {
    return;
  }
  WNDCLASSEX window_class = {0};
  window_class.cbSize = sizeof(WNDCLASSEX);
  window_class.style = CS_OWNDC;
  window_class.lpfnWndProc = window_proc;
  window_class.cbClsExtra = 0;
  window_class.cbWndExtra = 0;
  window_class.hInstance = GetModuleHandle(nullptr);
  window_class.hIcon = nullptr;
  window_class.hCursor = nullptr;
  window_class.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
  window_class.lpszMenuName = nullptr;
  window_class.lpszClassName = L"Intermediate D3D Window";
  window_class.hIconSm = nullptr;
  g_window_class = RegisterClassEx(&window_class);
  if (!g_window_class) {
    FML_LOG(ERROR) << "RegisterClassEx failed: " << GetLastError();
  }
}

}  // namespace

struct ChildWindowWin::WindowState {
  HWND window = nullptr;
  std::atomic<float> white_scale{1.0f};
  Microsoft::WRL::ComPtr<IDXGIFactory1> factory;
  Microsoft::WRL::ComPtr<IDXGIOutput6> output;
  HMONITOR monitor = nullptr;
  DISPLAYCONFIG_SDR_WHITE_LEVEL white = {};

  float CurrentWhiteScale() {
    HMONITOR current = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
    // DXGI output descriptions are snapshots. Re-enumerate after a topology
    // or HDR mode change, including when the window moves to another adapter.
    if (!factory || !factory->IsCurrent()) {
      output.Reset();
      monitor = nullptr;
      factory.Reset();
      if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
        return 1.0f;
      }
    }
    if (monitor != current) {
      monitor = current;
      white = {};
      output.Reset();
      Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
      for (UINT i = 0;
           !output && SUCCEEDED(factory->EnumAdapters1(i, &adapter)); ++i) {
        Microsoft::WRL::ComPtr<IDXGIOutput> candidate;
        for (UINT j = 0; SUCCEEDED(adapter->EnumOutputs(j, &candidate)); ++j) {
          DXGI_OUTPUT_DESC desc = {};
          if (SUCCEEDED(candidate->GetDesc(&desc)) && desc.Monitor == current) {
            candidate.As(&output);
            break;
          }
        }
      }
    }
    DXGI_OUTPUT_DESC1 desc = {};
    if (!output || FAILED(output->GetDesc1(&desc)) ||
        desc.ColorSpace != DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020) {
      return 1.0f;
    }
    if (!white.header.size && !FindWhiteLevelTarget(desc.DeviceName, &white)) {
      return 1.0f;
    }
    if (DisplayConfigGetDeviceInfo(&white.header) != ERROR_SUCCESS ||
        white.SDRWhiteLevel == 0) {
      white = {};
      return 1.0f;
    }
    // Windows reports the SDR reference white in units of 80 nits / 1000.
    return white.SDRWhiteLevel / 1000.0f;
  }

  static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam,
                                     LPARAM lparam) {
    if (message == WM_NCCREATE) {
      auto* create_struct = reinterpret_cast<CREATESTRUCT*>(lparam);
      auto* state = new std::shared_ptr<WindowState>(
          *static_cast<std::shared_ptr<WindowState>*>(
              create_struct->lpCreateParams));
      (*state)->window = window;
      SetWindowLongPtr(window, GWLP_USERDATA,
                       reinterpret_cast<LONG_PTR>(state));
    } else if (message == WM_TIMER && wparam == kWhiteLevelTimer) {
      auto* state = reinterpret_cast<std::shared_ptr<WindowState>*>(
          GetWindowLongPtr(window, GWLP_USERDATA));
      if (state && IsWindowVisible(window)) {
        const float scale = (*state)->CurrentWhiteScale();
        if ((*state)->white_scale.exchange(scale) != scale) {
          // Request a frame even when the retained scene is idle.
          RedrawWindow(GetParent(window), nullptr, nullptr,
                       RDW_INTERNALPAINT | RDW_NOCHILDREN);
        }
      }
      return 0;
    } else if (message == WM_NCDESTROY) {
      auto* state = reinterpret_cast<std::shared_ptr<WindowState>*>(
          GetWindowLongPtr(window, GWLP_USERDATA));
      if (state) {
        (*state)->window = nullptr;
        SetWindowLongPtr(window, GWLP_USERDATA, 0);
        delete state;
      }
    }
    return DefWindowProc(window, message, wparam, lparam);
  }
};

ChildWindowWin::ChildWindowWin(HWND parent_window, bool track_sdr_white)
    : parent_window_(parent_window),
      window_state_(std::make_shared<WindowState>()) {
  Initialize();
  if (track_sdr_white && window()) {
    window_state_->white_scale = window_state_->CurrentWhiteScale();
    // The SDR brightness slider can change without a window/display message.
    SetTimer(window(), kWhiteLevelTimer, 1000, nullptr);
  }
}

void ChildWindowWin::Initialize() {
  if (window_state_->window) {
    return;
  }
  RECT window_rect;
  GetClientRect(parent_window_, &window_rect);
  SIZE size = {window_rect.right - window_rect.left,
               window_rect.bottom - window_rect.top};
  InitializeWindowClass(WindowState::WindowProc);

  HWND window =
      CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_LAYERED | WS_EX_LAYERED |
                         WS_EX_TRANSPARENT | WS_EX_NOREDIRECTIONBITMAP,
                     L"Intermediate D3D Window", L"",
                     WS_CHILDWINDOW | WS_DISABLED | WS_VISIBLE, 0, 0, size.cx,
                     size.cy, parent_window_, nullptr, nullptr, &window_state_);

  if (!window) {
    std::cerr << "CreateWindowEx failed: " << GetLastError() << std::endl;
    return;
  }
}

ChildWindowWin::~ChildWindowWin() {
  if (!task_runner_) {
    task_runner_ = clay::Isolate::Instance().GetPlatformTaskRunner();
  }
  auto destroy_window = [state = window_state_]() {
    if (state->window) {
      DestroyWindow(state->window);
    }
  };
  if (task_runner_) {
    fml::TaskRunner::RunNowOrPostTask(task_runner_, std::move(destroy_window));
  } else {
    destroy_window();
  }
}

HWND ChildWindowWin::window() const { return window_state_->window; }

float ChildWindowWin::sdr_white_scale() const {
  return window_state_->white_scale.load();
}

void ChildWindowWin::Resize(int width, int height) {
  // Force a resize and redraw (but not a move, activate, etc.).
  if (!task_runner_) {
    task_runner_ = clay::Isolate::Instance().GetPlatformTaskRunner();
  }
  if (!task_runner_) {
    return;
  }
  task_runner_->PostTask([state = window_state_, width, height]() {
    if (state->window) {
      SetWindowPos(state->window, nullptr, 0, 0, width, height,
                   SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOCOPYBITS |
                       SWP_NOOWNERZORDER | SWP_NOZORDER);
    }
  });
}

}  // namespace egl

}  // namespace clay
