// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FML_PLATFORM_DARWIN_SCOPED_NSOBJECT_H_
#define CLAY_FML_PLATFORM_DARWIN_SCOPED_NSOBJECT_H_

// Include NSObject.h directly because Foundation.h pulls in many dependencies.
// (Approx 100k lines of code versus 1.5k for NSObject.h). scoped_nsobject gets
// singled out because it is most typically included from other header files.
#import <Foundation/NSObject.h>

#include <utility>

#include "base/include/compiler_specific.h"
#include "base/include/fml/macros.h"

#if !__has_feature(objc_arc)
#error "ARC must be enabled"
#endif

namespace fml {

// We enable ARC on all source set.
// For compatibility with the exisiting codebase, we keep this class but
// implement it as an pure wrapper.

template <typename NST>
class scoped_nsprotocol {
 public:
  explicit scoped_nsprotocol(NST object = nil) : object_(object) {}
  // NOLINTNEXTLINE
  scoped_nsprotocol(const scoped_nsprotocol<NST>& that)
      : object_(that.object_) {}

  template <typename NSU>
  // NOLINTNEXTLINE
  scoped_nsprotocol(const scoped_nsprotocol<NSU>& that) : object_(that.get()) {}

  ~scoped_nsprotocol() = default;

  scoped_nsprotocol& operator=(const scoped_nsprotocol<NST>& that) {
    reset(that.get());
    return *this;
  }

  void reset(NST object = nil) {
    // We intentionally do not check that object != object_ as the caller must
    // either already have an ownership claim over whatever it passes to this
    // method, or call it with the |RETAIN| policy which will have ensured that
    // the object is retained once more when reaching this point.
    object_ = object;
  }

  bool operator==(NST that) const { return object_ == that; }
  bool operator!=(NST that) const { return object_ != that; }

  operator NST() const { return object_; }  // NOLINT

  NST get() const { return object_; }

  void swap(scoped_nsprotocol& that) {
    NST temp = that.object_;
    that.object_ = object_;
    object_ = temp;
  }

 private:
  NST object_;

  // scoped_nsprotocol<>::release() is like scoped_ptr<>::release.  It is NOT a
  // wrapper for [object_ release].  To force a scoped_nsprotocol<> to call
  // [object_ release], use scoped_nsprotocol<>::reset().
  [[nodiscard]] NST release() {
    NST temp = object_;
    object_ = nil;
    return temp;
  }
};

// Free functions
template <class C>
void swap(scoped_nsprotocol<C>& p1, scoped_nsprotocol<C>& p2) {
  p1.swap(p2);
}

template <class C>
bool operator==(C p1, const scoped_nsprotocol<C>& p2) {
  return p1 == p2.get();
}

template <class C>
bool operator!=(C p1, const scoped_nsprotocol<C>& p2) {
  return p1 != p2.get();
}

template <typename NST>
class scoped_nsobject : public scoped_nsprotocol<NST*> {
 public:
  // NOLINTNEXTLINE
  explicit scoped_nsobject(NST* object = nil)
      : scoped_nsprotocol<NST*>(object) {}
  // NOLINTNEXTLINE
  scoped_nsobject(const scoped_nsobject<NST>& that)
      : scoped_nsprotocol<NST*>(that) {}

  template <typename NSU>
  // NOLINTNEXTLINE
  scoped_nsobject(const scoped_nsobject<NSU>& that)
      : scoped_nsprotocol<NST*>(that) {}

  scoped_nsobject& operator=(const scoped_nsobject<NST>& that) {
    scoped_nsprotocol<NST*>::operator=(that);
    return *this;
  }
};

// Specialization to make scoped_nsobject<id> work.
template <>
class scoped_nsobject<id> : public scoped_nsprotocol<id> {
 public:
  explicit scoped_nsobject(id object = nil) : scoped_nsprotocol<id>(object) {}
  // NOLINTNEXTLINE
  scoped_nsobject(const scoped_nsobject<id>& that)
      : scoped_nsprotocol<id>(that) {}

  template <typename NSU>
  // NOLINTNEXTLINE
  scoped_nsobject(const scoped_nsobject<NSU>& that)
      : scoped_nsprotocol<id>(that) {}

  scoped_nsobject& operator=(const scoped_nsobject<id>& that) {
    scoped_nsprotocol<id>::operator=(that);
    return *this;
  }
};

}  // namespace fml

#endif  // CLAY_FML_PLATFORM_DARWIN_SCOPED_NSOBJECT_H_
