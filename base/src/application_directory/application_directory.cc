// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/application_directory.h"

#include <mutex>
#include <utility>

#include "base/include/no_destructor.h"
#include "base/src/application_directory/application_directory_impl.h"

namespace lynx {
namespace base {
namespace {

struct ApplicationDirectory {
  std::mutex mutex;
  std::string path;
};

ApplicationDirectory& Directory() {
  static NoDestructor<ApplicationDirectory> directory;
  return *directory;
}

}  // namespace

std::string GetApplicationDataDirectory() {
  auto& directory = Directory();
  std::lock_guard<std::mutex> lock(directory.mutex);
  if (directory.path.empty()) {
    directory.path = GetDefaultApplicationDataDirectory();
  }
  return directory.path;
}

void SetApplicationDataDirectory(std::string path) {
  auto& directory = Directory();
  std::lock_guard<std::mutex> lock(directory.mutex);
  directory.path = std::move(path);
}

}  // namespace base
}  // namespace lynx
