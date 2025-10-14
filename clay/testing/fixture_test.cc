// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/testing/fixture_test.h"

#include <utility>

namespace clay {
namespace testing {

FixtureTest::FixtureTest() {}

Settings FixtureTest::CreateSettingsForFixture() {
  Settings settings;
  return settings;
}

}  // namespace testing
}  // namespace clay
