// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_JS_INSPECT_SCRIPT_MANAGER_NG_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_JS_INSPECT_SCRIPT_MANAGER_NG_H_

#include <mutex>
#include <set>
#include <string>
#include <unordered_map>

#include "base/include/base_export.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace devtool {

struct Breakpoint {
  std::string breakpoint_id_;
  int line_number_;
  int column_number_;
  std::string url_;
  std::string condition_;
};

class BASE_EXPORT ScriptManagerNG {
 public:
  ScriptManagerNG() = default;
  virtual ~ScriptManagerNG() = default;

  void SetBreakpointDetail(const rapidjson::Value& content);
  void SetBreakpointId(const rapidjson::Value& content);
  void RemoveBreakpoint(const std::string& breakpoint_id);
  const std::unordered_map<std::string, Breakpoint>& GetBreakpoints() {
    return breakpoints_;
  }

  void SetBreakpointsActive(bool active) { breakpoints_active_ = active; }
  bool GetBreakpointsActive() { return breakpoints_active_; }

  void InsertScriptId(int script_id);
  void ClearScriptIds();
  const std::set<int>& GetScriptIds() { return script_ids_; }

 private:
  void AddBreakpoint(const Breakpoint& breakpoint);

  std::mutex mutex_;
  bool breakpoints_active_{true};

  std::unordered_map<std::string, Breakpoint> breakpoints_;
  std::unordered_map<int, Breakpoint> set_breakpoint_map_;
  std::set<int> script_ids_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_JS_INSPECT_SCRIPT_MANAGER_NG_H_
