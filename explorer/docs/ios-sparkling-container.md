# Use Sparkling containers in iOS Lynx Explorer

Phase one gives Lynx Explorer for iOS an explicit full-Sparkling launch path
without changing the default container for ordinary bundles. One
`RouteCoordinator` parses every URL entry into an immutable `LaunchDescriptor`
and presents the selected container.

This is a container integration, not a second Lynx integration. `Lynx`,
`LynxBase`, `LynxServiceAPI`, `LynxService`, `LynxDevtool`, `BaseDevtool`, and
`XElement` continue to come from the current Lynx checkout in both build modes.
The generated `LynxLibraryRegistry` pod is also source-owned; it comes from
`generated/lynx-library` in the Explorer workspace.

## User-visible routing contract

Explorer keeps its universal homepage and exposes Sparkling Go as an optional
extension. Its ordinary open action and extension routes have deliberately
different meanings:

- **Open** keeps raw and Legacy bundle URLs on the Legacy Explorer container.
- **Sparkling Go** appears as one compact extension row only when the host
  advertises the Sparkling container capability. Its embedded root bundle asks
  for a full Sparkling container and opens further Sparkling pages.

The coordinator applies the following rules regardless of whether the request
came from the homepage, scanner, application delegate, or an in-page router:

| Input | Open | Explicit Sparkling route |
| --- | --- | --- |
| Raw HTTP/HTTPS bundle | Legacy | Sparkling |
| `file://lynx?local://bundle_path` bundle | Legacy | Sparkling |
| `lynx://open?url=encoded_url` around a raw or Legacy bundle | Legacy after unwrap | Sparkling after semantic mapping |
| `hybrid://lynxview_page?bundle=bundle_path` | Sparkling | Sparkling |
| `hybrid://lynxview_page?url=encoded_url` | Sparkling | Sparkling |
| Legacy wrapper around a canonical Sparkling scheme | Sparkling after unwrap | Sparkling after unwrap |
| Recorder URL | Recorder/Legacy | `recorder_unsupported_in_sparkling` |
| Malformed or unsupported URL | Typed route error | Typed route error |

A canonical Sparkling scheme always owns its request; an ordinary **Open**
cannot force it into Legacy. Conversely, phase one never promotes a raw bundle
to Sparkling unless the caller explicitly enters **Sparkling Go** or selects
Sparkling from the QR scanner.

The official Sparkling resolver validates canonical schemes. Wrapper unwrapping
is bounded and normally requires exactly one `url` target. The parser also
preserves Explorer's historical unescaped `url=https://...&...` tail form,
where the remaining query belongs to that single target rather than to the
wrapper. The original encoded URL is preserved.

## LaunchDescriptor

`LaunchDescriptor` is the software development kit (SDK)-type-neutral boundary
between URL syntax and container creation. It records:

- the original input and, when applicable, the original canonical scheme;
- a local bundle, remote bundle, or Recorder resource;
- initial data, common props, page props, page name, and lossless query items;
- viewport, background/transparency, navigation, and presentation options;
- cache and Node-API/debug options;
- requested and resolved container types plus the route source.

The Legacy launcher is the only place that turns the typed model back into the
existing string-keyed Explorer parameters. The Sparkling launcher builds an
`SPKContext` and calls `SPKRouter.create(withURL:context:frame:)`; it never
substitutes a plain `LynxView`.

## Legacy parameter compatibility

Known typed values use the last valued occurrence of a query name. Boolean
parameters preserve the Legacy shell's `NSString.boolValue` compatibility;
unrecognized values remain available in the lossless query/extra views instead
of making an otherwise valid Legacy URL fail.

