#!/usr/bin/env node
// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

'use strict';

const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { spawn } = require('node:child_process');

const DEFAULT_TEST_TIMEOUT_MS = 120_000;
const DEFAULT_CRASH_REPORT_TIMEOUT_MS = 30_000;
const DEFAULT_OUTPUT_TAIL_BYTES = 8_192;
const DEFAULT_TERMINATION_GRACE_MS = 5_000;
const MAX_CRASH_FRAMES = 40;
const TIMEOUT_EXIT_CODE = 124;
const CONFIGURATION_EXIT_CODE = 2;

class ConfigurationError extends Error {}

class OutputTail {
  constructor(limitBytes) {
    this.limitBytes = limitBytes;
    this.buffer = Buffer.alloc(0);
  }

  append(chunk) {
    if (this.limitBytes === 0) {
      return;
    }
    const buffer = Buffer.isBuffer(chunk) ? chunk : Buffer.from(chunk);
    if (buffer.length >= this.limitBytes) {
      this.buffer = buffer.subarray(buffer.length - this.limitBytes);
      return;
    }
    const combined = Buffer.concat([this.buffer, buffer]);
    this.buffer =
      combined.length > this.limitBytes
        ? combined.subarray(combined.length - this.limitBytes)
        : combined;
  }

  toString() {
    return this.buffer.length > 0 ? this.buffer.toString('utf8') : '<empty>';
  }
}

function writeLine(stream, message) {
  stream.write(`${message}\n`);
}

function parseNonNegativeInteger(env, name, defaultValue) {
  const rawValue = env[name];
  if (rawValue === undefined) {
    return defaultValue;
  }
  if (!/^\d+$/.test(rawValue)) {
    throw new ConfigurationError(
      `invalid ${name}=${JSON.stringify(
        rawValue,
      )}: expected a non-negative integer`,
    );
  }
  const value = Number(rawValue);
  if (!Number.isSafeInteger(value)) {
    throw new ConfigurationError(
      `invalid ${name}=${JSON.stringify(rawValue)}: value is too large`,
    );
  }
  return value;
}

function readConfiguration(env = process.env) {
  return {
    testTimeoutMs: parseNonNegativeInteger(
      env,
      'NODE_LYNX_TEST_TIMEOUT_MS',
      DEFAULT_TEST_TIMEOUT_MS,
    ),
    crashReportTimeoutMs: parseNonNegativeInteger(
      env,
      'NODE_LYNX_CRASH_REPORT_TIMEOUT_MS',
      DEFAULT_CRASH_REPORT_TIMEOUT_MS,
    ),
    outputTailBytes: parseNonNegativeInteger(
      env,
      'NODE_LYNX_TEST_OUTPUT_TAIL_BYTES',
      DEFAULT_OUTPUT_TAIL_BYTES,
    ),
  };
}

function parseArguments(args) {
  const separator = args.indexOf('--');
  if (separator !== 0 || args.length === 1) {
    throw new ConfigurationError(
      'usage: run_tests_with_diagnostics.js -- <test> [test ...]',
    );
  }
  return args.slice(1);
}

function parseIps(content) {
  const firstNewline = content.indexOf('\n');
  if (firstNewline < 0) {
    const report = JSON.parse(content);
    return { metadata: {}, ...report };
  }

  const metadata = JSON.parse(content.slice(0, firstNewline));
  const report = JSON.parse(content.slice(firstNewline + 1));
  return { metadata, ...report };
}

function summarizeObject(value) {
  if (!value || typeof value !== 'object') {
    return String(value ?? 'unavailable');
  }
  return Object.entries(value)
    .filter(([, item]) => item !== null && item !== undefined)
    .map(([key, item]) => {
      const rendered = typeof item === 'object' ? JSON.stringify(item) : item;
      return `${key}=${rendered}`;
    })
    .join(' ');
}

