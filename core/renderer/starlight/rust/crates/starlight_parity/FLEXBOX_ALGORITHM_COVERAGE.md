# W3C Flexbox Layout Algorithm Coverage

Source: https://www.w3.org/TR/css-flexbox-1/#layout-algorithm

This inventory is intentionally conservative. A section is not complete while
the Rust engine still depends on compatibility behavior that is not directly
traced to the W3C algorithm.

## W3C Alignment Clause Trace

Source clauses:
- CSS Flexbox §8.1 auto margins: `tests/flex_layout_tests.rs::main_axis_auto_margin_consumes_remaining_space_before_justify_content`, `tests/flex_layout_tests.rs::main_axis_auto_margin_without_positive_free_space_zeroes_margins_then_justify_content`, `tests/flex_layout_tests.rs::single_cross_axis_auto_margins_absorb_positive_free_space`, `tests/flex_layout_tests.rs::paired_cross_axis_auto_margins_center_item`, and `tests/flex_layout_tests.rs::overflowing_cross_axis_auto_margins_place_overflow_at_cross_end`.
- CSS Flexbox §8.2 / §9.5 main-axis distribution: `tests/flex_layout_tests.rs::justify_content_stretch_behaves_like_flex_start_in_flex_layout`, `tests/flex_layout_tests.rs::justify_content_center_uses_negative_free_space_when_items_overflow`, `tests/flex_layout_tests.rs::justify_content_space_between_single_item_falls_back_to_flex_start`, `tests/flex_layout_tests.rs::justify_content_space_between_keeps_gap_when_items_overflow`, `tests/flex_layout_tests.rs::justify_content_space_around_single_item_falls_back_to_center`, `tests/flex_layout_tests.rs::justify_content_space_around_centers_overflow_and_keeps_gap`, `tests/flex_layout_tests.rs::justify_content_space_evenly_distributes_free_space`, `tests/flex_layout_tests.rs::justify_content_space_evenly_single_item_uses_equal_edge_spaces`, `tests/flex_layout_tests.rs::justify_content_main_axis_direction_matrix_places_items`, `tests/flex_layout_tests.rs::justify_content_negative_free_space_direction_matrix_uses_w3c_fallbacks`, `tests/flex_layout_tests.rs::justify_content_gap_overflow_direction_matrix_preserves_gap_after_fallback`, and `tests/flex_layout_tests.rs::main_axis_auto_margin_direction_matrix_consumes_free_space`.
- CSS Flexbox §9.7 flexible lengths: `../starlight_layout/src/engine/flex.rs::used_flex_factor_is_grow_only_when_hypothetical_sum_is_less_than_container`, `../starlight_layout/src/engine/flex.rs::unfrozen_items_start_from_flex_base_size_before_distribution`, `../starlight_layout/src/engine/flex.rs::initial_free_space_uses_frozen_targets_unfrozen_bases_outer_sizes_and_gap`, `../starlight_layout/src/engine/flex.rs::distributes_positive_free_space_by_flex_grow`, `../starlight_layout/src/engine/flex.rs::distributes_negative_free_space_by_scaled_flex_shrink_factor`, `../starlight_layout/src/engine/flex.rs::freezes_min_violations_and_recomputes_remaining_shrink_space`, `../starlight_layout/src/engine/flex.rs::freezes_max_violations_and_recomputes_remaining_grow_space`, `../starlight_layout/src/engine/flex.rs::clamps_negative_inner_main_sizes_to_zero_before_freezing_min_violations`, `tests/flex_layout_tests.rs::flex_grow_sum_below_one_leaves_remaining_space_for_justify_content`, `tests/flex_layout_tests.rs::flex_shrink_sum_below_one_leaves_negative_space_for_justify_content`, `tests/flex_layout_tests.rs::flex_shrink_distribution_is_scaled_by_flex_base_size`, `tests/flex_layout_tests.rs::flex_shrink_negative_inner_size_is_floored_after_outer_margins`, `tests/flex_layout_tests.rs::all_zero_flex_grow_items_freeze_and_leave_space_for_justify_content`, `tests/flex_layout_tests.rs::flexible_lengths_resolve_independently_per_wrapped_line`, and `tests/flex_layout_tests.rs::flexible_lengths_direction_matrix_places_resolved_main_sizes`.
- CSS Flexbox §8.3 / §9.6 cross-axis self-alignment: `tests/flex_layout_tests.rs::align_self_overrides_container_align_items`, `tests/flex_layout_tests.rs::align_items_start_end_alias_flex_edges_for_items`, `tests/flex_layout_tests.rs::align_items_center_uses_negative_cross_space_when_item_overflows`, `tests/flex_layout_tests.rs::align_items_flex_end_uses_negative_cross_space_when_item_overflows`, `tests/flex_layout_tests.rs::align_items_stretch_only_stretches_auto_cross_size_items_without_auto_margins`, `tests/flex_layout_tests.rs::align_items_cross_axis_direction_and_wrap_reverse_matrix_places_items`, `tests/flex_layout_tests.rs::cross_axis_auto_margin_direction_and_wrap_reverse_matrix_resolves_margins`, `tests/flex_layout_tests.rs::overflowing_cross_axis_auto_margin_direction_matrix_overflows_cross_end`, `tests/flex_layout_tests.rs::stretched_flex_item_relayouts_percent_height_child_with_definite_cross_size`, and `tests/flex_layout_tests.rs::stretched_flex_item_cross_size_respects_min_max_constraints`.
- CSS Flexbox §8.3 baseline self-alignment: `tests/flex_layout_tests.rs::flex_row_baseline_aligns_items_by_fallback_border_box_baseline`, `tests/flex_layout_tests.rs::flex_row_baseline_uses_measured_content_baseline`, `tests/flex_layout_tests.rs::flex_row_align_self_baseline_triggers_baseline_line_sizing`, and `tests/flex_layout_tests.rs::flex_row_baseline_can_expand_auto_cross_size_for_bottom_margin`.
- CSS Flexbox §8.4 align-content and multi-line packing: `tests/flex_layout_tests.rs::single_line_align_content_does_not_pack_the_line`, `tests/flex_layout_tests.rs::align_content_start_end_alias_flex_edges_for_wrapped_lines`, `tests/flex_layout_tests.rs::align_content_centers_wrapped_lines_in_cross_axis`, `tests/flex_layout_tests.rs::align_content_stretch_expands_wrapped_line_cross_sizes`, `tests/flex_layout_tests.rs::align_content_space_between_negative_free_space_falls_back_to_flex_start`, `tests/flex_layout_tests.rs::align_content_space_between_keeps_row_gap_when_lines_overflow`, `tests/flex_layout_tests.rs::align_content_space_around_negative_free_space_falls_back_to_center`, `tests/flex_layout_tests.rs::align_content_space_around_centers_overflow_and_keeps_row_gap`, `tests/flex_layout_tests.rs::align_content_space_evenly_distributes_wrapped_lines`, and `tests/flex_layout_tests.rs::align_content_space_evenly_uses_negative_space_when_lines_overflow`.
- CSS Flexbox §8.5 flex container baselines: `tests/flex_layout_tests.rs::flex_row_baseline_uses_nested_flex_container_baseline`, `tests/flex_layout_tests.rs::flex_column_container_baseline_uses_first_item_baseline_after_main_axis_alignment`, and `tests/flex_layout_tests.rs::flex_layout_uses_external_text_layout_trait_for_content_size_and_baseline`.

