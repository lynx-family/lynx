// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_LYNX_ENTITY_ID_GENERATOR_H_
#define CORE_SHELL_LYNX_ENTITY_ID_GENERATOR_H_

#include "base/include/log/log_context.h"
#include "core/base/lynx_export.h"

namespace lynx::shell {

LYNX_EXPORT base::LynxEntityId GenerateLynxEntityId();

}  // namespace lynx::shell

#endif  // CORE_SHELL_LYNX_ENTITY_ID_GENERATOR_H_
