// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BINDING_NAPI_EXCEPTION_MESSAGE_H_
#define BINDING_NAPI_EXCEPTION_MESSAGE_H_

#include "third_party/binding/napi/shim/shim_napi.h"

namespace lynx {
namespace binding {

class ExceptionMessage {
 public:
  ~ExceptionMessage() = default;

  ExceptionMessage(const ExceptionMessage &) = delete;

  ExceptionMessage &operator=(const ExceptionMessage &) = delete;

  ExceptionMessage() = default;

  static void NonObjectReceived(const Napi::Env &env,
                                            const char *dictionary_name);

  static void NoRequiredProperty(const Napi::Env &env,
                                              const char *dictionary_name,
                                              const char *property_name);

  static void IllegalConstructor(const Napi::Env &env,
                                             const char *interface_name);

  static void FailedToCallOverload(const Napi::Env &env,
                                               const char *method_name);

  //  static void NotImplemented(const Napi::Env &env);

  static void NotEnoughArguments(const Napi::Env &env,
                                             const char *interface_name,
                                             const char *pretty_name,
                                             const char *expecting_name);

  static void InvalidType(const Napi::Env &env,
                                      const char *pretty_name,
                                      const char *expecting_name);

  //  static void NotSupportYet(const Napi::Env &env);

  static void FailedToCallOverloadExpecting(
      const Napi::Env &env, const char *overload_name,
      const char *expecting_name);
};

}  // namespace binding
}  // namespace lynx
#endif  // BINDING_NAPI_EXCEPTION_MESSAGE_H_
