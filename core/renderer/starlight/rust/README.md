# Rust Starlight

This workspace is the Rust-side Starlight layout effort.

The completion target is Rust standalone and C++ standalone interchangeability.
Runtime hot switching between layout engines is not required for that target.

The engine is intentionally adapter-based. `LayoutEngine` does not own tree
nodes; embedders implement `LayoutTree` for their existing tree storage and the
engine writes `LayoutResult` values back through that trait. This keeps DOM,
shadow, and test trees outside of the layout engine's ownership model.

`core/renderer/starlight/BUILD.gn` exposes a test-only
`starlight_rust_tests` bridge to this workspace. The bridge runs
`cargo test --workspace --quiet` and
`cargo clippy --workspace --all-targets -- -D warnings` through GN actions with
Cargo artifacts redirected into the GN output tree. It also exposes
`starlight_rust_native_parity_tests` for the source-built C++ standalone
handwritten head-to-head suite, generated matrix suite, and Rust standalone
owned-tree head-to-head suite,
`starlight_rust_cpp_import_tests` for the C++ standalone import contract
checks over headers, enum/value ABI, source lists, latest-mode config, and
Cargo rebuild inputs,
`starlight_rust_ffi_tests` for building the Rust FFI staticlib and running the
C/C++ ABI compile/link/run smoke tests against that artifact,
`starlight_rust_benchmarks` for the release Rust-vs-C++ performance gate, and
`starlight_rust_full_tests` for all of those checks together.
The GN benchmark action explicitly sets `STARLIGHT_BENCH_MIN_SPEEDUP=1.0`, so
the imported C++ baseline must be available and every benchmarked Rust scenario
must be faster than that baseline. A GN bridge guard checks the scoped
benchmark action body so the source-built C++ baseline requirement, release
mode, stable workload, and full-test dependency cannot drift independently.
`starlight_rust_ffi` builds the `starlight_ffi` Rust crate as a
release staticlib artifact in the GN output tree, and `starlight_rust_ffi_link`
explicitly lists the public C and C++ FFI headers while publishing their include
path and generated staticlib through a GN link config. The production
`starlight` GN target does not link or switch to Rust; the Rust engine is exposed
as standalone crates plus public C/C++ FFI for external callback-backed trees.
Platform `LayoutObject`/`LayoutComputedStyle` import glue is intentionally out
of scope; the only C++ entry path is the standalone external-tree FFI.

The public FFI exports stable status-name strings and buffers layout writebacks
during the Rust pass, flushing them to external callbacks only after the Rust
engine returns successfully. This preserves callback order and
`set_layout_with_constraints` writes while keeping failed Rust attempts from
partially mutating the external tree. If an external tree marks a node as
measured but its measure callback fails or returns a non-finite size, or if a
baseline callback claims success with a non-finite baseline, the FFI reports
`SLRustStatusInvalidTree` before flushing any layout results; non-finite
baseline content sizes are rejected before calling the external baseline
callback. The same pre-flush guard rejects non-finite root layout sizes and
non-finite pending layout writeback fields or constraint cache keys produced by
overflow in otherwise finite inputs. Oversized external snapshots, including
unbounded child counts, report `SLRustStatusUnsupportedTree` before the Rust
snapshot builder can grow without a hard limit. Callback-backed trees also map
`child_count`/`child_at` mismatches to an invalid-tree signal instead of a
missing-style failure, keeping diagnostics focused on tree consistency.

Current scope:

- `crates/starlight_layout`: pure safe-Rust layout engine and shared layout
  types.
