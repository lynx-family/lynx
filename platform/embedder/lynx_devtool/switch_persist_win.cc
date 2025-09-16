// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <Windows.h>

#include "base/include/string/string_conversion_win.h"
#include "platform/embedder/lynx_devtool/devtool_env_embedder.h"
#include "platform/embedder/lynx_devtool/switch_persist.h"

constexpr wchar_t regKey[] = L"Software\\Lynx\\DevTool";

namespace lynx {
namespace embedder {
bool SwitchPersist::SetValueToPersistent(const std::string& key, bool value) {
  HKEY hKey;
  LONG result;

  result = RegCreateKeyEx(HKEY_CURRENT_USER, regKey,
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

  result = RegOpenKeyEx(HKEY_CURRENT_USER, regKey, 0, KEY_READ, &hKey);
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