| Legacy parameter | Parse type | Descriptor field | Legacy consumer | Sparkling mapping | Invalid-value behavior |
| --- | --- | --- | --- | --- | --- |
| `animated` | Foundation-compatible Boolean, default `true` | `presentation.animated` | `NavigationHost` transition | Same coordinator transition; also retained in query/page props | A valueless item does not erase an earlier value; an unrecognized valued spelling follows `NSString.boolValue` and remains losslessly preserved |
| `hidden_nav` | Foundation-compatible Boolean, default `false` | `navigation.navigationHidden` | Legacy shell navigation visibility | Canonical `hide_nav_bar`; retained as `hiddenNav` page prop | Unrecognized values follow `NSString.boolValue` and remain losslessly preserved |
| `fullscreen` | Foundation-compatible Boolean, default `false` | `navigation.fullScreen`, navigation hidden, transparent appearance | Legacy fullscreen layout | `fullscreen`, `hide_nav_bar`, `hide_status_bar`, and transparent status-bar context | Unrecognized values follow `NSString.boolValue` and remain losslessly preserved |
| `title` | String | `navigation.title` | Legacy navigation title | Canonical/context `title` and `title` page prop | A valueless item is preserved but is not promoted to the typed field; it does not erase an earlier valued item |
| `title_color` | String | `navigation.titleColor` | Legacy title color | Canonical/context `title_color` and `titleColor` page prop | Preserved as a string; an unsupported color is ignored by the rendering owner |
| `bar_color` | String | `navigation.barColor` | Legacy navigation background | Canonical `nav_bar_color` and `barColor` page prop | Preserved as a string; an unsupported color is ignored by the rendering owner |
| `back_button_style` | String | `navigation.backButtonStyle` | Legacy back image and frontend theme | Preserved in the canonical query, `SPKContext.queryItems`, and `backButtonStyle` page prop | Preserved for forward compatibility; the active container decides which styles it supports |
| `width` + `height` | Positive Legacy integer-prefix values in physical pixels (`NSString.intValue` / Int32 semantics); both required | `viewport` | Legacy screen/viewport size after display-scale conversion | `SPKRouter` frame after the same display-scale conversion | An incomplete, non-positive, or out-of-range pair leaves the typed viewport unset while preserving the raw values |
| `orientation` | `portrait` or `landscape` | `navigation.orientation` | Legacy supported-orientation mask; an unknown spelling remains untyped and losslessly preserved | The pinned Sparkling API has no safe per-container equivalent: any non-canonical route explicitly forced into Sparkling with an `orientation` item returns `sparkling_option_unsupported`, including an unknown spelling; canonical input remains owned by the official resolver | Raw values are always preserved; only the forced non-canonical Sparkling conversion rejects them |
| `enable_napi_addon` | Narrow Legacy truthy set (`1`, `true`, or `yes`), default `false` | `debugOptions.enableNAPIAddon` | Legacy background runtime, lifecycle listener, and module | Page-owned background runtime and listener retained by the `SPKContext` configuration; Explorer module registered on the builder | Other values disable the addon and remain losslessly preserved |
| `initial_page` | Pass-through string | `pageName`, `extras`, `queryItems`, and `initialPage` page prop | Legacy global prop | Canonical query and `SPKContext.queryItems` | No additional typed validation; a valueless item is not promoted to a prop |
| Unknown query key | Pass-through | Ordered `queryItems`; last value in `extras` and camel-cased page props | Legacy parameter/global-prop compatibility | Canonical query plus `SPKContext.extra`/`queryItems` and camel-cased page prop | No typed rejection; duplicate encoded items remain ordered while dictionary views are last-value-wins |

The parser also recognizes `container_bg_color` and `trans_status_bar`. Fields
that cannot be represented safely fail explicitly instead of silently changing
container semantics.

Canonical `hide_nav_bar` and `nav_bar_color` take precedence over their Legacy
aliases independent of query order. `hide_status_bar` is modeled separately
from fullscreen, so a canonical page can hide the status bar without changing
its presentation mode.

## Global props and capabilities

Ordinary Sparkling page props use this precedence:

```text
Explorer common props
  < Sparkling stable/container props
  < launch/page props
```

The common layer does not shadow the pinned SDK's stable device, viewport,
safe-area, URL, `SPK_version`, `lynxSdkVersion`, `sparklingVersion`, or
`containerInitTime` fields. The following normalized identity/capability names
are reserved at every untrusted boundary:

- `containerID`
- `containerType`
- `explorerSupportsExplicitRouteOwnership`
- `explorerSupportsSparklingContainer`
- `sparklingAvailable`
- `sparklingNavigation`
- `spkContainerID`
- `spkPipe`

