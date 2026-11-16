// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_PATH_HARMONY_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_PATH_HARMONY_H_

#include <native_drawing/drawing_path.h>

#include <memory>
#include <vector>

#include "core/renderer/ui_wrapper/painting/harmony/path_harmony_node.h"
#include "gfx/public/path_impl_factory.h"

namespace lynx::gfx {
class Path;
}

namespace lynx::tasm {

enum class PathHarmonyStatus { kReady, kEmpty, kInvalid, kUnsupported };

struct DrawingPathResult {
  PathHarmonyStatus status;
  const OH_Drawing_Path* path;
};

struct NodePathResult {
  HarmonyNodePathStatus status;
  std::shared_ptr<const HarmonyNodePathDescriptor> descriptor;
};

// Construction only retains source. Native variants are accessed on the Harmony
// UI consumer executor; borrowed handles remain valid while this impl is alive.
class PathHarmony final : public gfx::PathImpl {
 public:
  explicit PathHarmony(gfx::PathSource source);
  ~PathHarmony() override;
  BackendKind GetBackendKind() const override { return BackendKind::kHarmony; }
  static DrawingPathResult GetDrawingPath(const gfx::Path& path,
                                          gfx::PathFillRule rule,
                                          float density);
  DrawingPathResult GetDrawingPath(gfx::PathFillRule rule, float density) const;
  static NodePathResult GetNodeClip(const gfx::Path& path,
                                    gfx::PathFillRule rule, float density);
  NodePathResult GetNodeClip(gfx::PathFillRule rule, float density) const;

 private:
  struct PathDeleter {
    void operator()(OH_Drawing_Path* path) const;
  };
  struct DrawingVariant {
    float density;
    gfx::PathFillRule rule;
    PathHarmonyStatus status = PathHarmonyStatus::kInvalid;
    std::unique_ptr<OH_Drawing_Path, PathDeleter> path;
  };
  DrawingVariant BuildDrawingPath(gfx::PathFillRule rule, float density) const;
  mutable std::vector<DrawingVariant> drawing_variants_;
  struct NodeVariant {
    float density;
    gfx::PathFillRule rule;
    std::shared_ptr<const HarmonyNodePathDescriptor> descriptor;
  };
  mutable std::vector<NodeVariant> node_variants_;
};

class PathHarmonyFactory final : public gfx::PathImplFactory {
 public:
  std::unique_ptr<gfx::PathImpl> Create(gfx::PathSource source) const override;
};

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_PATH_HARMONY_H_
