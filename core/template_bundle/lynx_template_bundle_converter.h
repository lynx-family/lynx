// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_LYNX_TEMPLATE_BUNDLE_CONVERTER_H_
#define CORE_TEMPLATE_BUNDLE_LYNX_TEMPLATE_BUNDLE_CONVERTER_H_

#include <string>

#include "core/template_bundle/lynx_template_bundle.h"

namespace lynx {
namespace tasm {

class LynxTemplateBundleConverter final {
 public:
  static std::string ConvertTemplateBundleToSerializedString(
      const LynxTemplateBundle& template_bundle);
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_LYNX_TEMPLATE_BUNDLE_CONVERTER_H_
