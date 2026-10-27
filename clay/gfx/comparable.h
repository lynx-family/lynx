// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_COMPARABLE_H_
#define CLAY_GFX_COMPARABLE_H_

#include <memory>

namespace clay {

// These templates implement deep pointer comparisons that compare not
// just the pointers to the objects, but also their contents (provided
// that the <T> class implements the == operator override).
// Any combination of shared_ptr<T> or T* are supported and null pointers
// are not equal to anything but another null pointer.

template <class T>
bool Equals(const T* a, const T* b) {
  if (a == b) {
    return true;
  }
  if (!a || !b) {
    return false;
  }
  return *a == *b;
}

template <class T>
bool Equals(std::shared_ptr<const T> a, const T* b) {
  return Equals(a.get(), b);
}

template <class T>
bool Equals(std::shared_ptr<T> a, const T* b) {
  return Equals(a.get(), b);
}

template <class T>
bool Equals(const T* a, std::shared_ptr<const T> b) {
  return Equals(a, b.get());
}

template <class T>
bool Equals(const T* a, std::shared_ptr<T> b) {
  return Equals(a, b.get());
}

template <class T>
bool Equals(std::shared_ptr<const T> a, std::shared_ptr<const T> b) {
  return Equals(a.get(), b.get());
}

template <class T>
bool Equals(std::shared_ptr<T> a, std::shared_ptr<const T> b) {
  return Equals(a.get(), b.get());
}

template <class T>
bool Equals(std::shared_ptr<const T> a, std::shared_ptr<T> b) {
  return Equals(a.get(), b.get());
}

template <class T>
bool Equals(std::shared_ptr<T> a, std::shared_ptr<T> b) {
  return Equals(a.get(), b.get());
}

template <class T>
bool NotEquals(const T* a, const T* b) {
  return !Equals<T>(a, b);
}

template <class T>
bool NotEquals(std::shared_ptr<const T> a, const T* b) {
  return !Equals(a.get(), b);
}

template <class T>
bool NotEquals(std::shared_ptr<T> a, const T* b) {
  return !Equals(a.get(), b);
}

template <class T>
bool NotEquals(const T* a, std::shared_ptr<const T> b) {
  return !Equals(a, b.get());
}

template <class T>
bool NotEquals(const T* a, std::shared_ptr<T> b) {
  return !Equals(a, b.get());
}

template <class T>
bool NotEquals(std::shared_ptr<const T> a, std::shared_ptr<const T> b) {
  return !Equals(a.get(), b.get());
}

template <class T>
bool NotEquals(std::shared_ptr<T> a, std::shared_ptr<const T> b) {
  return !Equals(a.get(), b.get());
}

template <class T>
bool NotEquals(std::shared_ptr<const T> a, std::shared_ptr<T> b) {
  return !Equals(a.get(), b.get());
}

template <class T>
bool NotEquals(std::shared_ptr<T> a, std::shared_ptr<T> b) {
  return !Equals(a.get(), b.get());
}

}  // namespace clay

#endif  // CLAY_GFX_COMPARABLE_H_
