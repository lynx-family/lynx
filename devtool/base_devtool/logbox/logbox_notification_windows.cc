// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/base_devtool/logbox/logbox_notification_windows.h"

#include <tchar.h>
#include <winuser.h>

#include "base/include/log/logging.h"
#include "base/include/string/string_conversion_win.h"
#include "devtool/base_devtool/logbox/logbox_base.h"

namespace {
#define CHILD_WINDOW_CLASS_NAME _T("ChildWindowClass")
#define GET_BG_COLOR_BRUSH() CreateSolidBrush(RGB(21, 21, 21))

constexpr int32_t NOTIFICATION_MARGIN = 5;
constexpr int32_t NOTIFICATION_HEIGHT = 45;
constexpr int32_t MARGIN = 9;
constexpr int32_t LABEL_X = MARGIN;
constexpr int32_t LABEL_WIDTH = NOTIFICATION_HEIGHT - (MARGIN * 2);
constexpr float LABEL_RADIUS = LABEL_WIDTH / 2.0;
constexpr int32_t SEPARATOR_X = NOTIFICATION_HEIGHT;
constexpr int32_t SEPARATOR_WIDTH = 1;
constexpr int32_t TEXT_X = SEPARATOR_X + SEPARATOR_WIDTH + MARGIN;
constexpr int32_t CLOSE_MARGIN = 11;
constexpr int32_t CLOSE_WIDTH = NOTIFICATION_HEIGHT - (CLOSE_MARGIN * 2);
COLORREF labelColors[3] = {
    RGB(228, 67, 105),  // red
    RGB(239, 189, 54),  // yellow
    RGB(92, 93, 93)     // grey
};

void SetChildWindowDrawRegion(HWND hChildWnd) {
  if (!hChildWnd) {
    return;
  }
  RECT childRect;
  GetClientRect(hChildWnd, &childRect);
  int32_t cornerRadius = 15;                        // round corder radius
  HRGN hRgn = CreateRoundRectRgn(0,                 // left
                                 0,                 // top
                                 childRect.right,   // right
                                 childRect.bottom,  // bottom
                                 cornerRadius,      // nWidthEllipse
                                 cornerRadius       // nHeightEllipse
  );

  if (hRgn) {
    // SetWindowRgn will take charge of hRgn, no need to DeleteObject explicitly
    SetWindowRgn(hChildWnd, hRgn, FALSE);
  }
}

LRESULT CALLBACK ChildWndProc(HWND hWnd, UINT message, WPARAM wParam,
                              LPARAM lParam) {
  auto *that = reinterpret_cast<lynx::devtool::LogBoxNotificationHolder *>(
      GetWindowLongPtr(hWnd, GWLP_USERDATA));
  if (!that) {
    return DefWindowProc(hWnd, message, wParam, lParam);
  }
  switch (message) {
    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hWnd, &ps);
      that->ContentPaint(hWnd, hdc);
      EndPaint(hWnd, &ps);
    } break;
    case WM_LBUTTONDOWN:
      that->OnLButtonDown(lParam);
      return DefWindowProc(hWnd, message, wParam, lParam);
    case WM_LBUTTONUP:
      that->OnLButtonUp(lParam);
      break;
    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return 0;
}

bool RegisterChildWindowClass(HINSTANCE hInstance) {
  WNDCLASS wce = {};

  wce.style = CS_HREDRAW | CS_VREDRAW;
  wce.lpfnWndProc = ChildWndProc;
  wce.hInstance = hInstance;
  wce.hCursor = LoadCursor(hInstance, IDC_ARROW);
  wce.hbrBackground = GET_BG_COLOR_BRUSH();
  wce.lpszClassName = CHILD_WINDOW_CLASS_NAME;

  return RegisterClass(&wce);
}

}  // namespace

