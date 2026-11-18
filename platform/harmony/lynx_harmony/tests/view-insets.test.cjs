// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// Run with node --test platform/harmony/lynx_harmony/tests/view-insets.test.cjs
// from the Lynx workspace. Requires the workspace TypeScript devDependency.
const assert = require('node:assert/strict');
const { test } = require('node:test');
const { readFileSync } = require('node:fs');
const path = require('node:path');
const ts = require('typescript');

const compiled = ts.transpileModule(readFileSync(path.join(__dirname,
  '../src/main/ets/tasm/LynxViewInsets.ets'), 'utf8'), {
  compilerOptions: { module: ts.ModuleKind.CommonJS, target: ts.ScriptTarget.ES2021 },
  reportDiagnostics: true,
});
assert.equal(compiled.diagnostics.length, 0);
const model = {};
new Function('exports', compiled.outputText)(model);
const { LynxViewInsets, LynxInsetType: Type } = model;

test('category patch keeps other categories; null skips; replacement clears old metadata', () => {
  const first = new LynxViewInsets(new Map([[Type.NAVIGATION_BARS, {
    geometry: { type: 'edges', left: 0, top: 0, right: 0, bottom: 24 }, visible: true,
  }]]));
  const next = first.merge(new LynxViewInsets(new Map([
    [Type.IME, { geometry: { type: 'edges', left: 0, top: 0, right: 0, bottom: 300 }, visible: true }],
    [Type.NAVIGATION_BARS, null],
  ])));
  assert.deepEqual(next.get(Type.NAVIGATION_BARS), first.get(Type.NAVIGATION_BARS));
  assert.equal(first.get(Type.IME), undefined);
  assert.deepEqual(next.toNative(), next.merge(new LynxViewInsets()).toNative());
  const last = next.merge(new LynxViewInsets(new Map([[Type.NAVIGATION_BARS, {
    geometry: { type: 'edges', left: 0, top: 0, right: 0, bottom: 0 }, visible: false,
  }]])));
  assert.equal(last.get(Type.NAVIGATION_BARS).visible, false);
  assert.equal(last.get(Type.IME).geometry.bottom, 300);
});

test('zero geometry preserves independent visibility', () => {
  const value = new LynxViewInsets(new Map([[Type.IME, {
    geometry: { type: 'edges', left: 0, top: 0, right: 0, bottom: 0 }, visible: true,
  }]]));
  assert.equal(value.get(Type.IME).visible, true);
  assert.equal(value.get(Type.IME).geometry.bottom, 0);
});

test('input and output collections cannot mutate captured snapshots', () => {
  const rects = [{ x: -10, y: -20, width: 40, height: 30 }];
  const entries = new Map([[Type.DISPLAY_CUTOUT, { geometry: { type: 'rects', rects } }]]);
  const value = new LynxViewInsets(entries);
  rects[0].x = 500;
  entries.clear();
  const output = value.toNative();
  assert.equal(Object.prototype.hasOwnProperty.call(output.displayCutout, 'visible'), false);
  assert.equal(output.displayCutout.geometry.rects[0].x, -10);
  output.displayCutout.geometry.rects.length = 0;
  assert.equal(value.get(Type.DISPLAY_CUTOUT).geometry.rects.length, 1);
  const cleared = value.merge(new LynxViewInsets(new Map([[Type.DISPLAY_CUTOUT,
    { geometry: { type: 'rects', rects: [] } }]])));
  assert.equal(cleared.get(Type.DISPLAY_CUTOUT).geometry.rects.length, 0);
});

test('corner replacement and signed centers', () => {
  const first = new LynxViewInsets(new Map([[Type.ROUNDED_CORNERS, { geometry: {
    type: 'corners', topLeft: { centerX: -20, centerY: 10, radius: 30 },
  } }]]));
  assert.equal(first.get(Type.ROUNDED_CORNERS).geometry.topLeft.centerX, -20);
  const next = first.merge(new LynxViewInsets(new Map([[Type.ROUNDED_CORNERS,
    { geometry: { type: 'corners' } }]])));
  assert.equal(next.get(Type.ROUNDED_CORNERS).geometry.topLeft, undefined);
  assert.deepEqual(next.toNative().roundedCorners.geometry, { type: 'corners' });
});

test('invalid geometry and unsupported categories are rejected', () => {
  for (const geometry of [
    { type: 'edges', left: -1, top: 0, right: 0, bottom: 0 },
    { type: 'edges', left: 0, top: NaN, right: 0, bottom: 0 },
    { type: 'rects', rects: [] },
    { type: 'path' },
  ]) assert.throws(() => new LynxViewInsets(new Map([[Type.IME, { geometry }]])));
  assert.throws(() => new LynxViewInsets(new Map([['safeArea', {
    geometry: { type: 'edges', left: 0, top: 0, right: 0, bottom: 0 },
  }]])));
});
