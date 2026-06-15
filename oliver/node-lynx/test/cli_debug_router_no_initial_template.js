const assert = require('assert');
const { spawn } = require('child_process');
const { mkdtemp, readFile, rm, writeFile } = require('fs/promises');
const os = require('os');
const path = require('path');

const PACKAGE_ROOT = path.resolve(__dirname, '..');
const CLI_PATH = path.join(PACKAGE_ROOT, 'dist', 'cli.js');

const MOCK_PRELOAD = String.raw`
const fs = require('fs');
const Module = require('module');

const state = {
  initCalls: 0,
  setAppInfoCalls: 0,
  connectDevtoolsCalls: 0,
  setOpenCardCallbackCalls: 0,
  clearOpenCardCallbackCalls: 0,
  setClosePageCallbackCalls: 0,
  clearClosePageCallbackCalls: 0,
  headlessViewConstructed: 0,
  windowedViewConstructed: 0,
  headlessScreenshotCalls: 0,
  headlessScreenshotArgs: [],
  windowedWaitUntilClosedCalls: 0,
};

function writeState() {
  if (process.env.NODE_LYNX_CLI_TEST_STATE) {
    fs.writeFileSync(process.env.NODE_LYNX_CLI_TEST_STATE, JSON.stringify(state));
  }
}

class MockHeadlessLynxView {
  constructor() {
    state.headlessViewConstructed += 1;
  }

  async loadTemplate() {}

  async loadTemplateFromUrl() {}

  async waitForFrame() {}

  async screenshot(...args) {
    state.headlessScreenshotCalls += 1;
    state.headlessScreenshotArgs = args;
    return Buffer.from('png');
  }

  destroy() {}
}

class MockWindowedLynxView {
  constructor() {
    state.windowedViewConstructed += 1;
  }

  async loadTemplate() {}

  async loadTemplateFromUrl() {}

  async waitForFrame() {}

  async waitUntilClosed() {
    state.windowedWaitUntilClosedCalls += 1;
  }

  close() {}

  destroy() {}
}

const mockLynxEnv = {
  init() {
    state.initCalls += 1;
  },
  setAppInfo() {
    state.setAppInfoCalls += 1;
  },
  connectDevtools() {
    state.connectDevtoolsCalls += 1;
    return true;
  },
  setOpenCardCallback() {
    state.setOpenCardCallbackCalls += 1;
  },
  clearOpenCardCallback() {
    state.clearOpenCardCallbackCalls += 1;
  },
  setClosePageCallback(callback) {
    state.setClosePageCallbackCalls += 1;
    setImmediate(callback);
  },
  clearClosePageCallback() {
    state.clearClosePageCallbackCalls += 1;
  },
};

const originalLoad = Module._load;
Module._load = function patchedLoad(request, parent, isMain) {
  if (request === './headless-lynx-view') {
    return { HeadlessLynxView: MockHeadlessLynxView };
  }
  if (request === './windowed-lynx-view') {
    return { WindowedLynxView: MockWindowedLynxView };
  }
  if (request === './lynx-env') {
    return { LynxEnv: mockLynxEnv };
  }
  return originalLoad.call(this, request, parent, isMain);
};

process.on('exit', writeState);
`;

async function runCli(args) {
  const tempDir = await mkdtemp(
    path.join(os.tmpdir(), 'node-lynx-cli-test-')
  );
  const preloadPath = path.join(tempDir, 'preload.js');
  const statePath = path.join(tempDir, 'state.json');
  await writeFile(preloadPath, MOCK_PRELOAD);

  const env = {
    ...process.env,
    NODE_LYNX_CLI_TEST_STATE: statePath,
  };

  try {
    const result = await new Promise((resolve, reject) => {
      const child = spawn(process.execPath, [
        '--require',
        preloadPath,
        CLI_PATH,
        ...args,
      ], {
        cwd: PACKAGE_ROOT,
        env,
        stdio: ['ignore', 'pipe', 'pipe'],
      });
      let stdout = '';
      let stderr = '';
      const timeout = setTimeout(() => {
        child.kill('SIGKILL');
        reject(new Error(`node-lynx CLI timed out for args: ${args.join(' ')}`));
      }, 5000);

      child.stdout.on('data', (chunk) => {
        stdout += chunk;
      });
      child.stderr.on('data', (chunk) => {
        stderr += chunk;
      });
      child.on('error', (error) => {
        clearTimeout(timeout);
        reject(error);
      });
      child.on('close', (code, signal) => {
        clearTimeout(timeout);
        resolve({ code, signal, stdout, stderr });
      });
    });

    const state = JSON.parse(await readFile(statePath, 'utf8'));
    return { ...result, state };
  } finally {
    await rm(tempDir, { recursive: true, force: true });
  }
}

function assertNoInitialView(result) {
  assert.strictEqual(result.state.headlessViewConstructed, 0);
  assert.strictEqual(result.state.windowedViewConstructed, 0);
  assert.strictEqual(result.state.headlessScreenshotCalls, 0);
  assert.strictEqual(result.state.windowedWaitUntilClosedCalls, 0);
}

