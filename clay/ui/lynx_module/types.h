// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_LYNX_MODULE_TYPES_H_
#define CLAY_UI_LYNX_MODULE_TYPES_H_

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "clay/public/value.h"

namespace clay {

struct LynxModuleMethod {
  bool is_async = false;
  std::string name;
  std::vector<std::string> arg_names;
  std::unordered_map<std::string, clay::Value::Type> arg_types;
  std::optional<clay::Value::Type> return_type;

  LynxModuleMethod(bool async, std::string&& method_name)
      : is_async(async), name(std::move(method_name)) {}
  void AppendArg(const std::string& name, clay::Value::Type type);
};

struct LynxModuleValues {
  LynxModuleValues();
  ~LynxModuleValues();
  LynxModuleValues(LynxModuleValues&&) = default;
  LynxModuleValues(const LynxModuleValues&) = delete;
  LynxModuleValues& operator=(const LynxModuleValues&) = delete;

  void DoCleanup();
  bool HasKey(const std::string& name) const;
  const clay::Value& Get(const std::string& name) const;

  std::vector<std::string> names;
  clay::Value::Array values;
  // Callback passed by lynx that used to cleanup values. `LynxModuleValues`
  // itself may be cleaned up as well, so don't access `LynxModuleValues` after
  // this is called.
  std::function<void()> cleanup_callback;
};

}  // namespace clay

#endif  // CLAY_UI_LYNX_MODULE_TYPES_H_
