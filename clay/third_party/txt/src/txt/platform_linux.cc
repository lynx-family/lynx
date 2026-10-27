// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "txt/platform.h"

#ifdef FLUTTER_USE_FONTCONFIG
#include "third_party/skia/include/ports/SkFontMgr_fontconfig.h"
#else
#include "third_party/skia/include/ports/SkFontMgr_directory.h"
#endif

namespace txt {

std::vector<std::string> GetDefaultFontFamilies() {
  return {"Ubuntu", "Cantarell", "DejaVu Sans", "Liberation Sans", "Arial"};
}

#ifdef ENABLE_SKITY
std::shared_ptr<skity::FontManager> GetDefaultFontManager(
    uint32_t font_initialization_data) {
  return nullptr;
}
#else
sk_sp<SkFontMgr> GetDefaultFontManager(uint32_t font_initialization_data) {
#ifdef FLUTTER_USE_FONTCONFIG
  return SkFontMgr_New_FontConfig(nullptr);
#else
  return SkFontMgr_New_Custom_Directory("/usr/share/fonts/");
#endif
}
#endif  // ENABLE_SKITY

}  // namespace txt
