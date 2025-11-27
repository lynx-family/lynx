// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_EMBEDDER_PLATFORM_VIEW_EMBEDDER_DELEGATE_H_
#define CLAY_SHELL_PLATFORM_EMBEDDER_PLATFORM_VIEW_EMBEDDER_DELEGATE_H_

#include <functional>
#include <memory>
#include <string>

#include "base/include/fml/macros.h"
#include "clay/net/loader/resource_loader_intercept.h"

namespace clay {

class PlatformViewEmbedderDelegate {
 public:
  PlatformViewEmbedderDelegate() = default;
  virtual ~PlatformViewEmbedderDelegate() = default;

  virtual std::string ShouldInterceptUrl(const std::string& origin_url,
                                         bool should_decode);

  virtual std::shared_ptr<clay::ResourceLoaderIntercept>
  GetResourceLoaderIntercept();

  virtual void BindShouldInterceptUrlCallback(
      ResourceLoaderShouldInterceptUrlCallback callback);

  virtual bool OffscreenRenderingEnabled() const { return false; }

 private:
  std::shared_ptr<clay::ResourceLoaderIntercept> resource_loader_intercept_;

  BASE_DISALLOW_COPY_AND_ASSIGN(PlatformViewEmbedderDelegate);
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_EMBEDDER_PLATFORM_VIEW_EMBEDDER_DELEGATE_H_
