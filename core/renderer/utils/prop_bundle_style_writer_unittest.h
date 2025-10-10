// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UTILS_PROP_BUNDLE_STYLE_WRITER_UNITTEST_H_
#define CORE_RENDERER_UTILS_PROP_BUNDLE_STYLE_WRITER_UNITTEST_H_

#include "core/renderer/ui_wrapper/common/testing/prop_bundle_mock.h"
#include "core/renderer/utils/prop_bundle_style_writer.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {
class PropBundleStyleWriterTest : public ::testing::Test {
 public:
  PropBundleStyleWriterTest() : style_(1, 1) {}
  ~PropBundleStyleWriterTest() = default;

  void SetUp() override {}
  void TearDown() override {}

 protected:
  PropBundleMock new_bundle_;
  PropBundleMock old_bundle_;
  PropBundleMock bitset_bundle_;

  starlight::ComputedCSSStyle style_;
};
}  // namespace test
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UTILS_PROP_BUNDLE_STYLE_WRITER_UNITTEST_H_
