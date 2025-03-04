// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/common/ios/platform_extra_bundle_darwin.h"

namespace lynx {
namespace tasm {

PlatformExtraBundleDarwin::PlatformExtraBundleDarwin(int32_t signature,
                                                     PlatformExtraBundleHolder* holder, id bundle)
    : PlatformExtraBundle(signature, holder), platform_bundle(bundle) {}

}  // namespace tasm
}  // namespace lynx
