// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_COMMON_GRAPHICS_GL_SCOPED_TEXTURE_BINDER_H_
#define CLAY_COMMON_GRAPHICS_GL_SCOPED_TEXTURE_BINDER_H_

#include <cstdint>

#include "base/include/fml/macros.h"
#include "build/build_config.h"
#include "clay/common/graphics/gl/gl_api.h"
#include "clay/common/graphics/gl/gl_api_common.h"

namespace clay {

class ScopedTextureBinder {
 public:
  ScopedTextureBinder(unsigned int target, unsigned int id,
                      GlApi* gl_api = GlApiCommon::GetInstance());  // NOLINT

  ~ScopedTextureBinder();

 private:
  // Failing that we use GL calls to save and restore state.
  int target_;
  int old_id_;
  GlApi* gl_api_ = nullptr;

  BASE_DISALLOW_COPY_AND_ASSIGN(ScopedTextureBinder);
};

}  // namespace clay

#endif  // CLAY_COMMON_GRAPHICS_GL_SCOPED_TEXTURE_BINDER_H_
