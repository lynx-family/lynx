# W3C Grid Layout Algorithm Coverage

Sources:
- https://www.w3.org/TR/css-grid-1/#alignment
- https://www.w3.org/TR/css-grid-1/#fragmentation
- https://www.w3.org/TR/css-grid-1/#layout-algorithm

This inventory is intentionally conservative. Passing C++ parity tests does not
prove that the Rust grid implementation fully follows the W3C grid placement
and track sizing algorithms.
Coverage gaps in private helpers should first be traced to a W3C clause and the
standalone C++ behavior before adding tests. If the helper only expresses an
internal guard, prefer a layout/parity test for the represented behavior, or
remove the helper when the clause does not require it.

## Grid Module Coverage Snapshot

- `command`: `cargo llvm-cov -q -p starlight_layout -p starlight_parity --lib --test grid_layout_tests --test grid_algorithm_coverage_tests --test native_head_to_head_tests --test native_generated_head_to_head_tests --lcov --output-path /private/tmp/starlight_grid_layout_coverage_with_unit_latest.lcov`
- `result`: `crates/starlight_layout/src/engine/grid.rs` line coverage is
  3571/3581 source-based lines, 99.72%; lcov DA line records are 3505/3505,
  100.00%.
- `uncovered`: `cargo llvm-cov report --text --show-missing-lines` does not
  list `grid.rs` for the command above. The remaining source-based notcovered
  segments are synthetic region counters rather than actionable uncovered grid
  algorithm lines. The command includes the grid layout module unit tests plus
  the parity grid layout and native head-to-head suites so private helpers are
  counted only when they have W3C-grounded unit coverage or layout/parity
  coverage.

## Current W3C Surface Boundaries

- The grid layout algorithm consumes style resolver output. Style values are
  assumed to be syntactically valid for their CSS property before they reach
  grid sizing, placement, or alignment code. Invalid CSS inputs belong to the
  parser/style resolver, not to layout-algorithm repair logic.
- The resolver boundary is only a CSS syntax/grammar boundary. If a value is
  syntactically valid but a specific layout mode or grid algorithm clause says
  that it does not apply, does not participate, or falls back for that layout
  context, that remains layout-engine logic and must stay represented in grid
  placement, sizing, and alignment code.
- The style resolver only rejects or normalizes syntax/grammar invalid values.
  It must not erase syntax-valid style data merely because a later layout mode
  ignores it or interprets it through a layout-specific fallback rule.
- The current Rust/C++ Starlight style surface does not carry raw W3C §7
  track-list grammar for named grid lines/areas, `repeat()`/`auto-fill`/
  `auto-fit`, `grid-template-areas`, or grid shorthands. The Rust engine
  consumes pre-expanded numeric track vectors and numeric line/span placement
  fields. Syntax validation and shorthand expansion happen before layout, but
  any semantic effect of syntactically valid values that changes placement,
  sizing, participation, fallback, or applicability is layout-engine logic once
  the style surface represents those inputs.
- W3C grid track sizing only supports `fit-content(<length-percentage>)`.
  Layout consumes style resolver output, so argumentless
  `Length::fit_content(None)` and empty `BaseLength` are not runtime CSS parse
  cases for grid. They are unsupported style-surface states for grid tracks:
  Rust grid conversion asserts the missing-argument invariant for grid track
  `fit-content`, and the standalone C++ bridge does not encode argumentless or
  empty-argument values as grid track functions. Argumentless `fit-content`
  remains valid only where another CSS sizing surface explicitly defines that
  keyword, such as width/height.
- W3C `<flex>` track sizing values are non-negative resolver output. Rust grid
  does not clamp negative `fr` values in the layout algorithm; invalid `fr`
  input belongs to the parser/style resolver, matching the C++ grid algorithm's
  raw `NLength` flex-factor use.
- W3C `span` grid-placement values are positive resolver output. Rust grid does
  not silently clamp zero spans to one in the layout algorithm; invalid zero
  spans are rejected at the Rust FFI boundary before constructing `Style`.
- W3C §12 Fragmenting Grid Layout is not represented by the current Rust/C++
  Starlight layout surface. Rust `Style`, C++ `GridData`, and the standalone C
  API do not expose fragmentainer, page-break, or break-before/after/inside
  inputs, so fragmentation cannot be implemented or tested head-to-head until
  that surface exists.
- Scrollable-overflow padding-edge lines are layout behavior, not resolver
  syntax work. Rust grid uses the laid-out track extent as the scrollable
  content size for absolutely-positioned auto grid lines when tracks overflow
  the content box, following W3C §5.3 and §9.1. The public Rust/C++ style and
  result surfaces still do not expose overflow/scroll-container inputs or a
  `LayoutResult` scrollable overflow rectangle, so clipping/scrollbar/export
  behavior remains outside the represented surface.
- External min-content/max-content text contributions are represented in Rust
  through `LayoutTree::measure_min_content` and
  `LayoutTree::measure_max_content`. The current C++ standalone measure surface
  exposes only a regular measure callback plus baseline callback, and the Rust
  C++ bridge mirrors only that delegate. Head-to-head tests therefore cannot
  faithfully represent cases where an external text engine reports distinct
  regular, min-content, and max-content sizes until C++/standalone expose
  distinct intrinsic measurement callbacks.
- Rust FFI can transport `Length::MinContent` through
  `SLRustLengthMinContent`. Remaining min-content head-to-head gaps are in the
  C++ standalone surface: current C++ `NLength` and the standalone public C API
  have no min-content value unit.

## Starlight C++ Grid Test Inventory

- `source`: `core/renderer/starlight/style/data_ref_unittest.cc`
- `tests`: `StyleDataCoverageTest.LayoutComputedStyleCopyOnWriteDataRefs`
  covers `grid_data_` copy-on-write through `grid_column_gap_`.
- `tests`: `StyleDataCoverageTest.LayoutComputedStyleCopyFromNullDataRefs`
  covers null `grid_data_` fallback to `DefaultLayoutStyle::SL_DEFAULT_GRID_GAP`.
- `rust`: `tests/style_data_coverage_tests.rs::layout_computed_style_copy_on_write_data_refs`
  and `tests/style_data_coverage_tests.rs::layout_computed_style_copy_from_null_data_refs`
  port the grid-related assertions from those C++ style-data tests.
- `note`: `core/renderer/starlight/layout/container_node_unittest.cc` currently
  has no grid-specific layout assertions. Algorithm-level C++ comparisons are
  represented by Rust native/standalone head-to-head suites and their ignored
  gap records below.

