// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BINDING_NAPI_CALLBACK_HELPER_H_
#define BINDING_NAPI_CALLBACK_HELPER_H_

#include <memory>
#include <unordered_map>
#include <utility>

#include "third_party/binding/napi/shim/shim_napi.h"

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace binding {

using UncaughtExceptionHandler = void (*)(Napi::Object exception);

struct ExceptionHandlerHolder {
  // Since Android relies on callbacks through source code in different, using
  // addresses as unique id will result in different exception handler ids for
  // different SOs. Therefore, a 64-bit random fixed value is temporarily used
  // as the exception handler id to ensure that the same env set/get the same
  // exception handler.
  static const uint64_t KEY = 0x98132fda8883fdb9;
  UncaughtExceptionHandler uncaught_handler_;
};

class CallbackHelper {
 public:
  CallbackHelper() = default;
  CallbackHelper(const CallbackHelper&) = delete;
  CallbackHelper& operator=(const CallbackHelper&) = delete;

  static void ReportException(Napi::Object error);

  // Used by callback functions.
  static void Invoke(const Napi::FunctionReference& cb, Napi::Value& result,
                     std::function<void(Napi::Env)> handler,
                     const std::initializer_list<napi_value>& args);

  bool PrepareForCall(Napi::Function& callback_function);

  bool PrepareForCall(Napi::Object& callback_interface,
                      const char* property_name, bool single_operation = false);

  Napi::Value Call(const std::initializer_list<napi_value>& args);
  Napi::Value CallWithThis(napi_value recv,
                           const std::initializer_list<napi_value>& args);

  static void SetUncaughtExceptionHandler(Napi::Env env,
                                          UncaughtExceptionHandler handler);

 private:
  Napi::FunctionReference function_;
};

class HolderStorage;

class InstanceGuard {
 public:
  InstanceGuard(HolderStorage* ptr) : ptr_(ptr) {}
  static std::shared_ptr<InstanceGuard> CreateSharedGuard(HolderStorage* ptr) {
    return std::make_shared<InstanceGuard>(ptr);
  }
  HolderStorage* Get() { return ptr_; }

 private:
  HolderStorage* ptr_;
};

class HolderStorage {
 public:
  HolderStorage() : instance_guard_(InstanceGuard::CreateSharedGuard(this)){};

  Napi::FunctionReference PopHolder(uintptr_t key) {
    auto ret = std::move(reference_holder_map_[key]);
    reference_holder_map_.erase(key);
    return ret;
  }

  const Napi::FunctionReference& PeekHolder(uintptr_t key) {
    return reference_holder_map_[key];
  }

  void PushHolder(uintptr_t key, Napi::FunctionReference holder) {
    reference_holder_map_[key] = std::move(holder);
  }

  std::weak_ptr<InstanceGuard> instance_guard() { return instance_guard_; }

 private:
  std::shared_ptr<InstanceGuard> instance_guard_;
  std::unordered_map<uintptr_t, Napi::FunctionReference> reference_holder_map_;
};

}  // namespace binding
}  // namespace lynx

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_undefs.h"
#endif

#endif  // BINDING_NAPI_CALLBACK_HELPER_H_
