// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/common/graphics/drawable_image.h"

#include <atomic>
#include <utility>

#include "clay/fml/logging.h"

namespace clay {

namespace {

uint64_t NextUniqueID() {
  static std::atomic<uint64_t> next_id(1);
  uint64_t id;
  do {
    id = next_id.fetch_add(1);
  } while (id == 0);  // 0 is reserved for an invalid id.
  return id;
}

}  // namespace

ContextListener::ContextListener() = default;

ContextListener::~ContextListener() = default;

DrawableImage::DrawableImage() : id_(NextUniqueID()) {}

DrawableImage::~DrawableImage() = default;

DrawableImageRegistry::DrawableImageRegistry() = default;

void DrawableImageRegistry::RegisterDrawableImage(
    const std::shared_ptr<DrawableImage>& image) {
  if (!image) {
    return;
  }
  mapping_[image->Id()] = image;
}

void DrawableImageRegistry::RegisterContextListener(
    uintptr_t id, std::weak_ptr<ContextListener> image) {
  images_[id] = std::move(image);
}

void DrawableImageRegistry::UnregisterDrawableImage(int64_t id) {
  auto found = mapping_.find(id);
  if (found == mapping_.end()) {
    return;
  }
  found->second->OnDrawableImageUnregistered();
  mapping_.erase(found);
}

void DrawableImageRegistry::UnregisterContextListener(uintptr_t id) {
  images_.erase(id);
}

void DrawableImageRegistry::OnGrContextCreated() {
  for (auto& it : mapping_) {
    it.second->OnGrContextCreated();
  }

  for (const auto& [id, weak_image] : images_) {
    if (auto image = weak_image.lock()) {
      image->OnGrContextCreated();
    } else {
      images_.erase(id);
    }
  }
}

void DrawableImageRegistry::OnGrContextDestroyed() {
  for (auto& it : mapping_) {
    it.second->OnGrContextDestroyed();
  }

  for (const auto& [id, weak_image] : images_) {
    if (auto image = weak_image.lock()) {
      image->OnGrContextDestroyed();
    } else {
      images_.erase(id);
    }
  }
}

std::shared_ptr<DrawableImage> DrawableImageRegistry::GetDrawableImage(
    int64_t id) {
  auto it = mapping_.find(id);
  return it != mapping_.end() ? it->second : nullptr;
}

}  // namespace clay
