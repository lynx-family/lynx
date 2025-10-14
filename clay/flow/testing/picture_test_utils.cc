// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/testing/picture_test_utils.h"

#include "clay/gfx/style/blend_mode.h"
#include "clay/gfx/testing_utils.h"

namespace clay {
namespace testing {

sk_sp<SkPicture> GetSamplePicture() {
  SkPictureRecorder recorder;
  SkCanvas* canvas = recorder.beginRecording(SkRect::MakeWH(150, 100));
  SkPaint paint;
  paint.setColor(SK_ColorRED);
  canvas->drawRect(SkRect::MakeXYWH(10, 10, 80, 80), paint);
  return recorder.finishRecordingAsPicture();
}

sk_sp<SkPicture> GetSampleNestedPicture() {
  SkPictureRecorder recorder;
  SkCanvas* canvas = recorder.beginRecording(SkRect::MakeWH(150, 100));
  for (int y = 10; y <= 60; y += 10) {
    for (int x = 10; x <= 60; x += 10) {
      SkPaint paint;
      paint.setColor(((x + y) % 20) == 10 ? SK_ColorRED : SK_ColorBLUE);
      canvas->drawRect(SkRect::MakeXYWH(x, y, 80, 80), paint);
    }
  }
  auto picture = recorder.finishRecordingAsPicture();
  SkPictureRecorder outer_recorder;
  canvas = outer_recorder.beginRecording(SkRect::MakeWH(150, 100));
  canvas->drawPicture(picture);
  return outer_recorder.finishRecordingAsPicture();
}

sk_sp<SkPicture> GetSamplePicture(int ops) {
  SkPictureRecorder recorder;
  SkCanvas* canvas = recorder.beginRecording(SkRect::MakeWH(150, 100));
  for (int i = 0; i < ops; i++) {
    canvas->drawColor(SK_ColorRED, SkBlendMode::kSrc);
  }
  return recorder.finishRecordingAsPicture();
}

}  // namespace testing
}  // namespace clay