## `8.5 Grid Item Placement Algorithm`
- `status`: partial
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::layout_grid`
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::resolve_grid_axis_placement`
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::initial_grid_auto_flow_limit`
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::grid_area_is_free`
- `tests`: `tests/grid_layout_tests.rs::explicit_grid_tracks_place_children_row_major`
- `tests`: `tests/grid_layout_tests.rs::dense_row_auto_flow_backfills_earlier_holes`
- `tests`: `tests/grid_layout_tests.rs::sparse_locked_row_auto_flow_keeps_later_items_past_previous_locked_item`
- `tests`: `tests/grid_layout_tests.rs::dense_locked_row_auto_flow_backfills_hole_before_previous_locked_item`
- `tests`: `tests/grid_layout_tests.rs::sparse_locked_column_auto_flow_keeps_later_items_past_previous_locked_item`
- `tests`: `tests/grid_layout_tests.rs::dense_locked_column_auto_flow_backfills_hole_before_previous_locked_item`
- `tests`: `tests/grid_layout_tests.rs::grid_sparse_column_auto_flow_definite_rows_keep_cursor_and_advance_on_backward_row`
- `tests`: `tests/grid_layout_tests.rs::grid_dense_column_auto_flow_definite_rows_restart_column_search`
- `tests`: `tests/grid_layout_tests.rs::grid_sparse_row_auto_flow_definite_columns_keep_cursor_and_advance_on_backward_column`
- `tests`: `tests/grid_layout_tests.rs::grid_dense_row_auto_flow_definite_columns_restart_row_search`
- `tests`: `tests/grid_layout_tests.rs::reversed_grid_lines_swap_start_and_end_on_both_axes`
- `tests`: `tests/grid_layout_tests.rs::equal_grid_lines_drop_end_and_use_default_span_on_both_axes`
- `tests`: `tests/grid_layout_tests.rs::negative_grid_end_line_with_span_computes_start_on_both_axes`
- `tests`: `tests/grid_layout_tests.rs::negative_grid_start_line_with_span_creates_trailing_implicit_tracks`
- `tests`: `tests/grid_layout_tests.rs::mixed_positive_and_negative_grid_lines_resolve_against_explicit_grid_end`
- `tests`: `tests/grid_layout_tests.rs::positive_grid_end_line_with_span_w3c_expected_leading_implicit_tracks`
- `tests`: `tests/grid_layout_tests.rs::negative_grid_line_before_explicit_grid_creates_leading_implicit_column`
- `tests`: `tests/grid_layout_tests.rs::negative_grid_line_before_explicit_grid_creates_leading_implicit_row`
- `tests`: `tests/grid_layout_tests.rs::grid_auto_placement_orders_in_flow_children_by_order`
- `tests`: `tests/grid_layout_tests.rs::grid_placement_properties_do_not_affect_flex_items`
- `tests`: `tests/grid_layout_tests.rs::hidden_and_collapse_grid_children_participate_in_auto_placement`
- `tests`: `tests/grid_layout_tests.rs::sparse_leading_implicit_column_auto_flow_keeps_cursor_after_span`
- `tests`: `tests/grid_layout_tests.rs::dense_leading_implicit_column_auto_flow_backfills_start_side_hole`
- `tests`: `tests/grid_layout_tests.rs::sparse_leading_implicit_row_column_auto_flow_keeps_cursor_after_span`
- `tests`: `tests/grid_layout_tests.rs::dense_leading_implicit_row_column_auto_flow_backfills_start_side_hole`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_negative_line_span_permutations`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_grid_negative_line_span_permutations`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_line_conflict_handling_swaps_reversed_lines_and_drops_equal_end`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_grid_line_conflict_handling`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_visibility_hidden_and_collapse_participate_in_auto_placement`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_records_cpp_gap_for_positive_end_span_leading_implicit_tracks`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_records_cpp_gap_for_positive_end_span_leading_implicit_tracks`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_records_cpp_gap_for_negative_column_line_before_explicit_grid`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_records_cpp_gap_for_negative_row_line_before_explicit_grid`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_records_cpp_gap_for_grid_negative_lines_and_leading_implicit_tracks`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_w3c_auto_placement_sparse_dense_matrix`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_w3c_auto_placement_row_dense_leading_implicit_backfill`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_w3c_auto_placement_column_dense_leading_implicit_backfill`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_w3c_grid_auto_placement_matrix`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_records_cpp_gap_for_grid_row_dense_leading_implicit_backfill`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_records_cpp_gap_for_grid_column_dense_leading_implicit_backfill`
- `ignored-head-to-head`: `tests/native_head_to_head_tests.rs::head_to_head_grid_w3c_auto_placement_row_dense_leading_implicit_backfill` -- current C++ standalone places the row-dense leading implicit backfill one track earlier than W3C section 8.5 dense auto-placement over the full implicit grid
- `ignored-head-to-head`: `tests/native_head_to_head_tests.rs::head_to_head_grid_w3c_auto_placement_column_dense_leading_implicit_backfill` -- current C++ standalone places the column-dense leading implicit backfill one track later than W3C section 8.5 dense auto-placement over the full implicit grid
- `ignored-head-to-head`: `tests/native_head_to_head_tests.rs::head_to_head_records_cpp_gap_for_positive_end_span_leading_implicit_tracks` -- current C++ standalone truncates trailing explicit tracks when a positive end line plus span creates leading implicit tracks; Rust follows W3C sections 7.5, 7.6, and 8.5 and preserves the full explicit grid
- `ignored-head-to-head`: `tests/native_head_to_head_tests.rs::head_to_head_records_cpp_gap_for_negative_column_line_before_explicit_grid` -- current C++ standalone truncates trailing explicit column tracks when a negative line creates leading implicit tracks; Rust follows W3C sections 7.5 and 7.6 and preserves the full explicit grid
- `ignored-head-to-head`: `tests/native_head_to_head_tests.rs::head_to_head_records_cpp_gap_for_negative_row_line_before_explicit_grid` -- current C++ standalone truncates trailing explicit row tracks when a negative line creates leading implicit tracks; Rust follows W3C sections 7.5 and 7.6 and preserves the full explicit grid
- `ignored-head-to-head`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_records_cpp_gap_for_grid_negative_lines_and_leading_implicit_tracks` -- current C++ standalone truncates trailing explicit tracks when negative lines create leading implicit tracks; Rust follows W3C sections 7.5 and 7.6 and preserves the full explicit grid
- `ignored-head-to-head`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_records_cpp_gap_for_positive_end_span_leading_implicit_tracks` -- current C++ standalone truncates trailing explicit tracks when a positive end line plus span creates leading implicit tracks; Rust follows W3C sections 7.5, 7.6, and 8.5 and preserves the full explicit grid
- `ignored-head-to-head`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_records_cpp_gap_for_grid_row_dense_leading_implicit_backfill` -- current C++ standalone places the row-dense leading implicit backfill one track earlier than W3C section 8.5 dense auto-placement over the full implicit grid
- `ignored-head-to-head`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_records_cpp_gap_for_grid_column_dense_leading_implicit_backfill` -- current C++ standalone places the column-dense leading implicit backfill one track later than W3C section 8.5 dense auto-placement over the full implicit grid
- `gap`: sparse/dense row and column flows now have W3C-derived fully-auto, locked-axis, leading-implicit, line-conflict, negative-end/span, negative-start/span, positive-end/span leading-implicit, negative-line leading-implicit, mixed-sign line coverage, a layout-applicability check that syntactically valid grid-placement properties do not affect flex items, and CSS Display §4 visibility coverage showing that `hidden` and non-flex/table `collapse` grid children still participate in grid auto-placement and sizing. Native C++ parity passes for represented visibility/grid placement; standalone C API parity is blocked until the public standalone surface maps visibility. C++ parity gaps are recorded for positive-end/span or negative-line placement that creates leading implicit tracks before a multi-track explicit grid and for row/column dense backfill across leading implicit tracks. Standalone C++ default uses `SetQuirksMode(kNegativePaddingFixedVersion)`, so `IsGridNewQuirksMode()` is false, but C++ still truncates trailing explicit tracks or places dense backfill at a different leading implicit track in these cases. Named grid lines/areas and grid-area shorthand are not represented by the current Rust/C++ style data; when those syntactically valid surfaces are added, their placement semantics belong in layout. The current Rust/C++ Starlight layout surface already consumes numeric line/span placement fields.

