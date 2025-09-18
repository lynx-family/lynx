// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_EMBEDDER_CORE_DEBUG_INFO_HELPER_H_
#define DEVTOOL_EMBEDDER_CORE_DEBUG_INFO_HELPER_H_

#include <string>

#include "base/include/fml/thread.h"

namespace lynx {
namespace devtool {

class DebugInfoHelper {
 public:
  DebugInfoHelper() = default;
  ~DebugInfoHelper() = default;

  void GetLepusDebugInfo(const std::string& url, std::string& debug_info);

 private:
  static const fml::RefPtr<fml::TaskRunner>& GetDownloadTaskRunner();
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_EMBEDDER_CORE_DEBUG_INFO_HELPER_H_
