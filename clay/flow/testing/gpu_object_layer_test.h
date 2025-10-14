// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_TESTING_GPU_OBJECT_LAYER_TEST_H_
#define CLAY_FLOW_TESTING_GPU_OBJECT_LAYER_TEST_H_

#include "clay/flow/testing/layer_test.h"
#include "clay/gfx/gpu_object.h"
#include "clay/testing/thread_test.h"

namespace clay {
namespace testing {

using clay::GPUUnrefQueue;

// This fixture allows generating tests that create |GPUObject|'s which
// are destroyed on a |GPUUnrefQueue|.
class GPUObjectLayerTest : public LayerTestBase<ThreadTest> {
 public:
  GPUObjectLayerTest();
  ~GPUObjectLayerTest() override;

  fml::RefPtr<GPUUnrefQueue> unref_queue() { return unref_queue_; }

 private:
  fml::RefPtr<GPUUnrefQueue> unref_queue_;
};

}  // namespace testing
}  // namespace clay

#endif  // CLAY_FLOW_TESTING_GPU_OBJECT_LAYER_TEST_H_
