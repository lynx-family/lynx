// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstddef>
#include <memory>

#include "base/include/fml/task_runner.h"
#include "clay/flow/layers/layer.h"
#include "clay/flow/layers/layer_tree.h"
#include "clay/flow/layers/transform_layer.h"
#include "clay/gfx/gpu_object.h"
#include "clay/gfx/picture.h"
#include "clay/testing/thread_test.h"
#include "clay/ui/compositing/frame_builder.h"
#include "clay/ui/compositing/pending_container_layer.h"
#include "clay/ui/compositing/pending_picture_layer.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkRect.h"

namespace clay {
namespace {

static sk_sp<SkPicture> MakeSizedPicture(int width, int height) {
  SkPictureRecorder recorder;
  SkCanvas* canvas = recorder.beginRecording(SkRect::MakeIWH(width, height));
  canvas->drawRect(SkRect::MakeXYWH(0, 0, width, height),
                   SkPaint(SkColor4f::FromColor(SK_ColorRED)));
  return recorder.finishRecordingAsPicture();
}

}  // namespace

class FrameBuilderTest : public clay::testing::ThreadTest {
 public:
  FrameBuilderTest()
      : queue_(fml::MakeRefCounted<GPUUnrefQueue>(GetCurrentTaskRunner())),
        frame_builder_(
            std::make_unique<FrameBuilder>(skity::Vec2{0, 0}, 1.0f, queue_)) {}
  ~FrameBuilderTest() {
    frame_builder_ = nullptr;
    queue_->Drain();
  }

  std::shared_ptr<clay::Layer> RootLayer() const {
    return frame_builder_->RootLayer();
  }
  std::unique_ptr<clay::LayerTree> TakeLayerTree() {
    return frame_builder_->TakeLayerTree();
  }
  void FinishBuild() { frame_builder_->FinishBuild(); }

  FrameBuilder* GetFrameBuilder() { return frame_builder_.get(); }

  std::unique_ptr<Picture> CreateNeutralPicture(sk_sp<SkPicture> picture) {
    clay::DynamicOps ops;
    auto picture_skia =
        fml::MakeRefCounted<clay::PictureSkia>(picture, std::move(ops));
    return std::make_unique<Picture>(
        GPUObject<clay::PictureSkia>(picture_skia, queue_));
  }

 private:
  fml::RefPtr<GPUUnrefQueue> queue_;
  std::unique_ptr<FrameBuilder> frame_builder_;
};

TEST_F(FrameBuilderTest, Simple) {
  EXPECT_TRUE(GetFrameBuilder()->RootLayer());
  EXPECT_FALSE(GetFrameBuilder()->TakeLayerTree());
}

TEST_F(FrameBuilderTest, BuildFrame) {
  GetFrameBuilder()->UpdateFrameSize(100, 100, 1.0f);

  // Construct a picture layer.
  std::unique_ptr<Picture> picture =
      CreateNeutralPicture(MakeSizedPicture(100, 100));
  std::unique_ptr<PendingPictureLayer> layer =
      std::make_unique<PendingPictureLayer>();
  layer->set_picture(std::move(picture));
  GetFrameBuilder()->AddPicture(0, 0, layer.get(), false, false);
  EXPECT_FALSE(GetFrameBuilder()->TakeLayerTree());

  FinishBuild();
  EXPECT_TRUE(GetFrameBuilder()->TakeLayerTree());
}

TEST_F(FrameBuilderTest, PushAndRetainLayerPreservesReplacementIdentity) {
  PendingContainerLayer pending_layer;
  auto old_layer =
      std::make_shared<TransformLayer>(skity::Matrix::Translate(10.0f, 20.0f));
  pending_layer.RetainLayer(old_layer);

  GetFrameBuilder()->PushStaticTransform(skity::Matrix::Translate(30.0f, 40.0f),
                                         &pending_layer);

  auto retained_layer = pending_layer.ReuseLayer();
  ASSERT_NE(retained_layer, nullptr);
  EXPECT_NE(retained_layer, old_layer);
  EXPECT_TRUE(retained_layer->IsReplacing(nullptr, old_layer.get()));
  auto root_layer = std::static_pointer_cast<ContainerLayer>(RootLayer());
  ASSERT_FALSE(root_layer->layers().empty());
  EXPECT_EQ(root_layer->layers().back(), retained_layer);
}

}  // namespace clay
