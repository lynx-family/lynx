// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_WINDOWS_FLUTTER_PROJECT_BUNDLE_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_FLUTTER_PROJECT_BUNDLE_H_

#include <filesystem>
#include <string>
#include <vector>

namespace clay {

// Properties for configuring a Flutter engine instance.
typedef struct {
  // The path to the icudtl.dat file for the version of Flutter you are using.
  // This can either be an absolute path or a path relative to the directory
  // containing the executable.
  const wchar_t* icu_data_path;

  bool enable_software_rendering;
} FlutterDesktopEngineProperties;

// The data associated with a Flutter project needed to run it in an engine.
class FlutterProjectBundle {
 public:
  // Creates a new project bundle from the given properties.
  //
  // Treats any relative paths as relative to the directory of this executable.
  explicit FlutterProjectBundle(
      const FlutterDesktopEngineProperties& properties);

  ~FlutterProjectBundle();

  // Whether or not the bundle is valid. This does not check that the paths
  // exist, or contain valid data, just that paths were able to be constructed.
  bool HasValidPaths();

  // Returns the path to the ICU data file.
  const std::filesystem::path& icu_path() { return icu_path_; }

  // Returns any switches that should be passed to the engine.
  const std::vector<std::string> GetSwitches();

  // Sets engine switches.
  void SetSwitches(const std::vector<std::string>& switches);

  bool use_software_rendering() { return enable_software_rendering_; }

 private:
  std::filesystem::path icu_path_;

  // Engine switches.
  std::vector<std::string> engine_switches_;

  bool enable_software_rendering_ = false;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_WINDOWS_FLUTTER_PROJECT_BUNDLE_H_
