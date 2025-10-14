// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/lynx_module/types.h"

#include <string>

#include "clay/fml/logging.h"

namespace clay {

LynxModuleValues::LynxModuleValues() {}

LynxModuleValues::~LynxModuleValues() {
  FML_DCHECK(cleanup_callback == nullptr) << "Forget to cleanup.";
}

void LynxModuleValues::DoCleanup() {
  auto cleanup = cleanup_callback;
  cleanup_callback = nullptr;
  if (cleanup) {
    cleanup();
  }
}

bool LynxModuleValues::HasKey(const std::string& name) const {
  return std::find(names.begin(), names.end(), name) != names.end();
}

const clay::Value& LynxModuleValues::Get(const std::string& name) const {
  auto it = std::find(names.begin(), names.end(), name);
  if (it != names.end()) {
    return values[it - names.begin()];
  }
  static const clay::Value invalid_value;
  return invalid_value;
}

void LynxModuleMethod::AppendArg(const std::string& name,
                                 clay::Value::Type type) {
  arg_names.emplace_back(name);
  arg_types.emplace(name, type);
}

}  // namespace clay
