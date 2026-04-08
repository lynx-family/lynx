// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_COMMON_LYNX_CONSOLE_HELPER_H_
#define CORE_RUNTIME_COMMON_LYNX_CONSOLE_HELPER_H_

namespace lynx {
namespace runtime {
constexpr int CONSOLE_UNKNOWN = -99;
constexpr int CONSOLE_LOG_VERBOSE = -1;
constexpr int CONSOLE_LOG_INFO = 0;
constexpr int CONSOLE_LOG_WARNING = 1;
constexpr int CONSOLE_LOG_ERROR = 2;
constexpr int CONSOLE_LOG_LOG = 3;
constexpr int CONSOLE_LOG_REPORT = 4;
constexpr int CONSOLE_LOG_ALOG = 5;

// lepus console method
constexpr char ConsoleAlog[] = "alog";
constexpr char ConsoleAssert[] = "assert";
constexpr char ConsoleCount[] = "count";
constexpr char ConsoleCountReset[] = "countReset";
constexpr char ConsoleDebug[] = "debug";
constexpr char ConsoleError[] = "error";
constexpr char ConsoleGroup[] = "group";
constexpr char ConsoleGroupCollapsed[] = "groupCollapsed";
constexpr char ConsoleGroupEnd[] = "groupEnd";
constexpr char ConsoleInfo[] = "info";
constexpr char ConsoleLog[] = "log";
constexpr char ConsoleReport[] = "report";
constexpr char ConsoleTable[] = "table";
constexpr char ConsoleTime[] = "time";
constexpr char ConsoleTimeEnd[] = "timeEnd";
constexpr char ConsoleTimeLog[] = "timeLog";
constexpr char ConsoleWarn[] = "warn";

}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_COMMON_LYNX_CONSOLE_HELPER_H_