Text layout boundary:
- Starlight core remains text-engine agnostic. Flex layout consumes external content size and baseline only through `LayoutTree::measure` and `LayoutTree::baseline`; shaping, bidi, line breaking, glyph metrics, and anonymous text item construction belong to the embedding tree/text adapter.

## W3C Initial Setup Clause Trace

Source clauses:
- CSS Flexbox §9.1 initial setup delegates flex-item generation to CSS Flexbox §4 before line layout starts: `tests/flex_layout_tests.rs::flex_initial_setup_adapter_generates_anonymous_items_for_contiguous_text_runs`.
- CSS Flexbox §4 flex items requires each in-flow child to become a flex item, each text sequence to be wrapped in an anonymous block container flex item, and whitespace-only text not to render: `tests/flex_layout_tests.rs::display_none_child_is_laid_out_as_zero_and_skipped_by_flex`, `tests/flex_layout_tests.rs::flex_order_reorders_visual_layout_without_reordering_tree`, and `tests/flex_layout_tests.rs::flex_initial_setup_adapter_generates_anonymous_items_for_contiguous_text_runs`.
- CSS Flexbox §4.1 absolutely-positioned flex children requires out-of-flow children not to participate in flex layout while their static-position rectangle is aligned against the flex container: `tests/flex_layout_tests.rs::flex_initial_setup_skips_out_of_flow_children_but_positions_static_rect` and `tests/native_head_to_head_tests.rs::head_to_head_absolute_flex_child_uses_static_position_without_participating_in_flex_layout`.
- The Rust trait boundary is now explicit: anonymous text item construction is an adapter responsibility, and the layout engine consumes the resulting generated item through `LayoutTree::children`, `LayoutTree::measure`, and `LayoutTree::baseline`.

## W3C Line Length Clause Trace

Source clauses:
- CSS Flexbox §9.2 available main and cross space uses the flex container content box when definite, otherwise subtracting border/padding from the available space before flex-basis measurement: `tests/flex_layout_tests.rs::flex_line_length_available_main_space_uses_inner_content_box_for_auto_basis` and `tests/native_head_to_head_tests.rs::head_to_head_flex_line_length_available_main_space_uses_inner_content_box_for_auto_basis`.
- CSS Flexbox §9.2 definite flex basis uses the resolved used flex-basis as the flex base size instead of the main-size property: `tests/flex_layout_tests.rs::flex_line_length_definite_flex_basis_overrides_main_size_property` and `tests/native_head_to_head_tests.rs::head_to_head_flex_line_length_definite_flex_basis_overrides_main_size_property`.
- CSS Flexbox §9.2 aspect-ratio flex base derives the main-axis flex base from a definite cross size when the used flex basis is content: `tests/flex_layout_tests.rs::flex_line_length_aspect_ratio_uses_definite_cross_size_for_content_basis`, `tests/flex_layout_tests.rs::column_flex_item_percent_cross_size_and_aspect_ratio_define_main_basis`, `tests/native_head_to_head_tests.rs::head_to_head_flex_line_length_aspect_ratio_uses_definite_cross_size_for_content_basis`, and `tests/native_head_to_head_tests.rs::head_to_head_column_flex_item_percent_cross_size_and_aspect_ratio_define_main_basis`.
- CSS Flexbox §9.8 definite and indefinite sizes propagate post-flexing main sizes and determined flex-line cross sizes as definite bases for descendant percentages: `tests/flex_layout_tests.rs::definite_flex_basis_post_flexing_main_size_defines_descendant_percent_flex_basis_base`, `tests/flex_layout_tests.rs::definite_container_main_size_defines_auto_basis_item_descendant_percent_flex_basis_base`, `tests/flex_layout_tests.rs::unresolved_percent_flex_basis_does_not_define_descendant_percent_flex_basis_base`, and `tests/flex_layout_tests.rs::resolved_flex_line_cross_size_defines_nested_flex_percent_basis_base`.
- CSS Flexbox §9.2 content flex basis into available space sizes measured content under the available main-axis constraints: `tests/flex_layout_tests.rs::flex_line_length_available_main_space_uses_inner_content_box_for_auto_basis`, `tests/flex_layout_tests.rs::flex_nowrap_at_most_main_axis_shrinks_to_items`, and `tests/native_head_to_head_tests.rs::head_to_head_flex_line_length_available_main_space_uses_inner_content_box_for_auto_basis`.
- CSS Flexbox §9.2 min/max ignored for flex base size keeps flex base size and hypothetical main size as separate values before line collection and flexible-length resolution: `crates/starlight_layout/src/engine/flex.rs::flex_basis`, `crates/starlight_layout/src/engine/flex.rs::clamp_flex_item_main`, `tests/flex_layout_tests.rs::flex_line_length_hypothetical_main_size_clamps_min_before_wrapping`, and `tests/flex_layout_tests.rs::flex_line_length_hypothetical_main_size_clamps_max_before_wrapping`.
- CSS Flexbox §9.2 hypothetical main size clamp is applied before wrapped line collection uses outer hypothetical main sizes: `tests/flex_layout_tests.rs::flex_line_length_hypothetical_main_size_clamps_min_before_wrapping`, `tests/flex_layout_tests.rs::flex_line_length_hypothetical_main_size_clamps_max_before_wrapping`, `tests/native_head_to_head_tests.rs::head_to_head_flex_line_length_hypothetical_main_size_clamps_min_before_wrapping`, and `tests/native_head_to_head_tests.rs::head_to_head_flex_line_length_hypothetical_main_size_clamps_max_before_wrapping`.
- CSS Flexbox §9.2 flex container main size uses the formatting-context result after flex base and hypothetical main sizes are known: `tests/flex_layout_tests.rs::flex_line_length_auto_container_main_size_uses_max_content_sum`, `tests/flex_layout_tests.rs::flex_nowrap_at_most_main_axis_shrinks_to_items`, and `tests/native_head_to_head_tests.rs::head_to_head_flex_line_length_auto_container_main_size_uses_max_content_sum`.

