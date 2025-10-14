// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <dlfcn.h>
#include <fcntl.h>

#include <climits>

#include "build/build_config.h"
#include "clay/fml/native_library.h"

#if OS_IOS || OS_ANDROID
#error This file is not for iOS & Android
#endif

namespace fml {

NativeLibrary::NativeLibrary(const char* path) : NativeLibrary(path, false) {}

NativeLibrary::NativeLibrary(const char* path, bool faker) {
  ::dlerror();

  if (handle_ == nullptr) {
    handle_ = ::dlopen(path, RTLD_NOW);
  }

  if (handle_ == nullptr) {
    FML_DLOG(ERROR) << "Could not open library '" << path << "' due to error '"
                    << ::dlerror() << "'.";
  }

  faker_ = faker;
}

NativeLibrary::NativeLibrary(Handle handle, bool close_handle)
    : handle_(handle), close_handle_(close_handle) {}

NativeLibrary::~NativeLibrary() {
  if (handle_ == nullptr) {
    return;
  }

  if (close_handle_) {
    ::dlerror();

    int close_status = INT_MAX;

    if (close_status == INT_MAX) {
      close_status = ::dlclose(handle_);
    }

    if (close_status != 0) {
      handle_ = nullptr;
      FML_LOG(ERROR) << "Could not close library due to error '" << ::dlerror()
                     << "'.";
    }
  }
}

NativeLibrary::Handle NativeLibrary::GetHandle() const { return handle_; }

fml::RefPtr<NativeLibrary> NativeLibrary::Create(const char* path) {
  return NativeLibrary::Create(path, false);
}

fml::RefPtr<NativeLibrary> NativeLibrary::Create(const char* path, bool faker) {
  auto library = fml::AdoptRef(new NativeLibrary(path, faker));
  return library->GetHandle() != nullptr ? library : nullptr;
}

fml::RefPtr<NativeLibrary> NativeLibrary::CreateWithHandle(
    Handle handle, bool close_handle_when_done) {
  auto library =
      fml::AdoptRef(new NativeLibrary(handle, close_handle_when_done));
  return library->GetHandle() != nullptr ? library : nullptr;
}

fml::RefPtr<NativeLibrary> NativeLibrary::CreateForCurrentProcess() {
  return fml::AdoptRef(new NativeLibrary(RTLD_DEFAULT, false));
}

NativeLibrary::SymbolHandle NativeLibrary::Resolve(const char* symbol) const {
  return ::dlsym(handle_, symbol);
}

}  // namespace fml
