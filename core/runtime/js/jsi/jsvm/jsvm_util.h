// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JS_JSI_JSVM_JSVM_UTIL_H_
#define CORE_RUNTIME_JS_JSI_JSVM_JSVM_UTIL_H_

#include <ark_runtime/jsvm.h>

#include <cstdlib>

#include "base/include/log/logging.h"
#include "core/runtime/js/jsi/jsvm/jsvm_dyn_load.h"
#include "core/runtime/js/jsi/jsvm/jsvm_exception.h"
#include "core/runtime/js/jsi/jsvm/jsvm_helper.h"

namespace lynx {
namespace piper {
#define LYNX_GET_FIRST_ARG(first, ...) first

#define JSVM_CALL_NO_ENV(name, ...)                                     \
  do {                                                                  \
    auto status = (DynamicLoader::GetFuncTable()) -> name(__VA_ARGS__); \
    if (status != JSVM_Status::JSVM_OK) {                               \
      LOGE("jsvm call failed status:" << status);                       \
    }                                                                   \
  } while (0)

#define JSVM_CALL(runtime_ptr, name, ...)                               \
  do {                                                                  \
    auto status = (DynamicLoader::GetFuncTable()) -> name(__VA_ARGS__); \
    if (status != JSVM_Status::JSVM_OK) {                               \
      LOGE("jsvm call failed status:" << status);                       \
      auto jsvm_env = LYNX_GET_FIRST_ARG(__VA_ARGS__);                  \
      if (runtime_ptr) {                                                \
        JSVMException::ReportExceptionIfNeeded(runtime_ptr);            \
      } else {                                                          \
        JSVMException::TryCatch(jsvm_env);                              \
      }                                                                 \
    }                                                                   \
  } while (0)

#define JSVM_CALL_RETURN(runtime_ptr, name, ret, ...)                   \
  do {                                                                  \
    auto status = (DynamicLoader::GetFuncTable()) -> name(__VA_ARGS__); \
    if (status != JSVM_Status::JSVM_OK) {                               \
      LOGE("jsvm call failed status:" << status);                       \
      auto jsvm_env = LYNX_GET_FIRST_ARG(__VA_ARGS__);                  \
      if (runtime_ptr) {                                                \
        JSVMException::ReportExceptionIfNeeded(runtime_ptr);            \
      } else {                                                          \
        JSVMException::TryCatch(jsvm_env);                              \
      }                                                                 \
      return ret;                                                       \
    }                                                                   \
  } while (0)

class HandleScopeWrapper {
 public:
  explicit HandleScopeWrapper(JSVM_Env env) : env(env) {
    JSVM_CALL(nullptr, OH_JSVM_OpenHandleScope, env, &handleScope);
  }

  ~HandleScopeWrapper() {
    JSVM_CALL(nullptr, OH_JSVM_CloseHandleScope, env, handleScope);
  }

  HandleScopeWrapper(const HandleScopeWrapper&) = delete;
  HandleScopeWrapper& operator=(const HandleScopeWrapper&) = delete;
  HandleScopeWrapper(HandleScopeWrapper&&) = delete;
  void* operator new(size_t) = delete;
  void* operator new[](size_t) = delete;

 protected:
  JSVM_Env env;
  JSVM_HandleScope handleScope;
};

class EnvHandleWrapper {
 public:
  explicit EnvHandleWrapper(JSVM_Env env) : env_(env) {
    if (env) {
      JSVM_CALL(nullptr, OH_JSVM_OpenEnvScope, env, &env_scope_);
    }
  }

  ~EnvHandleWrapper() {
    if (env_scope_) {
      JSVM_CALL(nullptr, OH_JSVM_CloseEnvScope, env_, env_scope_);
    }
  }

  EnvHandleWrapper(const EnvHandleWrapper&) = delete;
  EnvHandleWrapper& operator=(const EnvHandleWrapper&) = delete;
  EnvHandleWrapper(EnvHandleWrapper&&) = delete;
  void* operator new(size_t) = delete;
  void* operator new[](size_t) = delete;

 private:
  JSVM_Env env_;
  JSVM_EnvScope env_scope_ = nullptr;
};
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JS_JSI_JSVM_JSVM_UTIL_H_
