const assert = require('assert');
const fs = require('fs');
const os = require('os');
const path = require('path');
const { pathToFileURL } = require('url');
const zlib = require('zlib');
const { HeadlessLynxView, WindowedLynxView } = require('../dist/index.js');
const {
  getHelloWorldTemplateUrl,
} = require('./helpers/remote-templates.js');
const PNG_SIGNATURE = Buffer.from([
  0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a,
]);

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

function comparePngs(actualPng, expectedPng) {
  const actual = decodePng(actualPng);
  const expected = decodePng(expectedPng);
  assert.strictEqual(actual.width, expected.width, 'width mismatch');
  assert.strictEqual(actual.height, expected.height, 'height mismatch');

  let differingPixels = 0;
  let maxChannelDelta = 0;
  let totalChannelDelta = 0;
  const totalPixels = actual.width * actual.height;

  for (let index = 0; index < actual.rgba.length; index += 4) {
    let pixelDiffers = false;
    for (let channel = 0; channel < 4; channel++) {
      const delta = Math.abs(actual.rgba[index + channel] - expected.rgba[index + channel]);
      totalChannelDelta += delta;
      if (delta > maxChannelDelta) {
        maxChannelDelta = delta;
      }
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
    meanChannelDelta: totalChannelDelta / (totalPixels * 4),
  };
}

function normalizeTemplateInput(input) {
  if (!input) {
    return getHelloWorldTemplateUrl();
  }
  if (
    input.startsWith('http://') ||
    input.startsWith('https://') ||
    input.startsWith('file://')
  ) {
    return input;
  }
  return pathToFileURL(path.resolve(input)).href;
}

async function renderHeadless(templateUrl, options) {
  const view = new HeadlessLynxView(options);
  try {
    await view.loadTemplateFromUrl(templateUrl);
    return await view.screenshot();
  } finally {
    view.destroy();
  }
}

async function renderWindowed(templateUrl, options) {
  const view = new WindowedLynxView({
    ...options,
    title: 'node-lynx-windowed-screenshot-compare',
    visible: true,
  });
  let viewClosed = false;
  try {
    await view.loadTemplateFromUrl(templateUrl);
    const png = await view.screenshot();
    view.close();
    await view.waitUntilClosed(10000);
    viewClosed = true;
    return png;
  } finally {
    if (!viewClosed) {
      view.destroy();
    }
  }
}

async function main() {
  if (process.platform !== 'darwin') {
    console.log('windowed screenshot compare skipped: WindowedLynxView is macOS-only.');
    return;
  }

  const templateUrl = normalizeTemplateInput(process.argv[2]);
  const outputDir =
    process.argv[3] ||
    path.join(os.tmpdir(), 'node-lynx-windowed-screenshot-compare');
  fs.mkdirSync(outputDir, { recursive: true });

  const options = {
    width: 390,
    height: 844,
    devicePixelRatio: 2,
    timeoutMs: 30000,
  };
  const headlessPng = await renderHeadless(templateUrl, options);
  const windowedPng = await renderWindowed(templateUrl, options);
  const summary = comparePngs(windowedPng, headlessPng);

  const headlessPath = path.join(outputDir, 'headless.png');
  const windowedPath = path.join(outputDir, 'windowed.png');
  const summaryPath = path.join(outputDir, 'summary.json');
  fs.writeFileSync(headlessPath, headlessPng);
  fs.writeFileSync(windowedPath, windowedPng);
  fs.writeFileSync(summaryPath, `${JSON.stringify(summary, null, 2)}\n`);

  console.log(`headless=${headlessPath}`);
  console.log(`windowed=${windowedPath}`);
  console.log(`summary=${summaryPath}`);
  console.log(JSON.stringify(summary));

  assert(
    summary.differingPixelRatio <= 0.001,
    `windowed screenshot differs from headless screenshot: ${summary.differingPixelRatio}`
  );
}

main()
  .then(() => {
    process.exitCode = 0;
  })
  .catch((error) => {
    console.error(error instanceof Error ? error.stack || error.message : String(error));
    process.exit(1);
  });
