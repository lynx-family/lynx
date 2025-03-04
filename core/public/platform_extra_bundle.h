// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_PLATFORM_EXTRA_BUNDLE_H_
#define CORE_PUBLIC_PLATFORM_EXTRA_BUNDLE_H_

#include <cstdint>

namespace lynx {
namespace tasm {

class PlatformExtraBundle;

class PlatformExtraBundleHolder {
 public:
  PlatformExtraBundleHolder() = default;
  virtual ~PlatformExtraBundleHolder() = default;
};

class PlatformExtraBundle {
 public:
  PlatformExtraBundle(int32_t signature, PlatformExtraBundleHolder *holder)
      : signature_(signature), holder_(holder) {}

  virtual ~PlatformExtraBundle() = default;

  int32_t Signature() const { return signature_; }

  PlatformExtraBundleHolder *Holder() const { return holder_; }

 private:
  int32_t signature_;
  PlatformExtraBundleHolder *holder_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_PUBLIC_PLATFORM_EXTRA_BUNDLE_H_
