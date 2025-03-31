// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_SYSTEM_INFO_H_
#define CORE_RUNTIME_BINDINGS_JSI_SYSTEM_INFO_H_
#include <vector>

#include "lynx/core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {
class Runtime;

class SystemInfo {
 public:
  SystemInfo() = delete;
  ~SystemInfo() = delete;

  static piper::Object Bindings(Runtime &rt);
};
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_JSI_SYSTEM_INFO_H_
