// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/windows/flutter_project_bundle.h"

#include <filesystem>
#include <sstream>

#include "clay/fml/logging.h"
#include "clay/shell/common/switches.h"
#include "clay/shell/platform/common/engine_switches.h"  // nogncheck
#include "clay/shell/platform/common/path_utils.h"

namespace clay {

FlutterProjectBundle::FlutterProjectBundle(
    const FlutterDesktopEngineProperties& properties)
    : icu_path_(properties.icu_data_path),
      enable_software_rendering_(properties.enable_software_rendering) {
  // Resolve any relative paths.
  if (icu_path_.is_relative()) {
    std::filesystem::path executable_location = GetExecutableDirectory();
    if (executable_location.empty()) {
      FML_LOG(ERROR)
          << "Unable to find executable location to resolve resource paths.";
      icu_path_ = std::filesystem::path();
    } else {
      icu_path_ = std::filesystem::path(executable_location) / icu_path_;
    }
  }
}

bool FlutterProjectBundle::HasValidPaths() { return !icu_path_.empty(); }

FlutterProjectBundle::~FlutterProjectBundle() {}

void FlutterProjectBundle::SetSwitches(
    const std::vector<std::string>& switches) {
  engine_switches_ = switches;
}

const std::vector<std::string> FlutterProjectBundle::GetSwitches() {
  std::vector<std::string> switches = GetSwitchesFromEnvironment();
  if (enable_software_rendering_) {
    std::ostringstream switch_value_as_flag;
    switch_value_as_flag << "--"
                         << FlagForSwitch(Switch::EnableSoftwareRendering);
    std::string flag = switch_value_as_flag.str();
    if (std::find(switches.begin(), switches.end(), flag) == switches.end()) {
      switches.push_back(flag);
    }
  }
  return switches;
}

}  // namespace clay
