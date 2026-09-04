# Use Sparkling containers in Android Lynx Explorer

Android Explorer keeps Lynx as its universal runtime and offers Sparkling as an
optional flavor extension. The `withoutSparkling` flavor contains neither the
Sparkling runtime nor Sparkling Go bundles. The `withSparkling` flavor packages
the official playground bundles built from the shared pinned source in
`explorer/sparkling-source.json`.

## Route ownership

Every Android entry calls `RouteCoordinator`, which parses the input once into
an immutable `LaunchDescriptor`. Raw HTTP, HTTPS, `assets://`, and
`file://lynx?local://` routes use Lynx unless the caller explicitly requests
Sparkling. A canonical `hybrid://lynxview_page` route always belongs to
Sparkling, including when wrapped by `lynx://open?url=...`.

After ownership resolves to Sparkling, a launch error is returned as a typed
failure and is never retried through Lynx. Recorder routes remain Lynx-only.
Recent history is updated only after a launcher accepts the route.

Android uses the Activity task stack for toolbar and system back. Sparkling's
`router.open` re-enters the coordinator with explicit Sparkling ownership, and
`router.close` only finishes the owning Activity when an optional container ID
matches. `router.open` also forwards `options.extra` to the launched
`SparklingContext`; keys and values are normalized to strings, and these values
override matching URL query parameters. Runtime-owned properties remain
reserved for the host.

## Capabilities and appearance

The Universal Home receives
`explorerSupportsExplicitRouteOwnership=true` and a flavor-derived
`explorerSupportsSparklingContainer`. Ordinary Lynx pages have
`sparklingAvailable=false`, `sparklingNavigation=false`, and no Sparkling
container identity or MethodPipe. Sparkling pages receive those values from the
SDK and expose only the host modules installed by Explorer.

Both runtimes read the same Auto/Light/Dark preference from Explorer storage.
`force_theme_style` remains a page override and does not change the app
preference. Both launchers use the shared Android loading/error surface;
Sparkling receives it through `SparklingUIProvider`.

The released `SparklingUIProvider` API supplies loading, error, and toolbar
views, but it does not expose a retry callback or container lifecycle hooks.
Explorer therefore presents an honest terminal Sparkling error view; retrying
requires reopening the route. Lynx errors use Explorer's normal retry action.

## Verification

From `explorer/android`, run the two flavor unit-test tasks and
`verifySparklingAndroidRuntimeSmokeTestArtifacts`. The latter builds Debug and
Release artifacts, checks dependency ownership, verifies enabled/disabled APK
contents, and compiles both instrumentation variants. Device-backed
instrumentation can then run each variant's `SparklingRuntimeSmokeTest`.
