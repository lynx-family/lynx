// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_TRACE_NATIVE_TRACE_CONTROLLER_H_
#define BASE_TRACE_NATIVE_TRACE_CONTROLLER_H_

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/trace/native/trace_export.h"

namespace lynx {
namespace trace {

enum class RuntimeProfilerType { v8 = 0, quickjs };

struct TRACE_EXPORT TraceConfig {
  const uint32_t kDefaultBufferSize = 40960;  // kb
  enum RecordMode {
    RECORD_AS_MUCH_AS_POSSIBLE,
    RECORD_UNTIL_FULL,
    RECORD_CONTINUOUSLY,
    ECHO_TO_CONSOLE
  } record_mode;
  bool enable_systrace;
  uint32_t buffer_size;
  bool is_startup_tracing;
  std::vector<std::string> included_categories;
  std::vector<std::string> excluded_categories;
  std::string file_path;
  int32_t js_profile_interval;
  RuntimeProfilerType js_profile_type;
  TraceConfig()
      : record_mode(RECORD_AS_MUCH_AS_POSSIBLE),
        enable_systrace(false),
        buffer_size(kDefaultBufferSize),
        is_startup_tracing(false),
        js_profile_interval(-1),
        js_profile_type(RuntimeProfilerType::quickjs) {}
};

// DispatchBegin()/DispatchEnd() of TracePlugin that injected into
// TraceController is called when start/stop lynx trace.
class TRACE_EXPORT TracePlugin
    : public std::enable_shared_from_this<TracePlugin> {
 public:
  TracePlugin() = default;
  virtual ~TracePlugin() = default;
  virtual void DispatchBegin() = 0;
  virtual void DispatchEnd() = 0;
  virtual void DispatchSetup(const std::shared_ptr<TraceConfig>& config) {}
  virtual std::string Name() = 0;
};

class TRACE_EXPORT TraceController {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;
    virtual std::string GenerateTracingFileDir() = 0;
#ifdef OS_ANDROID
    virtual void RefreshATraceTags() = 0;
#endif  // BASE_TRACE_NATIVE_TRACE_CONTROLLER_H_
  };

  TraceController() = default;
  virtual ~TraceController() = default;

  TraceController(const TraceController&) = delete;
  TraceController& operator=(const TraceController&) = delete;
  TraceController(TraceController&&) = delete;
  TraceController& operator=(TraceController&&) = delete;

  static TraceController* Instance();

  void SetDelegate(std::unique_ptr<Delegate> delegate) {
    delegate_ = std::move(delegate);
  }

  virtual int StartTracing(const std::shared_ptr<TraceConfig>& config) {
    return -1;
  };
  virtual bool StopTracing(int session_id) { return false; }

  // trace plugin
  virtual void AddTracePlugin(TracePlugin* plugin) {}
  virtual bool DeleteTracePlugin(const std::string& plugin_name) {
    return false;
  }

  // register callback
  virtual void AddCompleteCallback(int session_id,
                                   const std::function<void()> callback) {}
  virtual void RemoveCompleteCallbacks(int session_id) {}

  // startup functions
  virtual void StartStartupTracingIfNeeded() {}
  virtual void SetStartupTracingConfig(std::string config) {}
  virtual std::string GetStartupTracingConfig() { return ""; }
  virtual std::string GetStartupTracingFilePath() { return ""; }
  virtual bool IsTracingStarted() { return false; }

 protected:
  std::unique_ptr<Delegate> delegate_ = nullptr;
};

[[maybe_unused]] TRACE_EXPORT TraceController* GetTraceControllerInstance();

}  // namespace trace
}  // namespace lynx

#endif  // BASE_TRACE_NATIVE_TRACE_CONTROLLER_H_
