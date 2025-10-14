// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/net/fetcher/http_resource_fetcher_host.h"

#include <cstring>
#include <string>
#include <utility>

namespace clay {

HttpResourceFetcherHost::HttpResourceFetcherHost(
    const url::Uri& uri, size_t request_seq, int failed_time,
    NetLoaderManager::HostNetLoader host_net_loader)
    : HttpResourceFetcher(uri, request_seq, failed_time),
      host_net_loader_(std::move(host_net_loader)) {}

HttpResourceFetcherHost::~HttpResourceFetcherHost() = default;

void HttpResourceFetcherHost::Load() {
  host_net_loader_(uri_.Spec(), method_.empty() ? "GET" : method_, body_,
                   headers_, request_seq_);
}

RawResource HttpResourceFetcherHost::LoadSync() {
  is_sync_ = true;
  host_net_loader_(uri_.Spec(), method_, body_, headers_, request_seq_);
  load_latch_.Wait();
  return resource_;
}

void HttpResourceFetcherHost::Cancel() {
  HttpResourceFetcher::Cancel();
  load_latch_.Signal();
}

void HttpResourceFetcherHost::OnFinished(bool success, const uint8_t* raw_data,
                                         size_t length) {
  auto data = std::shared_ptr<uint8_t>(new uint8_t[length],
                                       std::default_delete<uint8_t[]>());
  ::memcpy(reinterpret_cast<void*>(data.get()), raw_data, length);
  resource_.data = std::move(data);
  resource_.length = length;
  if (is_sync_) {
    load_latch_.Signal();
  } else {
    if (success) {
      NetLoaderManager::Instance().OnSucceeded(uri_.Spec(), request_seq_,
                                               std::move(resource_));
    } else {
      std::string reason;
      if (resource_.data) {
        reason.assign(reinterpret_cast<const char*>(resource_.data.get()),
                      resource_.length);
      }
      NetLoaderManager::Instance().OnFailed(uri_.Spec(), request_seq_,
                                            FailedTime(), reason);
    }
  }
}

}  // namespace clay
