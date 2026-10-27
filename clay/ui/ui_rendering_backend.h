// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_UI_RENDERING_BACKEND_H_
#define CLAY_UI_UI_RENDERING_BACKEND_H_

#ifndef ENABLE_SKITY
#include "clay/third_party/txt/src/txt/asset_font_manager_skia.h"
#include "clay/third_party/txt/src/txt/font_collection_skia.h"
#include "clay/ui/resource/asset_manager_font_provider_skia.h"
#else
#include "clay/gfx/image/animated_image.h"
#include "clay/gfx/image/base_image.h"
#include "clay/gfx/skity/skity_image.h"
#include "clay/third_party/txt/src/txt/asset_font_manager_skity.h"
#include "clay/third_party/txt/src/txt/font_collection_skity.h"
#include "clay/ui/resource/asset_manager_font_provider_skity.h"
#endif  // ENABLE_SKITY

#endif  // CLAY_UI_UI_RENDERING_BACKEND_H_