A full Sparkling page receives the SDK-owned container identity, MethodPipe,
router, lifecycle, stable props, Explorer resource/image providers, XElement
registrations, native modules, and the local Lynx DevTool configuration.
`sparklingNavigation=true` means the current page has that capability; it does
not merely mean that the app binary links Sparkling. A Legacy page does not
register `spkPipe` or advertise Sparkling navigation.
`explorerSupportsExplicitRouteOwnership` instead describes the installed iOS
coordinator and remains true in Legacy-only builds. The separate
`explorerSupportsSparklingContainer` build capability controls whether the
homepage offers the **Sparkling Go** extension.

The `nav-basic` runtime fixture exposes the SDK-owned `lynxSdkVersion` value.
The Sparkling acceptance checks reject an empty value and the sentinels
`absent`, `unknown`, and `unavailable`. They also require the fixture's
`nav-xelement-input` probe to map to exactly one visible native
`XCUIElementTypeTextField`. Each Chrome DevTools Protocol (CDP) read binds the
visible native LynxView to one DevTool session by matching anchor text and
frame coordinates. These checks prevent a different page or stale session from
satisfying the capability assertions.

## Failure contract

Once the descriptor resolves to Sparkling:

- container creation or presentation failure is returned to the caller;
- the Sparkling lifecycle error view shows asynchronous load failures;
- `router.open` reports the coordinator's actual acceptance or typed error;
- `router.close` accepts an absent or empty target as the current container,
  accepts a nonempty ID only when it matches that container, and rejects an
  unknown ID without closing another page;
- the request is never retried through Legacy;
- Explorer never creates a plain `LynxView` as a Sparkling fallback.

Recent history is updated only after a route is accepted. Native module
callbacks are completed exactly once with a stable code and message. Router
service entries marshal their complete operation to the main thread before
reading UIKit state, including when MethodPipe requests current-thread
execution.

## Audited route entries

All current iOS entries terminate at the same coordinator:

| Entry | Source/policy |
| --- | --- |
| `lynx_initial_url` environment value | Highest-priority startup route |
| `UIApplicationLaunchOptionsURLKey` | Cold custom-URL launch |
| `application:openURL:options:` | Hot app-specific `lynx://open?url=<percent-encoded target>` transport; the original encoded wrapper is retained |
| `application:continueUserActivity:restorationHandler:` | Universal-link route from `webpageURL` |
| Homepage manual **Open** / **Sparkling Go** extension | Push with automatic or explicit Sparkling intent |
| Homepage recent row | Automatic **Open**; history changes only after acceptance |
| Homepage showcase/navigation helpers | Coordinator-backed Legacy route outside Sparkling; Sparkling router inside a full Sparkling page |
| QR scanner | Choice sheet exposes Legacy **Open** and **Open with Sparkling**; scanning resumes after cancel or failure |
| DebugBridge local route | `replaceTop` policy |
| DebugBridge remote route | `resetAndPush` policy |
| `ExplorerModule.openSchema` | Backward-compatible one-argument Legacy/automatic adapter |
| `ExplorerModule.openRoute` / `navigateBack` | Capability-detected, callback-driven container adapter |
| Sparkling `RouterService` open/close | Original scheme with explicit Sparkling intent and actual coordinator result; no Legacy conversion |
| Native navigation button and completed edge-back gesture | Coordinator-owned close; Sparkling's system gesture and Legacy's custom gesture reserve the same stack, the Legacy commit is atomic, and SDK fallback pop is always consumed |
| Recorder callback | Legacy Recorder route where supported |

The current iOS Explorer has no Scene manifest, `SceneDelegate`, or
`startFromUrl` entry. A future entry of any of those kinds must call the same
coordinator rather than add another URL parser.

Explorer intentionally does not claim the generic `hybrid` URL scheme. A third
party that uses ordinary LaunchServices wraps a canonical Sparkling URL in the
registered app-specific `lynx://open?url=...` transport (or uses a universal
link); unwrapping still forces the canonical target into Sparkling. The Appium
suite uses its targeted deep-link API to deliver the same app-specific wrapper
to Explorer's bundle ID; it does not register or directly deliver the generic
`hybrid` transport.

