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
  /**
   * The thread name is a meaningful C language string, whose length is
   * restricted to 16 characters, including the terminating null byte ('\0').
   * Fails with error 34 (ERANGE) on Android if longer than 16 bytes.
   */
  std::string compliant_name(name.substr(0, 15));
  pthread_setname_np(pthread_self(), compliant_name.c_str());
}
}  // namespace fml
}  // namespace lynx
