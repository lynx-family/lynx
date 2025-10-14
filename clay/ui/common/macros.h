// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMMON_MACROS_H_
#define CLAY_UI_COMMON_MACROS_H_

#define DCHECK_RET0(__condition__) \
  FML_DCHECK(__condition__);       \
  do {                             \
    if (!(__condition__)) {        \
      return;                      \
    }                              \
  } while (0)

#define DCHECK_RET1(__condition__, __ret__) \
  FML_DCHECK(__condition__);                \
  do {                                      \
    if (!(__condition__)) {                 \
      return (__ret__);                     \
    }                                       \
  } while (0)

#endif  // CLAY_UI_COMMON_MACROS_H_