Remaining §9.2 audit gap:
- The engine does not yet model separate min-content/max-content constraint layout modes or the orthogonal-flow max-content branch from the W3C algorithm. Text and orthogonal writing-mode details remain external to `LayoutTree::measure` until a text/writing-mode adapter supplies that behavior.

## W3C Main Size Clause Trace

Source clauses:
- CSS Flexbox §9.3 single-line collection requires a single-line flex container to collect all flex items into one flex line: `tests/flex_layout_tests.rs::flex_main_size_nowrap_collects_all_items_into_single_line_even_when_overflowing` and `tests/native_head_to_head_tests.rs::head_to_head_flex_main_size_nowrap_collects_all_items_into_single_line_even_when_overflowing`.
- CSS Flexbox §9.3 multi-line collection until overflow collects consecutive flex items until the next item would not fit the flex container inner main size: `tests/flex_layout_tests.rs::flex_wrap_collects_items_into_multiple_lines`, `tests/flex_layout_tests.rs::flex_main_size_resolves_flexible_lengths_per_line_independently`, `tests/native_head_to_head_tests.rs::head_to_head_flex_wrap_collects_items_into_multiple_lines`, and `tests/native_head_to_head_tests.rs::head_to_head_flex_main_size_resolves_flexible_lengths_per_line_independently`.
- CSS Flexbox §9.3 oversized first item alone requires the first uncollected item to form a line by itself when it cannot fit: `tests/flex_layout_tests.rs::flex_main_size_wrap_collects_oversized_first_item_alone` and `tests/native_head_to_head_tests.rs::head_to_head_flex_main_size_wrap_collects_oversized_first_item_alone`.
- CSS Flexbox §9.3 outer hypothetical main size is the line-collection size, including margins and allowing negative outer contributions: `tests/flex_layout_tests.rs::flex_main_size_line_collection_uses_outer_hypothetical_main_with_negative_margin`, `tests/flex_layout_tests.rs::flex_line_length_hypothetical_main_size_clamps_min_before_wrapping`, `tests/flex_layout_tests.rs::flex_line_length_hypothetical_main_size_clamps_max_before_wrapping`, and `tests/native_head_to_head_tests.rs::head_to_head_flex_main_size_line_collection_uses_outer_hypothetical_main_with_negative_margin`.
- CSS Flexbox §9.3 exact-fit zero-sized flex item requires zero-sized flex items to remain on a previous exactly filled line: `tests/flex_layout_tests.rs::flex_wrap_collects_zero_sized_item_after_exact_fit_on_same_line`, `tests/native_head_to_head_tests.rs::head_to_head_flex_wrap_collects_zero_sized_item_after_exact_fit_on_same_line`, and `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_flex_wrap_zero_sized_item_after_exact_fit`.
- CSS Flexbox §9.3 repeat line collection continues until every flex item belongs to a line: `tests/flex_layout_tests.rs::flex_wrap_collects_items_into_multiple_lines`, `tests/flex_layout_tests.rs::flex_main_size_resolves_flexible_lengths_per_line_independently`, and `tests/native_head_to_head_tests.rs::head_to_head_flex_wrap_collects_items_into_multiple_lines`.
- CSS Flexbox §9.3 resolve flexible lengths calls the §9.7 flexible-length algorithm for all flex items, independently per flex line: `tests/flex_layout_tests.rs::flex_main_size_resolves_flexible_lengths_per_line_independently`, `tests/flex_layout_tests.rs::min_width_freezes_item_during_flex_shrink`, `tests/flex_layout_tests.rs::max_width_freezes_item_and_redistributes_flex_grow_space`, and `tests/native_head_to_head_tests.rs::head_to_head_flex_main_size_resolves_flexible_lengths_per_line_independently`.

Remaining §9.3 audit gap:
- Forced breaks from CSS fragmentation are not represented in the Rust style model, so line collection cannot yet stop at a forced break as described by §10 Fragmenting Flex Layout.

## W3C Cross Size Clause Trace

Source clauses:
- CSS Flexbox §9.4 hypothetical cross size lays each item out as an in-flow block-level box using its used main size and treating auto as fit-content: `tests/flex_layout_tests.rs::flex_cross_size_hypothetical_cross_layout_uses_used_main_size` and `tests/native_head_to_head_tests.rs::head_to_head_flex_cross_size_hypothetical_cross_layout_uses_used_main_size`.
- CSS Flexbox §9.4 single-line definite cross size makes the line cross size the flex container inner cross size: `tests/flex_layout_tests.rs::stretched_flex_item_relayouts_percent_height_child_with_definite_cross_size`, `tests/flex_layout_tests.rs::stretched_flex_item_with_aspect_ratio_keeps_flexed_main_size_and_uses_line_cross_size`, and `tests/native_head_to_head_tests.rs::head_to_head_stretched_flex_item_with_aspect_ratio_keeps_flexed_main_size_and_uses_line_cross_size`.
- CSS Flexbox §9.4 baseline cross-size contribution sums the largest baseline-to-cross-start and baseline-to-cross-end distances for baseline-aligned items: `tests/flex_layout_tests.rs::flex_cross_size_baseline_line_size_uses_largest_baseline_distances`, `tests/flex_layout_tests.rs::flex_row_align_self_baseline_triggers_baseline_line_sizing`, and `tests/native_head_to_head_tests.rs::head_to_head_flex_cross_size_baseline_line_size_uses_largest_baseline_distances`.
- CSS Flexbox §9.4 largest outer hypothetical cross size determines non-baseline line cross size candidates: `tests/flex_layout_tests.rs::flex_wrap_collects_items_into_multiple_lines`, `tests/flex_layout_tests.rs::align_content_stretch_expands_wrapped_line_cross_sizes`, and `tests/native_head_to_head_tests.rs::head_to_head_align_content_stretch_expands_wrapped_line_cross_sizes`.
- CSS Flexbox §9.4 single-line min/max cross clamp is covered by `tests/flex_layout_tests.rs::single_line_min_cross_size_clamps_line_before_cross_alignment`, `tests/native_head_to_head_tests.rs::head_to_head_single_line_min_cross_size_clamps_line_before_cross_alignment`, and `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_single_line_min_cross_size_clamp`.
- CSS Flexbox §9.4 align-content stretch line expansion distributes positive definite cross-axis free space across flex lines: `tests/flex_layout_tests.rs::align_content_stretch_expands_wrapped_line_cross_sizes`, `tests/native_head_to_head_tests.rs::head_to_head_align_content_stretch_expands_wrapped_line_cross_sizes`, and `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_align_content_stretch_line_expansion`.
- CSS Flexbox §9.4 visibility collapse struts use `Style::visibility`, record the first-round line cross size in collapse-strut storage, restart flex layout, collect collapsed items with zero main size, ignore collapsed items for the rest of layout, and apply struts after second-round line cross-size calculation: `tests/flex_layout_tests.rs::flex_visibility_collapse_preserves_line_cross_strut_and_removes_main_space` and `tests/flex_layout_tests.rs::flex_visibility_collapse_restarts_line_collection_with_zero_main_size`.
- CSS Flexbox §9.4 stretched flex item used cross size uses the flex line cross size, clamped by used min/max cross sizes: `tests/flex_layout_tests.rs::stretched_flex_item_cross_size_respects_min_max_constraints`, `tests/native_head_to_head_tests.rs::head_to_head_stretched_flex_item_cross_size_respects_min_max_constraints`, and `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_stretch_min_max_cross_size_clamp`.
- CSS Flexbox §9.4 stretch relayout with definite cross size reruns layout so percentage cross-size descendants resolve: `tests/flex_layout_tests.rs::stretched_flex_item_relayouts_percent_height_child_with_definite_cross_size`, `tests/native_head_to_head_tests.rs::head_to_head_stretched_flex_item_relayouts_percent_height_child_with_definite_cross_size`, and `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_stretch_percent_height_relayout`.
- CSS Flexbox §9.4 states stretch does not affect a flex item's main size even if it has a preferred aspect ratio: `tests/flex_layout_tests.rs::stretched_flex_item_with_aspect_ratio_keeps_flexed_main_size_and_uses_line_cross_size` and `tests/native_head_to_head_tests.rs::head_to_head_stretched_flex_item_with_aspect_ratio_keeps_flexed_main_size_and_uses_line_cross_size`.

