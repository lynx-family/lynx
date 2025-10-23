// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_COMMON_SERVICE_SERVICE_MANAGER_H_
#define CLAY_COMMON_SERVICE_SERVICE_MANAGER_H_

#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "clay/common/service/service.h"
#include "clay/common/task_runners.h"

namespace clay {

class ServiceManager : public std::enable_shared_from_this<ServiceManager> {
 public:
  using ServiceCreateFn = std::shared_ptr<BaseService> (*)();

  static std::shared_ptr<ServiceManager> Create(
      // TaskRunners is a tuple<fml::RefPtr<fml::TaskRunner>...>
      ServiceTaskRunners::TaskRunnersTuple task_runners);

  ~ServiceManager();

  template <typename T, typename = std::enable_if_t<details::IsService<T>>,
            typename A = Actor<std::shared_ptr<T>>>
  std::shared_ptr<A> GetService() {
    ServiceCreateFn create_fn = nullptr;
    if constexpr (T::kAutoInit) {
      create_fn = []() -> std::shared_ptr<BaseService> { return T::Create(); };
    }
    std::shared_ptr<T> service = std::static_pointer_cast<T>(
        GetService(ServiceIdNoRTTI<T>(), create_fn));
    return service ? A::template Create<false>(this->task_runners_, service)
                   : nullptr;
  }

  template <typename T,
            typename = std::enable_if_t<details::IsMultiThreadService<T>>>
  std::shared_ptr<T> GetMultiThreadService() {
    ServiceCreateFn create_fn = nullptr;
    if constexpr (T::kAutoInit) {
      create_fn = []() -> std::shared_ptr<BaseService> { return T::Create(); };
    }
    std::shared_ptr<T> service = std::static_pointer_cast<T>(
        GetService(ServiceIdNoRTTI<T>(), create_fn));
    return service;
  }

  template <typename T, typename I,
            typename = std::enable_if_t<std::is_base_of_v<BaseService, T> &&
                                        std::is_base_of_v<T, I>>>
  void RegisterService(std::shared_ptr<I> impl) {
    RegisterService(ServiceIdNoRTTI<T>(), std::move(impl));
  }

  template <Owner owner>
  void Attach(ServiceContext<owner> ctx);

  template <Owner owner>
  void Detach();

  const std::shared_ptr<ServiceTaskRunners>& GetTaskRunners() const {
    return task_runners_;
  }

  const auto& GetContexts() const { return contexts_; }

 private:
  explicit ServiceManager(std::shared_ptr<ServiceTaskRunners> task_runners);

  std::shared_ptr<BaseService> GetService(ServiceId id,
                                          ServiceCreateFn create_fn);

  void RegisterService(ServiceId id, std::shared_ptr<BaseService> service);

  std::unordered_map<ServiceId, std::shared_ptr<BaseService>> services_by_id_;
  std::shared_mutex mutex_;

  std::shared_ptr<ServiceTaskRunners> task_runners_;

  template <typename OwnerEnum>
  struct map_owner_to_optional_context {
    using type = std::optional<ServiceContext<OwnerEnum::value>>;
  };
  details::tuple_transform<map_owner_to_optional_context, Owners>::type
      contexts_;

  template <typename OwnerEnum>
  struct map_owner_to_services_with_life_cycle {
    using type = std::vector<
        std::shared_ptr<BaseServiceWithLifeCycle<OwnerEnum::value>>>;
  };
  details::tuple_transform<map_owner_to_services_with_life_cycle, Owners>::type
      services_by_owner_;

  void TryInitService(std::shared_ptr<BaseService> service);

  template <Owner owner>
  void DoServiceInit(BaseServiceWithLifeCycle<owner>& service);

  [[maybe_unused]] void CreateTemplateSpecificationToPreventSymbolErrors();
};

}  // namespace clay

#endif  // CLAY_COMMON_SERVICE_SERVICE_MANAGER_H_
