// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_PUBLIC_PROP_BUNDLE_H_
#define CORE_PUBLIC_PROP_BUNDLE_H_

#include <sys/types.h>

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "core/public/pub_value.h"

namespace lynx {
namespace tasm {

// TODO(chenyouhui): Remove GestureDetector from prop_bundle completely
class GestureDetector;

enum CSSPropertyID : int32_t;

class PropBundle {
 public:
  PropBundle() = default;

  virtual ~PropBundle() = default;
  virtual void SetNullProps(const char* key) = 0;
  virtual void SetProps(const char* key, unsigned int value) = 0;
  virtual void SetProps(const char* key, int value) = 0;
  virtual void SetProps(const char* key, const char* value) = 0;
  virtual void SetProps(const char* key, bool value) = 0;
  virtual void SetProps(const char* key, double value) = 0;
  virtual void SetProps(const char* key, const pub::Value& value) = 0;
  virtual void SetProps(const pub::Value& value) = 0;
  virtual void SetEventHandler(const pub::Value& event) = 0;
  virtual void SetGestureDetector(const GestureDetector& detector) = 0;
  virtual void ResetEventHandler() = 0;
  virtual bool Contains(const char* key) const = 0;

  // styles.
  virtual void SetNullPropsByID(CSSPropertyID id) = 0;
  virtual void SetPropsByID(CSSPropertyID id, unsigned int value) = 0;
  virtual void SetPropsByID(CSSPropertyID id, int value) = 0;
  virtual void SetPropsByID(CSSPropertyID id, const char* value) = 0;
  virtual void SetPropsByID(CSSPropertyID id, bool value) = 0;
  virtual void SetPropsByID(CSSPropertyID id, double value) = 0;
  virtual void SetPropsByID(CSSPropertyID id, const pub::Value& value) = 0;

  // TODO(wujintian): Currently, the copy of the element depends on the shallow
  // copy optimization of the prop bundle to improve performance. In the future,
  // when we implement the ability to update multiple prop bundles in a LynxUI
  // at once, the copied element can choose to create a new prop bundle for
  // updating styles instead of modifying a const prop bundle. At that time, the
  // copy of the element will no longer depend on the shallow copy of the prop
  // bundle, and the related code for the shallow copy of the prop bundle can be
  // removed.

  // This function is used to perform a shallow copy of the prop bundle. The
  // prop bundle is a map, and in this context, a shallow copy means that only
  // the first-level keys and values of the prop bundle are copied.
  virtual std::unique_ptr<PropBundle> ShallowCopy() = 0;

 private:
  // TODO: remove the friend later
  friend class RichTextNode;
};

class PropBundleCreator {
 public:
  PropBundleCreator() = default;
  virtual ~PropBundleCreator() = default;

  virtual std::unique_ptr<PropBundle> CreatePropBundle() = 0;

  /**
   * create prop bundle using mapBuffer or not. Only supported in Android by
   * now.
   */
  virtual std::unique_ptr<PropBundle> CreatePropBundle(bool use_map_buffer) {
    return CreatePropBundle();
  };
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_PUBLIC_PROP_BUNDLE_H_
