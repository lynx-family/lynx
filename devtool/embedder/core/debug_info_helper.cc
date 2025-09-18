// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/embedder/core/debug_info_helper.h"

#include <future>
#include <utility>

#include "base/include/log/logging.h"
#include "third_party/httplib/httplib.h"

namespace lynx {
namespace devtool {

namespace {

void SplitUrl(std::string url, std::string &host, std::string &path) {
  size_t protocol_end = url.find("://");
  size_t pos = 0;
  if (protocol_end != std::string::npos) {
    pos = protocol_end + 3;
  } else {
    size_t single_slash_pos = url.find(":/");
    if (single_slash_pos != std::string::npos) {
      pos = single_slash_pos + 2;
      url = url.substr(0, pos) + '/' + url.substr(pos);
      pos++;
    }
  }

  size_t path_start = url.find('/', pos);
  if (path_start != std::string::npos) {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    host = url.substr(0, path_start);
#else
    host = url.substr(pos, path_start - pos);
#endif
    path = url.substr(path_start);
  } else {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    host = url;
#else
    host = url.substr(pos);
#endif
    path = "/";
  }
}

constexpr int timeout_sec = 5;
constexpr char kDownloadThread[] = "MTS_DebugInfo_Download";

}  // namespace

void DebugInfoHelper::GetLepusDebugInfo(const std::string &url,
                                        std::string &debug_info) {
  LOGI("lepus debug: get debug info, url: " << url);
  std::string host, path;
  SplitUrl(url, host, path);
  if (host.empty()) {
    LOGE("lepus debug: Failed to download debug-info.json! Empty host!");
    return;
  }

  httplib::Client client(host);
  client.set_max_timeout(timeout_sec * 1000);
  client.set_connection_timeout(timeout_sec);
  client.set_read_timeout(timeout_sec);
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
  client.enable_server_certificate_verification(false);
#endif

  // Since httplib calls `CFRunLoopRunInMode()` during downloading (see
  // `getaddrinfo_with_timeout()`), which can cause subsequent tasks to be
  // executed prematurely and lead to unexpected behavior or errors.
  // Therefore, we dispatch the download task to a separate thread and use a
  // future to block the current thread while waiting for the result.
  std::promise<httplib::Result> promise;
  std::future<httplib::Result> future = promise.get_future();
  const auto &task_runner = GetDownloadTaskRunner();
  fml::TaskRunner::RunNowOrPostTask(task_runner,
                                    [client = std::move(client), path,
                                     promise = std::move(promise)]() mutable {
                                      auto res = client.Get(path.c_str());
                                      promise.set_value(std::move(res));
                                    });

  if (future.wait_for(std::chrono::seconds(timeout_sec)) !=
      std::future_status::ready) {
    LOGE("lepus debug: Failed to download debug-info.json! Timeout!");
    return;
  }

  auto res = future.get();
  if (res == nullptr) {
    LOGE("lepus debug: Failed to download debug-info.json! Null response!");
    return;
  }

  if (res->status == 200) {
    LOGI("lepus debug: Successfully downloaded debug-info.json!");
    debug_info = res->body;
  } else {
    LOGE("lepus debug: Failed to download debug-info.json! status: "
         << res->status << ", reason: " << res->reason);
  }
}

const fml::RefPtr<fml::TaskRunner> &DebugInfoHelper::GetDownloadTaskRunner() {
  static base::NoDestructor<fml::Thread> thread(fml::Thread::ThreadConfig(
      kDownloadThread, fml::Thread::ThreadPriority::NORMAL));
  return (*thread).GetTaskRunner();
}

}  // namespace devtool
}  // namespace lynx
