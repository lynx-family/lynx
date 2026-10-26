# Image loading

- `LynxImageManager` serves both content images and CSS background images. The CALayer background animation path consumes `UIImage.images` and needs preloaded frames; only content image views should disable `LynxImagePreloadAllFrames`.
- Start loading on the calling thread. Synchronize request generations, cancellation ownership and completion state; keep UI callbacks and cancellation on the main thread. Invalidate the old request before invoking cancellation, which may synchronously invoke its completion callback. Never invoke the loader or cancellation under the state lock.
- Set `LynxImageCancelRequest` for manager-owned requests so the service returns cancellation bound to that request. A cancellation-block mock alone cannot verify service-level request cancellation.
- Request lifecycle and preload policy regressions are covered by `LynxEmbeddedImageLoadingUnitTest` in `../../renderer/LynxRendererContextUnitTest.mm`.


- Select the image service when a media resource fetcher is available, the new-image policy is enabled, or no legacy image fetcher exists. Otherwise use the configured legacy image fetcher. Read the media fetcher through the context getter, which returns nil when `hasCustomMediaFetcher` is set.
- For image service requests without an available media fetcher, preserve direct loading by skipping redirection. Changing the generic-fetcher flag must not introduce local resource lookups. Preserve explicit redirection options for available media fetchers and legacy image fetchers.
