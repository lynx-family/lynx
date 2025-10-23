// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FML_PATHS_H_
#define CLAY_FML_PATHS_H_

#include <string>
#include <utility>

#include "base/include/fml/unique_fd.h"

namespace fml {
namespace paths {

std::pair<bool, std::string> GetExecutablePath();
std::pair<bool, std::string> GetExecutableDirectoryPath();

// Get the directory to the application's caches directory.
fml::UniqueFD GetCachesDirectory();

std::string JoinPaths(std::initializer_list<std::string> components);

// Returns the absolute path of a possibly relative path.
// It doesn't consult the filesystem or simplify the path.
std::string AbsolutePath(const std::string& path);

// Returns the directory name component of the given path.
std::string GetDirectoryName(const std::string& path);

// Decodes a URI encoded string.
std::string SanitizeURIEscapedCharacters(const std::string& str);

// Converts a file URI to a path.
std::string FromURI(const std::string& uri);

}  // namespace paths
}  // namespace fml

#endif  // CLAY_FML_PATHS_H_
