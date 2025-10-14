// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RENDERING_RENDERER_H_
#define CLAY_UI_RENDERING_RENDERER_H_

#include <list>
#include <unordered_set>

#include "clay/gfx/pixel_helper.h"
#include "clay/ui/compositing/pending_container_layer.h"

namespace clay {

class RenderObject;

enum class RenderPhase {
  kIdle,
  kLayout,
  kPaint,
  kBuildFrame,
};

class RendererClient {
 public:
  virtual void RequestNewFrame() = 0;
  virtual RenderPhase GetRenderPhase() const = 0;
  virtual fml::RefPtr<PaintImage> MakeRasterSnapshot(GrPicturePtr picture,
                                                     skity::Vec2 size) = 0;
  virtual void RegisterUploadTask(OneShotCallback<>&& task, int image_id) = 0;

 protected:
  virtual ~RendererClient() = default;
};

// The renderer manages the rendering pipeline.
//
// The renderer provides an interface for driving the rendering pipeline and
// stores the state about which render objects have requested to be visited
// in each stage of the pipeline.
class Renderer : public PixelHelper<kPixelTypeClay> {
 public:
  explicit Renderer(RendererClient* client,
                    fml::RefPtr<GPUUnrefQueue> unref_queue);
  ~Renderer();

  void SetRoot(RenderObject* root);
  void SetFrameSize(Size size) { frame_size_ = size; }
  Size GetFrameSize() { return frame_size_; }

  void Paint();
  void RequestPaint();

  void AddNeedPaint(RenderObject* node) {
    nodes_needing_paint_.push_back(node);
  }
  void AddOverlayChild(RenderObject* node);
  bool Contains(RenderObject* node) const;

  PendingContainerLayer* GetLayer() const;

  void SetPixelRatio(float pixel_ratio) { pixel_ratio_ = pixel_ratio; }

  float DevicePixelRatio() const override { return pixel_ratio_; }

  void SetDPI(float dpi) { dpi_ = dpi; }

  float DPI() const { return dpi_; }

  void RemoveDirtyNode(RenderObject* node);

  bool HasDirtyNodes() const { return !nodes_needing_paint_.empty(); }

  RendererClient* renderer_client() const { return client_; }

 private:
  // Append all layers of overlay children to root layer.
  void AddOverlayToRootLayer();

  // The unique object managed by the renderer that has no parent.
  RenderObject* root_ = nullptr;

  RendererClient* client_;
  fml::RefPtr<GPUUnrefQueue> unref_queue_;
  float pixel_ratio_ = 1.0f;
  float dpi_ = 90.0f;
  std::list<RenderObject*> nodes_needing_paint_;
  std::list<RenderObject*> overlay_children_;
  Size frame_size_;
};

}  // namespace clay

#endif  // CLAY_UI_RENDERING_RENDERER_H_
