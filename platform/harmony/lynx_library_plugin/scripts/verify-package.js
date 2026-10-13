// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

const assert = require('node:assert/strict');
const { execFileSync } = require('node:child_process');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');

const tarball = process.argv[2];
assert.ok(tarball, 'Usage: node scripts/verify-package.js <tarball>');
const tarballPath = path.resolve(tarball);
assert.ok(fs.statSync(tarballPath).isFile(), 'Tarball must be a file');

const consumerRoot = fs.mkdtempSync(
  path.join(os.tmpdir(), 'lynx-harmony-plugin-consumer-')
);

try {
  fs.writeFileSync(
    path.join(consumerRoot, 'package.json'),
    JSON.stringify({ private: true, type: 'module' })
  );
  execFileSync(
    'npm',
    [
      'install',
      tarballPath,
      '--ignore-scripts',
      '--legacy-peer-deps',
      '--package-lock=false',
      '--no-audit',
      '--no-fund',
    ],
    { cwd: consumerRoot, stdio: 'inherit' }
  );

  execFileSync(
    process.execPath,
    [
      '--input-type=commonjs',
      '--eval',
      `const assert = require('node:assert/strict');
const plugin = require('@lynx-js/lynx-library-plugin');
assert.equal(typeof plugin, 'function');
assert.equal(typeof plugin.enableHarmonyLynxAutolink, 'function');
assert.equal(plugin.default, plugin.enableHarmonyLynxAutolink);`,
    ],
    { cwd: consumerRoot, stdio: 'inherit' }
  );
  execFileSync(
    process.execPath,
    [
      '--input-type=module',
      '--eval',
      `import assert from 'node:assert/strict';
import plugin, { enableHarmonyLynxAutolink } from '@lynx-js/lynx-library-plugin';
assert.equal(typeof plugin, 'function');
assert.equal(plugin, enableHarmonyLynxAutolink);`,
    ],
    { cwd: consumerRoot, stdio: 'inherit' }
  );

  // Hvigor is supplied by Harmony tooling, not by the public npm registry.
  fs.writeFileSync(
    path.join(consumerRoot, 'hvigor.d.ts'),
    `declare module '@ohos/hvigor' {
  export const hvigorConfig: object;
  export const hvigor: object;
  export function parseJsonFile(filePath: string): object;
}
`
  );
  const typeConsumer = `import plugin, {
  enableHarmonyLynxAutolink,
  type HarmonyLynxAutolinkOptions,
} from '@lynx-js/lynx-library-plugin';
import * as hvigorApi from '@ohos/hvigor';

const options: HarmonyLynxAutolinkOptions = {
  moduleName: 'entry',
  projectRoot: '.',
};
const namedResult: void = enableHarmonyLynxAutolink(hvigorApi, options);
const defaultResult: void = plugin(hvigorApi);

// @ts-expect-error moduleName must be a string.
enableHarmonyLynxAutolink(hvigorApi, { moduleName: 1 });
// @ts-expect-error the Hvigor API is required.
enableHarmonyLynxAutolink({});
`;
  fs.writeFileSync(path.join(consumerRoot, 'consumer.cts'), typeConsumer);
  fs.writeFileSync(path.join(consumerRoot, 'consumer.mts'), typeConsumer);
  fs.writeFileSync(
    path.join(consumerRoot, 'tsconfig.json'),
    JSON.stringify({
      compilerOptions: {
        target: 'ES2022',
        module: 'NodeNext',
        moduleResolution: 'NodeNext',
        strict: true,
        noEmit: true,
        esModuleInterop: true,
        skipLibCheck: false,
        types: [],
      },
      files: ['hvigor.d.ts', 'consumer.cts', 'consumer.mts'],
    })
  );
  execFileSync(
    process.execPath,
    [require.resolve('typescript/bin/tsc'), '--project', 'tsconfig.json'],
    { cwd: consumerRoot, stdio: 'inherit' }
  );

  const packageRoot = path.join(
    consumerRoot,
    'node_modules',
    '@lynx-js',
    'lynx-library-plugin'
  );
  for (const excludedDir of ['test', 'scripts', 'node_modules']) {
    assert.ok(!fs.existsSync(path.join(packageRoot, excludedDir)));
  }
  console.log('Packed CommonJS, ESM, and TypeScript consumers passed.');
} finally {
  fs.rmSync(consumerRoot, { recursive: true, force: true });
}
