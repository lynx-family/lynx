const assert = require('assert');
const fs = require('fs');
const os = require('os');
const path = require('path');

const { HeadlessLynxView } = require('../dist/index.js');
const {
  getHelloWorldTemplateUrl,
} = require('./helpers/remote-templates.js');
const PNG_SIGNATURE = Buffer.from([
  0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a,
]);

function readPngSize(png) {
  assert(
    png.subarray(0, PNG_SIGNATURE.length).equals(PNG_SIGNATURE),
    'expected a PNG screenshot'
  );
  return {
    width: png.readUInt32BE(16),
    height: png.readUInt32BE(20),
  };
}

async function main() {
  const templateUrl = getHelloWorldTemplateUrl();
  const outputPath =
    process.env.NODE_LYNX_OUTPUT_PATH ||
    path.join(os.tmpdir(), 'node-lynx-remote-template-smoke.png');
  const view = new HeadlessLynxView({
    width: 390,
    height: 844,
    devicePixelRatio: 2,
    timeoutMs: Number(process.env.NODE_LYNX_TIMEOUT_MS || 30000),
  });

  try {
    await view.loadTemplateFromUrl(templateUrl);
    const png = await view.screenshot({
      settleMs: Number(process.env.NODE_LYNX_SETTLE_MS || 200),
    });
    const size = readPngSize(png);
    assert.strictEqual(size.width, 780);
    assert.strictEqual(size.height, 1688);
    fs.mkdirSync(path.dirname(outputPath), { recursive: true });
    fs.writeFileSync(outputPath, png);
    console.log(`remote template smoke rendered ${templateUrl} to ${outputPath}`);
  } finally {
    view.destroy();
  }
}

main().catch((error) => {
  console.error(error instanceof Error ? error.stack || error.message : String(error));
  process.exit(1);
});
