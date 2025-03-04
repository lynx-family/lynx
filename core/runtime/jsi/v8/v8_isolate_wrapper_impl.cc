// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/jsi/v8/v8_isolate_wrapper_impl.h"

#include <memory>
#include <mutex>
#include <string>

#include "base/include/log/logging.h"
#include "core/renderer/utils/lynx_env.h"
#include "libplatform/libplatform.h"
#if defined(OS_WIN)
#include "base/include/string/string_conversion_win.h"
#include "core/base/utils/paths_win.h"
#elif defined(OS_OSX)
#include "core/base/utils/paths_mac.h"
#endif

namespace lynx {
namespace piper {

V8IsolateInstanceImpl::V8IsolateInstanceImpl() = default;

V8IsolateInstanceImpl::~V8IsolateInstanceImpl() {
  if (isolate_ != nullptr) {
    isolate_->Dispose();
    LOGI("lynx ~V8IsolateInstance");
  }
}

std::once_flag flag;
void V8IsolateInstanceImpl::InitIsolate(const char* arg, bool useSnapshot) {
  LOGI("lynx V8IsolateInstanceImpl::InitIsolate");
  std::call_once(flag, []() {
    v8::V8::InitializeICU();
#if defined(OS_WIN)
    auto [_, path] = lynx::base::GetExecutableDirectoryPath();
    std::string path_ansi = lynx::base::Utf8ToANSIOrOEM(path);
    v8::V8::InitializeExternalStartupData((path_ansi + "\\").c_str());
#elif defined(OS_OSX)
    auto [_, path] = lynx::common::GetResourceDirectoryPath();
    v8::V8::InitializeExternalStartupData((path + "\\").c_str());
#endif

    v8::V8::InitializePlatform(v8::platform::NewDefaultPlatform().release());
    v8::V8::Initialize();
  });
  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator =
      v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  isolate_ = v8::Isolate::New(create_params);
}

v8::Isolate* V8IsolateInstanceImpl::Isolate() const { return isolate_; }

}  // namespace piper
}  // namespace lynx
