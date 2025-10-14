// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FML_NATIVE_LIBRARY_H_
#define CLAY_FML_NATIVE_LIBRARY_H_

#include <optional>

#include "base/include/fml/macros.h"
#include "base/include/fml/memory/ref_counted.h"
#include "base/include/fml/memory/ref_ptr.h"
#include "build/build_config.h"
#include "clay/fml/logging.h"

#if defined(OS_WIN)
#include <windows.h>
#endif  // defined(OS_WIN)

namespace fml {
class NativeLibrary : public fml::RefCountedThreadSafe<NativeLibrary> {
 public:
#if OS_WIN
  using Handle = HMODULE;
  using SymbolHandle = FARPROC;
#else   // OS_WIN
  using Handle = void*;
  using SymbolHandle = void*;
#endif  // OS_WIN

  static fml::RefPtr<NativeLibrary> Create(const char* path);

  static fml::RefPtr<NativeLibrary> Create(const char* path, bool faker);

  static fml::RefPtr<NativeLibrary> CreateWithHandle(
      Handle handle, bool close_handle_when_done);

  static fml::RefPtr<NativeLibrary> CreateForCurrentProcess();

  template <typename T>
  const std::optional<T> ResolveFunction(const char* symbol) {
    auto* resolved_symbol = Resolve(symbol);
    if (!resolved_symbol) {
      return std::nullopt;
    }
    return std::optional<T>(reinterpret_cast<T>(resolved_symbol));
  }

  const uint8_t* ResolveSymbol(const char* symbol) {
    auto* resolved_symbol = reinterpret_cast<const uint8_t*>(Resolve(symbol));
    if (resolved_symbol == nullptr) {
      FML_DLOG(INFO) << "Could not resolve symbol in library: " << symbol;
    }
    return resolved_symbol;
  }

 private:
  Handle handle_ = nullptr;
  bool close_handle_ = true;
  bool faker_ = false;

  explicit NativeLibrary(const char* path);
  NativeLibrary(const char* path, bool faker);

  NativeLibrary(Handle handle, bool close_handle);

  ~NativeLibrary();

  Handle GetHandle() const;
  SymbolHandle Resolve(const char* symbol) const;

  BASE_DISALLOW_COPY_AND_ASSIGN(NativeLibrary);
  FML_FRIEND_REF_COUNTED_THREAD_SAFE(NativeLibrary);
  FML_FRIEND_MAKE_REF_COUNTED(NativeLibrary);
};

}  // namespace fml

#endif  // CLAY_FML_NATIVE_LIBRARY_H_
