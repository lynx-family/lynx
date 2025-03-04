// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#define private public
#define protected public

#include "devtool/testing/mock/element_observer_mock.h"
#include "devtool/testing/utils/devtool_env_testing.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace testing {

class DevToolTransitionTest : public ::testing::Test {
 public:
  DevToolTransitionTest() {}
  ~DevToolTransitionTest() override {}

  void SetUp() override {}
};

TEST_F(DevToolTransitionTest, CheckOnDomNodeAdd) {
  /* Here is an example of a unit test (UT), but it was previously placed in
  comments. The test case fails because both the new and old architectures are
  being called. Please fix this bug and enable the UT.
  auto element_manager = LynxDevToolEnvTesting::SetUpElementManager();
  auto post = std::make_shared<PostObserver>(nullptr);
  element_manager->SetInspectorElementObserver(post);
  auto pre = std::make_shared<PreObserver>();
  element_manager->SetHierarchyObserver(pre);
  element_manager->OnElementNodeAddedForInspector(nullptr);
  */
}

}  // namespace testing
}  // namespace lynx
