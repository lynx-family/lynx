// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_PUBLIC_PUB_PROP_BUNDLE_HARMONY_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_PUBLIC_PUB_PROP_BUNDLE_HARMONY_H_

#include "base/include/base_export.h"
#include "core/public/pub_value.h"

namespace lynx {
namespace tasm {
class PropBundleHarmony;
namespace harmony {

class BASE_EXPORT PubPropBundleHarmony {
 public:
  PubPropBundleHarmony();
  ~PubPropBundleHarmony();
  void SetNullProps(const char* key) const;
  void SetProps(const char* key, unsigned int value) const;
  void SetProps(const char* key, int value) const;
  void SetProps(const char* key, const char* value) const;
  void SetProps(const char* key, bool value) const;
  void SetProps(const char* key, double value) const;
  void SetProps(const char* key, const pub::Value& value) const;
  void SetProps(const pub::Value& value) const;
  void SetEventHandler(const pub::Value& event) const;
  bool Contains(const char* key) const;
  void SetNullPropsByID(int id) const;
  void SetPropsByID(int id, unsigned int value) const;
  void SetPropsByID(int id, int value) const;
  void SetPropsByID(int id, const char* value) const;
  void SetPropsByID(int id, bool value) const;
  void SetPropsByID(int id, double value) const;
  void SetPropsByID(int id, const pub::Value& value) const;
  PropBundleHarmony* PropBundle() const;

 private:
  PropBundleHarmony* impl_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_PUBLIC_PUB_PROP_BUNDLE_HARMONY_H_
