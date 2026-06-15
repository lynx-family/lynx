# @lynx-js/node-lynx Usage

Use this skill when the task involves running Lynx bundles through `@lynx-js/node-lynx`, capturing screenshots, opening macOS preview windows, using DebugRouter OpenCard, or validating changes to the package under `lynx/oliver/node-lynx`.

## Package Boundary

- Package root: `lynx/oliver/node-lynx`.
- Public package name: `@lynx-js/node-lynx`.
- Native addons are resolved from a matching optional platform package, or from local `platform/<platform>-<arch>/node_lynx.node` / `build/<platform>/.../node_lynx.node` during development.
- Treat files under `lynx/` as open-source-facing. Keep internal-only notes out of this package.

## CLI

Render a headless screenshot:

```bash
node-lynx render https://lynxjs.org/lynx-examples/gallery/dist/GalleryComplete.lynx.bundle \
  --width 268 \
  --height 469 \
  --dpr 2 \
  --output ./gallery.png \
  --screenshot-delay 500
```

Open a visible macOS preview window:

```bash
node-lynx preview https://lynxjs.org/lynx-examples/gallery/dist/GalleryComplete.lynx.bundle \
  --width 268 \
  --height 469 \
  --dpr 2
```

Rules:

- `--template <path>` loads a local Lynx bundle.
- `--url <url>` loads a remote `http://` or `https://` bundle.
- A positional input starting with `http://` or `https://` is treated as a remote URL.
- `--width` and `--height` are CSS pixel viewport values.
- `--dpr` controls output scale; PNG size is `width * dpr` by `height * dpr`.
- `--timeout <ms>` is a maximum wait for bundle download, template load, CDP calls, and first-frame submission. It is not a fixed screenshot delay.
- `--screenshot-delay <ms>` waits after first-frame submission and before capture. It defaults to `100`; use `0` to skip the extra settle wait.
- Prefer a real template URL when the bundle uses URL-relative resources.

## Headless API

```js
const fs = require('fs');
const { HeadlessLynxView } = require('@lynx-js/node-lynx');

const view = new HeadlessLynxView({
  width: 268,
  height: 469,
  devicePixelRatio: 2,
  timeoutMs: 30000,
});

try {
  await view.loadTemplateFromUrl(
    'https://lynxjs.org/lynx-examples/gallery/dist/GalleryComplete.lynx.bundle'
  );
  const png = await view.screenshot({ settleMs: 500 });
  fs.writeFileSync('./screenshot.png', png);
} finally {
  view.destroy();
}
```

For a local bundle buffer, pass a URL so relative resource resolution has context:

```js
const fs = require('fs');
const path = require('path');
const { pathToFileURL } = require('url');
const { HeadlessLynxView } = require('@lynx-js/node-lynx');

const templatePath = path.resolve('./template.lynx.bundle');
const view = new HeadlessLynxView();

try {
  await view.loadTemplate(fs.readFileSync(templatePath), {
    url: pathToFileURL(templatePath).toString(),
  });
  fs.writeFileSync('./screenshot.png', await view.screenshot());
} finally {
  view.destroy();
}
```

## Windowed API

`WindowedLynxView` is macOS-only and creates an AppKit window.

```js
const fs = require('fs');
const { WindowedLynxView } = require('@lynx-js/node-lynx');

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
  fs.writeFileSync('./windowed.png', await view.screenshot({ settleMs: 500 }));
  await view.waitUntilClosed();
} finally {
  view.destroy();
}
```

Useful interaction APIs:

- `click(x, y)` uses CSS pixel coordinates.
- `typeText(text)` commits UTF-8 text into the focused Lynx input.
- `pressKey(key)` supports `Backspace`, `Delete`, `Enter`, `ArrowLeft`, `ArrowRight`, `ArrowUp`, and `ArrowDown`.

## Screenshot Timing

- `waitForFrame()` means a frame was submitted; it does not guarantee every image resource has decoded or repainted.
- `screenshot({ settleMs })` and CLI `--screenshot-delay` add a simple post-frame wait before capture.
- For image-heavy or network-heavy pages, a fixed delay is a pragmatic fallback, but a page-level ready signal is more reliable when available.
- If a screenshot still shows placeholders after longer delays, investigate image decode, repaint, and capture timing rather than assuming the generic resource fetcher is missing.

## DebugRouter OpenCard

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
  view: { width: 268, height: 469, devicePixelRatio: 2 },
});

openCards.install();
```

CLI behavior:

- `node-lynx render` or bare `node-lynx` with no initial template waits for DebugRouter OpenCard when debug-router is enabled.
- `node-lynx preview` with no initial template waits for DebugRouter OpenCard and does not create an initial window.
- `--no-debug-router` requires an initial template input.

## Validation

From `lynx/oliver/node-lynx`:

```bash
npx tsc --noEmit
npm run test
npm run test:gallery
```

Native rebuilds are heavier. Use them when native files or build graph changes are involved:

```bash
npm run build:darwin
```
