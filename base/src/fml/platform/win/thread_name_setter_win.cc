// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <windows.h>

#include "base/src/fml/thread_name_setter.h"

namespace lynx {
namespace fml {

// The information on how to set the thread name comes from
// a MSDN article: http://msdn2.microsoft.com/en-us/library/xcb2z8hs.aspx
const DWORD kVCThreadNameException = 0x406D1388;
typedef struct tagTHREADNAME_INFO {
  DWORD dwType;      // Must be 0x1000.
  LPCSTR szName;     // Pointer to name (in user addr space).
  DWORD dwThreadID;  // Thread ID (-1=caller thread).
  DWORD dwFlags;     // Reserved for future use, must be zero.
} THREADNAME_INFO;
// The SetThreadDescription API was brought in version 1607 of Windows 10.
typedef HRESULT(WINAPI* SetThreadDescription)(HANDLE hThread,
                                              PCWSTR lpThreadDescription);

void SetThreadName(const std::string& name) {
  if (name == "") {
    return;
  }

  static auto set_thread_description_func =
      reinterpret_cast<SetThreadDescription>(::GetProcAddress(
          ::GetModuleHandle(L"Kernel32.dll"), "SetThreadDescription"));
  if (set_thread_description_func) {
    wchar_t wstr[128];
    mbstowcs(wstr, name.c_str(), 128);
    set_thread_description_func(::GetCurrentThread(), wstr);
  }
  if (::IsDebuggerPresent()) {
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = name.c_str();
    info.dwThreadID = GetCurrentThreadId();
    info.dwFlags = 0;
    __try {
      RaiseException(kVCThreadNameException, 0, sizeof(info) / sizeof(DWORD),
                     reinterpret_cast<DWORD_PTR*>(&info));
    } __except (EXCEPTION_CONTINUE_EXECUTION) {  // NOLINT
    }
  }
}

}  // namespace fml
}  // namespace lynx