namespace lynx {
namespace devtool {

HINSTANCE LogBoxNotificationWindows::hInst_ = nullptr;

void LogBoxNotificationHolder::PaintLabel(HDC hdc, RECT &rect,
                                          COLORREF labelColor) {
  int label_left = MARGIN;
  int label_top = MARGIN;
  int label_right = label_left + LABEL_WIDTH;
  int label_bottom = label_top + LABEL_WIDTH;

  HBRUSH label_brush = CreateSolidBrush(labelColor);
  HPEN label_pen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
  HBRUSH old_brush = (HBRUSH)SelectObject(hdc, label_brush);
  HPEN old_pen = (HPEN)SelectObject(hdc, label_pen);
  Ellipse(hdc, label_left, label_top, label_right,
          label_bottom);  // Draw circle

  SetBkMode(hdc, TRANSPARENT);
  SetTextColor(hdc, RGB(255, 255, 255));

  rect.left = label_left;
  rect.top = label_top;
  rect.right = label_right;
  rect.bottom = label_bottom;
  auto count_str = std::to_wstring(log_count_);
  DrawText(hdc, count_str.c_str(), -1, &rect,
           DT_CENTER | DT_VCENTER | DT_SINGLELINE);

  SelectObject(hdc, old_brush);
  SelectObject(hdc, old_pen);
  DeleteObject(label_brush);
  DeleteObject(label_pen);
}

void LogBoxNotificationHolder::PaintSeparator(HDC hdc) {
  HPEN label_pen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
  SelectObject(hdc, label_pen);

  MoveToEx(hdc, SEPARATOR_X, MARGIN, NULL);
  LineTo(hdc, SEPARATOR_X, NOTIFICATION_HEIGHT - MARGIN);
  DeleteObject(label_pen);
}

void LogBoxNotificationHolder::PaintText(HDC hdc, RECT &rect) {
  rect.left = TEXT_X;
  rect.right = notificationWidth_ - NOTIFICATION_HEIGHT;

  // Clear the text area first
  HBRUSH bgBrush = GET_BG_COLOR_BRUSH();
  FillRect(hdc, &rect, bgBrush);
  DeleteObject(bgBrush);

  SetBkMode(hdc, TRANSPARENT);  // set the background of text to be transparent
  SetTextColor(hdc, RGB(255, 255, 255));

  std::wstring w_text = lynx::base::Utf16FromUtf8(msg_);
  DrawText(hdc,             // hdc: context
           w_text.c_str(),  // lpchText: text
           -1,     // size of the text(-1 determins that it ends with NULL)
           &rect,  // lprc:paint rect
           DT_LEFT | DT_VCENTER | DT_SINGLELINE  // format options
  );
}

void LogBoxNotificationHolder::PaintCloseBtn(HDC hdc, RECT &rect) {
  int close_left = notificationWidth_ - CLOSE_WIDTH - CLOSE_MARGIN;
  int close_top = CLOSE_MARGIN;
  int close_right = close_left + CLOSE_WIDTH;
  int close_bottom = close_top + CLOSE_WIDTH;

  HBRUSH close_brush = CreateSolidBrush(RGB(113, 114, 114));
  HBRUSH old_brush = (HBRUSH)SelectObject(hdc, close_brush);
  HPEN old_pen = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
  Ellipse(hdc, close_left, close_top, close_right,
          close_bottom);  // Draw circle

  HPEN white_pen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
  SelectObject(hdc, white_pen);

  float margin = CLOSE_WIDTH * 0.3;
  int x1 = close_left + margin;
  int y1 = close_top + margin;
  int x2 = close_right - margin;
  int y2 = close_bottom - margin;

  // Draw cross
  MoveToEx(hdc, x1, y1, NULL);
  LineTo(hdc, x2, y2);
  MoveToEx(hdc, x2, y1, NULL);
  LineTo(hdc, x1, y2);

  SelectObject(hdc, old_brush);
  SelectObject(hdc, old_pen);
  DeleteObject(close_brush);
  DeleteObject(white_pen);
}

void LogBoxNotificationHolder::ContentPaint(HWND hWnd, HDC hdc) {
  RECT rect;
  GetClientRect(hWnd, &rect);

  PaintLabel(hdc, rect, labelColors[level_]);
  PaintSeparator(hdc);
  PaintText(hdc, rect);
  PaintCloseBtn(hdc, rect);
}

void LogBoxNotificationHolder::OnLButtonDown(LPARAM lParam) {
  if (click_state_ != ClickState::None) {
    click_state_ = ClickState::None;
  }
  if (IsCloseBtnArea(lParam)) {
    click_state_ = ClickState::Close;
  } else {
    click_state_ = ClickState::Detail;
  }
}

void LogBoxNotificationHolder::OnLButtonUp(LPARAM lParam) {
  BOOL is_close_btn_area = IsCloseBtnArea(lParam);
  if (click_state_ == ClickState::Close && is_close_btn_area) {
    owner_->HandleClose(level_);
  } else if (click_state_ == ClickState::Detail && !is_close_btn_area) {
    owner_->HandleClick(level_);
  }
  click_state_ = ClickState::None;
}

BOOL LogBoxNotificationHolder::IsCloseBtnArea(LPARAM lParam) {
  int32_t x = LOWORD(lParam);

  int close_left = notificationWidth_ - CLOSE_WIDTH - CLOSE_MARGIN;
  int close_right = close_left + CLOSE_WIDTH;

  return x >= close_left && x <= close_right;
}

void LogBoxNotificationWindows::LayoutChildWindow() {
  if (!hWnd_) {
    return;
  }
  RECT new_rect;
  GetWindowRect(hWnd_, &new_rect);
  if (EqualRect(&last_rect_, &new_rect)) return;

  RECT parentRect;
  GetClientRect(hWnd_, &parentRect);
  notificationWidth_ =
      parentRect.right - parentRect.left - 2 * NOTIFICATION_MARGIN;
  POINT offset{parentRect.left, parentRect.top};
  ClientToScreen(hWnd_, &offset);
  int32_t index = 0;
  for (auto &holder : holders_) {
    holder.SetWidth(notificationWidth_);
    auto child_window = holder.GetWindow();
    SetWindowPos(child_window, NULL, offset.x + NOTIFICATION_MARGIN,
                 offset.y + NOTIFICATION_MARGIN +
                     index * (NOTIFICATION_HEIGHT + NOTIFICATION_MARGIN),
                 notificationWidth_, NOTIFICATION_HEIGHT,
                 SWP_NOZORDER | SWP_NOACTIVATE);
    SetChildWindowDrawRegion(child_window);
    ++index;
  }
}

LogBoxNotificationWindows::LogBoxNotificationWindows(
    std::weak_ptr<LogBoxManager> manager)
    : LogBoxNotification(manager) {
  hInst_ = GetModuleHandle(nullptr);
  RegisterChildWindowClass(hInst_);
}

HWND LogBoxNotificationWindows::CreateChildWindow(HINSTANCE hInstance,
                                                  HWND hParentWnd,
                                                  int32_t index) {
  if (!hParentWnd || !hInstance) {
    LOGE("CreateChildWindow: parent window is NULL");
    return NULL;
  }

  // Create a pop up window owned by the native window
  // Cannot use child window as clay may directly draw on DC
  HWND hChildWnd = CreateWindowEx(WS_EX_LAYERED,              // dwExStyle
                                  CHILD_WINDOW_CLASS_NAME,    // lpClassName
                                  _T("Notification Window"),  // lpWindowName
                                  WS_POPUP | WS_VISIBLE,      // dwStyle
                                  last_rect_.left,            // x
                                  last_rect_.top,             // y
                                  last_rect_.right - last_rect_.left,  // nWidth
                                  NOTIFICATION_HEIGHT,  // nHeight
                                  hParentWnd,           // hWndParent: the owner
                                  NULL,                 // hMenu:
                                  hInstance,            // hInstance
                                  NULL                  // lpParam
  );
  if (hChildWnd == NULL) {
    DWORD err = GetLastError();
    LOGE("Failed to create child window, err:" << err);
    return nullptr;
  }
  SetLayeredWindowAttributes(hChildWnd, NULL, 243, LWA_ALPHA);
  return hChildWnd;
}

void LogBoxNotificationWindows::UpdateInfo(int32_t level,
                                           const std::string &msg,
                                           int log_count) {
  bool is_new_holder = true;
  for (auto iter = holders_.begin(); iter != holders_.end(); ++iter) {
    if (iter->GetLevel() == level) {
      iter->UpdateInfo(level, msg, log_count);
      holders_.splice(holders_.begin(), holders_, iter);
      is_new_holder = false;
      break;
    }
  }
  if (is_new_holder) {
    holders_.emplace_front(this);
    LogBoxNotificationHolder &newHolder = holders_.front();
    newHolder.UpdateInfo(level, msg, log_count);
    newHolder.SetWindow(CreateChildWindow(hInst_, hWnd_, holders_.size()));
    SetWindowLongPtr(newHolder.GetWindow(), GWLP_USERDATA,
                     reinterpret_cast<LONG_PTR>(&newHolder));
  }
  LayoutChildWindow();  // layout for the first time child window paints
  for (auto iter = holders_.begin(); iter != holders_.end(); ++iter) {
    iter->Invalidate();
  }
}

void LogBoxNotificationWindows::Show() {
  for (auto &holder : holders_) {
    holder.Show();
  }
}
void LogBoxNotificationWindows::Hide() {
  for (auto &holder : holders_) {
    holder.Hide();
  }
}

void LogBoxNotificationWindows::HideNotification(int32_t level) {
  bool found = false;
  for (auto iter = holders_.begin(); iter != holders_.end(); iter++) {
    if (iter->GetLevel() == level) {
      iter->Hide();
      holders_.splice(holders_.end(), holders_,
                      iter);  // holder's position in the list determins the
                              // location in window
      found = true;
      break;
    }
  }
  if (found) {
    LayoutChildWindow();
  }
}

void LogBoxNotificationWindows::Destroy() {
  for (auto &holder : holders_) {
    holder.Destroy();
  }
  holders_.clear();
}

void LogBoxNotificationHolder::UpdateInfo(int32_t level, const std::string &msg,
                                          int log_count) {
  level_ = level;
  msg_ = msg;
  log_count_ = log_count;
  // Show();
}

void LogBoxNotificationHolder::Show() {
  if (hWnd_ == nullptr) {
    return;
  }
  ShowWindow(hWnd_, SW_SHOW);
}

void LogBoxNotificationHolder::Hide() {
  if (hWnd_ == nullptr) {
    return;
  }
  ShowWindow(hWnd_, SW_HIDE);
}

void LogBoxNotificationHolder::Destroy() {
  if (hWnd_ != nullptr) {
    DestroyWindow(hWnd_);
    hWnd_ = nullptr;
  }
}

void LogBoxNotificationHolder::Invalidate() {
  if (hWnd_ != nullptr) {
    InvalidateRect(hWnd_, NULL, FALSE);
  }
}

void LogBoxNotificationWindows::SetNativeWindow(void *window) {
  hWnd_ = (HWND)window;
}

void LogBoxNotificationWindows::OnNativeWindowChange() { LayoutChildWindow(); }

// Factory method implementation
std::shared_ptr<LogBoxNotification> LogBoxNotification::Create(
    std::weak_ptr<LogBoxManager> manager) {
  return std::make_shared<LogBoxNotificationWindows>(manager);
}

}  // namespace devtool
}  // namespace lynx
