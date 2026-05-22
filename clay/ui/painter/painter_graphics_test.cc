// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/painter/painter_graphics_test.h"

#include "clay/testing/mock_canvas.h"
#include "clay/testing/thread_test.h"

namespace clay {

namespace {

fml::RefPtr<fml::TaskRunner> GetCurrentTaskRunner() {
  fml::MessageLoop::EnsureInitializedForCurrentThread();
  return fml::MessageLoop::GetCurrent().GetTaskRunner();
}

}  // namespace

PainterGraphicsTest::PainterGraphicsTest()
    : unref_queue_(fml::MakeRefCounted<GPUUnrefQueue>(GetCurrentTaskRunner())),
      context_(unref_queue_) {
  context_.BeginRecording(skity::Rect::MakeWH(64, 64));
  mock_canvas_ = new clay::testing::MockCanvas();
}

PainterGraphicsTest::~PainterGraphicsTest() {
  unref_queue_->Drain();
  delete mock_canvas_;
}

clay::testing::MockCanvas& PainterGraphicsTest::mock_canvas() {
  return *mock_canvas_;
}

}  // namespace clay
