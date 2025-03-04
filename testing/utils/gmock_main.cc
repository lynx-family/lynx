// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <cstdio>

#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

void CustomInit(int argc, char** argv);

void InitListener(int argc, char** argv) {}

void Init(int argc, char** argv) {
  testing::InitGoogleMock(&argc, argv);
  CustomInit(argc, argv);
  InitListener(argc, argv);
}

GTEST_API_ int main(int argc, char** argv) {
  Init(argc, argv);
  printf("Running main() from Lynx %s\n", __FILE__);
  int result = RUN_ALL_TESTS();
  return result;
}
