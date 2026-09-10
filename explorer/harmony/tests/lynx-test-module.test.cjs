// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// Run with node --test explorer/harmony/tests/lynx-test-module.test.cjs from the workspace.
// These host-side contract tests do not replace ArkTS compilation or device tests.
const assert = require('node:assert/strict');
const { test } = require('node:test');
const { readFileSync } = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const ts = require('typescript');

function loadArkTS(file, dependencies) {
  const source = readFileSync(file, 'utf8');
  const compiled = ts.transpileModule(source, {
    compilerOptions: { module: ts.ModuleKind.CommonJS, target: ts.ScriptTarget.ES2021 },
  }).outputText;
  const module = { exports: {} };
  vm.runInNewContext(compiled, {
    module, exports: module.exports, setTimeout,
    require(name) {
      assert.ok(name in dependencies, `Unexpected dependency: ${name}`);
      return dependencies[name];
    },
  }, { filename: file });
  return module.exports;
}

const metadata = loadArkTS(path.join(__dirname,
  '../../../platform/harmony/lynx_harmony/src/main/ets/tasm/MetaData.ets'), {
  'liblynx.so': { default: {} },
  '../base/LLog': { LLog: { d() {}, e() {} } },
});
class Module {
  constructor(context, param) { this.context = context; this.param = param; }
}
const { LynxTestModule, LynxTestModuleHost, LynxTestButton } = loadArkTS(path.join(__dirname,
  '../lynx_explorer/src/main/ets/module/LynxTestModule.ets'), {
  '@lynx/lynx': { ...metadata, LynxModule: Module },
});
const plain = value => JSON.parse(JSON.stringify(value));

function fixture() {
  const calls = [];
  const context = {
    updateMetaData(value) { calls.push(['update', value]); },
    reload(value) { calls.push(['reload', value]); },
    loadTemplate(...values) { calls.push(['load', ...values]); },
    getPageDataByKeyAsync(keys, callback) { calls.push(['readKeys', keys]); callback({ selected: 7 }); },
    sendGlobalEvent(name, params) { calls.push(['event', name, params]); },
    updateScreenMetrics(width, height) { calls.push(['screen', width, height]); },
  };
  const host = new LynxTestModuleHost(info => calls.push(['button', info]), () => calls.push(['back']));
  return { calls, context, host, module: new LynxTestModule(context, host) };
}

test('exposes only the intended bridge methods and sync declarations', () => {
  assert.deepEqual(Object.getOwnPropertyNames(LynxTestModule.prototype).sort(), [
    'constructor', 'call', 'invoke', 'callSync', 'invokeSync', 'updateData', 'resetData',
    'updateGlobalProps', 'reloadTemplate', 'reload', 'getPageDataByKey',
    'updateScreenMatrix', 'addButton', 'eventTest', 'valueTest', 'back',
  ].sort());
  assert.deepEqual(plain(LynxTestModule.syncMethods), ['callSync', 'invokeSync']);
  assert.deepEqual(plain(fixture().module.callSync('test', {})), { result: '----lepus value--success' });
});

test('uses the SDK update, reset and reload modes without mixing page contexts', () => {
  const first = fixture();
  const second = fixture();
  first.module.updateData({ a: 1 });
  first.module.resetData({ b: 2 });
  first.module.updateGlobalProps({ theme: 'dark' });
  first.module.reloadTemplate({ c: 3 }, { theme: 'light' });
  assert.equal(first.calls[0][1].updateMode, metadata.LynxUpdateMode.UPDATE);
  assert.deepEqual(plain(first.calls[0][1].templateData.data), { a: 1 });
  assert.equal(first.calls[1][1].updateMode, metadata.LynxUpdateMode.RESET);
  assert.deepEqual(plain(first.calls[1][1].templateData.data), { b: 2 });
  assert.equal(first.calls[2][1].templateData, undefined);
  assert.deepEqual(plain(first.calls[2][1].globalProps.data), { theme: 'dark' });
  assert.equal(first.calls[3][0], 'reload');
  assert.deepEqual(plain(first.calls[3][1].templateData.data), { c: 3 });
  assert.deepEqual(plain(first.calls[3][1].globalProps.data), { theme: 'light' });
  assert.equal(second.calls.length, 0);
});

test('queries selected keys through the existing context API', () => {
  const { calls, module } = fixture();
  module.updateData({ initial: 1 });
  module.getPageDataByKey(['selected'], data => assert.deepEqual(data, { selected: 7 }));
  assert.deepEqual(calls[1], ['readKeys', ['selected']]);
});

test('preserves event payloads for objects, arrays, primitives and invalid JSON', () => {
  const { calls, module } = fixture();
  module.eventTest('hello');
  module.valueTest('{"answer":42}');
  module.valueTest('[1,2]');
  module.valueTest('42');
  module.valueTest('null');
  module.valueTest('not JSON');
  assert.deepEqual(plain(calls), [
    ['event', 'test', [10, 'hello']], ['event', 'test', [{ answer: 42 }]],
    ['event', 'test', [[1, 2]]], ['event', 'test', ['42']],
    ['event', 'test', ['null']], ['event', 'test', ['not JSON']],
  ]);
});

test('delegates reload, metrics, buttons and navigation to the current host', () => {
  const { calls, module } = fixture();
  module.reload();
  module.updateScreenMatrix({ width: 720, height: 1280 });
  module.addButton({ left: 10, top: 20, count: 3 });
  module.back();
  assert.equal(calls[0][0], 'load');
  assert.equal(calls[0][1], undefined);
  assert.deepEqual(plain(calls[0][4].templateData.data), {});
  assert.deepEqual(calls.slice(1), [
    ['screen', 720, 1280], ['button', { left: 10, top: 20, count: 3 }], ['back'],
  ]);
  const button = new LynxTestButton(1, { count: 3, left: 10 });
  assert.equal(button.count, 3);
  assert.equal(button.fontSize, 16);
});

test('callback is asynchronous and is suppressed when its page disappears', async () => {
  const first = fixture();
  const second = fixture();
  let count = 0;
  const completed = new Promise(resolve => first.module.call('test', {}, data => {
    assert.deepEqual(plain(data), { result: 'success' });
    count++;
    resolve();
  }));
  second.module.invoke({}, () => assert.fail('Callback outlived its page'));
  second.host.active = false;
  assert.equal(count, 0);
  await completed;
  await new Promise(resolve => setTimeout(resolve, 25));
  assert.equal(count, 1);
});

test('page disappearance disables host operations and late data callbacks', () => {
  const { calls, context, host, module } = fixture();
  let lateCallback;
  context.getPageDataByKeyAsync = (_, callback) => { lateCallback = callback; };
  module.getPageDataByKey([], () => assert.fail('Data callback outlived its page'));
  host.active = false;
  lateCallback({ ignored: true });
  module.updateData({ ignored: true });
  module.resetData({ ignored: true });
  module.reload();
  module.eventTest('ignored');
  module.addButton({});
  module.back();
  module.getPageDataByKey([], () => assert.fail('Queried a destroyed page'));
  assert.equal(calls.length, 0);
});
