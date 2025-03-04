// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef BASE_SRC_FML_THREAD_NAME_SETTER_H_
#define BASE_SRC_FML_THREAD_NAME_SETTER_H_

#include <string>

namespace lynx {
namespace fml {
void SetThreadName(const std::string& name);
}
}  // namespace lynx

#endif  // BASE_SRC_FML_THREAD_NAME_SETTER_H_
