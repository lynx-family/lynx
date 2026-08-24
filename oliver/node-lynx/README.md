# @lynx-js/node-lynx

Run Lynx bundles in Node.js for screenshots, visible macOS preview windows,
DebugRouter OpenCard sessions, and test automation.

```bash
npm i @lynx-js/node-lynx
```

The package loads the native addon from a matching optional platform package,
such as `@lynx-js/node-lynx-darwin-arm64`, or from a local `build/` /
`platform/` directory during development.

## CLI

Use the CLI for one-off screenshots, quick template validation, and preview
sessions.

```bash
node-lynx --help
```

Capture a headless screenshot and exit:

```bash
node-lynx render \
  "https://lynxjs.org/lynx-examples/gallery/dist/GalleryComplete.lynx.bundle" \
  --width 268 \
  --height 469 \
  --dpr 2 \
  --output ./gallery.png \
  --timeout 30000 \
  --screenshot-delay 500 \
  --no-debug-router
```

Open a visible macOS preview window:

```bash
node-lynx preview \
  "https://lynxjs.org/lynx-examples/gallery/dist/GalleryComplete.lynx.bundle" \
  --width 268 \
  --height 469 \
  --dpr 2
```

Wait for DebugRouter OpenCard without an initial template:

```bash
node-lynx render
node-lynx preview
```

Rules:

- `render` captures a headless screenshot.
- `preview` is macOS-only and opens a visible AppKit window.
- `--template <path>` loads a local Lynx bundle.
- `--url <url>` loads a remote `http://` or `https://` Lynx bundle URL.
- A positional input starting with `http://` or `https://` is treated as a
  remote URL.
- `--width` and `--height` are CSS pixel viewport values. They default to `390`
  and `844`.
- `--dpr` or `--device-pixel-ratio` defaults to `2`; the PNG pixel size is
  `width * dpr` by `height * dpr`.
- `--output` defaults to `screenshot.png` and parent directories are created
  recursively.
- `--timeout <ms>` is the maximum wait for bundle download, template load, CDP
  calls, and first-frame submission. It defaults to `10000`.
- `--screenshot-delay <ms>` waits after first-frame submission and before
  capture. It defaults to `100` and can be `0`.
- DebugRouter is enabled by default. Use `--no-debug-router` for one-shot
  screenshot commands that should exit immediately after writing the PNG.
- Without `--no-debug-router`, `render` with an initial template writes the PNG
  and then waits for `SIGINT` or `SIGTERM` so DebugRouter stays available.
- Without an initial template, `render` and `preview` wait for DebugRouter
  OpenCard. `--no-debug-router` requires an initial template input.
- Use `--debug-router-schema <schema>` to connect DebugRouter with an explicit
  schema.
- Use `--log-level error` to hide verbose, debug, info, and warning Lynx logs,
  or `--log-level silent` to hide all Lynx logger output. When omitted, Lynx
  keeps its default level.
- Prefer passing a real template URL when the bundle uses URL-relative
  resources.

## Headless API

Use `HeadlessLynxView` for programmatic screenshots, repeated captures, global
props, CDP inspection, and automation.

```js
const fs = require('fs/promises');
const { HeadlessLynxView } = require('@lynx-js/node-lynx');

async function main() {
  const view = new HeadlessLynxView({
    width: 268,
    height: 469,
    devicePixelRatio: 2,
    timeoutMs: 30000,
  });

  try {
    await view.loadTemplateFromUrl(
      'https://lynxjs.org/lynx-examples/gallery/dist/GalleryComplete.lynx.bundle',
      {
        initialData: { source: 'node-lynx' },
        globalProps: { theme: 'light' },
      }
    );
    await fs.writeFile('./gallery.png', await view.screenshot({ settleMs: 500 }));
  } finally {
    view.destroy();
  }
}

main().catch((error) => {
  console.error(error);
  process.exit(1);
});
```

For local buffers, pass a URL so relative resource resolution has context:

