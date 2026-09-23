// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/android/global_devtool_hsr_android.h"

#include <deque>
#include <memory>
#include <utility>
#include <vector>

#include "base/include/no_destructor.h"
#include "base/include/string/string_utils.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/utils/devtool_lifecycle.h"
#include "core/shell/host_script/android/runtime/process_runtime_android.h"
#include "core/shell/host_script/android/runtime/process_runtime_init_android.h"
#include "devtool/lynx_devtool/agent/android/global_devtool_platform_android.h"

namespace lynx {
namespace devtool {
namespace {
using shell::ProcessRuntime;
using Callback = GlobalDevToolPlatformFacade::HSRScriptCallback;
using Operation = HSRScriptRequest::Operation;

Json::Value Status() {
  Json::Value result(Json::objectValue);
  result["available"] = shell::HasHostScriptRuntime();
  result["enabled"] = tasm::DevToolLifecycle::GetInstance().IsEnabled();
  for (auto domain :
       {ProcessRuntime::Domain::kBTS, ProcessRuntime::Domain::kMTS,
        ProcessRuntime::Domain::kUI}) {
    constexpr const char* names[] = {"bts", "mts", "ui"};
    result["ready"][names[static_cast<size_t>(domain)]] =
        shell::IsHostScriptRuntimeReady(domain);
  }
  result["loaded"] = false;
  result["stopping"] = false;
  result["pending"] = 0;
  return result;
}

ProcessRuntime::Domain RuntimeDomain(HSRScriptRequest::Thread thread) {
  switch (thread) {
    case HSRScriptRequest::Thread::kBTS:
      return ProcessRuntime::Domain::kBTS;
    case HSRScriptRequest::Thread::kMTS:
      return ProcessRuntime::Domain::kMTS;
    case HSRScriptRequest::Thread::kUI:
      return ProcessRuntime::Domain::kUI;
  }
  return ProcessRuntime::Domain::kBTS;
}

void Evaluate(HSRScriptRequest request, std::string url, Callback callback) {
  const auto domain = RuntimeDomain(request.thread);
  constexpr const char* kDomainNames[] = {"bts", "mts", "ui"};
  const bool load = request.operation == Operation::kLoadScript;
  if (url.empty()) {
    url = std::string("host-script://cdp-") +
          kDomainNames[static_cast<size_t>(domain)] + ".js";
  }
  auto completion = std::make_shared<Callback>(std::move(callback));
  shell::EvaluateHostScriptRuntime(
      domain, request.source.empty() ? "void 0;" : std::move(request.source),
      std::move(url),
      [completion, load](ProcessRuntime::Result result) mutable {
        Json::Value response(Json::objectValue);
        if (result.success && !load) {
          response["valueType"] = result.has_value ? "json" : "undefined";
          if (result.has_value &&
              !Json::Reader().parse(result.value_json, response["value"],
                                    false)) {
            result.error = "Cannot decode Host Script result JSON";
          }
        }
        if (*completion) {
          auto callback = std::move(*completion);
          std::move(callback)(std::move(response), result.error);
        }
      });
}

// Only the UI lifecycle runner accesses this queue. It orders CDP commands
// across replacement; each runtime still owns its execution and pending calls.
class AndroidHSR {
 public:
  static AndroidHSR& Get() {
    static base::NoDestructor<AndroidHSR> instance;
    return *instance;
  }

  void Enqueue(HSRScriptRequest request, Callback callback, uint64_t epoch) {
    if (request.operation == Operation::kGetStatus) {
      auto result = Status();
      result["loaded"] =
          loaded_ && shell::HostScriptDebugEpoch() == loaded_epoch_;
      result["stopping"] = stopping_;
      result["pending"] = static_cast<Json::UInt64>(requests_.size());
      std::move(callback)(std::move(result), "");
      return;
    }
    if (request.operation == Operation::kStop) {
      Stop(std::move(callback));
      return;
    }
    requests_.push_back(
        {++next_id_, std::move(request), std::move(callback), {}, epoch});
    if (requests_.size() == 1) Start();
  }

  void Source(int64_t id, std::string source, std::string error) {
    if (!Current(id) || !requests_.front().fetching) return;
    if (RejectIfDisabled()) return;
    requests_.front().fetching = false;
    if (!error.empty()) {
      Complete(id, Json::Value(), error);
      return;
    }
    requests_.front().request.source = std::move(source);
    Replace();
  }

 private:
  void Stop(Callback callback) {
    stopped_ = true;
    loaded_ = false;
    const bool already_stopping = stopping_;
    stopping_ = true;
    stop_callbacks_.push_back(std::move(callback));
    // Invalidate downloads and replies before shutdown. Old IDs cannot complete
    // a request submitted by a later load, even if its URL is identical.
    auto cancelled = std::move(requests_);
    requests_.clear();
    for (auto& pending : cancelled) {
      std::move(pending.callback)(Json::Value(), "HSR_STOPPED");
    }
    if (already_stopping) return;
    shell::ShutdownHostScriptRuntime([](ProcessRuntime::Result result) {
      auto& self = Get();
      self.stopping_ = false;
      auto callbacks = std::move(self.stop_callbacks_);
      self.stop_callbacks_.clear();
      for (auto& callback : callbacks) {
        std::move(callback)(Json::Value(Json::objectValue), result.error);
      }
      self.Start();
    });
  }

