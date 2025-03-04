// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BINDING_NAPI_SHIM_SHIM_NAPI_H_
#define BINDING_NAPI_SHIM_SHIM_NAPI_H_

#ifdef OS_IOS
#include "napi.h"
#else
#include "third_party/napi/include/napi.h"
#endif

#endif  // BINDING_NAPI_SHIM_SHIM_NAPI_H_
