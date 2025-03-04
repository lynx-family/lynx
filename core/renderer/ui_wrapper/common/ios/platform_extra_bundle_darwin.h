// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_COMMON_IOS_PLATFORM_EXTRA_BUNDLE_DARWIN_H_
#define CORE_RENDERER_UI_WRAPPER_COMMON_IOS_PLATFORM_EXTRA_BUNDLE_DARWIN_H_

#include "core/public/platform_extra_bundle.h"

namespace lynx {
namespace tasm {

class PlatformExtraBundleDarwin : public PlatformExtraBundle {
 public:
  PlatformExtraBundleDarwin(int32_t signature,
                            PlatformExtraBundleHolder* holder, id bundle);

  ~PlatformExtraBundleDarwin() override = default;

  id PlatformBundle() { return platform_bundle; }

 private:
  id platform_bundle;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_COMMON_IOS_PLATFORM_EXTRA_BUNDLE_DARWIN_H_
