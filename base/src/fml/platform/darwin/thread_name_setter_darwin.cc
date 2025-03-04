// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <pthread.h>

#include "base/src/fml/thread_name_setter.h"

namespace lynx {
namespace fml {
void SetThreadName(const std::string& name) {
  if (name == "") {
    return;
  }
  pthread_setname_np(name.c_str());
}
}  // namespace fml
}  // namespace lynx
