// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/common/service/service_manager.h"

#include "clay/common/service/service.h"

namespace clay {

std::shared_ptr<ServiceManager> ServiceManager::Create(
    ServiceTaskRunners::TaskRunnersTuple task_runners) {
  return std::shared_ptr<ServiceManager>(new ServiceManager(
      std::make_shared<ServiceTaskRunners>(std::move(task_runners))));
}

ServiceManager::ServiceManager(std::shared_ptr<ServiceTaskRunners> task_runners)
    : task_runners_(std::move(task_runners)) {}

ServiceManager::~ServiceManager() {
  std::apply(
      [](auto&... ctx) {
        ([&] { FML_DCHECK(!ctx.has_value()) << "Not detached!"; }(), ...);
      },
      contexts_);
}

std::shared_ptr<BaseService> ServiceManager::GetService(
    ServiceId id, ServiceCreateFn create_fn) {
  {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = services_by_id_.find(id);
    if (it != services_by_id_.end()) {
      return it->second;
    }
  }

  std::shared_ptr<BaseService> service;

  {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    auto it = services_by_id_.find(id);
    if (it != services_by_id_.end()) {
      return it->second;
    }
    if (!create_fn) {
      return nullptr;
    }

    service = create_fn();
    if (!service) {
      return nullptr;
    }
    service->task_runners_ = task_runners_;

    auto [_, inserted] = services_by_id_.try_emplace(id, service);
    FML_DCHECK(inserted);
  }

  TryInitService(service);

  return service;
}

void ServiceManager::RegisterService(ServiceId id,
                                     std::shared_ptr<BaseService> service) {
  FML_DCHECK(service);

  {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    service->task_runners_ = task_runners_;

    auto [_, inserted] = services_by_id_.try_emplace(id, service);
    FML_DCHECK(inserted);
  }

  TryInitService(service);
}

template <Owner owner>
void ServiceManager::Attach(ServiceContext<owner> context) {
  FML_DCHECK(this->task_runners_->SelectTaskRunner<owner>()
                 ->RunsTasksOnCurrentThread());
  auto& ctx = std::get<+owner>(contexts_);
  FML_DCHECK(!ctx.has_value());
  ctx.emplace(std::move(context));

  for (auto& service : std::get<+owner>(services_by_owner_)) {
    DoServiceInit(*service);
  }
}

template <Owner owner>
void ServiceManager::Detach() {
  FML_DCHECK(this->task_runners_->SelectTaskRunner<owner>()
                 ->RunsTasksOnCurrentThread());
  std::vector<std::shared_ptr<BaseServiceWithLifeCycle<owner>>>& services =
      std::get<+owner>(services_by_owner_);
  for (auto& service : services) {
    service->DetachServiceManager();
  }
  std::get<+owner>(contexts_).reset();
  services.clear();
}

#if OS_LINUX || OS_MAC || OS_WIN
template void ServiceManager::Attach<Owner::kPlatform>(
    ServiceContext<Owner::kPlatform> context);
template void ServiceManager::Attach<Owner::kUI>(
    ServiceContext<Owner::kUI> context);
template void ServiceManager::Attach<Owner::kRaster>(
    ServiceContext<Owner::kRaster> context);
template void ServiceManager::Attach<Owner::kIO>(
    ServiceContext<Owner::kIO> context);

template void ServiceManager::Detach<Owner::kPlatform>();
template void ServiceManager::Detach<Owner::kUI>();
template void ServiceManager::Detach<Owner::kRaster>();
template void ServiceManager::Detach<Owner::kIO>();
#endif

void ServiceManager::TryInitService(std::shared_ptr<BaseService> service) {
  std::apply(
      [&](auto... arg) {
        (
            [&] {
              constexpr Owner owner = decltype(arg)::value;
              if (service->GetServiceOwner() == owner) {
                fml::TaskRunner::RunNowOrPostTask(
                    task_runners_->SelectTaskRunner<owner>(),
                    [service, self = shared_from_this()] {
                      // The order is important!
                      // If ServiceManager is not attached to A thread,
                      // Dependent services will be added before this service,
                      // so that dep's init happens before this.
                      self->DoServiceInit<owner>(
                          static_cast<BaseServiceWithLifeCycle<owner>&>(
                              *service));
                      std::get<+owner>(self->services_by_owner_)
                          .emplace_back(
                              std::static_pointer_cast<
                                  BaseServiceWithLifeCycle<owner>>(service));
                    });
              }
            }(),
            ...);
      },
      Owners());
}

template <Owner owner>
void ServiceManager::DoServiceInit(BaseServiceWithLifeCycle<owner>& service) {
  FML_DCHECK(service.GetServiceOwner() == owner);
  if (std::optional<ServiceContext<owner>>& ctx = std::get<+owner>(contexts_);
      ctx.has_value()) {
    FML_DCHECK(!service.valid_);
    service.AttachServiceManager(*this);
  }
}

[[maybe_unused]] void
ServiceManager::CreateTemplateSpecificationToPreventSymbolErrors() {
  std::apply(
      [&](auto... arg) {
        (
            [&] {
              constexpr Owner owner = decltype(arg)::value;
              Attach<owner>(ServiceContext<owner>{});
              Detach<owner>();
            }(),
            ...);
      },
      Owners());
}

}  // namespace clay
