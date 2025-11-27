// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/embedder/platform_view_embedder_delegate.h"

namespace clay {

std::string PlatformViewEmbedderDelegate::ShouldInterceptUrl(
    const std::string& origin_url, bool should_decode) {
  if (resource_loader_intercept_) {
    return resource_loader_intercept_->ShouldInterceptUrl(origin_url,
                                                          should_decode);
  }
  return origin_url;
}

void PlatformViewEmbedderDelegate::BindShouldInterceptUrlCallback(
    ResourceLoaderShouldInterceptUrlCallback callback) {
  GetResourceLoaderIntercept()->BindShouldInterceptUrlCallback(callback);
}

std::shared_ptr<clay::ResourceLoaderIntercept>
PlatformViewEmbedderDelegate::GetResourceLoaderIntercept() {
  if (!resource_loader_intercept_) {
    resource_loader_intercept_ =
        std::make_shared<clay::ResourceLoaderIntercept>();
  }
  return resource_loader_intercept_;
}

}  // namespace clay