## `9 Absolute Positioning`
- `status`: partial
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::grid_absolute_area`
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::grid_absolute_axis_area`
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::grid_out_of_flow_offset`
- `tests`: `tests/grid_layout_tests.rs::absolute_grid_item_without_grid_lines_uses_content_box_area`
- `tests`: `tests/grid_layout_tests.rs::absolute_grid_item_reversed_lines_create_zero_sized_area`
- `tests`: `tests/grid_layout_tests.rs::absolute_grid_item_auto_grid_lines_use_container_padding_edges`
- `tests`: `tests/grid_layout_tests.rs::absolute_grid_item_auto_lines_use_scrollable_overflow_padding_edges`
- `tests`: `tests/grid_layout_tests.rs::rtl_absolute_grid_item_auto_lines_use_scrollable_overflow_padding_edges`
- `tests`: `tests/grid_layout_tests.rs::absolute_grid_item_auto_end_line_uses_padding_edge_for_fit_content_alignment`
- `tests`: `tests/grid_layout_tests.rs::absolute_grid_item_static_position_uses_content_edges_with_padding_and_margins`
- `tests`: `tests/grid_layout_tests.rs::fixed_grid_item_static_position_uses_content_edges_with_padding_and_margins`
- `tests`: `tests/grid_layout_tests.rs::absolute_grid_item_static_position_auto_margins_follow_grid_item_rules`
- `tests`: `tests/grid_layout_tests.rs::absolute_and_fixed_grid_static_position_self_alignment_matrix`
- `tests`: `tests/grid_layout_tests.rs::absolute_and_fixed_grid_inset_pair_matrix_uses_grid_area_containing_block`
- `tests`: `tests/grid_layout_tests.rs::grid_absolute_child_does_not_affect_sizing_or_auto_placement`
- `tests`: `tests/grid_layout_tests.rs::absolute_grid_item_out_of_range_lines_fall_back_to_padding_edges`
- `tests`: `tests/grid_layout_tests.rs::fixed_grid_item_span_only_placement_uses_padding_edges_for_inset_containing_block`
- `tests`: `tests/grid_layout_tests.rs::rtl_absolute_grid_item_auto_lines_use_inline_start_padding_for_fill_available_insets`
- `tests`: `tests/grid_layout_tests.rs::rtl_absolute_grid_item_tracks_justify_content_end_area_shift`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_absolute_grid_item_auto_lines_use_scrollable_overflow_padding_edges`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_rtl_absolute_grid_item_auto_lines_use_scrollable_overflow_padding_edges`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_records_cpp_gap_for_grid_scrollable_overflow_abspos_auto_lines`
- `tests`: `tests/grid_algorithm_coverage_tests.rs::grid_inventory_surface_limits_are_grounded_in_starlight_sources`
- `ignored-head-to-head`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_records_cpp_gap_for_grid_scrollable_overflow_abspos_auto_lines` -- current standalone C API/C++ path uses the content-box padding edge for absolute grid auto lines when tracks overflow; Rust follows W3C sections 5.3 and 9.1 and uses the scrollable overflow padding edge
- `gap`: §9 now has direct layout coverage for empty grid line offset lists, reversed absolute grid lines producing a zero-sized area, auto start/end lines resolving to padding edges, scrollable-overflow auto padding-edge lines from overflowing track extent in LTR/RTL, out-of-range lines falling back to auto padding-edge lines, span-only placement falling back to auto lines, internal end-line gutter removal, §9.1 inset-pair cross-product over the grid-area containing block for absolute/fixed children, §9.2 static-position alignment against content edges with nonzero margins for absolute/fixed children, every represented self-alignment value and auto fallback, auto-margin static-position behavior matching grid item rules, out-of-flow children not affecting grid sizing or auto-placement, and RTL physical inset/track-origin behavior. Native C++ parity covers the represented scrollable-overflow auto-line geometry; the standalone source-built C API path still has an ignored gap for this geometry and uses the old content-box padding edge. The public Rust/C++ layout data model still has no overflow/scroll-container input and `LayoutResult` has no scrollable overflow rectangle output, so clipping/scrollbar/export behavior cannot be represented yet. Named grid lines/areas and grid-area shorthand are missing style-surface inputs; after syntax-valid values reach layout, their absolute-position placement semantics must be handled by the grid algorithm.

## `10 Alignment and Spacing`
- `status`: partial
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::layout_grid`
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::grid_inline_item_offset`
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::grid_block_item_offset`
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::grid_axis_item_offset`
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::grid_item_used_margin`
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::stretch_grid_auto_tracks`
- `implementation`: `crates/starlight_layout/src/engine.rs::fn resolve_gap`
- `implementation`: `crates/starlight_layout/src/engine.rs::fn justify`
- `implementation`: `crates/starlight_layout/src/engine.rs::fn align_content_with_gap`
- `tests`: `tests/grid_layout_tests.rs::grid_span_uses_combined_tracks_and_gap`
- `tests`: `tests/grid_layout_tests.rs::indefinite_grid_percentage_column_gap_resolves_after_container_min_width`
- `tests`: `tests/grid_layout_tests.rs::indefinite_grid_percentage_row_gap_resolves_after_container_min_height`
- `tests`: `tests/grid_layout_tests.rs::grid_item_horizontal_auto_margins_override_justify_self`
- `tests`: `tests/grid_layout_tests.rs::grid_item_vertical_auto_margins_override_align_self`
- `tests`: `tests/grid_layout_tests.rs::rtl_grid_auto_inline_margins_center_item_in_cell`
- `tests`: `tests/grid_layout_tests.rs::grid_items_center_child_with_justify_and_align_self`
- `tests`: `tests/grid_layout_tests.rs::grid_container_alignment_applies_to_auto_self_children`
- `tests`: `tests/grid_layout_tests.rs::grid_justify_items_auto_and_stretch_map_to_stretch`
- `tests`: `tests/grid_layout_tests.rs::grid_justify_self_overrides_container_justify_items`
- `tests`: `tests/grid_layout_tests.rs::grid_align_items_end_and_align_self_start_variants`
- `tests`: `tests/grid_layout_tests.rs::grid_align_self_baseline_overrides_container_end_alignment_to_start`
- `tests`: `tests/grid_layout_tests.rs::grid_align_items_baseline_uses_start_alignment`
- `tests`: `tests/grid_layout_tests.rs::grid_align_content_distributes_extra_row_space`
- `tests`: `tests/grid_layout_tests.rs::grid_align_content_start_end_alias_flex_edges`
- `tests`: `tests/grid_layout_tests.rs::grid_align_content_space_around_offsets_track_group`
- `tests`: `tests/grid_layout_tests.rs::grid_align_content_space_evenly_offsets_track_group`
- `tests`: `tests/grid_layout_tests.rs::grid_align_content_space_between_keeps_row_gap_when_tracks_overflow`
- `tests`: `tests/grid_layout_tests.rs::grid_align_content_space_around_falls_back_to_start_when_tracks_overflow`
- `tests`: `tests/grid_layout_tests.rs::grid_align_content_space_evenly_falls_back_to_start_when_tracks_overflow`
- `tests`: `tests/grid_layout_tests.rs::grid_justify_content_offsets_track_group`
- `tests`: `tests/grid_layout_tests.rs::grid_justify_content_space_around_offsets_track_group`
- `tests`: `tests/grid_layout_tests.rs::grid_justify_content_space_evenly_offsets_track_group`
- `tests`: `tests/grid_layout_tests.rs::grid_justify_content_space_between_offsets_track_group`
- `tests`: `tests/grid_layout_tests.rs::grid_justify_content_space_between_keeps_column_gap_when_tracks_overflow`
- `tests`: `tests/grid_layout_tests.rs::grid_justify_content_space_around_falls_back_to_start_when_tracks_overflow`
- `tests`: `tests/grid_layout_tests.rs::grid_justify_content_space_evenly_falls_back_to_start_when_tracks_overflow`
- `tests`: `tests/grid_layout_tests.rs::rtl_grid_justify_content_start_variants_align_track_group_to_right_edge`
- `tests`: `tests/grid_layout_tests.rs::rtl_grid_justify_content_end_variants_align_track_group_to_left_edge`
- `tests`: `tests/grid_layout_tests.rs::rtl_grid_justify_self_center_centers_item_in_cell`
- `tests`: `tests/grid_layout_tests.rs::rtl_grid_justify_content_space_between_keeps_right_origin_lines`
- `tests`: `tests/grid_layout_tests.rs::rtl_grid_justify_content_space_around_offsets_track_group_from_right_edge`
- `tests`: `tests/grid_layout_tests.rs::rtl_grid_justify_content_space_evenly_offsets_track_group_from_right_edge`
- `tests`: `tests/grid_layout_tests.rs::rtl_grid_justify_content_space_around_falls_back_to_right_edge_when_tracks_overflow`
- `tests`: `tests/grid_layout_tests.rs::rtl_grid_justify_content_space_evenly_falls_back_to_right_edge_when_tracks_overflow`
- `tests`: `tests/grid_layout_tests.rs::grid_container_baseline_uses_first_row_item_baseline`
- `tests`: `tests/grid_layout_tests.rs::grid_container_baseline_uses_row_major_item_before_source_order`
- `tests`: `tests/flex_layout_tests.rs::flex_row_baseline_uses_nested_grid_container_baseline`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_span_uses_combined_tracks_and_gap`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_item_horizontal_auto_margins_override_justify_self`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_item_vertical_auto_margins_override_align_self`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_align_content_distributes_extra_row_space`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_align_content_space_between_keeps_row_gap_when_tracks_overflow`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_justify_content_space_between_keeps_column_gap_when_tracks_overflow`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_rtl_grid_justify_self_center_centers_item_in_cell`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_rtl_grid_justify_content_space_between_keeps_right_origin_lines`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_container_baseline_uses_first_row_item_baseline`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_container_baseline_uses_row_major_item_before_source_order`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_row_baseline_uses_nested_grid_container_baseline`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_grid_directional_item_self_alignment_mapping`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_records_cpp_gap_for_grid_container_baseline`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_grid_directional_content_alignment_mapping`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_grid_directional_auto_margin_alignment`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_grid_percentage_gap_resolution_matrix`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_grid_align_content_distribution_matrix`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_grid_justify_content_distribution_matrix`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_rtl_grid_justify_content_distribution_matrix`
- `ignored-head-to-head`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_records_cpp_gap_for_grid_container_baseline` -- current C++ standalone exports the grid container fallback baseline from the container bottom edge; Rust follows W3C section 10.6 and exports the first-row grid item baseline
- `gap`: §10 now has direct layout and native parity coverage for represented gutters, post-min-size percentage gap resolution, in-flow grid-item auto margins, `justify-self`/`justify-items`, RTL inline-axis `justify-self` centering, `align-self`/`align-items`, baseline self-alignment fallback to start, grid container first-row baseline export, nested grid container baseline participation in flex baseline alignment, `justify-content`/`align-content` distribution with overflow fallback, and RTL inline-axis content alignment. Standalone C API parity covers the represented non-baseline alignment surfaces, while baseline differences for grid subtrees are normalized in the standalone harness after the explicit ignored gap record above so the remaining geometry remains head-to-head tested. Current Rust/C++ Starlight does not expose full CSS Box Alignment safe/unsafe/normal/left/right/self-start/self-end/first-baseline/last-baseline value surfaces, so the remaining baseline work is limited to those unrepresented value surfaces and full baseline-sharing groups beyond the represented first-baseline fallback.

## `11.1 Grid Sizing Algorithm`
- `status`: partial
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::layout_grid`
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::resolve_grid_tracks`
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::resolve_grid_tracks_for_axis`
- `tests`: `tests/grid_layout_tests.rs::explicit_grid_tracks_place_children_row_major`
- `tests`: `tests/grid_layout_tests.rs::grid_span_uses_combined_tracks_and_gap`
- `tests`: `tests/grid_layout_tests.rs::grid_fit_content_child_receives_at_most_measure_constraints`
- `tests`: `tests/grid_layout_tests.rs::grid_auto_rows_use_column_sized_measured_block_contribution`
- `tests`: `tests/grid_layout_tests.rs::grid_columns_re_resolve_when_row_sizing_changes_inline_min_content_contribution`
- `tests`: `tests/grid_layout_tests.rs::grid_columns_re_resolve_with_updated_inline_max_content_contribution`
- `tests`: `tests/grid_layout_tests.rs::grid_rows_re_resolve_after_column_re_resolution_changes_block_min_content_contribution`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_records_cpp_gap_for_grid_auto_rows_use_column_sized_measured_block_contribution`
- `tests`: `tests/grid_algorithm_coverage_tests.rs::grid_track_sizing_pipeline_preserves_w3c_phase_order`
- `tests`: `tests/grid_algorithm_coverage_tests.rs::grid_inventory_surface_limits_are_grounded_in_starlight_sources`
- `ignored-head-to-head`: `tests/native_head_to_head_tests.rs::head_to_head_records_cpp_gap_for_grid_auto_rows_use_column_sized_measured_block_contribution` -- current C++ standalone keeps the initial measured block contribution after column sizing; Rust follows W3C section 11.1 and recomputes it from the resolved inline space
- `gap`: the current Rust pipeline now has a source-level guard for the implemented column-first, row-next W3C phase order, row sizing uses measured block contributions recomputed from the resolved column inline space, and represented external intrinsic min-content and max-content contributions can trigger the W3C one-shot column re-resolution plus follow-up row re-resolution. Remaining work is broader §11.1 coverage for writing-mode, baseline, and text-layout surfaces that are not represented by the current Rust/C++ Starlight style surface and Rust test harness.

## `11.2 Track Sizing Terminology`
- `status`: partial
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::GridTrackSizing`
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::grid_track_definition`
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::grid_track_kind`
- `tests`: `tests/grid_layout_tests.rs::definite_grid_auto_fit_content_percent_column_max_caps_track`
- `tests`: `tests/grid_layout_tests.rs::grid_fit_content_child_receives_at_most_measure_constraints`
- `tests`: `tests/grid_layout_tests.rs::grid_fr_tracks_share_remaining_content_width`
- `tests`: `tests/grid_layout_tests.rs::grid_auto_column_pattern_initializes_leading_and_positive_implicit_tracks`
- `tests`: `tests/grid_layout_tests.rs::grid_auto_row_pattern_initializes_leading_and_positive_implicit_tracks`
- `tests`: `tests/grid_layout_tests.rs::grid_min_content_track_uses_minimum_contribution_not_measured_size`
- `tests`: `tests/grid_layout_tests.rs::grid_min_content_track_uses_external_min_content_contribution`
- `tests`: `tests/grid_layout_tests.rs::grid_max_content_track_uses_external_max_content_contribution`
- `tests`: `tests/grid_layout_tests.rs::grid_ignores_non_finite_external_max_content_contribution`
- `tests`: `tests/grid_layout_tests.rs::grid_minmax_min_content_maximum_uses_minimum_contribution_as_growth_limit`
- `tests`: `tests/grid_layout_tests.rs::grid_column_track_sizing_function_matrix_initializes_base_sizes`
- `tests`: `tests/grid_layout_tests.rs::grid_row_track_sizing_function_matrix_initializes_base_sizes`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_positive_implicit_columns_repeat_auto_track_pattern`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_leading_implicit_columns_align_auto_track_pattern`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_positive_implicit_rows_repeat_auto_track_pattern`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_leading_implicit_rows_align_auto_track_pattern`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_records_cpp_surface_gap_for_grid_min_content_sizing`
- `tests`: `tests/grid_algorithm_coverage_tests.rs::grid_inventory_surface_limits_are_grounded_in_starlight_sources`
- `tests`: `tests/grid_algorithm_coverage_tests.rs::grid_external_intrinsic_measurement_surface_is_rust_only_until_cpp_bridge_exists`
- `ignored-head-to-head`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_records_cpp_surface_gap_for_grid_min_content_sizing` -- current C++ standalone NLength surface has no min-content value unit; Rust follows W3C sections 11.2, 11.4, 11.5, 11.6, 11.7, and 11.7.1
- `gap`: fixed, percentage, calc, auto, min-content, max-content, W3C argumented fit-content, fr, and explicit/implicit auto-track pattern terms now have direct layout or native parity evidence. External trees can now expose distinct `LayoutTree::measure_min_content` and `LayoutTree::measure_max_content` callbacks for text-derived intrinsic contributions, with direct Rust grid coverage; this branch is Rust-only for now because the current C++ standalone measure delegate and Rust C++ bridge do not expose separate intrinsic measurement callbacks. Remaining terminology-level work is a spec wording audit for broader text/writing-mode combinations not represented by the current Rust/C++ Starlight style surface. Track-list grammar for named grid lines/areas, repeat()/auto-fill/auto-fit, grid-template-areas, and grid-template/grid shorthand is outside the current style surface; syntax validation and shorthand expansion are pre-layout, while any syntax-valid track or placement semantics that affect sizing stay in the layout algorithm once the style surface carries them. Standalone C++ parity cannot encode min-content because current C++ `NLength` has no min-content value unit.

