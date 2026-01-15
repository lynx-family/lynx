// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_PUBLIC_PROP_BUNDLE_H_
#define CORE_PUBLIC_PROP_BUNDLE_H_

#include <sys/types.h>

#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "base/include/fml/memory/ref_counted.h"
#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/vector.h"
#include "core/public/pub_value.h"

namespace lynx {
namespace tasm {

// TODO(chenyouhui): Remove GestureDetector from prop_bundle completely
class GestureDetector;

enum CSSPropertyID : int32_t;

class PropBundle : public fml::RefCountedThreadSafeStorage {
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
  virtual void SetPropsByID(CSSPropertyID id, const uint8_t* data,
                            size_t size) = 0;
  virtual void SetPropsByID(CSSPropertyID id, const uint32_t* data,
                            size_t size) = 0;

  template <typename T>
  void SetPropsByID(CSSPropertyID id, const base::Vector<T>& value) {
    SetPropsByIDInner(id, value);
  }

  template <typename T>
  void SetPropsByID(CSSPropertyID id, const std::vector<T>& value) {
    SetPropsByIDInner(id, value);
  }

  virtual bool IsNative() const { return false; }

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
  virtual fml::RefPtr<PropBundle> ShallowCopy() = 0;

  void ReleaseSelf() const override { delete this; }

 private:
  // TODO: remove the friend later
  friend class RichTextNode;

  template <typename T>
  void SetPropsByIDInner(CSSPropertyID id, const T& value) {
    if constexpr (std::is_same_v<typename T::value_type, uint8_t> ||
                  std::is_same_v<typename T::value_type, uint32_t>) {
      SetPropsByID(id, value.data(), value.size());
    } else {
      uint32_t buffer[value.size()];
      for (size_t i = 0; i < value.size(); i++) {
        buffer[i] = static_cast<uint32_t>(value[i]);
      }
      SetPropsByID(id, buffer, value.size());
    }
  }
};

class PropBundleCreator {
 public:
  PropBundleCreator() = default;
  virtual ~PropBundleCreator() = default;

  virtual fml::RefPtr<PropBundle> CreatePropBundle() = 0;

  /**
   * create prop bundle using mapBuffer or not. Only supported in Android by
   * now.
   */
  virtual fml::RefPtr<PropBundle> CreatePropBundle(bool use_map_buffer) {
    return CreatePropBundle();
  };

  /*
   * Create a lepus::Value based prop bundle named NativePropBundle when
   * use_native_value is true. And currently for fragment_layer_render only.
   */
  virtual fml::RefPtr<PropBundle> CreatePropBundle(bool use_map_buffer,
                                                   bool use_native_value) {
    return CreatePropBundle(use_map_buffer);
  }
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_PUBLIC_PROP_BUNDLE_H_
