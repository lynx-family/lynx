// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BINDING_NAPI_VALUE_H_
#define BINDING_NAPI_VALUE_H_

#include "third_party/binding/common/value.h"

namespace Napi {
class Env;
class Value;
}  // namespace Napi

namespace lynx {
namespace binding {

Napi::Value ToNAPI(Value&& value, Napi::Env env);

}  // namespace binding
}  // namespace lynx

#endif  // BINDING_NAPI_VALUE_H_
