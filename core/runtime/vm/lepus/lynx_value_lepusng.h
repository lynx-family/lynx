/**
 * Copyright (c) 2017 Node.js API collaborators. All Rights Reserved.
 *
 * Use of this source code is governed by a MIT license that can be
 * found in the LICENSE file in the root of the source tree.
 */

// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_VM_LEPUS_LYNX_VALUE_LEPUSNG_H_
#define CORE_RUNTIME_VM_LEPUS_LYNX_VALUE_LEPUSNG_H_

#include "base/include/value/lynx_value_api.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "quickjs/include/quickjs.h"
#ifdef __cplusplus
}
#endif

void lynx_attach_value_lepusng(lynx_api_env env, LEPUSContext* ctx);
void lynx_detach_value_lepusng(lynx_api_env env);

#endif  // CORE_RUNTIME_VM_LEPUS_LYNX_VALUE_LEPUSNG_H_
