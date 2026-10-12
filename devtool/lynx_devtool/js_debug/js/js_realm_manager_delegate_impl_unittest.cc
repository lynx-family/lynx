// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/js_debug/js/js_realm_manager_delegate_impl.h"

#include <string>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

TEST(JSRealmManagerDelegateImplTest,
     RepeatedRegistrationKeepsOriginalEngineUntilRelease) {
  JSRealmManagerDelegateImpl delegate;
  int quickjs_release_count = 0;
  int v8_release_count = 0;
  std::string released_group;
  delegate.SetReleaseContextCallback(runtime::js::JSRuntimeType::quickjs,
                                     [&](const std::string& group_id) {
                                       ++quickjs_release_count;
                                       released_group = group_id;
                                     });
  delegate.SetReleaseContextCallback(
      runtime::js::JSRuntimeType::v8,
      [&](const std::string&) { ++v8_release_count; });

  delegate.EnsureSharedContextRegistered("shared_group",
                                         runtime::js::JSRuntimeType::quickjs);
  delegate.EnsureSharedContextRegistered("shared_group",
                                         runtime::js::JSRuntimeType::v8);
  delegate.OnRelease("shared_group");

  EXPECT_EQ(quickjs_release_count, 1);
  EXPECT_EQ(v8_release_count, 0);
  EXPECT_EQ(released_group, "shared_group");
}

TEST(JSRealmManagerDelegateImplTest, ReleasedGroupCanBeRegisteredAgain) {
  JSRealmManagerDelegateImpl delegate;
  int quickjs_release_count = 0;
  int v8_release_count = 0;
  delegate.SetReleaseContextCallback(
      runtime::js::JSRuntimeType::quickjs,
      [&](const std::string&) { ++quickjs_release_count; });
  delegate.SetReleaseContextCallback(
      runtime::js::JSRuntimeType::v8,
      [&](const std::string&) { ++v8_release_count; });

  delegate.EnsureSharedContextRegistered("shared_group",
                                         runtime::js::JSRuntimeType::quickjs);
  delegate.OnRelease("shared_group");
  // Releasing an already released group should be a no-op.
  delegate.OnRelease("shared_group");
  delegate.EnsureSharedContextRegistered("shared_group",
                                         runtime::js::JSRuntimeType::v8);
  delegate.OnRelease("shared_group");

  EXPECT_EQ(quickjs_release_count, 1);
  EXPECT_EQ(v8_release_count, 1);
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
