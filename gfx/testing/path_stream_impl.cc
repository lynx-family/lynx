// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gfx/testing/path_stream_impl.h"

#include <memory>
#include <utility>

namespace lynx::gfx {
namespace {
class TestPathImplFactory final : public PathImplFactory {
 public:
  std::unique_ptr<PathImpl> Create(PathSource source) const override {
    return std::make_unique<PathStreamImpl>(std::move(source));
  }
};
}  // namespace

const PathImplFactory& GetTestPathImplFactory() {
  static const TestPathImplFactory factory;
  return factory;
}

}  // namespace lynx::gfx
