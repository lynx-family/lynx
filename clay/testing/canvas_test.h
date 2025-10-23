// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef TESTING_CANVAS_TEST_H_
#define TESTING_CANVAS_TEST_H_

#include "base/include/fml/macros.h"
#include "clay/testing/mock_canvas.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

// This fixture allows creating tests that make use of a mock |SkCanvas|.
template <typename BaseT>
class CanvasTestBase : public BaseT {
 public:
  CanvasTestBase() = default;

  MockCanvas& mock_canvas() { return canvas_; }
  SkColorSpace* mock_color_space() { return canvas_.imageInfo().colorSpace(); }

 private:
  MockCanvas canvas_;

  BASE_DISALLOW_COPY_AND_ASSIGN(CanvasTestBase);
};
using CanvasTest = CanvasTestBase<::testing::Test>;

}  // namespace testing
}  // namespace clay

#endif  // TESTING_CANVAS_TEST_H_
