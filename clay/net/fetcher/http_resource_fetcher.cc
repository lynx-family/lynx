// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/net/fetcher/http_resource_fetcher.h"

#include <memory>

#include "build/build_config.h"
#include "clay/fml/logging.h"
#include "clay/net/macros.h"

namespace clay {

HttpResourceFetcher::HttpResourceFetcher(const url::Uri& uri,
                                         size_t request_seq, int failed_time)
    : uri_(uri), request_seq_(request_seq), failed_time_(failed_time) {}

HttpResourceFetcher::~HttpResourceFetcher() = default;

// static
const char* HttpResourceFetcher::GetPlatformCAFile() {
#if defined(OS_LINUX)
  static constexpr char kUbuntuCertFile[] =
      "/etc/ssl/certs/ca-certificates.crt";
  return kUbuntuCertFile;
#elif defined(OS_MAC) || defined(OS_WIN) || defined(OS_ANDROID)
  return nullptr;
#else
  NET_LOG << "Won't set cert file on this platform (use default_verify_paths)";
  return nullptr;
#endif
}

// static
const char* HttpResourceFetcher::GetPlatformCAPath() {
#if defined(OS_ANDROID)
  static constexpr char kAndroidCertPath[] = "/system/etc/security/cacerts/";
  return kAndroidCertPath;
#elif defined(OS_MAC) || defined(OS_WIN) || defined(OS_LINUX)
  return nullptr;
#else
  NET_LOG << "Load on a platform not set cert file(use default_verify_paths)";
  return nullptr;
#endif
}

void HttpResourceFetcher::SetHeaders(
    std::map<std::string, std::string>&& headers) {
  headers_ = std::move(headers);
}

void HttpResourceFetcher::SetMethod(const std::string& method) {
  method_ = method;
}

void HttpResourceFetcher::SetBody(std::string&& body) {
  body_ = std::move(body);
}
}  // namespace clay
