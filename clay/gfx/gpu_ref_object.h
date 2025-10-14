// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_GPU_REF_OBJECT_H_
#define CLAY_GFX_GPU_REF_OBJECT_H_

#include "base/include/fml/memory/ref_counted.h"

namespace clay {

class GPURefObject : public fml::RefCountedThreadSafe<GPURefObject> {
 public:
  GPURefObject() = default;
  virtual ~GPURefObject() = default;
};

}  // namespace clay

#endif  // CLAY_GFX_GPU_REF_OBJECT_H_
