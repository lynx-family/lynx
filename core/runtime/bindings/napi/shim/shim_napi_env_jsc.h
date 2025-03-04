// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_NAPI_SHIM_SHIM_NAPI_ENV_JSC_H_
#define CORE_RUNTIME_BINDINGS_NAPI_SHIM_SHIM_NAPI_ENV_JSC_H_

#ifdef OS_IOS
#include <napi_env_jsc.h>
#else
#include "third_party/napi/include/napi_env_jsc.h"
#endif

#endif  // CORE_RUNTIME_BINDINGS_NAPI_SHIM_SHIM_NAPI_ENV_JSC_H_
