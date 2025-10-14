// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_GFX_RENDERING_BACKEND_H_
#define CLAY_GFX_GFX_RENDERING_BACKEND_H_

#ifndef ENABLE_SKITY
#ifdef ENABLE_SVG
#include "clay/gfx/image/svg_image_holder.h"
#endif  // ENABLE_SVG
#include "clay/gfx/image/graphics_image_skia.h"
#include "clay/gfx/image/graphics_image_skia_lazy.h"
#include "clay/gfx/paint_image_skia.h"
#include "clay/gfx/paint_image_skia_lazy.h"
#include "clay/gfx/shared_image/skia_gl_image_representation.h"
#include "clay/gfx/skia/picture_skia.h"
#include "clay/gfx/text_blob_skia.h"
#else
#ifdef ENABLE_SVG
#include "clay/gfx/svg/svg_dom.h"
#endif  // ENABLE_SVG
#include "clay/gfx/image/graphics_image_skity.h"
#include "clay/gfx/image/graphics_image_skity_lazy.h"
#include "clay/gfx/image/image_info.h"
#include "clay/gfx/paint_image_skity.h"
#ifdef OS_ANDROID
#include "clay/gfx/shared_image/skity_gl_image_representation.h"
#endif  // OS_ANDROID
#include "clay/gfx/skity/picture_skity.h"
#include "clay/gfx/text_blob_skity.h"
#endif  // ENABLE_SKITY

#endif  // CLAY_GFX_GFX_RENDERING_BACKEND_H_
