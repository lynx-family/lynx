// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_COMMON_SERVICE_SERVICE_H_
#define CLAY_COMMON_SERVICE_SERVICE_H_

#include <future>
#include <memory>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

#include "base/include/fml/memory/weak_ptr.h"
#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/fml/task_runner.h"
#include "clay/fml/logging.h"

namespace clay {
class Engine;
class PlatformView;
class Rasterizer;
class Shell;
}  // namespace clay

namespace clay {

enum class Owner : size_t {
  kPlatform = 0,
  kUI,
  kRaster,
  kIO,
};

enum class ServiceFlags : uint32_t {
  kNone = 0,
  kManualRegister = 1 << 0,
  // multi-thread service can get reference even not in the same thread
  kMultiThread = 1 << 2
};

template <typename T>
constexpr auto operator+(T e) noexcept
    -> std::enable_if_t<std::is_enum_v<T>, std::underlying_type_t<T>> {
  return static_cast<std::underlying_type_t<T>>(e);
}

inline constexpr ServiceFlags operator|(ServiceFlags lhs, ServiceFlags rhs) {
  return static_cast<ServiceFlags>(+lhs | +rhs);
}

inline constexpr ServiceFlags operator&(ServiceFlags lhs, ServiceFlags rhs) {
  return static_cast<ServiceFlags>(+lhs & +rhs);
}

template <Owner owner>
struct OwnerEnum {
  static constexpr Owner value = owner;
};

using Owners = std::tuple<OwnerEnum<Owner::kPlatform>, OwnerEnum<Owner::kUI>,
                          OwnerEnum<Owner::kRaster>, OwnerEnum<Owner::kIO>>;

struct PlatformServiceContext {
  clay::PlatformView* platform_view = nullptr;
  clay::Shell* shell = nullptr;
};

struct UIServiceContext {
  clay::Engine* engine = nullptr;
};

struct RasterServiceContext {
  clay::Rasterizer* rasterizer = nullptr;
  uint32_t page_unique_id = 0;
};

struct IOServiceContext {
  void* cookie = nullptr;
};

using ServiceContexts = std::tuple<PlatformServiceContext, UIServiceContext,
                                   RasterServiceContext, IOServiceContext>;

template <Owner owner>
using ServiceContext = std::tuple_element_t<+owner, ServiceContexts>;

namespace details {

template <template <typename> typename M, typename T>
struct tuple_transform;

template <template <typename> typename M, typename... Ts>
struct tuple_transform<M, std::tuple<Ts...>> {
  using type = std::tuple<typename M<Ts>::type...>;
};
}  // namespace details

// We use static analyzer to find it the puppet support sync call
template <Owner puppet_owner, Owner actor_owner>
struct puppet_can_sync_call_actor;

// Currently, only UI->Platform sync call is supported.
// MUST ensure puppet_owner->actor_owner doesn't create a cylic!
template <>
struct puppet_can_sync_call_actor<Owner::kUI, Owner::kPlatform> {
  static constexpr bool value = true;
};

template <>
struct puppet_can_sync_call_actor<Owner::kPlatform, Owner::kRaster> {
  static constexpr bool value = true;
};

template <>
struct puppet_can_sync_call_actor<Owner::kPlatform, Owner::kIO> {
  static constexpr bool value = true;
};

template <Owner puppet_owner, Owner actor_owner>
struct puppet_can_sync_call_actor {
  static constexpr bool value = false;
};

template <Owner owner>
struct owner_thread_override;

// In Clay's current implenmentation,
// UI thread is running on the same thread with the platform thread.
// To make `Act` more performant, we add this override.
template <>
struct owner_thread_override<Owner::kUI> {
  static constexpr Owner value = Owner::kPlatform;
};

template <Owner owner>
struct owner_thread_override {
  static constexpr Owner value = owner;
};

template <Owner a, Owner b>
struct is_owner_same_thread {
  static constexpr bool value =
      owner_thread_override<a>::value == owner_thread_override<b>::value;
};

class ServiceTaskRunners {
 public:
  template <typename OwnerEnum>
  struct map_owner_to_task_runner {
    using type = fml::RefPtr<fml::TaskRunner>;
  };

