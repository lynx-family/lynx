// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_COMMON_HARMONY_PLATFORM_EXTRA_BUNDLE_HARMONY_H_
#define CORE_RENDERER_UI_WRAPPER_COMMON_HARMONY_PLATFORM_EXTRA_BUNDLE_HARMONY_H_

#include <utility>

#include "base/include/fml/memory/ref_counted.h"
#include "core/public/platform_extra_bundle.h"

namespace lynx {
namespace tasm {

class PlatformExtraBundleHarmony final : public PlatformExtraBundle {
 public:
  PlatformExtraBundleHarmony(int32_t sign, PlatformExtraBundleHolder* holder,
                             fml::RefCountedThreadSafeStorage* bundle)
      : PlatformExtraBundle(sign, holder), bundle_(bundle) {}

  PlatformExtraBundleHarmony(
      int32_t sign, PlatformExtraBundleHolder* holder,
      fml::RefPtr<fml::RefCountedThreadSafeStorage>&& bundle)
      : PlatformExtraBundle(sign, holder), bundle_(std::move(bundle)) {}

  PlatformExtraBundleHarmony(
      int32_t sign, PlatformExtraBundleHolder* holder,
      fml::RefPtr<fml::RefCountedThreadSafeStorage> const& bundle)
      : PlatformExtraBundle(sign, holder), bundle_(bundle) {}

  fml::RefPtr<fml::RefCountedThreadSafeStorage> const& GetBundle() const {
    return this->bundle_;
  }

 private:
  fml::RefPtr<fml::RefCountedThreadSafeStorage> bundle_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_COMMON_HARMONY_PLATFORM_EXTRA_BUNDLE_HARMONY_H_
