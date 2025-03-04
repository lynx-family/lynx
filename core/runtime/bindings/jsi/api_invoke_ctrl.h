// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_BINDINGS_JSI_API_INVOKE_CTRL_H_
#define CORE_RUNTIME_BINDINGS_JSI_API_INVOKE_CTRL_H_
#include <string>

namespace lynx {
namespace piper {
const char* TAG = "ApiInvokeCtrl";

const char* FLAG_CALLBACK_ID = "callbackID";
const char* FLAG_API = "api";
const char* FLAG_ARGS = "args";
const char* FLAG_ERR_MSG = "errMsg";
const char* FLAG_RES = "res";
const char* FLAG_CODE = "code";
class ApiInvokeCtrl {
 public:
};

}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_BINDINGS_JSI_API_INVOKE_CTRL_H_