function formatCrashFrame(frame, index, usedImages) {
  const image = usedImages[frame.imageIndex];
  const imageName = image?.name || image?.path || `image[${frame.imageIndex}]`;
  let location;
  if (frame.symbol) {
    location = `${frame.symbol}`;
    if (Number.isFinite(frame.symbolLocation)) {
      location += ` + ${frame.symbolLocation}`;
    }
  } else if (Number.isFinite(frame.imageOffset)) {
    location = `0x${frame.imageOffset.toString(16)}`;
  } else {
    location = 'unknown offset';
  }
  return `[node-test]   #${index} ${imageName} ${location}`;
}

function formatCrashReport(report, maxFrames = MAX_CRASH_FRAMES) {
  const lines = [
    `[node-test] CRASH exception: ${summarizeObject(report.exception)}`,
    `[node-test] CRASH termination: ${summarizeObject(report.termination)}`,
  ];
  const threadIndex = report.faultingThread;
  const thread = Number.isInteger(threadIndex)
    ? report.threads?.[threadIndex]
    : undefined;
  if (!thread) {
    lines.push('[node-test] CRASH faulting-thread: unavailable');
    return lines.join('\n');
  }

  const frames = Array.isArray(thread.frames) ? thread.frames : [];
  lines.push(
    `[node-test] CRASH faulting-thread: ${threadIndex} frames=${frames.length} showing=${Math.min(
      frames.length,
      maxFrames,
    )}`,
  );
  const usedImages = Array.isArray(report.usedImages) ? report.usedImages : [];
  frames
    .slice(0, maxFrames)
    .forEach((frame, index) =>
      lines.push(formatCrashFrame(frame, index, usedImages)),
    );
  return lines.join('\n');
}

async function readMatchingCrashReport({
  pid,
  startedAtMs,
  reportsDirectory,
}) {
  let entries;
  try {
    entries = await fs.promises.readdir(reportsDirectory, {
      withFileTypes: true,
    });
  } catch (error) {
    if (error.code === 'ENOENT') {
      return null;
    }
    throw error;
  }

  const candidates = [];
  for (const entry of entries) {
    if (!entry.isFile() || !/^node.*\.ips$/.test(entry.name)) {
      continue;
    }
    const reportPath = path.join(reportsDirectory, entry.name);
    const stat = await fs.promises.stat(reportPath);
    if (stat.mtimeMs >= startedAtMs - 2_000) {
      candidates.push({ reportPath, mtimeMs: stat.mtimeMs });
    }
  }
  candidates.sort((left, right) => right.mtimeMs - left.mtimeMs);

  for (const candidate of candidates) {
    try {
      const content = await fs.promises.readFile(candidate.reportPath, 'utf8');
      const report = parseIps(content);
      const reportPid = report.pid ?? report.metadata?.pid;
      if (reportPid === pid) {
        return { path: candidate.reportPath, report };
      }
    } catch {
      // Crash reports can be observed while macOS is still writing them. A
      // later polling iteration will retry the file.
    }
  }
  return null;
}

function delay(milliseconds) {
  return new Promise((resolve) => setTimeout(resolve, milliseconds));
}

async function findCrashReport({
  pid,
  startedAtMs,
  reportsDirectory = path.join(
    os.homedir(),
    'Library',
    'Logs',
    'DiagnosticReports',
  ),
  timeoutMs = DEFAULT_CRASH_REPORT_TIMEOUT_MS,
  pollIntervalMs = 250,
}) {
  const deadline = Date.now() + timeoutMs;
  do {
    const match = await readMatchingCrashReport({
      pid,
      startedAtMs,
      reportsDirectory,
    });
    if (match) {
      return match;
    }
    if (Date.now() >= deadline) {
      return null;
    }
    await delay(Math.min(pollIntervalMs, Math.max(1, deadline - Date.now())));
  } while (Date.now() <= deadline);
  return null;
}

function signalExitCode(signal) {
  const signalNumber = os.constants.signals[signal];
  return Number.isInteger(signalNumber) ? 128 + signalNumber : 128;
}

