// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_NET_FETCHER_HTTP_RESOURCE_FETCHER_HOST_H_
#define CLAY_NET_FETCHER_HTTP_RESOURCE_FETCHER_HOST_H_

#include <memory>

#include "base/include/fml/synchronization/waitable_event.h"
#include "clay/net/fetcher/http_resource_fetcher.h"
#include "clay/net/net_loader_manager.h"

namespace clay {

class HttpResourceFetcherHost : public HttpResourceFetcher {
 public:
  HttpResourceFetcherHost(const url::Uri& uri, size_t request_seq,
                          int failed_time,
                          NetLoaderManager::HostNetLoader host_net_loader);

  virtual ~HttpResourceFetcherHost();

  void Load() override;

  RawResource LoadSync() override;

  void Cancel() override;

  void OnFinished(bool success, const uint8_t* raw_data, size_t length);

  bool IsHostLoader() override { return true; }

 private:
  bool is_sync_ = false;

  fml::AutoResetWaitableEvent load_latch_;
  RawResource resource_;

  NetLoaderManager::HostNetLoader host_net_loader_;
};

}  // namespace clay

#endif  // CLAY_NET_FETCHER_HTTP_RESOURCE_FETCHER_HOST_H_
