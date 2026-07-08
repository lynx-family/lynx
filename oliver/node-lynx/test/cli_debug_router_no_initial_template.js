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
  events: [],
  eventLog: [],
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
  traceStartCalls: 0,
  traceStartArgs: [],
  traceStopCalls: 0,
  traceStopArgs: [],
  traceTimerDelays: [],
};

function record(event) {
  state.events.push(event);
  state.eventLog.push({ event, time: Date.now() });
}

function writeState() {
  if (process.env.NODE_LYNX_CLI_TEST_STATE) {
    fs.writeFileSync(process.env.NODE_LYNX_CLI_TEST_STATE, JSON.stringify(state));
  }
}

const originalSetTimeout = global.setTimeout;
global.setTimeout = function patchedSetTimeout(callback, delay, ...args) {
  if (process.env.NODE_LYNX_CLI_TEST_TRACE_TIMER === '1' && typeof delay === 'number') {
    state.traceTimerDelays.push(delay);
    return originalSetTimeout(callback, Math.min(delay, 10), ...args);
  }
  return originalSetTimeout(callback, delay, ...args);
};

class MockHeadlessLynxView {
  constructor() {
    record('HeadlessLynxView.constructor');
    state.headlessViewConstructed += 1;
  }

  async loadTemplate() {}

  async loadTemplateFromUrl() {}

  async waitForFrame() {}

  async screenshot(...args) {
    record('HeadlessLynxView.screenshot');
    state.headlessScreenshotCalls += 1;
    state.headlessScreenshotArgs = args;
    return Buffer.from('png');
  }

  destroy() {}
}

