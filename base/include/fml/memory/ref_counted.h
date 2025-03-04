// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// Provides a base class for reference-counted classes.

#ifndef BASE_INCLUDE_FML_MEMORY_REF_COUNTED_H_
#define BASE_INCLUDE_FML_MEMORY_REF_COUNTED_H_

#include <type_traits>

#include "base/include/fml/macros.h"
#include "base/include/fml/memory/ref_counted_internal.h"
#include "base/include/fml/memory/ref_ptr.h"

namespace lynx {
namespace fml {

// A base class for (thread-safe) reference-counted classes. Use like:
//
//   class Foo : public RefCountedThreadSafe<Foo> {
//     ...
//   };
//
// |~Foo()| *may* be made private (e.g., to avoid accidental deletion of objects
// while there are still references to them), |Foo| should friend
// |RefCountedThreadSafe<Foo>|; use |FML_FRIEND_REF_COUNTED_THREAD_SAFE()|
// for this:
//
//   class Foo : public RefCountedThreadSafe<Foo> {
//     ...
//    private:
//     FML_FRIEND_REF_COUNTED_THREAD_SAFE(Foo);
//     ~Foo();
//     ...
//   };
//
// Similarly, |Foo(...)| may be made private. In this case, there should either
// be a static factory method performing the requisite adoption:
//
//   class Foo : public RefCountedThreadSafe<Foo> {
//     ...
//    public:
//     inline static RefPtr<Foo> Create() { return AdoptRef(new Foo()); }
//     ...
//    private:
//     Foo();
//     ...
//   };
//
// Or, to allow |MakeRefCounted()| to be used, use
// |FML_FRIEND_MAKE_REF_COUNTED()|:
//
//   class Foo : public RefCountedThreadSafe<Foo> {
//     ...
//    private:
//     FML_FRIEND_MAKE_REF_COUNTED(Foo);
//     Foo();
//     Foo(const Bar& bar, bool maybe);
//     ...
//   };
//
// For now, we only have thread-safe reference counting, since that's all we
// need. It's easy enough to add thread-unsafe versions if necessary.
template <typename T>
class RefCountedThreadSafe : public internal::RefCountedThreadSafeBase {
 public:
  // Adds a reference to this object.
  // Inherited from the internal superclass:
  //   void AddRef() const;

  // Releases a reference to this object. This will destroy this object once the
  // last reference is released.
  void Release() const {
    if (internal::RefCountedThreadSafeBase::Release()) {
      delete static_cast<const T*>(this);
    }
  }

  // Returns true if there is exactly one reference to this object. Use of this
  // is subtle, and should usually be avoided. To assert that there is only one
  // reference (typically held by the calling thread, possibly in a local
  // variable), use |AssertHasOneRef()| instead. However, a use is:
  //
  //   if (foo->HasOneRef()) {
  //     // Do something "fast", since |foo| is the only reference and no other
  //     // thread can get another reference.
  //     ...
  //   } else {
  //     // Do something slower, but still valid even if there were only one
  //     // reference.
  //     ...
  //   }
  //
  // Inherited from the internal superclass:
  //   bool HasOneRef();

  // Asserts that there is exactly one reference to this object; does nothing in
  // Release builds (when |NDEBUG| is defined).
  // Inherited from the internal superclass:
  //   void AssertHasOneRef();

 protected:
  // Constructor. Note that the object is constructed with a reference count of
  // 1, and then must be adopted (see |AdoptRef()| in ref_ptr.h).
  RefCountedThreadSafe() {}

  // Destructor. Note that this object should only be destroyed via |Release()|
  // (see above), or something that calls |Release()| (see, e.g., |RefPtr<>| in
  // ref_ptr.h).
  ~RefCountedThreadSafe() {}

 private:
#ifndef NDEBUG
  template <typename U>
  friend RefPtr<U> AdoptRef(U*);
  // Marks the initial reference (assumed on construction) as adopted. This is
  // only required for Debug builds (when |NDEBUG| is not defined).
  // TODO(vtl): Should this really be private? This makes manual ref-counting
  // and also writing one's own ref pointer class impossible.
  void Adopt() { internal::RefCountedThreadSafeBase::Adopt(); }
#endif

  BASE_DISALLOW_COPY_AND_ASSIGN(RefCountedThreadSafe);
};

// If you subclass |RefCountedThreadSafe| and want to keep your destructor
// private, use this. (See the example above |RefCountedThreadSafe|.)
#define FML_FRIEND_REF_COUNTED_THREAD_SAFE(T) \
  friend class lynx::fml::RefCountedThreadSafe<T>

// If you want to keep your constructor(s) private and still want to use
// |MakeRefCounted<T>()|, use this. (See the example above
// |RefCountedThreadSafe|.)
#define FML_FRIEND_MAKE_REF_COUNTED(T) \
  friend class lynx::fml::internal::MakeRefCountedHelper<T>

// RefCountedThreadSafe<T> supports compile time polymorphism destroy by delete
// static_cast<const T*>(this). But not support runtime polymorphism destroy
// since RefCountedThreadSafe<T> is not abstruct and destructor is not virtual.
// Use RefCountedThreadSafeStorage if need delete subclass by ref to superclass
// with virtual ReleaseSelf.
class RefCountedThreadSafeStorage
    : public fml::RefCountedThreadSafe<RefCountedThreadSafeStorage> {
 public:
  void Release() const {
    if (fml::internal::RefCountedThreadSafeBase::Release()) {
      ReleaseSelf();
    }
  }

 protected:
  virtual void ReleaseSelf() const = 0;
  virtual ~RefCountedThreadSafeStorage() = default;
};

// static_cast from base to derived
template <typename D, typename B = RefCountedThreadSafeStorage,
          typename = std::enable_if_t<std::is_base_of<B, D>::value, D>>
RefPtr<D> static_ref_ptr_cast(const RefPtr<B>& rhs) {
  return Ref<D>(static_cast<D*>(rhs.get()));
}

}  // namespace fml
}  // namespace lynx

namespace fml {
using lynx::fml::RefCountedThreadSafe;
}  // namespace fml

#endif  // BASE_INCLUDE_FML_MEMORY_REF_COUNTED_H_
