const fs = require('fs/promises');
const path = require('path');
const zlib = require('zlib');

const { HeadlessLynxView, LynxEnv } = require('@lynx-js/node-lynx');
const {
  attrsOf,
  initTestLynxEnv,
  invokeCDPFromSDK,
  walk,
} = require('../../../oliver/node-lynx/test/helpers/dom');
const { normalizeBoxModel } = require('./geometry');

let initialized = false;

function ensureEnvironment() {
  if (!initialized) {
    initTestLynxEnv(LynxEnv);
    initialized = true;
  }
}

function decodeDocument(result) {
  if (!result.compress) {
    return result.root;
  }
  const compressed = Buffer.from(result.root, 'base64');
  return JSON.parse(zlib.inflateSync(compressed).toString('utf8'));
}

async function renderNativeFixture(fixture) {
  ensureEnvironment();
  const { width, height, devicePixelRatio } = fixture.viewport;
  const errors = [];
  const view = new HeadlessLynxView({
    width,
    height,
    devicePixelRatio,
    timeoutMs: 30000,
    onErrorOccurred(_level, code, message) {
      errors.push(`${code}: ${message}`);
    },
  });

  try {
    const template = await fs.readFile(fixture.bundlePath);
    await view.loadTemplate(template, {
      url: new URL(`file://${path.resolve(fixture.bundlePath)}`).href,
    });
    await view.waitForFrame();
    await invokeCDPFromSDK(view, 'DOM.enable', { useCompression: false });
    const result = await invokeCDPFromSDK(view, 'DOM.getDocumentWithBoxModel');
    const root = decodeDocument(result);
    const geometry = {};
    walk(root, (node) => {
      const tag = attrsOf(node)['lynx-test-tag'];
      if (!tag) {
        return;
      }
      if (!node.box_model) {
        throw new Error(`native node #${tag} has no box model`);
      }
      geometry[tag] = normalizeBoxModel(node.box_model);
    });
    if (Object.keys(geometry).length === 0) {
      throw new Error('native document has no lynx-test-tag geometry');
    }
    if (errors.length > 0) {
      throw new Error(`native render reported errors:\n${errors.join('\n')}`);
    }
    return { geometry, errors };
  } finally {
    view.destroy();
  }
}

module.exports = { renderNativeFixture };
