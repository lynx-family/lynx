// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

const assert = require('assert');

const { HeadlessLynxView, LynxEnv } = require('../dist/index.js');
const { getHelloWorldTemplateUrl } = require('./helpers/remote-templates.js');

const EVALUATION_ACK = '__nodeLynxSharedRuntimeSmokeComplete__';

async function loadTemplate(view, templateUrl) {
  await view.loadTemplateFromUrl(templateUrl);
  await view.waitForFrame();
}

function createEvaluationReceipt() {
  let handle;
  const promise = new Promise((resolve, reject) => {
    const timeout = setTimeout(() => {
      reject(new Error('timed out waiting for the evaluated script'));
    }, 30000);
    handle = ({ code, message }) => {
      clearTimeout(timeout);
      if (message.includes(EVALUATION_ACK)) {
        resolve();
      } else {
        reject(new Error(`script evaluation failed (${code}): ${message}`));
      }
    };
  });
  return { handle, promise };
}

async function main() {
  LynxEnv.init();

  const errors = [];
  let handleEvaluationError = null;
  const groupName = `node-lynx-shared-runtime-${process.pid}-${Date.now()}`;
  const viewOptions = {
    width: 390,
    height: 844,
    devicePixelRatio: 2,
    timeoutMs: 30000,
    groupName,
    onErrorOccurred: (_level, code, message) => {
      const error = { code, message };
      errors.push(error);
      handleEvaluationError?.(error);
    },
  };
  const templateUrl = getHelloWorldTemplateUrl();
  const firstView = new HeadlessLynxView(viewOptions);
  let secondView;

  try {
    await loadTemplate(firstView, templateUrl);
    firstView.evaluateScript(
      'node-lynx://shared-runtime-write.js',
      'globalThis.__nodeLynxSharedRuntimeSmoke = 42;'
    );

    secondView = new HeadlessLynxView(viewOptions);
    await loadTemplate(secondView, templateUrl);
    assert.deepStrictEqual(errors, []);

    const evaluationReceipt = createEvaluationReceipt();
    handleEvaluationError = evaluationReceipt.handle;
    secondView.evaluateScript(
      'node-lynx://shared-runtime-read.js',
      [
        'if (globalThis.__nodeLynxSharedRuntimeSmoke !== 42) {',
        "  throw new Error('views in the same group did not share the JS context');",
        '}',
        `throw new Error('${EVALUATION_ACK}');`,
      ].join('\n')
    );

    await evaluationReceipt.promise;
    assert.strictEqual(errors.length, 1);
    assert(errors[0].message.includes(EVALUATION_ACK));
    console.log('views in the same group share a JS context.');
  } finally {
    secondView?.destroy();
    firstView.destroy();
  }
}

main().catch((error) => {
  console.error(
    error instanceof Error ? error.stack || error.message : String(error)
  );
  process.exit(1);
});
