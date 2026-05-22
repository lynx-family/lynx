// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RESOURCE_ASSET_FONT_MANAGER_CLAY_H_
#define CLAY_UI_RESOURCE_ASSET_FONT_MANAGER_CLAY_H_

#include <memory>

#include "clay/third_party/txt/src/txt/font_asset_provider.h"
#include "clay/ui/ui_rendering_backend.h"

namespace clay {
class AssetFontManagerClay : public txt::AssetFontManager {
 public:
  explicit AssetFontManagerClay(
      std::shared_ptr<FontResourceManager> font_resource_manager)
      : AssetFontManager(
            std::make_unique<AssetManagerFontProvider>(font_resource_manager)) {
  }

  ~AssetFontManagerClay() {}

  AssetManagerFontProvider& font_provider() {
    return static_cast<AssetManagerFontProvider&>(*font_provider_);
  }
};
}  // namespace clay

#endif  // CLAY_UI_RESOURCE_ASSET_FONT_MANAGER_CLAY_H_
