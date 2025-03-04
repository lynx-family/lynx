// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_INCLUDE_NO_DESTRUCTOR_H_
#define BASE_INCLUDE_NO_DESTRUCTOR_H_

#include <new>
#include <utility>

namespace lynx {
namespace base {

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

}  // namespace base
}  // namespace lynx

namespace fml {
using lynx::base::NoDestructor;
}

#endif  // BASE_INCLUDE_NO_DESTRUCTOR_H_
