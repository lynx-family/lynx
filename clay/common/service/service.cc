// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/common/service/service.h"

#include "clay/common/service/service_manager.h"

namespace clay {

BaseService::BaseService(ServiceId service_id, Owner service_owner)
    : service_id_(service_id), service_owner_(service_owner) {}

BaseService::~BaseService() = default;

void BaseService::AddPendingCall(
    lynx::base::MoveOnlyClosure<void, BaseService&> call) {
  pending_calls_.emplace_back(std::move(call));
}

void BaseService::AttachServiceManager(ServiceManager& service_manager) {
  task_runners_ = service_manager.GetTaskRunners();
  valid_ = true;

  weak_factory_.emplace(this);

  std::apply(
      [&](auto... arg) {
        (
            [&] {
              constexpr Owner owner = decltype(arg)::value;
              if (owner == service_owner_) {
                static_cast<BaseServiceWithLifeCycle<owner>*>(this)->OnInit(
                    service_manager,
                    *std::get<+owner>(service_manager.GetContexts()));
              }
            }(),
            ...);
      },
      Owners());

  for (auto& call : pending_calls_) {
    call(*this);
  }

  pending_calls_.clear();
}

void BaseService::DetachServiceManager() {
  std::apply(
      [&](auto... arg) {
        (
            [&] {
              constexpr Owner owner = decltype(arg)::value;
              if (owner == service_owner_) {
                static_cast<BaseServiceWithLifeCycle<owner>*>(this)
                    ->OnDestroy();
              }
            }(),
            ...);
      },
      Owners());

  task_runners_ = nullptr;
  weak_factory_.reset();
  valid_ = false;
}

}  // namespace clay
