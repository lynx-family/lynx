// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef GFX_PUBLIC_PATH_IMPL_FACTORY_H_
#define GFX_PUBLIC_PATH_IMPL_FACTORY_H_

#include <memory>

#include "gfx/public/path_impl.h"

namespace lynx::gfx {

// Create returns a non-null, source-only implementation. Implementations
// retain neither the factory nor its owner.
class GFX_EXPORT PathImplFactory {
 public:
  virtual ~PathImplFactory();
  virtual std::unique_ptr<PathImpl> Create(PathSource source) const = 0;
};

}  // namespace lynx::gfx

#endif  // GFX_PUBLIC_PATH_IMPL_FACTORY_H_
