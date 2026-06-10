// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <Windows.h>

#include "base/include/string/string_conversion_win.h"
#include "platform/embedder/switch_persist.h"

constexpr wchar_t regKeyBase[] = L"Software\\Lynx\\DevTool";

namespace lynx {
namespace embedder {

static std::wstring GetAppSpecificRegKey() {
  // Cache the registry key path since it won't change during execution
  static std::wstring cachedKey = []() {
    // Create a unique registry key per application.
    // We use the executable name as the key.

    TCHAR exePath[MAX_PATH];
    DWORD length = GetModuleFileName(NULL, exePath, MAX_PATH);

    if (length == 0 || length == MAX_PATH) {
      // Fallback if we can't get the executable path
      return std::wstring(regKeyBase) + L"\\Default";
    }

    // Convert TCHAR to wstring (handles both UNICODE and MBCS builds)
    std::wstring fullPath(exePath);

    // Extract just the executable name (without path and extension)
    size_t lastSlash = fullPath.find_last_of(L"\\/");
    std::wstring exeName = (lastSlash != std::wstring::npos)
                               ? fullPath.substr(lastSlash + 1)
                               : fullPath;

    // app.exe -> app
    size_t dotPos = exeName.find_last_of(L'.');
    if (dotPos != std::wstring::npos) {
      exeName = exeName.substr(0, dotPos);
    }

    std::wstring sanitized;
    for (const wchar_t& c : exeName) {
      if (iswalnum(c) || c == L'-') {
        sanitized += c;
      } else if (c == L'_') {
        // Escape underscore to distinguish from replaced spaces
        sanitized += L"__";
      } else if (c == L' ') {
        sanitized += L'_';
      }
      // Other invalid characters are simply removed
    }

    if (sanitized.empty()) {
      sanitized = L"Default";
    }

    return std::wstring(regKeyBase) + L"\\" + sanitized;
  }();

  return cachedKey;
}

bool SwitchPersist::SetValueToPersistent(const std::string& key, bool value) {
  HKEY hKey;
  LONG result;

  std::wstring app_reg_key = GetAppSpecificRegKey();
  result = RegCreateKeyEx(HKEY_CURRENT_USER, app_reg_key.c_str(),
                          0,                        // Reserved
                          NULL,                     // Class
                          REG_OPTION_NON_VOLATILE,  // Options
                          KEY_WRITE,                // Access
                          NULL,                     // Security
                          &hKey,                    // Result key
                          NULL);                    // Disposition
  if (result != ERROR_SUCCESS) return false;

  std::wstring w_key = base::Utf16FromUtf8(key);
  DWORD dwValue = value ? 1 : 0;
  result = RegSetValueEx(hKey,
                         w_key.c_str(),     // Value name
                         0,                 // Reserved
                         REG_DWORD,         // Type
                         (BYTE*)&dwValue,   // Data
                         sizeof(dwValue));  // Size

  RegCloseKey(hKey);
  return result == ERROR_SUCCESS;
}

bool SwitchPersist::GetValueFromPersistent(const std::string& key,
                                           bool default_value) {
  HKEY hKey;
  LONG result;

  std::wstring app_reg_key = GetAppSpecificRegKey();
  result =
      RegOpenKeyEx(HKEY_CURRENT_USER, app_reg_key.c_str(), 0, KEY_READ, &hKey);
  if (result != ERROR_SUCCESS) return default_value;

  std::wstring w_key = base::Utf16FromUtf8(key);
  DWORD value;
  DWORD size = sizeof(value);
  DWORD type;
  result =
      RegQueryValueEx(hKey, w_key.c_str(), NULL, &type, (BYTE*)&value, &size);
  RegCloseKey(hKey);
  if (result != ERROR_SUCCESS || type != REG_DWORD) return default_value;

  return (value != 0);
}
}  // namespace embedder
}  // namespace lynx
