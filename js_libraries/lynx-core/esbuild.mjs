// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

//@ts-check

import { execSync } from 'node:child_process';

import esbuild from 'esbuild';

const { TARGET } = process.env;

if (!TARGET || !['android', 'ios' , 'web'].includes(TARGET.toLowerCase())) {
  console.error(`target ${TARGET} not found`);
  process.exit(1);
}

const isWeb = TARGET.toLowerCase() === 'web';
/** @type {string} */
const commitHash = (function generateCommitHash() {
  try {
    const result = execSync('git rev-parse HEAD').toString();
    return result.trim();
  } catch (error) {
    console.error(`generateCommitHash error: ${error}`);
    try {
      const { Lynx, lynx } = JSON.parse(
        process.env['CUSTOM_CI_MR_DEPENDENCIES'] ?? '{}'
      );
      return Lynx?.commit ?? lynx?.commit ?? 'unknown_commit_hash';
    } catch {
      return 'unknown_commit_hash';
    }
  }
})();

/** @type {import('esbuild').BuildOptions} */
const commonOptions = {
  // General
  bundle: true,

  // Input
  entryPoints:isWeb ? ['src/index.web.ts']:['src/index.build.ts'],

  // Output
  charset: 'utf8',
  format: isWeb ? 'esm' :'iife',
  legalComments: 'none',
  write: true,

  // Path resolution
  alias: isWeb ? {
             // remove all polyfills for web
             '@lynx-js/ios-polyfill': './kernel-build/web-polyfill.js',
             '@lynx-js/ios-polyfill-promise': './kernel-build/web-polyfill.js',
             'regenerator-runtime/runtime': './kernel-build/web-polyfill.js',
           } :{
    '@lynx-js/ios-polyfill':
      TARGET.toLowerCase() === 'ios'
        ? '@lynx-js/ios-polyfill'
        : './kernel-build/android-polyfill.js',
  },

  // Transformation
  target:  isWeb ? ['esnext'] : TARGET.toLowerCase() === 'ios' ? ['ios11'] : ['es2019'],

  // Optimization
  define: {
    __BUILD_VERSION__: JSON.stringify(process.env.version || 'unknown_version'),
    __VERSION__: JSON.stringify(process.env.version || 'unknown_version'),
    __COMMIT_HASH__: JSON.stringify(
      commitHash || process.env.commitHash || 'unknown_commit_hash'
    ),
    __OPEN_INTERNAL_LOG__:JSON.stringify(false),
    __WEB__: isWeb ? 'true' : 'false',
  },

  // SourceMap
  sourcesContent: true,

  // Logging
  logOverride: {
    'direct-eval': 'info',
  },
};

// Development build
{
  const { errors, warnings } = esbuild.buildSync({
    ...commonOptions,
    // output
    outfile: 'output/lynx_core_dev.js',
    lineLimit: 80,
    banner: {
      js: `/** build time: ${new Date().toUTCString()}, commit: ${commitHash} */`,
    },

    // Optimization
    define: {
      ...commonOptions.define,
      NODE_ENV: JSON.stringify('development'),
    },

    // SourceMap
    sourcemap: 'inline',
  });

  if (warnings.length || errors.length) {
    process.exit(1);
  }
}

// Production build
{
  const { errors, warnings } = esbuild.buildSync({
    ...commonOptions,
    // Output
    outfile: isWeb ?'output/web/lynx_core.js' :'output/lynx_core.js',

    // Transformation
    target: isWeb ? ['esnext'] :TARGET.toLowerCase() === 'ios' ? ['ios11'] : ['es2015'],

    // Optimization
    define: {
      ...commonOptions.define,
      NODE_ENV: JSON.stringify('production'),
    },
    drop: ['console'],
    dropLabels: ['DEV', 'TEST'],
    minify: true,
    treeShaking: true,
    // TODO(wangqingyu): mangle props to reduce size.

    // SourceMap
    // TODO(wangqingyu): handle slardar
    sourcemap: 'external',
  });

  if (warnings.length || errors.length) {
    process.exit(1);
  }
}
