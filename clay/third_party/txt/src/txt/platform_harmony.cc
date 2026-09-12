/*
 * Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Copyright 2021 The Lynx Authors. All rights reserved.
 * Licensed under the Apache License Version 2.0 that can be found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "txt/platform.h"

#include "third_party/skia/include/ports/SkFontMgr_directory.h"

namespace txt {

std::vector<std::string> GetDefaultFontFamilies() {
  return {"sans-serif", "HarmonyOS Sans", "FZHeiT"};
}

sk_sp<SkFontMgr> GetDefaultFontManager(uint32_t font_initialization_data) {
  // OHOS ships its system fonts (HarmonyOS Sans / FZHeiT / DejaVu / emoji /
  // etc.) as loose .ttf files under /system/fonts/. Scan that directory so
  // Lynx <text> can actually rasterize glyphs -- SkFontMgr::RefDefault() would
  // otherwise be the custom-directory factory pointed at the Linux
  // /usr/share/fonts/, which does not exist here, giving a manager with no
  // families and text that lays out to nothing.
  //
  // Per-character fallback (a CJK codepoint missing from the requested family)
  // is handled by SkFontMgr_Custom::onMatchFamilyStyleCharacter.
  static sk_sp<SkFontMgr> mgr = SkFontMgr_New_Custom_Directory("/system/fonts/");
  if (mgr && mgr->countFamilies() > 0) {
    return mgr;
  }
  return SkFontMgr::RefDefault();
}

}  // namespace txt
