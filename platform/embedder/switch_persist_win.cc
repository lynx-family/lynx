// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <Windows.h>

#include <cwctype>
#include <string>

#include "base/include/string/string_conversion_win.h"
#include "platform/embedder/switch_persist.h"

constexpr wchar_t regKeyBase[] = L"Software\\Lynx\\DevTool";

namespace lynx {
namespace embedder {

namespace {

uint64_t StableHash(const std::wstring& input) {
  constexpr uint64_t kFnvOffsetBasis = 14695981039346656037ull;
  constexpr uint64_t kFnvPrime = 1099511628211ull;

  uint64_t hash = kFnvOffsetBasis;
  for (wchar_t c : input) {
    hash ^= static_cast<uint64_t>(c);
    hash *= kFnvPrime;
  }
  return hash;
}

std::wstring HexEncode(uint64_t value) {
  constexpr wchar_t kHexDigits[] = L"0123456789abcdef";
  std::wstring encoded(16, L'0');
  for (int i = 15; i >= 0; --i) {
    encoded[i] = kHexDigits[value & 0xf];
    value >>= 4;
  }
  return encoded;
}

std::wstring GetExecutablePathOrDefault() {
  wchar_t exe_path[MAX_PATH];
  DWORD length = GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
  if (length == 0 || length == MAX_PATH) {
    return L"Default";
  }
  return std::wstring(exe_path, length);
}

std::wstring GetLegacyRegKey() {
  static std::wstring cached_key = []() {
    std::wstring full_path = GetExecutablePathOrDefault();
    size_t last_slash = full_path.find_last_of(L"\\/");
    std::wstring exe_name = (last_slash != std::wstring::npos)
                                ? full_path.substr(last_slash + 1)
                                : full_path;

    size_t dot_pos = exe_name.find_last_of(L'.');
    if (dot_pos != std::wstring::npos) {
      exe_name = exe_name.substr(0, dot_pos);
    }

    std::wstring sanitized;
    for (const wchar_t& c : exe_name) {
      if (iswalnum(c) || c == L'-') {
        sanitized += c;
      } else if (c == L'_') {
        sanitized += L"__";
      } else if (c == L' ') {
        sanitized += L'_';
      }
    }

    if (sanitized.empty()) {
      sanitized = L"Default";
    }

    return std::wstring(regKeyBase) + L"\\" + sanitized;
  }();
  return cached_key;
}

std::wstring GetScopedRegKey() {
  static std::wstring scoped_key =
      std::wstring(regKeyBase) + L"\\app_" +
      HexEncode(StableHash(GetExecutablePathOrDefault()));
  return scoped_key;
}

bool QueryPersistedValue(HKEY key_handle, const std::wstring& key,
                         DWORD* value) {
  DWORD size = sizeof(*value);
  DWORD type = 0;
  LONG result = RegQueryValueEx(key_handle, key.c_str(), nullptr, &type,
                                reinterpret_cast<BYTE*>(value), &size);
  return result == ERROR_SUCCESS && type == REG_DWORD;
}

bool SetPersistedValue(HKEY key_handle, const std::wstring& key, bool value) {
  DWORD dwValue = value ? 1 : 0;
  LONG result =
      RegSetValueEx(key_handle, key.c_str(), 0, REG_DWORD,
                    reinterpret_cast<const BYTE*>(&dwValue), sizeof(dwValue));
  return result == ERROR_SUCCESS;
}

bool QueryPersistedValue(const std::wstring& reg_key, const std::wstring& key,
                         DWORD* value) {
  HKEY hKey;
  LONG result =
      RegOpenKeyEx(HKEY_CURRENT_USER, reg_key.c_str(), 0, KEY_READ, &hKey);
  if (result != ERROR_SUCCESS) {
    return false;
  }

  bool found = QueryPersistedValue(hKey, key, value);
  RegCloseKey(hKey);
  return found;
}

}  // namespace

bool SwitchPersist::SetValueToPersistent(const std::string& key, bool value) {
  HKEY hKey;
  LONG result;

  std::wstring scoped_reg_key = GetScopedRegKey();
  result = RegCreateKeyEx(HKEY_CURRENT_USER, scoped_reg_key.c_str(),
                          0,                        // Reserved
                          NULL,                     // Class
                          REG_OPTION_NON_VOLATILE,  // Options
                          KEY_WRITE,                // Access
                          NULL,                     // Security
                          &hKey,                    // Result key
                          NULL);                    // Disposition
  if (result != ERROR_SUCCESS) return false;

  std::wstring w_key = base::Utf16FromUtf8(key);
  bool success = SetPersistedValue(hKey, w_key, value);
  RegCloseKey(hKey);
  return success;
}

bool SwitchPersist::GetValueFromPersistent(const std::string& key,
                                           bool default_value) {
  std::wstring w_key = base::Utf16FromUtf8(key);
  DWORD value = 0;
  if (QueryPersistedValue(GetScopedRegKey(), w_key, &value) ||
      QueryPersistedValue(GetLegacyRegKey(), w_key, &value)) {
    return value != 0;
  }
  return default_value;
}
}  // namespace embedder
}  // namespace lynx
