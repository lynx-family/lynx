// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/frame_surface_registry.h"

#include <utility>

#include "clay/flow/layers/container_layer.h"
#include "clay/flow/layers/layer_tree.h"

namespace clay {
namespace {

bool AccumulateDrawableImageDamage(const Layer* layer,
                                   const PaintRegionMap& paint_regions,
                                   skity::Rect* damage) {
  if (!layer) {
    return false;
  }
  auto region_it = paint_regions.find(layer->unique_id());
  if (region_it == paint_regions.end() || !region_it->second.is_valid() ||
      !region_it->second.has_drawable_image()) {
    return false;
  }

  bool found_in_child = false;
  if (const auto* container = layer->as_container_layer()) {
    for (const auto& child : container->layers()) {
      found_in_child =
          AccumulateDrawableImageDamage(child.get(), paint_regions, damage) ||
          found_in_child;
    }
  }
  if (!found_in_child) {
    damage->Join(region_it->second.ComputeBounds());
  }
  return true;
}

std::optional<skity::Rect> ComputeDrawableImageDamage(
    const std::shared_ptr<LayerTree>& layer_tree) {
  if (!layer_tree || !layer_tree->root_layer()) {
    return std::nullopt;
  }
  skity::Rect damage = skity::Rect::MakeEmpty();
  if (!AccumulateDrawableImageDamage(layer_tree->root_layer(),
                                     layer_tree->paint_region_map(), &damage)) {
    return std::nullopt;
  }
  return damage;
}

}  // namespace

FrameSurfaceId::FrameSurfaceId(const ElementId& element_id,
                               uint64_t incarnation, uint64_t generation,
                               uint64_t size_generation)
    : element_id_(element_id),
      incarnation_(incarnation),
      generation_(generation),
      size_generation_(size_generation) {}

FrameSurface::FrameSurface(FrameSurfaceId surface_id,
                           std::shared_ptr<LayerTree> layer_tree,
                           const FloatSize& surface_size,
                           std::optional<skity::Rect> damage_rect)
    : surface_id_(std::move(surface_id)),
      layer_tree_(std::move(layer_tree)),
      surface_size_(surface_size),
      damage_rect_(damage_rect),
      drawable_image_damage_rect_(ComputeDrawableImageDamage(layer_tree_)) {}

FrameSurface::~FrameSurface() = default;

bool FrameSurfaceRegistry::SubmitPendingSurface(
    const FrameSurfaceId& surface_id, std::shared_ptr<LayerTree> layer_tree,
    const FloatSize& surface_size, std::optional<skity::Rect> damage_rect) {
  std::scoped_lock lock(mutex_);
  damage_rect = AccumulatePendingDamageLocked(surface_id, damage_rect);
  auto surface = SubmitSurfaceLocked(surface_id, std::move(layer_tree),
                                     surface_size, damage_rect);
  if (!surface) {
    return false;
  }
  pending_surfaces_.insert_or_assign(surface_id.element_id().unique_id(),
                                     std::move(surface));
  PruneExpiredSurfaceRefsLocked();
  return true;
}

bool FrameSurfaceRegistry::SubmitPendingSurface(
    const FrameSurfaceId& surface_id, std::unique_ptr<LayerTree> layer_tree,
    const FloatSize& surface_size, std::optional<skity::Rect> damage_rect) {
  return SubmitPendingSurface(surface_id,
                              std::shared_ptr<LayerTree>(std::move(layer_tree)),
                              surface_size, damage_rect);
}

std::shared_ptr<FrameSurface>
FrameSurfaceRegistry::AcquireSurfaceForParentFrame(
    const FrameSurfaceId& surface_id) {
  std::scoped_lock lock(mutex_);
  return AcquireSurfaceForParentFrameLocked(surface_id);
}

std::shared_ptr<FrameSurface>
FrameSurfaceRegistry::AcquireLatestSurfaceForParentFrame(
    const ElementId& element_id) {
  std::scoped_lock lock(mutex_);
  const uint64_t unique_id = element_id.unique_id();
  const uint64_t expected_size_generation =
      GetExpectedSizeGenerationLocked(unique_id);
  auto pending_it = pending_surfaces_.find(unique_id);
  if (pending_it != pending_surfaces_.end() &&
      pending_it->second->surface_id().size_generation() >=
          expected_size_generation) {
    return AcquireSurfaceForParentFrameLocked(pending_it->second->surface_id());
  }
  // Either no pending surface, or the pending surface is still behind parent's
  // expected size generation. Fall back to the currently active surface (if
  // any) and keep the old active state unchanged.
  auto active_it = active_surfaces_.find(unique_id);
  return active_it == active_surfaces_.end() ? nullptr : active_it->second;
}

std::shared_ptr<FrameSurface> FrameSurfaceRegistry::GetSurface(
    const FrameSurfaceId& surface_id) const {
  std::scoped_lock lock(mutex_);
  auto it = surfaces_.find(MakeSurfaceKey(surface_id));
  auto surface = it == surfaces_.end() ? nullptr : it->second.lock();
  return !surface || surface->surface_id() != surface_id ? nullptr : surface;
}

std::shared_ptr<FrameSurface> FrameSurfaceRegistry::GetActiveSurface(
    const ElementId& element_id) const {
  std::scoped_lock lock(mutex_);
  auto it = active_surfaces_.find(element_id.unique_id());
  return it == active_surfaces_.end() ? nullptr : it->second;
}

std::optional<FrameSurfaceId> FrameSurfaceRegistry::GetActiveSurfaceId(
    const ElementId& element_id) const {
  std::scoped_lock lock(mutex_);
  auto it = active_surfaces_.find(element_id.unique_id());
  return it == active_surfaces_.end()
             ? std::nullopt
             : std::make_optional(it->second->surface_id());
}

std::shared_ptr<FrameSurface> FrameSurfaceRegistry::GetPreviousSurface(
    const ElementId& element_id) const {
  std::scoped_lock lock(mutex_);
  auto it = previous_surfaces_.find(element_id.unique_id());
  return it == previous_surfaces_.end() ? nullptr : it->second.lock();
}

std::optional<FrameSurfaceId> FrameSurfaceRegistry::GetPreviousSurfaceId(
    const ElementId& element_id) const {
  std::scoped_lock lock(mutex_);
  auto it = previous_surfaces_.find(element_id.unique_id());
  auto surface = it == previous_surfaces_.end() ? nullptr : it->second.lock();
  return surface ? std::make_optional(surface->surface_id()) : std::nullopt;
}

std::shared_ptr<FrameSurface> FrameSurfaceRegistry::GetPendingSurface(
    const ElementId& element_id) const {
  std::scoped_lock lock(mutex_);
  auto it = pending_surfaces_.find(element_id.unique_id());
  return it == pending_surfaces_.end() ? nullptr : it->second;
}

std::optional<FrameSurfaceId> FrameSurfaceRegistry::GetPendingSurfaceId(
    const ElementId& element_id) const {
  std::scoped_lock lock(mutex_);
  auto it = pending_surfaces_.find(element_id.unique_id());
  return it == pending_surfaces_.end()
             ? std::nullopt
             : std::make_optional(it->second->surface_id());
}

void FrameSurfaceRegistry::RemoveSurfacesForElement(
    const ElementId& element_id) {
  std::scoped_lock lock(mutex_);
  const uint64_t unique_id = element_id.unique_id();
  active_surfaces_.erase(unique_id);
  previous_surfaces_.erase(unique_id);
  pending_surfaces_.erase(unique_id);
  expected_size_generations_.erase(unique_id);
  for (auto it = surfaces_.begin(); it != surfaces_.end();) {
    if (it->first.element_unique_id == unique_id) {
      it = surfaces_.erase(it);
    } else {
      ++it;
    }
  }
}

void FrameSurfaceRegistry::Clear() {
  std::scoped_lock lock(mutex_);
  active_surfaces_.clear();
  previous_surfaces_.clear();
  pending_surfaces_.clear();
  surfaces_.clear();
  expected_size_generations_.clear();
}

void FrameSurfaceRegistry::SetExpectedSizeGeneration(
    const ElementId& element_id, uint64_t size_generation) {
  std::scoped_lock lock(mutex_);
  const uint64_t unique_id = element_id.unique_id();
  if (size_generation == 0) {
    expected_size_generations_.erase(unique_id);
    return;
  }
  auto it = expected_size_generations_.find(unique_id);
  if (it == expected_size_generations_.end()) {
    expected_size_generations_.emplace(unique_id, size_generation);
    return;
  }
  // Version numbers are monotonically increasing; guard against accidental
  // regressions from out-of-order callers.
  if (size_generation > it->second) {
    it->second = size_generation;
  }
}

std::shared_ptr<FrameSurface> FrameSurfaceRegistry::SubmitSurfaceLocked(
    const FrameSurfaceId& surface_id, std::shared_ptr<LayerTree> layer_tree,
    const FloatSize& surface_size, std::optional<skity::Rect> damage_rect) {
  if (!layer_tree || !layer_tree->root_layer()) {
    return nullptr;
  }

  const SurfaceKey key = MakeSurfaceKey(surface_id);
  auto existing_it = surfaces_.find(key);
  if (existing_it != surfaces_.end() && !existing_it->second.expired()) {
    return nullptr;
  }

  auto surface = std::make_shared<FrameSurface>(
      surface_id, std::move(layer_tree), surface_size, damage_rect);
  surfaces_.insert_or_assign(key, surface);
  return surface;
}

std::shared_ptr<FrameSurface>
FrameSurfaceRegistry::AcquireSurfaceForParentFrameLocked(
    const FrameSurfaceId& surface_id) {
  const uint64_t unique_id = surface_id.element_id().unique_id();
  const uint64_t expected_size_generation =
      GetExpectedSizeGenerationLocked(unique_id);
  auto pending_it = pending_surfaces_.find(unique_id);
  if (pending_it != pending_surfaces_.end() &&
      pending_it->second->surface_id() == surface_id) {
    auto surface = pending_it->second;
    // Version gate: if the pending surface is still behind the parent's
    // expected `size_generation`, do NOT promote it. Keep the old active
    // surface in place so the compositor keeps rendering last-known-good
    // pixels until the child catches up.
    if (surface->surface_id().size_generation() < expected_size_generation) {
      auto active_it = active_surfaces_.find(unique_id);
      return active_it == active_surfaces_.end() ? nullptr : active_it->second;
    }
    auto active_it = active_surfaces_.find(unique_id);
    if (active_it != active_surfaces_.end() &&
        active_it->second->surface_id() != surface_id) {
      previous_surfaces_.insert_or_assign(unique_id, active_it->second);
    }
    active_surfaces_.insert_or_assign(unique_id, surface);
    pending_surfaces_.erase(pending_it);
    PruneExpiredSurfaceRefsLocked();
    return surface;
  }

  auto active_it = active_surfaces_.find(unique_id);
  if (active_it != active_surfaces_.end() &&
      active_it->second->surface_id() == surface_id) {
    return active_it->second;
  }
  return nullptr;
}

std::optional<skity::Rect> FrameSurfaceRegistry::AccumulatePendingDamageLocked(
    const FrameSurfaceId& surface_id,
    std::optional<skity::Rect> damage_rect) const {
  auto pending_it = pending_surfaces_.find(surface_id.element_id().unique_id());
  if (pending_it == pending_surfaces_.end()) {
    return damage_rect;
  }

  const auto& pending_surface = pending_it->second;
  const auto& pending_id = pending_surface->surface_id();
  if (pending_id.incarnation() != surface_id.incarnation() ||
      pending_id.size_generation() != surface_id.size_generation() ||
      !pending_surface->damage_rect() || !damage_rect) {
    return std::nullopt;
  }

  skity::Rect accumulated_damage = *pending_surface->damage_rect();
  accumulated_damage.Join(*damage_rect);
  return accumulated_damage;
}

uint64_t FrameSurfaceRegistry::GetExpectedSizeGenerationLocked(
    uint64_t element_unique_id) const {
  auto it = expected_size_generations_.find(element_unique_id);
  return it == expected_size_generations_.end() ? 0 : it->second;
}

void FrameSurfaceRegistry::PruneExpiredSurfaceRefsLocked() {
  for (auto it = surfaces_.begin(); it != surfaces_.end();) {
    if (it->second.expired()) {
      it = surfaces_.erase(it);
    } else {
      ++it;
    }
  }
  for (auto it = previous_surfaces_.begin(); it != previous_surfaces_.end();) {
    if (it->second.expired()) {
      it = previous_surfaces_.erase(it);
    } else {
      ++it;
    }
  }
}

}  // namespace clay
