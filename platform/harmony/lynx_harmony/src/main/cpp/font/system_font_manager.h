// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_FONT_SYSTEM_FONT_MANAGER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_FONT_SYSTEM_FONT_MANAGER_H_

#include <node_api.h>

#include <shared_mutex>
#include <string>

#include "base/include/closure.h"
#include "base/include/no_destructor.h"

namespace lynx {
namespace tasm {
namespace harmony {

using SystemFontCallback =
    base::MoveOnlyClosure<void, std::string, std::string>;

class SystemFontManager {
 public:
  static SystemFontManager& GetInstance() {
    static base::NoDestructor<SystemFontManager> kInstance;
    return *kInstance;
  }
  SystemFontManager() = default;
  ~SystemFontManager() = default;

  void GetSystemFont(napi_env env, SystemFontCallback callback,
                     bool force_update = false);

 private:
  bool CheckAndGetSystemFont(SystemFontCallback callback);
  std::string system_font_family_;
  std::string system_font_path_;
  std::shared_mutex mutex_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_FONT_SYSTEM_FONT_MANAGER_H_
