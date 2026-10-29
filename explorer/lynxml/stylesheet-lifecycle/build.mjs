// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { mkdirSync, readFileSync, writeFileSync } from 'node:fs';

// A custom host may require its own compatible bundle encoder.
const { default: tasm } = await import(
  process.env.TASM_MODULE || '@lynx-js/tasm'
);

// Set this to an address reachable by the device when not using adb reverse.
const bundleUrl = new URL(
  process.env.STYLESHEET_BUNDLE_URL || 'http://127.0.0.1:3078/themes.bundle'
).href;
const themes = JSON.parse(
  readFileSync(new URL('themes.json', import.meta.url), 'utf8')
);
const customSections = Object.fromEntries(
  Object.entries(themes).map(([name, selectors]) => [
    name,
    {
      encoding: 'CSS',
      content: {
        ruleList: Object.entries(selectors).map(([selector, properties]) => ({
          type: 'StyleRule',
          selectorText: { value: selector },
          style: Object.entries(properties).map(([name, value]) => ({
            name,
            value,
          })),
          variables: {},
        })),
      },
    },
  ])
);

// Match the native LynxML page's CSS compilation settings.
const result = await tasm.getEncodeMode('wasm')({
  compilerOptions: {
    enableFiberArch: true,
    useLepusNG: true,
    targetSdkVersion: '4.2',
    bundleModuleMode: 'ReturnByFunction',
    enableCSSSelector: true,
    enableCSSInvalidation: true,
    enableCSSParser: false,
    enableCSSStrictMode: false,
    enableCSSVariable: true,
    qjsCheck: false,
  },
  sourceContent: {
    appType: 'DynamicComponent',
    config: { enableCSSSelector: true, enableCSSInvalidation: true },
  },
  entryId: 'stylesheet-lifecycle',
  lepusCode: { root: '' },
  manifest: { '/app-service.js': '' },
  css: { cssMap: { 0: [] }, cssSource: { 0: 'empty.css' } },
  customSections,
});
if (result.status !== 0 || !result.buffer?.length) {
  throw new Error(result.error_msg || 'Stylesheet bundle encoding failed');
}

const output = new URL('dist/', import.meta.url);
mkdirSync(output, { recursive: true });
writeFileSync(new URL('themes.bundle', output), result.buffer);
const source = readFileSync(new URL('index.lynxml', import.meta.url), 'utf8');
// Escape '<' so a custom URL cannot terminate the XML script element.
const encodedUrl = JSON.stringify(bundleUrl).replaceAll('<', '\\u003c');
writeFileSync(
  new URL('index.lynxml', output),
  source.replace('__THEME_BUNDLE_URL__', () => encodedUrl)
);
console.log(
  `Built dist/index.lynxml and dist/themes.bundle; stylesheet URL: ${bundleUrl}`
);