Starlight stretch + aspect-ratio investigation:
- For this §9.4 stretch + preferred-aspect-ratio case, Rust follows the W3C cross-size step: stretch sets the used cross size and does not recompute the flexed main size from aspect ratio. C++/standalone results are compatibility evidence for this case, not a reason to preserve the previous Rust mismatch.
- C++ latest/standalone does support the W3C §9.4 stretch + preferred-aspect-ratio behavior. `core/services/starlight_standalone/core/src/starlight.cc::CreateDefaultLayoutConfigs` sets `SetQuirksMode(kNegativePaddingFixedVersion)` and `css_align_with_legacy_w3c_ = true`; the public standalone C API exposes physical-pixel config but no quirks/aspect/stretch toggle.
- `core/renderer/starlight/types/layout_configs.h` has flex toggles for alignment, wrap, wrap cross-size, indefinite percentage cross-size, and auto margins, but no aspect-ratio/stretch-specific switch. The verified path is therefore the latest default behavior, not an opt-in Starlight mode that Rust was missing.
- The previous Rust mismatch was in final stretch relayout: Rust set the flexed main size in the style override but did not also set the stretched used cross size, allowing `aspect-ratio` to recompute cross size from the flexed main size. Rust now writes the used cross size into the final stretch style override, matching C++ latest and W3C.

Remaining §9.4 audit gap:
- No remaining Rust flex algorithm gap. Native/standalone head-to-head collapse coverage still needs a public C++ visibility setter before parity can assert `visibility: collapse` through the existing bridge.

