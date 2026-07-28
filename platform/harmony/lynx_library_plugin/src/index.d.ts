// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export interface HarmonyLynxAutolinkOptions {
  /** Selects the entry or feature HAP module. Inferred when exactly one module depends on @lynx/lynx. */
  moduleName?: string;
  /** Overrides the directory from which ancestor node_modules folders are scanned. */
  projectRoot?: string;
}

export declare function enableHarmonyLynxAutolink(
  hvigorApi: typeof import('@ohos/hvigor'),
  options?: HarmonyLynxAutolinkOptions
): void;

export default enableHarmonyLynxAutolink;
