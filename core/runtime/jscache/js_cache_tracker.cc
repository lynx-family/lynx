// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/jscache/js_cache_tracker.h"

#include <utility>
#include <vector>

#include "core/runtime/piper/js/runtime_constant.h"
#include "core/services/event_report/event_tracker_platform_impl.h"

namespace lynx {
namespace piper {
namespace cache {
using tasm::report::EventTrackerPlatformImpl;
using tasm::report::MoveOnlyEvent;
JsCacheTracker::TestInterceptEvent JsCacheTracker::s_test_intercept_event_ =
    nullptr;
namespace {
constexpr char kBytecodeEventName[] = "lynxsdk_code_cache";

void SetCommonParams(MoveOnlyEvent& event, JSRuntimeType runtime_type,
                     const std::string stage) {
  event.SetName(kBytecodeEventName);
  event.SetProps("stage", stage);
  event.SetProps("runtime_type", static_cast<int>(runtime_type));
}

}  // namespace

void JsCacheTracker::FlushEventWithoutInstanceId(
    tasm::report::EventTracker::EventBuilder event_builder) {
  if (s_test_intercept_event_) {
    s_test_intercept_event_(std::move(event_builder));
    return;
  }
  EventTrackerPlatformImpl::GetReportTaskRunner()->PostTask(
      [builder = std::move(event_builder)]() mutable {
        MoveOnlyEvent event;
        builder(event);
        EventTrackerPlatformImpl::OnEvent(tasm::report::kUnknownInstanceId,
                                          std::move(event));
      });
}

void JsCacheTracker::OnPrepareJS(JSRuntimeType runtime_type,
                                 const std::string& source_url,
                                 bool load_success, JsScriptType script_type,
                                 double cost, JsCacheErrorCode error_code) {
  tasm::report::EventTracker::OnEvent(
      [runtime_type, source_url, load_success, script_type, cost,
       error_code](tasm::report::MoveOnlyEvent& event) {
        SetCommonParams(event, runtime_type, "prepare_js");
        event.SetProps("source_url", source_url);
        event.SetProps("script_type", static_cast<int>(script_type));
        event.SetProps("load_success", load_success);
        // ms
        event.SetProps("cost", cost);
        event.SetProps("error_code", static_cast<int>(error_code));
      });
}

void JsCacheTracker::OnGetBytecodeDisable(int64_t runtime_id,
                                          JSRuntimeType runtime_type,
                                          const std::string& source_url,
                                          bool enable_user_bytecode,
                                          bool enable_bytecode) {
  OnGetBytecode(runtime_id, runtime_type, source_url, enable_user_bytecode,
                enable_bytecode, false, JsCacheType::NONE,
                JsCacheErrorCode::NO_ERROR, 0, 0);
}

void JsCacheTracker::OnGetBytecode(
    int64_t runtime_id, JSRuntimeType runtime_type,
    const std::string& source_url, bool enable_user_bytecode,
    bool enable_bytecode, bool success, JsCacheType cache_type,
    JsCacheErrorCode error_code, double cost, double code_size) {
  if (runtime::IsAppServiceJS(source_url)) {
    tasm::report::EventTracker::UpdateGenericInfo(
        static_cast<int32_t>(runtime_id), "code_cache_hit",
        static_cast<int64_t>(success));
  }

  tasm::report::EventTracker::OnEvent(
      [runtime_type, source_url, enable_user_bytecode, enable_bytecode,
       cache_hit = success, cache_type, error_code, cost,
       code_size](tasm::report::MoveOnlyEvent& event) {
        SetCommonParams(event, runtime_type, "get_code_cache");
        event.SetProps("source_url", source_url);
        event.SetProps("enable_user_bytecode", enable_user_bytecode);
        event.SetProps("enable_bytecode", enable_bytecode);
        event.SetProps("cache_hit", cache_hit);
        event.SetProps("cache_type", static_cast<int>(cache_type));
        event.SetProps("error_code", static_cast<int>(error_code));
        // ms
        event.SetProps("cost", cost);
        // save by kb
        event.SetProps("code_size", code_size / 1024.0);
      });
}

void JsCacheTracker::OnGenerateBytecodeFailed(JSRuntimeType runtime_type,
                                              std::string url,
                                              std::string template_url,
                                              const std::string engine_version,
                                              JsCacheErrorCode error_code) {
  OnGenerateBytecode(runtime_type, url, template_url, false, 0, 0, false,
                     engine_version, 0, error_code);
}

void JsCacheTracker::OnGenerateBytecode(
    JSRuntimeType runtime_type, std::string url, std::string template_url,
    bool generate_success, double raw_size, double bytecode_size,
    bool persist_success, const std::string engine_version,
    double generate_cost, JsCacheErrorCode error_code) {
  auto builder = [runtime_type, url = std::move(url),
                  template_url = std::move(template_url), generate_success,
                  raw_size, bytecode_size, persist_success, engine_version,
                  error_code,
                  generate_cost](tasm::report::MoveOnlyEvent& event) {
    SetCommonParams(event, runtime_type, "generate_code_cache");
    event.SetProps("source_url", url);
    event.SetProps("template_url", template_url);
    event.SetProps("generate_success", generate_success);
    event.SetProps("raw_size", raw_size / 1024.0);
    event.SetProps("code_cache_size", bytecode_size / 1024.0);
    event.SetProps("persist_success", persist_success);
    event.SetProps("engine_version", engine_version);
    event.SetProps("generate_cost", generate_cost);
    event.SetProps("error_code", static_cast<int>(error_code));
  };
  FlushEventWithoutInstanceId(std::move(builder));
}

void JsCacheTracker::OnCleanUp(JSRuntimeType runtime_type, int file_count,
                               size_t current_total_size, size_t clean_size,
                               double cost, JsCacheErrorCode error_code) {
  auto builder = [runtime_type, file_count, current_total_size, clean_size,
                  cost, error_code](tasm::report::MoveOnlyEvent& event) {
    SetCommonParams(event, runtime_type, "cleanup");
    event.SetProps("disk_file_count", file_count);
    event.SetProps("disk_file_size", current_total_size / 1024.0);
    event.SetProps("clean_size", clean_size / 1024.0);
    event.SetProps("cost", cost);
    event.SetProps("error_code", static_cast<int>(error_code));
  };
  FlushEventWithoutInstanceId(std::move(builder));
}

}  // namespace cache
}  // namespace piper
}  // namespace lynx
