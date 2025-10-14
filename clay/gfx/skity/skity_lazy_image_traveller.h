// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SKITY_SKITY_LAZY_IMAGE_TRAVELLER_H_
#define CLAY_GFX_SKITY_SKITY_LAZY_IMAGE_TRAVELLER_H_

#include <functional>
#include <memory>
#include <vector>

#include "skity/render/canvas.hpp"

namespace skity {
class DisplayList;
class Image;
}  // namespace skity

namespace clay {

//------------------------------------------------------------------------------
/// Can be fed to the dispatch() method of a DisplayList to feed the resulting
/// rendering operations to an SkCanvas instance.
///
/// Receives all methods on Dispatcher and sends them to an SkCanvas
///

using LazyImageDecodeCallback = std::function<void(bool)>;

class SkityLazyImageTraveller {
 public:
  static bool Traversal(skity::DisplayList* disp_list,
                        const LazyImageDecodeCallback& callback);

 private:
  class TraversalHelper : public skity::Canvas {
   public:
    explicit TraversalHelper(const LazyImageDecodeCallback& callback)
        : callback_(callback){};

    bool hasLazyImage() const { return has_lazy_image_; }

   protected:
    // Spy skity canvas.
    using ClipOp = skity::Canvas::ClipOp;
    void OnClipRect(skity::Rect const& rect, ClipOp op) override {}
    void OnClipPath(skity::Path const& path, ClipOp op) override {}
    void OnDrawGlyphs(uint32_t count, const skity::GlyphID glyphs[],
                      const float position_x[], const float position_y[],
                      const skity::Font& font,
                      const skity::Paint& paint) override {}
    void OnDrawPath(skity::Path const& path,
                    skity::Paint const& paint) override {}
    void OnDrawPaint(skity::Paint const& paint) override {}
    void OnSaveLayer(const skity::Rect& bounds,
                     const skity::Paint& paint) override {}
    void OnDrawBlob(const skity::TextBlob* blob, float x, float y,
                    skity::Paint const& paint) override {}

    void OnSave() override {}
    void OnRestore() override {}
    void OnRestoreToCount(int saveCount) override {}
    void OnFlush() override {}
    uint32_t OnGetWidth() const override { return 0; }
    uint32_t OnGetHeight() const override { return 0; }
    void OnUpdateViewport(uint32_t width, uint32_t height) override {}

    void OnDrawImageRect(std::shared_ptr<skity::Image> image,
                         const skity::Rect& src, const skity::Rect& dst,
                         const skity::SamplingOptions& sampling,
                         skity::Paint const* paint) override {
      DecodeAndUploadImageIfNeeded(image);
    }

   private:
    void DecodeAndUploadImageIfNeeded(
        const std::shared_ptr<skity::Image> image);

    const LazyImageDecodeCallback& callback_;
    bool has_lazy_image_ = false;
  };
};

}  // namespace clay

#endif  // CLAY_GFX_SKITY_SKITY_LAZY_IMAGE_TRAVELLER_H_
