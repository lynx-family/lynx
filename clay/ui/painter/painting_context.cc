// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/painter/painting_context.h"

#include <memory>
#include <utility>

#include "clay/fml/logging.h"
#include "clay/gfx/geometry/float_rounded_rect.h"
#include "clay/gfx/gpu_object.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/ui/compositing/pending_container_layer.h"
#include "clay/ui/compositing/pending_drawable_image_layer.h"
#include "clay/ui/compositing/pending_effect_layer.h"
#include "clay/ui/compositing/pending_external_view_layer.h"
#include "clay/ui/compositing/pending_layer.h"
#include "clay/ui/compositing/pending_offset_layer.h"
#include "clay/ui/compositing/pending_picture_layer.h"
#include "clay/ui/compositing/pending_platform_view_layer.h"
#include "clay/ui/compositing/pending_punch_hole_layer.h"
#include "clay/ui/compositing/pending_transform_layer.h"
#include "clay/ui/painter/image_painter.h"
#include "clay/ui/rendering/render_external_view.h"
#include "clay/ui/rendering/render_object.h"

namespace clay {

// static
void PaintingContext::RepaintCompositedChild(
    RenderObject* child, fml::RefPtr<GPUUnrefQueue> unref_queue,
    PaintingContext* parent_context) {
  PendingContainerLayer* layer = child->GetLayer();
  if (layer) {
    layer->RemoveAllChildren();
    child->UnloadAllEffectLayers();
  } else {
    layer = new PendingOffsetLayer();
    child->SetLayer(std::unique_ptr<PendingContainerLayer>(layer));
  }

  // TODO(jinsong): Refactor PushXXX to make it easier to create
  // PendingEffectLayer.
  std::function<void(PaintingContext&, const FloatPoint&)> painter =
      [child](PaintingContext& ctx, const FloatPoint& offset) {
        child->PaintWithContext(ctx, offset);
      };
  if (child->CanDisplay()) {
    // PendingContainerLayer is used to contain all content layers of this
    // render object.
    if (child->IsRepaintBoundary() && !child->GetContainerLayer()) {
      painter = [old_painter = painter, child](PaintingContext& ctx,
                                               const FloatPoint& offset) {
        if (child->IsExternalView()) {
          // The ExternalView has different render backing , so we need to
          // create a new contianer layer to update it
          auto renderer = static_cast<RenderExternalView*>(child);
          skity::Vec2 size = renderer->GetSize();
          ctx.PushExternalContainer(renderer->element_id(), old_painter, size);
        } else {
          ctx.PushContainer(offset, old_painter);
        }
      };
    }

    if (child->HasOpacity() || child->HasOpacityRasterAnimation()) {
      painter = [child, old_painter = painter](PaintingContext& ctx,
                                               const FloatPoint& offset) {
        int alpha = 255;
        if (child->HasOpacity()) {
          // Convert opacity[0, 1.0] to alpha[0, 255].
          alpha = static_cast<int>(child->Opacity() * 255);
        }
        ctx.PushOpacity(alpha, offset, old_painter);
      };
    }

    if (child->HasColorFilter()) {
      painter = [child, old_painter = painter](PaintingContext& ctx,
                                               const FloatPoint& offset) {
        float m[20];
        // Get the matrix from SkColorMatrix.
        std::copy_n(child->ColorFilterMatrix().data(), 20, m);

        auto v = ColorFilter::MakeMatrix(m);
        ctx.PushColorFilter(v, offset, old_painter);
      };
    }

    if (child->HasBlur()) {
      painter = [child, old_painter = painter](PaintingContext& ctx,
                                               const FloatPoint& offset) {
        float blur = GraphicsContext::ConvertRadiusToSigma(child->BlurRadius());
        auto blur_filter =
            std::make_shared<BlurImageFilter>(blur, blur, TileMode::kDecal);
        ctx.PushImageFilter(blur_filter, offset, old_painter);
      };
    }
    if (child->HasClipPath()) {
      painter = [child, old_painter = painter](PaintingContext& ctx,
                                               const FloatPoint& offset) {
        auto& clip_path = child->ClipPath();
        if (auto ptr = std::get_if<FloatRoundedRect>(&clip_path)) {
          ctx.PushClipRRect(*ptr, offset, old_painter);
        } else if (auto ptr = std::get_if<GrPath>(&clip_path)) {
          ctx.PushClipPath(*ptr, offset, old_painter);
        }
      };
    }
    painter = child->FixupPainterIfNeeded(painter);
  }

  PaintingContext child_context(layer, child, unref_queue);
  // Inherit valid CacheStrategy from parent_context or from render object.
  if (parent_context &&
      parent_context->GetCacheStrategy() != CacheStrategy::None) {
    child_context.SetCacheStrategy(parent_context->GetCacheStrategy());
  } else {
    child_context.SetCacheStrategy(child->GetCacheStrategy());
  }

  if (child->HasMask()) {
    float width = child->GetFrameRect().width(),
          height = child->GetFrameRect().height();
#ifndef ENABLE_SKITY
    SkPictureRecorder recorder;
    SkCanvas* canvas = recorder.beginRecording(SkRect::MakeXYWH(
        0, 0, child->GetFrameRect().width(), child->GetFrameRect().height()));
    ImagePainter(static_cast<RenderBox*>(child)).PaintMaskImage(canvas);

    auto composite_picture = recorder.finishRecordingAsPicture();
    auto composite_shader = composite_picture->makeShader(
        SkTileMode::kClamp, SkTileMode::kClamp, SkFilterMode::kLinear);
    auto composite_color_source =
        std::make_shared<UnknownColorSource>(composite_shader);
#else
    auto deferred_image = skity::Image::MakeDeferredTextureImage(
        skity::TextureFormat::kRGBA, width, height,
        skity::AlphaType::kPremul_AlphaType);
    auto composite_shader = skity::Shader::MakeShader(deferred_image);
    auto composite_color_source =
        std::make_shared<UnknownColorSource>(composite_shader);

    skity::PictureRecorder recorder;
    recorder.BeginRecording(skity::Rect::MakeWH(width, height));
    skity::RecordingCanvas* canvas = recorder.GetRecordingCanvas();
    ImagePainter(static_cast<RenderBox*>(child))
        .PaintMaskImage(canvas, unref_queue);
    auto dl = recorder.FinishRecording();

    if (unref_queue && unref_queue->GetContext()) {
      unref_queue->GetTaskRunner()->PostTask(
          [dl = std::move(dl), context = unref_queue->GetContext(), width,
           height, deferred_image] {
            auto gpu_context = context;
            skity::GPURenderTargetDescriptor desc = {
                .width = static_cast<uint32_t>(width),
                .height = static_cast<uint32_t>(height),
            };
            auto render_target = gpu_context->CreateRenderTarget(desc);
            auto canvas = render_target->GetCanvas();
            dl->Draw(canvas);
            canvas->Flush();
            std::shared_ptr<skity::Image> snapshot =
                gpu_context->MakeSnapshot(std::move(render_target));
            deferred_image->SetTexture(*(snapshot->GetTexture()));
          });
    }
#endif  // ENABLE_SKITY
    painter = [width, height, composite_color_source, old_painter = painter](
                  PaintingContext& ctx, const FloatPoint& offset) {
      ctx.PushShaderMask(composite_color_source, FloatRect(0, 0, width, height),
                         BlendMode::kDstIn, offset, old_painter);
    };
  }

  if (child->HasTransform() || child->HasTransformRasterAnimation()) {
    FML_DCHECK(child->IsRepaintBoundary());
    painter = [child, old_painter = painter](PaintingContext& ctx,
                                             const FloatPoint& offset) {
      ctx.PushTransform(child->HasTransformOperations()
                            ? child->GetTransformOperations()
                            : TransformOperations(),
                        child->GetTransformOrigin(), child->GetPerspective(),
                        FloatPoint(), old_painter);
    };
  }

  painter(child_context, FloatPoint());
}

PaintingContext::PaintingContext(PendingContainerLayer* layer,
                                 RenderObject* object,
                                 fml::RefPtr<GPUUnrefQueue> unref_queue)
    : graphics_context_(unref_queue),
      render_object_(object),
      container_layer_(layer) {
  FML_DCHECK(render_object_);
}

PaintingContext::~PaintingContext() { StopRecordingIfNeeded(); }

GraphicsContext* PaintingContext::GetGraphicsContext() {
  if (!graphics_context_.IsRecording()) {
    StartRecording();
  }
  return &graphics_context_;
}

bool PaintingContext::IsRecording() const {
  return graphics_context_.IsRecording();
}

void PaintingContext::StartRecording() {
  FML_DCHECK(render_object());
  FML_DCHECK(!IsRecording());

  current_layer_ = new PendingPictureLayer();
  current_layer_->SetCacheStrategy(strategy_);
  double giant_scalar = 1.0E+9;
  skity::Rect rect = skity::Rect::MakeLTRB(-giant_scalar, -giant_scalar,
                                           giant_scalar, giant_scalar);
  auto success = graphics_context_.BeginRecording(rect);
  FML_DCHECK(success);
  container_layer_->AppendChild(current_layer_);
}

void PaintingContext::StopRecordingIfNeeded() {
  if (!IsRecording()) {
    return;
  }

  auto picture = graphics_context_.FinishRecording();
  if (!picture->IsEmpty()) {
    current_layer_->set_picture(std::move(picture));
  }
  current_layer_ = nullptr;
}

void PaintingContext::SetIsComplexHint() {
  FML_DCHECK(current_layer_);
  if (current_layer_) {
    current_layer_->SetIsComplexHint();
  }
}

void PaintingContext::SetWillChangeHint() {
  FML_DCHECK(current_layer_);
  if (current_layer_) {
    current_layer_->SetWillChangeHint();
  }
}

void PaintingContext::PaintChild(RenderObject* child,
                                 const FloatPoint& offset) {
  if (!child->Visible()) {
    return;
  }
  // If the child has its own composited layer, the child will be composited
  // into the layer subtree associated with this painting context. Otherwise,
  // the child will be painted into the current PendingPictureLayer for this
  // context
  if (!child->IsRepaintBoundary()) {
    child->PaintWithContext(*this, offset);
  } else {
    // In actual scenarios, the child may overlap with its own brother or
    // father's brother, or there may be other more complex situations.
    // Similar to how flutter handles overlay case, use different
    // |PendingPictureLayer| to record the remaining nodes.
    StopRecordingIfNeeded();
    CompositeChild(child, offset);
  }
}

void PaintingContext::CompositeChild(RenderObject* child,
                                     const FloatPoint& offset) {
  FML_DCHECK(child->IsRepaintBoundary());
  if (child->NeedsPaint() || child->NeedsEffect()) {
    RepaintCompositedChild(child, graphics_context_.GetUnrefQueue(), this);
  }
  FML_DCHECK(child->GetLayer());
  // Overlay will to add root layer to keep absolute position in page rather
  // add to parent's layer.
  if (!child->IsOverlay()) {
    AppendLayer(child->GetLayer());
  }

  PendingOffsetLayer* layer =
      static_cast<PendingOffsetLayer*>(child->GetLayer());
  if (layer) {
    if (!child->IsExternalView()) {
      layer->SetOffset(offset);
    } else {
      layer->SetOffset({});
    }
  }
}

void PaintingContext::AppendLayer(PendingLayer* layer) {
  layer->Remove();
  container_layer_->AppendChild(layer);
  if (strategy_ != CacheStrategy::None) {
    // When append a layer with CacheStrategy::None, do not recursively set in
    // case of descendant layers may using different strategy.
    layer->SetCacheStrategyRecursively(strategy_);
  }
}

void PaintingContext::AddLayer(PendingLayer* layer) { AppendLayer(layer); }

void PaintingContext::PushLayer(PendingContainerLayer* child_layer,
                                const PaintingContextCallback& painter,
                                const FloatPoint& offset) {
  // If a layer is being reused, it may already contain children. We remove
  // them so that `painter` can add children that are relevant for this frame.
  if (child_layer->HasChildren()) {
    child_layer->RemoveAllChildren();
  }
  AppendLayer(child_layer);
  PaintingContext child_context(child_layer, render_object(),
                                graphics_context_.GetUnrefQueue());
  child_context.SetCacheStrategy(strategy_);
  painter(child_context, offset);
}

void PaintingContext::PushOpacity(int alpha, const FloatPoint& offset,
                                  const PaintingContextCallback& painter) {
  PendingEffectLayer* opacity_layer =
      render_object()->GetOrCreateEffectLayer(EffectType::kOpacity);
  opacity_layer->SetOpacity(alpha);
  opacity_layer->SetOffset(offset);
  PushLayer(opacity_layer, painter, FloatPoint());
}

void PaintingContext::PushColorFilter(std::shared_ptr<ColorFilter> color_filter,
                                      const FloatPoint& offset,
                                      const PaintingContextCallback& painter) {
  PendingEffectLayer* effect_layer =
      render_object()->GetOrCreateEffectLayer(EffectType::kColorFilter);
  effect_layer->SetColorFilter(color_filter);
  PushLayer(effect_layer, painter, offset);
}

void PaintingContext::PushImageFilter(std::shared_ptr<ImageFilter> image_filter,
                                      const FloatPoint& offset,
                                      const PaintingContextCallback& painter) {
  PendingEffectLayer* effect_layer =
      render_object()->GetOrCreateEffectLayer(EffectType::kImageFilter);
  effect_layer->SetImageFilter(image_filter);
  PushLayer(effect_layer, painter, offset);
}

void PaintingContext::PushBackdropFilter(
    std::shared_ptr<ImageFilter> backdrop_filter, const FloatPoint& offset,
    const PaintingContextCallback& painter) {
  PendingEffectLayer* effect_layer =
      render_object()->GetOrCreateEffectLayer(EffectType::kBackdropFilter);
  effect_layer->SetBackdropFilter(backdrop_filter);

  PushLayer(effect_layer, painter, offset);
}

void PaintingContext::PushTransform(const TransformOperations& operations,
                                    const FloatPoint& origin,
                                    const float perspective,
                                    const FloatPoint& offset,
                                    const PaintingContextCallback& painter) {
  TransformOperations tmp = operations;
  tmp.SetPerspective(perspective);
  PendingTransformLayer* layer = new PendingTransformLayer(tmp, origin);
  AppendLayer(layer);
  PaintingContext child_context(layer, render_object(),
                                graphics_context_.GetUnrefQueue());
  child_context.SetCacheStrategy(strategy_);
  painter(child_context, offset);
}

void PaintingContext::PushClipRect(const FloatRect& clip_rect,
                                   const FloatPoint& offset,
                                   const PaintingContextCallback& painter) {
  PendingEffectLayer* effect_layer = new PendingEffectLayer();
  effect_layer->SetClipRect(clip_rect);
  PushLayer(effect_layer, painter, offset);
}

void PaintingContext::PushClipRRect(const FloatRoundedRect& clip_rrect,
                                    const FloatPoint& offset,
                                    const PaintingContextCallback& painter) {
  PendingEffectLayer* effect_layer = new PendingEffectLayer();
  effect_layer->SetClipRRect(clip_rrect);
  PushLayer(effect_layer, painter, offset);
}

void PaintingContext::PushClipPath(const GrPath& clip_path,
                                   const FloatPoint& offset,
                                   const PaintingContextCallback& painter) {
  PendingEffectLayer* effect_layer = new PendingEffectLayer();
  effect_layer->SetClipPath(clip_path);
  PushLayer(effect_layer, painter, offset);
}

void PaintingContext::PushShaderMask(std::shared_ptr<ColorSource> color_source,
                                     const FloatRect& mask_rect,
                                     BlendMode blend_mode,
                                     const FloatPoint& offset,
                                     const PaintingContextCallback& painter) {
  PendingEffectLayer* effect_layer = new PendingEffectLayer();
  effect_layer->SetShaderMask(std::move(color_source), mask_rect, blend_mode);
  PushLayer(effect_layer, painter, offset);
}

void PaintingContext::PushContainer(const FloatPoint& offset,
                                    const PaintingContextCallback& painter) {
  PendingContainerLayer* container_layer = new PendingContainerLayer();
  PushLayer(container_layer, painter, offset);
}

void PaintingContext::AddDrawableImage(int dx, int dy, int width, int height,
                                       int64_t image_id,
                                       clay::DrawableImage::FitMode fit_mode) {
  StopRecordingIfNeeded();
  PendingDrawableImageLayer* image_layer =
      new PendingDrawableImageLayer(dx, dy, width, height, image_id, fit_mode);
  AppendLayer(image_layer);
}

void PaintingContext::AddPlatformView(int dx, int dy, int width, int height,
                                      int64_t view_id) {
  StopRecordingIfNeeded();
  PendingPlatformViewLayer* platform_view_layer =
      new PendingPlatformViewLayer(dx, dy, width, height, view_id);
  AppendLayer(platform_view_layer);
}

void PaintingContext::AddPunchHole(const Rect& punch_hole_rect) {
  StopRecordingIfNeeded();
  PendingPunchHoleLayer* punch_hole_layer =
      new PendingPunchHoleLayer(punch_hole_rect);
  AppendLayer(punch_hole_layer);
}

void PaintingContext::PushExternalContainer(
    const ElementId& element_id, const PaintingContextCallback& painter,
    const skity::Vec2& size) {
  StopRecordingIfNeeded();
  PendingExternalViewLayer* external_view_layer =
      new PendingExternalViewLayer(element_id, size);
  PushLayer(external_view_layer, painter, {});
}

}  // namespace clay
