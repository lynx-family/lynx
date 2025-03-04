// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_MACROS_H_
#define BASE_INCLUDE_FML_MACROS_H_

#include <stdlib.h>

#define BASE_DISALLOW_COPY(TypeName) TypeName(const TypeName&) = delete

#define BASE_DISALLOW_ASSIGN(TypeName) \
  TypeName& operator=(const TypeName&) = delete

#define BASE_DISALLOW_MOVE(TypeName) \
  TypeName(TypeName&&) = delete;     \
  TypeName& operator=(TypeName&&) = delete

#define BASE_DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&) = delete;           \
  TypeName& operator=(const TypeName&) = delete

#define BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(TypeName) \
  TypeName(const TypeName&) = delete;                \
  TypeName(TypeName&&) = delete;                     \
  TypeName& operator=(const TypeName&) = delete;     \
  TypeName& operator=(TypeName&&) = delete

#define BASE_DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName) \
  TypeName() = delete;                                \
  BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(TypeName)

#ifndef FML_USED_ON_EMBEDDER

#define FML_EMBEDDER_ONLY [[deprecated]]

#else  // FML_USED_ON_EMBEDDER

#define FML_EMBEDDER_ONLY

#endif  // FML_USED_ON_EMBEDDER

// TODO(zhengsenyao): Support stream operator << for LYNX_(D)BASE_CHECK
#define LYNX_BASE_CHECK(condition) (condition) ? (void)0 : abort();

#ifndef NDEBUG
#define LYNX_BASE_DCHECK(condition) LYNX_BASE_CHECK(condition)
#else
#define LYNX_BASE_DCHECK(condition) (void)0
#endif

#endif  // BASE_INCLUDE_FML_MACROS_H_