- `crates/starlight_standalone`: safe Rust owned-tree standalone API for
  future standalone migration. It keeps standalone tree mutation, parent/child
  ownership checks, child-count/child-at/parent/RTL queries, public standalone
  reparenting on child insertion, an `Option<usize>` insert-or-append helper
  for the public C standalone index/append insertion contract, public
  standalone latest-mode default style differences (`flex`/`relative`/`content-box`),
  measured leaf support with static sizes, safe Rust measurement callbacks,
  and safe Rust content-size baseline callbacks, standalone physical-pixel
  config storage that drives
  measured-size ceil and exported layout rounding, and tree-level
  measured-size/baseline mutation APIs with dirty propagation, including the
  C++ standalone rule that fixed-width/fixed-height measured nodes skip the
  measure callback while auto measured nodes still measure under definite
  owner constraints. It also keeps
  C-compatible edge, gap, dimension, flex/alignment including the
  standalone `flex` shorthand, linear, relative, grid placement, track-vector,
  and flex-basis style setters and getters plus layout getters for position,
  size, baseline, box edges, and sticky insets. A Rust standalone guard parses
  the public C standalone header and requires every style setter/getter family
  to have a safe `StandaloneTree` API, so future C standalone style-surface
  additions cannot silently miss the Rust standalone layer. A second guard
  covers the public non-style standalone API surface for node creation,
  mutation/query, layout entry points, measure delegates, layout getters, and
  config functions, with explicit Rust ownership exemptions only for C free
  functions. It keeps
  owner-constraint layout entry points outside the C++ container model while
  delegating actual layout to `starlight_layout::LayoutTree`, including
  standalone owner-direction inheritance for nodes without an explicit
  direction, a safe API to clear an explicit direction override and restore
  owner-direction inheritance, while restoring Rust-owned style state after layout. It also tracks
  standalone dirty/reset state so migrated `SLNodeIsDirty`, `SLNodeMarkDirty`,
  and `SLNodeReset` tests have a Rust-owned API surface, including C++-matching
  clean-on-create dirty state, signed-index child insertion/query semantics,
  optional-reference insert-before append/reorder/reparent behavior, reparent
  dirty-state behavior, reset behavior that clears children, layout, and
  measurement data while leaving the reset node clean, plus C++-matching no-op
  removal when the child is not attached to the supplied parent. The imported
  C++ standalone crate also exposes a safe public tree-mutation transcript
  probe that validates Rust standalone structure and dirty-state snapshots
  directly against `SLNodeInsertChild`, `SLNodeInsertChildBefore`,
  `SLNodeRemoveChild`, `SLNodeRemoveAllChildren`, and `SLNodeReset`, plus a
  safe public dirty-state transcript probe that validates explicit
  `SLNodeMarkDirty` propagation and clean-layout reset behavior directly
  against C++ standalone, plus a
  safe public edge-style transcript probe that validates `SLEdge`
  Start/End/Horizontal/Vertical/All setter/getter behavior and `SLGapAll`
  getter behavior directly against C++ standalone, plus a safe public
  edge-style variant transcript probe that validates point, percent, calc,
  `fr`, max-content, `fit-content()`, and auto public position/margin/padding/gap
  setter entry points directly against C++ standalone, plus a safe public
  edge/gap layout-effect transcript probe that validates point, percent, calc,
  auto, `fr`, max-content, and `fit-content()` public position, margin,
  padding, and gap setter entry points through exported flex-wrap layout
  results directly against C++ standalone, and a safe public
  scalar-style transcript probe that validates scalar and enum style
  setter/getter behavior directly against C++ standalone, plus a safe public
  dimension-style transcript probe that validates width, height, min/max
  width/height, and flex-basis `StarlightValue` getter/setter behavior
  directly against C++ standalone, plus a safe public dimension-style variant
  transcript probe that validates point, percent, calc, `fr`, auto,
  max-content, and `fit-content()` public dimension/flex-basis setter entry
  points directly against C++ standalone, plus a safe public dimension
  layout-effect transcript probe that validates point, percent, calc, auto,
  `fr`, max-content, and `fit-content()` public flex-basis, width/height, and
  min/max dimension setter entry points through exported flex layout results
  directly against C++ standalone, plus a safe public direction transcript
  probe that validates `SLNodeIsRTL` strict-RTL query behavior for default,
  RTL and LTR public direction setters directly against C++
  standalone, plus a safe public direction layout-effect transcript probe that
  validates LTR and RTL public direction setters through row flex and
  logical start/end margin exported layout results directly against C++
  standalone, plus a safe public layout-getter transcript
  probe that validates exported size, offset, baseline, box-edge, and sticky
  result getters directly against C++ standalone, plus a safe public
  box-sizing/aspect-ratio layout-effect transcript probe that validates
  content-box, border-box, aspect-ratio, and min-height clamp public setters
  through exported layout results directly against C++ standalone, plus a safe public display
  layout-effect transcript probe that validates `none`, `block`, `flex`,
  `linear`, `relative`, and `grid` public display setters through exported
  layout results directly against C++ standalone, plus a safe public
  position-type layout-effect transcript probe that validates relative,
  absolute, fixed, and sticky public position-type setter entry points through
  exported layout results directly against C++ standalone, plus a safe public
  relative layout-effect transcript probe that validates every `relative_center`
  public setter value, parent/sibling relative alignment, before/after sibling dependencies, and
  `relative_layout_once` through exported layout results directly against C++
  standalone, plus a safe public flex
  layout-effect transcript probe that validates every `flex-direction`
  public setter value, every `flex-wrap` public setter value, every `justify-content`
  public setter value, every supported `align-items` public setter value, multi-line
  align-content distribution covering every public setter value plus start/end
  aliases, every `align-self` public setter
  value including explicit auto inheritance, measured-leaf `align-items: baseline` and
  `align-self: baseline` callback alignment, order, grow, shrink, shorthand
  flex, and point, percent, and calc flex-basis public setters through exported
  layout results directly against C++ standalone, plus a safe public linear layout-effect
  transcript probe that validates every linear-orientation public setter value
  including horizontal/vertical/reverse and row/column aliases, every linear gravity, linear layout gravity, and
  linear cross gravity value across horizontal and vertical containers, linear
  weight ratio distribution, vertical weight sizing, explicit weight-sum
  unallocated space, total-weight-below-one unallocated space, and linear
  weight sum public setters through
  exported layout results directly against C++ standalone, plus a safe public
  linear/list layout-effect transcript probe that validates
  `SLNodeStyleSetLinearColumnCount`, every list-component-type public setter
  value, and linear-list gap public setters through exported layout results directly
  against C++ standalone, plus a safe public list-gap layout-effect transcript
  probe that validates point, percent, calc, auto, `fr`, max-content, and
  `fit-content()` public main/cross-axis list-gap setter entry points through
  exported layout results directly against C++ standalone, plus a safe public grid track layout-effect
  transcript probe that validates grid template/auto track vector, max-vector,
  gap, placement, and every grid auto-flow public setter value through exported layout results directly
  against C++ standalone, plus a safe public grid alignment layout-effect
  transcript probe that validates grid `justify-content`, `align-content`,
  `justify-items`, `align-items`, `justify-self`, and `align-self` public
  setter variants through exported layout results directly against C++ standalone, and a safe public config
  transcript probe that validates `SLConfigCreate`, `SLConfigGetPhysicalPixelsPerLayoutUnit`,
  `SLConfigSetPhysicalPixelsPerLayoutUnit`, and `SLNodeNewWithConfig` rounding
  behavior directly against C++ standalone, plus a safe public
  measure-delegate transcript probe that validates measure delegate round-trip,
  `SLNodeHasMeasureFunc`, callback inputs, baseline export, and delegate
  clearing directly against C++ standalone, plus a safe public layout
  entrypoint transcript probe that validates `SLNodeCalculateLayout` finite
  owner constraints, the public `SLUndefined` owner-size sentinel, and
  `SLNodeCalculateLayoutWithMode` AtMost/Undefined owner mode preservation
  directly against C++ standalone. Its Rust-owned standalone
  head-to-head coverage now
  exercises measured flex rows, flex wrap alignment and AtMost cross-axis sizing,
  row/column wrap and wrap-reverse `AlignContent` enum mapping,
  wrap-reverse RTL row-reverse placement,
  `FlexDirection` by LTR/RTL direction mapping,
  flex `JustifyContent` enum mapping, row/column `AlignItems` enum mapping,
  row/column `align-self` enum mapping, main-axis auto margins and align-self
  overrides, wrapped `align-self:baseline` margins and zero-baseline fallback,
  flex display-none/grow/order,
  flex min-width shrink freeze and max-width grow-space redistribution,
  auto-sized roots, tree-mutation insertion/reparent layout order,
  mutation re-layout after a clean layout with measured-size updates,
  RTL owner-direction inheritance,
  physical-pixel measured rounding, public size setters,
  measurement/baseline callbacks, measured flex `max-content` and exact-size
  items, direction-aware grid positioning and fixed-descendant setters,
  sticky percent/calc inset export and in-flow positioning,
  relative-position visual offsets and flex percent offset resolution,
  static-position offset suppression and absolute right/bottom end-inset
  positioning,
  absolute measured auto-size single-inset constraint stripping and paired
  point-inset fill-available sizing,
  absolute measured explicit percent sizing including border-box measured
  children,
  absolute measured percent/calc fill-available sizing with in-flow siblings,
  absolute flex initial alignment including negative free-space, RTL fronts,
  and wrap-reverse placement, absolute linear initial alignment including RTL
  main-front placement,
  fixed descendant root-containing-block point/percent/calc inset resolution
  and root padding-box offset placement, fixed measured percent/calc
  fill-available sizing against the root containing block, and absolute/fixed
  `fit-content()`/`max-content` subtree and measured natural sizing,
  fixed measured aspect-ratio sizing against the root containing block,
  content-box and border-box aspect-ratio sizing plus block/flex auto
  cross-size derivation,
  latest-mode negative padding clamping, negative margin preservation, and
  percent edge resolution, vertical percent spacing width-base resolution,
  calc padding/margin/position edge resolution,
  relative calc end offsets, absolute block removal from normal flow,
  absolute edge export with root padding/border and own margins,
  block vertical stacking with ordered and
  display-none children, measured block leaf definite/AtMost/min-max
  constraint handling including explicit min/max branches under indefinite and
  definite-width constraints, block fit-content measured callback and baseline
  children, wrapped flex measured callback baseline export and fit-content
  measured callback container width, grid `justify-self`/`align-self`
  item-alignment enum mapping across LTR/RTL including inherited
  container defaults,
  grid `justify-content`/`align-content` content-alignment enum mapping across
  LTR/RTL,
  grid LTR/RTL auto-margin alignment and used-margin export,
  grid fit-content/max-content intrinsic track setters, row/column calc/percent
  fit-content minmax track caps, auto-track calc/percent fit-content max caps,
  spanning max-content intrinsic tracks, and row/column dense RTL
  auto-flow placement plus display-none, order, later locked-line auto-placement,
  and column auto-flow cursor-retention setters,
  negative line and leading implicit track setters, relative dependency ordering,
  centering, sibling-edge stretch, duplicate-id, and display-none anchor
  setters, root/child relative fit-content wrap-content sizing, and
  vertical/horizontal-reverse staggered
  linear-list column-count/gap/component-type setters including measured-child
  constraint propagation, plus linear display-none/order stack,
  AtMost main-axis sizing with non-definite weight suppression,
  AtMost cross-axis stretch suppression, measured cross-axis constraints,
  weight/gravity, weighted child min/max freeze
  redistribution across horizontal/vertical point and percent constraints,
  `weight_sum` and total-weight-below-one unallocated-space behavior,
  vertical `LinearGravity` enum mapping, horizontal gravity override of
  `justify-content`, RTL horizontal physical left/right gravity, full
  vertical/horizontal `LinearLayoutGravity` enum mapping including RTL
  physical left/right and stretch overrides of explicit and weighted cross
  sizes, full vertical/horizontal `LinearCrossGravity` enum mapping, and
  cross-gravity auto-margin/baseline setters against the imported C++
  standalone baseline.
