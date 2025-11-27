// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/windows/platform_handler.h"

#include <windows.h>

#include <cstring>
#include <string>

#include "clay/fml/platform/win/wstring_conversion.h"
#include "clay/shell/platform/windows/flutter_windows_view.h"

static constexpr int kErrorSuccess = 0;

namespace clay {

namespace {
// A Clipboard wrapper that automatically closes the clipboard when it goes out
// of scope.
class ScopedClipboard : public ScopedClipboardInterface {
 public:
  ScopedClipboard();
  virtual ~ScopedClipboard();

  // Prevent copying.
  ScopedClipboard(ScopedClipboard const&) = delete;
  ScopedClipboard& operator=(ScopedClipboard const&) = delete;

  int Open(HWND window) override;

  bool HasString() override;

  std::variant<std::wstring, int> GetString() override;

  int SetString(const std::wstring string) override;

 private:
  bool opened_ = false;
};

ScopedClipboard::ScopedClipboard() {}

ScopedClipboard::~ScopedClipboard() {
  if (opened_) {
    ::CloseClipboard();
  }
}

int ScopedClipboard::Open(HWND window) {
  opened_ = ::OpenClipboard(window);

  if (!opened_) {
    return ::GetLastError();
  }

  return kErrorSuccess;
}

bool ScopedClipboard::HasString() {
  // Allow either plain text format, since getting data will auto-interpolate.
  return ::IsClipboardFormatAvailable(CF_UNICODETEXT) ||
         ::IsClipboardFormatAvailable(CF_TEXT);
}

std::variant<std::wstring, int> ScopedClipboard::GetString() {
  assert(opened_);

  HANDLE data = ::GetClipboardData(CF_UNICODETEXT);
  if (data == nullptr) {
    return static_cast<int>(::GetLastError());
  }
  ScopedGlobalLock locked_data(data);

  if (!locked_data.get()) {
    return static_cast<int>(::GetLastError());
  }
  return static_cast<wchar_t*>(locked_data.get());
}

int ScopedClipboard::SetString(const std::wstring string) {
  assert(opened_);
  if (!::EmptyClipboard()) {
    return ::GetLastError();
  }
  size_t null_terminated_byte_count =
      sizeof(decltype(string)::traits_type::char_type) * (string.size() + 1);
  ScopedGlobalMemory destination_memory(GMEM_MOVEABLE,
                                        null_terminated_byte_count);
  ScopedGlobalLock locked_memory(destination_memory.get());
  if (!locked_memory.get()) {
    return ::GetLastError();
  }
  memcpy(locked_memory.get(), string.c_str(), null_terminated_byte_count);
  if (!::SetClipboardData(CF_UNICODETEXT, locked_memory.get())) {
    return ::GetLastError();
  }
  // The clipboard now owns the global memory.
  destination_memory.release();
  return kErrorSuccess;
}

}  // namespace

PlatformHandler::PlatformHandler(
    FlutterWindowsEngine* engine,
    std::optional<std::function<std::unique_ptr<ScopedClipboardInterface>()>>
        scoped_clipboard_provider)
    : engine_(engine) {
  if (scoped_clipboard_provider.has_value()) {
    scoped_clipboard_provider_ = scoped_clipboard_provider.value();
  } else {
    scoped_clipboard_provider_ = []() {
      return std::make_unique<ScopedClipboard>();
    };
  }
}

PlatformHandler::~PlatformHandler() = default;

void PlatformHandler::SetPlainText(const std::string& text) {
  const FlutterWindowsView* view = engine_->view();
  if (view == nullptr) {
    FML_LOG(ERROR) << "Clipboard is not available in Windows headless mode";
    return;
  }
  std::unique_ptr<ScopedClipboardInterface> clipboard =
      scoped_clipboard_provider_();
  int open_result = clipboard->Open(std::get<HWND>(*view->GetRenderTarget()));
  if (open_result != kErrorSuccess) {
    FML_LOG(ERROR) << "Unable to open clipboard, error_code = " << open_result;
    return;
  }
  int set_result = clipboard->SetString(fml::Utf8ToWideString(text));
  if (set_result != kErrorSuccess) {
    FML_LOG(ERROR) << "Unable to set clipboard data, error_code = "
                   << set_result;
  }
}
const std::string& PlatformHandler::GetPlainText() const {
  last_clipboard_text_.clear();
  const FlutterWindowsView* view = engine_->view();
  if (view == nullptr) {
    FML_LOG(ERROR) << "Clipboard is not available in Windows headless mode";
    return std::string();
  }
  std::unique_ptr<ScopedClipboardInterface> clipboard =
      scoped_clipboard_provider_();
  int open_result = clipboard->Open(std::get<HWND>(*view->GetRenderTarget()));
  if (open_result != kErrorSuccess) {
    FML_LOG(ERROR) << "Unable to open clipboard, error_code = " << open_result;
    return std::string();
  }
  if (!clipboard->HasString()) {
    return std::string();
  }
  std::variant<std::wstring, int> get_string_result = clipboard->GetString();
  if (std::holds_alternative<int>(get_string_result)) {
    FML_LOG(ERROR) << "Unable to get clipboard data, error_code = "
                   << std::get<int>(get_string_result);
    return std::string();
  }
  last_clipboard_text_ =
      fml::WideStringToUtf8(std::get<std::wstring>(get_string_result));
  return last_clipboard_text_;
}

}  // namespace clay
