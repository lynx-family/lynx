// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_RESOURCE_LYNX_RESOURCE_LOADER_EMBEDDER_H_
#define PLATFORM_EMBEDDER_RESOURCE_LYNX_RESOURCE_LOADER_EMBEDDER_H_

#include <memory>
#include <utility>

#include "core/public/lynx_resource_loader.h"
#include "platform/embedder/fetcher/lynx_resource_fetcher_holder.h"
#include "platform/embedder/resource/js_source_loader_desktop.h"

namespace lynx {
namespace embedder {

class LynxResourceLoaderEmbedder : public pub::LynxResourceLoader {
 public:
  LynxResourceLoaderEmbedder() = default;
  ~LynxResourceLoaderEmbedder() = default;

  void SetResourceFetcherHolder(
      std::shared_ptr<LynxResourceFetcherHolder> resource_fetcher_holder);

  void LoadResource(const pub::LynxResourceRequest& request,
                    base::MoveOnlyClosure<void, pub::LynxResourceResponse&>
                        callback) override;

 private:
  runtime::js::JSSourceLoaderDesktop js_source_loader_;
  std::shared_ptr<LynxResourceFetcherHolder> resource_fetcher_holder_;
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_RESOURCE_LYNX_RESOURCE_LOADER_EMBEDDER_H_
