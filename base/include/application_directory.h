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
// Android and Harmony use filesDir; iOS uses Documents. Desktop platforms
// create a "lynx" subdirectory in the current user's application data
// directory: Application Support on macOS, Local AppData on Windows, and
// XDG_DATA_HOME (or HOME/.local/share) on Linux. Returns empty when
// unavailable, including before mobile base initialization. Successful lookups
// are cached.
BASE_EXPORT std::string GetApplicationDataDirectory();

// Returns a UTF-8 path to a child of the effective application data directory,
// creating it when necessary. The name must be a single nonempty component,
// excluding "." and "..". Returns empty if the root is unavailable or the child
// cannot be created. Does not create an overridden root directory.
BASE_EXPORT std::string GetApplicationDataSubDirectory(const std::string& name);

// Overrides the cached directory on all platforms, including after a lookup.
// The caller supplies the complete absolute UTF-8 path and creates the
// directory before use. No subdirectory is appended. An empty path clears the
// override and restores default lookup (mobile platforms return empty until a
// base bridge sets the path). Set this before initializing consumers of
// application data.
BASE_EXPORT void SetApplicationDataDirectory(std::string path);

}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_APPLICATION_DIRECTORY_H_
