// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_COMMON_ANDROID_PROP_BUNDLE_ANDROID_TEST_H_
#define CORE_RENDERER_UI_WRAPPER_COMMON_ANDROID_PROP_BUNDLE_ANDROID_TEST_H_

#include "core/renderer/ui_wrapper/common/android/prop_bundle_android.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

const bool prop_bundle_test_params[] = {true, false};

class PropBundleAndroidTest : public ::testing::TestWithParam<bool> {
 public:
  PropBundleAndroidTest() { enable_map_buffer_ = GetParam(); };
  ~PropBundleAndroidTest() = default;

 protected:
  void SetUp() override;
  void TearDown() override;

  bool enable_map_buffer_;
  PropBundleCreatorAndroid prop_bundle_creator_;
  fml::RefPtr<PropBundleAndroid> prop_bundle_android_;
};
}  // namespace testing
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_COMMON_ANDROID_PROP_BUNDLE_ANDROID_TEST_H_
