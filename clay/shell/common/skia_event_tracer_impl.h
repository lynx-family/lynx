// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_SKIA_EVENT_TRACER_IMPL_H_
#define CLAY_SHELL_COMMON_SKIA_EVENT_TRACER_IMPL_H_

#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace clay {

void InitSkiaEventTracer(
    bool enabled, const std::optional<std::vector<std::string>>& allowlist);

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_SKIA_EVENT_TRACER_IMPL_H_
