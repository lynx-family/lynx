// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BINDING_COMMON_BASE_H_
#define BINDING_COMMON_BASE_H_

#include <cstdint>
#include <cstdlib>

#define BINDING_CHECK(expression) \
  do {                            \
    if (!(expression)) {          \
      std::abort();               \
    }                             \
  } while (false)

#ifdef DEBUG
#define BINDING_DCHECK(expression) BINDING_CHECK(expression)
#else
#define BINDING_DCHECK(expression)
#endif

#define BINDING_NOTREACHED() BINDING_CHECK(false)

namespace Napi {
class Env;
class Object;
}  // namespace Napi

namespace lynx {
namespace binding {

enum class RuntimeType {
  kNapi,
  kRemote,
  kUnknown,
};

class Env;
class Object;
class ObjectRef;

class BridgeBase {
 // This is a mixin interface class and does not declare a virtual destructor.
 public:
  virtual Object GetBaseObject() = 0;
  virtual Napi::Object NapiObject();
  virtual ObjectRef GetStrongRef() = 0;
  virtual Env GetEnv() = 0;
  virtual Napi::Env NapiEnv();
  virtual bool IsNapi() { return false; }
  virtual bool IsRemote() { return false; }
};

class ImplBase {
 public:
  // Use NapiObject() which is faster where Napi alone is targeted.
  Object GetBaseObject();
  Napi::Object NapiObject();
  ObjectRef ObtainStrongRef();
  // Use NapiEnv() which is faster where Napi alone is targeted.
  Env GetEnv();
  Napi::Env NapiEnv();
  bool IsWrapped() { return bridge_; }
  bool IsNapiWrapped() { return bridge_ && bridge_->IsNapi(); }
  bool IsRemoteWrapped() { return bridge_ && bridge_->IsRemote(); }

  void AssociateWithWrapper(BridgeBase* bridge);
  virtual void OnWrapped() {}
  virtual void OnOrphaned() {}
  virtual void Dispose() {}

  virtual ~ImplBase();

 protected:
  ImplBase();
  void OnExternalMemoryChanged(int64_t current_memory_in_bytes);
  int64_t GetResidentMemoryInBytes() const { return resident_memory_in_bytes_; }

  // A reasonable estimate for the initial size in memory for a binding object.
  static constexpr int kInitialMemoryInBytes = 32;

 private:
  BridgeBase* bridge_ = nullptr;
  int64_t resident_memory_in_bytes_ = 0;
  int64_t last_reported_memory_in_bytes_ = 0;
};

}  // namespace binding
}  // namespace lynx

#endif  // BINDING_COMMON_BASE_H_
