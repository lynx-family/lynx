// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_FETCHER_LYNX_RESOURCE_FETCHER_HOLDER_H_
#define PLATFORM_EMBEDDER_FETCHER_LYNX_RESOURCE_FETCHER_HOLDER_H_

#include "platform/embedder/fetcher/lynx_generic_resource_fetcher_priv.h"

namespace lynx {
namespace embedder {

class LynxResourceFetcherHolder {
 public:
  explicit LynxResourceFetcherHolder(
      lynx_generic_resource_fetcher_t* generic_fetcher);
  ~LynxResourceFetcherHolder();

  lynx_generic_resource_fetcher_t* GenericFetcher() const;

 private:
  lynx_generic_resource_fetcher_t* generic_fetcher_;
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_FETCHER_LYNX_RESOURCE_FETCHER_HOLDER_H_
