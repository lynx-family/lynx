// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export { prepareHarmonyAutolink, generateRegistrySource } from './generator.js';
export { harmonyLynxLibraryPlugin } from './plugin.js';
export { parseJson5File, scanHarmonyLibraries } from './scanner.js';
export type {
  HarmonyAutolinkPluginOptions,
  HarmonyLibraryInfo,
  HvigorNodeLike,
  HvigorPluginLike,
  PrepareHarmonyAutolinkOptions,
  PrepareHarmonyAutolinkResult,
} from './types.js';
