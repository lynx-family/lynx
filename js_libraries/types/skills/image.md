---
name: Image
description: Provide information about the `<image>` element. This document focuses on practical usage patterns, performance tips, and common pitfalls beyond the API reference.
---

# Lynx UI Image SKILL.md

The `<image>` element is used to render images from network URLs, bundled static assets, or local storage. This document complements the API reference with practical sizing patterns, event handling, animated image control, and performance/compatibility guidance.

## 1. Prerequisites

To display an image correctly, you must set a non-empty `src` and provide at least one sizing strategy:

- Define both `width` and `height` via CSS, **or**
- Use `prefetch-width` and `prefetch-height` to allow early requests when the layout size is 0, **or**
- Use `auto-size`

## 2. Basic Usage

### Scaling / Cropping (`mode`)

Use `mode` to control how the bitmap fits into the `<image>` box:

```tsx
let url = "https://example.com/demo-image.png";
export function ModeExample() {
  return (
    <view style={{ display: 'flex', flexDirection: 'column', gap: '12px' }}>
      <image
        src={url}
        mode="scaleToFill"
        style={{ width: '110px', height: '80px', backgroundColor: '#222' }}
      />
      <image
        src={url}
        mode="aspectFit"
        style={{ width: '110px', height: '80px', backgroundColor: '#222' }}
      />
      <image
        src={url}
        mode="aspectFill"
        style={{ width: '110px', height: '80px', backgroundColor: '#222' }}
      />
    </view>
  )
}
```

### Auto sizing to original aspect ratio (`auto-size`)

When `auto-size={true}` and either width or height is missing, `<image>` will update its own size after the image is downloaded to keep the same aspect ratio.

```tsx
let url = "https://example.com/demo-image.png";
export function AutoSizeExample() {
  return (
    <view style={{flexDirection: 'column'}}>
      <image src={url} auto-size={true} style={{ width: '200px'}}/>
      <image src={url} auto-size={true} style={{ height: '200px'}}/>
      <image src={url} auto-size={true}/>
    </view>
  )
}
```

### Styling & Effects

You can style `<image>` with CSS for borders, rounded corners, and background.

```tsx
let url = "https://example.com/demo-image.png";
export function StyleAndEffectsExample() {
  return (
    <view style={{ display: 'flex', flexDirection: 'column', gap: '12px' }}>
      <image
        src={url}
        style={{ width: '200px', height: '120px', borderRadius: '10px', borderWidth: '2px', borderColor: 'red' }}
      />
      <image
        src={url}
        style={{ width: '200px', height: '120px', borderRadius: '10px 40px 40px 10px', borderWidth: '2px', borderColor: 'red' }}
      />
    </view>
  )
}
```

#### Special Rendering Effects

Supports special rendering effects such as Gaussian blur and tint-color.

```tsx
let url = "https://example.com/demo-image.png";
export function FilterExample() {
  return (
    <view style={{ display: 'flex', flexDirection: 'column', gap: '12px' }}>
      <image
        src={url}
        blur-radius='5px'
        style={{ width: '200px', height: '120px' }}
      />
      <image
        src={url}
        tint-color='blue'
        style={{ width: '200px', height: '120px' }}
      />
    </view>
  )
}
```

## 3. Attributes

### Common Attributes

- `src: string`: Specifies the URL of the image to display.
- `mode?: 'scaleToFill' | 'aspectFit' | 'aspectFill'`: Specifies image cropping/scaling mode.
  - `scaleToFill` (default): stretch to fill, aspect ratio may change.
  - `aspectFit`: keep aspect ratio, show full image, may letterbox.
  - `aspectFill`: keep aspect ratio, fill the box, may crop.
- `placeholder?: string`: Specifies the placeholder image to display while the main image is loading.
- `prefetch-width/prefetch-height?: number`: Provides the ability to prefetch the image when the layout size is 0.
  Generally used in scenarios where images need to be loaded early. It is recommended that the prefetch size matches the actual layout size.
- `auto-size?: boolean`: Whether to auto-size to the original aspect ratio.
- `cap-insets?: string`: Stretchable area for 9patch images.
  The value must be four specific numerical values representing top, right, bottom, and left insets respectively. Percentages and decimals are not supported.
- `cap-insets-scale?: number`: Used with `cap-insets`. It adjusts the scale of the defined insets relative to the original image size. Default is 1.0.
- `blur-radius?: string`: Gaussian blur radius.
- `tint-color?: string`: Changes the color of all non-transparent pixels to the color specified by tint-color.
- `defer-src-invalidation?: boolean`: When set to true, the `<image>` will only clear the previously displayed image resource after a new image has successfully loaded.
  The default behavior is to clear the image resource before starting a new load. This can resolve flickering issues when the image src is switched and reloaded.

