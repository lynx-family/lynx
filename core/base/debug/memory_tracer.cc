// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/debug/memory_tracer.h"

#include <dlfcn.h>
#include <unwind.h>

#include <fstream>

#include "base/include/log/logging.h"
#include "third_party/xhook/libxhook/jni/xhook.h"

void* (*real_malloc)(size_t size);
void (*real_free)(void* ptr);
void* (*real_realloc)(void* ptr, size_t size);
void* (*real_calloc)(size_t nmemb, size_t size);

void* operator new(size_t size) {
  void* p;
  lynx::base::MemoryTracer::SetupRealFunctions();
  p = real_malloc(size);
  lynx::base::MemoryTracer::Instance().RecordAllocation(p, size);
  return p;
}

void* operator new[](size_t size) {
  void* p;
  lynx::base::MemoryTracer::SetupRealFunctions();
  p = real_malloc(size);
  lynx::base::MemoryTracer::Instance().RecordAllocation(p, size);
  return p;
}

void operator delete(void* p) noexcept {
  lynx::base::MemoryTracer::SetupRealFunctions();
  lynx::base::MemoryTracer::Instance().RecordRelease(p);
  real_free(p);
}

void operator delete[](void* p) noexcept {
  lynx::base::MemoryTracer::SetupRealFunctions();
  lynx::base::MemoryTracer::Instance().RecordRelease(p);
  real_free(p);
}

