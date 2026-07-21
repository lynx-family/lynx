# Lynx webview Element

This guide covers the current native Android, iOS, and Harmony `<webview>` element behavior.

## When to Use `<webview>`

### Use `<webview>` when:

- you need to host remote content through `src`
- you need to render inline HTML through `html`
- you need native webview events such as `load`, `error`, or `message`
- you need native webview control through `reload` or `eval`

### Do not use `<webview>` when:

- you need to assume PC or Windows-only APIs such as cookie methods, `initjs`, `use-osr`, `openwindow`, or `locationchange` on mobile
- you need Lynx child content inside the element instead of loading page content through `src` or `html`
- you need to assume `params` or custom `webview-type` behavior is portable without host-native wiring

## Portability Guardrails

- `src` overrides `html`
- treat `webview-type` as a host-integration surface
- treat `params` as loader-specific metadata, not as a portable page-loading API
- normalize iOS `binderror` payloads because its navigation-error keys differ from Android and Harmony
- treat transparent background as built-in native behavior, not as a dedicated prop
- use `enable-debug` only for development workflows

## Quick Start

### Cross-platform URL load

```tsx
<webview
  id="webview"
  src="https://example.com"
  bindload={handleLoad}
  bindmessage={handleMessage}
  binderror={handleError}
  style={{ width: '100%', height: '320px' }}
/>
```

### Inline HTML load

```tsx
<webview
  html={`<html><body><h1>Hello</h1><script>window.postMessage('ready')</script></body></html>`}
  bindmessage={handleMessage}
  style={{ width: '100%', height: '240px' }}
/>
```

### Transparent page content

```tsx
<view style={{ width: '100%', height: '240px', backgroundColor: '#1e293b' }}>
  <webview
    html={`<html><body style="margin:0;background:transparent;color:white">overlay</body></html>`}
    style={{ width: '100%', height: '100%' }}
  />
</view>
```

The built-in default webview paths can render transparent page content where the current native mobile implementations support it. The loaded page itself still needs transparent HTML or CSS for the parent background to show through.

## Properties

### Core cross-platform props

| Prop | Type | What It Does |
| --- | --- | --- |
| `src` | `string` | Loads a URL into the native webview |
| `html` | `string` | Loads inline HTML when no non-empty `src` is present |
| `webview-type` | `string` | Selects a host-integrated implementation key; keep `"default"` unless native code wires a custom type |
| `enable-debug` | `boolean` | Enables platform webview debugging or inspection features when supported |

### Host-integration or platform-specific props

| Prop | Platforms | What It Does |
| --- | --- | --- |
| `params` | Android, iOS | Passes loader-specific setup data; do not treat it as a portable content API |
| `bounces` | iOS | Controls `WKWebView` bounce behavior |
| `scroll-bar-enable` | iOS | Toggles both horizontal and vertical scroll indicators |

There is no dedicated transparent-background prop in the current native mobile implementations.

## Events

### Frontend events

| Event | Platforms | When It Fires | Payload |
| --- | --- | --- | --- |
| `load` | Android, iOS, Harmony | Page load setup finishes | none |
| `error` | Android, iOS, Harmony | Empty input or page-load failure occurs | Android and Harmony use `errorCode` / `errorMsg`; iOS navigation failures use `errCode` / `errMsg` |
| `message` | Android, iOS, Harmony | The injected native bridge receives `window.postMessage(...)` data | `msg` |

### Error payload normalization

```ts
const handleError = (event) => {
  const detail = event.detail ?? {};
  const code = detail.errorCode ?? detail.errCode;
  const message = detail.errorMsg ?? detail.errMsg;
  console.warn('webview error', code, message);
};
```

### Standard binding syntax

```tsx
<webview
  bindload={handleLoad}
  binderror={handleError}
  bindmessage={handleMessage}
/>
```

## UI Methods

### `eval`

Parameters:

- `func: string`

Use it to execute JavaScript in the hosted page:

```tsx
ref.current?.invoke({
  method: 'eval',
  params: {
    func: "window.postMessage('hello from eval')",
  },
});
```

### `reload`

Parameters:

- none

Use it to reload the current page:

```tsx
this.getNodeRef('#webview').invoke({
  method: 'reload',
});
```

## Host Integration Notes

### Custom `webview-type`

Use a custom `webview-type` only when the host-native app has registered that implementation:

- Android resolves the type through its native WebView service provider
- iOS resolves it through `LynxWebViewService`
- Harmony resolves it through its provider map

If you do not control the host-native integration, stay with the default type.

### `params`

Use `params` only as implementation-specific setup data for a custom native loader.

Do not assume:

- that the default native loaders consume it
- that Harmony supports it in the current native path
- that runtime `params` updates behave the same on Android and iOS

## What Not to Assume

Do not teach these as native Android, iOS, and Harmony `<webview>` facts:

- cookie methods
- `initjs`
- `use-osr`
- `bindopenwindow`
- `bindlocationchange`