### Animated Image Attributes

- `autoplay?: boolean`: Specifies whether the animated image should start playing automatically once it is loaded. Default is `true`.
- `loop-count?: number`: Number of times an animated image plays, 0 stands for infinite loop. Default is `0`.

### Platform Specific

- `image-config?: 'RGB_565' | 'ARGB_8888'` (Android only). Default is `ARGB_8888`.
  - `ARGB_8888`: 32-bit memory per pixel, supports semi-transparent images.
  - `RGB_565`: 16-bit memory per pixel, reduces memory usage but loses transparency.

## 4. Common Events

You can bind common events to observe the image loading lifecycle and handle errors:

- `bindload`: Triggered when the image is successfully loaded.
- `binderror`: Triggered when the image fails to load. You can check the event detail for error messages and codes.

```tsx
export function EventsExample() {
  return (
    <image
      src="https://example.com/demo-image.png"
      style={{ width: '160px', height: '100px' }}
      bindload={(e) => {
        console.log('image loaded', e)
      }}
      binderror={(e) => {
        console.log('image error', e)
      }}
    />
  )
}
```

## 5. Animated Image Events & Methods

### Animated Image Methods

For animated images (e.g., GIF), you can control playback via SelectorQuery `invoke` methods:

- `pauseAnimation`
- `resumeAnimation`
- `stopAnimation`
- `startAnimate`

```tsx
const GIF_URL = 'https://example.com/demo-image.gif'

function invokeGif(method: 'pauseAnimation' | 'resumeAnimation' | 'stopAnimation' | 'startAnimate') {
  return () => {
    lynx.createSelectorQuery()
      .select('#gifs')
      .invoke({ method })
      .exec()
  }
}

export function AnimatedImageControllerExample() {
  const onPause = invokeGif('pauseAnimation')
  const onResume = invokeGif('resumeAnimation')
  const onStop = invokeGif('stopAnimation')
  const onRestart = invokeGif('startAnimate')

  return (
    <view style={{ display: 'flex', flexDirection: 'column', gap: '12px' }}>
      <image
        id="gifs"
        src={GIF_URL}
        autoplay={true}
        loop-count={0}
        mode="aspectFit"
        style={{ width: '200px', height: '200px', backgroundColor: '#111' }}
      />

      <view style={{ display: 'flex', flexDirection: 'row', gap: '8px' }}>
        <view bindtap={onPause} style={{ padding: '8px', backgroundColor: '#EEE' }}>
          <text>Pause</text>
        </view>
        <view bindtap={onResume} style={{ padding: '8px', backgroundColor: '#EEE' }}>
          <text>Resume</text>
        </view>
        <view bindtap={onStop} style={{ padding: '8px', backgroundColor: '#EEE' }}>
          <text>Stop</text>
        </view>
        <view bindtap={onRestart} style={{ padding: '8px', backgroundColor: '#EEE' }}>
          <text>ReStart</text>
        </view>
      </view>
    </view>
  )
}
```

### Animated Image Events

You can bind specific events to monitor the playback status of animated images, such as start, loop completion, and final completion.

- `bindstartplay`: Triggered when the animated image starts playing.
- `bindcurrentloopcomplete`: Triggered when one loop of the animated image finishes playing.
- `bindfinalloopcomplete`: Triggered when the animated image finishes playing all `loop-count` loops. If `loop-count` is not set, this callback will not be triggered.

```tsx
const GIF_URL = 'https://example.com/demo-image.gif'

export function AnimatedImageEventsExample() {
  return (
    <image
      src={GIF_URL}
      loop-count={2}
      style={{ width: '200px', height: '200px' }}
      bindstartplay={() => {
        console.log('image start play')
      }}
      bindcurrentloopcomplete={() => {
        console.log('image current loop completed')
      }}
      bindfinalloopcomplete={() => {
        console.log('image final loop completed')
      }}
    />
  )
}
```

## 6. FAQ

**Q: The image is not displayed. How should I troubleshoot it?**
A:
- First, ensure that `src` is not empty and that you have applied a valid sizing strategy (e.g., setting `width` and `height`, using `prefetch-width` and `prefetch-height`, or enabling `auto-size`).
- Next, add a `binderror` event handler to capture and analyze the error message or code.

**Q: How to fix image flickering?**
A: If the `src` or the size of the image view changes, it is recommended to add `defer-src-invalidation={true}` to avoid flickering.

**Q: Does `<image>` support SVG?**
A: No. For SVG images, please use the `<svg>` component.

## 7. Anti-Patterns (Things to Avoid)

When using `<image>`, you strictly **MUST NOT** do the following:

- **DO NOT** nest any child nodes inside `<image>` (e.g., `<text>`). `<image>` must be used as a leaf (empty) element.
