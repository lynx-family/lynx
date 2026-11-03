// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/public/lynx_resource_loader.h"

#include <atomic>
#include <chrono>
#include <optional>
#include <utility>

#include "base/include/fml/message_loop.h"
#include "base/include/value/byte_array.h"
#include "base/include/value/table.h"
#include "core/shell/host_script/runtime/interceptor.h"

#if ENABLE_TESTBENCH_REPLAY
#include "core/services/replay/replay_resource_cache.h"
#endif  // ENABLE_TESTBENCH_REPLAY

namespace lynx {
namespace pub {
namespace {
using lepus::Value;
using shell::InterceptKind;
using shell::Interceptor;
std::atomic<uint64_t> next_request_id{0};

Value RequestSnapshot(const LynxResourceRequest& request) {
  Value event(lepus::Dictionary::Create());
  event.SetProperty("requestId", Value(std::to_string(++next_request_id)));
  event.SetProperty("viewId", Value(std::to_string(request.view_id)));
  event.SetProperty("url", Value(request.url));
  event.SetProperty("type", Value(static_cast<int32_t>(request.type)));
  event.SetProperty(
      "domain", Value(shell::HostScriptDomainName(request.execution_domain)));
  return event;
}
}  // namespace

void LynxResourceLoader::LoadResource(
    const LynxResourceRequest& request,
    base::MoveOnlyClosure<void, LynxResourceResponse&> callback) {
  std::optional<LynxResourceRequest> rewritten;
  std::shared_ptr<Interceptor> provider;
  if (Interceptor::IsEnabled() &&
      request.execution_domain != shell::HostScriptDomain::kCount &&
      Interceptor::IsViewAlive(request.view_id)) {
    provider = Interceptor::Current(
        {InterceptKind::kResourceFetch, InterceptKind::kResourceComplete},
        request.execution_domain);
  }
  shell::InterceptResult decision;
  Value event;
  if (provider) {
    event = RequestSnapshot(request);
    decision = provider->Dispatch(InterceptKind::kResourceFetch, event);
    if (!decision.failed && decision.patch.IsTable()) {
      rewritten = request;
      rewritten->url = decision.patch.GetProperty("url").StdString();
    }
  }
  const auto& effective_request = rewritten ? *rewritten : request;
  const bool mocked = !decision.failed && decision.mock.IsTable();
  // Completion only observes metadata. Preserve the resource consumer's thread
  // (a synchronous script load may be waiting for this callback on BTS).
  if (provider && provider->HasHandlers(InterceptKind::kResourceComplete)) {
    auto* loop = fml::MessageLoop::IsInitializedForCurrentThread();
    if (!loop) {
      Interceptor::ReportCoverageGap(InterceptKind::kResourceComplete,
                                     "Resource owner has no task runner");
    } else {
      event.SetProperty("originalUrl", Value(request.url));
      event.SetProperty("url", Value(effective_request.url));
      auto started = std::chrono::steady_clock::now();
      callback = [callback = std::move(callback), event = std::move(event),
                  runner = loop->GetTaskRunner(), started, mocked,
                  once = std::make_shared<std::atomic<bool>>(false),
                  view_id = request.view_id,
                  weak = std::weak_ptr<Interceptor>(provider)](
                     LynxResourceResponse& response) mutable {
        if (once->exchange(true)) return;
        event.SetProperty("mocked", Value(mocked));
        event.SetProperty("success", Value(response.Success()));
        event.SetProperty("errorCode", Value(response.err_code));
        event.SetProperty("errorMessage", Value(response.err_msg));
        event.SetProperty("size",
                          Value(static_cast<double>(response.data.size())));
        event.SetProperty("durationMs",
                          Value(std::chrono::duration<double, std::milli>(
                                    std::chrono::steady_clock::now() - started)
                                    .count()));
        callback(response);
        runner->PostTask([weak, event = std::move(event), view_id]() {
          // Lock only on the owner. Script replacement detaches this provider;
          // never deliver an old request to the replacement context.
          auto owner = weak.lock();
          if (owner && owner->IsAttached() && Interceptor::IsViewAlive(view_id))
            owner->Dispatch(InterceptKind::kResourceComplete, event);
        });
      };
    }
  }
  if (mocked) {
    LynxResourceResponse response{};
    const auto& bytes = decision.mock.GetProperty("bytes");
    if (bytes.IsByteArray()) {
      auto array = bytes.ByteArray();
      if (array->GetLength())
        response.data.assign(array->GetPtr(),
                             array->GetPtr() + array->GetLength());
    } else {
      const auto& error = decision.mock.GetProperty("error");
      response.err_code =
          static_cast<int32_t>(error.GetProperty("code").Number());
      response.err_msg = error.GetProperty("message").StdString();
    }
    callback(response);
    return;
  }
#if ENABLE_TESTBENCH_REPLAY
  auto* cache = replay_cache_ ? replay_cache_.get() : nullptr;
  if (cache != nullptr && !cache->IsEmpty()) {
    LynxResourceResponse cached_response;
    if (cache->ResolveResource(effective_request, &cached_response)) {
      callback(cached_response);
      return;
    }
  }
#endif  // ENABLE_TESTBENCH_REPLAY
  LoadResourceInternal(effective_request, std::move(callback));
}

void LynxResourceLoader::LoadResourcePath(
    const LynxResourceRequest& request,
    base::MoveOnlyClosure<void, LynxPathResponse&> path_callback) {
  LoadResourcePathInternal(request, std::move(path_callback));
}

void LynxResourceLoader::SetReplayResourceCache(
    std::shared_ptr<tasm::replay::ReplayResourceCache> cache) {
  replay_cache_ = std::move(cache);
}
}  // namespace pub
}  // namespace lynx