```js
const fs = require('fs/promises');
const path = require('path');
const { pathToFileURL } = require('url');
const { HeadlessLynxView } = require('@lynx-js/node-lynx');

async function main() {
  const templatePath = path.resolve('./dist/main/template.js');
  const view = new HeadlessLynxView({ width: 390, height: 844 });

  try {
    await view.loadTemplate(await fs.readFile(templatePath), {
      url: pathToFileURL(templatePath).toString(),
    });
    await fs.writeFile('./local.png', await view.screenshot({ settleMs: 500 }));
  } finally {
    view.destroy();
  }
}

main().catch((error) => {
  console.error(error);
  process.exit(1);
});
```

Useful APIs:

- `loadTemplateFromUrl(url, options)` downloads and loads a remote template.
- `loadTemplate(buffer, options)` loads a local template buffer.
- `updateData(data, options)` updates template data after load.
- `updateGlobalProps(globalProps)` updates global props after load.
- `invokeCDPFromSDK(cdpMessage)` invokes a CDP method with a JSON string.
- `waitForFrame()` waits until a frame has been submitted.
- `screenshot({ settleMs })` returns a PNG buffer.
- `destroy()` releases the native view. Always call it in a `finally` block.

Lynx logging is process-wide. Set the minimum level before creating a view:

```js
const { LynxEnv } = require('@lynx-js/node-lynx');

LynxEnv.setLogLevel('error');
```

## Windowed Preview

`WindowedLynxView` is macOS-only and creates a visible AppKit window.

```js
const fs = require('fs/promises');
const { WindowedLynxView } = require('@lynx-js/node-lynx');

async function main() {
  const view = new WindowedLynxView({
    width: 268,
    height: 469,
    devicePixelRatio: 2,
    title: 'Node Lynx Preview',
  });

  try {
    await view.loadTemplateFromUrl(
      'https://lynxjs.org/lynx-examples/gallery/dist/GalleryComplete.lynx.bundle'
    );
    await view.waitForFrame();
    await fs.writeFile('./windowed.png', await view.screenshot({ settleMs: 500 }));
    await view.waitUntilClosed();
  } finally {
    view.destroy();
  }
}

main().catch((error) => {
  console.error(error);
  process.exit(1);
});
```

`click(x, y)` takes CSS pixel coordinates. `typeText(text)` commits UTF-8 text
into the focused Lynx input. `pressKey(key)` supports `Backspace`, `Delete`,
`Enter`, `ArrowLeft`, `ArrowRight`, `ArrowUp`, and `ArrowDown`.

## DebugRouter OpenCard

Use OpenCard managers when the page should be opened by DebugRouter instead of
an initial CLI/API template load.

```js
const {
  HeadlessOpenCardManager,
  WindowedOpenCardManager,
  LynxEnv,
} = require('@lynx-js/node-lynx');

LynxEnv.init();
LynxEnv.setAppInfo(['App', 'AppVersion'], ['LynxPlayground', '0.0.2']);

const OpenCardManager =
  process.platform === 'darwin' ? WindowedOpenCardManager : HeadlessOpenCardManager;

const openCards = new OpenCardManager({
  view: { width: 268, height: 469, devicePixelRatio: 2, timeoutMs: 30000 },
  onCardLoaded(card) {
    console.log(`opened ${card.url}`);
  },
  onCardError(error, card) {
    console.error(`failed ${card.url}: ${error.message}`);
  },
});

openCards.install();

process.once('SIGINT', () => {
  openCards.dispose();
  process.exit(0);
});
```

`install()` registers a global OpenCard callback. Call `dispose()` when the
session ends so the callback and any current card are cleaned up.

## Screenshot Timing

`waitForFrame()` means a frame was submitted; it does not guarantee every image
resource has decoded or repainted. For image-heavy screenshot checks, pass
`screenshot({ settleMs })`, use CLI `--screenshot-delay`, or wait for a
page-level ready signal before capturing.

## Development

From the repository root:

```bash
pnpm --filter @lynx-js/node-lynx exec tsc --noEmit
pnpm --filter @lynx-js/node-lynx run test
pnpm --filter @lynx-js/node-lynx run test:gallery
```
