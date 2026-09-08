// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { prepareHarmonyAutolink } from './generator.js';
import type {
  HarmonyAutolinkPluginOptions,
  HvigorPluginLike,
} from './types.js';

export function harmonyLynxLibraryPlugin(
  options: HarmonyAutolinkPluginOptions = {}
): HvigorPluginLike {
  const taskName = options.taskName ?? 'generateLynxLibraryRegistry';
  return {
    pluginId: 'org.lynxsdk.library.harmony',
    apply(node) {
      prepareHarmonyAutolink(options);
      node.registerTask({
        name: taskName,
        run() {
          prepareHarmonyAutolink(options);
        },
        postDependencies: ['assembleHap'],
      });
    },
  };
}
