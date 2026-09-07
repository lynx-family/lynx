// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_TESTING_MOCK_ANIMATION_MANAGER_H_
#define CORE_LIST_TESTING_MOCK_ANIMATION_MANAGER_H_

#include "core/list/animation/animation_manager.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"

namespace lynx {
namespace list {

class MockAnimationManager : public AnimationManager {
 public:
  MockAnimationManager() = default;
  ~MockAnimationManager() override = default;

  MOCK_METHOD(void, SetUpdateAnimationConfig,
              (const UpdateAnimationConfig& config), (override));
  MOCK_METHOD(void, BeforeDataUpdate,
              (bool has_valid_diff, bool has_expected_diff_animation,
               bool has_completed_first_layout),
              (override));
  MOCK_METHOD(void, BeforeLayout, (), (override));
  MOCK_METHOD(void, AfterLayoutBeforeFlush, (), (override));
  MOCK_METHOD(void, AfterFlush, (), (override));
  MOCK_METHOD(void, CancelAnimationTransaction, (AnimationCancelReason reason),
              (override));
  MOCK_METHOD(void, Destroy, (), (override));
};

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_TESTING_MOCK_ANIMATION_MANAGER_H_
