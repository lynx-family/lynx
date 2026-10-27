// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "clay/ui/common/isolate.h"
#include "clay/ui/compositing/pending_container_layer.h"
#include "clay/ui/painter/painting_context.h"
#include "clay/ui/rendering/render_container.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkCanvas.h"

namespace clay {

namespace {

fml::RefPtr<fml::TaskRunner> GetCurrentTaskRunner() {
  fml::MessageLoop::EnsureInitializedForCurrentThread();
  return fml::MessageLoop::GetCurrent().GetTaskRunner();
}

}  // namespace

namespace testing {

TEST(PaintingContextTest, Simple) {
  auto unref_queue = fml::MakeRefCounted<GPUUnrefQueue>(GetCurrentTaskRunner());
  auto render_object = std::make_unique<RenderContainer>();
  render_object->SetLeft(0);
  render_object->SetTop(0);
  render_object->SetWidth(100);
  render_object->SetHeight(100);
  auto root_layer = std::make_unique<PendingContainerLayer>();
  EXPECT_FALSE(root_layer->HasChildren());

  PaintingContext painting_context(root_layer.get(), render_object.get(),
                                   unref_queue);

  EXPECT_FALSE(painting_context.IsRecording());

  // Start recording.
  painting_context.GetGraphicsContext();
  EXPECT_TRUE(painting_context.IsRecording());
  EXPECT_TRUE(root_layer->HasChildren());

  painting_context.StopRecordingIfNeeded();
  EXPECT_FALSE(painting_context.IsRecording());
  root_layer = nullptr;
  unref_queue->Drain();
}

}  // namespace testing
}  // namespace clay
