# Image loading

- `LynxImageManager` serves both content images and CSS background images. The CALayer background animation path consumes `UIImage.images` and needs preloaded frames; only content image views should disable `LynxImagePreloadAllFrames`.
- Identify CSS background images through `ImagePaintInfo::is_background`, set by the background-image producer. Do not infer preload policy from `disable_default_resize`.
- Preload policy regressions are covered in `../../renderer/LynxRendererContextUnitTest.mm`.
