// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_NAPI_TEST_TEST_MODULE_H_
#define CORE_RUNTIME_BINDINGS_NAPI_TEST_TEST_MODULE_H_

#include "core/runtime/bindings/napi/napi_environment.h"
#include "third_party/binding/napi/shim/shim_napi.h"

namespace lynx {
namespace test {

class TestModule : public piper::NapiEnvironment::Module {
 public:
  TestModule() = default;

  void OnLoad(Napi::Object& target) override;
};

}  // namespace test
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_NAPI_TEST_TEST_MODULE_H_