async function testRenderWaitsForOpenCardWithoutInitialView() {
  const result = await runCli(['render']);

  assert.strictEqual(result.code, 0, result.stderr);
  assert.match(
    result.stdout,
    /Headless Lynx is waiting for DebugRouter OpenCard\./
  );
  assert.strictEqual(result.state.initCalls, 1);
  assert.strictEqual(result.state.setOpenCardCallbackCalls, 1);
  assert.strictEqual(result.state.setClosePageCallbackCalls, 1);
  assert.strictEqual(result.state.clearOpenCardCallbackCalls, 1);
  assert.strictEqual(result.state.clearClosePageCallbackCalls, 1);
  assertNoInitialView(result);
}

async function testImplicitRenderWaitsForOpenCardWithoutInitialView() {
  const result = await runCli([]);

  assert.strictEqual(result.code, 0, result.stderr);
  assert.match(
    result.stdout,
    /Headless Lynx is waiting for DebugRouter OpenCard\./
  );
  assert.strictEqual(result.state.initCalls, 1);
  assert.strictEqual(result.state.setOpenCardCallbackCalls, 1);
  assert.strictEqual(result.state.setClosePageCallbackCalls, 1);
  assert.strictEqual(result.state.clearOpenCardCallbackCalls, 1);
  assert.strictEqual(result.state.clearClosePageCallbackCalls, 1);
  assertNoInitialView(result);
}

async function testPreviewWaitsForOpenCardWithoutInitialWindow() {
  const result = await runCli(['preview']);

  assert.strictEqual(result.code, 0, result.stderr);
  assert.match(
    result.stdout,
    /Node Lynx preview is waiting for DebugRouter OpenCard\./
  );
  assert.strictEqual(result.state.initCalls, 1);
  assert.strictEqual(result.state.setOpenCardCallbackCalls, 1);
  assert.strictEqual(result.state.setClosePageCallbackCalls, 1);
  assert.strictEqual(result.state.clearOpenCardCallbackCalls, 1);
  assert.strictEqual(result.state.clearClosePageCallbackCalls, 1);
  assertNoInitialView(result);
}

async function testNoTemplateStillRequiresDebugRouter() {
  const implicitWithoutDebugRouter = await runCli(['--no-debug-router']);
  assert.strictEqual(implicitWithoutDebugRouter.code, 1);
  assert.match(
    implicitWithoutDebugRouter.stderr,
    /missing Lynx bundle path or URL/
  );
  assert.strictEqual(implicitWithoutDebugRouter.state.initCalls, 0);
  assertNoInitialView(implicitWithoutDebugRouter);

  const renderWithoutDebugRouter = await runCli(['render', '--no-debug-router']);
  assert.strictEqual(renderWithoutDebugRouter.code, 1);
  assert.match(
    renderWithoutDebugRouter.stderr,
    /missing Lynx bundle path or URL/
  );
  assert.strictEqual(renderWithoutDebugRouter.state.initCalls, 0);
  assertNoInitialView(renderWithoutDebugRouter);

  const previewWithoutDebugRouter = await runCli([
    'preview',
    '--no-debug-router',
  ]);
  assert.strictEqual(previewWithoutDebugRouter.code, 1);
  assert.match(
    previewWithoutDebugRouter.stderr,
    /missing Lynx bundle path or URL/
  );
  assert.strictEqual(previewWithoutDebugRouter.state.initCalls, 0);
  assertNoInitialView(previewWithoutDebugRouter);
}

async function testRenderPassesScreenshotDelay() {
  const outputPath = path.join(
    os.tmpdir(),
    `node-lynx-cli-screenshot-delay-${process.pid}.png`
  );
  try {
    const result = await runCli([
      'render',
      '--no-debug-router',
      '--template',
      path.join(PACKAGE_ROOT, 'package.json'),
      '--output',
      outputPath,
      '--screenshot-delay',
      '250',
    ]);

    assert.strictEqual(result.code, 0, result.stderr);
    assert.strictEqual(result.state.headlessScreenshotCalls, 1);
    assert.deepStrictEqual(result.state.headlessScreenshotArgs, [
      { settleMs: 250 },
    ]);
  } finally {
    await rm(outputPath, { force: true });
  }
}

async function main() {
  await testRenderWaitsForOpenCardWithoutInitialView();
  await testImplicitRenderWaitsForOpenCardWithoutInitialView();
  await testPreviewWaitsForOpenCardWithoutInitialWindow();
  await testNoTemplateStillRequiresDebugRouter();
  await testRenderPassesScreenshotDelay();
}

main()
  .then(() => {
    console.log('CLI debug-router no initial template tests finished successfully.');
    process.exitCode = 0;
  })
  .catch((error) => {
    console.error('CLI debug-router no initial template tests failed:', error);
    process.exit(1);
  });
