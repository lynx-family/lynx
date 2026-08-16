const { pathToFileURL } = require('url');

require('./host-platform').configurePlaywrightHostPlatform();
const { chromium, webkit } = require('playwright');
const { normalizeBoxModel } = require('./geometry');

const ORACLES = {
  chromium: {
    browserType: chromium,
    launchOptions: {
      args: ['--enable-blink-features=CSSGridLanesLayout'],
    },
  },
  webkit: {
    browserType: webkit,
    launchOptions: {},
  },
};

async function renderOracleFixture(oracleName, fixture) {
  const oracle = ORACLES[oracleName];
  const browser = await oracle.browserType.launch({
    headless: true,
    ...oracle.launchOptions,
  });
  try {
    const page = await browser.newPage({
      viewport: {
        width: fixture.viewport.width,
        height: fixture.viewport.height,
      },
      deviceScaleFactor: fixture.viewport.devicePixelRatio,
    });
    await page.goto(pathToFileURL(fixture.oraclePath).href);
    await page.evaluate(() => document.fonts.ready);
    const supportsGridLanes = await page.evaluate(() =>
      CSS.supports('display', 'grid-lanes')
    );
    if (fixture.suite === 'grid-lanes' && !supportsGridLanes) {
      throw new Error(
        `${oracleName} ${browser.version()} does not support display: grid-lanes`
      );
    }

    const geometry =
      oracleName === 'chromium'
        ? await collectChromiumGeometry(page)
        : await collectWebKitGeometry(page);
    if (Object.keys(geometry).length === 0) {
      throw new Error(`${oracleName} document has no data-test-tag geometry`);
    }
    return {
      browserVersion: browser.version(),
      geometry,
      supportsGridLanes,
    };
  } finally {
    await browser.close();
  }
}

async function collectChromiumGeometry(page) {
  const client = await page.context().newCDPSession(page);
  await client.send('DOM.enable');
  const { root } = await client.send('DOM.getDocument', { depth: -1 });
  const geometry = {};
  await collectTaggedChromiumGeometry(client, root, geometry);
  return geometry;
}

async function collectTaggedChromiumGeometry(client, node, geometry) {
  const attributes = Array.isArray(node.attributes) ? node.attributes : [];
  for (let index = 0; index + 1 < attributes.length; index += 2) {
    if (attributes[index] === 'data-test-tag') {
      const { model } = await client.send('DOM.getBoxModel', {
        backendNodeId: node.backendNodeId,
      });
      geometry[attributes[index + 1]] = normalizeBoxModel(model);
      break;
    }
  }
  for (const child of node.children || []) {
    await collectTaggedChromiumGeometry(client, child, geometry);
  }
}

async function collectWebKitGeometry(page) {
  const rawGeometry = await page.evaluate(() => {
    const number = (value) => Number.parseFloat(value) || 0;
    const quad = (left, top, right, bottom) => [
      left,
      top,
      right,
      top,
      right,
      bottom,
      left,
      bottom,
    ];
    return Object.fromEntries(
      Array.from(document.querySelectorAll('[data-test-tag]')).map(
        (element) => {
          const rect = element.getBoundingClientRect();
          const style = getComputedStyle(element);
          const borderLeft = number(style.borderLeftWidth);
          const borderTop = number(style.borderTopWidth);
          const borderRight = number(style.borderRightWidth);
          const borderBottom = number(style.borderBottomWidth);
          const paddingLeft = number(style.paddingLeft);
          const paddingTop = number(style.paddingTop);
          const paddingRight = number(style.paddingRight);
          const paddingBottom = number(style.paddingBottom);
          const marginLeft = number(style.marginLeft);
          const marginTop = number(style.marginTop);
          const marginRight = number(style.marginRight);
          const marginBottom = number(style.marginBottom);
          const paddingBox = {
            left: rect.left + borderLeft,
            top: rect.top + borderTop,
            right: rect.right - borderRight,
            bottom: rect.bottom - borderBottom,
          };
          const contentBox = {
            left: paddingBox.left + paddingLeft,
            top: paddingBox.top + paddingTop,
            right: paddingBox.right - paddingRight,
            bottom: paddingBox.bottom - paddingBottom,
          };
          return [
            element.getAttribute('data-test-tag'),
            {
              width: rect.width,
              height: rect.height,
              content: quad(
                contentBox.left,
                contentBox.top,
                contentBox.right,
                contentBox.bottom
              ),
              padding: quad(
                paddingBox.left,
                paddingBox.top,
                paddingBox.right,
                paddingBox.bottom
              ),
              border: quad(rect.left, rect.top, rect.right, rect.bottom),
              margin: quad(
                rect.left - marginLeft,
                rect.top - marginTop,
                rect.right + marginRight,
                rect.bottom + marginBottom
              ),
            },
          ];
        }
      )
    );
  });
  return Object.fromEntries(
    Object.entries(rawGeometry).map(([tag, model]) => [
      tag,
      normalizeBoxModel(model),
    ])
  );
}

module.exports = {
  ORACLES: Object.keys(ORACLES),
  renderOracleFixture,
};
