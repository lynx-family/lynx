# Stylesheet lifecycle in LynxML

This example loads two compiled stylesheets and sets, replaces, and clears
them on the same page elements. The base styles live in the LynxML `<style>`
block; the theme handles come from a separately fetched bundle.

## Run

Use Node.js 18+, npm, Python 3, and an Android host that supports native
`.lynxml` loading and `__ReplaceStyleSheets`. For example,
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
| Set Theme A                             | Pink card, red 30px text, card opacity 0.45.                                           |
| Set Theme B                             | Blue card and text. Theme B omits font size and opacity, so they return to 16px and 1. |
| Clear Themes                            | White card and gray text again. The base `<style>` remains active.                     |
| Repeat the current theme or clear       | Returns false because the ordered adopted set is unchanged.                           |
| Set Theme A after clearing              | The original handle can be reused.                                                    |

The status shows the current theme and whether `__ReplaceStyleSheets` changed
the adopted set.

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

Button callbacks call `__ReplaceStyleSheets([a])`, `__ReplaceStyleSheets([b])`,
or `__ReplaceStyleSheets([])`, then flush the element tree. Each array supplies
the complete adopted set in cascade order. Loaded handles remain reusable.

The `.css` names identify compiled bundle sections, not standalone CSS files.
This example keeps the XML `<style>` node as its compiled base styles. The API
also accepts raw CSS strings mixed with handles, validates the complete array
before updating, and returns false for invalid input or an unchanged handle
array. It uses native LynxML and does not depend on ReactLynx.