## `9.1 Initial Setup`
- `status`: complete
- `implementation`: `crates/starlight_layout/src/engine.rs::LayoutTree`
- `implementation`: `crates/starlight_layout/src/engine/flex.rs::layout_flex`
- `implementation`: `crates/starlight_layout/src/engine.rs::ordered_in_flow_children`
- `implementation`: `crates/starlight_layout/src/engine.rs::is_out_of_flow`
- `implementation`: `crates/starlight_layout/src/engine.rs::layout_out_of_flow_children`
- `tests`: `tests/flex_layout_tests.rs::flex_order_reorders_visual_layout_without_reordering_tree`
- `tests`: `tests/flex_layout_tests.rs::display_none_child_is_laid_out_as_zero_and_skipped_by_flex`
- `tests`: `tests/flex_layout_tests.rs::flex_layout_uses_external_text_layout_trait_for_content_size_and_baseline`
- `tests`: `tests/flex_layout_tests.rs::flex_initial_setup_adapter_generates_anonymous_items_for_contiguous_text_runs`
- `tests`: `tests/flex_layout_tests.rs::flex_initial_setup_skips_out_of_flow_children_but_positions_static_rect`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_absolute_flex_child_uses_static_position_without_participating_in_flex_layout`
- `notes`: no remaining §9.1 layout-engine gap; DOM/text embedders must preserve the tested adapter contract when generating anonymous text flex items.

## `9.2 Line Length Determination`
- `status`: partial
- `implementation`: `crates/starlight_layout/src/engine/flex.rs::layout_flex`
- `implementation`: `crates/starlight_layout/src/engine/flex.rs::flex_basis`
- `implementation`: `crates/starlight_layout/src/engine/flex.rs::content_axis_constraint_from_border_axis`
- `implementation`: `crates/starlight_layout/src/engine/flex.rs::default_child_constraints_for_parent`
- `implementation`: `crates/starlight_layout/src/engine/flex.rs::clamp_flex_item_main`
- `implementation`: `crates/starlight_layout/src/engine/flex.rs::collect_flex_lines`
- `tests`: `tests/flex_layout_tests.rs::flex_wrap_collects_items_into_multiple_lines`
- `tests`: `tests/flex_layout_tests.rs::flex_nowrap_at_most_main_axis_shrinks_to_items`
- `tests`: `tests/flex_layout_tests.rs::flex_line_length_available_main_space_uses_inner_content_box_for_auto_basis`
- `tests`: `tests/flex_layout_tests.rs::flex_line_length_definite_flex_basis_overrides_main_size_property`
- `tests`: `tests/flex_layout_tests.rs::flex_line_length_aspect_ratio_uses_definite_cross_size_for_content_basis`
- `tests`: `tests/flex_layout_tests.rs::flex_line_length_hypothetical_main_size_clamps_min_before_wrapping`
- `tests`: `tests/flex_layout_tests.rs::flex_line_length_hypothetical_main_size_clamps_max_before_wrapping`
- `tests`: `tests/flex_layout_tests.rs::flex_line_length_auto_container_main_size_uses_max_content_sum`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_line_length_available_main_space_uses_inner_content_box_for_auto_basis`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_line_length_definite_flex_basis_overrides_main_size_property`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_line_length_aspect_ratio_uses_definite_cross_size_for_content_basis`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_line_length_hypothetical_main_size_clamps_min_before_wrapping`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_line_length_hypothetical_main_size_clamps_max_before_wrapping`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_line_length_auto_container_main_size_uses_max_content_sum`
- `gap`: available-space, definite flex-basis, aspect-ratio flex base, content measurement into available space, hypothetical main-size clamp, and auto main-size sizing are now traced with Rust and C++ head-to-head tests. Remaining §9.2 gaps are separate min-content/max-content constraint layout modes and the orthogonal-flow max-content branch.

## `9.3 Main Size Determination`
- `status`: partial
- `implementation`: `crates/starlight_layout/src/engine/flex.rs::layout_flex`
- `implementation`: `crates/starlight_layout/src/engine/flex.rs::collect_flex_lines`
- `implementation`: `crates/starlight_layout/src/engine/flex.rs::resolve_flexible_lengths`
- `implementation`: `crates/starlight_layout/src/engine/flex.rs::compute_elastic_item_sizes`
- `tests`: `tests/flex_layout_tests.rs::flex_main_size_nowrap_collects_all_items_into_single_line_even_when_overflowing`
- `tests`: `tests/flex_layout_tests.rs::flex_main_size_wrap_collects_oversized_first_item_alone`
- `tests`: `tests/flex_layout_tests.rs::flex_main_size_line_collection_uses_outer_hypothetical_main_with_negative_margin`
- `tests`: `tests/flex_layout_tests.rs::flex_main_size_resolves_flexible_lengths_per_line_independently`
- `tests`: `tests/flex_layout_tests.rs::flex_wrap_collects_items_into_multiple_lines`
- `tests`: `tests/flex_layout_tests.rs::flex_wrap_collects_zero_sized_item_after_exact_fit_on_same_line`
- `tests`: `tests/flex_layout_tests.rs::min_width_freezes_item_during_flex_shrink`
- `tests`: `tests/flex_layout_tests.rs::max_width_freezes_item_and_redistributes_flex_grow_space`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_main_size_nowrap_collects_all_items_into_single_line_even_when_overflowing`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_main_size_wrap_collects_oversized_first_item_alone`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_main_size_line_collection_uses_outer_hypothetical_main_with_negative_margin`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_main_size_resolves_flexible_lengths_per_line_independently`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_wrap_collects_items_into_multiple_lines`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_wrap_collects_zero_sized_item_after_exact_fit_on_same_line`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_flex_wrap_zero_sized_item_after_exact_fit`
- `gap`: single-line collection, multi-line collection until overflow, oversized first-item handling, outer hypothetical main-size line collection, exact-fit zero-sized items, repeated collection, and per-line flexible-length resolution now have W3C-traced Rust and C++ coverage. Forced breaks from CSS fragmentation remain unsupported because no break/fragmentation style is represented in the Rust layout model.

