// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "test_module.h"

#include "jsbridge/bindings/gen_test/napi_async_object.h"
#include "jsbridge/bindings/gen_test/napi_gen_test_command_buffer.h"
#include "jsbridge/bindings/gen_test/napi_test_context.h"
#include "jsbridge/bindings/gen_test/napi_test_element.h"

namespace lynx {
namespace gen_test {

void TestModule::OnLoad(Napi::Object& target) {
  Napi::Env env = target.Env();
  NapiGenTestCommandBuffer::Install(env, target);
  NapiTestElement::Install(env, target);
  NapiTestContext::Install(env, target);
  NapiAsyncObject::Install(env, target, "TestAsyncObject");
}

}  // namespace gen_test
}  // namespace lynx
