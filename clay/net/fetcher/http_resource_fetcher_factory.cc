// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/net/fetcher/http_resource_fetcher_factory.h"

#include <utility>

#include "clay/net/fetcher/http_resource_fetcher_host.h"

namespace clay {

ResourceFetcherCreator& HttpResourceFetcherFactory::GetCustomFetcherCreator() {
  static ResourceFetcherCreator custom_fetcher_creator;
  return custom_fetcher_creator;
}

// static
std::unique_ptr<HttpResourceFetcher> HttpResourceFetcherFactory::CreateFetcher(
    NetLoaderManager::HostNetLoader host_net_loader, const url::Uri& uri,
    size_t request_seq, int retry_time) {
  if (host_net_loader != nullptr) {
    return CreateHostFetcher(host_net_loader, uri, request_seq, retry_time);
  } else {
    return CreateCustomFetcher(uri, request_seq, retry_time);
  }
}

// static
void HttpResourceFetcherFactory::SetCustomFetcherCreator(
    ResourceFetcherCreator creator) {
  GetCustomFetcherCreator() = std::move(creator);
}

// static
std::unique_ptr<HttpResourceFetcher>
HttpResourceFetcherFactory::CreateCustomFetcher(const url::Uri& uri,
                                                size_t request_seq,
                                                int retry_time) {
  auto& custom_fetcher_creator = GetCustomFetcherCreator();
  if (custom_fetcher_creator) {
    return custom_fetcher_creator(uri, request_seq, retry_time);
  } else {
    return nullptr;
  }
}

// static
std::unique_ptr<HttpResourceFetcher>
HttpResourceFetcherFactory::CreateHostFetcher(
    NetLoaderManager::HostNetLoader host_net_loader, const url::Uri& uri,
    size_t request_seq, int retry_time) {
  return std::make_unique<HttpResourceFetcherHost>(uri, request_seq, retry_time,
                                                   host_net_loader);
}

}  // namespace clay
