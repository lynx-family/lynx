// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

const { setupHarmonyAutolink } = require('./core.js');

/**
 * Enables global Lynx library Autolink for one entry or feature HAP module.
 */
function enableHarmonyLynxAutolink(hvigorApi, options = {}) {
  if (hvigorApi === undefined || hvigorApi === null) {
    throw new Error('Harmony Lynx Autolink requires the @ohos/hvigor API');
  }
  if (typeof hvigorApi.parseJsonFile !== 'function') {
    throw new Error(
      'Harmony Lynx Autolink requires @ohos/hvigor.parseJsonFile'
    );
  }
  setupHarmonyAutolink(
    hvigorApi.hvigorConfig,
    hvigorApi.hvigor,
    options,
    hvigorApi.parseJsonFile
  );
}

module.exports = enableHarmonyLynxAutolink;
module.exports.enableHarmonyLynxAutolink = enableHarmonyLynxAutolink;
module.exports.default = enableHarmonyLynxAutolink;
