// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/napi/test/test_module.h"

#include "core/runtime/bindings/napi/test/napi_test_context.h"
#include "core/runtime/bindings/napi/test/napi_test_element.h"

namespace lynx {
namespace test {

void TestModule::OnLoad(Napi::Object& target) {
  Napi::Env env = target.Env();
  NapiTestElement::Install(env, target);
  NapiTestContext::Install(env, target);
}

}  // namespace test
}  // namespace lynx
