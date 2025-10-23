// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "build/build_config.h"
#include "clay/flow/flow_test_utils.h"
#include "clay/fml/backtrace.h"
#include "clay/fml/command_line.h"
#include "clay/fml/logging.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

int main(int argc, char** argv) {
  fml::InstallCrashHandler();
  testing::InitGoogleTest(&argc, argv);
  fml::CommandLine cmd = fml::CommandLineFromPlatformOrArgcArgv(argc, argv);

#if defined(OS_FUCHSIA)
  clay::SetGoldenDir(cmd.GetOptionValueWithDefault(
      "golden-dir", "/pkg/data/clay/testing/resources"));
#else
  clay::SetGoldenDir(cmd.GetOptionValueWithDefault(
      "golden-dir", "lynx/clay/testing/resources"));
#endif
  clay::SetFontFile(cmd.GetOptionValueWithDefault(
      "font-file",
      "lynx/clay/third_party/txt/third_party/fonts/Roboto-Regular.ttf"));
  return RUN_ALL_TESTS();
}
