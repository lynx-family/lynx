// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "explorer/windows/lynx_explorer/fetcher/example_generic_resource_fetcher.h"

#include <cstdint>
#include <string>
#include <thread>

#include "explorer/windows/lynx_explorer/httplib/httplib_client.h"
#include "third_party/httplib/httplib.h"

namespace lynx {
namespace example {

void ExampleGenericResourceFetcher::FetchResource(
    std::shared_ptr<pub::LynxResourceRequest> fetcher_equest,
    std::shared_ptr<pub::LynxResourceResponse> fetcher_response) {
  std::thread t([fetcher_equest, fetcher_response]() {
    const char* url_str = fetcher_equest->GetUrl();
    std::string url(url_str ? url_str : "");

    std::string response = HttplibClient::Get(url);
    if (response.empty()) {
      fetcher_response->SetCode(-1);
      fetcher_response->SetErrorMessage("http request failed");
    } else {
      fetcher_response->SetCode(0);
      fetcher_response->SetData((uint8_t*)response.c_str(), response.size());
    }
    fetcher_response->Complete();
  });
  t.detach();
}

void ExampleGenericResourceFetcher::FetchResourcePath(
    std::shared_ptr<pub::LynxResourceRequest> request,
    std::shared_ptr<pub::LynxResourceResponse> response) {}

}  // namespace example
}  // namespace lynx
