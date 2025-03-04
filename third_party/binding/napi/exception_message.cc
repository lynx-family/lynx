// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "third_party/binding/napi/exception_message.h"

#include <cstdio>

namespace lynx {

namespace {
constexpr size_t kMessageBufferSize = 256;
}

namespace binding {

void ExceptionMessage::NonObjectReceived(const Napi::Env &env,
                                         const char *dictionary_name) {
  char message[kMessageBufferSize];
  std::snprintf(message, kMessageBufferSize, "Received non-object type for %s",
                dictionary_name);
  Napi::TypeError::New(env, message).ThrowAsJavaScriptException();
}

void ExceptionMessage::NoRequiredProperty(const Napi::Env &env,
                                         const char *dictionary_name,
                                         const char *property_name) {
 char message[256];
 std::snprintf(message, kMessageBufferSize,
              "Received %s argument does not have required property '%s'",
              dictionary_name, property_name);
 Napi::TypeError::New(env, message).ThrowAsJavaScriptException();
}

void ExceptionMessage::IllegalConstructor(const Napi::Env &env,
                                          const char *interface_name) {
  char message[kMessageBufferSize];
  std::snprintf(message, kMessageBufferSize, "Illegal %s constructor call",
                interface_name);
  Napi::Error::New(env, message).ThrowAsJavaScriptException();
}

void ExceptionMessage::FailedToCallOverload(const Napi::Env &env,
                                            const char *method_name) {
  char message[kMessageBufferSize];
  std::snprintf(message, kMessageBufferSize,
                "Failed to resolve to a %s overload", method_name);
  Napi::TypeError::New(env, message).ThrowAsJavaScriptException();
}

// void ExceptionMessage::NotImplemented(const Napi::Env &env) {
//  Napi::Error::New(env, "Not implemented").ThrowAsJavaScriptException();
//}

void ExceptionMessage::NotEnoughArguments(const Napi::Env &env,
                                          const char *interface_name,
                                          const char *pretty_name,
                                          const char *expecting_name) {
  char message[kMessageBufferSize];
  std::snprintf(message, kMessageBufferSize,
                "Not enough arguments for %s.%s(), expecting: %s", interface_name,
                pretty_name, expecting_name);
  Napi::TypeError::New(env, message).ThrowAsJavaScriptException();
}

void ExceptionMessage::InvalidType(const Napi::Env &env,
                                   const char *pretty_name,
                                   const char *expecting_name) {
  char message[kMessageBufferSize];
  std::snprintf(message, kMessageBufferSize,
                "Invalid type for %s, expecting: %s", pretty_name,
                expecting_name);
  Napi::TypeError::New(env, message).ThrowAsJavaScriptException();
}

// void ExceptionMessage::NotSupportYet(const Napi::Env &env) {
//  Napi::Error::New(env, "Not supported yet").ThrowAsJavaScriptException();
//}

void ExceptionMessage::FailedToCallOverloadExpecting(
    const Napi::Env &env, const char *overload_name,
    const char *expecting_name) {
  char message[kMessageBufferSize];
  std::snprintf(
      message, kMessageBufferSize,
      "Failed to resolve to a %s overload, expecting the 1st argument to "
      "be one of: %s",
      overload_name, expecting_name);
  Napi::TypeError::New(env, message).ThrowAsJavaScriptException();
}

}  // namespace binding
}  // namespace lynx