  using TaskRunnersTuple =
      typename details::tuple_transform<map_owner_to_task_runner, Owners>::type;

  explicit ServiceTaskRunners(TaskRunnersTuple task_runners)
      : task_runners_(std::move(task_runners)) {}

  template <Owner owner>
  const fml::RefPtr<fml::TaskRunner>& SelectTaskRunner() const {
    return std::get<+owner>(task_runners_);
  }

 private:
  TaskRunnersTuple task_runners_;
};

class ServiceManager;

template <Owner puppet_owner, typename>
class Puppet;

template <typename S, Owner owner, ServiceFlags flags = ServiceFlags::kNone>
class Service;

// https://stackoverflow.com/a/23348670
using ServiceId = uint64_t;

namespace details {

template <typename S, typename Enabled = void>
struct service_has_custom_service_id {
  static constexpr bool value = false;
};

template <typename S>
struct service_has_custom_service_id<
    S, std::enable_if_t<
           std::is_convertible_v<decltype(S::kServiceId), ServiceId>>> {
  static constexpr bool value = true;
};

}  // namespace details

template <typename S>
ServiceId
ServiceIdNoRTTI()  // this function is instantiated for every different type
{
  if constexpr (details::service_has_custom_service_id<S>::value) {
    return S::kServiceId;
  } else {
    // WARNING: works only inside one module: same type coming from different
    // module will have different value!
    static S* TypeUniqueMarker =
        nullptr;  // thus this static variable will be created for each
                  // TypeIdNoRTTI<T> separately
    return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(
        &TypeUniqueMarker));  // it's address is unique identifier of
                              // TypeIdNoRTTI<T> type
  }
}

class BaseService {
 public:
  ServiceId GetServiceId() const { return service_id_; }
  Owner GetServiceOwner() const { return service_owner_; }
  bool IsValid() const { return valid_; }

  BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(BaseService);

 protected:
  BaseService(ServiceId service_id, Owner service_owner);
  virtual ~BaseService();

 private:
  const ServiceId service_id_;
  const Owner service_owner_;
  bool valid_ = false;
  std::vector<lynx::base::MoveOnlyClosure<void, BaseService&>> pending_calls_;
  std::shared_ptr<ServiceTaskRunners> task_runners_;
  std::optional<fml::WeakPtrFactory<BaseService>> weak_factory_;

  // called in target thread
  void AddPendingCall(lynx::base::MoveOnlyClosure<void, BaseService&> call);

  void AttachServiceManager(ServiceManager& service_manager);
  void DetachServiceManager();

  friend class ServiceManager;
  template <typename, Owner, ServiceFlags>
  friend class Service;
};

// A marker class to mark the object's owner thread
template <Owner owner>
class ActorObject {
 public:
  static constexpr Owner kOwner = owner;
};

namespace details {

template <typename S>
static constexpr bool IsService = std::is_base_of_v<BaseService, S>;

template <typename T, typename Enabled = void>
struct is_multi_thread_service;

template <typename T>
struct is_multi_thread_service<T, std::enable_if_t<IsService<T>>> {
  static constexpr bool value = T::kIsMultiThread;
};

template <typename T, typename Enabled>
struct is_multi_thread_service : std::false_type {};

template <typename S>
static constexpr bool IsMultiThreadService = is_multi_thread_service<S>::value;

template <typename T, typename Enabled = void>
struct is_actor_object : std::false_type {};

template <typename T>
struct is_actor_object<
    T, std::enable_if_t<std::is_convertible_v<decltype(T::kOwner), Owner>>>
    : std::true_type {};

template <typename T>
struct remove_ptr;

template <typename T>
struct remove_ptr<std::unique_ptr<T>> {
  using type = T;
  static constexpr bool value = true;
};

template <typename T>
struct remove_ptr<std::shared_ptr<T>> {
  using type = T;
  static constexpr bool value = true;
};

template <typename T>
struct remove_ptr<fml::WeakPtr<T>> {
  using type = T;
  static constexpr bool value = true;
};

template <typename T>
struct remove_ptr {
  static constexpr bool value = false;
};

}  // namespace details

