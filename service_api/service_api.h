// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef SERVICE_API_SERVICE_API_H_
#define SERVICE_API_SERVICE_API_H_

#include <service_api/service_api_utils.h>
#include <service_api/service_lazy_load.h>

#include <functional>
#include <mutex>
#include <string>

/**
 * @brief Declare a service interface class and create its registry anchor.
 * @param Base The service interface class name.
 *
 * @par Example
 * @code{.cpp}
 * // service_api/services/a_service.h
 * #include "../service_api.h"
 * namespace lynx {
 * namespace service {
 * namespace a_service {
 *   class LYNX_SERVICE_DECLARE(AService) : public BaseService<AService> {
 *    public:
 *     virtual void aaa() = 0;
 *     ~AService() override = default;
 *   };
 * }  // namespace a_service
 * }  // namespace service
 * }  // namespace lynx
 * @endcode
 */
#define LYNX_SERVICE_DECLARE(Base) \
  Base;                            \
  _LYNX_SERVICE_ANCHOR(Base)       \
  class EXPORT_CLASS Base

/**
 * @brief Register implementation type `Impl` for service interface `Base`.
 * @param Impl The implementation type (must derive from `Base`).
 *
 * @par Example
 * @code{.cpp}
 * // impls/impl.cpp
 * using lynx::service::a_service::AService;
 * class AServiceImpl : public AService {
 *  public:
 *   void aaa() override { ... }
 * };
 * LYNX_SERVICE_REGISTER(AServiceImpl);
 * @endcode
 */
#define LYNX_SERVICE_REGISTER(Impl)                             \
  namespace {                                                   \
  _LYNX_SERVICE_ENTRY_FUNC(BASE_CONCAT(_lynx_service_register_, \
                                       __COUNTER__)) {          \
    Impl::__register_self<Impl>();                              \
  }                                                             \
  }

/**
 * @brief Register an implementation factory for service interface `Base`.
 * @param ImplCreator A callable that returns `Base*`.
 *
 * @par Example
 * @code{.cpp}
 * // impls/impl.cpp
 * using lynx::service::b_service::BService;
 * class BServiceImpl : public BService {
 *  public:
 *   explicit BServiceImpl(const std::string &desc) { ... }
 *   void bbb() override { ... }
 * };
 * LYNX_SERVICE_REGISTER_CREATOR({
 *   return new BServiceImpl(std::string("from so"));
 * });
 * @endcode
 */
#define LYNX_SERVICE_REGISTER_CREATOR(ImplCreator)                           \
  namespace {                                                                \
  auto creator() ImplCreator;                                                \
  _LYNX_SERVICE_ENTRY_FUNC(BASE_CONCAT(_lynx_service_register_,              \
                                       __COUNTER__)) {                       \
    using ReturnType = std::invoke_result_t<decltype(creator)>;              \
    using CleanType = std::remove_pointer_t<ReturnType>;                     \
    static_assert(std::is_base_of_v<lynx::service::_BaseService, CleanType>, \
                  "Service creator must return a service instance");         \
    lynx::service::register_service(creator);                                \
  }                                                                          \
  }

namespace lynx {
namespace service {
/**
 * @typedef ServiceId
 * @brief Unique identifier for a service interface type.
 */
using ServiceId = unsigned long;

template <typename Base>
static const std::string& get_service_name();

template <typename Base, typename Impl>
static void register_service();

class _BaseService {
 public:
  virtual ~_BaseService() = default;
};

/**
 * @brief Base class for service interface.
 * @par Example
 * @code{.cpp}
 * // service_api/services/a_service.h
 * #include "../service_api.h"
 * namespace lynx {
 * namespace service {
 * namespace a_service {
 *   class LYNX_SERVICE_DECLARE(AService) : public BaseService<AService> {
 *    public:
 *     virtual void aaa() = 0;
 *     ~AService() override = default;
 *   };
 * }  // namespace a_service
 * }  // namespace service
 * }  // namespace lynx
 * @endcode
 */
template <typename Abs>
class EXPORT_CLASS BaseService : public _BaseService {
 public:
  static const std::string& get_service_name() {
    return service::get_service_name<Abs>();
  }

  template <typename Impl>
  static void __register_self() {
    register_service<Abs, Impl>();
  }

  static void __register_self(std::function<Abs*()> creator) {
    register_service<Abs>(creator);
  }

  BaseService(const BaseService&) = delete;
  BaseService& operator=(const BaseService&) = delete;

