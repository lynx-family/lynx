---
name: node-lynx
description: Use this skill when a task needs to run Lynx bundles with the node-lynx CLI, capture screenshots, open a macOS preview window, use DebugRouter OpenCard, inspect rendered content through CDP, or integrate node-lynx through JavaScript or TypeScript APIs.
---

# Node Lynx

Use node-lynx as a local Lynx runtime for screenshots, preview windows, smoke
tests, DebugRouter OpenCard sessions, and automation that needs to inspect or
interact with a rendered Lynx page from Node.js.

Default sample template URL:

```text
https://lynxjs.org/lynx-examples/gallery/dist/GalleryComplete.lynx.bundle
```

## CLI

Prefer the CLI for one-off screenshots, quick template validation, and simple
preview sessions.

Show available options:

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

Capture a startup trace while rendering:

```bash
node-lynx render ./bundle.lynx.bundle \
  --trace ./startup.pftrace \
  --trace-duration 10 \
  --no-debug-router
```

Render a local bundle:

```bash
node-lynx render \
  --template ./dist/main/template.js \
  --width 390 \
  --height 844 \
  --output ./node-lynx-local.png \
  --no-debug-router
```

Open a visible preview window on macOS:

```bash
node-lynx preview \
  "https://lynxjs.org/lynx-examples/gallery/dist/GalleryComplete.lynx.bundle" \
  --width 268 \
  --height 469 \
  --dpr 2 \
  --title "Node Lynx Preview"
```

Start a DebugRouter-backed session without an initial template:

```bash
node-lynx render
```

CLI rules:

- Use `render` for headless screenshots and automation.
- Use `preview` only on macOS; it creates an AppKit window.
- Use `--template <path>` for local bundles and `--url <url>` for remote
  bundles. A positional `http://` or `https://` input is treated as a remote
  bundle URL.
- Use `--width` and `--height` as CSS pixel viewport values.
- Use `--dpr` or `--device-pixel-ratio` to control output scale; the PNG pixel
  size is `width * dpr` by `height * dpr`.
- Use `--timeout <ms>` for bundle download, template load, CDP calls, and first
  frame submission timeouts.
- Use `--screenshot-delay <ms>` to wait after the first frame before capture.
  This defaults to `100`; use a larger delay for image-heavy or network-heavy
  pages.
- Use `--trace <path>` to capture a Lynx trace to a file. Parent directories
  are created recursively and an existing file is overwritten.
- Use `--trace-duration <seconds>` to control trace capture duration. It
  defaults to `5` and requires `--trace`.
- For `render --no-debug-router`, the process exits after both the screenshot
  is written and trace capture finishes.
- Pass `--no-debug-router` for CI or one-shot screenshot commands that should
  exit immediately after writing the PNG.
- Omit `--no-debug-router` when you need DebugRouter/OpenCard. Without an
  initial template, `render` waits for DebugRouter OpenCard. `preview` also
  waits for OpenCard without creating an initial window.

## TypeScript API

Prefer the API when the task needs programmatic setup, repeated captures,
global props, CDP calls, input simulation, or custom lifecycle handling.

Import from the node-lynx package declared by the target project. For public npm
usage this is typically:

```ts
import { HeadlessLynxView } from '@lynx-js/node-lynx';
```

Render a remote template to PNG:

```ts
import { writeFile } from 'node:fs/promises';
import { HeadlessLynxView } from '@lynx-js/node-lynx';

const templateUrl =
  'https://lynxjs.org/lynx-examples/gallery/dist/GalleryComplete.lynx.bundle';

const view = new HeadlessLynxView({
  width: 268,
  height: 469,
  devicePixelRatio: 2,
  timeoutMs: 30000,
});

try {
  await view.loadTemplateFromUrl(templateUrl, {
    initialData: { source: 'node-lynx' },
    globalProps: { theme: 'light' },
  });
  const png = await view.screenshot({ settleMs: 500 });
  await writeFile('./gallery.png', png);
} finally {
  view.destroy();
}
```

Render a local bundle buffer. Always pass a file URL so relative resources have
the right base URL:

```ts
import { readFile, writeFile } from 'node:fs/promises';
import { resolve } from 'node:path';
import { pathToFileURL } from 'node:url';
import { HeadlessLynxView } from '@lynx-js/node-lynx';

const templatePath = resolve('./dist/main/template.js');
const view = new HeadlessLynxView({ width: 390, height: 844 });

try {
  await view.loadTemplate(await readFile(templatePath), {
    url: pathToFileURL(templatePath).href,
  });
  await writeFile('./node-lynx-local.png', await view.screenshot());
} finally {
  view.destroy();
}
```

Inspect the DOM through CDP:

```ts
const responseText = await view.invokeCDPFromSDK(
  JSON.stringify({
    id: 1,
    method: 'DOM.getDocument',
    params: { depth: -1, pierce: true },
  })
);
const response = JSON.parse(responseText);
```

Update runtime data after load:

```ts
view.updateData({ selected: true });
view.updateGlobalProps({ locale: 'en-US' });
await view.waitForFrame();
```

## Windowed API

Use `WindowedLynxView` only on macOS when a visible app window is required.

```ts
import { WindowedLynxView } from '@lynx-js/node-lynx';

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
  view.click(20, 70);
  view.typeText('hello from node-lynx');
  view.pressKey('Enter');
  await view.waitUntilClosed();
} finally {
  view.destroy();
}
```

Interaction APIs use CSS pixel coordinates. Supported `pressKey` values are
`Backspace`, `Delete`, `Enter`, `ArrowLeft`, `ArrowRight`, `ArrowUp`, and
`ArrowDown`.

## DebugRouter OpenCard

Use OpenCard managers when the page should be opened by DebugRouter instead of
an initial CLI/API template load.

```ts
import {
  HeadlessOpenCardManager,
  LynxEnv,
  WindowedOpenCardManager,
} from '@lynx-js/node-lynx';

LynxEnv.init();
LynxEnv.setAppInfo(['App', 'AppVersion'], ['NodeLynxSkill', '1.0.0']);

const Manager =
  process.platform === 'darwin' ? WindowedOpenCardManager : HeadlessOpenCardManager;

const manager = new Manager({
  view: { width: 268, height: 469, devicePixelRatio: 2, timeoutMs: 30000 },
  onCardLoaded(card) {
    console.log(`opened ${card.url}`);
  },
  onCardError(error, card) {
    console.error(`failed ${card.url}: ${error.message}`);
  },
});

manager.install();

process.once('SIGINT', () => {
  manager.dispose();
  process.exit(0);
});
```

## Operational Notes

- Prefer a real remote template URL when the bundle loads URL-relative
  resources.
- Treat `waitForFrame()` as "a frame was submitted", not as proof that every
  image or network resource has decoded.
- Use `screenshot({ settleMs })` or CLI `--screenshot-delay` for pragmatic
  post-frame settling. Prefer a page-level ready signal when the page exposes
  one.
- Always call `destroy()` in a `finally` block for each view.
- Keep examples and troubleshooting scoped to node-lynx usage. Do not add
  internal package names, internal registries, or private distribution details
  to this open-source-facing skill.
