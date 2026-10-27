// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/shared_image_sink_accessor.h"

#include <memory>
#include <utility>

#include "clay/fml/logging.h"
#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/shared_image_backing.h"
#include "clay/gfx/shared_image/shared_image_representation.h"
#include "clay/gfx/shared_image/shared_image_sink.h"

namespace clay {

SharedImageSinkAccessor::SharedImageSinkAccessor(
    fml::RefPtr<SharedImageSink> sink,
    const RepresentationFactory& repr_factory)
    : sink_(std::move(sink)), repr_factory_(repr_factory) {}

SharedImageSinkAccessor::~SharedImageSinkAccessor() {
  repr_texture_manager_ = nullptr;
  image_repr_cache_.clear();
  if (front_repr_) {
    ReleaseFront();
  }
  if (back_repr_) {
    SwapBack();
  }
}

fml::RefPtr<SharedImageRepresentation> SharedImageSinkAccessor::UpdateFront() {
  switch (sink_->update_front_mode()) {
    case SharedImageSink::UpdateFrontMode::kSequence:
      return UpdateFrontSequence();
    case SharedImageSink::UpdateFrontMode::kLatest:
      return UpdateFrontToLatest();
    default:
      return UpdateFrontSequence();
  }
}

fml::RefPtr<SharedImageRepresentation>
SharedImageSinkAccessor::UpdateFrontSequence() {
  std::unique_ptr<FenceSync> front_fence;
  if (front_repr_) {
    if (!front_repr_->EndRead()) {
      FML_LOG(ERROR) << "Failed to end read";
      return nullptr;
    }
    front_fence = front_repr_->ProduceFence();
    front_repr_ = nullptr;
  }
  fml::RefPtr<SharedImageBacking> backing =
      sink_->UpdateFront(std::move(front_fence));
  if (!backing) {
    return nullptr;
  }
  front_repr_ = GetRepresentation(backing.get());
  front_repr_->ConsumeFence(backing->GetFenceSync());
  return front_repr_;
}

fml::RefPtr<SharedImageRepresentation>
SharedImageSinkAccessor::UpdateFrontToLatest() {
  std::unique_ptr<FenceSync> front_fence;
  if (front_repr_) {
    front_fence = front_repr_->ProduceFence();
  }
  fml::RefPtr<SharedImageBacking> backing =
      sink_->UpdateFrontToLatest(std::move(front_fence));
  if (!backing) {
    return nullptr;
  }
  if (front_repr_ && backing.get() == front_repr_->GetBacking()) {
    return front_repr_;
  }
  if (front_repr_ && !front_repr_->EndRead()) {
    FML_LOG(ERROR) << "Failed to end read";
  }
  front_repr_ = GetRepresentation(backing.get());
  front_repr_->ConsumeFence(backing->GetFenceSync());
  return front_repr_;
}

void SharedImageSinkAccessor::ReleaseFront() {
  std::unique_ptr<FenceSync> front_fence;
  if (front_repr_) {
    if (!front_repr_->EndRead()) {
      FML_LOG(ERROR) << "Failed to end read";
      return;
    }
    front_fence = front_repr_->ProduceFence();
    front_repr_ = nullptr;
  }
  sink_->ReleaseFront(std::move(front_fence));
}

std::tuple<fml::RefPtr<SharedImageRepresentation>, uint32_t>
SharedImageSinkAccessor::AcquireBack(const skity::Vec2& size) {
  auto [backing, buffer_age] = sink_->AcquireBack(size);
  if (!backing) {
    return std::make_tuple(nullptr, 0);
  }
  back_repr_ = GetRepresentation(backing.get());
  back_repr_->ConsumeFence(backing->GetFenceSync());
  return std::make_tuple(back_repr_, buffer_age);
}

std::tuple<fml::RefPtr<SharedImageRepresentation>, uint32_t>
SharedImageSinkAccessor::AcquireBackForced(const skity::Vec2& size) {
  auto [backing, buffer_age] = sink_->AcquireBackForced(size);
  if (!backing) {
    return std::make_tuple(nullptr, 0);
  }
  back_repr_ = GetRepresentation(backing.get());
  back_repr_->ConsumeFence(backing->GetFenceSync());
  return std::make_tuple(back_repr_, buffer_age);
}

std::tuple<fml::RefPtr<SharedImageRepresentation>, uint32_t,
           SharedImageSink::AcquireBackStatus>
SharedImageSinkAccessor::TryAcquireBack(const skity::Vec2& size) {
  auto [backing, buffer_age, status] = sink_->TryAcquireBack(size);
  if (!backing) {
    return std::make_tuple(nullptr, 0, status);
  }
  back_repr_ = GetRepresentation(backing.get());
  back_repr_->ConsumeFence(backing->GetFenceSync());
  return std::make_tuple(back_repr_, buffer_age, status);
}

bool SharedImageSinkAccessor::SwapBack() {
  if (!back_repr_) {
    return false;
  }
  if (!back_repr_->EndWrite()) {
    FML_LOG(ERROR) << "Failed to end write";
    return false;
  }
  std::unique_ptr<FenceSync> fence = back_repr_->ProduceFence();
  if (!sink_->SwapBack(std::move(fence))) {
    return false;
  }
  back_repr_ = nullptr;
  return true;
}

fml::RefPtr<SharedImageRepresentation>
SharedImageSinkAccessor::GetRepresentation(SharedImageBacking* backing) {
  for (auto& [cached_backing, cached_repr] : image_repr_cache_) {
    if (cached_backing == backing) {
      return cached_repr;
    }
  }
  if (image_repr_cache_.size() >= sink_->Capacity() && sink_->Capacity() > 0) {
    image_repr_cache_.pop_front();
  }
  fml::RefPtr<SharedImageRepresentation> repr =
      repr_factory_(fml::Ref(backing));

  if (!repr_texture_manager_) {
    repr_texture_manager_ = repr->GetTextureManager();
  }
  repr->SetTextureManager(repr_texture_manager_);

  if (sink_->Capacity() > 0) {
    image_repr_cache_.emplace_back(backing, repr);
  }
  return repr;
}
}  // namespace clay
