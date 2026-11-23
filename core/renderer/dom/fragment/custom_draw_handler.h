// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_CUSTOM_DRAW_HANDLER_H_
#define CORE_RENDERER_DOM_FRAGMENT_CUSTOM_DRAW_HANDLER_H_

#include <cstdint>

#include "base/include/fml/memory/ref_counted.h"

namespace lynx::tasm {
class DisplayListBuilder;
class Fragment;

// Element-owned custom drawing handler, installed on its Fragment during Flush.
// Handlers record commands directly into the active DisplayListBuilder. They
// belong to one element instance and are not copied when the element is cloned.
// kLayerProperties and kRequiresLayer request an independent platform layer;
// other content handlers can flatten into the ancestor's display list. Clearing
// a binding retains an existing layer. Bindings must remain unchanged during
// drawing; updates are installed during Flush.
class CustomDrawHandler : public fml::RefCountedThreadSafe<CustomDrawHandler> {
 public:
  enum Phase : uint8_t {
    // Runs before ordinary content on a full redraw. Fragment always draws its
    // default content next, then invokes kForeground.
    kBehind = 1 << 0,
    // Runs after ordinary background, behavior and children, inside Begin/End.
    // Content phases run only when contents are recorded; property-only updates
    // retain the existing contents without invoking either phase.
    // TODO(songshourui.null): Preserve foreground Fill, Border, background
    // image and gradient ordering in the iOS display-list consumer. These
    // commands are currently placed below the host content.
    kForeground = 1 << 1,
    // Runs in the subtree-property phase, after the current CSS/default layer
    // properties are recorded. Record only subtree properties here; properties
    // omitted by the handler retain their CSS/default values, not its previous
    // output. This phase also runs on property-only updates without redrawing
    // content. Removing the handler restores the CSS/default properties.
    // Transform overrides must also publish the same local matrix through
    // Element::SetCustomLayerTransform for geometry queries.
    kLayerProperties = 1 << 2,
  };

  enum Flags : uint8_t {
    kNone = 0,
    // Content clips affect the owning platform layer and must not flatten.
    kRequiresLayer = 1 << 3,
    // Set when Draw may invoke user/runtime callbacks that could rebind or
    // clear this handler during evaluation, so Fragment retains a local ref.
    kMayReenter = 1 << 4,
    kPhaseMask = kBehind | kForeground | kLayerProperties,
  };

  explicit CustomDrawHandler(uint8_t flags) : flags_(flags) {}
  virtual ~CustomDrawHandler() = default;

  uint8_t flags() const { return flags_; }
  bool HasAnyPhase() const { return (flags_ & kPhaseMask) != 0; }
  bool HasPhase(Phase phase) const { return (flags_ & phase) != 0; }
  bool HasDrawBehind() const { return HasPhase(kBehind); }
  bool HasDrawForeground() const { return HasPhase(kForeground); }
  bool HasDrawLayerProperties() const { return HasPhase(kLayerProperties); }
  bool RequiresLayer() const {
    return (flags_ & (kLayerProperties | kRequiresLayer)) != 0;
  }
  bool MayReenter() const { return (flags_ & kMayReenter) != 0; }

  virtual void Draw(Fragment& fragment, DisplayListBuilder& builder,
                    Phase phase) = 0;

 private:
  uint8_t flags_;
};
}  // namespace lynx::tasm

#endif  // CORE_RENDERER_DOM_FRAGMENT_CUSTOM_DRAW_HANDLER_H_
