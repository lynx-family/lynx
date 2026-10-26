# Renderer resource ownership

- Publish image managers before display-list consumption so every image key can be resolved even when request execution is queued on the main thread.
- Replacing the primary image for a sign must cancel its old requests without cancelling that sign's independent CSS background layers. Removing an old image key must not remove the replacement's primary mapping.
- Preserve the content/background preload distinction described in `../ui/image/AGENTS.md`. Resource ownership regressions belong in `LynxRendererContextUnitTest.mm`.

- Identify CSS background images through `ImagePaintInfo::is_background`, set by the background-image producer. Do not infer image ownership or preload policy from `disable_default_resize`.

- In OCMock `andDo` handlers that return a local block through `NSInvocation`, call `retainArguments` before `setReturnValue` so the block survives until the caller receives it.

- `LynxImageLoadOptions.imageURL` is borrowed (`assign`). Deferred loader mocks must retain the URL separately for cancellation and late completion callbacks.

- Background callback tests must use asynchronous dispatch and explicitly synchronize completion. `dispatch_sync` can execute on the calling thread and does not prove background execution; keep the main queue blocked when testing queued result invalidation.
- When checking state after a partial mock call, let the state-changing method execute; an `OCMExpect` on that method intercepts it. Observe its notification or explicitly forward to the real implementation.
- Use initialized real views and layers for geometry tests. If a mock is necessary, stub every geometry getter consumed by the code, including `bounds`; verify the resulting mask frame and path instead of only asserting that execution did not throw.

- Scope display-list content reuse to consecutive lists and resource identity. A removed resource must leave the cache at the end of the update; explicit reset must discard cached content. Preserve draw order when native children are rebuilt.
- TextKit layout reuse does not imply reusable pixels after a bounds change or backing-content loss. TextService pages can mutate without changing their IDs, so do not infer unchanged text from those IDs.
- TextService measurement publishes a new Page, while layout may republish the same Page for unchanged text. Reuse follows the context's owned Page lifetime: replacement, removal, or destruction invalidates its identity even if an address is later recycled. The cache token must not prolong Page lifetime. Size changes and missing backing still require redraw; do not add in-place mutations of a published Page without explicit invalidation.
