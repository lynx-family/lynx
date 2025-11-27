// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_LYNX_ADAPTOR_PLATFORM_EXTRA_BUNDLE_CLAY_H_
#define CLAY_LYNX_ADAPTOR_PLATFORM_EXTRA_BUNDLE_CLAY_H_

#include <memory>
#include <utility>

#include "clay/ui/shadow/text_update_bundle.h"
#include "core/public/platform_extra_bundle.h"

namespace lynx {
namespace tasm {

class PlatformExtraBundleClay final : public PlatformExtraBundle {
 public:
  PlatformExtraBundleClay(int32_t sign, PlatformExtraBundleHolder* holder,
                          clay::TextUpdateBundle* bundle)
      : PlatformExtraBundle(sign, holder), bundle_(bundle) {}

  std::shared_ptr<clay::TextUpdateBundle> const& GetBundle() const {
    return this->bundle_;
  }

 private:
  std::shared_ptr<clay::TextUpdateBundle> bundle_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CLAY_LYNX_ADAPTOR_PLATFORM_EXTRA_BUNDLE_CLAY_H_
