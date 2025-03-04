// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_COMMON_PROP_BUNDLE_CREATOR_DEFAULT_H_
#define CORE_RENDERER_UI_WRAPPER_COMMON_PROP_BUNDLE_CREATOR_DEFAULT_H_

#include <memory>

#include "core/public/prop_bundle.h"

namespace lynx {
namespace tasm {

class PropBundleCreatorDefault : public PropBundleCreator {
 public:
  std::unique_ptr<PropBundle> CreatePropBundle() override;
};

}  // namespace tasm

}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_COMMON_PROP_BUNDLE_CREATOR_DEFAULT_H_
