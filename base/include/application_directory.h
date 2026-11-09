// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_APPLICATION_DIRECTORY_H_
#define BASE_INCLUDE_APPLICATION_DIRECTORY_H_

#include <string>

#include "base/include/base_export.h"

namespace lynx {
namespace base {

// Returns an absolute UTF-8 path to a directory for application data.
// Android uses filesDir initialized by LynxBase; iOS uses Documents.
// Returns empty when unavailable, including before Android base initialization
// and on platforms without a native implementation.
BASE_EXPORT std::string GetApplicationDataDirectory();

}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_APPLICATION_DIRECTORY_H_
