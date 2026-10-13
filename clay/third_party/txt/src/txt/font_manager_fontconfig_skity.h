// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef TXT_FONT_MANAGER_FONTCONFIG_SKITY_H_
#define TXT_FONT_MANAGER_FONTCONFIG_SKITY_H_
#include <memory>
#include <skity/text/font_manager.hpp>
namespace txt {
std::shared_ptr<skity::FontManager> MakeFontconfigSkityFontManager();
}
#endif
