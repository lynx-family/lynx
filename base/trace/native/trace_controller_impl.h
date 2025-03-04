// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_TRACE_NATIVE_TRACE_CONTROLLER_IMPL_H_
#define BASE_TRACE_NATIVE_TRACE_CONTROLLER_IMPL_H_

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "base/include/fml/thread.h"
#include "base/trace/native/hook_systrace/hook_system_trace.h"
#include "base/trace/native/trace_controller.h"
#include "third_party/perfetto/perfetto.h"

namespace lynx {
namespace trace {

class TraceControllerImpl : public TraceController {
 public:
  struct TracingSession {
    TracingSession() : id(-1), started(false), all_read(false){};
    ~TracingSession() {
      for (int fd : opened_fds) {
        if (fd > 0) {
          close(fd);
        }
      }
    }
    std::shared_ptr<TraceConfig> config;
    int id;
    std::unique_ptr<::perfetto::TracingSession> session_impl;
    std::vector<int> opened_fds;
    std::vector<std::function<void()>> complete_callbacks;
    bool started;
    std::vector<std::function<void(const std::vector<char>&)>> event_callbacks;
    std::vector<char> raw_traces;
    std::vector<char> unsent_traces;
    bool all_read;
    std::mutex read_mutex;
    std::condition_variable read_cv;
    std::chrono::high_resolution_clock::time_point read_trace_begin;
    std::chrono::high_resolution_clock::time_point read_trace_end;
  };

  TraceControllerImpl();
  ~TraceControllerImpl() override = default;

  int StartTracing(const std::shared_ptr<TraceConfig>& config) override;
  bool StopTracing(int session_id) override;

  // trace plugin
  void AddTracePlugin(TracePlugin* plugin) override;
  bool DeleteTracePlugin(const std::string& plugin_name) override;

  // register callback
  void AddCompleteCallback(int session_id,
                           const std::function<void()> callback) override;
  void RemoveCompleteCallbacks(int session_id) override;

  // startup functions
  void StartStartupTracingIfNeeded() override;
  void SetStartupTracingConfig(std::string config) override;
  std::string GetStartupTracingConfig() override;
  std::string GetStartupTracingFilePath() override;
  bool IsTracingStarted() override;

 private:
  TracingSession& CreateNewSession(const std::shared_ptr<TraceConfig> config);

  std::string GenerateTraceFilePath(const std::string& file_dir);

  std::map<int, std::unique_ptr<TracingSession>> tracing_sessions_;
  std::mutex mutex_;
  std::map<std::string, TracePlugin*> trace_plugins_;
  std::unique_ptr<HookSystemTrace> hook_systrace_;

  const std::string kStartupTracingFile = "/trace-config.json";
  std::string trace_file_dir_;
  std::string startup_tracing_file_;
  bool is_tracing_started_ = false;
};

}  // namespace trace
}  // namespace lynx

#endif  // BASE_TRACE_NATIVE_TRACE_CONTROLLER_IMPL_H_
