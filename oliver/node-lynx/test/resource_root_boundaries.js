// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

const assert = require('assert');
const fs = require('fs');
const os = require('os');
const path = require('path');

const { HeadlessLynxView, LynxEnv } = require('../dist/index.js');
const { getHelloWorldTemplateUrl } = require('./helpers/remote-templates.js');

const INSIDE_FLAG = '__nodeLynxResourceRootLoaded__';
const ESCAPE_FLAG = '__nodeLynxResourceRootEscaped__';
const ESCAPE_ERROR = '__nodeLynxResourceRootEscapeDetected__';

async function loadTemplate(view) {
  await view.loadTemplateFromUrl(getHelloWorldTemplateUrl());
  await view.waitForFrame();
}

async function main() {
  LynxEnv.init();

  const tempDir = fs.mkdtempSync(
    path.join(os.tmpdir(), 'node-lynx-resource-root-')
  );
  const resourceRoot = path.join(tempDir, 'allowed');
  const insideScript = path.join(resourceRoot, 'inside.js');
  const outsideScript = path.join(tempDir, 'outside.js');
  fs.mkdirSync(resourceRoot);
  fs.writeFileSync(insideScript, `globalThis.${INSIDE_FLAG} = true;`);
  fs.writeFileSync(outsideScript, `globalThis.${ESCAPE_FLAG} = true;`);
  fs.symlinkSync(outsideScript, path.join(resourceRoot, 'escape-link.js'));

  let handleEvaluationError = null;
  const view = new HeadlessLynxView({
    timeoutMs: 30000,
    resourceRootPaths: [resourceRoot],
    onErrorOccurred: (_level, _code, message) => {
      handleEvaluationError?.(message);
    },
  });

  async function expectBlocked(target, name) {
    const ack = `__nodeLynxResourceRootBoundary_${name}__`;
    const outcome = new Promise((resolve, reject) => {
      const timeout = setTimeout(() => {
        reject(new Error(`timed out checking resource root boundary: ${name}`));
      }, 30000);
      handleEvaluationError = (message) => {
        if (message.includes(ack) || message.includes(ESCAPE_ERROR)) {
          clearTimeout(timeout);
          resolve(message);
        }
      };
    });

    view.evaluateScript(
      `node-lynx://resource-root-boundary-${name}.js`,
      [
        `globalThis.${ESCAPE_FLAG} = false;`,
        `try { multiApps[currentAppId].nativeApp.loadScript(${JSON.stringify(
          target
        )}, '__Card__', { timeout: 5000 }); } catch (_) {}`,
        `if (globalThis.${ESCAPE_FLAG}) { throw new Error('${ESCAPE_ERROR}'); }`,
        `throw new Error('${ack}');`,
      ].join('\n')
    );

    const message = await outcome;
    handleEvaluationError = null;
    assert(
      message.includes(ack),
      `${name} escaped resourceRootPaths: ${message}`
    );
  }

  async function expectAllowed() {
    const ack = '__nodeLynxResourceRootAllowed__';
    const failure = '__nodeLynxResourceRootAllowedLoadFailed__';
    const outcome = new Promise((resolve, reject) => {
      const timeout = setTimeout(() => {
        reject(new Error('timed out loading a script inside the resource root'));
      }, 30000);
      handleEvaluationError = (message) => {
        if (message.includes(ack) || message.includes(failure)) {
          clearTimeout(timeout);
          resolve(message);
        }
      };
    });

    view.evaluateScript(
      'node-lynx://resource-root-allowed.js',
      [
        `globalThis.${INSIDE_FLAG} = false;`,
        "multiApps[currentAppId].nativeApp.loadScript('/inside.js', '__Card__', { timeout: 5000 });",
        `if (!globalThis.${INSIDE_FLAG}) { throw new Error('${failure}'); }`,
        `throw new Error('${ack}');`,
      ].join('\n')
    );

    const message = await outcome;
    handleEvaluationError = null;
    assert(message.includes(ack), `inside-root script did not load: ${message}`);
  }

  try {
    await loadTemplate(view);
    await expectAllowed();
    await expectBlocked('/../outside.js', 'parent-traversal');
    await expectBlocked('/escape-link.js', 'symlink');
    console.log('local resource lookup stays within configured roots.');
  } finally {
    view.destroy();
    fs.rmSync(tempDir, { recursive: true, force: true });
  }
}

main().catch((error) => {
  console.error(
    error instanceof Error ? error.stack || error.message : String(error)
  );
  process.exit(1);
});
