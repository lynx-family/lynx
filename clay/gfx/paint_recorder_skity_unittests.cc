// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/message_loop.h"
#include "clay/gfx/graphics_canvas.h"
#include "clay/gfx/paint_recorder.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

TEST(PaintRecorderSkityTest, BuildsDisplayListRTree) {
  fml::MessageLoop::EnsureInitializedForCurrentThread();
  auto unref_queue = fml::MakeRefCounted<GPUUnrefQueue>(
      fml::MessageLoop::GetCurrent().GetTaskRunner());
  PaintRecorder recorder(unref_queue);
  GraphicsCanvas* canvas =
      recorder.BeginRecording(skity::Rect::MakeWH(200.0f, 200.0f));
  ASSERT_NE(canvas, nullptr);

  Paint paint;
  canvas->DrawRect(skity::Rect::MakeLTRB(10.0f, 10.0f, 40.0f, 40.0f), paint);
  canvas->DrawRect(skity::Rect::MakeLTRB(120.0f, 120.0f, 160.0f, 160.0f),
                   paint);

  auto picture = recorder.FinishRecordingAsPicture();
  ASSERT_NE(picture, nullptr);
  auto display_list = picture->picture()->raw();
  ASSERT_NE(display_list, nullptr);

  EXPECT_EQ(
      display_list->Search(skity::Rect::MakeLTRB(20.0f, 20.0f, 30.0f, 30.0f))
          .size(),
      1u);
  EXPECT_TRUE(
      display_list->Search(skity::Rect::MakeLTRB(70.0f, 70.0f, 80.0f, 80.0f))
          .empty());

  display_list.reset();
  picture.reset();
  unref_queue->Drain();
}

}  // namespace testing
}  // namespace clay
