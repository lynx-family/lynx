// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPOSITING_TESTING_MOCK_PENDING_LAYER_H_
#define CLAY_UI_COMPOSITING_TESTING_MOCK_PENDING_LAYER_H_

#include "clay/gfx/geometry/float_point.h"
#include "clay/ui/compositing/frame_builder.h"
#include "clay/ui/compositing/pending_layer.h"

namespace clay {
namespace testing {

// Mock implementation of the |PendingLayer| interface that does nothing.
class MockPendingLayer : public PendingLayer {
 public:
  MockPendingLayer() {}
  ~MockPendingLayer() override {}

  void AddToFrame(FrameBuilder* builder, const FloatPoint& offset) override {}
};

}  // namespace testing
}  // namespace clay

#endif  // CLAY_UI_COMPOSITING_TESTING_MOCK_PENDING_LAYER_H_
