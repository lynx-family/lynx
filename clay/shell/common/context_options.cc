// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/context_options.h"

#include "clay/common/graphics/persistent_cache.h"
#include "clay/common/settings.h"

namespace clay {

#ifndef ENABLE_SKITY
GrContextOptions MakeDefaultContextOptions(ContextType type,
                                           std::optional<GrBackendApi> api) {
  GrContextOptions options;

  if (PersistentCache::cache_sksl()) {
    options.fShaderCacheStrategy = GrContextOptions::ShaderCacheStrategy::kSkSL;
  }
  PersistentCache::MarkStrategySet();
  options.fPersistentCache =
      PersistentCache::GetCacheForProcess()->GetResourceCache();

  if (api.has_value() && api.value() == GrBackendApi::kOpenGL) {
    options.fAvoidStencilBuffers = !Settings::ShouldEnableStencilBuffer();

    // To get video playback on the widest range of devices, we limit Skia to
    // ES2 shading language when the ES3 external image extension is missing.
    options.fPreferExternalImagesOverES3 = true;
  }

  // TODO(goderbauer): remove option when skbug.com/7523 is fixed.
  options.fDisableGpuYUVConversion = true;

  options.fReduceOpsTaskSplitting = GrContextOptions::Enable::kNo;

  // options.fReducedShaderVariations = false;

  return options;
}
#endif  // ENABLE_SKITY
}  // namespace clay
