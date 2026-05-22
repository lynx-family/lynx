// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_CONTEXT_OPTIONS_H_
#define CLAY_SHELL_COMMON_CONTEXT_OPTIONS_H_

#include <optional>

#include "base/include/fml/macros.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

enum class ContextType {
  /// The context is used to render to a texture or renderbuffer.
  kRender,
  /// The context will only be used to transfer resources to and from device
  /// memory. No rendering will be performed using this context.
  kResource,
};

//------------------------------------------------------------------------------
/// @brief      Initializes GrContextOptions with values suitable for Flutter.
///             The options can be further tweaked before a GrContext is created
///             from these options.
///
/// @param[in]  type  The type of context that will be created using these
///                   options.
/// @param[in]  type  The client rendering API that will be wrapped using a
///                   context with these options. This argument is only required
///                   if the context is going to be used with a particular
///                   client rendering API.
///
/// @return     The default graphics context options.
///
#ifndef ENABLE_SKITY
GrContextOptions MakeDefaultContextOptions(
    ContextType type, std::optional<GrBackendApi> api = std::nullopt);
#endif  // ENABLE_SKITY

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_CONTEXT_OPTIONS_H_