class MockWindowedLynxView {
  constructor() {
    record('WindowedLynxView.constructor');
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
    record('LynxEnv.init');
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

const mockNativeBinding = {
  startTracing(filePath) {
    record('startTracing');
    state.traceStartCalls += 1;
    state.traceStartArgs.push(filePath);
    return 1001;
  },
  stopTracing(sessionId) {
    record('stopTracing');
    state.traceStopCalls += 1;
    state.traceStopArgs.push(sessionId);
    return true;
  },
};

const originalLoad = Module._load;
Module._load = function patchedLoad(request, parent, isMain) {
  if (request === './headless-lynx-view') {
    return {
      HeadlessLynxView: MockHeadlessLynxView,
      loadNodeLynxNativeBinding() {
        return mockNativeBinding;
      },
    };
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

async function runCli(args, extraEnv = {}) {
  const tempDir = await mkdtemp(
    path.join(os.tmpdir(), 'node-lynx-cli-test-')
  );
  const preloadPath = path.join(tempDir, 'preload.js');
  const statePath = path.join(tempDir, 'state.json');
  await writeFile(preloadPath, MOCK_PRELOAD);

  const env = {
    ...process.env,
    ...extraEnv,
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

function eventIndex(result, event) {
  return result.state.events.indexOf(event);
}

function eventTime(result, event) {
  const entry = result.state.eventLog.find((item) => item.event === event);
  assert.ok(entry, `missing event: ${event}`);
  return entry.time;
}

async function testTraceStartsBeforeDebugRouterAndView() {
  const outputPath = path.join(
    os.tmpdir(),
    `node-lynx-cli-trace-order-${process.pid}.png`
  );
  const tracePath = path.join(
    os.tmpdir(),
    `node-lynx-cli-trace-order-${process.pid}.pftrace`
  );
  try {
    const result = await runCli(
      [
        'render',
        '--template',
        path.join(PACKAGE_ROOT, 'package.json'),
        '--output',
        outputPath,
        '--trace',
        tracePath,
        '--trace-duration',
        '0.01',
      ],
      { NODE_LYNX_CLI_TEST_TRACE_TIMER: '1' }
    );

    assert.strictEqual(result.code, 0, result.stderr);
    assert.strictEqual(result.state.traceStartCalls, 1);
    assert.deepStrictEqual(result.state.traceStartArgs, [tracePath]);
    assert.strictEqual(result.state.traceStopCalls, 1);
    assert.deepStrictEqual(result.state.traceStopArgs, [1001]);
    assert.ok(
      eventIndex(result, 'startTracing') < eventIndex(result, 'LynxEnv.init')
    );
    assert.ok(
      eventIndex(result, 'startTracing') <
        eventIndex(result, 'HeadlessLynxView.constructor')
    );
  } finally {
    await rm(outputPath, { force: true });
    await rm(tracePath, { force: true });
  }
}

async function testTraceDurationDefaultsToFiveSeconds() {
  const outputPath = path.join(
    os.tmpdir(),
    `node-lynx-cli-trace-default-duration-${process.pid}.png`
  );
  const tracePath = path.join(
    os.tmpdir(),
    `node-lynx-cli-trace-default-duration-${process.pid}.pftrace`
  );
  try {
    const result = await runCli(
      [
        'render',
        '--no-debug-router',
        '--template',
        path.join(PACKAGE_ROOT, 'package.json'),
        '--output',
        outputPath,
        '--trace',
        tracePath,
      ],
      { NODE_LYNX_CLI_TEST_TRACE_TIMER: '1' }
    );

    assert.strictEqual(result.code, 0, result.stderr);
    assert.deepStrictEqual(result.state.traceTimerDelays, [5000]);
    assert.strictEqual(result.state.traceStopCalls, 1);
  } finally {
    await rm(outputPath, { force: true });
    await rm(tracePath, { force: true });
  }
}

async function testShortRenderWaitsForTraceStop() {
  const outputPath = path.join(
    os.tmpdir(),
    `node-lynx-cli-trace-wait-${process.pid}.png`
  );
  const tracePath = path.join(
    os.tmpdir(),
    `node-lynx-cli-trace-wait-${process.pid}.pftrace`
  );
  try {
    const result = await runCli([
      'render',
      '--no-debug-router',
      '--template',
      path.join(PACKAGE_ROOT, 'package.json'),
      '--output',
      outputPath,
      '--trace',
      tracePath,
      '--trace-duration',
      '0.05',
    ]);

    assert.strictEqual(result.code, 0, result.stderr);
    assert.strictEqual(result.state.traceStopCalls, 1);
    assert.ok(
      eventIndex(result, 'HeadlessLynxView.screenshot') <
        eventIndex(result, 'stopTracing')
    );
    assert.ok(
      eventTime(result, 'stopTracing') -
        eventTime(result, 'HeadlessLynxView.screenshot') >=
        25
    );
  } finally {
    await rm(outputPath, { force: true });
    await rm(tracePath, { force: true });
  }
}

async function testTraceArgValidation() {
  const missingTraceValue = await runCli([
    'render',
    '--trace',
    '--no-debug-router',
  ]);
  assert.strictEqual(missingTraceValue.code, 1);
  assert.match(missingTraceValue.stderr, /missing value for --trace/);

  const invalidDuration = await runCli([
    'render',
    '--no-debug-router',
    '--template',
    path.join(PACKAGE_ROOT, 'package.json'),
    '--trace',
    path.join(
      os.tmpdir(),
      `node-lynx-cli-invalid-duration-${process.pid}.pftrace`
    ),
    '--trace-duration',
    '0',
  ]);
  assert.strictEqual(invalidDuration.code, 1);
  assert.match(
    invalidDuration.stderr,
    /traceDuration must be a positive number/
  );

  const durationWithoutTrace = await runCli([
    'render',
    '--no-debug-router',
    '--template',
    path.join(PACKAGE_ROOT, 'package.json'),
    '--trace-duration',
    '5',
  ]);
  assert.strictEqual(durationWithoutTrace.code, 1);
  assert.match(durationWithoutTrace.stderr, /--trace-duration requires --trace/);
}

async function main() {
  await testRenderWaitsForOpenCardWithoutInitialView();
  await testImplicitRenderWaitsForOpenCardWithoutInitialView();
  await testPreviewWaitsForOpenCardWithoutInitialWindow();
  await testNoTemplateStillRequiresDebugRouter();
  await testRenderPassesScreenshotDelay();
  await testTraceStartsBeforeDebugRouterAndView();
  await testTraceDurationDefaultsToFiveSeconds();
  await testShortRenderWaitsForTraceStop();
  await testTraceArgValidation();
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
