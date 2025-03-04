// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSCACHE_JS_CACHE_TRACKER_H_
#define CORE_RUNTIME_JSCACHE_JS_CACHE_TRACKER_H_

#include <string>

#include "core/runtime/jsi/jsi.h"
#include "core/services/event_report/event_tracker.h"

namespace lynx {
namespace piper {
namespace cache {

enum class JsCacheErrorCode {
  NO_ERROR,
  TARGET_SDK_MISMATCH = 1,
  BINARY_FORMAT_ERROR = 2,
  FILE_READ_ERROR = 3,
  META_FILE_WRITE_ERROR = 4,
  CACHE_FILE_WRITE_ERROR = 5,
  RUNTIME_GENERATE_FAILED = 6,
  META_FILE_READ_ERROR = 7,
  CACHE_FILE_READ_ERROR = 8,
};

enum class JsCacheType {
  NONE,
  MEMORY,
  FILE,
};

enum class JsScriptType {
  SOURCE,
  BINARY,
  LOCAL_BINARY,
};

class JsCacheTracker final {
 public:
  JsCacheTracker() = delete;
  ~JsCacheTracker() = delete;
  static void OnPrepareJS(JSRuntimeType runtime_type,
                          const std::string& source_url, bool load_success,
                          JsScriptType script_type, double cost,
                          JsCacheErrorCode error_code);

  static void OnGetBytecodeDisable(int64_t runtime_id,
                                   JSRuntimeType runtime_type,
                                   const std::string& source_url,
                                   bool enable_user_bytecode,
                                   bool enable_bytecode);
  static void OnGetBytecode(int64_t runtime_id, JSRuntimeType runtime_type,
                            const std::string& source_url,
                            bool enable_user_bytecode, bool enable_bytecode,
                            bool success, JsCacheType type,
                            JsCacheErrorCode error_code, double cost,
                            double code_size);

  static void OnGenerateBytecodeFailed(JSRuntimeType runtime_type,
                                       std::string url,
                                       std::string template_url,
                                       const std::string vm_sdk_version,
                                       JsCacheErrorCode error_code);
  static void OnGenerateBytecode(JSRuntimeType runtime_type, std::string url,
                                 std::string template_url,
                                 bool generate_success, double raw_size,
                                 double bytecode_size, bool persist_success,
                                 const std::string vm_sdk_version,
                                 double generate_cost,
                                 JsCacheErrorCode error_code);

  static void OnCleanUp(JSRuntimeType runtime_type, int file_count,
                        size_t current_total_size, size_t clean_size,
                        double cost, JsCacheErrorCode error_code);

  // TODO(limeng): delete this.
  static void FlushEventWithoutInstanceId(
      tasm::report::EventTracker::EventBuilder event_builder);

  // Only used for test
  using TestInterceptEvent =
      void (*)(tasm::report::EventTracker::EventBuilder event_builder);
  static TestInterceptEvent s_test_intercept_event_;
};

}  // namespace cache
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JSCACHE_JS_CACHE_TRACKER_H_
