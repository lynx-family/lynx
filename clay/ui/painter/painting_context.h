// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_PAINTER_PAINTING_CONTEXT_H_
#define CLAY_UI_PAINTER_PAINTING_CONTEXT_H_

#include <memory>

#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/float_rounded_rect.h"
#include "clay/gfx/geometry/path.h"
#include "clay/gfx/geometry/rect.h"
#include "clay/gfx/graphics_context.h"
#include "clay/ui/compositing/pending_layer.h"
#include "clay/ui/compositing/pending_picture_layer.h"

namespace clay {

class ImageResource;
class PendingContainerLayer;
class PendingLayer;
class PendingPictureLayer;
class RenderObject;

class PaintingContext;
typedef std::function<void(PaintingContext&, const FloatPoint&)>
    PaintingContextCallback;

// A place to paint and build a composited layer.
class PaintingContext {
 public:
  PaintingContext(PendingContainerLayer* layer, RenderObject* object,
                  fml::RefPtr<GPUUnrefQueue> unref_queue);
  ~PaintingContext();

  static void RepaintCompositedChild(RenderObject* object,
                                     fml::RefPtr<GPUUnrefQueue> unref_queue,
                                     PaintingContext* parent_context = nullptr);

  void SetIsComplexHint();
  void SetWillChangeHint();

  // Paint a child [RenderObject].
  void PaintChild(RenderObject* child, const FloatPoint& offset);

  bool IsRecording() const;
  GraphicsContext* GetGraphicsContext();
  PendingLayer* ContentLayer() const { return current_layer_; }
  PendingContainerLayer* GetContainerLayer() const { return container_layer_; }

  void StopRecordingIfNeeded();

  RenderObject* render_object() { return render_object_; }

  // Adds a layer to the recording requiring that the recording is already
  // stopped.
  void AppendLayer(PendingLayer* layer);
  // Adds a composited leaf layer to the recording.
  void AddLayer(PendingLayer* layer);
  // Appends the given layer to the recording, and calls the `painter` callback
  // with that layer.
  void PushLayer(PendingContainerLayer* child_layer,
                 const PaintingContextCallback& painter,
                 const FloatPoint& offset);

  void PushOpacity(int alpha, const FloatPoint& offset,
                   const PaintingContextCallback& painter);
  void PushColorFilter(std::shared_ptr<ColorFilter> color_filter,
                       const FloatPoint& offset,
                       const PaintingContextCallback& painter);
  void PushImageFilter(std::shared_ptr<ImageFilter> image_filter,
                       const FloatPoint& offset,
                       const PaintingContextCallback& painter);
  void PushBackdropFilter(std::shared_ptr<ImageFilter> backdrop_filter,
                          const FloatPoint& offset,
                          const PaintingContextCallback& painter);
  void PushTransform(const TransformOperations& operations,
                     const FloatPoint& origin, const float perspective,
                     const FloatPoint& offset,
                     const PaintingContextCallback& painter);
  // Clip further painting using a rectangle.
  void PushClipRect(const FloatRect& clip_rect, const FloatPoint& offset,
                    const PaintingContextCallback& painter);
  // Clip further painting using a rounded rectangle.
  void PushClipRRect(const FloatRoundedRect& clip_rrect,
                     const FloatPoint& offset,
                     const PaintingContextCallback& painter);
  void PushClipPath(const GrPath& clip_rrect, const FloatPoint& offset,
                    const PaintingContextCallback& painter);
  void PushShaderMask(std::shared_ptr<ColorSource> color_source,
                      const FloatRect& mask_rect, BlendMode blend_mode,
                      const FloatPoint& offset,
                      const PaintingContextCallback& painter);

  // Container layer will contain picture layer and other layer subtree.
  void PushContainer(const FloatPoint& offset,
                     const PaintingContextCallback& painter);

  void PushExternalContainer(const ElementId& element_id,
                             const PaintingContextCallback& painter,
                             const skity::Vec2& size);

  void AddDrawableImage(int dx, int dy, int width, int height, int64_t image_id,
                        clay::DrawableImage::FitMode fit_mode);
  void AddPlatformView(int dx, int dy, int width, int height, int64_t view_id);
  void AddPunchHole(const Rect& punch_hole_rect);
  void SetCacheStrategy(CacheStrategy strategy) { strategy_ = strategy; }
  CacheStrategy GetCacheStrategy() const { return strategy_; }

 private:
  void StartRecording();
  void CompositeChild(RenderObject* child, const FloatPoint& offset);

  // Recording state.
  PendingPictureLayer* current_layer_ = nullptr;
  GraphicsContext graphics_context_;

  RenderObject* render_object_ = nullptr;
  PendingContainerLayer* container_layer_ = nullptr;
  // The strategy used for recorded layer and appended layers. This will inherit
  // from parent context or get from render object.
  CacheStrategy strategy_ = CacheStrategy::None;
};

}  // namespace clay

#endif  // CLAY_UI_PAINTER_PAINTING_CONTEXT_H_
