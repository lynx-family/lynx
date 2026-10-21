|root: ./ai
|IMPORTANT: AI context for Fragment Layer Render architecture
|architecture:{fragment_layer_render.md}

## Image loading

- Layout sizes use platform layout units: iOS points and Android physical pixels. Convert with `PhysicalPixelsPerLayoutUnit()` when applying a physical-pixel reload tolerance.
- Include scale 3 when testing one-physical-pixel boundaries; float layout sizes can otherwise exceed the boundary by a rounding error.
- Compare size changes with the last requested size, while drawing with the latest layout geometry. Source, paint-property and zero/positive size changes must still reload.
- Image fragment tests live in `dom_unittest_exec`, not `fragment_test_exec`.
