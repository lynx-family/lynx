// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/thread/thread_utils.h"

#include <cstdbool>
#include <thread>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

#if defined(OS_WIN)
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace lynx {
namespace base {

bool SetThreadName(const std::string& name) {
  char buf[16] = {};
  size_t sz = std::min(name.size(), static_cast<size_t>(15));
  strncpy(buf, name.c_str(), sz);

#if defined(OS_IOS) || defined(OS_OSX)
  return pthread_setname_np(buf) == 0;
#elif defined(OS_WIN)
  wchar_t wstr[128];
  mbstowcs(wstr, buf, sizeof(buf));
  HRESULT hr = SetThreadDescription(GetCurrentThread(), wstr);
  return SUCCEEDED(hr);
#else
  return pthread_setname_np(pthread_self(), buf) == 0;
#endif
}

TEST(ThreadUtilsTest, GetCurrentThreadName) {
  std::string threadName = GetCurrentThreadName();
  // Test case 1: Set the thread name and verify it's retrieved correctly
  std::string testThreadName = "MyTestThread";
  SetThreadName(testThreadName);
  EXPECT_EQ(GetCurrentThreadName(), testThreadName);

  // Test case 2: Verify the function returns origin thread name.
  SetThreadName(threadName);
  EXPECT_EQ(GetCurrentThreadName(), threadName);
}

}  // namespace base
}  // namespace lynx
