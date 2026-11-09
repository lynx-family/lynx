// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_SRC_APPLICATION_DIRECTORY_APPLICATION_DIRECTORY_IMPL_H_
#define BASE_SRC_APPLICATION_DIRECTORY_APPLICATION_DIRECTORY_IMPL_H_

#include <string>

#include "base/include/base_export.h"

namespace lynx {
namespace base {

std::string GetDefaultApplicationDataDirectory();

// The Android base bridge initializes the application directory before SDK use.
BASE_EXPORT void SetApplicationDataDirectory(std::string path);

}  // namespace base
}  // namespace lynx

#endif  // BASE_SRC_APPLICATION_DIRECTORY_APPLICATION_DIRECTORY_IMPL_H_
