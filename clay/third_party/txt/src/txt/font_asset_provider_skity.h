// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_THIRD_PARTY_TXT_SRC_TXT_FONT_ASSET_PROVIDER_SKITY_H_
#define CLAY_THIRD_PARTY_TXT_SRC_TXT_FONT_ASSET_PROVIDER_SKITY_H_

#include <string>
#include "skity/text/font_manager.hpp"

namespace txt {

class FontAssetProvider {
 public:
  virtual ~FontAssetProvider() = default;

  virtual size_t GetFamilyCount() const = 0;
  virtual std::string GetFamilyName(int index) const = 0;
  virtual std::shared_ptr<skity::FontStyleSet> MatchFamily(const std::string& family_name) = 0;

 protected:
  static std::string CanonicalFamilyName(std::string family_name);
};

}  // namespace txt

#endif  // CLAY_THIRD_PARTY_TXT_SRC_TXT_FONT_ASSET_PROVIDER_SKITY_H_