function captureChildOutput(child, { stdout, stderr, outputTailBytes }) {
  const stdoutTail = new OutputTail(outputTailBytes);
  const stderrTail = new OutputTail(outputTailBytes);
  const onStdout = (chunk) => {
    stdoutTail.append(chunk);
    stdout.write(chunk);
  };
  const onStderr = (chunk) => {
    stderrTail.append(chunk);
    stderr.write(chunk);
  };
  child.stdout?.on('data', onStdout);
  child.stderr?.on('data', onStderr);

  return {
    stdoutTail,
    stderrTail,
    stop() {
      child.stdout?.off('data', onStdout);
      child.stderr?.off('data', onStderr);
    },
  };
}

function waitForChild(child, { testTimeoutMs, terminationGraceMs }) {
  return new Promise((resolve) => {
    let settled = false;
    let timedOut = false;
    let terminationSignal;
    let timeoutTimer;
    let forceKillTimer;

    const finish = (result) => {
      if (settled) {
        return;
      }
      settled = true;
      clearTimeout(timeoutTimer);
      clearTimeout(forceKillTimer);
      resolve({ ...result, timedOut, terminationSignal });
    };

    child.once('error', (error) => finish({ spawnError: error }));
    child.once('close', (code, signal) => finish({ code, signal }));

    if (testTimeoutMs > 0) {
      timeoutTimer = setTimeout(() => {
        timedOut = true;
        terminationSignal = 'SIGTERM';
        child.kill('SIGTERM');
        forceKillTimer = setTimeout(() => {
          if (child.exitCode === null && child.signalCode === null) {
            terminationSignal = 'SIGKILL';
            child.kill('SIGKILL');
          }
        }, terminationGraceMs);
      }, testTimeoutMs);
    }
  });
}

function writeOutputTail(stderr, name, tail) {
  writeLine(
    stderr,
    `[node-test] OUTPUT ${name}-tail begin captured-bytes=${tail.buffer.length} limit-bytes=${tail.limitBytes}`,
  );
  writeLine(stderr, tail.toString());
  writeLine(stderr, `[node-test] OUTPUT ${name}-tail end`);
}

async function writeCrashDiagnostics({
  pid,
  signal,
  startedAtMs,
  stderr,
  crashReportTimeoutMs,
  reportsDirectory,
  pollIntervalMs,
  findCrashReportImpl,
}) {
  const signalNumber = os.constants.signals[signal];
  const lookupStartedAtMs = Date.now();
  try {
    const crashReport = await findCrashReportImpl({
      pid,
      startedAtMs,
      reportsDirectory,
      timeoutMs: crashReportTimeoutMs,
      pollIntervalMs,
    });
    const waitedMs = Date.now() - lookupStartedAtMs;
    if (!crashReport) {
      writeLine(
        stderr,
        `[node-test] CRASH REPORT status=not-found pid=${pid} timeout-ms=${crashReportTimeoutMs} waited-ms=${waitedMs} confirmed-signal=${signal} signal-number=${
          signalNumber ?? 'unknown'
        }`,
      );
      return;
    }
    writeLine(
      stderr,
      `[node-test] CRASH REPORT status=found pid=${pid} waited-ms=${waitedMs} path=${JSON.stringify(
        crashReport.path,
      )}`,
    );
    writeLine(stderr, formatCrashReport(crashReport.report));
  } catch (error) {
    writeLine(
      stderr,
      `[node-test] CRASH REPORT status=error pid=${pid} error=${JSON.stringify(
        error.message,
      )} confirmed-signal=${signal} signal-number=${
        signalNumber ?? 'unknown'
      }`,
    );
  }
}

