// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

const assert = require('assert');
const { spawnSync } = require('child_process');
const fs = require('fs/promises');
const os = require('os');
const path = require('path');

const COMPLETE = 'Windowed lifecycle child completed';

async function runChild(packageRoot, templatePath, templateUrl, iterations) {
  const { WindowedLynxView } = require(path.join(packageRoot, 'dist/index.js'));
  const template = await fs.readFile(templatePath);
  for (let index = 0; index < iterations; index++) {
    const view = new WindowedLynxView({
      width: 390,
      height: 844,
      // The screen may have a different scale, causing another frame after load.
      devicePixelRatio: 1,
      visible: true,
      title: 'node-lynx-windowed-lifecycle',
    });
    try {
      if (index % 3 === 2) {
        const frame = assert.rejects(view.waitForFrame(), /view is destroyed/);
        const closed = assert.rejects(view.waitUntilClosed(), /view is destroyed/);
        view.destroy();
        await Promise.all([frame, closed]);
      } else {
        await view.loadTemplate(template, { url: templateUrl });
        await view.waitForFrame();
        // Input simulation needs a desktop input service, which some macOS
        // workers cannot access. Keep it opt-in, separate from teardown stress.
        if (process.env.NODE_LYNX_WINDOWED_INPUT_TESTS === '1') {
          view.click(20, 70);
          view.typeText('lifecycle');
          view.pressKey('Backspace');
        }
        // Keep rendering in flight. Do not settle the last frame before teardown.
        const frame = view.flushFrame(0);
        if (index % 3 === 0) {
          const closed = view.waitUntilClosed(5000);
          view.close();
          view.close();
          await closed;
        }
        view.destroy();
        await frame;
      }
      assert.doesNotThrow(() => view.destroy());
      assert.throws(() => view.show(), /WindowedLynxView has been destroyed/);
    } finally {
      view.destroy();
    }
    // Exercise queued AppKit work and finalizers after the native host is gone.
    await new Promise((resolve) => setImmediate(resolve));
    global.gc();
  }
  console.log(`${COMPLETE}: ${iterations}`);
}

async function runWindowedLifecycleSmoke({ packageRoot, templateUrl }) {
  if (process.platform !== 'darwin') {
    console.log('windowed lifecycle skipped: macOS-only.');
    return;
  }
  const iterations = Number(process.env.NODE_LYNX_WINDOWED_LIFECYCLE_ITERATIONS || 12);
  assert(Number.isSafeInteger(iterations) && iterations >= 3, 'expected at least 3 iterations');
  const { fetchBuffer } = require(path.join(packageRoot, 'dist/headless-lynx-view.js'));
  const tempDir = await fs.mkdtemp(path.join(os.tmpdir(), 'node-lynx-windowed-lifecycle-'));
  try {
    const templatePath = path.join(tempDir, 'template.bundle');
    await fs.writeFile(templatePath, await fetchBuffer(templateUrl));
    // Separate processes catch native crashes and leaked handles as test failures.
    for (let run = 0; run < 3; run++) {
      const result = spawnSync(process.execPath, [
        '--expose-gc', __filename, '--child', packageRoot, templatePath, templateUrl,
        String(iterations),
      ], { encoding: 'utf8', timeout: 60000, maxBuffer: 16 * 1024 * 1024 });
      assert.strictEqual(result.status, 0,
        `windowed lifecycle run ${run + 1}: signal=${result.signal}, error=${result.error}\n` +
        `${result.stdout}\n${result.stderr}`);
      assert(result.stdout.includes(`${COMPLETE}: ${iterations}`), 'child exited before completing');
    }
    console.log(`Windowed lifecycle passed: ${3 * iterations} windows across 3 processes; ` +
      `input simulation=${process.env.NODE_LYNX_WINDOWED_INPUT_TESTS === '1'}.`);
  } finally {
    await fs.rm(tempDir, { recursive: true, force: true });
  }
}

if (require.main === module) {
  assert.strictEqual(process.argv[2], '--child');
  runChild(process.argv[3], process.argv[4], process.argv[5], Number(process.argv[6]))
    .catch((error) => { console.error(error); process.exitCode = 1; });
}

module.exports = { runWindowedLifecycleSmoke };
