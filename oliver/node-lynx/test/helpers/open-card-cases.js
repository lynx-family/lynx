const assert = require('assert');
const os = require('os');
const path = require('path');
const { pathToFileURL } = require('url');
const { mkdtemp, rm, writeFile } = require('fs/promises');

const DEFAULT_TEARDOWN_SETTLE_MS = 1000;

function delay(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

async function settleOpenCardTeardown() {
  const override = process.env.NODE_LYNX_OPEN_CARD_TEARDOWN_SETTLE_MS;
  const settleMs =
    override === undefined
      ? DEFAULT_TEARDOWN_SETTLE_MS
      : Number(override);

  if (!Number.isFinite(settleMs) || settleMs <= 0) {
    return;
  }
  await delay(settleMs);
}

async function disposeOpenCardManager(manager) {
  manager.dispose();
  await settleOpenCardTeardown();
}

function initTestLynxEnv(LynxEnv) {
  LynxEnv.init();
  LynxEnv.setAppInfo(['App', 'AppVersion'], ['LynxPlayground', '0.0.2']);
}

function createHeadlessManager(HeadlessOpenCardManager, options = {}) {
  return new HeadlessOpenCardManager({
    view: {
      width: 390,
      height: 844,
      devicePixelRatio: 2,
      timeoutMs: 30000,
      ...(options.view || {}),
    },
    ...options,
  });
}

function createWindowedManager(WindowedOpenCardManager, options = {}) {
  return new WindowedOpenCardManager({
    view: {
      width: 390,
      height: 844,
      devicePixelRatio: 2,
      timeoutMs: 30000,
      visible: false,
      ...(options.view || {}),
    },
    ...options,
  });
}

async function testSingleCardReplacesOldView(
  HeadlessOpenCardManager,
  fixtureUrl
) {
  const manager = createHeadlessManager(HeadlessOpenCardManager);
  try {
    const first = await manager.open(fixtureUrl);
    const second = await manager.open(fixtureUrl);
    const cards = manager.list();

    assert.strictEqual(first.state, 'closed');
    assert.strictEqual(second.state, 'loaded');
    assert.notStrictEqual(first.view, second.view);
    assert.strictEqual(cards.length, 1);
    assert.strictEqual(cards[0].id, second.id);
    assert.throws(
      () => first.view.updateGlobalProps({ text: 'closed' }),
      /destroyed/
    );
    await second.view.waitForFrame();
  } finally {
    await disposeOpenCardManager(manager);
  }
}

async function testReplacingLoadingCardIgnoresClosedResult(
  HeadlessOpenCardManager,
  fixtureUrl
) {
  const loaded = [];
  const errors = [];
  const manager = createHeadlessManager(HeadlessOpenCardManager, {
    onCardLoaded(card) {
      loaded.push(card.id);
    },
    onCardError(error, card) {
      errors.push({ error, card });
    },
  });
  try {
    const firstOpen = manager.open(fixtureUrl);
    const secondOpen = manager.open(fixtureUrl);
    const [first, second] = await Promise.all([firstOpen, secondOpen]);
    const cards = manager.list();

    assert.strictEqual(first.state, 'closed');
    assert.strictEqual(second.state, 'loaded');
    assert.notStrictEqual(first.view, second.view);
    assert.strictEqual(cards.length, 1);
    assert.strictEqual(cards[0].id, second.id);
    assert.deepStrictEqual(loaded, [second.id]);
    assert.strictEqual(errors.length, 0);
    assert.throws(
      () => first.view.updateGlobalProps({ text: 'closed' }),
      /destroyed/
    );
    await second.view.waitForFrame();
  } finally {
    await disposeOpenCardManager(manager);
  }
}

async function testInvalidTemplateRejectsAndCleansUp(HeadlessOpenCardManager) {
  const errors = [];
  const manager = createHeadlessManager(HeadlessOpenCardManager, {
    onCardError(error, card) {
      errors.push({ error, card });
    },
  });
  const tempDir = await mkdtemp(path.join(os.tmpdir(), 'node-lynx-open-card-'));
  try {
    const invalidTemplate = path.join(tempDir, 'invalid-template.lynx.bundle');
    await writeFile(invalidTemplate, Buffer.from('not a lynx template'));
    await assert.rejects(
      () => manager.open(pathToFileURL(invalidTemplate).href),
      /Lynx render error 10204|Decode error|unknown Decode Error/
    );

    assert.strictEqual(manager.list().length, 0);
    assert.strictEqual(errors.length, 1);
    assert.strictEqual(errors[0].card.state, 'failed');
    assert.ok(errors[0].error instanceof Error);
  } finally {
    await disposeOpenCardManager(manager);
    await rm(tempDir, { recursive: true, force: true });
  }
}

async function testWindowedSingleCardReplacesOldWindow(
  WindowedOpenCardManager,
  fixtureUrl
) {
  if (process.platform !== 'darwin' || !WindowedOpenCardManager) {
    return;
  }
  const manager = createWindowedManager(WindowedOpenCardManager);
  try {
    const first = await manager.open(fixtureUrl);
    const second = await manager.open(fixtureUrl);
    const cards = manager.list();

    assert.strictEqual(first.state, 'closed');
    assert.strictEqual(second.state, 'loaded');
    assert.notStrictEqual(first.view, second.view);
    assert.strictEqual(cards.length, 1);
    assert.strictEqual(cards[0].id, second.id);
    assert.throws(
      () => first.view.updateGlobalProps({ text: 'closed' }),
      /destroyed/
    );
    await second.view.waitForFrame();
  } finally {
    await disposeOpenCardManager(manager);
  }
}

async function runOpenCardManagerTests({
  HeadlessOpenCardManager,
  LynxEnv,
  WindowedOpenCardManager,
  fixtureUrl,
}) {
  initTestLynxEnv(LynxEnv);
  await testSingleCardReplacesOldView(HeadlessOpenCardManager, fixtureUrl);
  await testReplacingLoadingCardIgnoresClosedResult(
    HeadlessOpenCardManager,
    fixtureUrl
  );
  await testInvalidTemplateRejectsAndCleansUp(HeadlessOpenCardManager);
  await testWindowedSingleCardReplacesOldWindow(
    WindowedOpenCardManager,
    fixtureUrl
  );
}

module.exports = {
  runOpenCardManagerTests,
};
