// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <iostream>
#include <optional>
#include <string>

#include "build/build_config.h"
#include "clay/fml/backtrace.h"
#include "clay/fml/command_line.h"
#include "clay/testing/debugger_detection.h"
#include "clay/testing/test_timeout_listener.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

#ifdef OS_IOS
#include <asl.h>
#endif  // OS_IOS

std::optional<fml::TimeDelta> GetTestTimeoutFromArgs(int argc, char** argv) {
  const auto command_line = fml::CommandLineFromPlatformOrArgcArgv(argc, argv);

  std::string timeout_seconds;
  if (!command_line.GetOptionValue("timeout", &timeout_seconds)) {
    // No timeout specified. Default to 120s.
    return fml::TimeDelta::FromSeconds(120u);
  }

  const auto seconds = std::stoi(timeout_seconds);

  if (seconds < 1) {
    return std::nullopt;
  }

  return fml::TimeDelta::FromSeconds(seconds);
}

int main(int argc, char** argv) {
  fml::InstallCrashHandler();
#ifdef OS_IOS
  asl_log_descriptor(NULL, NULL, ASL_LEVEL_NOTICE, STDOUT_FILENO,
                     ASL_LOG_DESCRIPTOR_WRITE);
  asl_log_descriptor(NULL, NULL, ASL_LEVEL_ERR, STDERR_FILENO,
                     ASL_LOG_DESCRIPTOR_WRITE);
#endif  // OS_IOS

  ::testing::InitGoogleTest(&argc, argv);

  // Check if the user has specified a timeout.
  const auto timeout = GetTestTimeoutFromArgs(argc, argv);
  if (!timeout.has_value()) {
    FML_LOG(INFO) << "Timeouts disabled via a command line flag.";
    return RUN_ALL_TESTS();
  }

  // Check if the user is debugging the process.
  if (clay::testing::GetDebuggerStatus() ==
      clay::testing::DebuggerStatus::kAttached) {
    FML_LOG(INFO) << "Debugger is attached. Suspending test timeouts.";
    return RUN_ALL_TESTS();
  }

  auto timeout_listener =
      new clay::testing::TestTimeoutListener(timeout.value());
  auto& listeners = ::testing::UnitTest::GetInstance()->listeners();
  listeners.Append(timeout_listener);
  fml::MessageLoop::EnsureInitializedForCurrentThread();
  auto result = RUN_ALL_TESTS();
  delete listeners.Release(timeout_listener);
  return result;
}
