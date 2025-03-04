// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/lynx_js_error.h"

namespace lynx::piper {

Value LynxError::get(Runtime* rt, const PropNameID& name) {
  auto name_str = name.utf8(*rt);

  if (name_str == "name") {
    return String::createFromUtf8(*rt, name_);
  } else if (name_str == "message") {
    return String::createFromUtf8(*rt, message_);
  } else if (name_str == "stack") {
    return String::createFromUtf8(*rt, stack_);
  }

  return Value::undefined();
}

void LynxError::set(Runtime* rt, const PropNameID& name, const Value& value) {
  auto name_str = name.utf8(*rt);

  if (name_str == "name" && value.isString()) {
    name_ = value.getString(*rt).utf8(*rt);
    return;
  }

  if (name_str == "message" && value.isString()) {
    message_ = value.getString(*rt).utf8(*rt);
    return;
  }

  if (name_str == "stack" && value.isString()) {
    stack_ = value.getString(*rt).utf8(*rt);
    return;
  }
}

std::vector<PropNameID> LynxError::getPropertyNames(Runtime& rt) {
  // As the standard specficated, `Error.prototype.message` and
  // `Error.prototype.name` are both non-enumerable. Here we return an empty
  // vector to avoid them being iterated.
  // @see: https://tc39.es/ecma262/#sec-error-message
  return {};
}
}  // namespace lynx::piper