## `9.4 Cross Size Determination`
- `status`: complete
- `implementation`: `crates/starlight_layout/src/engine/flex.rs::layout_flex`
- `implementation`: `crates/starlight_layout/src/engine/flex.rs::layout_flex_with_collapse_struts`
- `implementation`: `crates/starlight_layout/src/engine/flex.rs::flex_line_cross_considering_baseline`
- `implementation`: `crates/starlight_layout/src/engine/flex.rs::flex_child_style_override`
- `tests`: `tests/flex_layout_tests.rs::flex_cross_size_hypothetical_cross_layout_uses_used_main_size`
- `tests`: `tests/flex_layout_tests.rs::flex_cross_size_baseline_line_size_uses_largest_baseline_distances`
- `tests`: `tests/flex_layout_tests.rs::flex_stretch_column_rtl_offsets_using_final_stretched_cross_size`
- `tests`: `tests/flex_layout_tests.rs::flex_row_baseline_aligns_items_by_fallback_border_box_baseline`
- `tests`: `tests/flex_layout_tests.rs::flex_wrap_reverse_places_first_line_at_cross_end`
- `tests`: `tests/flex_layout_tests.rs::flex_wrap_reverse_reverses_space_between_line_distribution`
- `tests`: `tests/flex_layout_tests.rs::single_line_min_cross_size_clamps_line_before_cross_alignment`
- `tests`: `tests/flex_layout_tests.rs::align_content_stretch_expands_wrapped_line_cross_sizes`
- `tests`: `tests/flex_layout_tests.rs::stretched_flex_item_relayouts_percent_height_child_with_definite_cross_size`
- `tests`: `tests/flex_layout_tests.rs::stretched_flex_item_cross_size_respects_min_max_constraints`
- `tests`: `tests/flex_layout_tests.rs::stretched_flex_item_with_aspect_ratio_keeps_flexed_main_size_and_uses_line_cross_size`
- `tests`: `tests/flex_layout_tests.rs::flex_visibility_collapse_preserves_line_cross_strut_and_removes_main_space`
- `tests`: `tests/flex_layout_tests.rs::flex_visibility_collapse_restarts_line_collection_with_zero_main_size`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_cross_size_hypothetical_cross_layout_uses_used_main_size`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_cross_size_baseline_line_size_uses_largest_baseline_distances`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_single_line_min_cross_size_clamps_line_before_cross_alignment`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_align_content_stretch_expands_wrapped_line_cross_sizes`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_stretched_flex_item_relayouts_percent_height_child_with_definite_cross_size`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_stretched_flex_item_cross_size_respects_min_max_constraints`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_stretched_flex_item_with_aspect_ratio_keeps_flexed_main_size_and_uses_line_cross_size`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_wrap_reverse_reverses_space_between_line_distribution`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_single_line_min_cross_size_clamp`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_align_content_stretch_line_expansion`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_stretch_percent_height_relayout`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_stretch_min_max_cross_size_clamp`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_flex_wrap_reverse_space_between_lines`
- `notes`: hypothetical cross layout with used main size, baseline line cross-size contribution, single-line definite cross sizing, single-line min/max cross clamp, align-content stretch line expansion, visibility-collapse struts plus second-round layout, stretched item percent-child relayout, stretch min/max cross-size clamp, stretch plus aspect-ratio main-size preservation, baseline fallback, and direct wrap-reverse line distribution have Rust coverage. Native/standalone collapse parity is blocked on a public C++ visibility setter, not on Rust flex algorithm behavior.

## `9.5 Main-Axis Alignment`
- `status`: partial
- `implementation`: `crates/starlight_layout/src/engine/flex.rs::flex_justify_interval`
- `implementation`: `crates/starlight_layout/src/engine/flex.rs::flex_used_margin`
- `tests`: `tests/flex_layout_tests.rs::main_axis_auto_margin_consumes_remaining_space_before_justify_content`
- `tests`: `tests/flex_layout_tests.rs::multiple_main_axis_auto_margins_share_positive_free_space_before_justify_content`
- `tests`: `tests/flex_layout_tests.rs::main_axis_auto_margin_without_positive_free_space_zeroes_margins_then_justify_content`
- `tests`: `tests/flex_layout_tests.rs::justify_content_space_evenly_distributes_free_space`
- `tests`: `tests/flex_layout_tests.rs::justify_content_space_evenly_single_item_uses_equal_edge_spaces`
- `tests`: `tests/flex_layout_tests.rs::justify_content_space_between_single_item_falls_back_to_flex_start`
- `tests`: `tests/flex_layout_tests.rs::justify_content_space_around_single_item_falls_back_to_center`
- `tests`: `tests/flex_layout_tests.rs::justify_content_center_uses_negative_free_space_when_items_overflow`
- `tests`: `tests/flex_layout_tests.rs::justify_content_space_between_keeps_gap_when_items_overflow`
- `tests`: `tests/flex_layout_tests.rs::justify_content_space_around_centers_overflow_and_keeps_gap`
- `tests`: `tests/flex_layout_tests.rs::justify_content_main_axis_direction_matrix_places_items`
- `tests`: `tests/flex_layout_tests.rs::justify_content_negative_free_space_direction_matrix_uses_w3c_fallbacks`
- `tests`: `tests/flex_layout_tests.rs::justify_content_gap_overflow_direction_matrix_preserves_gap_after_fallback`
- `tests`: `tests/flex_layout_tests.rs::main_axis_auto_margin_direction_matrix_consumes_free_space`
- `tests`: `tests/flex_layout_tests.rs::flex_grow_sum_below_one_leaves_remaining_space_for_justify_content`
- `tests`: `tests/flex_layout_tests.rs::flex_shrink_sum_below_one_leaves_negative_space_for_justify_content`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_main_axis_auto_margin_without_positive_free_space_zeroes_margins_then_justify_content`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_multiple_main_axis_auto_margins_share_positive_free_space`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_justify_content_space_evenly_single_item_equal_edge_spaces`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_justify_content_space_between_single_item_fallback`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_justify_content_space_around_single_item_fallback`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_justify_content_main_axis_direction_matrix`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_main_axis_auto_margin_direction_matrix`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_justify_content_gap_overflow_direction_matrix`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_multiple_main_axis_auto_margins`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_space_evenly_single_item_distribution`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_space_between_single_item_fallback`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_space_around_single_item_fallback`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_flex_justify_content_direction_matrix`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_flex_main_axis_auto_margin_direction_matrix`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_flex_justify_content_gap_overflow_direction_matrix`
- `gap`: Rust now traces the Flexbox §8.1/§8.2/§9.5 core clauses to layout tests, including positive and non-positive main-axis auto margin resolution, distributed alignment, single-item fallbacks, no-gap negative free-space fallback, gap overflow behavior, and row/row-reverse/column/column-reverse combined with LTR/RTL. Remaining partial status is compatibility tracking rather than missing Rust Flexbox logic: `space-evenly` and standalone `start`/`end` are local Box Alignment extensions beyond the Flexbox §8.2 value grammar, and the C++ baseline still has a no-gap negative-free-space TODO for `space-between`/`space-around`, so Rust's W3C no-gap fallback is intentionally covered by Rust layout tests rather than native head-to-head.

