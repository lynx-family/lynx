// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_RESOURCE_JS_SOURCE_LOADER_DESKTOP_H_
#define PLATFORM_EMBEDDER_RESOURCE_JS_SOURCE_LOADER_DESKTOP_H_

#include <string>

namespace lynx {
namespace runtime {
namespace js {
/*
 * JSSourceLoaderDesktop is used on desktop headless mode
 */
class JSSourceLoaderDesktop {
 private:
  static std::string LoadFileData(const std::string& path);

 public:
  std::string LoadJSSource(const std::string& name);
};

}  // namespace js

}  // namespace runtime
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_RESOURCE_JS_SOURCE_LOADER_DESKTOP_H_
