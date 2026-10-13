# Optional Linux GTK4 host — design draft

This layer embeds Lynx/Clay content in a GTK application without depending on
Lynxtron or Node. GTK owns native widgets and the application main context;
Lynx/Clay owns rendering, input routing and engine lifetime. Lynxtron separately
owns top-level windows, Node integration and desktop services.

## Review order and dependencies

1. Skity BGRA span fix: https://github.com/lynx-family/skity/pull/514,
   reviewed head `d723a4552c4cb21154611b1d4aae08a977e71acc`.
2. This Lynx/Clay host draft.
3. Lynxtron desktop integration: https://github.com/lynx-family/lynxtron/pull/331,
   reviewed head `6586be3c272dad36401f0000b116bd1146cb1ba8`.

These are review references, not automated dependency rolls. The published
Lynxtron pin does not include this host; no clean-checkout integrated build is
claimed. Existing public GN linkage work also needs coordination (Lynx #7284,
Skity #376). Dependency roll/patch reconciliation is still pending.

## Responsibilities and interfaces

- `clay/shell/platform/linux/gtk/GtkSurface` owns the software/GL widget and
  cross-thread frame presentation. Software frames are immutable pixel copies;
  GL texture storage is shared between producer and consumer GDK contexts.
- `platform/linux/gtk/View` owns the embedder renderer, pointer/keyboard/text
  input bridges, GLib task dispatch and attachment to a GTK container.
- `lynx_gtk_view_capi.h` exposes opaque view handles: initialize, create,
  attach/detach, load copied template bytes and destroy. Borrowed renderer/widget
  handles remain owned by the view. A successful load call means accepted,
  not that asynchronous loading has completed.
- Initialization and view mutation require the owner UI thread. Per-view queued
  callbacks use weak references; disposal stops engine work before releasing
  GTK and rendering resources. Reparenting moves the complete host container.

## Rendering and lifecycle choices

GTK remains opt-in with `enable_lynx_gtk=true`; full-profile headers/runtime
require GTK/GDK 4.22+. `lynx_gtk_legacy_rendering_probe` is an explicit historical
probe, not a supported release profile. Skity and Fontconfig provide Linux
rendering and font matching. GL requests desktop GL 3.3 sharing; initialization
failure chooses software before engine construction.

Three texture slots bound storage: producer write, replaceable pending frame
and retained front frame. Each context owns its FBO while sharing textures.
Producer/consumer fences protect ownership and slot reuse; resize reallocates a
writable slot. This trades unbounded frame queues for bounded/latest-frame
presentation. Recovery and fallback behavior needs fresh review and validation.

The native-child plane uses GtkOverlay/GtkFixed. Empty overlay regions pass
input to the rendering surface. Full hybrid composition is not advertised:
there is no integrated PlatformOverlayService/PresenterService, so embedding
applications must suppress native children beneath overlapping Lynx overlays.
IME/selection/clipboard callbacks bridge GTK state to Clay without a second
blocking main loop. Native UI and frame notifications use UI task priority to
avoid starvation under native-widget redraw.

## Current review and validation gaps

The prototype was extracted onto current develop. Upstream now has newer touch
handler and SHM implementations; those were preserved rather than replaced by
older experimental refactors. Multi-view touch-state isolation, partial cancel,
template-reset semantics and SHM representation compatibility must be reconciled
and tested against this exact draft. Event-through hit-test changes from the
older prototype are also deferred for review against upstream overlay routing.

Scope/local-path/binary and conflict scans, patch checks and source formatting
were performed. No complete native build, runtime suite or other-platform
regression has passed for this extracted commit. Older ARM64/GTK4/llvmpipe
experiments do not attest this head. Hardware GPU, x64/distribution packaging,
complete IME, accessibility, long-run testing and full hybrid composition remain
pending. Local VM scripts, generated binaries, logs and screenshots are excluded.

Standalone smoke/lifecycle/task-runner fixtures are included as source targets;
they require an embedder-configured GN workspace and complete dependency setup.
They are not a turnkey distribution build or Linux release acceptance.