async function runNodeTest(testFile, options = {}) {
  const stdout = options.stdout || process.stdout;
  const stderr = options.stderr || process.stderr;
  const cwd = options.cwd || process.cwd();
  const env = options.env || process.env;
  const platform = options.platform || process.platform;
  const nodeExecutable = options.nodeExecutable || process.execPath;
  const testTimeoutMs = options.testTimeoutMs ?? DEFAULT_TEST_TIMEOUT_MS;
  const crashReportTimeoutMs =
    options.crashReportTimeoutMs ?? DEFAULT_CRASH_REPORT_TIMEOUT_MS;
  const outputTailBytes =
    options.outputTailBytes ?? DEFAULT_OUTPUT_TAIL_BYTES;
  const terminationGraceMs =
    options.terminationGraceMs ?? DEFAULT_TERMINATION_GRACE_MS;
  const startedAtMs = Date.now();
  const startedAt = new Date(startedAtMs).toISOString();
  const child = spawn(nodeExecutable, [testFile], {
    cwd,
    env,
    stdio: ['inherit', 'pipe', 'pipe'],
  });
  const output = captureChildOutput(child, {
    stdout,
    stderr,
    outputTailBytes,
  });
  writeLine(
    stdout,
    `[node-test] TEST START index=${options.index ?? 1}/${
      options.total ?? 1
    } file=${JSON.stringify(testFile)} pid=${child.pid ?? 'unavailable'} started-at=${startedAt}`,
  );

  const processResult = await waitForChild(child, {
    testTimeoutMs,
    terminationGraceMs,
  });
  output.stop();
  const durationMs = Date.now() - startedAtMs;
  const baseResult = {
    ...processResult,
    pid: child.pid,
    durationMs,
    stdoutTail: output.stdoutTail,
    stderrTail: output.stderrTail,
  };

  if (processResult.spawnError) {
    const exitCode = 1;
    writeLine(
      stderr,
      `[node-test] TEST FAIL reason=spawn-error file=${JSON.stringify(
        testFile,
      )} pid=${child.pid ?? 'unavailable'} duration-ms=${durationMs} exit=${exitCode} error=${JSON.stringify(
        processResult.spawnError.message,
      )}`,
    );
    writeLine(
      stderr,
      `[node-test] COMMAND executable=${JSON.stringify(
        nodeExecutable,
      )} argv=${JSON.stringify([testFile])} cwd=${JSON.stringify(cwd)}`,
    );
    writeLine(
      stderr,
      `[node-test] SPAWN ERROR ${
        processResult.spawnError.stack || processResult.spawnError.message
      }`,
    );
    writeOutputTail(stderr, 'stdout', output.stdoutTail);
    writeOutputTail(stderr, 'stderr', output.stderrTail);
    return { ...baseResult, reason: 'spawn-error', exitCode };
  }

  if (processResult.timedOut) {
    const exitCode = TIMEOUT_EXIT_CODE;
    writeLine(
      stderr,
      `[node-test] TEST FAIL reason=timeout file=${JSON.stringify(
        testFile,
      )} pid=${child.pid} duration-ms=${durationMs} timeout-ms=${testTimeoutMs} termination-signal=${
        processResult.terminationSignal || processResult.signal || 'unknown'
      } exit=${exitCode}`,
    );
    writeOutputTail(stderr, 'stdout', output.stdoutTail);
    writeOutputTail(stderr, 'stderr', output.stderrTail);
    return { ...baseResult, reason: 'timeout', exitCode };
  }

  if (processResult.signal) {
    const signalNumber = os.constants.signals[processResult.signal];
    const exitCode = signalExitCode(processResult.signal);
    writeLine(
      stderr,
      `[node-test] TEST FAIL reason=signal file=${JSON.stringify(
        testFile,
      )} pid=${child.pid} duration-ms=${durationMs} signal=${
        processResult.signal
      } signal-number=${signalNumber ?? 'unknown'} exit=${exitCode}`,
    );
    writeOutputTail(stderr, 'stdout', output.stdoutTail);
    writeOutputTail(stderr, 'stderr', output.stderrTail);
    if (platform === 'darwin') {
      await writeCrashDiagnostics({
        pid: child.pid,
        signal: processResult.signal,
        startedAtMs,
        stderr,
        crashReportTimeoutMs,
        reportsDirectory: options.reportsDirectory,
        pollIntervalMs: options.pollIntervalMs,
        findCrashReportImpl: options.findCrashReport || findCrashReport,
      });
    }
    return { ...baseResult, reason: 'signal', exitCode };
  }

  const exitCode = processResult.code ?? 1;
  if (exitCode !== 0) {
    writeLine(
      stderr,
      `[node-test] TEST FAIL reason=exit-code file=${JSON.stringify(
        testFile,
      )} pid=${child.pid} duration-ms=${durationMs} exit=${exitCode}`,
    );
    writeOutputTail(stderr, 'stdout', output.stdoutTail);
    writeOutputTail(stderr, 'stderr', output.stderrTail);
    return { ...baseResult, reason: 'exit-code', exitCode };
  }

  writeLine(
    stdout,
    `[node-test] TEST PASS file=${JSON.stringify(
      testFile,
    )} pid=${child.pid} duration-ms=${durationMs} exit=0`,
  );
  return { ...baseResult, reason: 'success', exitCode: 0 };
}

