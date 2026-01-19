// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef SERVICE_API_SERVICE_API_UTILS_H_
#define SERVICE_API_SERVICE_API_UTILS_H_

#include <string>
#include <utility>
#if defined(OS_IOS)
#include <service_api/service_lazy_load.h>
#endif

#define EXPORT_FUNC                                                \
  __attribute__((visibility("default"))) __attribute__((noinline)) \
  __attribute__((used))
#define EXPORT_VAR __attribute__((visibility("default"))) __attribute__((used))
#define EXPORT_CLASS __attribute__((visibility("default")))

#define DYLIB_ENTRY(name) __attribute__((constructor)) EXPORT_FUNC void name()

#if defined(OS_IOS)
#define _LYNX_SERVICE_ENTRY_FUNC(name) SERVICE_LAZY_LOAD_CPP(name)
#else
#define _LYNX_SERVICE_ENTRY_FUNC(name) DYLIB_ENTRY(name)
#endif

#ifndef BASE_CONCAT
#define BASE_CONCAT2(A, B) A##B
#define BASE_CONCAT(A, B) BASE_CONCAT2(A, B)
#endif

#ifdef LYNX_SERVICE_API_NEED_EXPORTS  // Only need the anchors in the
                                      // service_api module
#define _LYNX_SERVICE_ANCHOR(ServiceClass)                                  \
  /* Invoke get_registry<>() to trigger template instantiation and preserve \
   * the function in the service_api.so/service_api.o, preventing it from   \
   * being optimized away by the compiler or linker.                        \
   */                                                                       \
  namespace {                                                               \
  __attribute__((used)) void __##ServiceClass##_registry_f() {              \
    lynx::service::get_registry<ServiceClass>();                            \
  }                                                                         \
  }
#else
#define _LYNX_SERVICE_ANCHOR(ServiceClass)
#endif

/**
 * Defines a link anchor function that forces the linker to keep associated
 * code. This macro creates a dummy function marked with __attribute__((used))
 * to prevent the linker from optimizing it away.
 *
 * This pattern is commonly used to ensure that self-registering code (like
 * static initializers or factory registration) is linked into the final binary,
 * even when not directly referenced by other code.
 *
 * @param name - Identifier for the link anchor
 *
 * @example
 * @code{.cpp}
 *  // In a.cc which contains standalone self-registering code
 * DEFINE_LINK_ANCHOR(my_module)
 * // In b.cc which is directly referenced by the main program
 * USE_LINK_ANCHOR(my_module)
 * @endcode
 *
 * @see USE_LINK_ANCHOR
 */
#define DEFINE_LINK_ANCHOR(name) \
  __attribute__((used)) void __lynx_link_anchor_##name(void) {}

/**
 * References a previously defined link anchor to establish a dependency.
 *
 * @param name - Identifier matching a DEFINE_LINK_ANCHOR declaration
 *
 * @see DEFINE_LINK_ANCHOR
 */
#define USE_LINK_ANCHOR(name)                                        \
  extern void __lynx_link_anchor_##name(void);                       \
  __attribute__((used)) static void *__lynx_link_anchor_ref_##name = \
      (void *)&__lynx_link_anchor_##name

namespace lynx {
namespace service {
namespace utils {
/**
 * Get type name at compilation time without RTTI.
 */
template <typename T>
constexpr std::string_view type_name() {
#ifdef __clang__
  std::string_view name = __PRETTY_FUNCTION__;
  std::string_view prefix =
      "std::string_view lynx::service::utils::type_name() [T = ";
  std::string_view suffix = "]";
#elif defined(__GNUC__)
  std::string_view name = __PRETTY_FUNCTION__;
  std::string_view prefix =
      "constexpr std::string_view lynx::service::utils::type_name() "
      "[with T = ";
  std::string_view suffix = "]";
#elif defined(_MSC_VER)
  std::string_view name = __FUNCSIG__;
  std::string_view prefix =
      "class std::basic_string_view<char,struct std::char_traits<char> > "
      "__cdecl lynx::service::utils::type_name<";
  std::string_view suffix = ">(void)";
#endif

  name.remove_prefix(prefix.size());
  name.remove_suffix(suffix.size());
  return name;
}

template <typename T>
class NoDestructor {
 public:
  // Not constexpr; just write static constexpr T x = ...; if the value should
  // be a constexpr.
  template <typename... Args>
  explicit NoDestructor(Args &&...args) {
    new (storage_) T(std::forward<Args>(args)...);
  }

  // Allows copy and move construction of the contained type, to allow
  // construction from an initializer list, e.g. for std::vector.
  explicit NoDestructor(const T &x) { new (storage_) T(x); }

  explicit NoDestructor(T &&x) { new (storage_) T(std::move(x)); }

  NoDestructor(const NoDestructor &) = delete;

  NoDestructor &operator=(const NoDestructor &) = delete;

  ~NoDestructor() = default;

  const T &operator*() const { return *get(); }

  T &operator*() { return *get(); }

  const T *operator->() const { return get(); }

  T *operator->() { return get(); }

  const T *get() const { return reinterpret_cast<const T *>(storage_); }

  T *get() { return reinterpret_cast<T *>(storage_); }

 private:
  alignas(T) char storage_[sizeof(T)];

#if defined(LEAK_SANITIZER)
  // TODO(https://crbug.com/812277): This is a hack to work around the fact
  // that LSan doesn't seem to treat NoDestructor as a root for reachability
  // analysis. This means that code like this:
  //   static base::NoDestructor<std::vector<int>> v({1, 2, 3});
  // is considered a leak. Using the standard leak sanitizer annotations to
  // suppress leaks doesn't work: std::vector is implicitly constructed before
  // calling the base::NoDestructor constructor.
  //
  // Unfortunately, I haven't been able to demonstrate this issue in simpler
  // reproductions: until that's resolved, hold an explicit pointer to the
  // placement-new'd object in leak sanitizer mode to help LSan realize that
  // objects allocated by the contained type are still reachable.
  T *storage_ptr_ = reinterpret_cast<T *>(storage_);
#endif  // defined(LEAK_SANITIZER)
};
}  // namespace utils
}  // namespace service
}  // namespace lynx

#endif  // SERVICE_API_SERVICE_API_UTILS_H_
