// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FML_LOG_SETTINGS_H_
#define CLAY_FML_LOG_SETTINGS_H_

#include "clay/fml/logging.h"  // The new, unified bridge header

namespace fml {

// The original LogSettings struct. Kept for source compatibility.
struct LogSettings {
  LogSeverity min_log_level = LOG_INFO;
};

// DEPRECATED: Redirecting to new base logging implementation.
inline void SetLogSettings(const LogSettings& settings) {
  ::lynx::base::logging::SetMinLogLevel(settings.min_log_level);
}

// DEPRECATED: Redirecting to new base logging implementation.
inline LogSettings GetLogSettings() {
  return {::lynx::base::logging::GetMinLogLevel()};
}

// DEPRECATED: Redirecting to new base logging implementation.
inline int GetMinLogLevel() { return ::lynx::base::logging::GetMinLogLevel(); }

class ScopedSetLogSettings {
 public:
  explicit ScopedSetLogSettings(const LogSettings& settings) {
    old_level_ = ::lynx::base::logging::GetMinLogLevel();
    ::lynx::base::logging::SetMinLogLevel(settings.min_log_level);
  }
  ~ScopedSetLogSettings() { ::lynx::base::logging::SetMinLogLevel(old_level_); }

 private:
  int old_level_;
};

}  // namespace fml

#endif  // CLAY_FML_LOG_SETTINGS_H_
