// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "third_party/binding/napi/callback_helper.h"

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace binding {

// static
void CallbackHelper::ReportException(Napi::Object error_obj) {
  Napi::Env env = error_obj.Env();
  ExceptionHandlerHolder* holder =
      env.GetInstanceData<ExceptionHandlerHolder>();
  if (holder && holder->uncaught_handler_) {
    holder->uncaught_handler_(error_obj);
  }
}

// static
void CallbackHelper::Invoke(const Napi::FunctionReference& cb,
                            Napi::Value& result,
                            std::function<void(Napi::Env)> handler,
                            const std::initializer_list<napi_value>& args) {
  Napi::ContextScope cs(cb.Env());
  Napi::HandleScope hs(cb.Env());
  if (cb.IsEmpty() || !cb.Value().IsFunction()) {
    ReportException(Napi::TypeError::New(
        cb.Env(), "The OnLoadCallback callback is not callable."));
    return;
  }
  result = cb.Value().Call(args);
  if (cb.Env().IsExceptionPending()) {
    if (handler) {
      handler(cb.Env());
      return;
    }
    ReportException(cb.Env().GetAndClearPendingException().As<Napi::Object>());
    return;
  }
}

bool CallbackHelper::PrepareForCall(Napi::Function& callback_function) {
  if (callback_function.IsEmpty() || !callback_function.IsFunction()) {
    Napi::TypeError error = Napi::TypeError::New(
        callback_function.Env(), "The provided callback is not callable.");
    ReportException(error);
    return false;
  }
  function_ = Napi::Persistent(callback_function);
  return true;
}

bool CallbackHelper::PrepareForCall(Napi::Object& callback_interface,
                                    const char* property_name,
                                    bool single_operation) {
  bool is_callable = true;
  if (callback_interface.IsEmpty()) {
    is_callable = false;
  }

  if (single_operation && callback_interface.IsFunction()) {
    function_ = Napi::Persistent(callback_interface.As<Napi::Function>());
  } else {
    Napi::Value function = callback_interface[property_name];
    if (!function.IsFunction()) {
      is_callable = false;
    } else {
      function_ = Napi::Persistent(function.As<Napi::Function>());
    }
  }
  if (!is_callable) {
    Napi::TypeError error = Napi::TypeError::New(
        callback_interface.Env(), "The provided callback is not callable.");
    ReportException(error);
    return false;
  }
  return true;
}

Napi::Value CallbackHelper::Call(
    const std::initializer_list<napi_value>& args) {
  Napi::Value result;
  result = function_.Value().Call(args);
  if (function_.Env().IsExceptionPending()) {
    Napi::Object error =
        function_.Env().GetAndClearPendingException().As<Napi::Object>();
    ReportException(error);
  }
  return result;
}

Napi::Value CallbackHelper::CallWithThis(
    napi_value recv, const std::initializer_list<napi_value>& args) {
  Napi::Value result;
  result = function_.Value().Call(recv, args);
  if (function_.Env().IsExceptionPending()) {
    Napi::Object error =
        function_.Env().GetAndClearPendingException().As<Napi::Object>();
    ReportException(error);
  }
  return result;
}

// static
void CallbackHelper::SetUncaughtExceptionHandler(
    Napi::Env env, UncaughtExceptionHandler handler) {
  ExceptionHandlerHolder* maybe_holder =
      env.GetInstanceData<ExceptionHandlerHolder>();
  // Setting exception handler will overwrite previous value. The same Napi env
  // use the same handler.
  if (maybe_holder) {
    maybe_holder->uncaught_handler_ = handler;
  } else {
    ExceptionHandlerHolder* holder = new ExceptionHandlerHolder{};
    holder->uncaught_handler_ = handler;
    env.SetInstanceData(holder);
  }
}

}  // namespace binding
}  // namespace lynx
