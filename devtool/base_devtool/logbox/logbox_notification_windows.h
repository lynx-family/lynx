// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_LOGBOX_LOGBOX_NOTIFICATION_WINDOWS_H_
#define DEVTOOL_BASE_DEVTOOL_LOGBOX_LOGBOX_NOTIFICATION_WINDOWS_H_

#include <windows.h>

#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>

#include "devtool/base_devtool/logbox/logbox_notification.h"

namespace lynx {
namespace devtool {
class LogBoxNotificationWindows;
class LogBoxNotificationHolder {
 public:
  LogBoxNotificationHolder(LogBoxNotificationWindows *owner) : owner_(owner) {}
  virtual ~LogBoxNotificationHolder() = default;

  void UpdateInfo(int32_t level, const std::string &msg, int log_count);
  void Show();
  void Hide();
  void Destroy();
  void Invalidate();

  void ContentPaint(HWND hWnd, HDC hdc);
  void OnLButtonDown(LPARAM lParam);
  void OnLButtonUp(LPARAM lParam);

  HWND GetWindow() const { return hWnd_; }
  int32_t GetLevel() const { return level_; }
  void SetWindow(HWND window) { hWnd_ = window; }
  void SetWidth(float width) { notificationWidth_ = width; }
  LogBoxNotificationWindows *owner_ = nullptr;

 private:
  enum class ClickState : int32_t {
    None = 0,
    Detail = 1,
    Close = 2,
  };
  ClickState click_state_ = ClickState::None;
  int32_t level_ = 0;
  int32_t log_count_ = 0;
  std::string msg_;

  HWND hWnd_ = nullptr;
  float notificationWidth_ = 90.0f;

  void PaintLabel(HDC hdc, RECT &rect, COLORREF labelColor);
  void PaintSeparator(HDC hdc);
  void PaintText(HDC hdc, RECT &rect);
  void PaintCloseBtn(HDC hdc, RECT &rect);
  BOOL IsCloseBtnArea(LPARAM lParam);
};

class LogBoxNotificationWindows : public LogBoxNotification {
 public:
  LogBoxNotificationWindows(std::weak_ptr<LogBoxManager> manager);
  virtual ~LogBoxNotificationWindows() = default;

  void UpdateInfo(int32_t level, const std::string &msg,
                  int log_count) override;
  void Show() override;
  void Hide() override;
  void HideNotification(int32_t level) override;
  void Destroy() override;
  void OnNativeWindowChange() override;

  void SetNativeWindow(void *window) override;
  HWND GetWindow() const { return hWnd_; }

 private:
  HWND hWnd_ = nullptr;
  float notificationWidth_ = 90.0f;
  std::list<LogBoxNotificationHolder> holders_;
  RECT last_rect_ = {};

  void LayoutChildWindow();
  HWND CreateChildWindow(HINSTANCE hInstance, HWND hParentWnd, int32_t index);

  static HINSTANCE hInst_;
};
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_BASE_DEVTOOL_LOGBOX_LOGBOX_NOTIFICATION_WINDOWS_H_