## `9.6 Cross-Axis Alignment`
- `status`: partial
- `implementation`: `crates/starlight_layout/src/engine/flex.rs::flex_align_cross_offset`
- `implementation`: `crates/starlight_layout/src/engine/flex.rs::flex_used_margin`
- `implementation`: `crates/starlight_layout/src/engine/flex.rs::flex_physical_cross_offset_from_logical_border_start`
- `tests`: `tests/flex_layout_tests.rs::align_self_overrides_container_align_items`
- `tests`: `tests/flex_layout_tests.rs::align_items_start_end_alias_flex_edges_for_items`
- `tests`: `tests/flex_layout_tests.rs::align_items_cross_axis_direction_and_wrap_reverse_matrix_places_items`
- `tests`: `tests/flex_layout_tests.rs::align_items_stretch_only_stretches_auto_cross_size_items_without_auto_margins`
- `tests`: `tests/flex_layout_tests.rs::cross_axis_auto_margin_overrides_stretch_alignment`
- `tests`: `tests/flex_layout_tests.rs::single_cross_axis_auto_margins_absorb_positive_free_space`
- `tests`: `tests/flex_layout_tests.rs::paired_cross_axis_auto_margins_center_item`
- `tests`: `tests/flex_layout_tests.rs::cross_axis_auto_margin_direction_and_wrap_reverse_matrix_resolves_margins`
- `tests`: `tests/flex_layout_tests.rs::overflowing_cross_axis_auto_margins_place_overflow_at_cross_end`
- `tests`: `tests/flex_layout_tests.rs::overflowing_cross_axis_auto_margin_direction_matrix_overflows_cross_end`
- `tests`: `tests/flex_layout_tests.rs::flex_row_align_self_baseline_triggers_baseline_line_sizing`
- `tests`: `tests/flex_layout_tests.rs::flex_column_container_baseline_uses_first_item_baseline_after_main_axis_alignment`
- `tests`: `tests/flex_layout_tests.rs::flex_wrap_reverse_reverses_space_between_line_distribution`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_single_cross_axis_auto_margins_absorb_positive_free_space`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_column_container_baseline_uses_first_item_baseline_after_main_axis_alignment`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_align_self_overrides_container_align_items`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_cross_axis_auto_margin_overrides_stretch_alignment`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_paired_cross_axis_auto_margins_center_item`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_overflowing_cross_axis_auto_margins_place_overflow_at_cross_end`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_row_align_self_baseline_triggers_baseline_line_sizing`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_wrap_reverse_reverses_space_between_line_distribution`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_align_self_override`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_cross_axis_auto_margin_over_stretch`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_paired_cross_axis_auto_margins`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_flex_align_self_baseline_wrap_margins`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_flex_wrap_reverse_space_between_lines`
- `gap`: §8.3/§8.5/§9.6 cross-axis alignment clauses are now traced to layout tests, including align-self override, start/end aliases, stretch eligibility, single and paired cross-axis auto margins, overflow placement, row/column baseline export, and row/row-reverse/column/column-reverse combined with LTR/RTL and wrap-reverse. Rust now applies the W3C cross-start/cross-end swap for wrap-reverse with XOR direction composition, so column RTL wrap-reverse aligns from the swapped physical side instead of preserving the original RTL cross front. Overflow placement keeps Rust/W3C behavior (`margin-bottom: -20` in the simple row case), while the native/standalone C++ baseline reports `0` for that resolved margin; that getter divergence remains a separate compatibility decision. A full `native_head_to_head_tests flex` run also still has unrelated percent-basis/fr flex parity failures outside §9.6.

## `9.7 Resolving Flexible Lengths`
- `status`: complete
- `implementation`: `crates/starlight_layout/src/engine/flex.rs::resolve_flexible_lengths`
- `implementation`: `crates/starlight_layout/src/engine/flex.rs::used_flex_factor_is_grow`
- `implementation`: `crates/starlight_layout/src/engine/flex.rs::compute_elastic_item_sizes`
- `tests`: `../starlight_layout/src/engine/flex.rs::used_flex_factor_is_grow_only_when_hypothetical_sum_is_less_than_container`
- `tests`: `../starlight_layout/src/engine/flex.rs::unfrozen_items_start_from_flex_base_size_before_distribution`
- `tests`: `../starlight_layout/src/engine/flex.rs::initial_free_space_uses_frozen_targets_unfrozen_bases_outer_sizes_and_gap`
- `tests`: `../starlight_layout/src/engine/flex.rs::distributes_positive_free_space_by_flex_grow`
- `tests`: `../starlight_layout/src/engine/flex.rs::distributes_negative_free_space_by_scaled_flex_shrink_factor`
- `tests`: `../starlight_layout/src/engine/flex.rs::grow_factor_sum_below_one_leaves_part_of_positive_free_space`
- `tests`: `../starlight_layout/src/engine/flex.rs::shrink_factor_sum_below_one_leaves_part_of_negative_free_space`
- `tests`: `../starlight_layout/src/engine/flex.rs::freezes_min_violations_and_recomputes_remaining_shrink_space`
- `tests`: `../starlight_layout/src/engine/flex.rs::freezes_max_violations_and_recomputes_remaining_grow_space`
- `tests`: `../starlight_layout/src/engine/flex.rs::clamps_negative_inner_main_sizes_to_zero_before_freezing_min_violations`
- `tests`: `../starlight_layout/src/engine/flex.rs::freezes_inflexible_items_to_hypothetical_main_size`
- `tests`: `../starlight_layout/src/engine/flex.rs::shrink_mode_freezes_items_with_base_smaller_than_hypothetical_size`
- `tests`: `tests/flex_layout_tests.rs::fit_content_min_width_freezes_item_during_flex_shrink`
- `tests`: `tests/flex_layout_tests.rs::min_width_above_flex_basis_freezes_shrinking_item_to_hypothetical_main_size`
- `tests`: `tests/flex_layout_tests.rs::multiple_min_width_violations_freeze_before_redistributing_flex_shrink_space`
- `tests`: `tests/flex_layout_tests.rs::max_width_below_flex_basis_freezes_growing_item_to_hypothetical_main_size`
- `tests`: `tests/flex_layout_tests.rs::zero_flex_grow_freezes_item_before_distributing_positive_free_space`
- `tests`: `tests/flex_layout_tests.rs::min_width_violation_freezes_item_during_flex_grow_and_restarts_distribution`
- `tests`: `tests/flex_layout_tests.rs::main_axis_gap_reduces_free_space_before_flex_grow_distribution`
- `tests`: `tests/flex_layout_tests.rs::percent_max_width_freezes_item_and_redistributes_flex_grow_space`
- `tests`: `tests/flex_layout_tests.rs::multiple_max_width_violations_freeze_before_redistributing_flex_grow_space`
- `tests`: `tests/flex_layout_tests.rs::flex_grow_sum_below_one_leaves_remaining_space_for_justify_content`
- `tests`: `tests/flex_layout_tests.rs::flex_shrink_sum_below_one_leaves_negative_space_for_justify_content`
- `tests`: `tests/flex_layout_tests.rs::flex_shrink_distribution_is_scaled_by_flex_base_size`
- `tests`: `tests/flex_layout_tests.rs::flex_shrink_negative_inner_size_is_floored_after_outer_margins`
- `tests`: `tests/flex_layout_tests.rs::all_zero_flex_grow_items_freeze_and_leave_space_for_justify_content`
- `tests`: `tests/flex_layout_tests.rs::zero_flex_shrink_freezes_item_before_distributing_negative_free_space`
- `tests`: `tests/flex_layout_tests.rs::max_width_violation_freezes_item_during_flex_shrink_and_restarts_distribution`
- `tests`: `tests/flex_layout_tests.rs::column_max_content_max_height_does_not_cap_flex_grow_space`
- `tests`: `tests/flex_layout_tests.rs::row_reverse_flex_grow_freeze_places_flexed_items_from_right_edge`
- `tests`: `tests/flex_layout_tests.rs::column_reverse_flex_shrink_freeze_places_flexed_items_from_bottom_edge`
- `tests`: `tests/flex_layout_tests.rs::flexible_lengths_resolve_independently_per_wrapped_line`
- `tests`: `tests/flex_layout_tests.rs::flexible_lengths_direction_matrix_places_resolved_main_sizes`
- `tests`: `tests/flex_layout_tests.rs::measured_flex_basis_grow_max_width_violation_restarts_distribution`
- `tests`: `tests/flex_layout_tests.rs::measured_flex_basis_shrink_min_width_violation_restarts_distribution`
- `tests`: `tests/flex_layout_tests.rs::nested_intrinsic_flex_basis_grow_max_width_violation_restarts_distribution`
- `tests`: `tests/flex_layout_tests.rs::nested_intrinsic_flex_basis_shrink_min_width_violation_restarts_distribution`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_min_width_above_basis_freezes_shrinking_item_to_hypothetical_main_size`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_multiple_min_width_violations_freeze_before_redistributing_flex_shrink_space`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_max_width_below_basis_freezes_growing_item_to_hypothetical_main_size`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_zero_grow_freezes_item_before_distributing_positive_free_space`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_min_width_violation_freezes_item_during_grow_and_restarts_distribution`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_main_axis_gap_reduces_free_space_before_grow_distribution`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_all_zero_grow_items_leave_space_for_justify_content`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_shrink_distribution_is_scaled_by_flex_base_size`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_shrink_negative_inner_size_is_floored_after_outer_margins`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_multiple_max_width_violations_freeze_before_redistributing_flex_grow_space`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_zero_shrink_freezes_item_before_distributing_negative_free_space`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_max_width_violation_freezes_item_during_shrink_and_restarts_distribution`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_column_percent_min_height_freezes_item_during_flex_shrink`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_column_fit_content_min_height_freezes_item_during_flex_shrink`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_column_fit_content_min_height_without_argument_does_not_freeze_item`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_column_percent_max_height_freezes_item_and_redistributes_flex_grow_space`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_column_fit_content_max_height_freezes_item_and_redistributes_flex_grow_space`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_column_max_content_max_height_does_not_cap_flex_grow_space`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_row_reverse_flex_grow_freeze_places_flexed_items_from_right_edge`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_column_reverse_flex_shrink_freeze_places_flexed_items_from_bottom_edge`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flexible_lengths_resolve_independently_per_wrapped_line`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flexible_lengths_direction_matrix_places_resolved_main_sizes`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_measured_flex_basis_grow_max_width_violation_restarts_distribution`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_measured_flex_basis_shrink_min_width_violation_restarts_distribution`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_nested_intrinsic_flex_basis_grow_max_width_violation_restarts_distribution`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_nested_intrinsic_flex_basis_shrink_min_width_violation_restarts_distribution`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_flex_min_max_freeze_distribution`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_flexible_lengths_direction_mapping`
- `notes`: 0 post-resolve percent-basis adjustment functions remain. The W3C §9.7 loop is traced step-by-step at the solver level: used flex-factor selection, target-main initialization from flex base, inflexible item freezing, initial free-space calculation from frozen targets/unfrozen bases/outer sizes/gaps, grow distribution, scaled-shrink distribution, flex-factor sums below one, min/max clamp violations, negative inner main-size flooring, violation-driven freeze/restart, and used main-size writeback into layout. Layout, native C++, and standalone C API coverage additionally cover row and column main axes, reverse/RTL placement after flexible-length resolution, independent per-line resolution, percentage/fit-content/max-content min/max forms, non-capping fit-content-without-argument behavior, measured natural main-size flex bases, and nested intrinsic main-size flex bases in grow/shrink freeze-restart loops. The remaining full-suite native mismatches involving percent flex-basis / `fr` descendants are tracked under §9.8 because they depend on definite/indefinite size propagation into descendant layout, not on §9.7 free-space distribution.

## `9.8 Definite and Indefinite Sizes`
- `status`: partial
- `implementation`: `crates/starlight_layout/src/engine/flex.rs::FlexPercentBase`
- `implementation`: `crates/starlight_layout/src/engine.rs::FlexNodeContext`
- `implementation`: `crates/starlight_layout/src/engine/flex.rs::FlexBasisResult`
- `implementation`: `crates/starlight_layout/src/engine/flex.rs::post_flex_main_defines_percent_base`
- `tests`: `tests/flex_layout_tests.rs::definite_flex_basis_post_flexing_main_size_defines_descendant_percent_flex_basis_base`
- `tests`: `tests/flex_layout_tests.rs::definite_container_main_size_defines_auto_basis_item_descendant_percent_flex_basis_base`
- `tests`: `tests/flex_layout_tests.rs::unresolved_percent_flex_basis_does_not_define_descendant_percent_flex_basis_base`
- `tests`: `tests/flex_layout_tests.rs::resolved_flex_line_cross_size_defines_nested_flex_percent_basis_base`
- `tests`: `tests/flex_layout_tests.rs::column_flex_item_percent_cross_size_and_aspect_ratio_define_main_basis`
- `tests`: `tests/flex_layout_tests.rs::root_flex_fit_content_percent_argument_caps_final_width`
- `tests`: `tests/flex_layout_tests.rs::root_flex_fit_content_calc_argument_caps_final_width`
- `tests`: `tests/flex_layout_tests.rs::root_column_flex_fit_content_percent_argument_caps_final_height`
- `tests`: `tests/flex_layout_tests.rs::root_column_flex_fit_content_calc_argument_caps_final_height`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_flex_preserved_percent_basis_parent_defines_growing_percent_child_base`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_column_flex_item_percent_cross_size_and_aspect_ratio_define_main_basis`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_root_flex_fit_content_percent_argument_caps_final_width`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_root_flex_fit_content_calc_argument_caps_final_width`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_root_column_flex_fit_content_percent_argument_caps_final_height`
- `tests`: `tests/native_head_to_head_tests.rs::head_to_head_root_column_flex_fit_content_calc_argument_caps_final_height`
- `tests`: `tests/standalone_head_to_head_tests.rs::standalone_owned_tree_matches_cpp_for_definite_indefinite_flex_size_matrix`
- `gap`: post-flexing main-size propagation into descendant percentage flex-basis is now tied to the §9.8 definite-size clauses instead of flex-basis syntax: a definite flex container main size or a flex-basis property that resolves to a definite value propagates the final main size as a percentage base, while an unresolved percentage flex-basis does not. Rust layout coverage now exercises the positive definite-container and definite-basis cases, the unresolved-percent negative case, and resolved flex-line cross size as a definite base for nested flex percentage flex-basis. Percent cross-size propagation into aspect-ratio main basis and root fit-content percent/calc arguments have layout-level, native parity, and standalone C API parity coverage across row and column axes. Remaining §9.8 work is a broader W3C audit of nested percentage descendants and mixed at-most/indefinite owner constraints. Directed native C++ parity now passes for the growing unresolved-percent descendant case, but C++ still diverges from Rust's §9.8 interpretation in `head_to_head_flex_auto_main_preserves_intrinsic_percent_basis_child` by preserving intrinsic fallback despite a definite parent main size, and in `head_to_head_flex_preserved_percent_basis_parent_defines_inflexible_percent_basis_and_main_size_child_base` by resolving a descendant percent width against an unresolved-percent parent's post-flex size. `Length::fr` native parity tests are ignored for the Flexbox audit because `fr` is a Lynx extension outside the W3C Flexbox algorithm.

## `9.9 Intrinsic Sizes`
- `status`: partial
- `implementation`: `crates/starlight_layout/src/engine/flex.rs::flex_basis`
- `tests`: `tests/flex_layout_tests.rs::flex_item_fit_content_width_uses_natural_main_axis_size`
- `tests`: `tests/flex_layout_tests.rs::flex_basis_fit_content_argument_resolves_before_measuring_item`
- `gap`: 0 FlexBasisFallbacks flags remain; intrinsic sizing still needs a direct W3C-derived audit.
