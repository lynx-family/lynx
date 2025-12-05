// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/style/mask_filter.h"

namespace clay {

std::shared_ptr<MaskFilter> MaskFilter::MakeBlur(GrBlurStyle style, float sigma,
                                                 bool respect_ctm) {
  return std::make_shared<BlurMaskFilter>(style, sigma, respect_ctm);
}

}  // namespace clay
