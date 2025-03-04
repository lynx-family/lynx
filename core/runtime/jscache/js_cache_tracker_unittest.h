// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSCACHE_JS_CACHE_TRACKER_UNITTEST_H_
#define CORE_RUNTIME_JSCACHE_JS_CACHE_TRACKER_UNITTEST_H_

#include <string>

#include "core/runtime/jscache/js_cache_tracker.h"
#include "core/runtime/jsi/jsi.h"
#include "core/services/event_report/event_tracker.h"

namespace lynx {
namespace tasm {
namespace report {
namespace test {
void GetEventParams(MoveOnlyEvent &event, int event_depth);
}
}  // namespace report
}  // namespace tasm
namespace piper {
namespace cache {
namespace testing {
using lynx::tasm::report::MoveOnlyEvent;

void CheckCommonEventTrackerParams(MoveOnlyEvent &event, JSRuntimeType type,
                                   const std::string &stage);

void CheckOnGetBytecodeEvent(JSRuntimeType type, const std::string &source_url,
                             JsCacheType cache_type, bool cache_hit,
                             bool enable_user_bytecode, bool enable_bytecode,
                             double cost, double code_size);

void CheckBytecodeGenerateEvent(JSRuntimeType runtime_type, std::string url,
                                std::string template_url, bool generate_success,
                                double raw_size, double bytecode_size,
                                bool persist_success,
                                JsCacheErrorCode error_code,
                                MoveOnlyEvent event);

void CheckCleanUpEvent(JSRuntimeType runtime_type, JsCacheErrorCode error_code,
                       MoveOnlyEvent event);

void CheckCommonEventTrackerParams(MoveOnlyEvent &event);

void GetEventParams(MoveOnlyEvent &event, int event_depth);

void CheckPrepareJSEvent(const std::string &source_url, bool load_success,
                         cache::JsScriptType script_type, double cost,
                         cache::JsCacheErrorCode error_code, int event_depth);

}  // namespace testing
}  // namespace cache
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JSCACHE_JS_CACHE_TRACKER_UNITTEST_H_
