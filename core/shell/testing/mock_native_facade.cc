// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/testing/mock_native_facade.h"

#include "core/shell/testing/mock_runner_manufactor.h"

namespace lynx {
namespace shell {

namespace {

bool IsOnUIThread() { return MockRunnerManufactor::IsOnUIThread(); }

}  // namespace

MockNativeFacade::~MockNativeFacade() {
  if (arwe != nullptr) {
    arwe->Signal();
  }
}

void MockNativeFacade::OnLogContextUpdated(const base::LogContext& context) {
  std::lock_guard<std::mutex> lock(log_context_updates_mutex_);
  log_context_updates_.push_back(context);
}

std::vector<base::LogContext> MockNativeFacade::GetLogContextUpdates() const {
  std::lock_guard<std::mutex> lock(log_context_updates_mutex_);
  return log_context_updates_;
}

void MockNativeFacade::OnDataUpdated() {
  result.on_correct_thread = IsOnUIThread();
  arwe->Signal();
}

void MockNativeFacade::OnTemplateLoaded(const std::string& url) {
  result.on_correct_thread = IsOnUIThread();
  result["url"] = url;
  arwe->Signal();
}

void MockNativeFacade::OnSSRHydrateFinished(const std::string& url) {
  result.on_correct_thread = IsOnUIThread();
  result["url"] = url;
  arwe->Signal();
}

void MockNativeFacade::OnRuntimeReady() {
  result.on_correct_thread = IsOnUIThread();
  arwe->Signal();
}

void MockNativeFacade::OnTasmFinishByNative() {
  result.on_correct_thread = IsOnUIThread();
  arwe->Signal();
}

void MockNativeFacade::SetEmbeddedTiming(const std::string& timing_key,
                                         uint64_t timestamp_us,
                                         const std::string& pipeline_id) {
  result.on_correct_thread = IsOnUIThread();
  result["timing_key"] = timing_key;
  result["timestamp_us"] = timestamp_us;
  result["pipeline_id"] = pipeline_id;
  arwe->Signal();
}

void MockNativeFacade::ReportError(const base::LynxError& error) {
  result.on_correct_thread = IsOnUIThread();
  result["error_code"] = error.error_code_;
  result["msg"] = error.error_message_;
  arwe->Signal();
}

void MockNativeFacade::OnModuleMethodInvoked(const std::string& module,
                                             const std::string& method,
                                             int32_t code) {
  result.on_correct_thread = IsOnUIThread();
  result["module"] = module;
  result["method"] = method;
  result["code"] = code;
  arwe->Signal();
}

void MockNativeFacade::OnConfigUpdated(const lepus::Value& data) {
  result.on_correct_thread = IsOnUIThread();
  result["data"] = data;
  arwe->Signal();
}

void MockNativeFacade::OnUpdateDataWithoutChange() {
  result.on_correct_thread = IsOnUIThread();
  arwe->Signal();
}

}  // namespace shell
}  // namespace lynx
