// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/runtime/platform/android/host_script_resource_interceptor.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <utility>

#include "base/include/fml/message_loop.h"
#include "base/include/value/byte_array.h"
#include "base/include/value/table.h"
#include "core/public/lynx_resource_loader.h"
#include "core/shell/host_script/runtime/interceptor.h"

namespace lynx::shell {
namespace {

using lepus::Value;
BASE_STATIC_STRING_DECL(kMocked, "mocked");
BASE_STATIC_STRING_DECL(kSuccess, "success");
BASE_STATIC_STRING_DECL(kErrorCode, "errorCode");
BASE_STATIC_STRING_DECL(kErrorMessage, "errorMessage");
BASE_STATIC_STRING_DECL(kSize, "size");
BASE_STATIC_STRING_DECL(kDurationMs, "durationMs");
BASE_STATIC_STRING_DECL(kRequestId, "requestId");
BASE_STATIC_STRING_DECL(kViewId, "viewId");
BASE_STATIC_STRING_DECL(kUrl, "url");
BASE_STATIC_STRING_DECL(kType, "type");
BASE_STATIC_STRING_DECL(kDomain, "domain");
BASE_STATIC_STRING_DECL(kOriginalUrl, "originalUrl");
BASE_STATIC_STRING_DECL(kBytes, "bytes");
BASE_STATIC_STRING_DECL(kError, "error");
BASE_STATIC_STRING_DECL(kCode, "code");
BASE_STATIC_STRING_DECL(kMessage, "message");

std::atomic<uint64_t> next_request_id{0};

void ObserveCompletion(pub::ResourceLoadCallback& callback, Value event,
                       const std::shared_ptr<Interceptor>& provider,
                       base::LynxEntityId view, bool mocked) {
  auto* loop = fml::MessageLoop::IsInitializedForCurrentThread();
  if (!loop) {
    Interceptor::ReportCoverageGap(InterceptKind::kResourceComplete,
                                   "Resource owner has no task runner");
    return;
  }
  auto started = std::chrono::steady_clock::now();
  callback = [callback = std::move(callback), event = std::move(event),
              runner = loop->GetTaskRunner(), started, mocked, view,
              once = std::make_shared<std::atomic<bool>>(false),
              weak = std::weak_ptr<Interceptor>(provider)](
                 pub::LynxResourceResponse& response) mutable {
    if (once->exchange(true)) return;
    event.SetProperty(kMocked, Value(mocked));
    event.SetProperty(kSuccess, Value(response.Success()));
    event.SetProperty(kErrorCode, Value(response.err_code));
    event.SetProperty(kErrorMessage, Value(response.err_msg));
    event.SetProperty(kSize, Value(static_cast<double>(response.data.size())));
    event.SetProperty(kDurationMs,
                      Value(std::chrono::duration<double, std::milli>(
                                std::chrono::steady_clock::now() - started)
                                .count()));
    // Preserve the original delivery thread and avoid making the consumer wait
    // for an observer on the resource's owning domain.
    callback(response);
    runner->PostTask([weak, event = std::move(event), view]() {
      auto owner = weak.lock();
      if (owner && owner->IsAttached() && Interceptor::IsViewAlive(view))
        owner->Dispatch(InterceptKind::kResourceComplete, event);
    });
  };
}

}  // namespace

bool InterceptResourceLoad(pub::LynxResourceLoader& loader,
                           const pub::LynxResourceRequest& request,
                           base::LynxEntityId view, HostScriptDomain domain,
                           pub::ResourceLoadCallback& callback) {
  if (!Interceptor::IsEnabled() || view < 0) return false;
  auto provider = Interceptor::Current(
      {InterceptKind::kResourceFetch, InterceptKind::kResourceComplete},
      domain);
  if (!provider || !Interceptor::IsViewAlive(view)) return false;
  Value event(lepus::Dictionary::Create());
  event.SetProperty(kRequestId, Value(std::to_string(++next_request_id)));
  event.SetProperty(kViewId, Value(std::to_string(view)));
  event.SetProperty(kUrl, Value(request.url));
  event.SetProperty(kType, Value(static_cast<int32_t>(request.type)));
  event.SetProperty(kDomain, Value(HostScriptDomainName(domain)));
  auto decision = provider->Dispatch(InterceptKind::kResourceFetch, event);
  const auto& url = decision.patch.GetProperty(kUrl);
  const bool rewrite = !decision.failed && url.IsString();
  const bool mocked = !decision.failed && decision.mock.IsTable();
  if (provider->HasHandlers(InterceptKind::kResourceComplete)) {
    event.SetProperty(kOriginalUrl, Value(request.url));
    if (rewrite) event.SetProperty(kUrl, url);
    ObserveCompletion(callback, std::move(event), provider, view, mocked);
  }
  if (mocked) {
    pub::LynxResourceResponse response{};
    const auto& bytes = decision.mock.GetProperty(kBytes);
    if (bytes.IsByteArray()) {
      auto array = bytes.ByteArray();
      if (array->GetLength())
        response.data.assign(array->GetPtr(),
                             array->GetPtr() + array->GetLength());
    } else {
      const auto& error = decision.mock.GetProperty(kError);
      response.err_code =
          static_cast<int32_t>(error.GetProperty(kCode).Number());
      response.err_msg = error.GetProperty(kMessage).StdString();
    }
    callback(response);
    return true;
  }
  if (!rewrite) return false;
  auto effective_request = request;
  effective_request.url = url.StdString();
  loader.LoadResource(effective_request, std::move(callback));
  return true;
}

}  // namespace lynx::shell
