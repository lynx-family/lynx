# Stylesheet lifecycle in LynxML

This example loads two compiled stylesheets and adopts, replaces, and removes
them on the same page elements. The base styles live in the LynxML `<style>`
block; the theme handles come from a separately fetched bundle.

## Run

Use Node.js 18+, npm, Python 3, and an Android host that supports native
`.lynxml` loading and `__RemoveStyleSheet` / `__ReplaceStyleSheet`. For example,
build [Lynx Explorer](../../android/README.md) with an SDK containing these APIs.
The `engine-version="4.2"` value in the XML is a parsing target, not a guarantee
that a released SDK has the new APIs. The page reports missing APIs explicitly.

From this directory:

```sh
npm ci --registry=https://registry.npmjs.org
npm run build
npm run serve
```

The build uses the public `@lynx-js/tasm` package's WebAssembly encoder. It emits
`dist/index.lynxml` and `dist/themes.bundle`; generated files are not checked in.

The encoder must match the host's bundle codec. If your host uses a different
codec, set `TASM_MODULE` to the absolute path of its compatible TASM package's
`index.js` when running `npm run build`. That module must expose
`getEncodeMode('wasm')`. Do not mix a custom host codec with the default public
bundle; the host will reject it before loading the CSS sections.

For an Android device connected over USB, in another terminal:

```sh
adb devices
DEVICE=your-device-serial
adb -s "$DEVICE" reverse tcp:3078 tcp:3078
adb -s "$DEVICE" shell am start -a android.intent.action.VIEW \
  -d 'lynx://open?url=http%3A%2F%2F127.0.0.1%3A3078%2Findex.lynxml' \
  com.lynx.explorer
```

You can also open `http://127.0.0.1:3078/index.lynxml` through the host's URL
launcher after setting up port forwarding. Keep the server running.

For a network-accessible server, set the bundle URL when building and open the
generated XML from that server:

```sh
STYLESHEET_BUNDLE_URL=https://example.com/demo/themes.bundle npm run build
```

Upload both generated files to the corresponding locations. Refresh the page
after rebuilding; this example does not require changes to the host app.

## Try it

Wait for **Ready - base styles**, then use these buttons:

| Action                                  | Expected result                                                                        |
| --------------------------------------- | -------------------------------------------------------------------------------------- |
| Initial load                            | White card, gray 16px text, opacity 1. Loading alone does not apply a theme.           |
| Adopt A                                 | Pink card, red 30px text, card opacity 0.45.                                           |
| Replace A to B                          | Blue card and text. Theme B omits font size and opacity, so they return to 16px and 1. |
| Remove                                  | White card and gray text again. The base `<style>` remains active.                     |
| Remove again / Replace A to B without A | Returns false and leaves the base styles unchanged.                                    |
| Adopt A again                           | The original handle can be reused after removal.                                       |
| Reset                                   | Removes either theme and restores the base styles.                                     |

The status shows the current theme and the remove/replace return value. For
Adopt and Reset, `true` only indicates that the demo performed the action;
`__AdoptStyleSheet` itself has no return value.

## Source and API flow

- `index.lynxml`: base CSS, main-thread script, preview elements, and buttons.
- `themes.json`: selector/property definitions for the two named CSS sections.
- `build.mjs`: encodes those sections and substitutes the configured bundle URL.

The main-thread script waits for `__RenderPage`, creates the elements once, and
fetches the theme bundle. It keeps the handles returned by:

```js
const a = __LoadStyleSheet('theme-a.css', response.url);
const b = __LoadStyleSheet('theme-b.css', response.url);
```

Button callbacks call `__AdoptStyleSheet(a)`, `__ReplaceStyleSheet(a, b)`, or
`__RemoveStyleSheet(current)`, then flush the element tree. Replace preserves
the adopted position; after replacing A with B, subsequent removal uses B's
handle. Re-loading the same section creates another handle and cannot be used
to remove an earlier adoption.

The `.css` names identify compiled bundle sections, not standalone CSS files.
This example does not remove the XML `<style>` node or accept raw CSS text in
the handle APIs. It uses native LynxML and does not depend on ReactLynx.
