// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/**
 * this file for time different interface and struct in different platform
 */
#ifndef CORE_RUNTIME_VM_LEPUS_TT_TM_H_
#define CORE_RUNTIME_VM_LEPUS_TT_TM_H_
namespace lynx {
namespace lepus {

void GetTimeZone(tm_extend& tm);

#if defined(OS_WIN)
#include <windows.h>
#include <winsock.h>

// struct timeval defines in winsock.h of MSVC
int gettimeofday(struct timeval* tp, void* tzp) {
  std::chrono::system_clock::duration d =
      std::chrono::system_clock::now().time_since_epoch();
  std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(d);
  tp->tv_sec = s.count();
  tp->tv_usec =
      std::chrono::duration_cast<std::chrono::microseconds>(d - s).count();
  return 0;
}

#define timegm _mkgmtime

#define localtime_r(T, Tm) localtime_s(Tm, T)

#define gmtime_r(T, Tm) gmtime_s(Tm, T)

void GetTimeZone(tm_extend& tm) {
  const bool is_dst = tm.tm_isdst > 0;
  _get_timezone(&tm.tm_gmtoff);
  long dstbias;
  _get_dstbias(&dstbias);
  tm.tm_gmtoff = tm.tm_gmtoff + (is_dst ? dstbias : 0);
}

#else  // other platform
void GetTimeZone(tm_extend& tm) {
  // do nothing
}

#endif  // OS_WIN
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_TT_TM_H_
