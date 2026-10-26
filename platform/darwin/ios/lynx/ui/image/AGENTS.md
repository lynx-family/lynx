# Image loading

- `LynxImageManager` serves both content images and CSS background images. The CALayer background animation path consumes `UIImage.images` and needs preloaded frames; only content image views should disable `LynxImagePreloadAllFrames`.
- Start loading on the calling thread. Synchronize request generations, cancellation ownership and completion state; keep UI callbacks and cancellation on the main thread. Invalidate the old request before invoking cancellation, which may synchronously invoke its completion callback. Never invoke the loader or cancellation under the state lock.
- Set `LynxImageCancelRequest` for manager-owned requests so the service returns cancellation bound to that request. A cancellation-block mock alone cannot verify service-level request cancellation.
- Request lifecycle and preload policy regressions are covered by `LynxEmbeddedImageLoadingUnitTest` in `../../renderer/LynxRendererContextUnitTest.mm`.

- For content images, suppress requests only when both layout dimensions are zero or negative and no prefetch dimension is positive; auto-size still requires at least one layout dimension. Keep this guard out of CSS background loading.
- Prefetch changes the request target size only. Auto-size completion must compare the image with the original layout size, including a missing layout dimension.
- Forward explicit downsampling only for non-auto-size content images and preserve the context's downsampling scale policy. Request priority changes apply to subsequent requests without restarting the active load.

- Select the image service when a media resource fetcher is available, the new-image policy is enabled, or no legacy image fetcher exists. Otherwise use the configured legacy image fetcher. Read the media fetcher through the context getter, which returns nil when `hasCustomMediaFetcher` is set.
- For image service requests without an available media fetcher, preserve direct loading by skipping redirection. Changing the generic-fetcher flag must not introduce local resource lookups. Preserve explicit redirection options for available media fetchers and legacy image fetchers.
- Resize reuse excludes both legacy and media fetchers, including an injected media fetcher hidden by `hasCustomMediaFetcher`. Failed or invalidated manager requests retain resize-triggered retries. Keep reuse checks out of initial loading and the existing equal-size fast path.