async function runTestSuite(testFiles, options = {}) {
  const stdout = options.stdout || process.stdout;
  const stderr = options.stderr || process.stderr;
  const cwd = options.cwd || process.cwd();
  const platform = options.platform || process.platform;
  const arch = options.arch || process.arch;
  const testTimeoutMs = options.testTimeoutMs ?? DEFAULT_TEST_TIMEOUT_MS;
  const crashReportTimeoutMs =
    options.crashReportTimeoutMs ?? DEFAULT_CRASH_REPORT_TIMEOUT_MS;
  const outputTailBytes =
    options.outputTailBytes ?? DEFAULT_OUTPUT_TAIL_BYTES;
  const suiteStartedAtMs = Date.now();
  writeLine(
    stdout,
    `[node-test] SUITE START total=${testFiles.length} node=${process.version} platform=${platform} arch=${arch} cwd=${JSON.stringify(
      cwd,
    )} timeout-ms=${testTimeoutMs} crash-report-timeout-ms=${crashReportTimeoutMs} output-tail-bytes=${outputTailBytes}`,
  );

  for (let index = 0; index < testFiles.length; index += 1) {
    const result = await runNodeTest(testFiles[index], {
      ...options,
      stdout,
      stderr,
      cwd,
      platform,
      testTimeoutMs,
      crashReportTimeoutMs,
      outputTailBytes,
      index: index + 1,
      total: testFiles.length,
    });
    if (result.exitCode !== 0) {
      writeLine(
        stderr,
        `[node-test] SUITE FAIL passed=${index} failed=1 skipped=${
          testFiles.length - index - 1
        } duration-ms=${Date.now() - suiteStartedAtMs} exit=${result.exitCode}`,
      );
      return result.exitCode;
    }
  }

  writeLine(
    stdout,
    `[node-test] SUITE PASS passed=${testFiles.length} failed=0 skipped=0 duration-ms=${
      Date.now() - suiteStartedAtMs
    } exit=0`,
  );
  return 0;
}

async function main(args = process.argv.slice(2), options = {}) {
  const testFiles = parseArguments(args);
  const configuration = readConfiguration(options.env || process.env);
  return runTestSuite(testFiles, { ...options, ...configuration });
}

if (require.main === module) {
  main()
    .then((exitCode) => {
      process.exitCode = exitCode;
    })
    .catch((error) => {
      if (error instanceof ConfigurationError) {
        console.error(`[node-test] CONFIG ERROR ${error.message}`);
        process.exitCode = CONFIGURATION_EXIT_CODE;
        return;
      }
      console.error(`[node-test] RUNNER ERROR ${error.stack || error.message}`);
      process.exitCode = 1;
    });
}

module.exports = {
  CONFIGURATION_EXIT_CODE,
  ConfigurationError,
  DEFAULT_CRASH_REPORT_TIMEOUT_MS,
  DEFAULT_OUTPUT_TAIL_BYTES,
  DEFAULT_TEST_TIMEOUT_MS,
  OutputTail,
  TIMEOUT_EXIT_CODE,
  findCrashReport,
  formatCrashReport,
  main,
  parseArguments,
  parseIps,
  readConfiguration,
  readMatchingCrashReport,
  runNodeTest,
  runTestSuite,
  signalExitCode,
};
