// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ATTRIBUTES_H_
#define CLAY_GFX_ATTRIBUTES_H_

#include <cstddef>
#include <memory>

#include "clay/gfx/rendering_backend.h"

namespace clay {

// |D| is the base type for the attribute
//     (i.e. DlColorFilter, etc.)
// |S| is the base type for the Skia version of the attribute
//     (i.e. SkColorFilter, etc.)
// |T| is the enum that describes the specific subclasses
//     (i.e DlColorFilterType, etc.)
template <class D, class S, typename T>
class Attribute {
 public:
#ifndef ENABLE_SKITY
  using GrPtr = sk_sp<S>;
#else
  using GrPtr = std::shared_ptr<S>;
#endif  // ENABLE_SKITY

  // Return the recognized specific type of the attribute.
  virtual T type() const = 0;

  // Return the size of the instantiated data (typically used to allocate)
  // storage in the DisplayList buffer.
  virtual size_t size() const = 0;

  // Return a shared version of |this| attribute. The |shared_ptr| returned
  // will reference a copy of this object so that the lifetime of the shared
  // version is not tied to the storage of this particular instance.
  virtual std::shared_ptr<D> shared() const = 0;

  // Return an equivalent GrPtr version of this object.
  virtual GrPtr gr_object() const = 0;

  // Perform a content aware |==| comparison of the Attribute.
  bool operator==(D const& other) const {
    return type() == other.type() && equals_(other);
  }
  // Perform a content aware |!=| comparison of the Attribute.
  bool operator!=(D const& other) const { return !(*this == other); }

  virtual ~Attribute() = default;

 protected:
  // Virtual comparison method to support |==| and |!=|.
  virtual bool equals_(D const& other) const = 0;
};

}  // namespace clay

#endif  // CLAY_GFX_ATTRIBUTES_H_
