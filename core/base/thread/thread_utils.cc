// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/thread/thread_utils.h"

#if defined(OS_ANDROID)
#include <sys/prctl.h>
#include <unistd.h>
#elif defined(OS_WIN)
#include <Windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

namespace lynx {
namespace base {

#if defined(OS_WIN)
// The GetThreadDescription API was brought in version 1607 of Windows 10.
typedef HRESULT(WINAPI* GetThreadDescription)(HANDLE hThread,
                                              PWSTR* ppszThreadDescription);
#endif

std::string GetCurrentThreadName() {
  char buf[64] = {};
#if defined(OS_ANDROID)
  if (prctl(PR_GET_NAME, buf) == 0) {
    return buf;
  }
#elif defined(OS_WIN)
  static auto get_thread_description_func =
      reinterpret_cast<GetThreadDescription>(
          reinterpret_cast<void*>(::GetProcAddress(
              ::GetModuleHandleA("Kernel32.dll"), "GetThreadDescription")));
  if (get_thread_description_func) {
    PWSTR data;
    HRESULT hr = get_thread_description_func(GetCurrentThread(), &data);
    if (!FAILED(hr)) {
      wcstombs(buf, data, sizeof(buf));
      LocalFree(data);
      return buf;
    }
  }
#else
  if (pthread_getname_np(pthread_self(), buf, sizeof(buf)) == 0) {
    return buf;
  }
#endif
  return "unknow";  // If thread name is not set, return unknow
}

}  // namespace base
}  // namespace lynx
