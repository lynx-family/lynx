// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_CUSTOM_DRAW_FUNCTIONS_H_
#define CORE_RENDERER_DOM_FRAGMENT_CUSTOM_DRAW_FUNCTIONS_H_

#include <functional>

namespace lynx::tasm {
class DisplayListBuilder;
class Fragment;

// Element-owned callbacks, installed on its Fragment during Flush. Callbacks
// record commands directly into the active DisplayListBuilder. They belong to
// one element instance and are not copied when the element is cloned. A binding
// requires an independent platform layer; clearing it retains an existing
// layer. Bindings must remain unchanged during drawing; updates are installed
// during Flush.
struct CustomDrawFunctions {
  // Runs after ordinary background, behavior and children, inside Begin/End.
  // These content callbacks run only when contents are recorded; property-only
  // updates retain the existing contents without invoking either callback.
  // TODO(songshourui.null): Preserve foreground Fill, Border, background image
  // and gradient ordering in the iOS display-list consumer. These commands are
  // currently placed below the host content.
  std::function<void(Fragment&, DisplayListBuilder&)> draw_foreground;
  // Runs before ordinary content on a full redraw. Fragment always draws its
  // default content next, then invokes draw_foreground.
  std::function<void(Fragment&, DisplayListBuilder&)> draw_behind;
  // Runs in the subtree-property phase, after the current CSS/default layer
  // properties are recorded. Record only subtree properties here; properties
  // omitted by the callback retain their CSS/default values, not its previous
  // output. This hook also runs on property-only updates without redrawing
  // content. Removing the hook restores the CSS/default properties.
  std::function<void(Fragment&, DisplayListBuilder&)> draw_layer_properties;
};
}  // namespace lynx::tasm

#endif  // CORE_RENDERER_DOM_FRAGMENT_CUSTOM_DRAW_FUNCTIONS_H_
