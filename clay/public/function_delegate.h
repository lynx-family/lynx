// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_PUBLIC_FUNCTION_DELEGATE_H_
#define CLAY_PUBLIC_FUNCTION_DELEGATE_H_

#include <functional>
#include <string>

#include "clay/public/value.h"
namespace clay {
using CallbackType = std::function<void(clay::Value)>;
class FunctionDelegate {
 public:
  inline virtual ~FunctionDelegate() {}

  virtual void CallFunction(const std::string& function_name,
                            clay::Value::Map args,
                            const CallbackType& callback) = 0;
};

}  // namespace clay

#endif  // CLAY_PUBLIC_FUNCTION_DELEGATE_H_
