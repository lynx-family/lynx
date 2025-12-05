// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_STYLE_MASK_FILTER_H_
#define CLAY_GFX_STYLE_MASK_FILTER_H_

#include <memory>
#include <utility>

#include "clay/fml/logging.h"
#include "clay/gfx/attributes.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

#ifdef ENABLE_SKITY
// Copy from third_party/skia/src/core/SkBlurMask.cpp.
// Converts a sigma to blur radius in pixels.
inline float ConvertSigmaToRadius(float sigma) {
  return sigma > 0.5f ? (sigma - 0.5f) / 0.57735f : 0.0f;
}
#endif  // ENABLE_SKITY

class BlurMaskFilter;
// An enumerated type for the recognized MaskFilter operations.
// If a custom MaskFilter outside of the recognized types is needed
// then a |kUnknown| type that simply defers to an SkMaskFilter is
// provided as a fallback.
enum class MaskFilterType { kBlur, kUnknown };

class MaskFilter : public Attribute<MaskFilter, GrMaskFilter, MaskFilterType> {
 public:
  // Return a BlurMaskFilter pointer to this object iff it is a Blur
  // type of MaskFilter, otherwise return nullptr.
  virtual const BlurMaskFilter* asBlur() const { return nullptr; }

  std::shared_ptr<MaskFilter> MakeBlur(GrBlurStyle style, float sigma,
                                       bool respect_ctm = true);
};

// The Blur type of MaskFilter which specifies modifying the
// colors as if the color specified in the Blur filter is the
// source color and the color drawn by the rendering operation
// is the destination color. The mode parameter of the Blur
// filter is then used to combine those colors.
class BlurMaskFilter final : public MaskFilter {
 public:
  BlurMaskFilter(GrBlurStyle style, float sigma, bool respect_ctm = true)
      : style_(style), sigma_(sigma), respect_ctm_(respect_ctm) {}
  BlurMaskFilter(const BlurMaskFilter& filter)
      : BlurMaskFilter(filter.style_, filter.sigma_, filter.respect_ctm_) {}
  BlurMaskFilter(const BlurMaskFilter* filter)
      : BlurMaskFilter(filter->style_, filter->sigma_, filter->respect_ctm_) {}

  MaskFilterType type() const override { return MaskFilterType::kBlur; }
  size_t size() const override { return sizeof(*this); }

  std::shared_ptr<MaskFilter> shared() const override {
    return std::make_shared<BlurMaskFilter>(this);
  }

  GrMaskFilterPtr gr_object() const override {
#ifndef ENABLE_SKITY
    return SkMaskFilter::MakeBlur(style_, sigma_, respect_ctm_);
#else
    return skity::MaskFilter::MakeBlur(style_, ConvertSigmaToRadius(sigma_));
#endif  // ENABLE_SKITY
  }

  const BlurMaskFilter* asBlur() const override { return this; }

  GrBlurStyle style() const { return style_; }
  float sigma() const { return sigma_; }
  bool respectCTM() const { return respect_ctm_; }

 protected:
  bool equals_(MaskFilter const& other) const override {
    FML_DCHECK(other.type() == MaskFilterType::kBlur);
    auto that = static_cast<BlurMaskFilter const*>(&other);
    return style_ == that->style_ && sigma_ == that->sigma_ &&
           respect_ctm_ == that->respect_ctm_;
  }

 private:
  GrBlurStyle style_;
  float sigma_;
  // Added for backward compatibility with Flutter text shadow rendering which
  // uses Skia blur filters with this flag set to false.
  bool respect_ctm_;
};

// A wrapper class for a Skia MaskFilter of unknown type. The above 4 types
// are the only types that can be constructed by Flutter using the
// ui.MaskFilter class so this class should be rarely used. The main use
// would come from the |DisplayListCanvasRecorder| recording Skia rendering
// calls. This would primarily happen in the Paragraph code that renders the
// text using the SkCanvas interface which we capture into DisplayList data
// structures.
class UnknownMaskFilter final : public MaskFilter {
 public:
  UnknownMaskFilter(GrMaskFilterPtr sk_filter)
      : sk_filter_(std::move(sk_filter)) {}

  UnknownMaskFilter(const UnknownMaskFilter& filter)
      : UnknownMaskFilter(filter.sk_filter_) {}
  UnknownMaskFilter(const UnknownMaskFilter* filter)
      : UnknownMaskFilter(filter->sk_filter_) {}

  MaskFilterType type() const override { return MaskFilterType::kUnknown; }
  size_t size() const override { return sizeof(*this); }

  std::shared_ptr<MaskFilter> shared() const override {
    return std::make_shared<UnknownMaskFilter>(this);
  }

  GrMaskFilterPtr gr_object() const override { return sk_filter_; }

  virtual ~UnknownMaskFilter() = default;

 protected:
  bool equals_(const MaskFilter& other) const override {
    FML_DCHECK(other.type() == MaskFilterType::kUnknown);
    auto that = static_cast<UnknownMaskFilter const&>(other);
    return sk_filter_ == that.sk_filter_;
  }

 private:
  GrMaskFilterPtr sk_filter_;
};

}  // namespace clay

#endif  // CLAY_GFX_STYLE_MASK_FILTER_H_
