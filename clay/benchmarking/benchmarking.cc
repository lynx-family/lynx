// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/benchmarking/benchmarking.h"

#include <string>

#include "build/build_config.h"
#include "clay/fml/backtrace.h"
#include "clay/fml/command_line.h"
#include "clay/fml/icu_util.h"

namespace benchmarking {

int Main(int argc, char** argv) {
  fml::InstallCrashHandler();
#if !defined(OS_ANDROID)
  fml::CommandLine cmd = fml::CommandLineFromPlatformOrArgcArgv(argc, argv);
  std::string icudtl_path =
      cmd.GetOptionValueWithDefault("icu-data-file-path", "icudtl.dat");
  fml::icu::InitializeICU(icudtl_path);
#endif
  benchmark::Initialize(&argc, argv);
  ::benchmark::RunSpecifiedBenchmarks();
  return 0;
}

}  // namespace benchmarking

int main(int argc, char** argv) { return benchmarking::Main(argc, argv); }
