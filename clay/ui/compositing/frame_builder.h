// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPOSITING_FRAME_BUILDER_H_
#define CLAY_UI_COMPOSITING_FRAME_BUILDER_H_

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "base/include/fml/memory/ref_ptr.h"
#include "clay/flow/layers/layer_tree.h"
#include "clay/flow/raster_cache.h"
#include "clay/gfx/geometry/float_rounded_rect.h"
#include "clay/gfx/geometry/transform_operations.h"
#include "clay/gfx/gpu_object.h"
#include "clay/gfx/picture.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/gfx/style/color_filter.h"
#include "clay/gfx/style/image_filter.h"

namespace clay {
class AnimationHost;
class ContainerLayer;
class Layer;
class LayerTree;
}  // namespace clay

namespace clay {

class PendingLayer;
class PendingPictureLayer;

using CacheStrategy = clay::CacheStrategy;

class FrameBuilder {
 public:
  FrameBuilder(const skity::Vec2& frame_size, float device_pixel_ratio,
               fml::RefPtr<GPUUnrefQueue> unref_queue);
  ~FrameBuilder();

  FrameBuilder(const FrameBuilder&) = delete;
  FrameBuilder& operator=(const FrameBuilder&) = delete;

  void UpdateFrameSize(const skity::Vec2& size, float device_pixel_ratio);
  void UpdateFrameSize(uint32_t physical_width, uint32_t physical_height,
                       float device_pixel_ratio) {
    UpdateFrameSize({physical_width, physical_height}, device_pixel_ratio);
  }

  std::unique_ptr<Picture> GeneratePicture(const skity::Rect& bounds);

  void PushTransformOperations(const TransformOperations& transform,
                               double origin_x, double origin_y,
                               double offset_x, double offset_y,
                               PendingLayer* old_layer);
  void PushStaticTransform(skity::Matrix transform, PendingLayer* old_layer);
  void PushOffset(double dx, double dy, PendingLayer* old_layer);
  void PushScrollOffset(double x, double y, double scroll_x, double scroll_y,
                        const FloatRect& offset_range,
                        const FloatRect& max_offset_range,
                        std::shared_ptr<clay::ScrollOffsetAnimation> animation,
                        PendingLayer* old_layer);
  void PushOpacity(int alpha, double dx, double dy, PendingLayer* old_layer);
  void PushColorFilter(std::shared_ptr<ColorFilter> color_filter,
                       PendingLayer* old_layer);
  void PushImageFilter(std::shared_ptr<ImageFilter> image_filter,
                       PendingLayer* old_layer);
  void PushShaderMask(std::shared_ptr<ColorSource> color_source,
                      const FloatRect& mask_rect, BlendMode blend_mode,
                      PendingLayer* old_layer);
  void PushBackdropFilter(std::shared_ptr<ImageFilter>,
                          PendingLayer* old_layer);
  void PushClipRect(const FloatRect& clip_rect, int clip_behavior,
                    PendingLayer* old_layer);
  void PushClipRRect(const FloatRoundedRect& clip_rrect, int clip_behavior,
                     PendingLayer* old_layer);
  void PushClipPath(const GrPath& path, int clip_behavior,
                    PendingLayer* old_layer);
  void Pop();

  void AddPicture(double dx, double dy, PendingPictureLayer* picture_layer,
                  bool complex_hint, bool change_hint,
                  CacheStrategy strategy = CacheStrategy::None);

  void AddDrawableImage(double dx, double dy, double width, double height,
                        int64_t image_id,
                        clay::DrawableImage::FitMode fit_mode);

  void AddPlatformView(double dx, double dy, double width, double height,
                       int64_t view_id);

  void AddPunchHole(const skity::Rect& rect);

  void PushExternalViewLayer(const ElementId& element_id,
                             const skity::Vec2& size);

  void AddPerformanceOverlay(uint64_t enable_options, double left, double right,
                             double top, double bottom);

  std::shared_ptr<clay::Layer> RootLayer() const;
  std::unique_ptr<clay::LayerTree> TakeLayerTree();

  template <class T>
  GPUObject<T> CreateGPUObject(fml::RefPtr<T> object) {
    if (!object) {
      return {};
    }
    return {std::move(object), unref_queue_};
  }

  // Build a frame (a tree of layers) in the engine.
  void BuildFrame(PendingLayer* root_layer);

  // Build a frame (a tree of layers) in the engine.
  void BuildSubtreeFrame(PendingLayer* root_layer);

  // Reset |FrameBuilder| for reuse.
  void Reset();

  void AddRetained(std::shared_ptr<clay::Layer> engine_layer);

#ifndef NDEBUG
  void DumpLayerTree() const;
#endif

 private:
  // Friend tests that need direct access to private methods.
  friend class FrameBuilderTest;

  void AddLayer(std::shared_ptr<clay::Layer> layer);
  void PushLayer(std::shared_ptr<clay::ContainerLayer> layer);
  void PopLayer();

  // Finish building the layer tree.
  void FinishBuild();

  PendingLayer* FindOwnedLayer(PendingLayer* layer) const;
  void CopyRasterAnimationsFromRetained(
      const std::shared_ptr<clay::Layer>& layer);

  // Physical pixels.
  skity::Vec2 frame_size_ = {0, 0};
  // Logical / Physical pixels ratio.
  float device_pixel_ratio_ = 1.0f;
  std::unique_ptr<clay::LayerTree> layer_tree_;
  fml::RefPtr<GPUUnrefQueue> unref_queue_;
  std::vector<std::shared_ptr<clay::ContainerLayer>> layer_stack_;
  std::shared_ptr<clay::AnimationHost> animation_host_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPOSITING_FRAME_BUILDER_H_
