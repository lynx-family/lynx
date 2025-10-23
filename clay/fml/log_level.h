// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FML_LOG_LEVEL_H_
#define CLAY_FML_LOG_LEVEL_H_

namespace fml {

typedef int LogSeverity;

// Default log levels. Negative values can be used for verbose log levels.
constexpr LogSeverity LOG_INFO = 0;
constexpr LogSeverity LOG_WARNING = 1;
constexpr LogSeverity LOG_ERROR = 2;
constexpr LogSeverity LOG_FATAL = 3;
constexpr LogSeverity LOG_NUM_SEVERITIES = 4;

// One of the Windows headers defines ERROR to 0. This makes the token
// concatenation in FML_LOG(ERROR) to resolve to LOG_0. We define this back to
// the appropriate log level. See more:
// https://github.com/flutter/engine/pull/6677
#if defined(_WIN32) && !defined(LOG_0)
#define LOG_0 LOG_ERROR
#endif

// LOG_DFATAL is LOG_FATAL in debug mode, ERROR in normal mode
#ifdef NDEBUG
const LogSeverity LOG_DFATAL = LOG_ERROR;
#else
const LogSeverity LOG_DFATAL = LOG_FATAL;
#endif

}  // namespace fml

#endif  // CLAY_FML_LOG_LEVEL_H_
