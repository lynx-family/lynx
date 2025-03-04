// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BINDING_NAPI_EXCEPTION_STATE_H_
#define BINDING_NAPI_EXCEPTION_STATE_H_

#include <cstdio>
#include <string>

#include "third_party/binding/common/env.h"
#include "third_party/binding/napi/napi_bridge.h"
#include "third_party/binding/napi/shim/shim_napi.h"

namespace lynx {
namespace binding {

class ExceptionState {
 public:
 explicit ExceptionState(Napi::Env env) : env_(FromNAPI(env)) {}
  ExceptionState(Napi::Env env, const std::string& message)
      : env_(FromNAPI(env)), message_(message) {}
  explicit ExceptionState(Env env) : env_(env) {}

  ExceptionState(const ExceptionState&) = delete;
  ExceptionState& operator=(const ExceptionState&) = delete;

  ~ExceptionState() {
    if (env_.IsNapi() && HadException()) {
      exception_.Value().ThrowAsJavaScriptException();
    }
  }

  bool HadException() { return !message_.empty(); }
  void ClearException() { message_.clear(); }

  enum ErrorType { kTypeError, kRangeError, kError };

  void SetException(const std::string& message, ErrorType error_type = kError) {
    message_ = message;
    // TODO(yuyifei): Support remote exceptions.
    if (env_.IsRemote()) {
      return;
    }
    Napi::Env env = ToNAPI(env_);
    switch (error_type) {
      case kTypeError:
        exception_ = Napi::Persistent(static_cast<Napi::Error>(
            Napi::TypeError::New(env, message_.c_str())));
        break;
      case kRangeError:
        exception_ = Napi::Persistent(static_cast<Napi::Error>(
            Napi::RangeError::New(env, message_.c_str())));
        break;
      default:
        exception_ = Napi::Persistent(Napi::Error::New(env, message_.c_str()));
        break;
    }
  }
  const std::string& Message() const { return message_; }

 private:
  Env env_;
  std::string message_;
  Napi::Reference<Napi::Error> exception_;
};

}  // namespace binding
}  // namespace lynx

#endif  // BINDING_NAPI_EXCEPTION_STATE_H_