## `11.3 Track Sizing Algorithm`
- `status`: partial
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::resolve_grid_tracks`
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::resolve_grid_tracks_for_axis`
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::finalize_grid_tracks`
- `tests`: `tests/grid_layout_tests.rs::definite_grid_auto_track_caps_intrinsic_growth_to_available_space`
- `tests`: `tests/grid_layout_tests.rs::indefinite_grid_minmax_fixed_max_tracks_grow_to_limits`
- `tests`: `tests/grid_layout_tests.rs::grid_auto_rows_use_column_sized_measured_block_contribution`
- `tests`: `tests/grid_layout_tests.rs::grid_columns_re_resolve_when_row_sizing_changes_inline_min_content_contribution`
- `tests`: `tests/grid_layout_tests.rs::grid_columns_re_resolve_with_updated_inline_max_content_contribution`
- `tests`: `tests/grid_layout_tests.rs::grid_rows_re_resolve_after_column_re_resolution_changes_block_min_content_contribution`
- `tests`: `tests/grid_algorithm_coverage_tests.rs::grid_track_sizing_pipeline_preserves_w3c_phase_order`
- `tests`: `tests/grid_algorithm_coverage_tests.rs::grid_inventory_surface_limits_are_grounded_in_starlight_sources`
- `gap`: the full track sizing algorithm is present as helper phases, and the implemented Initialize -> Resolve Intrinsic -> Maximize -> Expand Flexible -> Stretch phase order is guarded. Row sizing now recomputes block-axis measured contributions after column sizing supplies the inline available space, and row-sized inline min-content changes trigger the W3C one-shot column re-resolution path with updated inline min-content and max-content callbacks. Remaining work is broader cross-axis contribution coverage for writing-mode, baseline, and text-layout combinations not represented by the current Rust/C++ Starlight style surface and Rust test harness.

## `11.4 Initialize Track Sizes`
- `status`: partial
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::grid_track_fixed_growth_limit`
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::grid_track_definition`
- `tests`: `tests/grid_layout_tests.rs::explicit_grid_tracks_place_children_row_major`
- `tests`: `tests/grid_layout_tests.rs::calc_grid_tracks_resolve_against_definite_content_size`
- `tests`: `tests/grid_layout_tests.rs::grid_auto_column_pattern_initializes_leading_and_positive_implicit_tracks`
- `tests`: `tests/grid_layout_tests.rs::grid_auto_row_pattern_initializes_leading_and_positive_implicit_tracks`
- `tests`: `tests/grid_layout_tests.rs::grid_min_content_track_uses_minimum_contribution_not_measured_size`
- `tests`: `tests/grid_layout_tests.rs::grid_minmax_min_content_maximum_uses_minimum_contribution_as_growth_limit`
- `tests`: `tests/grid_layout_tests.rs::grid_column_track_sizing_function_matrix_initializes_base_sizes`
- `tests`: `tests/grid_layout_tests.rs::grid_row_track_sizing_function_matrix_initializes_base_sizes`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_positive_implicit_columns_repeat_auto_track_pattern`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_leading_implicit_columns_align_auto_track_pattern`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_positive_implicit_rows_repeat_auto_track_pattern`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_leading_implicit_rows_align_auto_track_pattern`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_records_cpp_surface_gap_for_grid_min_content_sizing`
- `gap`: base-size initialization now has direct row/column matrices for fixed, percentage, calc, auto, min-content, max-content, and fit-content tracks; fixed growth-limit initialization also has leading and positive implicit auto-track pattern coverage across rows and columns. Remaining work is native parity for min-content after the current C++ `NLength` and standalone public C API grow a min-content value unit. W3C track-list grammar for named grid lines/areas, repeat()/auto-fill/auto-fit, grid-template-areas, and grid-template/grid shorthand is outside the current Rust/C++ style surface; once syntax-valid resolved style data exposes those inputs, their sizing effects belong to grid layout.

