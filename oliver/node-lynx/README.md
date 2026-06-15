# @lynx-js/node-lynx

Run Lynx bundles in Node.js for screenshots, visible macOS preview windows, and test automation.

```bash
npm i @lynx-js/node-lynx
```

The package loads the native addon from a matching optional platform package, such as `@lynx-js/node-lynx-darwin-arm64`, or from a local `build/` / `platform/` directory during development.

## Headless Screenshot

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
  fs.writeFileSync(
    './screenshot.png',
    await view.screenshot({ settleMs: 500 })
  );
} finally {
  view.destroy();
}
```

For local buffers, pass a URL so relative resource resolution has context:

```js
const fs = require('fs');
const path = require('path');
const { pathToFileURL } = require('url');
const { HeadlessLynxView } = require('@lynx-js/node-lynx');

const templatePath = path.resolve('./template.lynx.bundle');
await view.loadTemplate(fs.readFileSync(templatePath), {
  url: pathToFileURL(templatePath).toString(),
});
```

## Windowed Preview

`WindowedLynxView` is macOS-only and creates a visible AppKit window.

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

`click(x, y)` takes CSS pixel coordinates. `typeText(text)` commits UTF-8 text into the focused Lynx input. `pressKey(key)` supports `Backspace`, `Delete`, `Enter`, `ArrowLeft`, `ArrowRight`, `ArrowUp`, and `ArrowDown`.

## CLI

```bash
node-lynx render https://lynxjs.org/lynx-examples/gallery/dist/GalleryComplete.lynx.bundle --width 268 --height 469 --dpr 2 --output ./gallery.png --screenshot-delay 500
node-lynx preview https://lynxjs.org/lynx-examples/gallery/dist/GalleryComplete.lynx.bundle --width 268 --height 469 --dpr 2
```

Rules:

- `--template` is a local Lynx bundle path.
- `--url` is a remote `http://` or `https://` Lynx bundle URL.
- A positional input starting with `http://` or `https://` is treated as a remote URL.
- `--width` and `--height` are CSS pixel viewport values.
- `--dpr` defaults to `2`; the PNG pixel size is `width * dpr` by `height * dpr`.
- `--output` is created recursively if parent directories do not exist.
- `preview` opens a visible macOS window and exits after that window closes.
- Use `--timeout <ms>` as the maximum wait for bundle download, template load, and first-frame submission.
- Use `--screenshot-delay <ms>` to wait after the first frame and before capture; it defaults to `100` and can be `0`.
- Prefer passing a real template URL when the bundle uses URL-relative resources.

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

## Development

```bash
pnpm --filter @lynx-js/node-lynx exec tsc --noEmit
pnpm --filter @lynx-js/node-lynx run test
pnpm --filter @lynx-js/node-lynx run test:gallery
```

`waitForFrame()` means a frame was submitted; it does not guarantee that every image resource has decoded. For image-heavy screenshot checks, pass `screenshot({ settleMs })`, use `--screenshot-delay`, or wait for a runtime signal before capturing.