template <typename ImplPtr>
class Actor : public std::enable_shared_from_this<Actor<ImplPtr>> {
 public:
  using Impl = typename details::remove_ptr<ImplPtr>::type;
  static_assert(details::is_actor_object<Impl>::value, "Must be ActorObject");
  static constexpr Owner kOwner = Impl::kOwner;

  BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(Actor);

  ~Actor() {
    if (impl_) {
      // move impl to destruct on target thread
      fml::TaskRunner::RunNowOrPostTask(
          this->task_runners_->template SelectTaskRunner<kOwner>(),
          [impl = std::move(this->impl_)] { (void)impl; });
    }
  }

 private:
  template <bool check_thread>
  static std::shared_ptr<Actor> Create(
      std::shared_ptr<ServiceTaskRunners> task_runners, ImplPtr impl) {
    if constexpr (check_thread) {
      FML_DCHECK(task_runners->template SelectTaskRunner<kOwner>()
                     ->RunsTasksOnCurrentThread());
    }
    return std::shared_ptr<Actor>(
        new Actor(std::move(task_runners), std::move(impl)));
  }

  Actor(std::shared_ptr<ServiceTaskRunners> task_runners, ImplPtr impl)
      : task_runners_(task_runners),
        impl_(std::make_unique<ImplPtr>(std::move(impl))) {}

  template <Owner get_owner, typename I = Impl,
            typename = std::enable_if_t<details::IsMultiThreadService<I> ||
                                        get_owner == kOwner>>
  Impl& GetImpl() const {
    if constexpr (details::IsService<Impl> &&
                  !details::IsMultiThreadService<I>) {
      FML_DCHECK((**impl_).IsValid())
          << "Service is not valid, do not use -> or *, use Act() instead";
    }
    return **impl_;
  }

  // Post task automatically to the target thread
  template <Owner from_owner, typename F,
            typename = std::enable_if_t<std::is_invocable_v<F, Impl&>>>
  void Act(F&& func) const {
    auto c = [](const Actor& actor, F&& func) {
      FML_DCHECK(actor.impl_);
      if (auto& impl = *actor.impl_) {
        if constexpr (details::IsService<Impl>) {
          impl->TryCall(std::forward<F>(func));
        } else {
          func(*impl);
        }
      } else {
        FML_DLOG(ERROR) << "Actor::Act: Actor is not initialized";
      }
    };
    if constexpr (is_owner_same_thread<from_owner, kOwner>::value) {
      c(*this, std::forward<F>(func));
    } else {
      this->task_runners_->template SelectTaskRunner<kOwner>()->PostTask(
          [self = this->shared_from_this(), c = std::move(c),
           func = std::move(func)]() mutable { c(*self, std::move(func)); });
    }
  }

  template <Owner from_owner, typename F, typename C,
            typename = std::enable_if_t<
                std::is_invocable_v<C, std::invoke_result_t<F, Impl&>>>>
  void Act(F&& func, C&& callback) const {
    constexpr bool is_same_thread =
        is_owner_same_thread<from_owner, kOwner>::value;
    this->template Act<from_owner>(
        [task_runner =
             is_same_thread
                 ? nullptr
                 : this->task_runners_->template SelectTaskRunner<from_owner>(),
         func = std::move(func),
         callback = std::move(callback)](Impl& impl) mutable {
          if constexpr (is_same_thread) {
            (void)task_runner;
            callback(func(impl));
          } else {
            task_runner->PostTask([result = func(impl),
                                   callback = std::move(callback)]() mutable {
              callback(std::move(result));
            });
          }
        });
  }

