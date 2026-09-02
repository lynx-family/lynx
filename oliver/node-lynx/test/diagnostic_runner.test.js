// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { spawnSync } = require('node:child_process');
const {
  CONFIGURATION_EXIT_CODE,
  DEFAULT_CRASH_REPORT_TIMEOUT_MS,
  DEFAULT_OUTPUT_TAIL_BYTES,
  DEFAULT_TEST_TIMEOUT_MS,
  TIMEOUT_EXIT_CODE,
  findCrashReport,
  formatCrashReport,
  parseArguments,
  parseIps,
  readConfiguration,
  runNodeTest,
  runTestSuite,
  signalExitCode,
} = require('../scripts/run_tests_with_diagnostics');

const RUNNER_PATH = path.join(
  __dirname,
  '..',
  'scripts',
  'run_tests_with_diagnostics.js',
);

function writeScript(directory, name, source) {
  const scriptPath = path.join(directory, name);
  fs.writeFileSync(scriptPath, source);
  return scriptPath;
}

function writeIps(directory, name, metadata, report) {
  const reportPath = path.join(directory, name);
  fs.writeFileSync(
    reportPath,
    `${JSON.stringify(metadata)}\n${JSON.stringify(report, null, 2)}`,
  );
  return reportPath;
}

function createCaptureStream() {
  let output = '';
  return {
    stream: {
      write(chunk) {
        output += Buffer.isBuffer(chunk) ? chunk.toString('utf8') : chunk;
        return true;
      },
    },
    get output() {
      return output;
    },
  };
}

function createCapturedOptions(overrides = {}) {
  const stdout = createCaptureStream();
  const stderr = createCaptureStream();
  return {
    captures: { stdout, stderr },
    options: {
      crashReportTimeoutMs: 0,
      outputTailBytes: 256,
      platform: 'linux',
      stderr: stderr.stream,
      stdout: stdout.stream,
      terminationGraceMs: 100,
      testTimeoutMs: 2_000,
      ...overrides,
    },
  };
}

async function testProcessResults(directory) {
  const successFile = writeScript(
    directory,
    'success.js',
    "console.log('success output');\n",
  );
  const successCapture = createCapturedOptions();
  const success = await runNodeTest(successFile, successCapture.options);
  assert.equal(success.exitCode, 0);
  assert.equal(success.reason, 'success');
  assert.match(successCapture.captures.stdout.output, /success output/);
  assert.match(successCapture.captures.stdout.output, /TEST PASS .*exit=0/);
  assert.equal(successCapture.captures.stderr.output, '');

  const failureFile = writeScript(
    directory,
    'failure.js',
    [
      "process.stdout.write('stdout before failure\\n');",
      "process.stderr.write('stderr before failure\\n');",
      'process.exit(7);',
    ].join('\n'),
  );
  const failureCapture = createCapturedOptions();
  const failed = await runNodeTest(failureFile, failureCapture.options);
  assert.equal(failed.exitCode, 7);
  assert.equal(failed.reason, 'exit-code');
  assert.match(failureCapture.captures.stdout.output, /stdout before failure/);
  assert.match(failureCapture.captures.stderr.output, /stderr before failure/);
  assert.match(
    failureCapture.captures.stderr.output,
    /TEST FAIL reason=exit-code .*exit=7/,
  );
  assert.match(
    failureCapture.captures.stderr.output,
    /OUTPUT stdout-tail begin/,
  );
  assert.match(
    failureCapture.captures.stderr.output,
    /OUTPUT stderr-tail begin/,
  );

  const throwFile = writeScript(
    directory,
    'throw.js',
    "throw new Error('uncaught test failure');\n",
  );
  const throwCapture = createCapturedOptions();
  const threw = await runNodeTest(throwFile, throwCapture.options);
  assert.equal(threw.exitCode, 1);
  assert.equal(threw.reason, 'exit-code');
  assert.match(throwCapture.captures.stderr.output, /uncaught test failure/);
  assert.match(throwCapture.captures.stderr.output, /throw\.js:\d+/);

  const longOutput = '0123456789abcdefghijklmnopqrstuvwxyz';
  const tailFile = writeScript(
    directory,
    'tail.js',
    `process.stdout.write(${JSON.stringify(
      longOutput,
    )}, () => process.exit(9));\n`,
  );
  const tailCapture = createCapturedOptions({ outputTailBytes: 10 });
  const tailed = await runNodeTest(tailFile, tailCapture.options);
  assert.equal(tailed.stdoutTail.toString(), longOutput.slice(-10));
  assert.equal(tailed.stdoutTail.buffer.length, 10);

  const signalFile = writeScript(
    directory,
    'signal.js',
    "process.kill(process.pid, 'SIGTERM');\n",
  );
  const signalCapture = createCapturedOptions();
  const signaled = await runNodeTest(signalFile, signalCapture.options);
  assert.equal(signaled.signal, 'SIGTERM');
  assert.equal(signaled.exitCode, signalExitCode('SIGTERM'));
  assert.equal(signaled.reason, 'signal');
  assert.match(
    signalCapture.captures.stderr.output,
    /reason=signal .*signal=SIGTERM .*exit=/,
  );

  const timeoutFile = writeScript(
    directory,
    'timeout.js',
    [
      "process.on('SIGTERM', () => {});",
      'setInterval(() => {}, 1000);',
    ].join('\n'),
  );
  const timeoutCapture = createCapturedOptions({
    terminationGraceMs: 50,
    testTimeoutMs: 50,
  });
  const timedOut = await runNodeTest(timeoutFile, timeoutCapture.options);
  assert.equal(timedOut.exitCode, TIMEOUT_EXIT_CODE);
  assert.equal(timedOut.reason, 'timeout');
  assert.equal(timedOut.terminationSignal, 'SIGKILL');
  assert.match(
    timeoutCapture.captures.stderr.output,
    /reason=timeout .*termination-signal=SIGKILL .*exit=124/,
  );

  const spawnCapture = createCapturedOptions({
    nodeExecutable: path.join(directory, 'missing-node'),
  });
  const spawnFailed = await runNodeTest(successFile, spawnCapture.options);
  assert.equal(spawnFailed.exitCode, 1);
  assert.equal(spawnFailed.reason, 'spawn-error');
  assert.match(
    spawnCapture.captures.stderr.output,
    /reason=spawn-error .*ENOENT/,
  );
  assert.match(spawnCapture.captures.stderr.output, /COMMAND executable=/);
}

