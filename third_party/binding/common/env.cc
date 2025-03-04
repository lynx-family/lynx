// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "third_party/binding/common/env.h"

namespace lynx {
namespace binding {

EnvImpl::~EnvImpl() = default;

std::ostream& operator<<(std::ostream& os, const Env& env) {
  os << (env.IsNapi() ? "Napi " : "Remote ") << env.impl_;
  return os;
}

}  // namespace binding
}  // namespace lynx
