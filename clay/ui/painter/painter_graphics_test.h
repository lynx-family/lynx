// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_PAINTER_PAINTER_GRAPHICS_TEST_H_
#define CLAY_UI_PAINTER_PAINTER_GRAPHICS_TEST_H_

#include "clay/gfx/graphics_context.h"

namespace clay {
namespace testing {
class MockCanvas;
}
}  // namespace clay

namespace clay {

class PainterGraphicsTest {
 public:
  PainterGraphicsTest();
  ~PainterGraphicsTest();
  GraphicsContext& mock_context() { return context_; }
  clay::testing::MockCanvas& mock_canvas();

 private:
  fml::RefPtr<GPUUnrefQueue> unref_queue_;
  GraphicsContext context_;
  clay::testing::MockCanvas* mock_canvas_;
  BASE_DISALLOW_COPY_AND_ASSIGN(PainterGraphicsTest);
};

}  // namespace clay

#endif  // CLAY_UI_PAINTER_PAINTER_GRAPHICS_TEST_H_