async function testDarwinSignalDiagnostics(directory) {
  const signalFile = writeScript(
    directory,
    'darwin-signal.js',
    "process.kill(process.pid, 'SIGTERM');\n",
  );
  const errorCapture = createCapturedOptions({
    findCrashReport: async () => {
      throw new Error('report directory unavailable');
    },
    platform: 'darwin',
  });
  const reportLookupFailed = await runNodeTest(
    signalFile,
    errorCapture.options,
  );
  assert.equal(reportLookupFailed.exitCode, signalExitCode('SIGTERM'));
  assert.match(
    errorCapture.captures.stderr.output,
    /CRASH REPORT status=error .*report directory unavailable/,
  );

  const missingCapture = createCapturedOptions({
    findCrashReport: async () => null,
    platform: 'darwin',
  });
  const reportMissing = await runNodeTest(signalFile, missingCapture.options);
  assert.equal(reportMissing.exitCode, signalExitCode('SIGTERM'));
  assert.match(
    missingCapture.captures.stderr.output,
    /CRASH REPORT status=not-found .*confirmed-signal=SIGTERM/,
  );
}

async function testSuiteLifecycle(directory) {
  const first = writeScript(directory, 'suite-pass.js', 'process.exit(0);\n');
  const second = writeScript(directory, 'suite-fail.js', 'process.exit(5);\n');
  const markerPath = path.join(directory, 'should-not-run');
  const third = writeScript(
    directory,
    'suite-skipped.js',
    `require('node:fs').writeFileSync(${JSON.stringify(
      markerPath,
    )}, 'ran');\n`,
  );
  const capture = createCapturedOptions();
  const exitCode = await runTestSuite(
    [first, second, third],
    capture.options,
  );
  assert.equal(exitCode, 5);
  assert.equal(fs.existsSync(markerPath), false);
  assert.match(capture.captures.stdout.output, /SUITE START total=3/);
  assert.match(
    capture.captures.stderr.output,
    /SUITE FAIL passed=1 failed=1 skipped=1 .*exit=5/,
  );

  const passCapture = createCapturedOptions();
  const successExitCode = await runTestSuite([first], passCapture.options);
  assert.equal(successExitCode, 0);
  assert.match(
    passCapture.captures.stdout.output,
    /SUITE PASS passed=1 failed=0 skipped=0 .*exit=0/,
  );
}

