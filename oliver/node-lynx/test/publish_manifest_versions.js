// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

const assert = require('assert');

const rootManifest = require('../package.json');
const darwinManifest = require('../platform/darwin-arm64/package.json');
const linuxManifest = require('../platform/linux-x64/package.json');
const { validatePackageVersions } = require('../scripts/publish.js');

const platformManifests = [darwinManifest, linuxManifest].map((manifest) => ({
  manifest,
}));

assert.doesNotThrow(() =>
  validatePackageVersions(rootManifest, platformManifests)
);
assert.throws(
  () =>
    validatePackageVersions(rootManifest, [
      ...platformManifests,
      {
        manifest: {
          name: '@lynx-js/node-lynx-test-platform',
          version: '0.0.0',
        },
      },
    ]),
  /node-lynx package versions must match/
);

console.log('node-lynx package versions are consistent.');
