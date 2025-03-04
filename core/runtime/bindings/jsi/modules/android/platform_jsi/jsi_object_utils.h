// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_PLATFORM_JSI_JSI_OBJECT_UTILS_H_
#define CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_PLATFORM_JSI_JSI_OBJECT_UTILS_H_

#include <jni.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {
namespace platform_jsi {

/**
 * Hold and convert data of platform JSIObject value
 * TODO(zhoupeng.z): use template
 */
class JSIObject {
 public:
  virtual ~JSIObject() = default;
  JSIObject(const JSIObject &) = delete;
  JSIObject &operator=(const JSIObject &) = delete;
  JSIObject(JSIObject &&) = delete;
  JSIObject &operator=(JSIObject &&) = delete;

  virtual std::optional<Value> ConvertToValue(lynx::piper::Runtime *rt) = 0;

  static std::unique_ptr<JSIObject> Create(bool value);
  static std::unique_ptr<JSIObject> Create(double value);
  static std::unique_ptr<JSIObject> Create(jlong value);
  static std::unique_ptr<JSIObject> Create(std::string value);
  static std::unique_ptr<JSIObject> Create(std::shared_ptr<HostObject> value);
  static std::unique_ptr<JSIObject> Create(
      std::vector<std::unique_ptr<JSIObject>> value);

 protected:
  JSIObject() = default;
};

class JSIObjectBool : public JSIObject {
 public:
  std::optional<Value> ConvertToValue(lynx::piper::Runtime *rt) override;

 private:
  friend JSIObject;
  explicit JSIObjectBool(bool value) : value_(value) {}
  bool value_;
};

class JSIObjectNumber : public JSIObject {
 public:
  std::optional<Value> ConvertToValue(lynx::piper::Runtime *rt) override;

 private:
  friend JSIObject;
  explicit JSIObjectNumber(double value) : value_(value) {}
  double value_;
};

class JSIObjectJLong : public JSIObject {
 public:
  std::optional<Value> ConvertToValue(lynx::piper::Runtime *rt) override;

 private:
  friend JSIObject;
  explicit JSIObjectJLong(jlong value) : value_(value) {}
  jlong value_;
};

class JSIObjectString : public JSIObject {
 public:
  std::optional<Value> ConvertToValue(lynx::piper::Runtime *rt) override;

 private:
  friend JSIObject;
  explicit JSIObjectString(std::string value) : value_(std::move(value)) {}
  std::string value_;
};

class JSIObjectHostObject : public JSIObject {
 public:
  std::optional<Value> ConvertToValue(lynx::piper::Runtime *rt) override;

 private:
  friend JSIObject;
  explicit JSIObjectHostObject(std::shared_ptr<HostObject> value)
      : value_(std::move(value)) {}
  std::shared_ptr<HostObject> value_;
};

class JSIObjectArray : public JSIObject {
 public:
  std::optional<Value> ConvertToValue(lynx::piper::Runtime *rt) override;

 private:
  friend JSIObject;
  explicit JSIObjectArray(std::vector<std::unique_ptr<JSIObject>> value)
      : value_(std::move(value)) {}
  std::vector<std::unique_ptr<JSIObject>> value_;
};

}  // namespace platform_jsi
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_PLATFORM_JSI_JSI_OBJECT_UTILS_H_
