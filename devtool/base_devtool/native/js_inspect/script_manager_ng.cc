// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/base_devtool/native/js_inspect/script_manager_ng.h"

#include "devtool/js_inspect/inspector_const.h"

namespace lynx {
namespace devtool {

void ScriptManagerNG::SetBreakpointDetail(const rapidjson::Value &content) {
  if (!content.HasMember(kKeyMethod) ||
      strcmp(content[kKeyMethod].GetString(),
             kMethodDebuggerSetBreakpointByUrl) != 0) {
    return;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  int message_id = content[kKeyId].GetInt();
  Breakpoint bp;
  bp.line_number_ = content[kKeyParams][kKeyLineNumber].GetInt();
  bp.column_number_ = content[kKeyParams][kKeyColumnNumber].GetInt();
  if (content[kKeyParams].HasMember(kKeyUrl)) {
    bp.url_ = content[kKeyParams][kKeyUrl].GetString();
  } else if (content[kKeyParams].HasMember(kKeyUrlRegex)) {
    bp.url_ = content[kKeyParams][kKeyUrlRegex].GetString();
  }
  if (content[kKeyParams].HasMember(kKeyCondition)) {
    bp.condition_ = content[kKeyParams][kKeyCondition].GetString();
  }
  set_breakpoint_map_.emplace(message_id, bp);
}

void ScriptManagerNG::SetBreakpointId(const rapidjson::Value &content) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!content.HasMember(kKeyId) || set_breakpoint_map_.empty()) {
    return;
  }
  auto it = set_breakpoint_map_.find(content[kKeyId].GetInt());
  if (it != set_breakpoint_map_.end()) {
    if (!content.HasMember(kKeyError)) {
      Breakpoint &bp = it->second;
      bp.breakpoint_id_ = content[kKeyResult][kKeyBreakpointId].GetString();
      AddBreakpoint(bp);
    }
    set_breakpoint_map_.erase(it);
  }
}

void ScriptManagerNG::RemoveBreakpoint(const std::string &breakpoint_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  breakpoints_.erase(breakpoint_id);
}

void ScriptManagerNG::InsertScriptId(int script_id) {
  script_ids_.emplace(script_id);
}

void ScriptManagerNG::ClearScriptIds() { script_ids_.clear(); }

void ScriptManagerNG::AddBreakpoint(const Breakpoint &breakpoint) {
  if (breakpoints_.find(breakpoint.breakpoint_id_) == breakpoints_.end()) {
    breakpoints_[breakpoint.breakpoint_id_] = breakpoint;
  }
}

}  // namespace devtool
}  // namespace lynx
