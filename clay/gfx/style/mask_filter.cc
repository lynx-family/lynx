// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/style/mask_filter.h"

namespace clay {

std::shared_ptr<MaskFilter> MaskFilter::From(GrMaskFilter* sk_filter) {
  if (sk_filter == nullptr) {
    return nullptr;
  }
#ifndef ENABLE_SKITY
  // There are no inspection methods for SkMaskFilter so we cannot break
  // the Skia filter down into a specific subclass (i.e. Blur).
  return std::make_shared<UnknownMaskFilter>(sk_ref_sp(sk_filter));
#else
  FML_UNIMPLEMENTED();
  return nullptr;
#endif  // ENABLE_SKITY
}

}  // namespace clay
