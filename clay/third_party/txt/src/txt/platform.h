// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_THIRD_PARTY_TXT_SRC_TXT_PLATFORM_H_
#define CLAY_THIRD_PARTY_TXT_SRC_TXT_PLATFORM_H_

#include <memory>
#include <string>
#include <vector>

#include "base/include/fml/macros.h"
#ifdef ENABLE_SKITY
#include "skity/text/font_manager.hpp"
#else
#include "third_party/skia/include/core/SkFontMgr.h"
#endif

namespace txt {

std::vector<std::string> GetDefaultFontFamilies();

#ifdef ENABLE_SKITY
std::shared_ptr<skity::FontManager> GetDefaultFontManager(
    uint32_t font_initialization_data);
#else
sk_sp<SkFontMgr> GetDefaultFontManager(uint32_t font_initialization_data);
#endif  // ENABLE_SKITY

}  // namespace txt

#endif  // CLAY_THIRD_PARTY_TXT_SRC_TXT_PLATFORM_H_
