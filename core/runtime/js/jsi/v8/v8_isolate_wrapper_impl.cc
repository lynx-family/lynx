// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/js/jsi/v8/v8_isolate_wrapper_impl.h"

#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include "base/include/log/logging.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/js/jsi/v8/v8_helper.h"
#include "libplatform/libplatform.h"
#if defined(OS_WIN)
#include "base/include/string/string_conversion_win.h"
#include "core/base/utils/paths_win.h"
#elif defined(OS_OSX)
#include "core/base/utils/paths_mac.h"
#endif

namespace lynx {
namespace runtime {
namespace js {
namespace {

// Capture the current JS call stack from a V8 interrupt. The interrupt runs on
// the JS thread even while it is stuck, which is why capturing works during a
// dead loop. Currently only the V8 engine supports this.
void CaptureStackInterrupt(v8::Isolate* isolate, void* data) {
  std::unique_ptr<base::MoveOnlyClosure<void, std::string>> callback(
      static_cast<base::MoveOnlyClosure<void, std::string>*>(data));
  v8::HandleScope scope(isolate);
  auto trace =
      v8::StackTrace::CurrentStackTrace(isolate, 64, v8::StackTrace::kDetailed);
  std::string stack;
  for (int i = 0; i < trace->GetFrameCount(); ++i) {
    auto frame = trace->GetFrame(isolate, i);
    stack.append("at ").append(detail::V8Helper::JSStringToSTLString(
        frame->GetFunctionName(), isolate));
    stack.append(" (").append(detail::V8Helper::JSStringToSTLString(
        frame->GetScriptNameOrSourceURL(), isolate));
    stack.append(":").append(std::to_string(frame->GetLineNumber()));
    stack.append(":").append(std::to_string(frame->GetColumn())).append(")\n");
  }
  (*callback)(std::move(stack));
}

}  // namespace

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
    auto [_, path] = lynx::base::GetModuleDirectoryPath();
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

bool V8IsolateInstanceImpl::CaptureJavaScriptStack(
    base::MoveOnlyClosure<void, std::string> callback) {
  if (isolate_ == nullptr) return false;
  isolate_->RequestInterrupt(
      CaptureStackInterrupt,
      new base::MoveOnlyClosure<void, std::string>(std::move(callback)));
  return true;
}

bool V8IsolateInstanceImpl::TerminateJavaScriptExecution() {
  if (isolate_ == nullptr) return false;
  isolate_->TerminateExecution();
  return true;
}

}  // namespace js

}  // namespace runtime
}  // namespace lynx
