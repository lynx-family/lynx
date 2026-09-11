// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/**
 * Compatibility entry point for the historical declaration path.
 *
 * New integrations should select `stable`, `experimental`, or `internal`.
 * This wrapper intentionally retains the complete legacy surface.
 */
export * from './internal';
