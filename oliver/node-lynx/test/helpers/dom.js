const assert = require('assert');
const fs = require('fs/promises');
const { pathToFileURL } = require('url');

let cdpId = 0;

function initTestLynxEnv(LynxEnv) {
  LynxEnv.init();
  LynxEnv.setAppInfo(['App', 'AppVersion'], ['LynxPlayground', '0.0.2']);
}

async function invokeCDPFromSDK(view, method, params = {}) {
  const responseText = await view.invokeCDPFromSDK(
    JSON.stringify({
      id: ++cdpId,
      method,
      params,
    })
  );
  const response = JSON.parse(responseText);
  assert.ifError(response.error);
  return response.result;
}

function nodeLocalName(node) {
  return String(node.localName || node.nodeName || '').toLowerCase();
}

function attrsOf(node) {
  const attrs = {};
  const attributes = Array.isArray(node.attributes) ? node.attributes : [];
  for (let i = 0; i + 1 < attributes.length; i += 2) {
    attrs[attributes[i]] = attributes[i + 1];
  }
  return attrs;
}

function walk(node, visitor) {
  visitor(node);
  const children = Array.isArray(node.children) ? node.children : [];
  for (const child of children) {
    walk(child, visitor);
  }
}

function collect(root, predicate) {
  const matches = [];
  walk(root, (node) => {
    if (predicate(node)) {
      matches.push(node);
    }
  });
  return matches;
}

function findNode(root, predicate, message) {
  const match = collect(root, predicate)[0];
  assert(match, message);
  return match;
}

function rawTextValues(root) {
  return collect(root, (node) => nodeLocalName(node) === 'raw-text')
    .map((node) => attrsOf(node).text)
    .filter((text) => typeof text === 'string');
}

async function loadTemplateDocument({
  HeadlessLynxView,
  LynxEnv,
  templateUrl,
  templatePath,
  viewOptions = {},
  documentDepth = -1,
}) {
  initTestLynxEnv(LynxEnv);
  const view = new HeadlessLynxView({
    width: 390,
    height: 844,
    devicePixelRatio: 2,
    timeoutMs: 30000,
    ...viewOptions,
  });

  try {
    if (templateUrl) {
      await view.loadTemplateFromUrl(templateUrl);
    } else {
      const template = await fs.readFile(templatePath);
      await view.loadTemplate(template, {
        url: pathToFileURL(templatePath).href,
      });
    }
    await view.waitForFrame();
    await invokeCDPFromSDK(view, 'DOM.enable', {
      useCompression: false,
    });
    const document = await invokeCDPFromSDK(view, 'DOM.getDocument', {
      depth: documentDepth,
    });
    assert(document.root, 'expected DOM.getDocument to return a root node');
    return document.root;
  } finally {
    view.destroy();
  }
}

module.exports = {
  attrsOf,
  collect,
  findNode,
  initTestLynxEnv,
  invokeCDPFromSDK,
  loadTemplateDocument,
  nodeLocalName,
  rawTextValues,
  walk,
};
