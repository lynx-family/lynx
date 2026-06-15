const assert = require('assert');
const fs = require('fs');
const os = require('os');
const path = require('path');
const zlib = require('zlib');

const { HeadlessLynxView, WindowedLynxView } = require('../dist/index.js');
const {
  getGalleryTemplateUrl,
} = require('./helpers/remote-templates.js');
const DEFAULT_OUTPUT_DIR = path.join(os.tmpdir(), 'node-lynx-gallery-validation');
const MIN_GALLERY_SAMPLED_COLORS = 1000;
const MIN_GALLERY_BRIGHT_SAMPLES = 50;
const PNG_SIGNATURE = Buffer.from([
  0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a,
]);

function delay(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

function readUInt32(buffer, offset) {
  return buffer.readUInt32BE(offset);
}

function decodePng(buffer) {
  assert(buffer.subarray(0, PNG_SIGNATURE.length).equals(PNG_SIGNATURE));

  let offset = PNG_SIGNATURE.length;
  let width = 0;
  let height = 0;
  const idat = [];

  while (offset < buffer.length) {
    const length = readUInt32(buffer, offset);
    const type = buffer.toString('ascii', offset + 4, offset + 8);
    const dataStart = offset + 8;
    const dataEnd = dataStart + length;
    const data = buffer.subarray(dataStart, dataEnd);
    offset = dataEnd + 4;

    if (type === 'IHDR') {
      width = readUInt32(data, 0);
      height = readUInt32(data, 4);
      assert.strictEqual(data[8], 8, 'expected 8-bit PNG');
      assert.strictEqual(data[9], 6, 'expected RGBA PNG');
    } else if (type === 'IDAT') {
      idat.push(data);
    } else if (type === 'IEND') {
      break;
    }
  }

  assert(width > 0 && height > 0, 'missing PNG dimensions');
  const inflated = zlib.inflateSync(Buffer.concat(idat));
  const stride = width * 4;
  const rgba = Buffer.alloc(width * height * 4);

  for (let y = 0; y < height; y++) {
    const rowOffset = y * (stride + 1);
    const filter = inflated[rowOffset];
    assert.strictEqual(filter, 0, 'only unfiltered PNG rows are supported');
    inflated.copy(rgba, y * stride, rowOffset + 1, rowOffset + 1 + stride);
  }

  return { width, height, rgba };
}

function smokeSummary(png) {
  const decoded = decodePng(png);
  const totalPixels = decoded.width * decoded.height;
  const sampleStep = Math.max(4, Math.floor(decoded.rgba.length / 4096 / 4) * 4);
  const colors = new Set();
  let nonTransparentPixels = 0;
  let brightPixels = 0;
  let darkPixels = 0;

  for (let index = 0; index < decoded.rgba.length; index += sampleStep) {
    const r = decoded.rgba[index];
    const g = decoded.rgba[index + 1];
    const b = decoded.rgba[index + 2];
    const a = decoded.rgba[index + 3];
    colors.add(`${r},${g},${b},${a}`);
    if (a > 0) {
      nonTransparentPixels++;
    }
    const gray = 0.2126 * r + 0.7152 * g + 0.0722 * b;
    if (gray > 120) {
      brightPixels++;
    }
    if (gray < 40) {
      darkPixels++;
    }
  }

  return {
    width: decoded.width,
    height: decoded.height,
    totalPixels,
    sampledColors: colors.size,
    nonTransparentSamples: nonTransparentPixels,
    brightSamples: brightPixels,
    darkSamples: darkPixels,
  };
}

function assertGalleryContent(mode, summary) {
  assert(
    summary.sampledColors >= MIN_GALLERY_SAMPLED_COLORS,
    `${mode} screenshot looks too flat`
  );
  assert(
    summary.darkSamples > 0,
    `${mode} screenshot lacks dark gallery background`
  );
  assert(
    summary.brightSamples >= MIN_GALLERY_BRIGHT_SAMPLES,
    `${mode} screenshot lacks bright furniture highlights`
  );
}

function hasGalleryContent(summary) {
  return (
    summary.sampledColors >= MIN_GALLERY_SAMPLED_COLORS &&
    summary.darkSamples > 0 &&
    summary.brightSamples >= MIN_GALLERY_BRIGHT_SAMPLES
  );
}

function comparePngs(actualPng, expectedPng) {
  const actual = decodePng(actualPng);
  const expected = decodePng(expectedPng);
  assert.strictEqual(actual.width, expected.width, 'width mismatch');
  assert.strictEqual(actual.height, expected.height, 'height mismatch');

  let differingPixels = 0;
  let maxChannelDelta = 0;
  const totalPixels = actual.width * actual.height;

  for (let index = 0; index < actual.rgba.length; index += 4) {
    let pixelDiffers = false;
    for (let channel = 0; channel < 4; channel++) {
      const delta = Math.abs(actual.rgba[index + channel] - expected.rgba[index + channel]);
      maxChannelDelta = Math.max(maxChannelDelta, delta);
      if (delta > 2) {
        pixelDiffers = true;
      }
    }
    if (pixelDiffers) {
      differingPixels++;
    }
  }

  return {
    width: actual.width,
    height: actual.height,
    totalPixels,
    differingPixels,
    differingPixelRatio: differingPixels / totalPixels,
    maxChannelDelta,
  };
}

async function captureWhenReady(view, minWaitMs, maxWaitMs) {
  const startedAt = Date.now();
  let attempts = 0;
  let lastPng;
  let lastSmoke;
  while (Date.now() - startedAt <= maxWaitMs || attempts === 0) {
    if (attempts > 0) {
      await delay(250);
    }
    const png = await view.screenshot();
    const smoke = smokeSummary(png);
    attempts++;
    lastPng = png;
    lastSmoke = smoke;
    if (hasGalleryContent(smoke) && Date.now() - startedAt >= minWaitMs) {
      break;
    }
  }
  return {
    png: lastPng,
    smoke: lastSmoke,
    readyElapsedMs: Date.now() - startedAt,
    attempts,
  };
}

async function renderHeadless(templateUrl, options, minWaitMs, maxWaitMs) {
  const view = new HeadlessLynxView(options);
  try {
    await view.loadTemplateFromUrl(templateUrl);
    return await captureWhenReady(view, minWaitMs, maxWaitMs);
  } finally {
    view.destroy();
  }
}

async function renderWindowed(templateUrl, options, minWaitMs, maxWaitMs) {
  if (process.platform !== 'darwin') {
    return undefined;
  }
  const view = new WindowedLynxView({
    ...options,
    title: 'node-lynx-gallery-validation',
    visible: true,
  });
  let closed = false;
  try {
    await view.loadTemplateFromUrl(templateUrl);
    const capture = await captureWhenReady(view, minWaitMs, maxWaitMs);
    view.close();
    await view.waitUntilClosed(10000);
    closed = true;
    return capture;
  } finally {
    if (!closed) {
      view.destroy();
    }
  }
}

async function main() {
  const outputDir = process.argv[2] || DEFAULT_OUTPUT_DIR;
  const templateUrl = getGalleryTemplateUrl();
  const minWaitMs = Number(process.env.NODE_LYNX_GALLERY_MIN_WAIT_MS || 2000);
  const maxWaitMs = Number(process.env.NODE_LYNX_GALLERY_SETTLE_MS || 3000);
  fs.mkdirSync(outputDir, { recursive: true });

  const options = {
    width: 268,
    height: 469,
    devicePixelRatio: 2,
    timeoutMs: 30000,
  };

  const headlessCapture = await renderHeadless(
    templateUrl,
    options,
    minWaitMs,
    maxWaitMs
  );
  const { png: headlessPng, smoke: headlessSummary } = headlessCapture;
  const headlessPath = path.join(outputDir, 'headless.png');
  fs.writeFileSync(headlessPath, headlessPng);
  assert.strictEqual(headlessSummary.width, 536);
  assert.strictEqual(headlessSummary.height, 938);
  assertGalleryContent('headless', headlessSummary);

  const summary = {
    url: templateUrl,
    outputDir,
    minWaitMs,
    maxWaitMs,
    headless: {
      path: headlessPath,
      readyElapsedMs: headlessCapture.readyElapsedMs,
      attempts: headlessCapture.attempts,
      smoke: headlessSummary,
    },
  };

  const windowedCapture = await renderWindowed(
    templateUrl,
    options,
    minWaitMs,
    maxWaitMs
  );
  if (windowedCapture) {
    const { png: windowedPng, smoke: windowedSmoke } = windowedCapture;
    const windowedPath = path.join(outputDir, 'windowed.png');
    fs.writeFileSync(windowedPath, windowedPng);
    assertGalleryContent('windowed', windowedSmoke);
    const comparison = comparePngs(windowedPng, headlessPng);
    summary.windowed = {
      path: windowedPath,
      readyElapsedMs: windowedCapture.readyElapsedMs,
      attempts: windowedCapture.attempts,
      smoke: windowedSmoke,
      comparison,
    };
  } else {
    summary.windowed = { skipped: 'WindowedLynxView is macOS-only.' };
  }

  const summaryPath = path.join(outputDir, 'summary.json');
  fs.writeFileSync(summaryPath, `${JSON.stringify(summary, null, 2)}\n`);

  console.log(`headless=${headlessPath}`);
  if (summary.windowed.path) {
    console.log(`windowed=${summary.windowed.path}`);
  }
  console.log(`summary=${summaryPath}`);
  console.log(JSON.stringify(summary));
}

main().catch((error) => {
  console.error(error instanceof Error ? error.stack || error.message : String(error));
  process.exit(1);
});