- `crates/starlight_cpp`: Rust API boundary for importing the existing C++
  Starlight baseline. The `native-standalone` feature contains a safe public
  wrapper over the repository's `starlight_standalone` C API for the block,
  flex/box, fixed/sticky positioning, initial linear-layout, initial
  relative-display, and initial grid subsets, including measured baseline
  callbacks, baseline result export, sticky inset result export, aspect-ratio
  setter coverage, linear display/orientation/gravity/weight setters,
  staggered list column-count, list component type, and full `StarlightValue`
  list-gap setters including raw point/percent/calc, auto, `fr`, max-content,
  and `fit-content()` values, relative dependency setters, grid display,
  track-vector, auto-flow, line/span, justify-items/self setters,
  point/percent/calc edge, full `StarlightValue` position/margin/padding edge,
  row/column gap, size setters, and full `StarlightValue`
  width/height/min/max/flex-basis setters including `fr`,
  grid track value encoding for point, percent, auto, max-content, fit-content,
  fr, and calc values, and standalone width/height `max-content` and
  `fit-content` setters including length arguments, min/max width/height
  no-value intrinsic and `fit-content()` length-argument setters, flex-basis
  intrinsic and `fit-content()` length-argument setters, start/end alignment
  and justification enum mapping, align-content start/end alias mapping,
  layout-mode enum mapping guards, a
  `Style`-field native apply/validation coverage guard, a full native extern
  import guard against the public standalone header function surface including
  config, tree mutation/query, measurement delegate, style setter/getter, and
  layout getter APIs, source-built native API smokes that actually call those
  config/tree/query/measure/style getter paths including detached-node
  `SLNodeFree`, a source guard requiring every public non-style standalone
  function to be exercised outside the extern declarations, native baseline mirroring of
  per-node `LayoutTree` physical-pixel-ratio hooks, Rust-vs-C++ physical-pixel
  config measured rounding including reset-preserved config, signed-index
  insert/query, optional-reference insert-before, owner-direction inheritance
  for native standalone subtrees without explicit direction, standalone
  measurement/baseline callback import, exact fixed-size measure-delegate skip
  behavior, reset measure-delegate/context clearing, dirty/detach, remove
  no-op/remove-all, and style getter/setter round-trip behavior checks, an explicit
  `unsafe_op_in_unsafe_fn` glue-boundary lint, plus mode-aware root constraints
  for `at-most` parity cases, a public standalone style getter guard that
  requires parity snapshots to declare and consume every exported style getter,
  and a public standalone layout result getter guard that requires native
  head-to-head readback to declare and consume every exported layout result
  field.
  The source-built
  standalone baseline is configured in latest mode with
  W3C-aligned display handling and fixed-new root propagation for parity, not
  historical quirks mode.
  Its build script can either build the standalone C++ baseline from repository
  sources with `STARLIGHT_CPP_NATIVE_STANDALONE_BUILD_FROM_SOURCE=1` or link a
  prebuilt standalone native library when `STARLIGHT_CPP_NATIVE_STANDALONE_LINK_LIB`
  and optional `STARLIGHT_CPP_NATIVE_STANDALONE_LIB_DIR` are provided; without
  those env vars the feature reports `NativeLinkUnavailable` instead of failing
  the default Rust build.