  const std::shared_ptr<ServiceTaskRunners> task_runners_;
  // WeakPtr must be constructed and destructed on the same thread.
  // So we add unique_ptr<ImplPtr> to totally move ImplPtr to target thread
  std::unique_ptr<ImplPtr> impl_ = nullptr;

  template <Owner, typename>
  friend class Puppet;

  template <typename S, Owner, ServiceFlags>
  friend class Service;

  friend class ServiceManager;
};

template <Owner puppet_owner, typename T>
class Puppet {
 public:
  static_assert(details::IsService<T> || details::remove_ptr<T>::value,
                "T must be Service or ptr<ActorObject>");
  using ActorType =
      Actor<std::conditional_t<details::IsService<T>, std::shared_ptr<T>, T>>;
  using Impl = typename ActorType::Impl;

  static constexpr Owner kPuppetOwner = puppet_owner;
  static constexpr Owner kActorOwner = Impl::kOwner;

  Puppet() = default;
  // NOLINTNEXTLINE(google-explicit-constructor)
  Puppet(std::shared_ptr<ActorType> actor) : actor_(std::move(actor)) {
    CheckActor();
  }

  Puppet(const Puppet& other) { *this = other; }

  Puppet(Puppet&& other) { *this = std::move(other); }

  // Post task to the target thread
  // If puppet_owner == kActorOwner, use `->` instead.
  template <typename F,
            typename = std::enable_if_t<std::is_invocable_v<F, Impl&>>>
  void Act(F&& func) const {
    this->actor_->template Act<puppet_owner>(std::forward<F>(func));
  }

  // Post task to the target thread,
  // the callback is invoked in caller thread.
  // If puppet_owner == kActorOwner, use `->` instead.
  template <typename F, typename C,
            typename = std::enable_if_t<
                std::is_invocable_v<C, std::invoke_result_t<F, Impl&>>>>
  void Act(F&& func, C&& callback) const {
    this->actor_->template Act<puppet_owner>(std::forward<F>(func),
                                             std::forward<C>(callback));
  }

  template <typename F,
            typename = std::enable_if_t<
                puppet_can_sync_call_actor<puppet_owner, kActorOwner>::value &&
                std::is_invocable_v<F, Impl&>>,
            typename Result = std::invoke_result_t<F, Impl&>>
  std::future<std::optional<Result>> ActWithPromise(F&& f) const {
    struct no_except_promise {
      explicit no_except_promise(std::promise<std::optional<Result>> p)
          : p(std::move(p)) {}
      ~no_except_promise() {
        if (!has_set_value) {
          p.set_value(std::optional<Result>{});
        }
      }
      void SetValue(Result&& value) {
        FML_DCHECK(!has_set_value);
        p.set_value(std::optional<Result>{std::forward<Result>(value)});
        has_set_value = true;
      }
      bool has_set_value = false;
      std::promise<std::optional<Result>> p;
    };
    std::promise<std::optional<Result>> promise;
    auto future = promise.get_future();
    this->Act([p = std::make_unique<no_except_promise>(std::move(promise)),
               f = std::move(f)](Impl& impl) { p->SetValue(f(impl)); });

    return future;
  }

  // Directly call the method,
  template <typename I = Impl,
            typename = std::enable_if_t<details::IsMultiThreadService<I> ||
                                        kPuppetOwner == kActorOwner>>
  Impl* operator->() const {
    return &this->actor_->template GetImpl<puppet_owner>();
  }

  template <typename I = Impl,
            typename = std::enable_if_t<details::IsMultiThreadService<I> ||
                                        kPuppetOwner == kActorOwner>>
  Impl& operator*() const {
    return this->actor_->template GetImpl<puppet_owner>();
  }

  explicit operator bool() const { return actor_ != nullptr; }

