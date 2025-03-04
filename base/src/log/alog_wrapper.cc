// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "base/include/log/alog_wrapper.h"

namespace lynx {
namespace base {

namespace {

static alog_write_func_ptr s_alog_write_func_ptr = nullptr;

}  // namespace

bool InitAlog(alog_write_func_ptr addr) {
  if (s_alog_write_func_ptr != nullptr) {
    return true;
  }
  if (addr == nullptr) {
    return false;
  }
  s_alog_write_func_ptr = addr;
  return true;
}

void ALogWrite(unsigned int level, const char* tag, const char* msg) {
  if (s_alog_write_func_ptr != nullptr) {
    s_alog_write_func_ptr(level, tag, msg);
  }
}

void ALogWriteV(const char* tag, const char* msg) {
  ALogWrite(ALOG_LEVEL_VERBOSE, tag, msg);
}

void ALogWriteD(const char* tag, const char* msg) {
  ALogWrite(ALOG_LEVEL_DEBUG, tag, msg);
}

void ALogWriteI(const char* tag, const char* msg) {
  ALogWrite(ALOG_LEVEL_INFO, tag, msg);
}

void ALogWriteW(const char* tag, const char* msg) {
  ALogWrite(ALOG_LEVEL_WARN, tag, msg);
}

void ALogWriteE(const char* tag, const char* msg) {
  ALogWrite(ALOG_LEVEL_ERROR, tag, msg);
}

void ALogWriteF(const char* tag, const char* msg) {
  ALogWrite(ALOG_LEVEL_FATAL, tag, msg);
}

}  // namespace base
}  // namespace lynx