## `11.5 Resolve Intrinsic Track Sizes`
- `status`: partial
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::grid_intrinsic_growth_groups`
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::grow_grid_intrinsic_tracks`
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::update_grid_intrinsic_growth_limits`
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::distribute_grid_growth_limits_to_tracks`
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::distribute_grid_growth_limits_beyond_limits`
- `tests`: `../starlight_layout/src/engine/grid.rs::grid_growth_limit_distribution_uses_non_affected_tracks_before_beyond_limits`
- `tests`: `../starlight_layout/src/engine/grid.rs::grid_growth_limit_distribution_marks_newly_finite_tracks_as_infinitely_growable`
- `tests`: `../starlight_layout/src/engine/grid.rs::grid_growth_limit_distribution_continues_beyond_limits_when_needed`
- `tests`: `../starlight_layout/src/engine/grid.rs::grid_growth_limit_distribution_treats_fit_content_at_limit_as_fixed_beyond_limits`
- `tests`: `../starlight_layout/src/engine/grid.rs::grid_growth_limit_distribution_stops_when_fit_content_argument_already_reached`
- `tests`: `../starlight_layout/src/engine/grid.rs::grid_growth_limit_distribution_keeps_fit_content_fixed_after_reaching_argument`
- `tests`: `../starlight_layout/src/engine/grid.rs::grid_growth_limit_beyond_limits_stops_fit_content_at_argument`
- `tests`: `../starlight_layout/src/engine/grid.rs::grid_intrinsic_growth_non_definite_non_flexible_tracks_use_intrinsic_distribution`
- `tests`: `../starlight_layout/src/engine/grid.rs::grid_intrinsic_growth_indefinite_non_flexible_tracks_continue_to_intrinsic_distribution`
- `tests`: `tests/grid_layout_tests.rs::grid_intrinsic_growth_processes_shorter_spans_before_longer_spans`
- `tests`: `tests/grid_layout_tests.rs::definite_grid_auto_track_uses_definite_child_minimum_contribution`
- `tests`: `tests/grid_layout_tests.rs::definite_grid_fixed_min_intrinsic_max_updates_growth_limit_without_base_growth`
- `tests`: `tests/grid_layout_tests.rs::grid_max_content_track_grows_from_measured_child_when_container_is_indefinite`
- `tests`: `tests/grid_layout_tests.rs::grid_minmax_max_content_minimum_can_exceed_fixed_maximum`
- `tests`: `tests/grid_layout_tests.rs::grid_spanning_max_content_minimum_track_can_exceed_fixed_maximum`
- `tests`: `tests/grid_layout_tests.rs::definite_grid_max_content_minimum_floors_fixed_maximum_before_alignment`
- `tests`: `tests/grid_layout_tests.rs::grid_max_content_minimum_floors_fit_content_maximum`
- `tests`: `tests/grid_layout_tests.rs::grid_min_content_track_uses_minimum_contribution_not_measured_size`
- `tests`: `tests/grid_layout_tests.rs::grid_min_content_track_uses_external_min_content_contribution`
- `tests`: `tests/grid_layout_tests.rs::grid_max_content_track_uses_external_max_content_contribution`
- `tests`: `tests/grid_layout_tests.rs::grid_ignores_non_finite_external_max_content_contribution`
- `tests`: `tests/grid_layout_tests.rs::grid_minmax_min_content_maximum_uses_minimum_contribution_as_growth_limit`
- `tests`: `tests/grid_layout_tests.rs::grid_spanning_min_content_maximum_distributes_minimum_contribution_across_tracks`
- `tests`: `tests/grid_layout_tests.rs::grid_spanning_minimum_uses_non_affected_track_before_exceeding_limits`
- `tests`: `tests/grid_layout_tests.rs::grid_spanning_minimum_continues_beyond_fixed_growth_limits`
- `tests`: `tests/grid_layout_tests.rs::grid_spanning_growth_limit_uses_newly_finite_limit_as_infinitely_growable`
- `tests`: `tests/grid_layout_tests.rs::definite_grid_calc_percent_preferred_size_uses_minimum_contribution`
- `tests`: `tests/grid_layout_tests.rs::fixed_preferred_size_grid_item_uses_external_min_content_contribution`
- `tests`: `tests/grid_layout_tests.rs::definite_grid_spanning_fit_content_max_track_contributes_to_content_alignment_size`
- `tests`: `tests/grid_layout_tests.rs::definite_grid_spanning_flexible_max_tracks_expand_before_max_content_sibling`
- `tests`: `tests/grid_layout_tests.rs::indefinite_grid_spanning_fr_item_with_flex_sum_below_one_distributes_remainder_equally`
- `tests`: `tests/grid_layout_tests.rs::indefinite_grid_spanning_zero_fr_item_distributes_remainder_equally`
- `tests`: `tests/grid_layout_tests.rs::dense_grid_spanning_auto_rows_prefer_indefinite_growth_limits`
- `tests`: `tests/grid_layout_tests.rs::grid_intrinsic_growth_planned_increases_are_source_order_independent`
- `tests`: `tests/grid_layout_tests.rs::grid_row_intrinsic_growth_planned_increases_are_source_order_independent`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_max_content_track_grows_from_measured_child_when_container_is_indefinite`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_definite_grid_fixed_min_intrinsic_max_updates_growth_limit_without_base_growth`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_minmax_max_content_minimum_can_exceed_fixed_maximum`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_spanning_max_content_minimum_track_can_exceed_fixed_maximum`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_definite_grid_max_content_minimum_floors_fixed_maximum_before_alignment`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_max_content_minimum_floors_fit_content_maximum`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_spanning_minimum_uses_non_affected_track_before_exceeding_limits`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_spanning_minimum_continues_beyond_fixed_growth_limits`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_definite_grid_spanning_fit_content_max_track_contributes_to_content_alignment_size`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_indefinite_grid_fit_content_max_spanning_items_grow_base_tracks`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_definite_grid_spanning_flexible_max_tracks_expand_before_max_content_sibling`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_indefinite_grid_spanning_fr_item_with_flex_sum_below_one_distributes_remainder_equally`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_indefinite_grid_mixed_fr_fixed_intrinsic_spans`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_dense_grid_spanning_auto_rows_prefer_indefinite_growth_limits`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_intrinsic_growth_planned_increases_are_source_order_independent`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_row_intrinsic_growth_planned_increases_are_source_order_independent`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_grid_spanning_max_content_intrinsic_tracks`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_grid_max_content_minimum_matrix`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_records_cpp_surface_gap_for_grid_min_content_sizing`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_records_cpp_gap_for_grid_spanning_fit_content_max_alignment_size`
- `tests`: `tests/grid_algorithm_coverage_tests.rs::grid_external_intrinsic_measurement_surface_is_rust_only_until_cpp_bridge_exists`
- `gap`: intrinsic minimum, fixed-min/intrinsic-max growth-limit-only sizing, min-content track sizing, external text-derived min-content/max-content contribution callbacks, non-finite external max-content fallback at the Rust `LayoutTree` boundary, min-content maximum growth limits, max-content minimum, fixed and fit-content growth-limit flooring, spanning max-content minimum, row/column same-span planned-increase source-order independence, non-affected track redistribution after affected tracks freeze, base-size beyond-limit fallback after all spanned tracks freeze, non-definite non-flexible fallback to intrinsic distribution, growth-limit non-affected and beyond-limit distribution, fit-content growth-limit tracks stopping once the argument is already reached, newly finite growth limits remaining infinitely growable for the next max-content growth-limit distribution, spanning fit-content max-track contribution, spanning flexible/max-content sibling ordering, flexible spanning distribution when flex sums are below one or zero, mixed fr/fixed intrinsic spans, and dense implicit spanning growth-limit preference now have layout coverage. The external min/max-content callback branch, including the W3C newly-finite-growth-limit example that needs distinct min and max contributions, remains Rust-only until C++/standalone expose distinct intrinsic measurement callbacks. Most represented native C++ units also have native parity and standalone C API parity coverage; the non-affected redistribution branch, below-one flexible spanning branch, fit-content growth-limit argument branch, and mixed fr/fixed intrinsic span branch have ignored native or standalone head-to-head records because current C++ standalone grows affected tracks beyond their limits, distributes all flexible extra by `flex/sum`, lets fit-content max tracks continue past their argument, or leaves an indefinite mixed fr/fixed span at the non-flexible intrinsic width. Remaining work is a full W3C substep audit for broader affected-track/text/writing-mode combinations, plus standalone C++ parity for min-content after current C++ `NLength` and its C API expose that unit.

## `11.5.1 Distributing Extra Space Across Spanned Tracks`
- `status`: partial
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::distribute_grid_intrinsic_growth`
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::distribute_grid_intrinsic_growth_to_tracks`
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::distribute_grid_intrinsic_growth_beyond_limits`
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::distribute_grid_growth_limits_to_tracks`
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::distribute_grid_growth_limits_beyond_limits`
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::distribute_grid_flexible_intrinsic_growth`
- `tests`: `../starlight_layout/src/engine/grid.rs::grid_intrinsic_distribution_uses_non_affected_tracks_before_exceeding_limits`
- `tests`: `../starlight_layout/src/engine/grid.rs::grid_intrinsic_distribution_continues_beyond_limits_when_needed`
- `tests`: `../starlight_layout/src/engine/grid.rs::grid_intrinsic_distribution_prioritizes_max_content_maximum_beyond_limits`
- `tests`: `../starlight_layout/src/engine/grid.rs::grid_intrinsic_distribution_keeps_unresolved_fit_content_intrinsic_beyond_limits`
- `tests`: `../starlight_layout/src/engine/grid.rs::grid_intrinsic_distribution_treats_fit_content_as_max_content_until_argument`
- `tests`: `../starlight_layout/src/engine/grid.rs::grid_intrinsic_distribution_treats_fit_content_at_limit_as_fixed_beyond_limits`
- `tests`: `../starlight_layout/src/engine/grid.rs::grid_intrinsic_distribution_caps_unresolved_fit_content_only_when_requested`
- `tests`: `../starlight_layout/src/engine/grid.rs::grid_growth_limit_distribution_uses_non_affected_tracks_before_beyond_limits`
- `tests`: `../starlight_layout/src/engine/grid.rs::grid_growth_limit_distribution_marks_newly_finite_tracks_as_infinitely_growable`
- `tests`: `../starlight_layout/src/engine/grid.rs::grid_growth_limit_distribution_continues_beyond_limits_when_needed`
- `tests`: `../starlight_layout/src/engine/grid.rs::grid_growth_limit_distribution_treats_fit_content_at_limit_as_fixed_beyond_limits`
- `tests`: `../starlight_layout/src/engine/grid.rs::grid_growth_limit_distribution_stops_when_fit_content_argument_already_reached`
- `tests`: `../starlight_layout/src/engine/grid.rs::grid_growth_limit_distribution_keeps_fit_content_fixed_after_reaching_argument`
- `tests`: `../starlight_layout/src/engine/grid.rs::grid_growth_limit_beyond_limits_stops_fit_content_at_argument`
- `tests`: `../starlight_layout/src/engine/grid.rs::grid_intrinsic_growth_non_definite_non_flexible_tracks_use_intrinsic_distribution`
- `tests`: `../starlight_layout/src/engine/grid.rs::grid_intrinsic_growth_indefinite_non_flexible_tracks_continue_to_intrinsic_distribution`
- `tests`: `tests/grid_layout_tests.rs::grid_intrinsic_growth_processes_shorter_spans_before_longer_spans`
- `tests`: `tests/grid_layout_tests.rs::grid_intrinsic_growth_batches_equal_span_planned_increases`
- `tests`: `tests/grid_layout_tests.rs::grid_intrinsic_growth_planned_increases_are_source_order_independent`
- `tests`: `tests/grid_layout_tests.rs::grid_row_intrinsic_growth_planned_increases_are_source_order_independent`
- `tests`: `tests/grid_layout_tests.rs::grid_spanning_auto_minimum_redistributes_after_fixed_growth_limit`
- `tests`: `tests/grid_layout_tests.rs::grid_spanning_minimum_uses_non_affected_track_before_exceeding_limits`
- `tests`: `tests/grid_layout_tests.rs::grid_spanning_minimum_continues_beyond_fixed_growth_limits`
- `tests`: `tests/grid_layout_tests.rs::grid_spanning_min_content_maximum_distributes_minimum_contribution_across_tracks`
- `tests`: `tests/grid_layout_tests.rs::grid_spanning_growth_limit_uses_newly_finite_limit_as_infinitely_growable`
- `tests`: `tests/grid_layout_tests.rs::indefinite_grid_spanning_fr_item_distributes_growth_by_flex_factor`
- `tests`: `tests/grid_layout_tests.rs::indefinite_grid_spanning_fr_item_with_flex_sum_below_one_distributes_remainder_equally`
- `tests`: `tests/grid_layout_tests.rs::indefinite_grid_spanning_zero_fr_item_distributes_remainder_equally`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_intrinsic_growth_processes_shorter_spans_before_longer_spans`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_intrinsic_growth_batches_equal_span_planned_increases`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_intrinsic_growth_planned_increases_are_source_order_independent`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_row_intrinsic_growth_planned_increases_are_source_order_independent`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_spanning_auto_minimum_redistributes_after_fixed_growth_limit`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_spanning_minimum_uses_non_affected_track_before_exceeding_limits`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_spanning_minimum_continues_beyond_fixed_growth_limits`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_indefinite_grid_spanning_fr_item_distributes_growth_by_flex_factor`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_indefinite_grid_spanning_fr_item_with_flex_sum_below_one_distributes_remainder_equally`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_indefinite_grid_mixed_fr_fixed_intrinsic_spans`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_grid_intrinsic_growth_distribution_matrix`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_grid_flexible_track_expansion_matrix`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_records_cpp_gap_for_grid_spanning_fit_content_max_alignment_size`
- `ignored-head-to-head`: `tests/native_head_to_head_tests.rs::head_to_head_grid_spanning_minimum_uses_non_affected_track_before_exceeding_limits` -- current C++ standalone skips W3C section 11.5.1 non-affected track redistribution and grows affected tracks beyond their limits
- `ignored-head-to-head`: `tests/native_head_to_head_tests.rs::head_to_head_indefinite_grid_spanning_fr_item_with_flex_sum_below_one_distributes_remainder_equally` -- current C++ standalone distributes all flexible spanning extra space by flex/sum when the spanned flex factor sum is below one, instead of distributing the remaining space equally per W3C section 11.5.1
- `ignored-head-to-head`: `tests/native_head_to_head_tests.rs::head_to_head_indefinite_grid_mixed_fr_fixed_intrinsic_spans` -- current C++ standalone leaves this indefinite mixed fr/fixed intrinsic span at the non-flexible intrinsic width; Rust expands the spanned flexible tracks per W3C sections 11.5.1 and 11.7
- `ignored-head-to-head`: `tests/native_head_to_head_tests.rs::head_to_head_indefinite_grid_fit_content_max_spanning_items_grow_base_tracks` -- current C++ standalone lets spanning fit-content max tracks keep receiving growth-limit space after reaching the fit-content argument; Rust follows W3C section 11.5.1 and treats them as fixed at that point
- `ignored-head-to-head`: `tests/native_head_to_head_tests.rs::head_to_head_definite_grid_spanning_fit_content_max_track_contributes_to_content_alignment_size` -- current C++ standalone lets a spanning item push a fit-content max track past its argument for intrinsic growth-limit sizing; Rust follows W3C section 11.5.1
- `ignored-head-to-head`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_records_cpp_gap_for_grid_spanning_fit_content_max_alignment_size` -- current C++ standalone lets a spanning item push a fit-content max track past its argument for intrinsic growth-limit sizing; Rust follows W3C section 11.5.1
- `gap`: shorter-span ordering, equal-span planned increase batching, source-order-independent row/column same-span planned increases, fixed growth-limit capping with redistribution, non-affected track redistribution after affected tracks freeze, base-size beyond-limit distribution after affected and non-affected tracks freeze, max-content maximum prioritization in beyond-limit base-size distribution, unresolved fit-content participation in intrinsic/max-content maximum beyond-limit distribution, fit-content-as-fixed after reaching its argument during beyond-limit distribution, unresolved fit-content cap/no-cap behavior during affected-track distribution, non-definite non-flexible fallback to intrinsic distribution, growth-limit non-affected distribution before beyond-limit distribution, growth-limit beyond-limit distribution, spanning min-content maximum distribution, newly finite growth-limit tracks staying infinitely growable for the immediately following max-content distribution, external min/max-content contribution callbacks, flexible spanning growth by fr factor, flexible spanning distribution when the spanned flex factor sum is below one or zero, and mixed fr/fixed intrinsic spanning growth now have layout coverage. Most represented native C++ units also have native parity and standalone C API parity coverage; the non-affected redistribution branch has an ignored native head-to-head record because current C++ standalone grows affected tracks beyond their limits, the below-one flexible spanning branch has an ignored native head-to-head record because current C++ standalone distributes all extra space by `flex/sum`, the fit-content growth-limit branch has ignored native and standalone records because current C++ standalone continues distributing past the fit-content argument, and the mixed fr/fixed branch has an ignored native record because current C++ standalone leaves the indefinite width at the non-flexible intrinsic total. Remaining work is broader affected-track matrices and additional text/writing-mode contribution branches.

## `11.6 Maximize Tracks`
- `status`: partial
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::maximize_grid_tracks`
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::maximize_grid_tracks_for_indefinite_content`
- `tests`: `tests/grid_layout_tests.rs::indefinite_grid_justify_content_uses_container_min_width`
- `tests`: `tests/grid_layout_tests.rs::grid_minmax_fixed_max_tracks_share_definite_free_space_up_to_limits`
- `tests`: `tests/grid_layout_tests.rs::grid_maximize_tracks_resolves_percent_and_calc_growth_limits_before_redistribution`
- `tests`: `tests/grid_layout_tests.rs::grid_maximize_tracks_does_not_grow_indefinite_growth_limits`
- `tests`: `tests/grid_layout_tests.rs::grid_maximize_tracks_subtracts_gaps_from_definite_free_space`
- `tests`: `tests/grid_layout_tests.rs::grid_maximize_tracks_redistributes_after_fixed_growth_limit_freezes`
- `tests`: `tests/grid_layout_tests.rs::indefinite_grid_minmax_fixed_max_tracks_grow_to_limits`
- `tests`: `tests/grid_layout_tests.rs::indefinite_grid_minmax_fixed_max_tracks_respect_container_max_size`
- `tests`: `tests/grid_layout_tests.rs::min_content_grid_size_uses_zero_free_space_for_fixed_growth_limits`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_minmax_fixed_max_tracks_share_definite_free_space_up_to_limits`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_maximize_tracks_resolves_percent_and_calc_growth_limits_before_redistribution`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_maximize_tracks_does_not_grow_indefinite_growth_limits`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_maximize_tracks_subtracts_gaps_from_definite_free_space`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_maximize_tracks_redistributes_after_fixed_growth_limit_freezes`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_indefinite_grid_minmax_fixed_max_tracks_grow_to_limits`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_indefinite_grid_minmax_fixed_max_tracks_respect_container_max_size`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_grid_maximize_tracks_subtracting_gaps`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_grid_maximize_track_growth_matrix`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_records_cpp_surface_gap_for_grid_min_content_sizing`
- `gap`: definite equal distribution, gap-aware free-space calculation, fixed/percent/calc growth-limit resolution, non-finite growth-limit non-growth, growth-limit freezing with redistribution, indefinite max-content-like growth to limits, min-content zero-free-space suppression, and max-size clamp now have direct layout coverage. Represented non-min-content units also have native parity and standalone C API parity coverage; min-content remains Rust-only because current C++ `NLength` and the standalone public C API have no min-content value unit.

## `11.7 Expand Flexible Tracks`
- `status`: partial
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::expand_grid_flexible_tracks`
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::expand_grid_flexible_tracks_for_indefinite_content`
- `tests`: `tests/grid_layout_tests.rs::grid_fr_tracks_share_remaining_content_width`
- `tests`: `tests/grid_layout_tests.rs::grid_fr_tracks_reserve_fixed_tracks_and_gaps`
- `tests`: `tests/grid_layout_tests.rs::grid_fr_tracks_do_not_expand_when_available_space_is_exhausted`
- `tests`: `tests/grid_layout_tests.rs::indefinite_grid_fr_tracks_expand_from_existing_flex_base`
- `tests`: `tests/grid_layout_tests.rs::indefinite_grid_without_flexible_tracks_skips_fr_expansion`
- `tests`: `tests/grid_layout_tests.rs::indefinite_grid_fixed_track_items_do_not_seed_fr_fraction`
- `tests`: `tests/grid_layout_tests.rs::indefinite_grid_max_width_below_fixed_tracks_suppresses_fr_expansion`
- `tests`: `tests/grid_layout_tests.rs::indefinite_grid_fr_columns_expand_to_container_min_width`
- `tests`: `tests/grid_layout_tests.rs::indefinite_grid_fr_columns_redo_flex_fraction_with_container_max_width`
- `tests`: `tests/grid_layout_tests.rs::indefinite_grid_fr_rows_expand_to_container_min_height`
- `tests`: `tests/grid_layout_tests.rs::min_content_grid_width_uses_zero_flex_fraction_for_fr_tracks`
- `tests`: `tests/grid_layout_tests.rs::min_content_grid_height_uses_zero_flex_fraction_for_fr_tracks`
- `tests`: `tests/grid_layout_tests.rs::max_content_grid_width_uses_item_contribution_to_expand_fr_tracks`
- `tests`: `tests/grid_layout_tests.rs::indefinite_grid_spanning_fr_item_distributes_growth_by_flex_factor`
- `tests`: `tests/grid_layout_tests.rs::indefinite_grid_spanning_fr_item_with_flex_sum_below_one_distributes_remainder_equally`
- `tests`: `tests/grid_layout_tests.rs::indefinite_grid_spanning_zero_fr_item_distributes_remainder_equally`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_fr_tracks_share_remaining_content_width`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_fr_tracks_reserve_fixed_tracks_and_gaps`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_indefinite_grid_fr_tracks_expand_from_existing_flex_base`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_indefinite_grid_fr_columns_expand_to_container_min_width`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_indefinite_grid_fr_columns_redo_flex_fraction_with_container_max_width`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_max_content_grid_width_expands_fr_tracks_from_item_contribution`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_indefinite_grid_fr_rows_expand_to_container_min_height`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_indefinite_grid_spanning_fr_item_distributes_growth_by_flex_factor`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_indefinite_grid_spanning_fr_item_with_flex_sum_below_one_distributes_remainder_equally`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_indefinite_grid_mixed_fr_fixed_intrinsic_spans`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_grid_flexible_track_expansion_matrix`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_records_cpp_surface_gap_for_grid_min_content_sizing`
- `gap`: definite available space, fixed-track/gap reservation, indefinite available space, existing flex base expansion, spanning fr growth distribution including below-one and zero flex-factor sums, mixed fr/fixed intrinsic span growth, orthogonal row expansion, min/max container-size retry branches, max-content constraint expansion, and min-content constraint zero-flex-fraction behavior now have direct layout coverage. Represented native C++ units also have native and standalone C API parity coverage except for the below-one flexible spanning record, where current C++ standalone distributes all extra space by `flex/sum`, and the mixed fr/fixed intrinsic record, where current C++ standalone leaves the indefinite width at the non-flexible intrinsic total. Min-content remains Rust-only because current C++ `NLength` and the standalone public C API have no min-content value unit.

## `11.7.1 Find the Size of an fr`
- `status`: partial
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::find_grid_fr_size`
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::grid_flex_factors`
- `tests`: `tests/grid_layout_tests.rs::grid_fr_tracks_flex_factor_sum_below_one_leaves_remaining_space`
- `tests`: `tests/grid_layout_tests.rs::grid_fr_tracks_freeze_large_base_sizes_when_finding_fr_size`
- `tests`: `tests/grid_layout_tests.rs::grid_fr_size_restarts_after_each_large_base_freeze`
- `tests`: `tests/grid_layout_tests.rs::grid_fr_size_uses_spanning_intrinsic_growth_as_base_size`
- `tests`: `tests/grid_layout_tests.rs::grid_minmax_tracks_use_fr_size_from_available_track_space`
- `tests`: `tests/grid_layout_tests.rs::min_content_grid_width_uses_zero_flex_fraction_for_fr_tracks`
- `tests`: `tests/grid_layout_tests.rs::min_content_grid_height_uses_zero_flex_fraction_for_fr_tracks`
- `tests`: `tests/grid_layout_tests.rs::max_content_grid_width_uses_item_contribution_to_expand_fr_tracks`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_fr_tracks_flex_factor_sum_below_one_leaves_remaining_space`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_fr_tracks_freeze_large_base_sizes_when_finding_fr_size`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_fr_size_restarts_after_each_large_base_freeze`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_fr_size_uses_spanning_intrinsic_growth_as_base_size`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_minmax_tracks_use_fr_size_from_available_track_space`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_max_content_grid_width_expands_fr_tracks_from_item_contribution`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_grid_fr_size_matrix`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_records_cpp_surface_gap_for_grid_min_content_sizing`
- `gap`: flex-factor sum normalization, single- and multi-step large-base freezing/restart, spanning intrinsic growth as a fr base-size input, minmax fr sizing, max-content constraint item contribution sizing, and min-content constraint zero flex fraction now have direct layout coverage. Represented native C++ units also have native and standalone C API parity coverage; min-content remains Rust-only until current C++ `NLength` and the standalone C API expose a min-content value unit.

## `11.8 Stretch auto Tracks`
- `status`: partial
- `implementation`: `crates/starlight_layout/src/engine/grid.rs::stretch_grid_auto_tracks`
- `implementation`: `crates/starlight_layout/src/engine.rs::align_content_with_gap`
- `tests`: `tests/grid_layout_tests.rs::indefinite_grid_stretch_auto_track_uses_container_min_width`
- `tests`: `tests/grid_layout_tests.rs::definite_grid_stretch_distributes_free_space_to_auto_max_tracks_only`
- `tests`: `tests/grid_layout_tests.rs::indefinite_grid_percentage_column_gap_resolves_after_container_min_width`
- `tests`: `tests/grid_layout_tests.rs::indefinite_grid_percentage_row_gap_resolves_after_container_min_height`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_indefinite_grid_stretch_auto_track_uses_container_min_width`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_definite_grid_stretch_distributes_free_space_to_auto_max_tracks_only`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_indefinite_grid_percentage_column_gap_resolves_after_container_min_width`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_indefinite_grid_percentage_row_gap_resolves_after_container_min_height`
- `tests`: `tests/grid_layout_tests.rs::grid_align_content_distributes_extra_row_space`
- `tests`: `tests/grid_layout_tests.rs::grid_align_content_start_end_alias_flex_edges`
- `tests`: `tests/grid_layout_tests.rs::grid_align_content_space_around_offsets_track_group`
- `tests`: `tests/grid_layout_tests.rs::grid_align_content_space_evenly_offsets_track_group`
- `tests`: `tests/grid_layout_tests.rs::grid_align_content_space_between_keeps_row_gap_when_tracks_overflow`
- `tests`: `tests/grid_layout_tests.rs::grid_align_content_space_around_falls_back_to_start_when_tracks_overflow`
- `tests`: `tests/grid_layout_tests.rs::grid_align_content_space_evenly_falls_back_to_start_when_tracks_overflow`
- `tests`: `tests/grid_layout_tests.rs::grid_justify_content_space_evenly_offsets_track_group`
- `tests`: `tests/grid_layout_tests.rs::grid_justify_content_space_between_offsets_track_group`
- `tests`: `tests/grid_layout_tests.rs::grid_justify_content_space_around_offsets_track_group`
- `tests`: `tests/grid_layout_tests.rs::grid_justify_content_space_between_keeps_column_gap_when_tracks_overflow`
- `tests`: `tests/grid_layout_tests.rs::grid_justify_content_space_evenly_falls_back_to_start_when_tracks_overflow`
- `tests`: `tests/grid_layout_tests.rs::grid_justify_content_space_around_falls_back_to_start_when_tracks_overflow`
- `tests`: `tests/grid_layout_tests.rs::rtl_grid_justify_content_start_variants_align_track_group_to_right_edge`
- `tests`: `tests/grid_layout_tests.rs::rtl_grid_justify_content_end_variants_align_track_group_to_left_edge`
- `tests`: `tests/grid_layout_tests.rs::rtl_grid_justify_content_space_between_keeps_right_origin_lines`
- `tests`: `tests/grid_layout_tests.rs::rtl_grid_justify_content_space_evenly_offsets_track_group_from_right_edge`
- `tests`: `tests/grid_layout_tests.rs::rtl_grid_justify_content_space_around_offsets_track_group_from_right_edge`
- `tests`: `tests/grid_layout_tests.rs::rtl_grid_justify_content_space_evenly_falls_back_to_right_edge_when_tracks_overflow`
- `tests`: `tests/grid_layout_tests.rs::rtl_grid_justify_content_space_around_falls_back_to_right_edge_when_tracks_overflow`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_align_content_distributes_extra_row_space`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_align_content_start_end_alias_flex_edges`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_align_content_space_around_offsets_track_group`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_align_content_space_evenly_offsets_track_group`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_align_content_space_between_keeps_row_gap_when_tracks_overflow`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_align_content_space_around_falls_back_to_start_when_tracks_overflow`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_align_content_space_evenly_falls_back_to_start_when_tracks_overflow`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_justify_content_space_evenly_offsets_track_group`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_justify_content_space_between_offsets_track_group`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_justify_content_space_around_offsets_track_group`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_justify_content_space_between_keeps_column_gap_when_tracks_overflow`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_justify_content_space_evenly_falls_back_to_start_when_tracks_overflow`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_grid_justify_content_space_around_falls_back_to_start_when_tracks_overflow`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_rtl_grid_justify_content_start_variants_align_track_group_to_right_edge`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_rtl_grid_justify_content_end_variants_align_track_group_to_left_edge`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_rtl_grid_justify_content_space_between_keeps_right_origin_lines`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_rtl_grid_justify_content_space_evenly_offsets_track_group_from_right_edge`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_rtl_grid_justify_content_space_around_offsets_track_group_from_right_edge`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_rtl_grid_justify_content_space_evenly_falls_back_to_right_edge_when_tracks_overflow`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_rtl_grid_justify_content_space_around_falls_back_to_right_edge_when_tracks_overflow`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_grid_stretch_auto_track_matrix`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_grid_percentage_gap_resolution_matrix`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_grid_align_content_distribution_matrix`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_grid_justify_content_distribution_matrix`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_rtl_grid_justify_content_distribution_matrix`
- `gap`: §11.8 now has direct layout, native parity, and standalone C API parity coverage for definite free space, the indefinite min-size fallback, percentage row/column gap resolution after min-size, align-content start/end/space-around/space-evenly/overflow fallback distribution, and LTR/RTL justify-content space distribution/overflow fallback. Remaining max/min-content constraint work is tracked under §11.5 intrinsic track sizing.
