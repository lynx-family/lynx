// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_ELEMENT_LAYOUT_COORDINATES_H_
#define CORE_RENDERER_DOM_ELEMENT_LAYOUT_COORDINATES_H_

#include <cstdint>

namespace lynx::tasm {

class ElementManager;

// Values are part of the JS/RTS binding contract. Keep them stable.
enum class LayoutCoordinatesQuery : int32_t {
  // Returns only attachment state; does not read geometry.
  kAttached = 0,
  // Maps the input point to the nearest Overlay or Page coordinate root.
  kLocalToRoot = 1,
  // Maps the input point to the platform window, using the live host origin.
  kLocalToWindow = 2,
  // Returns the transformed bounds after ancestor and host viewport clipping.
  kBoundsInWindow = 3,
  // Returns the current local width and height without coordinate conversion.
  kSize = 4,
};

struct LayoutCoordinatesResult {
  bool attached{false};
  // An attachment query succeeds even when the node is detached.
  bool valid{false};
  double x{0.};
  double y{0.};
  double left{0.};
  double top{0.};
  double right{0.};
  double bottom{0.};
  double width{0.};
  double height{0.};
};

// Synchronous, non-retaining query on the owning engine thread.
// The coordinate root is resolved from the Element tree.
// Input points and output geometry use physical pixels. Read geometry only
// when valid is true and the query requests it. Bindings emit requested fields.
LayoutCoordinatesResult QueryLayoutCoordinates(ElementManager* manager,
                                               int32_t uid,
                                               LayoutCoordinatesQuery query,
                                               double x = 0., double y = 0.);

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_DOM_ELEMENT_LAYOUT_COORDINATES_H_
