// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FML_LOG_SETTINGS_H_
#define CLAY_FML_LOG_SETTINGS_H_

#include <string>

#include "clay/fml/log_level.h"

namespace fml {

// Settings which control the behavior of FML logging.
struct LogSettings {
  // The minimum logging level.
  // Anything at or above this level will be logged (if applicable).
  // Anything below this level will be silently ignored.
  //
  // The log level defaults to 0 (LOG_INFO).
  //
  // Log messages for FML_VLOG(x) are logged
  // at level -x, so setting the min log level to negative values enables
  // verbose logging.
  LogSeverity min_log_level = LOG_INFO;
};

// Gets the active log settings for the current process.
void SetLogSettings(const LogSettings& settings);

// Sets the active log settings for the current process.
LogSettings GetLogSettings();

// Gets the minimum log level for the current process. Never returs a value
// higher than LOG_FATAL.
int GetMinLogLevel();

class ScopedSetLogSettings {
 public:
  explicit ScopedSetLogSettings(const LogSettings& settings);
  ~ScopedSetLogSettings();

 private:
  LogSettings old_settings_;
};

}  // namespace fml

#endif  // CLAY_FML_LOG_SETTINGS_H_
