// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_LOG_ALOG_WRAPPER_H_
#define BASE_INCLUDE_LOG_ALOG_WRAPPER_H_

namespace lynx {
namespace base {

#define ALOG_LEVEL_VERBOSE 0
#define ALOG_LEVEL_DEBUG 1
#define ALOG_LEVEL_INFO 2
#define ALOG_LEVEL_WARN 3
#define ALOG_LEVEL_ERROR 4

// This parameter is only valid in darwin.
#define ALOG_LEVEL_FATAL 5

using alog_write_func_ptr = void (*)(unsigned int level, const char* tag,
                                     const char* msg);
using alog_init_mutex_cond_thread_ptr = void (*)(void);

bool InitAlog(alog_write_func_ptr addr);

void ALogWrite(unsigned int level, const char* tag, const char* msg);

void ALogWriteV(const char* tag, const char* msg);
void ALogWriteD(const char* tag, const char* msg);
void ALogWriteI(const char* tag, const char* msg);
void ALogWriteW(const char* tag, const char* msg);
void ALogWriteE(const char* tag, const char* msg);
void ALogWriteF(const char* tag, const char* msg);

}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_LOG_ALOG_WRAPPER_H_