namespace lynx {
namespace base {

constexpr size_t kMaxStackDepth = 16;
static Dl_info so_dl_info_;

static void* local_malloc(size_t size) {
  void* p;
  p = real_malloc(size);
  lynx::base::MemoryTracer::Instance().RecordAllocation(p, size);
  return p;
}

static void local_free(void* ptr) {
  lynx::base::MemoryTracer::Instance().RecordRelease(ptr);
  real_free(ptr);
}

static void* local_realloc(void* ptr, size_t size) {
  void* p;
  p = real_realloc(ptr, size);
  lynx::base::MemoryTracer::Instance().RecordRelease(ptr);
  lynx::base::MemoryTracer::Instance().RecordAllocation(p, size);
  return p;
}

static void* local_calloc(size_t nmemb, size_t size) {
  void* p;
  p = real_calloc(nmemb, size);
  lynx::base::MemoryTracer::Instance().RecordAllocation(p, nmemb * size);
  return p;
}

using LibcFunc = struct {
  const char* name;
  void* local_func;
  void** real_func;
};

static const LibcFunc libc_funcs[] = {
    {"malloc", reinterpret_cast<void*>(local_malloc),
     reinterpret_cast<void**>(&real_malloc)},
    {"realloc", reinterpret_cast<void*>(local_realloc),
     reinterpret_cast<void**>(&real_realloc)},
    {"calloc", reinterpret_cast<void*>(local_calloc),
     reinterpret_cast<void**>(&real_calloc)},
    {"free", reinterpret_cast<void*>(local_free),
     reinterpret_cast<void**>(&real_free)}};

const char kLynxLibraryPathRegex[] = ".*/liblynx\\.so$";

_Unwind_Reason_Code UnwindCallback(_Unwind_Context* context, void* stack) {
  _Unwind_Word ip = _Unwind_GetIP(context);
  auto* s =
      reinterpret_cast<std::vector<uintptr_t, InternalAllocator<uintptr_t>>*>(
          stack);
  s->reserve(kMaxStackDepth);
  if (s->size() < kMaxStackDepth) {
    //    auto offset = reinterpret_cast<uintptr_t>(ip -
    //    (_Unwind_Word)so_dl_info_.dli_fbase); s->push_back(offset);
    s->push_back(ip);
    return _URC_NO_REASON;
  }
  return _URC_END_OF_STACK;
}

MemoryTracer& MemoryTracer::Instance() {
  static std::unique_ptr<MemoryTracer> instance_;
  static std::once_flag instance_once_flag;
  std::call_once(instance_once_flag, [&]() {
    SetupRealFunctions();
    instance_.reset(
        static_cast<MemoryTracer*>(real_malloc(sizeof(MemoryTracer))));
    ::new (instance_.get()) MemoryTracer();
  });
  return *instance_;
}

int MemoryTracer::AddrToBufferIndex(void* addr) {
  return (reinterpret_cast<uintptr_t>(addr) >> 8) & (kBufferCount - 1);
}

void MemoryTracer::SetupRealFunctions() {
  static std::once_flag setup_once_flag;
  std::call_once(setup_once_flag, [] {
    for (auto func : libc_funcs) {
      *(func.real_func) = dlsym(RTLD_NEXT, func.name);
      if (*(func.real_func) == nullptr) {
        LOGF("failed to get symbol:" << func.name);
      }
    }
    dladdr(reinterpret_cast<const void*>(SetupRealFunctions), &so_dl_info_);
  });
}

void MemoryTracer::InstallLibcFunctionsHook() {
  for (auto func : libc_funcs) {
    xhook_register(kLynxLibraryPathRegex, func.name, func.local_func, nullptr);
    if (*(func.real_func) == nullptr) {
      LOGF("failed to get symbol:" << func.name);
    }
  }
  xhook_refresh(1);
}

void MemoryTracer::UnInstallLibcFunctionsHook() {
  SetupRealFunctions();
  for (auto func : libc_funcs) {
    xhook_register(kLynxLibraryPathRegex, func.name, *(func.real_func),
                   nullptr);
  }
  xhook_refresh(1);
}

void MemoryTracer::InitBuffer() {
  for (unsigned int i = 0; i < kBufferCount; i++) {
    record_buffers_[i];
  }
}

void MemoryTracer::RecordAllocation(void* ptr, size_t size) {
  if (!enable_.load() || !ptr ||
      size < static_cast<size_t>(min_watched_size_)) {
    return;
  }
  int buffer_index = AddrToBufferIndex(ptr);
  auto& buffer = record_buffers_.at(buffer_index);
  Record record{};
  record.addr = reinterpret_cast<uintptr_t>(ptr);
  record.size = size;
  _Unwind_Backtrace(UnwindCallback, &record.stack);
  buffer.AddRecord(record);
}

void MemoryTracer::RecordRelease(void* ptr) {
  if (!enable_.load() || !ptr) {
    return;
  }
  int buffer_index = AddrToBufferIndex(ptr);
  auto& buffer = record_buffers_.at(buffer_index);
  buffer.RemoveRecord(reinterpret_cast<uintptr_t>(ptr));
}

void MemoryTracer::StartTracing(int min_watched_size) {
  if (enable_.load()) {
    LOGW("MemoryTracer: tracing already started.");
    return;
  }
  LOGI("MemoryTracer: start tracing.");
  if (record_buffers_.size() < kBufferCount) {
    InitBuffer();
  }
  min_watched_size_ = min_watched_size;
  InstallLibcFunctionsHook();
  enable_.store(true);
}

void MemoryTracer::StopTracing() {
  if (!enable_.load()) {
    LOGW("MemoryTracer: tracing not started.");
  }
  LOGI("MemoryTracer: stop tracing.");
  enable_.store(false);
  UnInstallLibcFunctionsHook();
  // buffer.Clear holds the lock to wait for all the recording operations
  // finishing.
  for (auto& buffer : record_buffers_) {
    buffer.second.Clear();
  }
  record_buffers_.clear();
}

void MemoryTracer::WriteRecordsToFile(const std::string& file_path) {
  if (!enable_.load()) {
    LOGW("MemoryTracer: tracing not started.");
    return;
  }
  std::ofstream output(file_path);
  if (!output.is_open()) {
    LOGE("MemoryTracer: failed to create file(" << strerror(errno)
                                                << "): " << file_path);
    return;
  }
  output << "Lynx Memory Report:" << std::endl;
  for (auto& pair : record_buffers_) {
    pair.second.DumpRecordsToStream(output);
  }
  LOGI("MemoryTracer: dump memory to " << file_path);
}

void MemoryTracer::Enable() { enable_.store(true); }

void MemoryTracer::Disable() { enable_.store(false); }

MemoryTracer::~MemoryTracer() {
  record_buffers_.clear();
  DlInfo::Instance().ClearCache();
}

}  // namespace base
}  // namespace lynx
