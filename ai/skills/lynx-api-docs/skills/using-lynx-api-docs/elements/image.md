# Lynx image Element

This guide covers the current native Android, iOS, and Harmony `<image>` element behavior.

## When to Use `<image>`

### Use `<image>` when:

- you need to render bitmap-oriented image content from a remote URL, local resource, or supported base64 image data
- you need image lifecycle hooks such as `bindload` or `binderror`
- you need placeholders while the main source loads
- you need animated image playback control such as `pauseAnimation` or `resumeAnimation`
- you need image-specific behavior such as `mode`, `auto-size`, `tint-color`, or `cap-insets`

### Do not use `<image>` when:

- the image is purely decorative background for a container and should stay in CSS or layout space
- you need a background that participates in `background-*` rules rather than image loading events
- you need SVG rendering on Android, iOS, or Harmony native element paths; use `<svg>` instead
- you are embedding an inline image inside text flow and should use the inline-text image path instead

### Choosing between `<image>` and alternatives

| Scenario | Use |
| --- | --- |
| Actual content image with load or error handling | `<image>` |
| Decorative container background | CSS `background-image` on a `<view>` |
| Animated image that needs play or pause control | `<image>` |
| SVG content | `<svg>` |
| Inline image mixed into text layout | Text or inline image workflow |

> **Sizing note:** For the most reliable loading behavior, give `<image>` a resolved size through `style`, or use `prefetch-width` / `prefetch-height`, or enable `auto-size`.
>
> **Boundary note:** In the current Android + iOS + Harmony native paths, `<image>` does not own SVG rendering. Use `<svg>` for SVG.
>
> **Accessibility note:** The current Lynx `<image>` surface does not expose a Web-style `alt` prop. If product requirements need alternate text or spoken labels, handle that through the surrounding accessibility strategy instead of assuming an `alt` attribute exists.

## Quick Start

### Basic image

```tsx
<image
  src="https://example.com/banner.png"
  mode="aspectFill"
  style={{ width: '320px', height: '180px' }}
/>
```

### Placeholder + auto size

```tsx
<image
  src="https://example.com/photo.png"
  placeholder="https://example.com/photo-thumb.png"
  auto-size={true}
  mode="aspectFit"
  style={{ width: '240px' }}
/>
```

## Properties

### Core props

| Prop | Type | Default | What It Does |
| --- | --- | --- | --- |
| `src` | `string` | — | Main bitmap image source |
| `placeholder` | `string` | — | Placeholder source shown while the main image loads |
| `mode` | `"scaleToFill" \| "aspectFit" \| "aspectFill" \| "center"` | `"scaleToFill"` | Controls how the bitmap fits the image box |
| `auto-size` | `boolean` | `false` | Resizes the element to preserve the bitmap aspect ratio after load |
| `blur-radius` | `string` | `0` | Applies blur to the image |
| `defer-src-invalidation` | `boolean` | `false` | Keeps the previous image visible until the replacement finishes loading |
| `tint-color` | `string` | — | Recolors the non-transparent pixels |
| `cap-insets` | `string` | — | Defines stretchable inset regions for 9-patch style rendering |
| `cap-insets-scale` | `number` | `1` | Scales the inset values used by `cap-insets` |
| `skip-redirection` | `boolean` | `false` | Bypasses the usual resource redirection flow before final load |

> **Harmony note for `center`:** `center` is implemented on iOS and Android, and on Harmony's new-image path. For the most consistent cross-platform behavior, prefer `scaleToFill`, `aspectFit`, or `aspectFill`.
>
> **SVG note:** `src` should not be treated as SVG-capable on Android, iOS, and Harmony native `image` paths.

### Animated image props

| Prop | Type | Default | What It Does |
| --- | --- | --- | --- |
| `autoplay` | `boolean` | `true` | Starts animated images automatically after load |
| `loop-count` | `number` | `0` | Sets the repeat count for animated images |

### Platform-specific and advanced props

| Prop | Platforms | What It Does |
| --- | --- | --- |
| `prefetch-width` / `prefetch-height` | iOS, Android | Lets the request start before layout size is ready |
| `enable-report-info` | iOS, Android, Harmony (`UINewImage`) | Expands `bindload` payload with timing, origin, and view-size diagnostics |
| `extra-load-info` | iOS | Adds extra legacy loader details to `bindload` payload |
| `downsampling` | iOS, Harmony (`UINewImage`) | Enables downsampling while loading large images |
| `progressive-rendering` | iOS, Android | Enables progressive image rendering when supported |
| `additional-custom-info` | iOS, Android | Attaches custom metadata to the request or reporting flow |
| `enable-super-resolution` | iOS, Android | Enables the super-resolution request path |
| `super-resolution-scale` | iOS, Android | Sets the scale used by super-resolution |
| `ios-frame-cache-automatically` | iOS | Controls animated frame caching behavior |
| `use-new-image` | iOS | Switches iOS to the newer image loading path |
| `ignore-memory-cache` / `ignore-disk-cache` | iOS | Skips cache lookup for the request |
| `not-cache-to-memory` / `not-cache-to-disk` | iOS | Prevents storing the result into cache |
| `image-transition-style` | iOS | Enables transition effects such as fade-in |
| `request-priority` / `cache-choice` | iOS | Tunes request priority and cache strategy |
| `placeholder-hash-config` | iOS | Configures hashed placeholder behavior |
| `async-request` | Android | Enables async redirect, check, or request behavior |
| `image-config` | Android | Chooses bitmap config such as `ARGB_8888` or `RGB_565` |
| `local-cache` | Android | Adjusts local-cache usage behavior |
| `disable-default-placeholder` | Android | Disables the platform default placeholder behavior |
| `disable-default-resize` | Android | Disables the default resize optimization path |
| `enable-resource-hint` | Android | Enables resource-hint-assisted loading behavior |
| `enable-custom-gif-decoder` | Android | Enables a custom GIF decoder path |
| `android-enable-smooth-animation` | Android | Enables smoother animated playback behavior |

