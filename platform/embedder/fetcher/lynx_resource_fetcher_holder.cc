// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/fetcher/lynx_resource_fetcher_holder.h"

namespace lynx {
namespace embedder {

LynxResourceFetcherHolder::LynxResourceFetcherHolder(
    lynx_generic_resource_fetcher_t* generic_fetcher) {
  generic_fetcher_ = generic_fetcher;
  if (generic_fetcher_) {
    lynx_generic_resource_fetcher_ref(generic_fetcher_);
  }
}

lynx_generic_resource_fetcher_t* LynxResourceFetcherHolder::GenericFetcher()
    const {
  return generic_fetcher_;
}

LynxResourceFetcherHolder::~LynxResourceFetcherHolder() {
  if (generic_fetcher_) {
    lynx_generic_resource_fetcher_unref(generic_fetcher_);
  }
}

}  // namespace embedder
}  // namespace lynx
