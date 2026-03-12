// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef EXPLORER_WINDOWS_LYNX_EXPLORER_FETCHER_EXAMPLE_GENERIC_RESOURCE_FETCHER_H_
#define EXPLORER_WINDOWS_LYNX_EXPLORER_FETCHER_EXAMPLE_GENERIC_RESOURCE_FETCHER_H_

#include <memory>

#include "platform/embedder/public/lynx_generic_resource_fetcher.h"

namespace lynx {
namespace example {
class ExampleGenericResourceFetcher : public pub::LynxGenericResourceFetcher {
 public:
  void FetchResource(
      std::shared_ptr<pub::resource::LynxResourceRequest> request,
      std::shared_ptr<pub::resource::LynxResourceResponse> response) override;

  void FetchResourcePath(
      std::shared_ptr<pub::resource::LynxResourceRequest> request,
      std::shared_ptr<pub::resource::LynxResourceResponse> response) override;
};
}  // namespace example
}  // namespace lynx

#endif  // EXPLORER_WINDOWS_LYNX_EXPLORER_FETCHER_EXAMPLE_GENERIC_RESOURCE_FETCHER_H_
