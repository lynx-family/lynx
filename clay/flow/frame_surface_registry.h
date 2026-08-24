// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_FRAME_SURFACE_REGISTRY_H_
#define CLAY_FLOW_FRAME_SURFACE_REGISTRY_H_

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <optional>

#include "clay/common/element_id.h"
#include "clay/gfx/geometry/float_size.h"
#include "clay/gfx/rendering_backend.h"
#include "skity/geometry/rect.hpp"

namespace clay {

class LayerTree;

class FrameSurfaceId {
 public:
  FrameSurfaceId(const ElementId& element_id, uint64_t incarnation,
                 uint64_t generation, uint64_t size_generation);

  const ElementId& element_id() const { return element_id_; }
  uint64_t incarnation() const { return incarnation_; }
  uint64_t generation() const { return generation_; }
  uint64_t size_generation() const { return size_generation_; }

  bool operator==(const FrameSurfaceId& other) const {
    return element_id_ == other.element_id_ &&
           incarnation_ == other.incarnation_ &&
           generation_ == other.generation_ &&
           size_generation_ == other.size_generation_;
  }
  bool operator!=(const FrameSurfaceId& other) const {
    return !(*this == other);
  }

 private:
  ElementId element_id_;
  uint64_t incarnation_ = 0;
  uint64_t generation_ = 0;
  uint64_t size_generation_ = 0;
};

class FrameSurface {
 public:
  FrameSurface(FrameSurfaceId surface_id, std::shared_ptr<LayerTree> layer_tree,
               const FloatSize& surface_size,
               std::optional<skity::Rect> damage_rect);
  ~FrameSurface();

  const FrameSurfaceId& surface_id() const { return surface_id_; }
  const std::shared_ptr<LayerTree>& layer_tree() const { return layer_tree_; }
  const FloatSize& surface_size() const { return surface_size_; }
  const std::optional<skity::Rect>& damage_rect() const { return damage_rect_; }
  const std::optional<skity::Rect>& drawable_image_damage_rect() const {
    return drawable_image_damage_rect_;
  }

 private:
  FrameSurfaceId surface_id_;
  std::shared_ptr<LayerTree> layer_tree_;
  FloatSize surface_size_;
  std::optional<skity::Rect> damage_rect_;
  std::optional<skity::Rect> drawable_image_damage_rect_;
};

class FrameSurfaceRegistry {
 public:
  bool SubmitPendingSurface(
      const FrameSurfaceId& surface_id, std::shared_ptr<LayerTree> layer_tree,
      const FloatSize& surface_size = FloatSize(),
      std::optional<skity::Rect> damage_rect = std::nullopt);
  bool SubmitPendingSurface(
      const FrameSurfaceId& surface_id, std::unique_ptr<LayerTree> layer_tree,
      const FloatSize& surface_size = FloatSize(),
      std::optional<skity::Rect> damage_rect = std::nullopt);
  std::shared_ptr<FrameSurface> AcquireSurfaceForParentFrame(
      const FrameSurfaceId& surface_id);
  std::shared_ptr<FrameSurface> AcquireLatestSurfaceForParentFrame(
      const ElementId& element_id);
  std::shared_ptr<FrameSurface> GetSurface(
      const FrameSurfaceId& surface_id) const;
  std::shared_ptr<FrameSurface> GetActiveSurface(
      const ElementId& element_id) const;
  std::optional<FrameSurfaceId> GetActiveSurfaceId(
      const ElementId& element_id) const;
  std::shared_ptr<FrameSurface> GetPreviousSurface(
      const ElementId& element_id) const;
  std::optional<FrameSurfaceId> GetPreviousSurfaceId(
      const ElementId& element_id) const;
  std::shared_ptr<FrameSurface> GetPendingSurface(
      const ElementId& element_id) const;
  std::optional<FrameSurfaceId> GetPendingSurfaceId(
      const ElementId& element_id) const;
  // Declares the parent's currently expected `size_generation` for an element.
  // Only surfaces whose `size_generation` is not smaller than the expected
  // version will be allowed to become the active surface. A value of 0 clears
  // the expectation (no gating).
  void SetExpectedSizeGeneration(const ElementId& element_id,
                                 uint64_t size_generation);
  void RemoveSurfacesForElement(const ElementId& element_id);
  void Clear();

 private:
  struct SurfaceKey {
    uint64_t element_unique_id = 0;
    uint64_t incarnation = 0;
    uint64_t generation = 0;
    uint64_t size_generation = 0;

    bool operator==(const SurfaceKey& other) const {
      return element_unique_id == other.element_unique_id &&
             incarnation == other.incarnation &&
             generation == other.generation &&
             size_generation == other.size_generation;
    }

    bool operator<(const SurfaceKey& other) const {
      if (element_unique_id != other.element_unique_id) {
        return element_unique_id < other.element_unique_id;
      }
      if (incarnation != other.incarnation) {
        return incarnation < other.incarnation;
      }
      if (generation != other.generation) {
        return generation < other.generation;
      }
      return size_generation < other.size_generation;
    }
  };

  static SurfaceKey MakeSurfaceKey(const FrameSurfaceId& surface_id) {
    return {surface_id.element_id().unique_id(), surface_id.incarnation(),
            surface_id.generation(), surface_id.size_generation()};
  }
  std::shared_ptr<FrameSurface> SubmitSurfaceLocked(
      const FrameSurfaceId& surface_id, std::shared_ptr<LayerTree> layer_tree,
      const FloatSize& surface_size, std::optional<skity::Rect> damage_rect);
  std::shared_ptr<FrameSurface> AcquireSurfaceForParentFrameLocked(
      const FrameSurfaceId& surface_id);
  std::optional<skity::Rect> AccumulatePendingDamageLocked(
      const FrameSurfaceId& surface_id,
      std::optional<skity::Rect> damage_rect) const;
  uint64_t GetExpectedSizeGenerationLocked(uint64_t element_unique_id) const;
  void PruneExpiredSurfaceRefsLocked();

  mutable std::mutex mutex_;
  std::map<SurfaceKey, std::weak_ptr<FrameSurface>> surfaces_;
  std::map<uint64_t, std::shared_ptr<FrameSurface>> active_surfaces_;
  std::map<uint64_t, std::weak_ptr<FrameSurface>> previous_surfaces_;
  std::map<uint64_t, std::shared_ptr<FrameSurface>> pending_surfaces_;
  // The parent-declared expected `size_generation` per element. Guarded by
  // `mutex_`. Missing / zero entry means "no expectation" (gate disabled).
  std::map<uint64_t, uint64_t> expected_size_generations_;
};

}  // namespace clay

#endif  // CLAY_FLOW_FRAME_SURFACE_REGISTRY_H_
