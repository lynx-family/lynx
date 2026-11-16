// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_PATH_HARMONY_NODE_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_PATH_HARMONY_NODE_H_

#include <array>
#include <string>
#include <vector>

#include "gfx/public/path_types.h"

namespace lynx::tasm {

enum class HarmonyNodePathStatus { kReady, kEmpty, kInvalid, kUnsupported };

struct HarmonyNodePathPayload {
  HarmonyNodePathStatus status;
  std::string commands;
  float width{0};
  float height{0};
};

// Immutable geometry for the node path API, whose fill rule is winding only.
// It is independent of the Drawing path resource and never represents reset.
class HarmonyNodePathDescriptor {
 public:
  static HarmonyNodePathDescriptor Build(gfx::PathSourceView source,
                                         gfx::PathFillRule fill_rule,
                                         float density);
  HarmonyNodePathStatus status() const { return status_; }

  // All attachment arguments are device pixels. Source scale and density have
  // already been applied to geometry; placement never applies source scale.
  // Dimensions must be positive, even for an empty clip. An empty result must
  // be handled explicitly by the attachment layer, never as a reset request.
  HarmonyNodePathPayload MakePayload(float x, float y, float width,
                                     float height) const;

 private:
  struct Command {
    char verb;
    unsigned count;
    std::array<float, 7> args;
  };
  struct Builder;
  explicit HarmonyNodePathDescriptor(HarmonyNodePathStatus status)
      : status_(status) {}
  HarmonyNodePathStatus status_;
  std::vector<Command> commands_;
};

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_PATH_HARMONY_NODE_H_