  Puppet& operator=(const Puppet& other) {
    actor_ = other.actor_;
    CheckActor();
    return *this;
  }

  Puppet& operator=(Puppet&& other) {
    actor_ = std::move(other.actor_);
    CheckActor();
    return *this;
  }

  void operator=(std::nullptr_t) { actor_ = nullptr; }

  void operator=(const std::shared_ptr<ActorType>& actor) {
    actor_ = actor;
    CheckActor();
  }

  void operator=(std::shared_ptr<ActorType>&& actor) {
    actor_ = std::move(actor);
    CheckActor();
  }

  template <typename C,
            typename = std::enable_if_t<std::is_invocable_v<C, Impl&>>,
            typename Result = std::invoke_result_t<C, Impl&>,
            typename A = Actor<Result>>
  std::shared_ptr<A> CreateObjectInActorThread(C&& ctor) {
    std::shared_ptr<A> actor =
        A::template Create<false>(this->actor_->task_runners_, nullptr);
    this->Act([actor, ctor = std::move(ctor)](auto& impl) mutable {
      actor->impl_ = std::make_unique<Result>(ctor(impl));
    });

    return actor;
  }

  template <typename ActorImplPtr, typename F, typename A = Actor<ActorImplPtr>,
            typename = std::enable_if_t<
                std::is_invocable_v<F, Impl&, std::shared_ptr<A>>>>
  void PostObjectToActorThread(ActorImplPtr actor_impl, F&& fn) {
    static_assert(!std::is_pointer_v<ActorImplPtr>,
                  "do not use raw pointers, use std::unique_ptr, "
                  "std::shared_ptr or fml::WeakPtr");
    std::shared_ptr<A> actor = A::template Create<true>(
        this->actor_->task_runners_, std::move(actor_impl));
    this->Act([actor = std::move(actor), fn = std::move(fn)](
                  auto& impl) mutable { fn(impl, std::move(actor)); });
  }

 private:
  void CheckActor() {
    FML_DCHECK(!this->actor_ || this->actor_->task_runners_
                                    ->template SelectTaskRunner<puppet_owner>()
                                    ->RunsTasksOnCurrentThread());
  }

  std::shared_ptr<ActorType> actor_;

  template <typename S, Owner, ServiceFlags>
  friend class Service;
};

template <Owner owner>
class BaseServiceWithLifeCycle : public BaseService, public ActorObject<owner> {
 public:
  using ContextType = ServiceContext<owner>;

 protected:
  explicit BaseServiceWithLifeCycle(ServiceId id) : BaseService(id, owner) {}

 private:
  friend class BaseService;

  virtual void OnInit(ServiceManager& service_manager, const ContextType&) {}
  virtual void OnDestroy() {}
};

template <typename S, Owner owner, ServiceFlags flags>
class Service : public BaseServiceWithLifeCycle<owner> {
 public:
  using Impl = S;
  static constexpr bool kAutoInit =
      (flags & ServiceFlags::kManualRegister) == ServiceFlags::kNone;
  static constexpr bool kIsMultiThread =
      (flags & ServiceFlags::kMultiThread) != ServiceFlags::kNone;

  Service() : BaseServiceWithLifeCycle<owner>(ServiceIdNoRTTI<S>()) {}

 protected:
  fml::WeakPtr<S> GetWeakPtr() const {
    FML_DCHECK(this->weak_factory_.has_value()) << "Service is invalid now!";
    return this->weak_factory_->GetWeakPtr();
  }

 private:
  template <typename F, typename = std::enable_if_t<std::is_invocable_v<F, S&>>>
  void TryCall(F&& f) {
    if (this->valid_) {
      f(static_cast<S&>(*this));
    } else {
      this->AddPendingCall([f = std::move(f)](BaseService& service) mutable {
        f(static_cast<S&>(service));
      });
    }
  }

  template <typename>
  friend class Actor;
};

}  // namespace clay

#endif  // CLAY_COMMON_SERVICE_SERVICE_H_
