// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/component_attributes.h"

namespace lynx {
namespace tasm {

const std::set<std::string>& ComponentAttributes::GetAttrNames() {
  const static base::NoDestructor<std::set<std::string>> kAttrNames{{
      "name",
      "style",
      "class",
      "flatten",
      "clip-radius",
      "consume-slide-event",
      "overlap",
      "user-interaction-enabled",
      "native-interaction-enabled",
      "block-native-event",
      "block-native-event-areas",
      "enableLayoutOnly",
      "cssAlignWithLegacyW3C",
      "intersection-observers",
      "trigger-global-event",
      "ios-enable-simultaneous-touch",
      "enable-new-animator",
      "enable-touch-pseudo-propagation",
      "exposure-scene",
      "exposure-id",
      "exposure-screen-margin-top",
      "exposure-screen-margin-bottom",
      "exposure-screen-margin-left",
      "exposure-screen-margin-right",
      "enable-exposure-ui-margin",
      "exposure-ui-margin-top",
      "exposure-ui-margin-bottom",
      "exposure-ui-margin-left",
      "exposure-ui-margin-right",
      "exposure-area",
      "focusable",
      "focus-index",
      "accessibility-label",
      "accessibility-element",
      "accessibility-traits",
      "__lynx_timing_flag",
  }};
  return *kAttrNames;
}

}  // namespace tasm
}  // namespace lynx