- `crates/starlight_ffi`: C ABI boundary for calling the safe Rust engine from
  C/C++ over an external callback-backed tree. It mirrors the `LayoutTree`
  trait with child/style/measure/baseline/physical-pixel-ratio/set-layout
  callbacks, so C++ callers can keep tree ownership outside Rust while the Rust layout core remains
  `unsafe`-free, including flex, linear, and grid display-none child skipping
  with zero-size exported layouts at the C++ parent-padding-bound origin,
  margin/padding/border and sticky inset result
  export, absolute/fixed positioning result export across external callback
  trees including measured auto-size constraint stripping and fill-available
  sizing with paired percent/calc insets, calc padding/position edge mapping,
  content-size baseline callback input, nested flex/linear baseline propagation
  and nested grid fallback baseline alignment through external callback trees, grid explicit
  placement/self-alignment field mapping including container alignment overrides, relative dependency field mapping,
  grid auto-margin alignment override mapping with used-margin export,
  staggered linear reverse main-axis offset export and owner-indefinite
  measured-child natural sizing through external callback trees,
  min/max-adjusted measured callback constraints with C++-style AtMost
  non-clamping, non-finite style/constraint scalar validation,
  style/style-data enum mapping coverage guard including align-content
  start/end alias values, C++ `LayoutObject` style-converter guards
  that require every layout-facing C++ style and list-attribute enum variant to
  be explicitly handled, C++ `NLengthType` conversion coverage, bidirectional
  C++/Rust measure-mode constraint conversion coverage, public
  `SLRustStyle` field
  coverage guard against `Style`, `SLRustStyle` conversion consumption and
  default-initialization guards, explicit-direction metadata propagation from
  `LayoutObject::HasExplicitDirectionStyle()` through the callback snapshot,
  and owner-direction-aware external-tree entry points that apply
  standalone-style direction inheritance to unset callback-backed nodes,
  header-only `LayoutObject` style conversion smoke/field coverage guards,
  and null
  entrypoint pointer, invalid style enum, and null track-vector pointer
  validation through the callback bridge. Callback-backed trees can optionally
  provide per-node physical-pixels-per-layout-unit values for measured-size
  ceil and exported layout rounding, defaulting to the standalone value `1.0`.
  Its public C header is guarded by
  C/C++ compile smoke tests, C and C++ link/run smokes that call the Rust FFI
  entry points when the library artifact is available, including a C++
  external-tree measured-child final constraint and baseline callback path,
  linear baseline and cross-axis auto-margin result path,
  staggered linear reverse main-axis result path, owner-indefinite staggered
  measured-child natural sizing path,
  flex row/column full-value gap paths for `fr`, max-content, and
  `fit-content()` lengths,
  full-value position/margin/padding edge paths for `fr`, max-content, and
  `fit-content()` through callback-backed external trees, public C linked
  smoke, and the public C++ adapter,
  grid track-vector, percent/calc track-vector, fr/max-content track-vectors,
  fixed and percent/calc argument fit-content column/row track-vectors,
  column/row min/max track-vectors including fit-content max caps,
  auto-track min/max column/row vectors including fit-content max caps and
  owner-indefinite intrinsic growth through callback-backed external trees
  and the public C++ adapter,
  RTL inline placement horizontal linear physical-gravity swapping
  through Rust parity, public C++ smoke, and native C++ head-to-head coverage,
  absolute/fixed grid-area containing blocks,
  explicit placement/self-alignment,
  and auto-margin used-margin result paths, sticky calc inset result path,
  relative dependency result path, absolute/fixed out-of-flow result paths,
  absolute/fixed measured fill-available constraint/result paths including
  paired percent/calc insets, grid display-none skip/zero-size result path,
  staggered linear auto, `fr`, and `fit-content()` raw list-gap constraint
  paths,
  null-pointer, invalid-style, disabled-layout, unsupported-tree guard,
  fixed-node-set mismatch fallback status,
  ABI-mismatch status returns, stable status-name diagnostics,
  ABI version, size, and alignment metadata checks
  plus a header-only caller ABI helper for the checked layout entry point,
  header-only length, size, rect,
  and constraint constructors for C/C++ glue callers, a header-only C++ external
  tree adapter that turns C++ tree methods into `SLRustTreeCallbacks`, validates
  the required `ChildCount`/`ChildAt`/`Style`/`SetLayout` methods at compile
  time, supports optional measurement and baseline methods, rejects C++ trees
  that provide only one half of the `HasMeasure`/`Measure` pair, forwards an
  optional per-node physical-pixels-per-layout-unit method for rounding parity,
  explicit owner-constraints and node-constraints layout entry points,
  snapshot-local cached layout readback for public flex stretch subtree
  re-export,
  Rust-vs-header `repr(C)` struct
  field-order checks, and exhaustive public enum value checks.
- `crates/starlight_parity`: translated Rust tests and head-to-head parity
  harness, including layout size, offset, edges, sticky insets, and baseline
  field comparison, plus a workspace guard that keeps `unsafe` confined to
  the FFI glue crates while requiring safe Rust crate roots to use
  `#![forbid(unsafe_code)]`, and a current C++ `starlight_testset` inventory
  guard that requires every translated C++ gtest entry to point at an existing
  Rust test function, plus public standalone layout variant matrix count guards
  for enum and `StarlightValue` families that can otherwise be missed symmetrically by both
  the imported C++ probe and Rust mirror. Its generated deterministic head-to-head fuzz includes
  linear list-gap coverage for point, percent, calc, auto, `fr`, max-content,
  and `fit-content()` values, and its native handwritten suite covers
  row/column `fr` and intrinsic gap values against the source-built C++
  baseline. Its GN bridge guards keep the Rust workspace exposed through
  standalone test/build groups while asserting that the production `starlight`
  target does not depend on Rust FFI or runtime switch glue.