## Simulator smoke acceptance

`scripts/run_sparkling_smoke.sh` installs the selected app once, launches it
with `lynx_initial_url`, and uses `simctl openurl` twice through the real
LaunchServices path without another launch or terminate. The first hot URL is
a malformed canonical route. The second is a valid canonical route; both are
transported in the registered app-specific `lynx://open` wrapper.

The shell checks the scoped process log for the initial container, a typed
failure for the malformed route, and the expected result for the valid route.
It also requires a nonempty local Lynx version, keeps the app alive through both
hot deliveries, rejects a new duplicate-class diagnostic, and waits before
checking that the failed route did not fall back to another container. The
`+sparkling` mode expects two Sparkling successes and no Legacy success. The
no-`+sparkling` mode expects one Legacy success and explicit
`sparkling_unavailable` failures.

This smoke test covers LaunchServices delivery, route results, local Lynx
version logging, and the no-fallback contract. The Appium suite supplies the
DevTool-session and native XElement runtime evidence.

## Dependency modes and ownership

Sparkling is an explicit build mode and defaults to no `+sparkling`:

```bash
cd explorer/darwin/ios/lynx_explorer
./bundle_install.sh --sparkling-mode disable_sparkling
./bundle_install.sh --sparkling-mode enable_sparkling
```

