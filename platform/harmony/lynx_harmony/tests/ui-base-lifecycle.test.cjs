// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// Run with node --test platform/harmony/lynx_harmony/tests/ui-base-lifecycle.test.cjs
// from the Lynx workspace. These bridge contract tests use a native test double;
// they do not replace native compilation or device tree-removal tests.
const assert = require('node:assert/strict');
const { test } = require('node:test');
const { readFileSync } = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const ts = require('typescript');

class NativeUI {
  constructor(...args) {
    this.receiver = args[0];
    this.ready = args[11];
    this.reload = args[23];
    this.removed = args[24];
  }
  setFrameNode() {}
  notifyReady() { this.ready.call(this.receiver); }
  notifyReload() { this.reload.call(this.receiver); }
  notifyRemoved() { this.removed.call(this.receiver); }
}

const file = path.join(__dirname, '../src/main/ets/tasm/behavior/ui/UIBase.ets');
const compiled = ts.transpileModule(readFileSync(file, 'utf8'), {
  compilerOptions: { module: ts.ModuleKind.CommonJS, target: ts.ScriptTarget.ES2021 },
}).outputText;
const dependencies = {
  'liblynx.so': { default: { UIBase: NativeUI } },
  '@kit.ArkUI': {},
  '../../../base/TraceEvent': {
    TraceCategory: { Lynx: 0 },
    TraceEvent: { beginSection() {}, endSection() {} },
  },
  '../../../base/TraceEventDef': { TraceEventDef: {} },
};
const loaded = { exports: {} };
vm.runInNewContext(compiled, {
  module: loaded, exports: loaded.exports,
  require(name) {
    assert.ok(name in dependencies, `Unexpected dependency: ${name}`);
    return dependencies[name];
  },
}, { filename: file });
const { UIBase } = loaded.exports;

class CustomUI extends UIBase {
  events = [];
  onNodeReady() { super.onNodeReady(); this.events.push('ready'); }
  onNodeReload() { this.events.push('reload'); }
  onNodeRemoved() { this.events.push('removed'); }
  dispose() { this.events.push('dispose'); super.dispose(); }
}

function attach(UI = CustomUI) {
  const ui = new UI({});
  ui.setSign(1, 'custom-view');
  ui.attachToNative({}, [0, 0]);
  return ui;
}

test('forwards removal to the correct instance without shifting ready or reload', () => {
  const first = attach();
  const second = attach();
  first.nativeUI.notifyReady();
  first.nativeUI.notifyReload();
  first.nativeUI.notifyRemoved();
  assert.deepEqual(first.events, ['ready', 'reload', 'removed']);
  assert.deepEqual(second.events, []);
});

test('removal keeps the UI available for later ready and removal callbacks', () => {
  const ui = attach();
  const nativeUI = ui.nativeUI;
  nativeUI.notifyReady();
  nativeUI.notifyRemoved();
  assert.equal(ui.nativeUI, nativeUI);
  nativeUI.notifyReady();
  nativeUI.notifyRemoved();
  assert.deepEqual(ui.events, ['ready', 'removed', 'ready', 'removed']);
  ui.dispose();
  assert.equal(ui.nativeUI, undefined);
  assert.deepEqual(ui.events, ['ready', 'removed', 'ready', 'removed', 'dispose']);
});

test('existing subclasses can inherit the no-op removal callback', () => {
  const ui = attach(UIBase);
  const nativeUI = ui.nativeUI;
  assert.doesNotThrow(() => nativeUI.notifyRemoved());
  assert.equal(ui.nativeUI, nativeUI);
});