- `crates/starlight_bench`: release-mode, multi-scenario benchmark entry point
  for Rust vs C++ layout performance comparisons. When the imported C++
  baseline is linked it fails if any supported scenario does not exceed the
  configured Rust-over-C++ speedup threshold, defaulting to faster-than-C++,
  including the staggered linear list, a staggered linear raw/intrinsic
  list-gap case covering auto, `fr`, max-content, `fit-content()`, and list
  component rows/default regular items,
  an at-most owner constraint matrix covering block, flex, linear, grid, and
  relative containers under two-axis `AtMost` root constraints with
  percent/calc min/max, `fit-content()` sizing, and measured leaves,
  a standalone owner-direction inheritance scenario that compares inherited
  RTL owner direction against explicit LTR rows through the Rust and imported
  C++ standalone paths,
  linear gravity, layout-gravity, and cross-gravity matrices covering every
  `LinearGravity`, `LinearLayoutGravity`, and `LinearCrossGravity` value across
  orientation and LTR/RTL direction,
  a flex axis alignment matrix covering every `FlexDirection`, every
  `JustifyContent`, every `AlignItems`, and LTR/RTL direction with
  auto cross-size children,
  a flex distribution matrix covering grow, shrink, explicit/percentage
  `flex-basis`, positive and negative `order`, point and percentage min/max
  freeze redistribution, every `FlexDirection`, and LTR/RTL direction,
  a flex wrap alignment matrix covering every `FlexWrap`, every
  `AlignContent`, every `AlignItems`, every `FlexDirection`, gap handling, and
  LTR/RTL direction,
  an in-flow child ordering matrix covering negative, zero, and positive
  `order` values across block, flex, linear, and grid containers with
  LTR/RTL direction and row/column grid auto-flow,
  a full-value spacing matrix covering margin, padding, border,
  relative position offsets, row/column gaps, and linear list gaps across
  block, flex, linear, and grid containers with point, percent, calc, auto,
  `fr`, max-content, and no-argument, fixed-argument, and percent/calc
  `fit-content()` values,
  a measured callback matrix covering both static measured leaves and real
  Rust function-pointer measurement/content-baseline callbacks across block,
  flex, linear, grid, and relative containers with `fit-content()` sizing and
  min/max constraints,
  a baseline propagation matrix covering measured leaf baselines and nested
  flex, linear, grid, and relative baseline sources through both container
  `align-items: baseline` and child `align-self: baseline` triggers,
  aspect-ratio block, a block/flex/linear/relative/grid box-sizing matrix with
  content-box and border-box aspect-ratio plus min/max sizing, measured flex
  baseline alignment, and mixed position scenarios, a mixed
  static/relative/absolute/fixed/sticky `PositionType` matrix with
  percent/calc insets, a mixed
  block/flex/linear/relative/grid `fit-content()` subtree scenario, a relative
  dependency graph scenario, a relative-center matrix covering all
  `RelativeCenter` values with parent-edge alignment and measured children, a sticky percent-inset
  scenario across flex, linear, grid, and relative containers, a mixed
  display-none scenario across the same container families, plus out-of-flow
  intrinsic sizing and paired percent/calc fill-available scenarios that
  exercise absolute/fixed `max-content`, `fit-content()`, measured auto-sized
  subtrees, and inset resolution in block and grid containers, a grid
  absolute/fixed grid-area containing-block scenario with LTR/RTL
  containers,
  augmented auto lines, last-line-to-auto-end placement, fill-available
  insets, measured alignment, and subtree/measured `fit-content()`, a grid item
  alignment matrix covering every `JustifyItems` value including `Auto`, every
  `AlignItems` value, `justify-self`/`align-self` overrides, and LTR/RTL,
  a grid content alignment matrix covering every `JustifyContent` and
  `AlignContent` value across extra-space and overflow track groups plus
  LTR/RTL,
  a grid auto-flow matrix covering row, column, dense, row-dense, and
  column-dense placement across LTR/RTL with spans, locked lines,
  implicit lines, and display-none children,
  a grid auto-margin alignment scenario with alternating LTR/RTL containers and
  measured grid items,
  minmax/intrinsic track scenario with measured spanning items, `max-content`,
  `fit-content()`, and `fr` track limits, and a grid auto-track
  `fit-content()` max-limit scenario covering definite percent/calc caps and
  owner-indefinite fixed caps. Its unit tests guard that the benchmark
  table keeps required coverage for block, flex, linear, relative, grid,
  out-of-flow, sticky, intrinsic sizing, measured content, `fit-content()`,
  minmax, percent/calc, display-none, aspect-ratio, box-sizing, baseline,
  baseline propagation,
  alignment, flex axis alignment, flex distribution, flex wrap alignment, grid item alignment,
  grid content alignment, grid auto-flow, auto-margin, direction,
  linear-gravity/layout-gravity/cross-gravity, in-flow ordering,
  full-value spacing, measured callbacks, owner constraints, owner direction,
  position-type, relative-center, list-component, and staggered-linear
  feature families, and feature-gated
  tests require every benchmark scenario to be accepted by the
  imported C++ baseline whenever that native baseline is available.
- Shared layout primitives: constraints, lengths, edges, sizes, and results.
- Layout-facing style data for latest-mode block-as-linear, flex, linear, and
  grid layout.
- `LayoutTree` trait for external tree adapters, including optional content
  measurement and content-box baseline callbacks, with generated native
  head-to-head coverage across block, flex, linear, relative, and grid
  containers for measured leaves, baseline callbacks, min/max clamps, and
  border-box aspect-ratio measurement paths, plus a pure Rust external-tree
  integration test using a custom non-`usize` node id, adapter-owned child
  storage, per-node physical pixel ratio, and constraint-aware layout
  writeback, plus a write-only minimal adapter test that implements only the
  required `children`/`style`/`set_layout` trait methods and relies on default
  callbacks, plus an internal cache re-export guard that requires the engine to
  read cached descendant layouts through `LayoutTree::layout`, including the
  public flex stretch reuse path.
- `SimpleTree` test/helper implementation.
- Shared box sizing behavior including content-box and border-box
  aspect-ratio/explicit-size resolution and flex min/max clamping, C++ leaf
  sizing under at-most constraints, and measured leaf sizing under definite
  constraints, including min/max-adjusted measure constraints and C++-style
  AtMost measured callback constraints that do not clamp the callback result
  unless min/max does, C++ measured callback content-size ceil behavior on
  non-definite axes, plus measured root `fit-content()` percent/calc owner-constraint
  caps and block-as-linear fixed/percent/calc `fit-content()` latest natural
  sizing, C++ edge-difference pixel-grid size export, and raw
  fallback baseline export when non-measured layout rounding changes fallback height, with
  generated native head-to-head coverage for root percent/calc sizing,
  root and subtree `fit-content()` sizing, content-box and border-box
  percent/calc min/max constraints, content-box and border-box aspect-ratio
  roots, and intrinsic measured children across block, flex, linear, relative,
  and grid containers.
