// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <lib/zx/channel.h>

#include "third_party/skia/include/ports/SkFontMgr_fuchsia.h"
#include "txt/platform.h"

namespace txt {

std::vector<std::string> GetDefaultFontFamilies() {
  return {"Roboto"};
}

#ifdef ENABLE_SKITY
std::shared_ptr<skity::FontManager> GetDefaultFontManager(
    uint32_t font_initialization_data) {
  return nullptr;
}
#else
sk_sp<SkFontMgr> GetDefaultFontManager(uint32_t font_initialization_data) {
  if (font_initialization_data) {
    fuchsia::fonts::ProviderSyncPtr sync_font_provider;
    sync_font_provider.Bind(zx::channel(font_initialization_data));
    return SkFontMgr_New_Fuchsia(std::move(sync_font_provider));
  } else {
    return SkFontMgr::RefDefault();
  }
}
#endif  // ENABLE_SKITY

}  // namespace txt
