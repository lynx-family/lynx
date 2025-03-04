// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef THIRD_PARTY_BINDINGS_GEN_TEST_TEST_MODULE_H_
#define THIRD_PARTY_BINDINGS_GEN_TEST_TEST_MODULE_H_

#include "core/runtime/bindings/napi/napi_environment.h"
#include "third_party/binding/napi/shim/shim_napi.h"

namespace lynx {
namespace gen_test {

class TestModule : public piper::NapiEnvironment::Module {
 public:
  TestModule() = default;

  void OnLoad(Napi::Object& target) override;
};

}  // namespace gen_test
}  // namespace lynx

#endif  // THIRD_PARTY_BINDINGS_GEN_TEST_TEST_MODULE_H_