- Latest-mode `display: block` dispatch through Starlight's linear layout
  behavior, and flex layout with grow/shrink, order, growing and shrinking flex
  targets that define percent main-size child bases, growing targets that define
  combined percent flex-basis/main-size child bases, aligned, local-inflexible,
  and local-flexible growing percent-basis targets that define child basis
  bases, percent main-length and fixed flex-basis parents that define growing
  percent-basis child bases, own percent-basis/percent-main-size parents that
  define percent child bases, shrunk-below-base parents that define percent
  flex-basis child bases, oversized inflexible fixed-basis siblings, max-target
  inflexible percent-basis child base resolution, min/max-target and
  unchanged-main stretch/inflexible percent flex-basis descendant base
  resolution, preserved
  percent-basis parents with growing
  percent-basis children and inflexible percent-basis/main-size children,
  explicit no-wrap,
  wrapping, wrap-reverse, auto-main flex items preserving intrinsic
  percent-basis children, align-self including start/end, align-content
  including start/end aliases plus explicit flex-end/stretch mapping, gaps,
  justify
  including start/end, stretch, padding, border, margin including full-value
  `fr`, max-content, and `fit-content()` edge lengths, percentages, AtMost
  main-axis shrink-to-fit and wrapped
  largest-line sizing, latest-mode cross-axis AtMost non-clamping,
  direction-aware LTR/RTL main-axis and column cross-axis placement,
  generated native head-to-head axis alignment coverage across every
  `FlexDirection`, LTR/RTL direction, distributed `JustifyContent`
  value, stretch/start/end aliases, and auto cross-size children,
  generated native head-to-head wrap alignment coverage across wrap and
  wrap-reverse, distributed `AlignContent`, start/end aliases, gaps, and
  LTR/RTL direction,
  wrap-reverse stretched-line cross-axis alignment, wrap-reverse center
  fractional subtree offset re-export, flex stretch cached block-subtree
  re-export including fractional offset rounding, orthogonal flex percent-basis
  subtree reuse, implicit and explicit stretch remeasurement for aligned or
  shrinking percent-basis subtrees including explicit shrinking aligned
  inflexible children, growing stretched flex items, and implicit growing
  aligned percent-basis items, plus shared stretch/growing
  percent-basis lines, local inflexible percent-basis subtrees, implicit
  non-shrinking aligned inflexible percent-basis subtrees, and mixed
  inflexible/growing, aligned shrinking, and unresolved fallback percent-basis
  descendant subtrees, plus resolved stretch-defined percent-basis bases for
  non-shrinking descendants and explicit stretched inflexible percent-basis
  children, explicit stretched growing percent-basis children, and explicit
  stretched percent-basis parents with flexible percent-basis subtrees,
  plus explicit stretched row-flex items whose `fr` flex-basis sibling keeps
  an inflexible percent-basis child on the C++ unresolved fallback path,
  generated high-case regression coverage for the 262144-case source-built
  sweep's mixed `fr`/percent-basis stretch fallback combinations across
  implicit/explicit, growing/shrinking, aligned/inflexible descendants,
  including explicit stretched growing/shrinking parents whose stretched
  percent-basis children stay on the unresolved fallback shrink path and
  implicit growing same-axis flex items whose aligned shrinking percent-basis
  children use the target-main percent base, and linear centered percent cross
  children whose half-pixel start edges expand like C++ edge-difference
  rounding,
  overflow-with-gap distributed alignment including horizontal and vertical
  edge-difference export coverage, row-axis baseline alignment using measured
  content baselines with the C++ default border-box fallback, nested
  flex container baseline propagation, nested grid container border-box
  fallback baselines, generated native head-to-head baseline propagation
  coverage across measured leaves, nested row/column/column-reverse flex,
  nested horizontal/vertical/vertical-reverse linear, grid fallback, and
  relative fallback sources for container `align-items: baseline` and child
  `align-self: baseline` under definite, AtMost, and indefinite owner
  constraints, generated native head-to-head start/end alias coverage across
  flex direction and LTR/RTL direction, point and percentage min/max freeze
  redistribution, block-as-linear child auto-width fill under definite parent
  widths, flex-item `fit-content` width natural main-axis sizing,
  column flex-item `fit-content` height natural main-axis sizing, root flex
  width/height percent/calc `fit-content()` owner-constraint caps,
  argumentless `fit-content()` min/max non-freezing/cap behavior,
  block-as-linear container first-child baseline propagation, display-none
  children skipped during flex item collection with zero exported layouts, and
  custom measurement callbacks.
- Basic linear layout with horizontal/vertical and row/column orientation,
  standalone row/row-reverse/column/column-reverse enum alias mapping,
  reverse main-axis positioning, weighted main-axis distribution including
  point and percentage min/max freeze redistribution, main-axis justification,
  cross-axis alignment,
  C++ linear gravity and layout-gravity mapping for the latest behavior,
  including physical default/after/center/fill groups, definite-only weight
  distribution, non-definite AtMost container sizing, C++-style child cross-axis
  AtMost constraints, intrinsic cross-size stretch suppression, stretch
  including layout-gravity overrides of explicit and weighted child cross sizes,
  generated native head-to-head coverage for every `LinearGravity`,
  `LinearLayoutGravity`, and `LinearCrossGravity` value across orientation and
  LTR/RTL direction, weight/min/max, weight-sum with main-axis gravity,
  start/end justify aliases, layout-gravity overrides, cross-axis auto-margin
  baseline fallback, definite, AtMost, and indefinite owner constraints across
  horizontal/vertical reverse orientations and LTR/RTL direction,
  margins, padding, border, in-flow child ordering by `order`, display-none
  children skipped during stack layout with zero-size layouts at the C++
  parent-padding-bound origin, generated native head-to-head coverage for
  display-none origins and hidden descendants across block, flex, linear,
  relative, and grid containers, and initial staggered-grid
  column-count cross-axis constraints with C++ raw list-gap resolution,
  3+ column regular-item margin stripping, default list components treated as
  regular items, header/footer/list-row full-width preservation, generated
  native head-to-head coverage for every `ListComponentType` value and across
  horizontal/vertical/reverse orientations, LTR/RTL direction, 2/3 columns,
  point/percent/calc/auto/`fr`/intrinsic list gaps, oversized gap column-width
  clamping, fit-content child cross-axis owner constraints, and fixed/measured mixed
  children under definite, cross-axis-indefinite, and fully indefinite owner
  constraints, and reverse
  main-axis placement with staggered cross-axis constraints, plus
  horizontal/vertical container baseline propagation for parent baseline
  alignment, including horizontal baseline parity for the C++ order that
  computes baseline before cross-axis auto margins are resolved and zero
  vertical container baselines export through the C++ fallback-to-height path.
