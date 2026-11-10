// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef GFX_TESTING_PATH_STREAM_IMPL_H_
#define GFX_TESTING_PATH_STREAM_IMPL_H_

#include <utility>

#include "gfx/public/path_impl_factory.h"

namespace lynx::gfx {

class PathStreamImpl final : public PathImpl {
 public:
  explicit PathStreamImpl(PathSource source) : PathImpl(std::move(source)) {}
};

const PathImplFactory& GetTestPathImplFactory();

}  // namespace lynx::gfx

#endif  // GFX_TESTING_PATH_STREAM_IMPL_H_
