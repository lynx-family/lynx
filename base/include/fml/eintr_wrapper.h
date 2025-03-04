// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_EINTR_WRAPPER_H_
#define BASE_INCLUDE_FML_EINTR_WRAPPER_H_

#include <errno.h>

#include "build/build_config.h"

#if defined(OS_WIN)

// Windows has no concept of EINTR.
#define FML_HANDLE_EINTR(x) (x)
#define FML_IGNORE_EINTR(x) (x)

#else

#define FML_HANDLE_EINTR(x)                                 \
  ({                                                        \
    decltype(x) eintr_wrapper_result;                       \
    do {                                                    \
      eintr_wrapper_result = (x);                           \
    } while (eintr_wrapper_result == -1 && errno == EINTR); \
    eintr_wrapper_result;                                   \
  })

#define FML_IGNORE_EINTR(x)                               \
  ({                                                      \
    decltype(x) eintr_wrapper_result;                     \
    do {                                                  \
      eintr_wrapper_result = (x);                         \
      if (eintr_wrapper_result == -1 && errno == EINTR) { \
        eintr_wrapper_result = 0;                         \
      }                                                   \
    } while (0);                                          \
    eintr_wrapper_result;                                 \
  })

#endif  // defined(OS_WIN)

#endif  // BASE_INCLUDE_FML_EINTR_WRAPPER_H_
