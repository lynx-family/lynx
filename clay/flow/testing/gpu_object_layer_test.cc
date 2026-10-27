// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/testing/gpu_object_layer_test.h"

#include "base/include/fml/time/time_delta.h"

namespace clay {
namespace testing {

GPUObjectLayerTest::GPUObjectLayerTest()
    : unref_queue_(fml::MakeRefCounted<GPUUnrefQueue>(GetCurrentTaskRunner())) {
}

GPUObjectLayerTest::~GPUObjectLayerTest() { unref_queue_->Drain(); }

}  // namespace testing
}  // namespace clay
