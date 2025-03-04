// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_COMMON_LYNX_CONSOLE_HELPER_H_
#define CORE_RUNTIME_COMMON_LYNX_CONSOLE_HELPER_H_

namespace lynx {
namespace piper {

constexpr int CONSOLE_LOG_VERBOSE = -1;
constexpr int CONSOLE_LOG_INFO = 0;
constexpr int CONSOLE_LOG_WARNING = 1;
constexpr int CONSOLE_LOG_ERROR = 2;
constexpr int CONSOLE_LOG_LOG = 3;
constexpr int CONSOLE_LOG_REPORT = 4;
constexpr int CONSOLE_LOG_ALOG = 5;

// lepus console method
constexpr char LepusConsoleAlog[] = "alog";
constexpr char LepusConsoleDebug[] = "debug";
constexpr char LepusConsoleError[] = "error";
constexpr char LepusConsoleInfo[] = "info";
constexpr char LepusConsoleLog[] = "log";
constexpr char LepusConsoleReport[] = "report";
constexpr char LepusConsoleWarn[] = "warn";

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_COMMON_LYNX_CONSOLE_HELPER_H_
