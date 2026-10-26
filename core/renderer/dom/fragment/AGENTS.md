|root: ./ai
|IMPORTANT: AI context for Fragment Layer Render architecture
|architecture:{fragment_layer_render.md}

## Image loading

- Layout sizes use platform layout units: iOS points and Android physical pixels. Convert with `PhysicalPixelsPerLayoutUnit()` when applying a physical-pixel reload tolerance.
- Include scale 3 when testing one-physical-pixel boundaries; float layout sizes can otherwise exceed the boundary by a rounding error.
- Compare size changes with the last requested size, while drawing with the latest layout geometry. Source, paint-property and zero/positive size changes must still reload.
- Platform image resources may opt into resize reuse: without downsampling, reuse across positive sizes; with downsampling, reuse only within the last requested dimensions. Auto-size keeps its layout-dependent request behavior. Failed requests and custom fetchers must not opt in.
- Image fragment tests live in `dom_unittest_exec`, not `fragment_test_exec`.

- A placeholder is a drawable image source even when `src` is empty. Its first assignment, replacement and removal must invalidate drawing; removing the last source must clear the previous image.
- Parse prefetch, downsampling and priority request options only on iOS until other platform renderers consume them; unsupported attribute updates must not recreate images.
- Use compile-time `GetOSType()` conditions for platform gates; new `#if OS_*` branches fail the macro checker. Include `macro` when validating platform-specific changes.