## Events

### Event payload fields

| Field | Available On | Appears In | Description |
| --- | --- | --- | --- |
| `width` | All | `bindload` | Loaded image width |
| `height` | All | `bindload` | Loaded image height |
| `errMsg` | iOS, Android | `binderror` | Human-readable error message |
| `err_msg` | Harmony | `binderror` | Human-readable error message on Harmony |
| `error_code` | All | `binderror` | Platform or native error code |
| `lynx_categorized_code` | iOS, Android | `binderror` | Lynx categorized image error code |
| `src` | iOS, Android, Harmony (`UINewImage`) | `bindload` with reporting | Final reported source URL |
| `load_start` | iOS, Android, Harmony (`UINewImage`) | `bindload` with reporting | Load start timestamp |
| `load_finish` | iOS, Android, Harmony (`UINewImage`) | `bindload` with reporting | Load finish timestamp |
| `cost` | iOS, Android, Harmony (`UINewImage`) | `bindload` with reporting | Total load cost |
| `view_width` / `view_height` | iOS, Android, Harmony (`UINewImage`) | `bindload` with reporting | View size when reporting is enabled |
| `origin` | iOS, Harmony (`UINewImage`) | `bindload` with reporting | Resource origin or cache source info |
| `memory_cost` | iOS, Harmony (`UINewImage`) | `bindload` with reporting | Reported memory cost |
| `memoryCost` | Android | `bindload` with reporting | Reported memory cost on Android |
| `scale` | iOS | `bindload` with reporting | Loaded image scale |
| `downsampled` | iOS | `bindload` with reporting | Whether the iOS request was downsampled |

### Event bindings

| Event Binding | Platforms | When It Fires | Payload Highlights |
| --- | --- | --- | --- |
| `bindload` | All | Main image source loads successfully | Always includes `width` and `height`; can include report fields when reporting is enabled |
| `binderror` | All | Main image source fails to load | Error message and error code |
| `bindstartplay` | iOS, Android, Harmony (animated image paths) | Animated playback starts | Empty detail payload |
| `bindcurrentloopcomplete` | iOS, Android, Harmony (animated image paths) | One animation loop finishes | Empty detail payload |
| `bindfinalloopcomplete` | iOS, Android, Harmony (animated image paths) | Final configured loop finishes | Empty detail payload |

### Event example

```tsx
const onLoad = (event) => {
  const { width, height } = event.detail;
  console.log(`loaded: ${width}x${height}`);
};

const onError = (event) => {
  console.log('image error', event.detail);
};

<image
  src="https://example.com/cover.png"
  placeholder="https://example.com/cover-thumb.png"
  bindload={onLoad}
  binderror={onError}
  style={{ width: '240px', height: '160px' }}
/>;
```

## UI Methods

| Method | Parameters | Description |
| --- | --- | --- |
| `startAnimate` | none | Starts or restarts animated image playback |
| `pauseAnimation` | none | Pauses animated image playback |
| `resumeAnimation` | none | Resumes a paused animated image |
| `stopAnimation` | none | Stops playback and resets animation state |

### Method example

```tsx
lynx.createSelectorQuery()
  .select('#hero-gif')
  .invoke({ method: 'pauseAnimation' })
  .exec();

lynx.createSelectorQuery()
  .select('#hero-gif')
  .invoke({ method: 'resumeAnimation' })
  .exec();
```

## Common Patterns

### Prevent flicker when swapping sources

```tsx
<image
  src={activeSrc}
  placeholder={previewSrc}
  defer-src-invalidation={true}
  mode="aspectFill"
  style={{ width: '320px', height: '180px' }}
/>
```

### Auto-size by aspect ratio

```tsx
<image
  src="https://example.com/poster.png"
  auto-size={true}
  mode="aspectFit"
  style={{ width: '240px' }}
/>
```

### Animated image with playback events

```tsx
<image
  id="hero-gif"
  src="https://example.com/hero.gif"
  autoplay={true}
  loop-count={2}
  bindstartplay={() => console.log('start')}
  bindcurrentloopcomplete={() => console.log('loop')}
  bindfinalloopcomplete={() => console.log('done')}
  style={{ width: '200px', height: '200px' }}
/>
```

## Platform Availability Matrix

| Feature | iOS | Android | Harmony |
| --- | --- | --- | --- |
| Basic image load and placeholder | Yes | Yes | Yes |
| SVG rendering through `<image>` | No | No | No |
| `auto-size` | Yes | Yes | Yes |
| `tint-color` | Yes | Yes | Yes |
| `cap-insets` | Yes | Yes | Yes |
| Animated image methods | Yes | Yes | Yes |
| Animated image events | Yes | Yes | Yes |
| `enable-report-info` frontend payload | Yes | Yes | `UINewImage` only |
| `prefetch-width` / `prefetch-height` | Yes | Yes | No |
| `image-config` | No | Yes | No |
| `async-request` | No | Yes | No |
| `downsampling` prop | Yes | No | `UINewImage` only |
| `mode="center"` | Yes | Yes | `UINewImage` only |