async function testCrashReport(directory) {
  writeIps(directory, 'node-wrong.ips', { pid: 100 }, { pid: 100 });
  const frames = Array.from({ length: 45 }, (_, index) => ({
    imageIndex: index === 1 ? 1 : 0,
    imageOffset: index * 16,
    ...(index === 0
      ? { symbol: 'native::Run()', symbolLocation: 4 }
      : {}),
  }));
  const expectedPath = path.join(directory, 'node-match.ips');
  const writeTimer = setTimeout(() => {
    writeIps(
      directory,
      'node-match.ips',
      { pid: 200 },
      {
        pid: 200,
        exception: { type: 'EXC_BAD_ACCESS', signal: 'SIGSEGV' },
        termination: { namespace: 'SIGNAL', code: 11 },
        faultingThread: 0,
        threads: [{ frames }],
        usedImages: [
          { name: 'native-addon.node', path: '/tmp/native-addon.node' },
          { name: 'libsystem_platform.dylib' },
        ],
      },
    );
  }, 50);
  const match = await findCrashReport({
    pid: 200,
    startedAtMs: Date.now(),
    reportsDirectory: directory,
    timeoutMs: 500,
    pollIntervalMs: 10,
  });
  clearTimeout(writeTimer);
  assert.equal(match.path, expectedPath);
  assert.equal(parseIps(fs.readFileSync(expectedPath, 'utf8')).pid, 200);

  const summary = formatCrashReport(match.report);
  assert.match(
    summary,
    /CRASH exception: .*type=EXC_BAD_ACCESS.*signal=SIGSEGV/,
  );
  assert.match(
    summary,
    /CRASH termination: .*namespace=SIGNAL.*code=11/,
  );
  assert.match(summary, /faulting-thread: 0 frames=45 showing=40/);
  assert.match(summary, /#0 native-addon\.node native::Run\(\) \+ 4/);
  assert.match(summary, /#1 libsystem_platform\.dylib 0x10/);
  assert.match(summary, /#39 native-addon\.node/);
  assert.doesNotMatch(summary, /#40 /);
}

function testConfiguration(directory) {
  assert.deepEqual(readConfiguration({}), {
    testTimeoutMs: DEFAULT_TEST_TIMEOUT_MS,
    crashReportTimeoutMs: DEFAULT_CRASH_REPORT_TIMEOUT_MS,
    outputTailBytes: DEFAULT_OUTPUT_TAIL_BYTES,
  });
  assert.deepEqual(
    readConfiguration({
      NODE_LYNX_CRASH_REPORT_TIMEOUT_MS: '0',
      NODE_LYNX_TEST_OUTPUT_TAIL_BYTES: '32',
      NODE_LYNX_TEST_TIMEOUT_MS: '500',
    }),
    {
      testTimeoutMs: 500,
      crashReportTimeoutMs: 0,
      outputTailBytes: 32,
    },
  );
  assert.deepEqual(parseArguments(['--', 'one.js', 'two.js']), [
    'one.js',
    'two.js',
  ]);
  assert.throws(() => parseArguments(['one.js']), /usage:/);
  assert.throws(
    () => readConfiguration({ NODE_LYNX_TEST_TIMEOUT_MS: 'invalid' }),
    /invalid NODE_LYNX_TEST_TIMEOUT_MS/,
  );

  const cliResult = spawnSync(process.execPath, [RUNNER_PATH, '--', 'unused.js'], {
    cwd: directory,
    encoding: 'utf8',
    env: {
      ...process.env,
      NODE_LYNX_TEST_TIMEOUT_MS: 'invalid',
    },
  });
  assert.equal(cliResult.status, CONFIGURATION_EXIT_CODE);
  assert.match(cliResult.stderr, /CONFIG ERROR .*NODE_LYNX_TEST_TIMEOUT_MS/);
}

async function main() {
  const directory = fs.mkdtempSync(
    path.join(os.tmpdir(), 'node-lynx-diagnostic-runner-'),
  );
  try {
    await testProcessResults(directory);
    await testDarwinSignalDiagnostics(directory);
    await testSuiteLifecycle(directory);
    await testCrashReport(directory);
    testConfiguration(directory);
    console.log('diagnostic runner tests passed');
  } finally {
    fs.rmSync(directory, { recursive: true, force: true });
  }
}

main().catch((error) => {
  console.error(error.stack || error.message);
  process.exitCode = 1;
});
