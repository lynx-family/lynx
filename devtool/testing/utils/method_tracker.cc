// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/testing/utils/method_tracker.h"

#include "base/include/no_destructor.h"
#include "gtest/gtest.h"

namespace lynx {
namespace testing {

MethodTracker& MethodTracker::GetInstance() {
  static lynx::base::NoDestructor<MethodTracker> instance;
  return *instance;
}

bool MethodTracker::CallMethod(const MethodName& method,
                               bool is_post_refactor) {
  if (is_post_refactor) {
    post_refactor_method_.insert(method);
  } else {
    pre_refactor_method_.insert(method);
  }
  auto find_in_post =
      post_refactor_method_.find(method) != post_refactor_method_.end();
  auto find_in_pre =
      pre_refactor_method_.find(method) != pre_refactor_method_.end();
  EXPECT_FALSE(find_in_post && find_in_pre);
  return true;
}

void MethodTracker::CheckMethodFrom(const MethodName& method,
                                    bool from_post_refactor) {
  std::set<MethodName> method_set;
  if (from_post_refactor) {
    method_set = post_refactor_method_;
  } else {
    method_set = pre_refactor_method_;
  }
  EXPECT_TRUE(method_set.find(method) != method_set.end());
}

}  // namespace testing
}  // namespace lynx