- Relative display layout with parent/sibling edge alignment, explicit none,
  horizontal, vertical, and both-axis center alignment, wrap-content sizing, two-sided
  constraint remeasurement, single-sided at-most constraint reduction,
  C++-style non-once horizontal measure plus vertical
  remeasure with frozen horizontal proposed sizes, non-once wrap-content
  horizontal proposed-size remeasurement after container width determination,
  vertical pre-final proposed-position recomputation before container height
  determination, and `relative_layout_once`
  combined dependency order plus C++-style final position recomputation after
  wrap-content container sizing, duplicate `relative_id` resolution to the last
  matching sibling for position and edge-alignment dependencies while skipping
  display-none dependency anchors, C++-style final parent-edge/center
  recomputation for non-once wrap-content containers, generated native
  head-to-head coverage for duplicate-id position and edge dependencies,
  display-none duplicate anchors, parent-end wrap-content recomputation, and
  combined-order dependency graphs across definite, AtMost, and indefinite
  owner constraints in both one-pass and two-pass relative modes, generated native
  head-to-head coverage for constraint-sensitive measured children across
  parent-edge, sibling-before/after, and two-anchor stretch constraints in
  one-pass and two-pass relative modes, including one-pass parent-edge stretch
  margin stripping and single-end vertical at-most reductions, and root and child
  relative width/height percent/calc `fit-content()` latest wrap-content sizing.
- Basic relative, absolute, fixed, and sticky positioning paths, including
  static-position offset suppression, point/percent relative offsets including
  flex children, calc right/bottom relative offsets, flex/linear absolute initial
  alignment, root-based fixed containing block handling including percent/calc
  end-inset resolution, C++ default
  padding-box containing blocks for absolute/fixed children, auto-size
  constraint stripping and fill-available sizing for definite out-of-flow
  insets, absolute/fixed oversized paired-inset auto-size cases that preserve
  definite measured-child constraint modes even when fill-available space is negative,
  single-pass percent/calc size resolution for absolute/fixed measured and
  subtree layout including border-box measured children, percent/calc
  out-of-flow measured auto-size min/max clamps resolved against containing
  blocks, fixed-descendant
  aspect-ratio sizing from root percent widths, `fit-content()` percent-argument
  out-of-flow subtrees, generated native head-to-head coverage for nested
  fixed descendants under block, flex, linear, relative, and grid ancestors
  using the root containing block across percent/calc insets, fill-available
  sizing, measured aspect-ratio sizing, and `fit-content()` subtrees, and
  exported sticky insets including flex-child, linear-child,
  grid-child, relative-child percent insets, block-child `calc()` insets, and
  auto-inset sentinel export, plus absolute and fixed measured auto-size
  fill-available constraints from paired percent/calc insets, with generated
  native head-to-head sizing coverage across block, flex, linear, relative, and
  grid containers for percent/calc explicit sizing, fill-available paired
  insets, oversized measured fill-available paired insets that preserve definite
  constraint modes under negative available space, measured min/max clamps,
  `fit-content()` measured sizing, and border-box aspect-ratio measured sizing,
  plus generated sticky sizing
  head-to-head coverage for in-flow sticky percent/calc sizing, measured
  auto-size min/max clamps, `fit-content()` measured sizing, and border-box
  aspect-ratio measured sizing across the same container families.
- Initial grid layout with row/column auto-placement, explicit
  point/percentage/calc tracks, min/max track vectors for fixed-to-fr minmax
  sizing, fixed max track growth-limit maximization, C++-style fr-size freeze
  distribution including indefinite-container flex fraction expansion from
  measured item contributions, definite-container spanning item handling for
  flexible max tracks, and min/max-applied container sizes, max-content growth
  for indefinite containers, spanning auto/max-content growth-limit updates
  that prefer still-indefinite growth limits, C++-style indefinite auto minimum
  base sizing that keeps spanning max-content contributions in growth limits
  until fixed-max maximization, C++-style fixed-minimum fit-content maximum
  tracks that keep indefinite base growth separate from growth-limit
  maximization, AtMost fit-content maximum track growth-limit updates without
  max-content base growth, fit-content maximum tracks that keep
  indefinite growth limits while still capping intrinsic growth, auto tracks with repeated implicit sizing patterns,
  non-measured block-axis contribution remeasurement after inline track sizing
  including aspect-ratio child contributions, and C++-style measured grid item
  initial block contributions,
  content alignment that stretches auto tracks only for stretch alignment and
  uses min/max-applied container sizes, row/column gaps including post-size
  percentage gap resolution, basic line placement, spans, start/end/span
  derivation, explicit row-dense auto-flow mapping, negative
  explicit grid lines including leading implicit tracks before the explicit
  grid, max-size-clamped indefinite fixed-max track redistribution that
  preserves C++ gap overflow semantics including row-axis max-height
  constrained redistribution, occupancy-aware placement, dense
  backfill, C++-style column auto-flow cursor retention across following searches,
  max-content minimum tracks that can floor smaller fixed and fit-content
  maximum tracks for single-span, spanning, and definite content-alignment
  intrinsic contributions,
  fit-content track growth
  caps, multi-track fit-content max-track spanning growth for content
  alignment, grid-area containing blocks for absolute/fixed grid children
  including augmented auto lines at container padding edges with fit-content
  self-alignment and direction-aware RTL inline-start padding for auto
  augmented lines, percent/calc oversized paired-inset grid-area children that
  preserve definite measured-child constraint modes when fill-available space
  goes negative, generated native head-to-head coverage for absolute/fixed
  grid-area containing blocks across LTR/RTL, single-track, spanning,
  auto-padding-edge, and last-line-to-auto-end areas, fill-available insets,
  measured alignment, subtree `fit-content()`, and measured `fit-content()`,
  generated native head-to-head item alignment coverage for every
  `JustifyItems` value including `Auto` and every `AlignItems` value across
  LTR/RTL, justify-items/self,
  align-items/self including start/end overrides, stretch versus non-stretch child
  measurement constraints, C++-style final stretch only for at-most child
  constraints while preserving explicit and max-content grid item sizes,
  C++-style baseline alignment fallback to start alignment,
  intrinsic inline/block contribution measurement separated from final stretch
  measurement, fixed/percent/calc `fit-content()` single-track and minmax
  growth caps for fixed and measured items, root and child grid width/height percent/calc `fit-content()`
  latest sizing that preserves larger fixed track sums, container-auto and explicit stretch justify item
  mapping, auto margins for in-flow grid items with generated native
  head-to-head coverage across LTR/RTL, explicit and auto placement, and
  single- or paired-axis auto-margin overrides,
  generated native head-to-head track-sizing coverage across definite,
  indefinite, and at-most owner constraints for flexible minmax tracks, fixed
  max growth limits, max-content minimums, fit-content max caps, and implicit
  auto tracks including implicit auto-track `fit-content()` max caps,
  root `AtMost` constraints that do not cap intrinsic fixed-max track growth,
  justify-content/align-content distributed alignment including
  align-content start/end aliases and overflow-with-gap fallback behavior,
  generated native head-to-head content alignment coverage across `start`/`end`,
  LTR/RTL, extra-space, and
  overflowing track groups, direction-aware RTL inline placement for
  in-flow and absolute grid items, generated native head-to-head auto-flow
  coverage across row, column, dense, row-dense, and column-dense placement
  with LTR/RTL, spans, locked lines, display-none children, and implicit or
  negative line placement, padding, border, and display-none items skipped
  during auto-placement with zero exported layouts.

