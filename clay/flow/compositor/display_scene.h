// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_COMPOSITOR_DISPLAY_SCENE_H_
#define CLAY_FLOW_COMPOSITOR_DISPLAY_SCENE_H_

#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <vector>

#include "clay/flow/embedded_views.h"

namespace clay {
class Layer;
class DrawableImageLayer;
struct PaintContext;

// Immutable paint order and leaf state, independent of the source LayerTree.
// Static recordings are rasterized once; external images are resolved at each
// display frame using the existing DrawableImage registry and synchronization.
class DisplayScene : public std::enable_shared_from_this<DisplayScene> {
 public:
  using Rasterize =
      std::function<GrImagePtr(EmbedderViewSlice&, const skity::Rect&)>;

  static bool CanRecord(const Layer& root);
  DisplayScene(const skity::Rect& bounds, const skity::Matrix& root_transform);
  ~DisplayScene();

  void Begin(PaintContext& context);
  void RecordImage(const DrawableImageLayer& layer, PaintContext& context);
  void Finish(PaintContext& context);
  void RecordGroupInput(PaintContext& context, const skity::Rect& bounds,
                        const std::function<void(PaintContext&)>& paint);
  bool Prepare(const Rasterize& rasterize);
  void Paint(GrCanvas* canvas, GrContext* context,
             std::optional<int64_t> slice = std::nullopt) const;
  std::optional<int64_t> SwitchToView(PaintContext& context,
                                      std::optional<int64_t> view_id);
  std::list<skity::Rect> SearchSlice(int64_t view_id,
                                     const skity::Rect& query) const;
  bool ContainsImage(int64_t image_id) const;

 private:
  using Draw = std::function<void(GrCanvas*, GrContext*, const skity::Matrix&)>;

  struct Entry {
    void Paint(GrCanvas* canvas, GrContext* context,
               const skity::Matrix& base) const;

    std::unique_ptr<EmbedderViewSlice> recording;
    GrImagePtr image;
    skity::Rect bounds;
    Draw draw;
    bool can_rasterize = false;
    std::optional<int64_t> update_image_id;
    std::optional<int64_t> slice;
  };
  void StartRecording(PaintContext& context);
  void FinishRecording(PaintContext& context);
  void RecordGroup(PaintContext& context, const skity::Rect* bounds,
                   const GrPaint* paint,
                   const std::shared_ptr<const ImageFilter>& backdrop);

  struct Group {
    Draw resume;
    skity::Rect bounds;
  };
  skity::Rect bounds_;
  skity::Matrix root_transform_;
  skity::Matrix inverse_root_;
  std::optional<int64_t> current_slice_;
  std::vector<Group> groups_;
  std::unique_ptr<EmbedderViewSlice> recording_;
  std::vector<Entry> entries_;
  bool prepared_ = false;
};

}  // namespace clay
#endif  // CLAY_FLOW_COMPOSITOR_DISPLAY_SCENE_H_
