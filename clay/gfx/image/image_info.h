// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_IMAGE_INFO_H_
#define CLAY_GFX_IMAGE_IMAGE_INFO_H_

#include "clay/fml/logging.h"
#include "skity/geometry/vector.hpp"

namespace clay {

enum ColorType : int {
  kUnknown_ColorType,  //!< uninitialized
  kAlpha_8_ColorType,  //!< pixel with alpha in 8-bit byte
  kRGB_565_ColorType,  //!< pixel with 5 bits red, 6 bits green, 5 bits blue, in
                       //!< 16-bit word
  kARGB_4444_ColorType,  //!< pixel with 4 bits for alpha, red, green, blue; in
                         //!< 16-bit word
  kRGBA_8888_ColorType,  //!< pixel with 8 bits for red, green, blue, alpha; in
                         //!< 32-bit word
  kRGB_888x_ColorType,   //!< pixel with 8 bits each for red, green, blue; in
                         //!< 32-bit word
  kBGRA_8888_ColorType,  //!< pixel with 8 bits for blue, green, red, alpha; in
                         //!< 32-bit word
  kRGBA_1010102_ColorType,  //!< 10 bits for red, green, blue; 2 bits for alpha;
                            //!< in 32-bit word
  kBGRA_1010102_ColorType,  //!< 10 bits for blue, green, red; 2 bits for alpha;
                            //!< in 32-bit word
  kRGB_101010x_ColorType,  //!< pixel with 10 bits each for red, green, blue; in
                           //!< 32-bit word
  kBGR_101010x_ColorType,  //!< pixel with 10 bits each for blue, green, red; in
                           //!< 32-bit word
  kBGR_101010x_XR_ColorType,  //!< pixel with 10 bits each for blue, green, red;
                              //!< in 32-bit word, extended range
  kGray_8_ColorType,          //!< pixel with grayscale level in 8-bit byte
  kRGBA_F16Norm_ColorType,  //!< pixel with half floats in [0,1] for red, green,
                            //!< blue, alpha;
                            //   in 64-bit word
  kRGBA_F16_ColorType,  //!< pixel with half floats for red, green, blue, alpha;
                        //   in 64-bit word
  kRGBA_F32_ColorType,  //!< pixel using C float for red, green, blue, alpha; in
                        //!< 128-bit word

  // The following 6 colortypes are just for reading from - not for rendering to
  kR8G8_unorm_ColorType,  //!< pixel with a uint8_t for red and green

  kA16_float_ColorType,     //!< pixel with a half float for alpha
  kR16G16_float_ColorType,  //!< pixel with a half float for red and green

  kA16_unorm_ColorType,     //!< pixel with a little endian uint16_t for alpha
  kR16G16_unorm_ColorType,  //!< pixel with a little endian uint16_t for red and
                            //!< green
  kR16G16B16A16_unorm_ColorType,  //!< pixel with a little endian uint16_t for
                                  //!< red, green, blue
                                  //   and alpha

  kSRGBA_8888_ColorType,
  kR8_unorm_ColorType,

  kLastEnum_ColorType = kR8_unorm_ColorType,  //!< last valid value
};

enum AlphaType : int {
  kUnknown_AlphaType,   //!< uninitialized
  kOpaque_AlphaType,    //!< pixel is opaque
  kPremul_AlphaType,    //!< pixel components are premultiplied by alpha
  kUnpremul_AlphaType,  //!< pixel components are independent of alpha
  kLastEnum_AlphaType = kUnpremul_AlphaType,  //!< last valid value
};

// Information for a single frame of an animation.
struct ImageInfo {
  ImageInfo() = default;
  ImageInfo(int width, int height, ColorType color_type, AlphaType alpha_type)
      : width_(width),
        height_(height),
        color_type_(color_type),
        alpha_type_(alpha_type) {}

  int width() const { return width_; }
  int height() const { return height_; }

  size_t bytesPerPixel() const {
    switch (color_type_) {
      case kUnknown_ColorType:
        return 0;
      case kAlpha_8_ColorType:
        return 1;
      case kRGB_565_ColorType:
        return 2;
      case kARGB_4444_ColorType:
        return 2;
      case kRGBA_8888_ColorType:
        return 4;
      case kBGRA_8888_ColorType:
        return 4;
      case kRGB_888x_ColorType:
        return 4;
      case kRGBA_1010102_ColorType:
        return 4;
      case kRGB_101010x_ColorType:
        return 4;
      case kBGRA_1010102_ColorType:
        return 4;
      case kBGR_101010x_ColorType:
        return 4;
      case kBGR_101010x_XR_ColorType:
        return 4;
      case kGray_8_ColorType:
        return 1;
      case kRGBA_F16Norm_ColorType:
        return 8;
      case kRGBA_F16_ColorType:
        return 8;
      case kRGBA_F32_ColorType:
        return 16;
      case kR8G8_unorm_ColorType:
        return 2;
      case kA16_unorm_ColorType:
        return 2;
      case kR16G16_unorm_ColorType:
        return 4;
      case kA16_float_ColorType:
        return 2;
      case kR16G16_float_ColorType:
        return 4;
      case kR16G16B16A16_unorm_ColorType:
        return 8;
      case kSRGBA_8888_ColorType:
        return 4;
      case kR8_unorm_ColorType:
        return 1;
    }
    FML_UNREACHABLE();
  }

  bool isEmpty() const { return width_ == 0 || height_ == 0; }

  // To align with Skia's API
  static ImageInfo makeWH(int width, int height) {
    return ImageInfo(width, height, kRGBA_8888_ColorType, kPremul_AlphaType);
  }

  static ImageInfo makeDimensions(skity::Vec2 newSize) {
    return makeWH(newSize.x, newSize.y);
  }

  skity::Vec2 dimensions() const { return skity::Vec2(width_, height_); }

  ColorType colorType() const { return color_type_; }
  AlphaType alphaType() const { return alpha_type_; }

  void reset() {
    width_ = 0;
    height_ = 0;
  }

  // image width
  int width_ = 0;

  // image height
  int height_ = 0;

  ColorType color_type_ = kRGBA_8888_ColorType;
  AlphaType alpha_type_ = kPremul_AlphaType;
};

static inline bool operator==(const ImageInfo& a, const ImageInfo& b) {
  return a.width() == b.width() && a.height() == b.height() &&
         a.colorType() == b.colorType() && a.alphaType() == b.alphaType();
}

}  // namespace clay

#endif  // CLAY_GFX_IMAGE_IMAGE_INFO_H_