 private:
  BaseService() = default;  // for CRTP
  friend Abs;
};

class EXPORT_CLASS _BaseRegistry {
 public:
  explicit _BaseRegistry() = default;
  _BaseRegistry(const _BaseRegistry&) = delete;
  _BaseRegistry& operator=(const _BaseRegistry&) = delete;
  virtual ~_BaseRegistry() = default;

  void set_service_name(const std::string& service_name) {
    this->name_ = service_name;
  }
  const std::string& get_service_name() { return this->name_; }
  void set_creator(std::function<_BaseService*()> c);
  virtual _BaseService* get();

 private:
  std::string name_{"<unnamed>"};
  std::mutex mutex_;
  std::function<_BaseService*()> creator_{nullptr};
  std::atomic<_BaseService*> impl_{nullptr};
};

/**
 * @class _Registry
 *
 * @brief The registry maintains and manages service instances for each service.
 *
 * @tparam Base The service interface class
 */
template <typename Base,
          typename = std::enable_if_t<std::is_base_of_v<_BaseService, Base>>>
class _Registry : public _BaseRegistry {
 public:
  using Creator = std::function<Base*()>;

  static _Registry& instance() {
    static utils::NoDestructor<_Registry> inst;
    return *inst;
  }

 private:
  _Registry() { set_service_name(std::string(utils::type_name<Base>())); }
  friend class utils::NoDestructor<_Registry>;
};

/**
 * @brief Core API to obtain the shared registry for a service interface.
 * @tparam S Service interface type.
 *
 * This function is the core of the service mechanism.
 * By exporting this symbol in the service_api module for each service,
 * multiple modules can access the same service registry through this function.
 *
 * For example:
 * - Dynamic linking: if the service_api module is compiled as a shared object
 * (e.g. Android and HarmonyOS), service_api.so will export the
 * get_registry<>() function, and
 * other shared objects depend on this exported symbol.
 * - Static linking: if the service_api module and other modules are compiled
 * together (e.g. iOS), there will be only one instance of the get_registry<>()
 * function for each service during linking.
 *
 * Helper APIs such as `get_service` and `get_service_name` are implemented
 * based on `get_registry`.
 */
template <typename S>
EXPORT_FUNC _Registry<S, void>& get_registry();

#ifdef LYNX_SERVICE_API_NEED_EXPORTS
// We need to provide the get_registry<>() implementation if we need to preserve
// the symbols in current compilation unit.
template <typename S>
EXPORT_FUNC _Registry<S, void>& get_registry() {
  return _Registry<S, void>::instance();
}
#endif

/**
 * @brief Get the implementation instance pointer for service interface.
 * @tparam Base Service interface type.
 * @return Pointer to `Base` or `nullptr` if not registered.
 * @note Returns the cached singleton instance if available.
 */
template <typename Base>
static Base* get_service() {
  return static_cast<Base*>(get_registry<Base>().get());
}

/**
 * @brief Get the service name string for a service interface type.
 * @tparam Base Service interface type.
 * @return Reference to the service name string.
 *
 * The service name of each service may not be unique but is fixed after
 * compilation.
 */
template <typename Base>
static const std::string& get_service_name() {
  return get_registry<Base>().get_service_name();
}

/**
 * @brief Register a custom creator for a service interface.
 * @tparam Base Service interface type.
 * @param creator A factory of implementation.
 *
 * The implementation instance will be created when the first time the
 * service is retrieved.
 */
template <typename Base>
static void register_service(typename _Registry<Base, void>::Creator creator) {
  get_registry<Base>().set_creator(creator);
}

/**
 * @brief Register an implementation type for a service interface.
 * @tparam Base Service interface type.
 * @tparam Impl Implementation type (must derive from `Base`).
 *
 * The implementation instance will be created when the first time the
 * service is retrieved.
 */
template <typename Base, typename Impl>
static void register_service() {
  register_service<Base>([] { return new Impl; });
}

/**
 * @brief Register an implementation for a service interface with a service
 * instance creator.
 * @tparam Impl Implementation type (must derive from `Base`). It can be
 * inferred by the creator type.
 *
 * The implementation instance will be created when the first time the
 * service is retrieved.
 *
 * @par Example
 * @code{.cpp}
 * AServiceImpl *creator() {
 *   return new AServiceImpl();
 * }
 * lynx::service::register_service(creator);
 * @endcode
 */
template <typename Impl>
static void register_service(Impl* (*creator)()) {
  Impl::__register_self(creator);
}
}  // namespace service
}  // namespace lynx

// Exports service declarations from service_api.h
#include <service_api/service_api_services.h>

#endif  // SERVICE_API_SERVICE_API_H_
