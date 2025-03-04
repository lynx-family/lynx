// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_TESTING_UTILS_METHOD_TRACKER_H_
#define DEVTOOL_TESTING_UTILS_METHOD_TRACKER_H_

#include <set>

namespace lynx {
namespace testing {

class MethodTracker {
 public:
  enum class MethodName {
    ON_EVENT_NODE_ADDED,

  };

  static MethodTracker& GetInstance();

  bool CallMethod(const MethodName& method, bool is_post_refactor);

  void CheckMethodFrom(const MethodName& method, bool from_post_refactor);

  std::set<MethodName> pre_refactor_method_;
  std::set<MethodName> post_refactor_method_;
};

}  // namespace testing
}  // namespace lynx

#endif  // DEVTOOL_TESTING_UTILS_METHOD_TRACKER_H_
