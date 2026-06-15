const assert = require('assert');

const { HeadlessLynxView, LynxEnv } = require('../dist/index.js');
const {
  attrsOf,
  collect,
  findNode,
  loadTemplateDocument,
  nodeLocalName,
  rawTextValues,
} = require('./helpers/dom.js');
const {
  getHelloWorldTemplateUrl,
} = require('./helpers/remote-templates.js');

function assertHelloWorldDom(root) {
  findNode(
    root,
    (node) => nodeLocalName(node) === 'page',
    'expected hello-world document to contain a page node'
  );

  const elementNodes = collect(root, (node) => {
    const name = nodeLocalName(node);
    return name === 'view' || name === 'text' || name === 'raw-text';
  });
  assert(
    elementNodes.length > 0,
    'expected hello-world document to contain rendered element nodes'
  );

  const textValues = collect(root, (node) => nodeLocalName(node) === 'text')
    .map((node) => attrsOf(node).text)
    .filter((text) => typeof text === 'string');
  for (const expectedText of ['React', 'on Lynx', 'Tap the logo and have fun!']) {
    assert(
      textValues.includes(expectedText),
      `expected text ${JSON.stringify(expectedText)} in hello-world DOM`
    );
  }

  const rawTextFragments = rawTextValues(root)
    .map((text) => text.trim())
    .filter(Boolean);
  for (const expectedText of ['Edit', 'src/App.tsx', 'to see updates!']) {
    assert(
      rawTextFragments.includes(expectedText),
      `expected raw text ${JSON.stringify(expectedText)} in hello-world DOM`
    );
  }

  findNode(
    root,
    (node) => {
      const attrs = attrsOf(node);
      return nodeLocalName(node) === 'text' && attrs.class === 'Title';
    },
    'expected hello-world title text node'
  );
}

async function main() {
  const root = await loadTemplateDocument({
    HeadlessLynxView,
    LynxEnv,
    templateUrl: getHelloWorldTemplateUrl(),
  });
  assertHelloWorldDom(root);
  console.log('remote template DOM tree matches expected semantics.');
}

main().catch((error) => {
  console.error(error instanceof Error ? error.stack || error.message : String(error));
  process.exit(1);
});
