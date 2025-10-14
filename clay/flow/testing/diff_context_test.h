// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CLAY_FLOW_TESTING_DIFF_CONTEXT_TEST_H_
#define CLAY_FLOW_TESTING_DIFF_CONTEXT_TEST_H_

#include <memory>

#include "clay/flow/layers/container_layer.h"
#include "clay/flow/layers/opacity_layer.h"
#include "clay/flow/layers/picture_layer.h"
#include "clay/flow/testing/gpu_object_layer_test.h"
#include "third_party/skia/include/core/SkPicture.h"
#include "third_party/skia/include/core/SkPictureRecorder.h"

namespace clay {
namespace testing {

class MockLayerTree {
 public:
  explicit MockLayerTree(skity::Vec2 size = {1000, 1000})
      : root_(std::make_shared<ContainerLayer>()), size_(size) {}

  ContainerLayer* root() { return root_.get(); }
  const ContainerLayer* root() const { return root_.get(); }

  PaintRegionMap& paint_region_map() { return paint_region_map_; }
  const PaintRegionMap& paint_region_map() const { return paint_region_map_; }

  const skity::Vec2& size() const { return size_; }

 private:
  std::shared_ptr<ContainerLayer> root_;
  PaintRegionMap paint_region_map_;
  skity::Vec2 size_;
};

class DiffContextTest : public ThreadTest {
 public:
  DiffContextTest();
  ~DiffContextTest() override;

  Damage DiffLayerTree(
      MockLayerTree& layer_tree, const MockLayerTree& old_layer_tree,
      const skity::Rect& additional_damage = skity::Rect::MakeEmpty(),
      int horizontal_clip_alignment = 0, int vertical_alignment = 0,
      bool use_raster_cache = true);

  // Create SkPicture consisting of filled rect with given color; Being able
  // to specify different color is useful to test deep comparison of pictures
  sk_sp<SkPicture> CreatePicture(const skity::Rect& bounds, uint32_t color);

  std::shared_ptr<PictureLayer> CreatePictureLayer(
      sk_sp<SkPicture> picture, const skity::Vec2& offset = skity::Vec2(0, 0));

  std::shared_ptr<ContainerLayer> CreateContainerLayer(
      std::initializer_list<std::shared_ptr<Layer>> layers);

  std::shared_ptr<ContainerLayer> CreateContainerLayer(
      std::shared_ptr<Layer> l) {
    return CreateContainerLayer({l});
  }

  std::shared_ptr<OpacityLayer> CreateOpacityLater(
      std::initializer_list<std::shared_ptr<Layer>> layers, SkAlpha alpha,
      const skity::Vec2& offset = skity::Vec2(0, 0));

  fml::RefPtr<GPUUnrefQueue> unref_queue() { return unref_queue_; }

 private:
  fml::RefPtr<GPUUnrefQueue> unref_queue_;
};

}  // namespace testing
}  // namespace clay

#endif  // CLAY_FLOW_TESTING_DIFF_CONTEXT_TEST_H_
