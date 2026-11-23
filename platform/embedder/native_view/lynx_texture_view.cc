// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/native_view/lynx_texture_view.h"

// cspell:ignore setsurfacecolorspace

#include <algorithm>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#elif defined(__APPLE__)
#import <IOSurface/IOSurface.h>
#include <unistd.h>
#endif

#include "platform/embedder/public/lynx_value.h"

using lynx::pub::LynxValue;

static double Number(const LynxValue& v) {
  if (v.Type() == lynx_value_double) {
    return v.Double();
  }
  if (v.Type() == lynx_value_int32) {
    return v.Int32();
  }
  if (v.Type() == lynx_value_uint32) {
    return v.UInt32();
  }
  if (v.Type() == lynx_value_int64) {
    return v.Int64();
  }
  if (v.Type() == lynx_value_uint64) {
    return v.UInt64();
  }
  if (v.Type() == lynx_value_string) {
    std::string s = v.StdString();
    return strtod(s.c_str(), nullptr);
  }
  return 0;
}

LynxTextureView::~LynxTextureView() {}

void LynxTextureView::OnDestroy() {
  TriggerEvent("destroy", LynxValue(LynxValue::kCreateAsNullTag));
}

bool LynxTextureView::OnCreate() {
  TriggerEvent("create", LynxValue(LynxValue::kCreateAsNullTag));
  return true;
}

void LynxTextureView::OnAttach() {
  TriggerEvent("attach", LynxValue(LynxValue::kCreateAsNullTag));
}

void LynxTextureView::OnDetach() {
  TriggerEvent("detach", LynxValue(LynxValue::kCreateAsNullTag));
}

void LynxTextureView::OnLayoutChanged(float left, float top, float width,
                                      float height, float pixel_ratio) {
  width_ = width;
  height_ = height;

  LynxValue detail(LynxValue::kCreateAsMapTag);
  detail.SetProperty("pixelRatio", LynxValue(pixel_ratio));
  detail.SetProperty("width", LynxValue(width));
  detail.SetProperty("height", LynxValue(height));
  TriggerEvent("resize", std::move(detail));
}

void LynxTextureView::OnMethodInvoked(
    const char* method, const LynxValue& attrs,
    std::function<void(int, LynxValue&&)> callback) {
  if (strcmp(method, "setsurfacecolorspace") == 0) {
    const auto value = attrs.GetProperty("colorSpace");
    if (value.Type() != lynx_value_string) {
      callback(kInvalidParameter, LynxValue(LynxValue::kCreateAsNullTag));
      return;
    }
    const auto name = value.StdString();
    lynx_color_space_e space;
    if (name == "display-p3") {
      space = kLynxColorSpaceDisplayP3;
    } else if (name == "srgb") {
      space = kLynxColorSpaceSRGB;
    } else {
      callback(kInvalidParameter, LynxValue(LynxValue::kCreateAsNullTag));
      return;
    }
    const bool changed = SetSurfaceColorSpace(space);
    callback(changed ? kSuccess : kInvalidStateError, LynxValue(changed));
  } else if (strcmp(method, "acquiresurface") == 0) {
    lynx_surface_handle_t* handle = AcquireSurface(width_, height_);
    if (handle) {
#ifdef _WIN32
      int32_t pid = GetCurrentProcessId();
      int64_t surface = int64_t(handle);
#elif defined(__APPLE__)
      int32_t pid = getpid();
      int64_t surface = IOSurfaceGetID((IOSurfaceRef)handle);
#endif
      LynxValue detail(LynxValue::kCreateAsMapTag);
      detail.SetProperty("processid", LynxValue(pid));
      detail.SetProperty("surface", LynxValue(surface));
      detail.SetProperty("width", LynxValue(width_));
      detail.SetProperty("height", LynxValue(height_));
      detail.SetProperty(
          "colorSpace",
          LynxValue(GetSurfaceColorSpace() == kLynxColorSpaceDisplayP3
                        ? "display-p3"
                        : "srgb"));
      callback(kSuccess, std::move(detail));
      return;
    }
    callback(kInvalidStateError, LynxValue(LynxValue::kCreateAsNullTag));
  } else if (strcmp(method, "swapbuffer") == 0) {
    bool ret = SwapBack();
    callback(kSuccess, LynxValue(ret));
  } else if (strcmp(method, "update") == 0) {
    double surface = Number(attrs.GetProperty("surface"));
#ifdef _WIN32
    static const float transform[3 * 3] = {1, 0, 0, 0, -1, 1, 0, 0, 1};
    bool ret = false;
    DWORD pid = Number(attrs.GetProperty("processid"));
    HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    HANDLE texture;
    if (process &&
        DuplicateHandle(process, reinterpret_cast<HANDLE>((DWORD)surface),
                        GetCurrentProcess(), &texture, 0, FALSE,
                        DUPLICATE_SAME_ACCESS)) {
      ret = PresentSurface(0, 0, transform,
                           reinterpret_cast<lynx_surface_handle_t*>(texture));
      CloseHandle(texture);
    }
    CloseHandle(process);
#elif defined(__APPLE__)
    static const float transform[3 * 3] = {1, 0, 0, 0, -1, 1, 0, 0, 1};
    IOSurfaceRef ref = IOSurfaceLookup((IOSurfaceID)surface);
    bool ret = false;
    if (ref) {
      ret = PresentSurface(IOSurfaceGetWidth(ref), IOSurfaceGetHeight(ref),
                           transform,
                           reinterpret_cast<lynx_surface_handle_t*>(ref));
      CFRelease(ref);
    }
#endif
    callback(kSuccess, LynxValue(ret));
  } else {
    callback(kMethodNotFound, LynxValue(LynxValue::kCreateAsNullTag));
  }
}
