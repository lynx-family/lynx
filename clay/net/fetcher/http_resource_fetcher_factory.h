// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_NET_FETCHER_HTTP_RESOURCE_FETCHER_FACTORY_H_
#define CLAY_NET_FETCHER_HTTP_RESOURCE_FETCHER_FACTORY_H_

#include <memory>

#include "clay/net/fetcher/http_resource_fetcher.h"
#include "clay/net/net_loader_manager.h"

namespace clay {

using ResourceFetcherCreator =
    std::function<std::unique_ptr<HttpResourceFetcher>(
        const url::Uri& uri, size_t request_seq, int retry_time)>;

class HttpResourceFetcherFactory {
 public:
  static std::unique_ptr<HttpResourceFetcher> CreateFetcher(
      NetLoaderManager::HostNetLoader host_net_loader, const url::Uri& uri,
      size_t request_seq, int retry_time = 0);

  static void SetCustomFetcherCreator(ResourceFetcherCreator creator);

 private:
  static std::unique_ptr<HttpResourceFetcher> CreateCustomFetcher(
      const url::Uri& uri, size_t request_seq, int retry_time);

  static std::unique_ptr<HttpResourceFetcher> CreateHostFetcher(
      NetLoaderManager::HostNetLoader host_net_loader, const url::Uri& uri,
      size_t request_seq, int retry_time);

  static ResourceFetcherCreator custom_fetcher_creator_;
};

}  // namespace clay

#endif  // CLAY_NET_FETCHER_HTTP_RESOURCE_FETCHER_FACTORY_H_
