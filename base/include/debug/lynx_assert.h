// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef BASE_INCLUDE_DEBUG_LYNX_ASSERT_H_
#define BASE_INCLUDE_DEBUG_LYNX_ASSERT_H_

#include <string>
#include <utility>

#include "base/include/compiler_specific.h"
#include "base/include/debug/lynx_error.h"
#include "base/include/log/logging.h"
#include "base/include/string/string_utils.h"

// TODO(yanghuiwen): As the new error reporting interface has added error
// levels, to avoid confusion caused by the name of the old macros, it is
// necessary to replace the following macro with macro LYNX_ERROR.
#define LynxInfo(error_code, ...)                                  \
  auto exception = lynx::base::LynxError(error_code, __VA_ARGS__); \
  lynx::base::ErrorStorage::GetInstance().SetError(std::move(exception));

#define LynxWarning(expression, error_code, ...)                            \
  if (!(expression)) {                                                      \
    auto exception = lynx::base::LynxError(error_code, __VA_ARGS__);        \
    lynx::base::ErrorStorage::GetInstance().SetError(std::move(exception)); \
  }

// ATTENTION: invoke this, will log and abort
#define LynxFatal(expression, error_code, ...)                           \
  if (!(expression)) {                                                   \
    LOGF("LynxFatal error: error_code:"                                  \
         << error_code                                                   \
         << " error_message:" << lynx::base::FormatString(__VA_ARGS__)); \
  }

#endif  // BASE_INCLUDE_DEBUG_LYNX_ASSERT_H_