Native C++ baseline:

`starlight_parity` also contains a native enum-mapping coverage guard that
checks every layout-facing `Style` and style-data enum variant is explicitly
mentioned by the native head-to-head suite.

```sh
# Run Rust tests, including the starlight_ffi C/C++ public-header ABI smoke
# test when clang/clang++ are available.
cargo test --workspace

# The native C++ source depends on generated CSS headers. If
# core/renderer/starlight/style/auto_gen_css_type.h is missing, generate it via
# the repository CSS generator before standalone C++ syntax/build checks.
python3 tools/css_generator/css_parser_generator.py

# Type-check the feature-gated standalone C++ baseline without linking native
# symbols. Layout calls still report NativeLinkUnavailable until a real library
# is provided.
STARLIGHT_CPP_NATIVE_STANDALONE_CHECK=1 \
  cargo clippy -p starlight_cpp --features native-standalone -- -D warnings

# Build the standalone C++ baseline directly from this checkout and run the
# head-to-head suite against the real imported C++ engine.
STARLIGHT_CPP_NATIVE_STANDALONE_BUILD_FROM_SOURCE=1 \
STARLIGHT_GENERATED_CASE_COUNT=32768 \
  cargo test -p starlight_parity --features native-standalone \
    --test native_head_to_head_tests --test native_generated_head_to_head_tests \
    --test standalone_head_to_head_tests

# Require the imported C++ baseline while running the Rust-vs-C++ benchmark.
# The command exits non-zero if C++ is unavailable or if any scenario does not
# exceed the configured Rust-over-C++ speedup threshold. The default threshold
# is 1.0, which preserves the faster-than-C++ gate.
STARLIGHT_CPP_NATIVE_STANDALONE_BUILD_FROM_SOURCE=1 \
STARLIGHT_BENCH_REQUIRE_CPP_BASELINE=1 \
STARLIGHT_BENCH_MIN_SPEEDUP=1.0 \
  cargo run --release -p starlight_bench --features native-standalone -- 1000 200 10

# Link an externally built standalone Starlight library.
STARLIGHT_CPP_NATIVE_STANDALONE_LIB_DIR=/path/to/lib \
STARLIGHT_CPP_NATIVE_STANDALONE_LINK_LIB=static=starlight_standalone \
STARLIGHT_CPP_NATIVE_STANDALONE_CXX_STDLIB=c++ \
  cargo run --release -p starlight_bench --features native-standalone -- 1000 200 10

# Run standalone-compatible Rust-vs-C++ head-to-head parity cases.
STARLIGHT_CPP_NATIVE_STANDALONE_LIB_DIR=/path/to/lib \
STARLIGHT_CPP_NATIVE_STANDALONE_LINK_LIB=static=starlight_standalone \
STARLIGHT_CPP_NATIVE_STANDALONE_CXX_STDLIB=c++ \
  cargo test -p starlight_parity --features native-standalone \
    --test native_head_to_head_tests --test standalone_head_to_head_tests
```

The generated native head-to-head suite defaults to 32768 deterministic supported
tree cases. Use `STARLIGHT_GENERATED_CASE_COUNT=<count>` for longer sweeps or
`STARLIGHT_GENERATED_CASE=<index>` to isolate a single generated case.

The benchmark arguments are `<nodes> <timed-iterations> <warmup-iterations>`.
`STARLIGHT_BENCH_MIN_SPEEDUP` raises the required `cpp_duration / rust_duration`
ratio for every linked C++ scenario; invalid, zero, negative, NaN, or infinite
values fall back to the default threshold of `1.0`.

Replacement readiness:

The Rust engine is not a drop-in default replacement for the production C++
Starlight engine yet. The current Rust workspace has broad latest-mode layout
coverage, a callback-backed `LayoutTree` trait for external trees, C/C++ FFI
glue, C++ standalone head-to-head parity suites, C/C++ ABI smoke tests, and
Rust-vs-C++ benchmark gates. That is enough for standalone parity hardening, but
not enough to remove the C++ backend from production.

Work still required before Rust can fully replace C++:

- Validate renderer lifecycle and cache invalidation before any production
  replacement work, including dirty-bit propagation, constraint-key reuse,
  fixed-node updates, incremental relayout, and platform alignment callbacks.
- Complete platform measurement and baseline validation through the external
  tree callback contract for every text/image/custom-measure path.
- Expand parity beyond the standalone C API surface. The source-built baseline
  covers latest-mode block/flex/fixed/sticky/linear/relative/grid subsets, but
  a full replacement needs head-to-head coverage against the renderer's full
  platform style surface and any non-standalone behavior that affects layout
  results.
- Keep extending algorithm parity where new C++ behavior appears, especially
  grid intrinsic/minmax track sizing, content alignment, baseline propagation,
  fit-content final sizing, out-of-flow sizing, relative dependency ordering,
  and linear/staggered-list edge cases. Benchmark full-layout parity is now
  guarded for every benchmark scenario, including grid item percent edge
  box-data update order, grid alignment/auto-flow/minmax/fit-content matrices,
  linear auto-main sizing from final grid/aspect-ratio child layout, and
  non-root grid fixed descendants being finalized by the root fixed pass.
  Current handwritten and generated suites cover many of these areas, so new
  work should add concrete failing or missing C++ or W3C cases rather
  than broad placeholder TODOs.
- Finish standalone C/C++ glue ownership hardening: ABI version policy, header
  distribution, adapter lifetime audits for track vectors and callbacks, and
  full link/run integration tests for real external-tree embedders.
- Keep the safe-Rust boundary enforced. `starlight_layout` should remain
  `unsafe`-free; any unavoidable unsafe code belongs in FFI/import glue with
  explicit ABI tests.
