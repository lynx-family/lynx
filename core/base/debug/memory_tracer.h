// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_DEBUG_MEMORY_TRACER_H_
#define CORE_BASE_DEBUG_MEMORY_TRACER_H_

#include <dlfcn.h>

#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

extern void* (*real_malloc)(size_t size);
extern void (*real_free)(void* ptr);
extern void* (*real_realloc)(void* ptr, size_t size);
extern void* (*real_calloc)(size_t nmemb, size_t size);

namespace lynx {
namespace base {

template <typename T>
class InternalAllocator {
 public:
  typedef T value_type;
  typedef T* pointer;
  typedef const T* const_pointer;
  typedef T& reference;
  typedef const T& const_reference;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;

  template <typename U>
  struct rebind {
    typedef InternalAllocator<U> other;
  };

  pointer allocate(size_type n, const void* hint = 0) {
    return static_cast<pointer>(real_malloc(sizeof(T) * n));
  }

  void deallocate(pointer p, size_type n) { real_free(p); }

  template <typename U, typename... Args>
  void construct(U* p, Args&&... args) {
    ::new (p) U(std::forward<Args>(args)...);
  }

  template <typename U>
  void destroy(U* p) {
    p->~U();
  }

  pointer address(reference x) { return (pointer)&x; }

  const_pointer address(const_reference x) { return (const_pointer)&x; }

  size_type max_size() const { return size_type(UINTMAX_MAX / sizeof(T)); }
};

struct Record {
  uintptr_t addr;
  size_t size;
  std::vector<uintptr_t, InternalAllocator<uintptr_t>> stack;
};

struct AddrHash {
  size_t operator()(const uintptr_t addr) const {
    return static_cast<size_t>(addr);
  }
};

struct AddrEq {
  bool operator()(const uintptr_t lhs, const uintptr_t rhs) const {
    return lhs == rhs;
  }
};

class DlInfo {
 public:
  static DlInfo& Instance() {
    static std::unique_ptr<DlInfo> instance_;
    static std::once_flag instance_once_flag;
    std::call_once(instance_once_flag, [&]() {
      instance_.reset((DlInfo*)real_malloc(sizeof(DlInfo)));
      ::new (instance_.get()) DlInfo();
    });
    return *instance_;
  }

  Dl_info* GetDlInfo(uintptr_t addr) {
    auto& info = dl_infos_[addr];
    if (!info.dli_fname) {
      dladdr(reinterpret_cast<void*>(addr), &info);
    }
    return &info;
  }

  void ClearCache() { dl_infos_.clear(); }

 private:
  using DlInfoAllocator =
      InternalAllocator<std::pair<const uintptr_t, Dl_info>>;
  std::map<uintptr_t, Dl_info, std::less<int>, DlInfoAllocator> dl_infos_;
};

class RecordBuffer {
 public:
  RecordBuffer() = default;
  virtual ~RecordBuffer() { records_.clear(); };
  inline void AddRecord(Record& record) {
    std::lock_guard<std::mutex> lock(lock_);
    records_[record.addr] = record;
  }

  inline void RemoveRecord(uintptr_t addr) {
    std::lock_guard<std::mutex> lock(lock_);
    auto iter = records_.find(addr);
    if (iter != records_.end()) {
      records_.erase(iter);
    }
  }

  // before this function be called, we need to ensure that
  // all the recording operations are finished or in progress.
  void Clear() {
    std::lock_guard<std::mutex> lock(lock_);
    records_.clear();
  }

  void DumpRecordsToStream(std::ostream& os) {
    std::lock_guard<std::mutex> lock(lock_);
    for (auto& pair : records_) {
      os << "object: addr=" << std::hex << "0x" << pair.first
         << " size=" << std::dec << pair.second.size << std::endl;
      for (uintptr_t frame : pair.second.stack) {
        Dl_info* info = DlInfo::Instance().GetDlInfo(frame);
        os << "0x" << std::hex
           << frame - reinterpret_cast<uintptr_t>(info->dli_fbase) << " "
           << (info->dli_fname ? info->dli_fname : "") << std::endl;
      }
      os << std::endl;
    }
  }

 private:
  std::mutex lock_;
  std::unordered_map<uintptr_t, Record, AddrHash, AddrEq,
                     InternalAllocator<std::pair<const uintptr_t, Record>>>
      records_;
};

class MemoryTracer {
 public:
  static MemoryTracer& Instance();
  static void SetupRealFunctions();
  static void InstallLibcFunctionsHook();
  static void UnInstallLibcFunctionsHook();

  MemoryTracer() : enable_(false), min_watched_size_(0){};
  ~MemoryTracer();
  void StartTracing(int min_watch_size);
  void StopTracing();
  inline void RecordAllocation(void* ptr, size_t size);
  inline void RecordRelease(void* ptr);
  void WriteRecordsToFile(const std::string& file_path);
  void Enable();
  void Disable();

 private:
  using RecordBufferAllocator =
      InternalAllocator<std::pair<const int, RecordBuffer>>;
  using BufferIndexAllocator =
      InternalAllocator<std::pair<const int, const int>>;

  static inline int AddrToBufferIndex(void* addr);
  void InitBuffer();

  static constexpr unsigned int kBufferCount = 8;
  std::map<int, RecordBuffer, std::less<int>, RecordBufferAllocator>
      record_buffers_;
  std::atomic_bool enable_;
  int min_watched_size_;
};

}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_DEBUG_MEMORY_TRACER_H_
