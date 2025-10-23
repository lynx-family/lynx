// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef TESTING_FIXTURE_TEST_H_
#define TESTING_FIXTURE_TEST_H_

#include <string>

#include "clay/common/settings.h"
#include "clay/testing/thread_test.h"

namespace clay {
namespace testing {

class FixtureTest : public ThreadTest {
 public:
  // Uses the default filenames from the fixtures generator.
  FixtureTest();

  virtual Settings CreateSettingsForFixture();

 private:
  BASE_DISALLOW_COPY_AND_ASSIGN(FixtureTest);
};

}  // namespace testing
}  // namespace clay

#endif  // TESTING_FIXTURE_TEST_H_