  struct Request {
    int64_t id;
    HSRScriptRequest request;
    Callback callback;
    std::string url;
    uint64_t debug_epoch;
    bool started = false;
    bool fetching = false;
  };

  bool Current(int64_t id) const {
    return !requests_.empty() && requests_.front().id == id;
  }

  bool RejectIfDisabled() {
    if (tasm::DevToolLifecycle::GetInstance().IsEnabled() &&
        requests_.front().debug_epoch == shell::HostScriptDebugEpoch())
      return false;
    Complete(requests_.front().id, Json::Value(), "HSR_DEBUG_DISABLED");
    return true;
  }

  void Start() {
    if (stopping_ || requests_.empty() || requests_.front().started) return;
    if (RejectIfDisabled()) return;
    auto& pending = requests_.front();
    pending.started = true;
    if (pending.request.operation == Operation::kEvaluate) {
      if (stopped_) {
        Complete(pending.id, Json::Value(), "HSR_STOPPED");
        return;
      }
      Execute();
    } else if (pending.request.source_type ==
               HSRScriptRequest::SourceType::kUrl) {
      pending.url = pending.request.source;
      pending.fetching = true;
      const auto id = pending.id;
      if (!FetchHSRScriptSource(pending.url, id)) {
        Source(id, "", "HSR resource provider threw a JNI exception");
        return;
      }
      base::UIThread::GetRunner()->PostDelayedTask(
          [id] { Get().Source(id, "", "HSR resource request timed out"); },
          fml::TimeDelta::FromSeconds(30));
    } else {
      Replace();
    }
  }

  void Replace() {
    if (RejectIfDisabled()) return;
    auto& pending = requests_.front();
    const auto id = pending.id;
    // Ignore the entry script's last value, including asynchronous work it
    // starts. A successful load acknowledges top-level execution completion.
    pending.request.source += "\n;void 0;";
    const auto& source = pending.request.source;
    if (source.size() > 512 * 1024 ||
        !base::IsValidUtf8(reinterpret_cast<const uint8_t*>(source.data()),
                           source.size())) {
      Complete(id, Json::Value(), "Invalid HSR source bytes or size");
      return;
    }
    // Resolve and validate resource bytes before invalidating the old script.
    loaded_ = false;
    shell::ShutdownHostScriptRuntime([id](ProcessRuntime::Result result) {
      auto& self = Get();
      if (!self.Current(id)) return;
      if (self.RejectIfDisabled()) return;
      if (!result.success || !shell::PrepareHostScriptRuntime()) {
        self.Complete(id, Json::Value(),
                      result.success ? "HSR runtime initialization was rejected"
                                     : result.error);
        return;
      }
      self.Execute();
    });
  }

  void Execute() {
    if (RejectIfDisabled()) return;
    auto& pending = requests_.front();
    Evaluate(std::move(pending.request), pending.url,
             [id = pending.id](Json::Value result, const std::string& error) {
               base::UIThread::GetRunner()->PostTask(
                   [id, result = std::move(result), error]() mutable {
                     Get().Complete(id, std::move(result), error);
                   });
             });
  }

  void Complete(int64_t id, Json::Value result, const std::string& error) {
    if (!Current(id)) return;
    if (requests_.front().request.operation == Operation::kLoadScript &&
        error.empty()) {
      loaded_ = true;
      stopped_ = false;
      loaded_epoch_ = requests_.front().debug_epoch;
    }
    auto callback = std::move(requests_.front().callback);
    requests_.pop_front();
    std::move(callback)(std::move(result), error);
    base::UIThread::GetRunner()->PostTask([] { Get().Start(); });
  }

  int64_t next_id_ = 0;
  uint64_t loaded_epoch_ = 0;
  bool loaded_ = false;
  bool stopped_ = false;
  bool stopping_ = false;
  std::vector<Callback> stop_callbacks_;
  std::deque<Request> requests_;
};
}  // namespace

void GlobalDevToolPlatformAndroid::HandleHSRScript(HSRScriptRequest request,
                                                   HSRScriptCallback callback) {
  if (!callback) return;
  if (!shell::HasHostScriptRuntime()) {
    const bool status = request.operation == Operation::kGetStatus;
    std::move(callback)(status ? Status() : Json::Value(),
                        status ? "" : "HSR_DEBUG_LIBRARY_REQUIRED");
    return;
  }
  const auto epoch = shell::HostScriptDebugEpoch();
  if (!tasm::DevToolLifecycle::GetInstance().IsEnabled() &&
      request.operation != Operation::kGetStatus &&
      request.operation != Operation::kStop) {
    std::move(callback)(Json::Value(), "HSR_DEBUG_DISABLED");
    return;
  }
  auto ui = base::UIThread::GetRunner();
  if (!ui) {
    std::move(callback)(Json::Value(), "Cannot find Android UI task runner");
    return;
  }
  ui->PostTask([request = std::move(request), callback = std::move(callback),
                epoch]() mutable {
    AndroidHSR::Get().Enqueue(std::move(request), std::move(callback), epoch);
  });
}

void CompleteHSRScriptSource(int64_t request_id, std::string source,
                             std::string error) {
  auto ui = base::UIThread::GetRunner();
  if (!ui) return;
  ui->PostTask([request_id, source = std::move(source),
                error = std::move(error)]() mutable {
    AndroidHSR::Get().Source(request_id, std::move(source), std::move(error));
  });
}
}  // namespace devtool
}  // namespace lynx
