// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/testing/diff_context_test.h"

#include <utility>

#include "clay/gfx/skia/picture_skia.h"
#include "clay/gfx/skity_to_skia_utils.h"

namespace clay {
namespace testing {

using clay::GPUObject;
using clay::GPUUnrefQueue;

DiffContextTest::DiffContextTest()
    : unref_queue_(fml::MakeRefCounted<GPUUnrefQueue>(GetCurrentTaskRunner())) {
}

DiffContextTest::~DiffContextTest() { unref_queue_->Drain(); }

Damage DiffContextTest::DiffLayerTree(MockLayerTree& layer_tree,
                                      const MockLayerTree& old_layer_tree,
                                      const skity::Rect& additional_damage,
                                      int horizontal_clip_alignment,
                                      int vertical_clip_alignment,
                                      bool use_raster_cache) {
  FML_CHECK(layer_tree.size() == old_layer_tree.size());

  DiffContext dc(layer_tree.size(), 1, layer_tree.paint_region_map(),
                 old_layer_tree.paint_region_map(), use_raster_cache);
  dc.PushCullRect(
      skity::Rect::MakeWH(layer_tree.size().x, layer_tree.size().y));
  layer_tree.root()->Diff(&dc, old_layer_tree.root());
  return dc.ComputeDamage(additional_damage, horizontal_clip_alignment,
                          vertical_clip_alignment);
}

sk_sp<SkPicture> DiffContextTest::CreatePicture(const skity::Rect& bounds,
                                                SkColor color) {
  SkPictureRecorder recorder;
  auto bbh_factory = std::make_unique<SkRTreeFactory>();
  SkCanvas* canvas = recorder.beginRecording(
      clay::ConvertSkityRectToSkRect(bounds), bbh_factory.get());
  SkPaint paint;
  paint.setColor(color);
  canvas->drawRect(clay::ConvertSkityRectToSkRect(bounds), paint);
  return recorder.finishRecordingAsPicture();
}

std::shared_ptr<PictureLayer> DiffContextTest::CreatePictureLayer(
    sk_sp<SkPicture> picture, const skity::Vec2& offset) {
  clay::DynamicOps ops;
  auto picture_skia =
      fml::MakeRefCounted<clay::PictureSkia>(picture, std::move(ops));
  return std::make_shared<PictureLayer>(
      offset, GPUObject(std::move(picture_skia), unref_queue()), false, false);
}

std::shared_ptr<ContainerLayer> DiffContextTest::CreateContainerLayer(
    std::initializer_list<std::shared_ptr<Layer>> layers) {
  auto res = std::make_shared<ContainerLayer>();
  for (const auto& l : layers) {
    res->Add(l);
  }
  return res;
}

std::shared_ptr<OpacityLayer> DiffContextTest::CreateOpacityLater(
    std::initializer_list<std::shared_ptr<Layer>> layers, SkAlpha alpha,
    const skity::Vec2& offset) {
  auto res = std::make_shared<OpacityLayer>(alpha, offset);
  for (const auto& l : layers) {
    res->Add(l);
  }
  return res;
}

}  // namespace testing
}  // namespace clay
