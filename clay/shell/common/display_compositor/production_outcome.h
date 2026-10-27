// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_PRODUCTION_OUTCOME_H_
#define CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_PRODUCTION_OUTCOME_H_

namespace clay::display {

// Distinguish an accepted mailbox frame from absent, retryable or failed work.
enum class ProductionOutcome { kPublished, kNoWork, kDeferred, kFailed };

}  // namespace clay::display

#endif  // CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_PRODUCTION_OUTCOME_H_