The `+sparkling` mode materializes the official
[`tiktok/sparkling`](https://github.com/tiktok/sparkling) repository at commit
`5fa010cd2e669a73651a2e9321f28458ca3f6b2c` in an ignored generated directory.
That exact revision is the head of
[`tiktok/sparkling#113`](https://github.com/tiktok/sparkling/pull/113), pending
its inclusion in an official release.
It builds exactly `Sparkling`, `SparklingMacro`, `SparklingMethod`, and
`Sparkling-Router` from source alongside the local Lynx pods. The no-`+sparkling`
mode contains no Sparkling pod. `Sparkling-DebugTool`, `Sparkling-Media`, and
`Sparkling-Storage` are outside this phase-one dependency graph.

The `+sparkling` mode packages the official `packages/playground/dist/*.lynx.bundle`
artifacts built from that same pinned checkout. The entry bundle lives at
`Resource/extensions/sparkling-go/main.lynx.bundle`; sibling page bundles share
that namespace. Explorer does not maintain a fork of the Sparkling Go UI. The
official playground contains Media and Storage pages, but their native pods are
not linked in phase one, so those method calls remain unavailable.
No-`+sparkling` builds remove the generated resources so an old `+sparkling`
build cannot leak the extension into a universal-only artifact.

For a local `+sparkling` build, use Node 22 and pnpm 10.26.0, sync the pinned source,
then build its official playground before running `bundle_install.sh`:

```bash
python3 scripts/sync_sparkling_source.py \
  --manifest sparkling-source.json \
  --source-root generated/sparkling-source
pnpm --dir generated/sparkling-source install --frozen-lockfile
pnpm --dir generated/sparkling-source --filter sparkling-playground build
bash bundle_install.sh --sparkling-mode enable_sparkling
```

The `+sparkling` CI action performs the same sequence. The no `+sparkling` path does not
install Node or build Sparkling Go.

At the pinned revision, the selected `Sparkling`, `SparklingMethod`, and
`Sparkling-Router` podspecs do not constrain Lynx versions.
`Sparkling-DebugTool`, however, still constrains `Lynx`,
`LynxService/Devtool`, and `LynxDevtool/Framework` to `~> 3.9.0`; the ownership
verifier therefore rejects that subspec instead of allowing it to introduce a
released Lynx dependency.

`bundle_install.sh` runs the ownership verifier after CocoaPods installation.
It checks eight source-owned pods. `LynxLibraryRegistry` must resolve to
`generated/lynx-library`; the other seven must resolve to the current checkout
root. The verifier rejects a mismatched or dirty pin, a package-manager or
CocoaPods cache destination, a non-local source-owned Lynx pod, a forbidden
Sparkling pod, or a second Lynx owner. It never edits `node_modules`, a
downloaded podspec, or a CocoaPods cache.

Explorer pins the available official AnimaX 1.1.0 release. That release
packages an FML `ThreadConfig` declaration that is ABI-incompatible with the
current Lynx checkout. The Podfile therefore prepends the current Lynx root to
`HEADER_SEARCH_PATHS` for the AnimaX target only. AnimaX and LynxBase then
compile against the same canonical `base/include/fml/thread.h`; other targets
are unchanged. The artifact verifier fails closed unless AnimaX dependency
files prove that header owner and also rejects the incompatible constructor in
the link map. Remove this narrow override after AnimaX stops packaging the
duplicate FML header, then rerun both build modes and the artifact matrix.

The source override can be removed after an official Sparkling release includes
the APIs from `tiktok/sparkling#113`, has no version constraints on source-owned
Lynx pods, resolves against the current checkout without mutation, and passes
the dependency, Debug/Release link, and runtime ownership matrix.

## CI build matrix and release artifacts

CI and release build only what is shipped or what a downstream job consumes, so
no `.app` is released or exercised without a build check.

`publish-release.yml` ships **four** iOS Explorer apps -- `arm64`/`x86_64` x
`{no +sparkling, +sparkling}`, all Debug simulator:

| Artifact                                  | Arch   | Mode                        |
| ----------------------------------------- | ------ | --------------------------- |
| `LynxExplorer-arm64.app.tar.gz`           | arm64  | no `+sparkling` (original)  |
| `LynxExplorer-x86_64.app.tar.gz`          | x86_64 | no `+sparkling` (original)  |
| `LynxExplorer-arm64-sparkling.app.tar.gz` | arm64  | `+sparkling`                |
| `LynxExplorer-x86_64-sparkling.app.tar.gz`| x86_64 | `+sparkling`                |

`ci.yml`'s `ios-explorer-build` builds those same four (Debug simulator), so
every released app is guaranteed to compile and link. Why four, and why
`+sparkling` does not subsume the original:

- Routing is gated with `#if canImport(Sparkling)`. A `+sparkling` build compiles
  the `#if` branches; a no-`+sparkling` build compiles the `#else` branches a
  `+sparkling` build never sees. The no-`+sparkling` build is therefore not
  redundant -- it is the only thing that type-checks the `#else` path, and it is
  the original `.app` we still ship.
- `ios-e2e-test` downloads only the `arm64` Debug `+sparkling` build.
  `LegacyContainerLauncher` is compiled unconditionally, so that single
  `+sparkling` build also exercises the Legacy routing path at runtime -- which
  is why e2e needs just one build rather than one per mode.

CI builds exactly these four -- no Release-only or real-device jobs -- to keep
the shared macOS runner budget minimal: every job either ships an artifact or is
consumed by e2e.

## Upstream API dependency and removal conditions

The pinned revision exposes the permanent integration points proposed in
[`tiktok/sparkling#113`](https://github.com/tiktok/sparkling/pull/113):

- `SPKHybridSchemeParam.buildLynxPageScheme` owns canonical URL construction;
- `SPKContext.navigationBarBackHandler` lets the host coordinator own its stack
  after Sparkling emits the page-back and finish-back events;
- `SPKContext.interactivePopGestureDelegate` lets the same coordinator
  serialize cancellable system edge-back gestures without replacing UIKit's
  transition;
- `SPKContext.failedViewBuilder` lets Explorer provide the single load-error UI
  while Sparkling retains failure lifecycle and retry ownership.

Explorer uses those public APIs directly. It does not replace Sparkling's
navigation bar, reproduce canonical syntax, or implement a parallel load-error
lifecycle/overlay. Its load-error view registers the SDK-provided refresh block,
so Retry follows Sparkling's reload contract. When the pull request lands,
advance the exact pin to the first containing official revision, rerun
parser/encoding tests, `+sparkling` Debug and Release builds, ownership verification,
and the navigation smoke test, and then prefer the first release containing
that revision. The semantic route model and coordinator remain unchanged across
that upgrade.
