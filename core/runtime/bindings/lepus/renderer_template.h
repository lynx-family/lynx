// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_LEPUS_RENDERER_TEMPLATE_H_
#define CORE_RUNTIME_BINDINGS_LEPUS_RENDERER_TEMPLATE_H_

#include "core/base/lynx_trace_categories.h"
#include "core/runtime/bindings/lepus/renderer_functions_def.h"

#if defined(OS_WIN)
#ifdef SetProp
#define REDEF_SETPROP SetProp
#undef SetProp
#endif  // SetProp
#endif  // OS_WIN

#define NORMAL_FUNCTION_DEF(name)  \
  RENDERER_FUNCTION(name) {        \
    PREPARE_ARGS(name);            \
    CALL_RUNTIME_AND_RETURN(name); \
  }
NORMAL_RENDERER_FUNCTIONS(NORMAL_FUNCTION_DEF)

#undef NORMAL_FUNCTION_DEF

#if defined(OS_WIN)
#ifdef REDEF_SETPROP
#define SetProp REDEF_SETPROP
#endif  // REDEF_SETPROP
#endif

#endif  // CORE_RUNTIME_BINDINGS_LEPUS_RENDERER_TEMPLATE_H_
