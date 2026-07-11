// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use super::*;

#[derive(Clone, Copy, Debug, PartialEq)]
struct FlexResolveItem {
    pub flex_base_size: f32,
    pub hypothetical_main_size: f32,
    pub flex_grow: f32,
    pub flex_shrink: f32,
    pub outer_non_main_size: f32,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
enum FlexFreezeReason {
    Inflexible,
    MinViolation,
    MaxViolation,
}

#[derive(Clone, Copy, Debug, PartialEq)]
struct FlexFreezeEvent {
    pub item_index: usize,
    pub reason: FlexFreezeReason,
}

#[derive(Clone, Debug, PartialEq)]
struct FlexResolveTrace {
    pub initial_free_space: f32,
    pub freeze_events: Vec<FlexFreezeEvent>,
}

#[derive(Clone, Debug, PartialEq)]
struct FlexResolveResult {
    pub target_main_sizes: Vec<f32>,
    pub remaining_free_space: f32,
    pub frozen: Vec<bool>,
    pub trace: FlexResolveTrace,
}

#[derive(Clone, Copy, Debug)]
struct FlexResolveInput<'a> {
    pub items: &'a [FlexResolveItem],
    pub available_main_space: f32,
    pub main_axis_gap: f32,
    pub is_grow: bool,
    pub epsilon: f32,
}

#[derive(Clone, Debug)]
struct FlexInternalVariables {
    initial_total_elastic_factor: f32,
    total_elastic_factor: f32,
    total_scaled_elastic_factor: f32,
    initial_free_space: f32,
    remaining_space: f32,
}

fn compute_elastic_item_sizes(
    input: FlexResolveInput<'_>,
    mut clamp: impl FnMut(usize, f32) -> f32,
) -> FlexResolveResult {
    let mut target_main_sizes = vec![0.0; input.items.len()];
    let mut frozen = vec![false; input.items.len()];
    let mut variables = initial_variables(input);
    let mut freeze_events = Vec::new();

    for (idx, item) in input.items.iter().enumerate() {
        let factor = elastic_factor(item, input.is_grow);
        let is_inflexible = factor <= input.epsilon
            || (input.is_grow && item.flex_base_size > item.hypothetical_main_size)
            || (!input.is_grow && item.flex_base_size < item.hypothetical_main_size);
        if is_inflexible {
            freeze_item(idx, input, &mut variables, &mut frozen);
            target_main_sizes[idx] = item.hypothetical_main_size;
            freeze_events.push(FlexFreezeEvent {
                item_index: idx,
                reason: FlexFreezeReason::Inflexible,
            });
        } else {
            target_main_sizes[idx] = item.flex_base_size;
        }
    }

    let mut free_space = calculate_remaining_space(input, &target_main_sizes, &frozen);
    variables.initial_free_space = free_space;
    variables.remaining_space = free_space;

    loop {
        let step = resolve_one_line(
            input,
            &mut variables,
            &mut target_main_sizes,
            &mut frozen,
            free_space,
            &mut clamp,
        );

        match step {
            ResolveStep::Done => break,
            ResolveStep::Freeze { reason, items } => {
                for idx in items {
                    freeze_item(idx, input, &mut variables, &mut frozen);
                    freeze_events.push(FlexFreezeEvent {
                        item_index: idx,
                        reason,
                    });
                }
                free_space = calculate_remaining_space(input, &target_main_sizes, &frozen);
            }
        }
    }

    FlexResolveResult {
        target_main_sizes,
        remaining_free_space: variables.remaining_space,
        frozen,
        trace: FlexResolveTrace {
            initial_free_space: variables.initial_free_space,
            freeze_events,
        },
    }
}

fn used_flex_factor_is_grow(
    outer_hypothetical_main_sum: f32,
    container_inner_main_size: f32,
    epsilon: f32,
) -> bool {
    outer_hypothetical_main_sum + epsilon < container_inner_main_size
}

fn initial_variables(input: FlexResolveInput<'_>) -> FlexInternalVariables {
    let mut total_elastic_factor = 0.0;
    let mut total_scaled_elastic_factor = 0.0;

    for item in input.items {
        let factor = elastic_factor(item, input.is_grow);
        if factor > input.epsilon {
            total_elastic_factor += factor;
            if !input.is_grow {
                total_scaled_elastic_factor += factor * item.flex_base_size;
            }
        }
    }

    FlexInternalVariables {
        initial_total_elastic_factor: total_elastic_factor,
        total_elastic_factor,
        total_scaled_elastic_factor,
        initial_free_space: 0.0,
        remaining_space: 0.0,
    }
}

fn calculate_remaining_space(
    input: FlexResolveInput<'_>,
    target_main_sizes: &[f32],
    frozen: &[bool],
) -> f32 {
    debug_assert_eq!(input.items.len(), target_main_sizes.len());
    debug_assert_eq!(input.items.len(), frozen.len());

    let mut remaining = input.available_main_space;
    for (idx, item) in input.items.iter().enumerate() {
        let main_size = if frozen[idx] {
            target_main_sizes[idx]
        } else {
            item.flex_base_size
        };
        remaining -= main_size + item.outer_non_main_size;
    }
    remaining - gap_total(input.main_axis_gap, input.items.len())
}

fn freeze_item(
    idx: usize,
    input: FlexResolveInput<'_>,
    variables: &mut FlexInternalVariables,
    frozen: &mut [bool],
) {
    if frozen[idx] {
        return;
    }

    frozen[idx] = true;
    let item = input.items[idx];
    let factor = elastic_factor(&item, input.is_grow);
    if factor > input.epsilon {
        variables.total_elastic_factor -= factor;
        if !input.is_grow {
            variables.total_scaled_elastic_factor -= factor * item.flex_base_size;
        }
    }
}

enum ResolveStep {
    Done,
    Freeze {
        reason: FlexFreezeReason,
        items: Vec<usize>,
    },
}

fn resolve_one_line(
    input: FlexResolveInput<'_>,
    variables: &mut FlexInternalVariables,
    target_main_sizes: &mut [f32],
    frozen: &mut [bool],
    mut free_space: f32,
    clamp: &mut impl FnMut(usize, f32) -> f32,
) -> ResolveStep {
    let mut total_violations = 0.0;
    let mut min_violations = Vec::new();
    let mut max_violations = Vec::new();
    let mut used_space = 0.0;

    variables.remaining_space = free_space;

    let adjusted_free_space =
        variables.initial_free_space * variables.total_elastic_factor_for_adjustment();
    if adjusted_free_space.abs() < free_space.abs() {
        free_space = adjusted_free_space;
    }

    for (idx, item) in input.items.iter().enumerate() {
        if frozen[idx] {
            continue;
        }

        let mut raw_main_size = item.flex_base_size;
        if input.is_grow
            && free_space > input.epsilon
            && variables.total_elastic_factor > input.epsilon
        {
            raw_main_size +=
                (item.flex_grow.max(0.0) / variables.total_elastic_factor) * free_space;
        } else if free_space < -input.epsilon
            && variables.total_scaled_elastic_factor > input.epsilon
        {
            raw_main_size += (item.flex_shrink.max(0.0) * item.flex_base_size
                / variables.total_scaled_elastic_factor)
                * free_space;
        }

        let adjusted_main_size = clamp(idx, raw_main_size);
        target_main_sizes[idx] = adjusted_main_size;
        used_space += adjusted_main_size - item.flex_base_size;

        let violation = adjusted_main_size - raw_main_size;
        total_violations += violation;
        if violation > input.epsilon {
            min_violations.push(idx);
        } else if violation < -input.epsilon {
            max_violations.push(idx);
        }
    }

    if total_violations.abs() <= input.epsilon {
        variables.remaining_space -= used_space;
        for frozen in frozen.iter_mut() {
            *frozen = true;
        }
        return ResolveStep::Done;
    }

    if total_violations > 0.0 {
        ResolveStep::Freeze {
            reason: FlexFreezeReason::MinViolation,
            items: min_violations,
        }
    } else {
        ResolveStep::Freeze {
            reason: FlexFreezeReason::MaxViolation,
            items: max_violations,
        }
    }
}

impl FlexInternalVariables {
    fn total_elastic_factor_for_adjustment(&self) -> f32 {
        self.total_elastic_factor
            .min(self.initial_total_elastic_factor)
    }
}

fn elastic_factor(item: &FlexResolveItem, is_grow: bool) -> f32 {
    if is_grow {
        item.flex_grow.max(0.0)
    } else {
        item.flex_shrink.max(0.0)
    }
}

fn gap_total(gap: f32, item_count: usize) -> f32 {
    gap * item_count.saturating_sub(1) as f32
}

#[cfg(test)]
mod flex_resolve_tests {
    use super::*;

    const EPSILON: f32 = 0.0001;

    fn item(
        flex_base_size: f32,
        hypothetical_main_size: f32,
        flex_grow: f32,
        flex_shrink: f32,
        outer_non_main_size: f32,
    ) -> FlexResolveItem {
        FlexResolveItem {
            flex_base_size,
            hypothetical_main_size,
            flex_grow,
            flex_shrink,
            outer_non_main_size,
        }
    }

    #[test]
    fn used_flex_factor_is_grow_only_when_hypothetical_sum_is_less_than_container() {
        assert!(used_flex_factor_is_grow(99.0, 100.0, EPSILON));
        assert!(!used_flex_factor_is_grow(100.0, 100.0, EPSILON));
        assert!(!used_flex_factor_is_grow(
            100.0,
            100.0 + EPSILON / 2.0,
            EPSILON
        ));
        assert!(!used_flex_factor_is_grow(101.0, 100.0, EPSILON));
    }

    #[test]
    fn unfrozen_items_start_from_flex_base_size_before_distribution() {
        let items = [
            item(20.0, 30.0, 1.0, 1.0, 0.0),
            item(40.0, 40.0, 1.0, 1.0, 0.0),
        ];

        let result = compute_elastic_item_sizes(
            FlexResolveInput {
                items: &items,
                available_main_space: 60.0,
                main_axis_gap: 0.0,
                is_grow: true,
                epsilon: EPSILON,
            },
            |_, size| size,
        );

        assert_eq!(result.target_main_sizes, vec![20.0, 40.0]);
        assert!((result.trace.initial_free_space - 0.0).abs() <= EPSILON);
        assert!(result.trace.freeze_events.is_empty());
    }

    #[test]
    fn distributes_positive_free_space_by_flex_grow() {
        let items = [
            FlexResolveItem {
                flex_base_size: 20.0,
                hypothetical_main_size: 20.0,
                flex_grow: 1.0,
                flex_shrink: 1.0,
                outer_non_main_size: 0.0,
            },
            FlexResolveItem {
                flex_base_size: 20.0,
                hypothetical_main_size: 20.0,
                flex_grow: 3.0,
                flex_shrink: 1.0,
                outer_non_main_size: 0.0,
            },
        ];

        let result = compute_elastic_item_sizes(
            FlexResolveInput {
                items: &items,
                available_main_space: 120.0,
                main_axis_gap: 0.0,
                is_grow: true,
                epsilon: EPSILON,
            },
            |_, size| size,
        );

        assert_eq!(result.target_main_sizes, vec![40.0, 80.0]);
        assert!((result.remaining_free_space - 0.0).abs() <= EPSILON);
        assert_eq!(result.frozen, vec![true, true]);
        assert!(result.trace.freeze_events.is_empty());
    }

    #[test]
    fn initial_free_space_uses_frozen_targets_unfrozen_bases_outer_sizes_and_gap() {
        let items = [
            item(20.0, 20.0, 0.0, 1.0, 3.0),
            item(10.0, 10.0, 1.0, 1.0, 2.0),
            item(30.0, 30.0, 1.0, 1.0, 5.0),
        ];

        let result = compute_elastic_item_sizes(
            FlexResolveInput {
                items: &items,
                available_main_space: 100.0,
                main_axis_gap: 4.0,
                is_grow: true,
                epsilon: EPSILON,
            },
            |_, size| size,
        );

        assert_eq!(result.target_main_sizes, vec![20.0, 21.0, 41.0]);
        assert!((result.trace.initial_free_space - 22.0).abs() <= EPSILON);
        assert!((result.remaining_free_space - 0.0).abs() <= EPSILON);
        assert_eq!(
            result.trace.freeze_events,
            vec![FlexFreezeEvent {
                item_index: 0,
                reason: FlexFreezeReason::Inflexible,
            }]
        );
    }

    #[test]
    fn grow_factor_sum_below_one_leaves_part_of_positive_free_space() {
        let items = [
            FlexResolveItem {
                flex_base_size: 10.0,
                hypothetical_main_size: 10.0,
                flex_grow: 0.25,
                flex_shrink: 1.0,
                outer_non_main_size: 0.0,
            },
            FlexResolveItem {
                flex_base_size: 10.0,
                hypothetical_main_size: 10.0,
                flex_grow: 0.25,
                flex_shrink: 1.0,
                outer_non_main_size: 0.0,
            },
        ];

        let result = compute_elastic_item_sizes(
            FlexResolveInput {
                items: &items,
                available_main_space: 100.0,
                main_axis_gap: 0.0,
                is_grow: true,
                epsilon: EPSILON,
            },
            |_, size| size,
        );

        assert_eq!(result.target_main_sizes, vec![30.0, 30.0]);
        assert!((result.remaining_free_space - 40.0).abs() <= EPSILON);
        assert_eq!(result.frozen, vec![true, true]);
        assert!(result.trace.freeze_events.is_empty());
    }

    #[test]
    fn shrink_factor_sum_below_one_leaves_part_of_negative_free_space() {
        let items = [
            FlexResolveItem {
                flex_base_size: 50.0,
                hypothetical_main_size: 50.0,
                flex_grow: 0.0,
                flex_shrink: 0.25,
                outer_non_main_size: 0.0,
            },
            FlexResolveItem {
                flex_base_size: 50.0,
                hypothetical_main_size: 50.0,
                flex_grow: 0.0,
                flex_shrink: 0.25,
                outer_non_main_size: 0.0,
            },
        ];

        let result = compute_elastic_item_sizes(
            FlexResolveInput {
                items: &items,
                available_main_space: 80.0,
                main_axis_gap: 0.0,
                is_grow: false,
                epsilon: EPSILON,
            },
            |_, size| size,
        );

        assert_eq!(result.target_main_sizes, vec![45.0, 45.0]);
        assert!((result.remaining_free_space + 10.0).abs() <= EPSILON);
        assert_eq!(result.frozen, vec![true, true]);
        assert!(result.trace.freeze_events.is_empty());
    }

    #[test]
    fn distributes_negative_free_space_by_scaled_flex_shrink_factor() {
        let items = [
            item(100.0, 100.0, 0.0, 1.0, 0.0),
            item(50.0, 50.0, 0.0, 1.0, 0.0),
        ];

        let result = compute_elastic_item_sizes(
            FlexResolveInput {
                items: &items,
                available_main_space: 120.0,
                main_axis_gap: 0.0,
                is_grow: false,
                epsilon: EPSILON,
            },
            |_, size| size,
        );

        assert_eq!(result.target_main_sizes, vec![80.0, 40.0]);
        assert!((result.remaining_free_space - 0.0).abs() <= EPSILON);
        assert!(result.trace.freeze_events.is_empty());
    }

    #[test]
    fn freezes_min_violations_and_recomputes_remaining_shrink_space() {
        let items = [
            FlexResolveItem {
                flex_base_size: 80.0,
                hypothetical_main_size: 80.0,
                flex_grow: 0.0,
                flex_shrink: 1.0,
                outer_non_main_size: 0.0,
            },
            FlexResolveItem {
                flex_base_size: 40.0,
                hypothetical_main_size: 40.0,
                flex_grow: 0.0,
                flex_shrink: 1.0,
                outer_non_main_size: 0.0,
            },
        ];

        let result = compute_elastic_item_sizes(
            FlexResolveInput {
                items: &items,
                available_main_space: 90.0,
                main_axis_gap: 0.0,
                is_grow: false,
                epsilon: EPSILON,
            },
            |idx, size| {
                if idx == 0 {
                    size.max(70.0)
                } else {
                    size
                }
            },
        );

        assert_eq!(result.target_main_sizes, vec![70.0, 20.0]);
        assert!((result.remaining_free_space - 0.0).abs() <= EPSILON);
        assert_eq!(result.frozen, vec![true, true]);
        assert_eq!(
            result.trace.freeze_events,
            vec![FlexFreezeEvent {
                item_index: 0,
                reason: FlexFreezeReason::MinViolation,
            }]
        );
    }

    #[test]
    fn freezes_max_violations_and_recomputes_remaining_grow_space() {
        let items = [
            item(20.0, 20.0, 1.0, 1.0, 0.0),
            item(20.0, 20.0, 1.0, 1.0, 0.0),
        ];

        let result = compute_elastic_item_sizes(
            FlexResolveInput {
                items: &items,
                available_main_space: 100.0,
                main_axis_gap: 0.0,
                is_grow: true,
                epsilon: EPSILON,
            },
            |idx, size| {
                if idx == 0 {
                    size.min(30.0)
                } else {
                    size
                }
            },
        );

        assert_eq!(result.target_main_sizes, vec![30.0, 70.0]);
        assert!((result.remaining_free_space - 0.0).abs() <= EPSILON);
        assert_eq!(result.frozen, vec![true, true]);
        assert_eq!(
            result.trace.freeze_events,
            vec![FlexFreezeEvent {
                item_index: 0,
                reason: FlexFreezeReason::MaxViolation,
            }]
        );
    }

    #[test]
    fn clamps_negative_inner_main_sizes_to_zero_before_freezing_min_violations() {
        let items = [
            item(10.0, 10.0, 0.0, 1.0, 20.0),
            item(10.0, 10.0, 0.0, 1.0, 20.0),
        ];

        let result = compute_elastic_item_sizes(
            FlexResolveInput {
                items: &items,
                available_main_space: 0.0,
                main_axis_gap: 0.0,
                is_grow: false,
                epsilon: EPSILON,
            },
            |_, size| size.max(0.0),
        );

        assert_eq!(result.target_main_sizes, vec![0.0, 0.0]);
        assert!((result.trace.initial_free_space + 60.0).abs() <= EPSILON);
        assert!((result.remaining_free_space + 40.0).abs() <= EPSILON);
        assert_eq!(
            result.trace.freeze_events,
            vec![
                FlexFreezeEvent {
                    item_index: 0,
                    reason: FlexFreezeReason::MinViolation,
                },
                FlexFreezeEvent {
                    item_index: 1,
                    reason: FlexFreezeReason::MinViolation,
                },
            ]
        );
    }

    #[test]
    fn freezes_inflexible_items_to_hypothetical_main_size() {
        let items = [
            FlexResolveItem {
                flex_base_size: 30.0,
                hypothetical_main_size: 25.0,
                flex_grow: 0.0,
                flex_shrink: 1.0,
                outer_non_main_size: 0.0,
            },
            FlexResolveItem {
                flex_base_size: 20.0,
                hypothetical_main_size: 20.0,
                flex_grow: 1.0,
                flex_shrink: 1.0,
                outer_non_main_size: 0.0,
            },
        ];

        let result = compute_elastic_item_sizes(
            FlexResolveInput {
                items: &items,
                available_main_space: 65.0,
                main_axis_gap: 0.0,
                is_grow: true,
                epsilon: EPSILON,
            },
            |_, size| size,
        );

        assert_eq!(result.target_main_sizes, vec![25.0, 40.0]);
        assert_eq!(result.frozen, vec![true, true]);
        assert_eq!(
            result.trace.freeze_events,
            vec![FlexFreezeEvent {
                item_index: 0,
                reason: FlexFreezeReason::Inflexible,
            }]
        );
    }

    #[test]
    fn shrink_mode_freezes_items_with_base_smaller_than_hypothetical_size() {
        let items = [
            item(30.0, 40.0, 1.0, 1.0, 0.0),
            item(60.0, 60.0, 1.0, 1.0, 0.0),
        ];

        let result = compute_elastic_item_sizes(
            FlexResolveInput {
                items: &items,
                available_main_space: 80.0,
                main_axis_gap: 0.0,
                is_grow: false,
                epsilon: EPSILON,
            },
            |_, size| size,
        );

        assert_eq!(result.target_main_sizes, vec![40.0, 40.0]);
        assert_eq!(result.frozen, vec![true, true]);
        assert_eq!(
            result.trace.freeze_events,
            vec![FlexFreezeEvent {
                item_index: 0,
                reason: FlexFreezeReason::Inflexible,
            }]
        );
    }
}

struct FlexPhysicalCrossOffsetInput {
    logical_border_offset: f32,
    content_size: f32,
    item_border_size: f32,
    margin: Edges,
    parent_is_row: bool,
    reversed: bool,
}

#[derive(Clone, Copy)]
struct FlexAlignCrossOffsetInput {
    align_items: AlignItems,
    line_cross_offset: f32,
    content_cross: f32,
    child_cross: f32,
    margin: Edges,
    parent_is_row: bool,
    reverse_cross: bool,
}

#[derive(Clone, Copy, Debug)]
struct FlexUsedMarginContext {
    item_cross: f32,
    line_cross: f32,
    is_row: bool,
    reverse_main: bool,
    reverse_cross: bool,
    auto_main_margin: Option<f32>,
}

#[derive(Clone, Debug)]
struct FlexItem<N> {
    id: N,
    style: Style,
    style_override: Option<Style>,
    constraints: Constraints,
    edges: ResolvedEdges,
    base_main: f32,
    hypothetical_main: f32,
    target_main: f32,
    post_flex_main_defines_percent_base: bool,
    suppress_flexible_min: bool,
    basis_measure: Option<CachedLayoutMeasure>,
    collapsed: bool,
    collapse_strut_cross: Option<f32>,
    cross: f32,
    measured: Option<LayoutBox>,
    baseline_from_top_margin_edge: f32,
    baseline_from_cross_start_margin_edge: f32,
    baseline_from_cross_start_border_edge: f32,
}

#[derive(Clone, Copy, Debug)]
struct FlexBasisResult {
    main: f32,
    measure: Option<CachedLayoutMeasure>,
    post_flex_main_defines_percent_base: bool,
}

impl FlexBasisResult {
    const fn resolved_flex_basis(main: f32) -> Self {
        Self {
            main,
            measure: None,
            post_flex_main_defines_percent_base: true,
        }
    }

    const fn derived(main: f32) -> Self {
        Self {
            main,
            measure: None,
            post_flex_main_defines_percent_base: false,
        }
    }
}

#[derive(Clone, Copy, Debug)]
struct CachedLayoutMeasure {
    constraints: Constraints,
    size: Size,
    main_percent_base_was_unresolved: bool,
}

#[derive(Clone, Copy, Debug)]
struct FlexCacheAxisCheck {
    current: SideConstraint,
    last: SideConstraint,
    last_size: f32,
    is_horizontal: bool,
}

#[derive(Clone, Copy, Debug, Default)]
struct FlexItemBaselineOffsets {
    from_cross_start_margin_edge: f32,
    from_cross_start_border_edge: f32,
}

#[derive(Clone, Copy, Debug)]
struct FlexContainerBaselineContext<'a> {
    container_style: &'a Style,
    main_gap: f32,
    is_row: bool,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
enum FlexPercentBaseSource {
    DefiniteContainerMain,
    NoDefiniteContainerMain,
    SuppressedForUnresolvedParent,
}

#[derive(Clone, Copy, Debug, PartialEq)]
struct FlexPercentBase {
    value: Option<f32>,
    source: FlexPercentBaseSource,
}

impl FlexPercentBase {
    const fn from_layout_state(content_main: Option<f32>, suppressed: bool) -> Self {
        if suppressed {
            Self {
                value: None,
                source: FlexPercentBaseSource::SuppressedForUnresolvedParent,
            }
        } else if let Some(value) = content_main {
            Self {
                value: Some(value),
                source: FlexPercentBaseSource::DefiniteContainerMain,
            }
        } else {
            Self {
                value: None,
                source: FlexPercentBaseSource::NoDefiniteContainerMain,
            }
        }
    }

    const fn value(self) -> Option<f32> {
        self.value
    }
}

#[derive(Clone, Copy, Debug)]
struct FlexBasisContext {
    is_row: bool,
    flex_basis_percent_base: FlexPercentBase,
    content_main_limit: Option<f32>,
    content_cross_limit: Option<f32>,
    parent_constraints: Constraints,
    edges: ResolvedEdges,
}

#[derive(Clone, Copy, Debug)]
struct FlexLine {
    start: usize,
    end: usize,
    remaining_main: f32,
    cross: f32,
    baseline: f32,
}

impl FlexLine {
    fn new(start: usize, end: usize) -> Self {
        Self {
            start,
            end,
            remaining_main: 0.0,
            cross: 0.0,
            baseline: 0.0,
        }
    }

    fn len(self) -> usize {
        self.end - self.start
    }
}

#[derive(Clone, Copy, Debug, Default, PartialEq)]
struct FlexBaselineCross {
    baseline: f32,
    cross: f32,
}

impl LayoutEngine {
    pub(super) fn layout_flex<T: LayoutTree>(
        &mut self,
        tree: &mut T,
        style: &Style,
        children: &[T::NodeId],
        context: NodeLayoutContext,
    ) -> LayoutOutput {
        self.layout_flex_with_collapse_struts(tree, style, children, context, None)
    }

    fn layout_flex_with_collapse_struts<T: LayoutTree>(
        &mut self,
        tree: &mut T,
        style: &Style,
        children: &[T::NodeId],
        context: NodeLayoutContext,
        collapse_struts: Option<&[Option<f32>]>,
    ) -> LayoutOutput {
        let constraints = context.constraints;
        let edges = context.edges;
        let rounding = context.rounding;
        let flex_percent = context.flex.percent;
        let requested_suppress_flex_basis_percent_base =
            flex_percent.suppress_flex_basis_percent_base;
        let suppress_main_size_percent_base = flex_percent.suppress_main_size_percent_base;
        let collapse_second_round = collapse_struts.is_some();
        let is_row = style.flex_direction.is_row();
        let reverse_main = self.flex_main_front_is_reversed(style, is_row);
        let cross_front_reversed = self.flex_cross_front_is_reversed(style, is_row);
        let fixed_main = self.resolve_border_axis(
            if is_row { style.width } else { style.height },
            if is_row {
                Axis::Horizontal
            } else {
                Axis::Vertical
            },
            constraints,
            edges,
            style.box_sizing,
        );
        let fixed_cross = self.resolve_border_axis(
            if is_row { style.height } else { style.width },
            if is_row {
                Axis::Vertical
            } else {
                Axis::Horizontal
            },
            constraints,
            edges,
            style.box_sizing,
        );
        let (fixed_main, fixed_cross) =
            self.apply_aspect_ratio_to_main_cross(style, edges, fixed_main, fixed_cross, is_row);
        let main_axis = if is_row {
            Axis::Horizontal
        } else {
            Axis::Vertical
        };
        let cross_axis = if is_row {
            Axis::Vertical
        } else {
            Axis::Horizontal
        };
        let fixed_main =
            fixed_main.map(|size| self.clamp_axis(style, main_axis, size, constraints, edges));
        let fixed_cross =
            fixed_cross.map(|size| self.clamp_axis(style, cross_axis, size, constraints, edges));
        let main_axis_constraint = self.axis_constraint(constraints, is_row);
        let main_axis_is_definite = fixed_main.is_some() || main_axis_constraint.is_definite();
        let cross_axis_constraint = self.axis_constraint(constraints, !is_row);
        let cross_axis_ignores_at_most =
            fixed_cross.is_none() && cross_axis_constraint.is_at_most();
        let content_main_definite_limit = fixed_main
            .or_else(|| {
                main_axis_constraint
                    .is_definite()
                    .then_some(main_axis_constraint.size)
            })
            .map(|size| self.inner_axis(size, is_row, edges));
        let content_main_bound = fixed_main
            .or_else(|| main_axis_constraint.bounded_size())
            .map(|size| self.inner_axis(size, is_row, edges));
        let suppress_flex_basis_percent_base = requested_suppress_flex_basis_percent_base;
        let flex_basis_percent_base = FlexPercentBase::from_layout_state(
            content_main_definite_limit,
            suppress_flex_basis_percent_base,
        );
        let content_main_percent_limit = if suppress_main_size_percent_base {
            None
        } else {
            content_main_definite_limit
        };
        let content_cross_limit = fixed_cross
            .or_else(|| {
                cross_axis_constraint
                    .is_definite()
                    .then_some(cross_axis_constraint.size)
            })
            .map(|size| self.inner_axis(size, !is_row, edges));
        let content_cross_bound = fixed_cross
            .or_else(|| cross_axis_constraint.bounded_size())
            .map(|size| self.inner_axis(size, !is_row, edges));
        let content_cross_constraint = self.content_axis_constraint_from_border_axis(
            fixed_cross,
            cross_axis_constraint,
            !is_row,
            edges,
        );
        let flex_basis_fixed_main = if suppress_flex_basis_percent_base {
            None
        } else {
            fixed_main
        };
        let flex_basis_main_axis_constraint = if suppress_flex_basis_percent_base {
            SideConstraint::indefinite()
        } else {
            main_axis_constraint
        };
        let flex_basis_parent_constraints = self.content_constraints_from_optional_border_axes(
            flex_basis_fixed_main,
            fixed_cross,
            flex_basis_main_axis_constraint,
            cross_axis_constraint,
            is_row,
            edges,
        );
        self.layout_display_none_children(tree, children, edges, rounding);
        let visible_children = self.ordered_in_flow_children(&*tree, children);

        let main_gap = {
            let fallback = self.resolve_gap(style, is_row, constraints).unwrap_or(0.0);
            let content_main_gap_base = if suppress_flex_basis_percent_base {
                None
            } else {
                content_main_definite_limit.or(content_main_bound)
            };
            content_main_gap_base.map_or(fallback, |content_size| {
                self.resolve_gap_with_content_size(style, is_row, content_size, fallback)
            })
        };
        let mut items = Vec::with_capacity(visible_children.len());

        for (child_index, child) in visible_children.into_iter().enumerate() {
            let child_style = tree.style(child).clone();
            let collapsed = child_style.visibility == Visibility::Collapse;
            let collapse_strut_cross =
                collapse_struts.and_then(|struts| struts.get(child_index).copied().flatten());
            let child_edges = self.resolve_edges_for_parent(
                &child_style,
                self.axis_constraint_from_optional(content_main_percent_limit),
            );
            let basis = self.flex_basis(
                tree,
                child,
                &child_style,
                FlexBasisContext {
                    is_row,
                    flex_basis_percent_base,
                    content_main_limit: content_main_percent_limit,
                    content_cross_limit,
                    parent_constraints: flex_basis_parent_constraints,
                    edges: child_edges,
                },
            );
            let hypothetical_main = self.clamp_flex_item_main(
                &child_style,
                child_edges,
                basis.main,
                is_row,
                content_main_definite_limit,
                false,
            );
            let base_main = if collapse_second_round && collapsed {
                0.0
            } else {
                basis.main
            };
            let hypothetical_main = if collapse_second_round && collapsed {
                0.0
            } else {
                hypothetical_main
            };
            items.push(FlexItem {
                id: child,
                style: child_style,
                style_override: None,
                constraints: Constraints::indefinite(),
                edges: child_edges,
                base_main,
                hypothetical_main,
                target_main: hypothetical_main,
                post_flex_main_defines_percent_base: main_axis_is_definite
                    || basis.post_flex_main_defines_percent_base,
                suppress_flexible_min: false,
                basis_measure: basis.measure,
                collapsed,
                collapse_strut_cross,
                cross: 0.0,
                measured: None,
                baseline_from_top_margin_edge: 0.0,
                baseline_from_cross_start_margin_edge: 0.0,
                baseline_from_cross_start_border_edge: 0.0,
            });
        }

        let natural_main = self.line_hypothetical_main(&items, main_gap, is_row);
        let line_collection_main = if main_axis_is_definite {
            content_main_bound.unwrap_or(natural_main)
        } else if main_axis_constraint.is_at_most() {
            content_main_bound
                .map(|bound| natural_main.min(bound))
                .unwrap_or(natural_main)
        } else {
            natural_main
        }
        .max(0.0);
        let mut lines = self.collect_flex_lines(
            &items,
            style.flex_wrap,
            line_collection_main,
            main_gap,
            is_row,
        );
        let unclamped_content_main = if main_axis_constraint.is_at_most()
            && !main_axis_is_definite
            && style.flex_wrap != FlexWrap::NoWrap
        {
            lines
                .iter()
                .map(|line| {
                    self.line_hypothetical_main(&items[line.start..line.end], main_gap, is_row)
                        .min(line_collection_main)
                })
                .fold(0.0, f32::max)
        } else {
            line_collection_main
        }
        .max(0.0);
        let content_main = self.clamp_content_axis(
            style,
            if is_row {
                Axis::Horizontal
            } else {
                Axis::Vertical
            },
            unclamped_content_main,
            constraints,
            edges,
        );

        let flex_resolve_content_main = if main_axis_constraint.is_at_most()
            && !main_axis_is_definite
            && style.flex_wrap != FlexWrap::NoWrap
        {
            unclamped_content_main
        } else {
            content_main
        };
        for line in &mut lines {
            let line_items = &mut items[line.start..line.end];
            line.remaining_main = self.resolve_flexible_lengths(
                line_items,
                flex_resolve_content_main,
                main_gap,
                is_row,
            );
        }
        let child_parent_constraints = if is_row {
            Constraints::new(
                SideConstraint::definite(content_main),
                content_cross_constraint,
            )
        } else {
            Constraints::new(
                content_cross_constraint,
                SideConstraint::definite(content_main),
            )
        };

        let cross_gap = if lines.len() > 1 {
            let fallback = self.resolve_gap(style, !is_row, constraints).unwrap_or(0.0);
            content_cross_bound.map_or(fallback, |content_size| {
                self.resolve_gap_with_content_size(style, !is_row, content_size, fallback)
            })
        } else {
            0.0
        };

        for line in &mut lines {
            let mut max_outer_cross: f32 = 0.0;
            for item in &mut items[line.start..line.end] {
                if collapse_second_round && item.collapsed {
                    item.constraints = Constraints::indefinite();
                    item.measured = Some(LayoutBox::new(
                        Size::ZERO,
                        item.edges.margin,
                        None,
                        Constraints::indefinite(),
                    ));
                    item.cross = 0.0;
                    item.baseline_from_top_margin_edge = 0.0;
                    item.baseline_from_cross_start_margin_edge = 0.0;
                    item.baseline_from_cross_start_border_edge = 0.0;
                    continue;
                }
                let child_cross = self.resolve_child_cross_size(
                    &item.style,
                    is_row,
                    content_cross_limit,
                    item.edges,
                );
                let child_cross_constraint = self.flex_child_cross_constraint(
                    &item.style,
                    child_cross,
                    child_parent_constraints,
                    item.edges,
                    is_row,
                );
                let child_constraints =
                    self.child_flex_constraints(item.target_main, child_cross_constraint, is_row);
                let cross_constraint_defines_percent_flex_basis_base = style.align_items
                    == AlignItems::Stretch
                    && item.style.align_self.is_none()
                    && !(item
                        .basis_measure
                        .is_some_and(|measure| measure.main_percent_base_was_unresolved)
                        && Self::length_contains_percentage(item.style.flex_basis))
                    && self
                        .axis_constraint(child_constraints, !is_row)
                        .is_definite()
                    && (self.flex_main_axis_has_non_shrinking_percent_flex_basis_descendant(
                        &*tree,
                        item.id,
                        &item.style,
                        !is_row,
                    ) || (item.style.flex_basis.is_flexible()
                        && self
                            .flex_main_axis_has_default_aligned_shrinking_percent_flex_basis_child(
                                &*tree,
                                item.id,
                                &item.style,
                                !is_row,
                            )));
                let child_flex_main_had_unresolved_percent_basis =
                    item.basis_measure.is_some_and(|measure| {
                        measure.main_percent_base_was_unresolved
                            && self.child_flex_main_tracks_parent_main(&item.style, is_row)
                            && (self.cached_flex_basis_measure_matches(
                                &*tree,
                                item.id,
                                &item.style,
                                measure,
                                self.apply_min_max_to_constraints(
                                    &item.style,
                                    child_constraints,
                                    item.edges,
                                ),
                            ) || (self.main_size(measure.size, is_row) - item.target_main)
                                .abs()
                                <= self.epsilon)
                    });
                let child_flex_main_inherits_unresolved_parent = self
                    .child_flex_main_inherits_unresolved_parent_main(
                        &*tree,
                        item.id,
                        &item.style,
                        is_row,
                        main_axis_is_definite,
                    );
                let child_main_length = Self::style_main_axis_length(&item.style, is_row);
                let child_main_defines_percent_main_size_base =
                    self.main_length_defines_percent_main_size_base(
                        child_main_length,
                        item.style.flex_shrink,
                    ) && self.child_flex_main_tracks_parent_main(&item.style, is_row);
                let suppress_child_flex_basis_percent =
                    (child_flex_main_had_unresolved_percent_basis
                        || child_flex_main_inherits_unresolved_parent)
                        && !cross_constraint_defines_percent_flex_basis_base;
                let suppress_child_main_size_percent = (child_flex_main_had_unresolved_percent_basis
                    || child_flex_main_inherits_unresolved_parent)
                    && !child_main_defines_percent_main_size_base
                    && !cross_constraint_defines_percent_flex_basis_base;
                let child_style = self.flex_child_style_override(
                    &item.style,
                    item.edges,
                    item.target_main,
                    child_cross,
                    is_row,
                    child_parent_constraints,
                );
                let measured = self.layout_node_with_edges(
                    tree,
                    item.id,
                    Some(child_style.clone()),
                    NodeLayoutContext {
                        constraints: child_constraints,
                        offset: Point::ZERO,
                        sticky_constraints: child_constraints,
                        edges: item.edges,
                        rounding,
                        flex: FlexNodeContext::from_parts(FlexPercentPropagation::new(
                            suppress_child_flex_basis_percent,
                            suppress_child_main_size_percent,
                        )),
                    },
                );
                item.style_override = Some(child_style);
                item.constraints = child_constraints;
                item.measured = Some(measured);
                item.cross = self.cross_size(measured.size, is_row);
                let baseline_offsets =
                    self.flex_item_baseline_offsets(measured, item.edges, is_row);
                item.baseline_from_top_margin_edge =
                    self.item_baseline_from_top_margin_edge(measured, item.edges);
                item.baseline_from_cross_start_margin_edge =
                    baseline_offsets.from_cross_start_margin_edge;
                item.baseline_from_cross_start_border_edge =
                    baseline_offsets.from_cross_start_border_edge;
                max_outer_cross =
                    max_outer_cross.max(item.cross + self.axis_margin(item.edges.margin, !is_row));
            }
            let baseline_cross = self.flex_line_cross_considering_baseline(
                &items[line.start..line.end],
                style.align_items,
                is_row,
            );
            line.baseline = baseline_cross.baseline;
            line.cross = if style.flex_wrap == FlexWrap::NoWrap {
                content_cross_limit.unwrap_or(max_outer_cross.max(baseline_cross.cross))
            } else {
                max_outer_cross.max(baseline_cross.cross)
            }
            .max(0.0);
        }
        if collapse_second_round {
            for line in &mut lines {
                let strut_cross = items[line.start..line.end]
                    .iter()
                    .filter_map(|item| item.collapse_strut_cross)
                    .fold(0.0, f32::max);
                line.cross = line.cross.max(strut_cross);
            }
        } else if items.iter().any(|item| item.collapsed) {
            for item in &mut items {
                if item.collapsed {
                    item.collapse_strut_cross =
                        Some(item.cross + self.axis_margin(item.edges.margin, !is_row));
                }
            }
            debug_assert!(items
                .iter()
                .filter(|item| item.collapsed)
                .all(|item| item.collapse_strut_cross.is_some()));
            let collapse_struts = items
                .iter()
                .map(|item| item.collapse_strut_cross)
                .collect::<Vec<_>>();
            return self.layout_flex_with_collapse_struts(
                tree,
                style,
                children,
                context,
                Some(&collapse_struts),
            );
        }

        let natural_cross = self.total_line_cross(&lines, cross_gap);
        let content_cross = content_cross_limit.unwrap_or(natural_cross).max(0.0);
        let cross_clamp_constraints = if cross_axis_ignores_at_most {
            let mut constraints = constraints;
            if is_row {
                constraints.height = SideConstraint::indefinite();
            } else {
                constraints.width = SideConstraint::indefinite();
            }
            constraints
        } else {
            constraints
        };
        let content_cross = self.clamp_content_axis(
            style,
            if is_row {
                Axis::Vertical
            } else {
                Axis::Horizontal
            },
            content_cross,
            cross_clamp_constraints,
            edges,
        );
        if style.flex_wrap == FlexWrap::NoWrap && !lines.is_empty() {
            lines[0].cross = content_cross;
        } else if style.align_content == AlignContent::Stretch
            && content_cross > natural_cross
            && !lines.is_empty()
        {
            let extra_cross = (content_cross - natural_cross) / lines.len() as f32;
            for line in &mut lines {
                line.cross += extra_cross;
            }
        }
        for line in &mut lines {
            line.baseline = self.flex_line_baseline(
                &items[line.start..line.end],
                line.cross,
                style.align_items,
                is_row,
            );
        }
        let line_cross_sum = lines.iter().map(|line| line.cross).sum::<f32>();
        let free_cross = (content_cross - line_cross_sum) - self.gap_total(cross_gap, lines.len());
        let (cross_start, line_interval) = if style.flex_wrap == FlexWrap::NoWrap {
            (0.0, 0.0)
        } else {
            self.flex_align_content(style.align_content, free_cross, lines.len())
        };
        let mut cross_cursor = cross_start;
        let baseline = self.flex_container_baseline(
            &items,
            &lines,
            FlexContainerBaselineContext {
                container_style: style,
                main_gap,
                is_row,
            },
        );
        let reverse_cross = cross_front_reversed ^ (style.flex_wrap == FlexWrap::WrapReverse);
        let sticky_constraints =
            self.content_constraints_from_main_cross(content_main, content_cross, is_row);

        let mut final_baseline = if is_row { None } else { baseline };
        for (line_index, line) in lines.iter().enumerate() {
            let remaining_main = line.remaining_main;
            let line_items = &items[line.start..line.end];
            let participating_len = self.flex_line_participating_len(line_items);
            let auto_main_margin_count = self.auto_margin_count(line_items, is_row);
            let auto_main_margin = if remaining_main > self.epsilon && auto_main_margin_count > 0 {
                Some(remaining_main / auto_main_margin_count as f32)
            } else {
                None
            };
            let (mut main_cursor, main_interval, item_gap) = auto_main_margin.map_or_else(
                || {
                    let (main_start, main_interval) = self.flex_justify_interval(
                        style.justify_content,
                        remaining_main,
                        participating_len,
                    );
                    (main_start - main_interval, main_interval, main_gap)
                },
                |_| (0.0, 0.0, main_gap),
            );
            let line_cross_offset = cross_cursor;
            let mut line_first_baseline: f32 = 0.0;
            let mut line_max_baseline: f32 = 0.0;
            let mut participating_item_index = 0usize;
            for item in &items[line.start..line.end] {
                let item_id = item.id;
                let item_style = item.style.clone();
                let item_edges = item.edges;
                if collapse_second_round && item.collapsed {
                    let hidden_style = Style {
                        display: Display::None,
                        ..item_style
                    };
                    self.layout_node_with_edges(
                        tree,
                        item_id,
                        Some(hidden_style),
                        NodeLayoutContext {
                            constraints: Constraints::indefinite(),
                            offset: Point::ZERO,
                            sticky_constraints,
                            edges: item_edges,
                            rounding,
                            flex: FlexNodeContext::default(),
                        },
                    );
                    continue;
                }
                let target_main = item.target_main;
                let child_flex_main_had_unresolved_percent_basis =
                    item.basis_measure.is_some_and(|measure| {
                        measure.main_percent_base_was_unresolved
                            && self.child_flex_main_tracks_parent_main(&item_style, is_row)
                            && (self.cached_flex_basis_measure_matches(
                                &*tree,
                                item_id,
                                &item_style,
                                measure,
                                self.apply_min_max_to_constraints(
                                    &item_style,
                                    item.constraints,
                                    item_edges,
                                ),
                            ) || (self.main_size(measure.size, is_row) - target_main).abs()
                                <= self.epsilon)
                    });
                let child_flex_main_inherits_unresolved_parent = self
                    .child_flex_main_inherits_unresolved_parent_main(
                        &*tree,
                        item_id,
                        &item_style,
                        is_row,
                        main_axis_is_definite,
                    );
                let align_items = item_style.align_self.unwrap_or(style.align_items);
                let child_main_length = Self::style_main_axis_length(&item_style, is_row);
                let child_main_defines_percent_main_size_base =
                    self.main_length_defines_percent_main_size_base(
                        child_main_length,
                        item_style.flex_shrink,
                    ) && self.child_flex_main_tracks_parent_main(&item_style, is_row);
                let suppress_child_flex_basis_percent =
                    (child_flex_main_had_unresolved_percent_basis
                        || child_flex_main_inherits_unresolved_parent)
                        && !item.post_flex_main_defines_percent_base;
                let suppress_child_main_size_percent = (child_flex_main_had_unresolved_percent_basis
                    || child_flex_main_inherits_unresolved_parent)
                    && !child_main_defines_percent_main_size_base
                    && !item.post_flex_main_defines_percent_base;
                let measured = item
                    .measured
                    .expect("flex item must be measured before alignment");
                let used_main = self.main_size(measured.size, is_row);
                let item_cross = item.cross;
                main_cursor += main_interval;
                let main_start_margin = self.flex_main_start_margin_with_auto(
                    &item_style,
                    item_edges.margin,
                    is_row,
                    reverse_main,
                    auto_main_margin,
                );
                let main_end_margin = self.flex_main_end_margin_with_auto(
                    &item_style,
                    item_edges.margin,
                    is_row,
                    reverse_main,
                    auto_main_margin,
                );
                let has_auto_cross_margin = self.has_axis_auto_margin(&item_style, !is_row);
                let explicit_cross = self.resolve_child_cross_size(
                    &item_style,
                    is_row,
                    content_cross_limit,
                    item_edges,
                );
                let has_explicit_cross = explicit_cross.is_some();
                let cross_size_is_auto = if is_row {
                    item_style.height == Length::Auto
                } else {
                    item_style.width == Length::Auto
                };
                let stretched_cross =
                    (line.cross - self.axis_margin(item_edges.margin, !is_row)).max(0.0);
                let stretched_constraints = self.child_flex_constraints(
                    target_main,
                    SideConstraint::definite(stretched_cross),
                    is_row,
                );
                let stretched_cross_for_alignment = self
                    .axis_constraint(
                        self.apply_min_max_to_constraints(
                            &item_style,
                            stretched_constraints,
                            item_edges,
                        ),
                        !is_row,
                    )
                    .size
                    .max(self.padding_border(
                        if is_row {
                            Axis::Vertical
                        } else {
                            Axis::Horizontal
                        },
                        item_edges,
                    ));
                let current_cross_constraint = self.axis_constraint(item.constraints, !is_row);
                let needs_final_cross_layout = align_items == AlignItems::Stretch
                    && cross_size_is_auto
                    && !has_explicit_cross
                    && !has_auto_cross_margin
                    && ((item_cross - stretched_cross).abs() > self.epsilon
                        || !current_cross_constraint.is_definite());
                let aligned_item_cross = if needs_final_cross_layout {
                    stretched_cross_for_alignment
                } else {
                    item_cross
                };
                let cross_offset =
                    if is_row && align_items == AlignItems::Baseline && !has_auto_cross_margin {
                        line_cross_offset + line.baseline
                            - item.baseline_from_cross_start_border_edge
                    } else if has_auto_cross_margin {
                        line_cross_offset
                            + self.flex_auto_cross_offset(
                                &item_style,
                                item_edges.margin,
                                line.cross,
                                aligned_item_cross,
                                is_row,
                                reverse_cross,
                            )
                    } else {
                        let align_input = FlexAlignCrossOffsetInput {
                            align_items,
                            line_cross_offset,
                            content_cross: line.cross,
                            child_cross: aligned_item_cross,
                            margin: item_edges.margin,
                            parent_is_row: is_row,
                            reverse_cross,
                        };
                        self.flex_align_cross_offset(align_input)
                    };
                let child_main_offset = self.flex_physical_offset_from_logical_margin_start(
                    main_cursor,
                    content_main,
                    used_main,
                    main_start_margin,
                    main_end_margin,
                    reverse_main,
                );
                let child_cross_offset = if style.flex_wrap == FlexWrap::WrapReverse
                    || self.axis_margin(item_edges.margin, !is_row).abs() > self.epsilon
                {
                    self.flex_physical_cross_offset_from_logical_border_start(
                        FlexPhysicalCrossOffsetInput {
                            logical_border_offset: cross_offset,
                            content_size: content_cross,
                            item_border_size: aligned_item_cross,
                            margin: item_edges.margin,
                            parent_is_row: is_row,
                            reversed: reverse_cross,
                        },
                    )
                } else {
                    self.flex_physical_offset_from_logical_border_start(
                        cross_offset,
                        content_cross,
                        aligned_item_cross,
                        reverse_cross,
                    )
                };
                let preserve_wrap_reverse_cross_offset =
                    style.flex_wrap == FlexWrap::WrapReverse && reverse_cross;
                let child_cross_parent_offset = |border: f32, padding: f32, offset: f32| {
                    if preserve_wrap_reverse_cross_offset {
                        border + padding + offset
                    } else {
                        Self::parent_border_offset(border, padding, offset)
                    }
                };
                let child_offset = if is_row {
                    Point::new(
                        Self::parent_border_offset(
                            edges.border.left,
                            edges.padding.left,
                            child_main_offset,
                        ),
                        child_cross_parent_offset(
                            edges.border.top,
                            edges.padding.top,
                            child_cross_offset,
                        ),
                    )
                } else {
                    Point::new(
                        child_cross_parent_offset(
                            edges.border.left,
                            edges.padding.left,
                            child_cross_offset,
                        ),
                        Self::parent_border_offset(
                            edges.border.top,
                            edges.padding.top,
                            child_main_offset,
                        ),
                    )
                };
                let cross_constraint = if needs_final_cross_layout {
                    Some(stretched_cross_for_alignment)
                } else {
                    explicit_cross
                };
                let used_margin = self.flex_used_margin(
                    &item_style,
                    item_edges.margin,
                    FlexUsedMarginContext {
                        item_cross: aligned_item_cross,
                        line_cross: line.cross,
                        is_row,
                        reverse_main,
                        reverse_cross,
                        auto_main_margin,
                    },
                );
                let stretched_child_constraints = self.child_flex_constraints(
                    target_main,
                    SideConstraint::definite(stretched_cross_for_alignment),
                    is_row,
                );
                let stretched_child_constraints = self.apply_aspect_ratio_to_constraints(
                    &item_style,
                    item_edges,
                    self.apply_min_max_to_constraints(
                        &item_style,
                        stretched_child_constraints,
                        item_edges,
                    ),
                );
                let has_cached_subtree = tree.children(item_id).next().is_some();
                let cached_subtree_requires_final_rounding_layout = has_cached_subtree
                    && (child_offset.x.abs() > self.epsilon || child_offset.y.abs() > self.epsilon);
                let can_reuse_stretched_measure = needs_final_cross_layout
                    && !child_flex_main_had_unresolved_percent_basis
                    && !cached_subtree_requires_final_rounding_layout
                    && self.cached_layout_measure_matches(
                        &*tree,
                        item_id,
                        &item_style,
                        measured,
                        stretched_child_constraints,
                    );
                let item_absolute = Point::new(
                    rounding.container_absolute.x + child_offset.x,
                    rounding.container_absolute.y + child_offset.y,
                );
                let item_physical_pixels_per_layout_unit =
                    tree.physical_pixels_per_layout_unit(item_id);
                let item_absolute_is_fractional = (item_absolute.x
                    - Self::round_to_pixel_grid(
                        item_absolute.x,
                        item_physical_pixels_per_layout_unit,
                    ))
                .abs()
                    > self.epsilon
                    || (item_absolute.y
                        - Self::round_to_pixel_grid(
                            item_absolute.y,
                            item_physical_pixels_per_layout_unit,
                        ))
                    .abs()
                        > self.epsilon;
                let can_reuse_positioned_measure = !needs_final_cross_layout
                    && has_cached_subtree
                    && align_items == AlignItems::Center
                    && item_absolute_is_fractional
                    && style.flex_wrap == FlexWrap::WrapReverse
                    && item_style.display == Display::Block
                    && item_style.width == Length::Auto
                    && item_style.height == Length::Auto
                    && !suppress_child_flex_basis_percent
                    && !suppress_child_main_size_percent
                    && self.cached_layout_measure_matches(
                        &*tree,
                        item_id,
                        &item_style,
                        measured,
                        item.constraints,
                    );
                let child_box = if needs_final_cross_layout {
                    let mut child_box = if can_reuse_stretched_measure {
                        let raw_layout = LayoutResult {
                            offset: child_offset,
                            size: measured.size,
                            baseline: measured.baseline,
                            padding: item_edges.padding,
                            border: item_edges.border,
                            margin: item_edges.margin,
                            sticky_pos: self.sticky_pos(&item_style, sticky_constraints),
                        };
                        let exported_layout = self.exported_layout_result(
                            raw_layout,
                            rounding,
                            item_physical_pixels_per_layout_unit,
                        );
                        self.reexport_cached_subtree(tree, item_id, raw_layout, rounding);
                        let constraints = stretched_child_constraints;
                        LayoutBox::from_raw_and_exported(raw_layout, exported_layout, constraints)
                    } else {
                        let final_cross = cross_constraint
                            .expect("stretch relayout must have a definite cross size");
                        let child_constraints = self.child_flex_constraints(
                            target_main,
                            SideConstraint::definite(final_cross),
                            is_row,
                        );
                        let final_style_cross = cross_constraint;
                        let child_style_override = self.flex_child_style_override(
                            &item_style,
                            item_edges,
                            target_main,
                            final_style_cross,
                            is_row,
                            if is_row {
                                Constraints::new(
                                    self.axis_constraint_from_optional(Some(content_main)),
                                    self.axis_constraint_from_optional(content_cross_limit),
                                )
                            } else {
                                Constraints::new(
                                    self.axis_constraint_from_optional(content_cross_limit),
                                    self.axis_constraint_from_optional(Some(content_main)),
                                )
                            },
                        );
                        self.layout_node_with_edges(
                            tree,
                            item_id,
                            Some(child_style_override),
                            NodeLayoutContext {
                                constraints: child_constraints,
                                offset: child_offset,
                                sticky_constraints,
                                edges: item_edges,
                                rounding,
                                flex: FlexNodeContext::from_parts(FlexPercentPropagation::new(
                                    suppress_child_flex_basis_percent,
                                    suppress_child_main_size_percent,
                                )),
                            },
                        )
                    };
                    child_box.layout.margin = used_margin;
                    child_box
                } else {
                    let mut child_box = if can_reuse_positioned_measure {
                        let raw_layout = LayoutResult {
                            offset: child_offset,
                            size: measured.size,
                            baseline: measured.baseline,
                            padding: item_edges.padding,
                            border: item_edges.border,
                            margin: item_edges.margin,
                            sticky_pos: self.sticky_pos(&item_style, sticky_constraints),
                        };
                        let exported_layout = self.exported_layout_result(
                            raw_layout,
                            rounding,
                            item_physical_pixels_per_layout_unit,
                        );
                        self.reexport_cached_subtree(tree, item_id, raw_layout, rounding);
                        LayoutBox::from_raw_and_exported(
                            raw_layout,
                            exported_layout,
                            item.constraints,
                        )
                    } else {
                        self.layout_node_with_edges(
                            tree,
                            item_id,
                            item.style_override.clone(),
                            NodeLayoutContext {
                                constraints: item.constraints,
                                offset: child_offset,
                                sticky_constraints,
                                edges: item_edges,
                                rounding,
                                flex: FlexNodeContext::from_parts(FlexPercentPropagation::new(
                                    suppress_child_flex_basis_percent,
                                    suppress_child_main_size_percent,
                                )),
                            },
                        )
                    };
                    child_box.layout.margin = used_margin;
                    child_box
                };
                if is_row {
                    let final_item_baseline =
                        self.item_baseline_from_top_margin_edge(child_box, item_edges);
                    if participating_item_index == 0 {
                        line_first_baseline = final_item_baseline;
                        let final_cross = self.cross_size(child_box.size, is_row);
                        let cross_margin_bound =
                            final_cross + self.axis_margin(item_edges.margin, !is_row);
                        match align_items {
                            AlignItems::FlexEnd | AlignItems::End => {
                                line_first_baseline += line.cross - cross_margin_bound;
                            }
                            AlignItems::Center => {
                                line_first_baseline += (line.cross - cross_margin_bound) / 2.0;
                            }
                            AlignItems::FlexStart
                            | AlignItems::Start
                            | AlignItems::Stretch
                            | AlignItems::Baseline => {}
                        }
                    }
                    if align_items == AlignItems::Baseline {
                        line_max_baseline = line_max_baseline.max(final_item_baseline);
                    }
                }
                tree.set_layout_with_constraints(item_id, child_box.constraints, child_box.layout);
                let item_outer_main = used_main + main_start_margin + main_end_margin;
                main_cursor += item_outer_main;
                participating_item_index += 1;
                if participating_item_index < participating_len {
                    main_cursor += item_gap;
                }
            }
            if is_row && line_index == 0 {
                final_baseline =
                    self.exported_container_baseline(Some(if line_max_baseline > self.epsilon {
                        line_max_baseline
                    } else {
                        line_first_baseline
                    }));
            }
            // Keep line size, align-content interval, and cross gap as
            // separate f32 additions for stable public output.
            cross_cursor += line.cross;
            cross_cursor += line_interval;
            cross_cursor += cross_gap;
        }

        let mut width = if is_row { content_main } else { content_cross };
        let mut height = if is_row { content_cross } else { content_main };
        width += edges.padding.horizontal() + edges.border.horizontal();
        height += edges.padding.vertical() + edges.border.vertical();

        if is_row {
            width = self.clamp_axis(style, Axis::Horizontal, width, constraints, edges);
            height = self.clamp_axis(
                style,
                Axis::Vertical,
                height,
                cross_clamp_constraints,
                edges,
            );
        } else {
            width = self.clamp_axis(
                style,
                Axis::Horizontal,
                width,
                cross_clamp_constraints,
                edges,
            );
            height = self.clamp_axis(style, Axis::Vertical, height, constraints, edges);
        }
        let size = Size::new(width.max(0.0), height.max(0.0));
        self.layout_out_of_flow_children(tree, children, style, size, edges, rounding);
        LayoutOutput {
            size,
            baseline: final_baseline.or(baseline),
        }
    }

    pub(super) fn flex_out_of_flow_alignment(
        &self,
        container_style: &Style,
        child_style: &Style,
    ) -> OutOfFlowAlignment {
        let is_row = container_style.flex_direction.is_row();
        let reverse_main = self.flex_main_front_is_reversed(container_style, is_row);
        let reverse_cross = self.flex_cross_front_is_reversed(container_style, is_row);
        let main = match container_style.justify_content {
            JustifyContent::Stretch
            | JustifyContent::FlexStart
            | JustifyContent::Start
            | JustifyContent::SpaceBetween => {
                OutOfFlowAxisAlignment::start_with_front(!reverse_main)
            }
            JustifyContent::FlexEnd | JustifyContent::End => {
                OutOfFlowAxisAlignment::end_with_front(!reverse_main)
            }
            JustifyContent::Center | JustifyContent::SpaceAround | JustifyContent::SpaceEvenly => {
                OutOfFlowAxisAlignment::center()
            }
        };
        let mut cross_position = match child_style
            .align_self
            .unwrap_or(container_style.align_items)
        {
            AlignItems::Stretch
            | AlignItems::FlexStart
            | AlignItems::Start
            | AlignItems::Baseline => OutOfFlowPosition::Start,
            AlignItems::Center => OutOfFlowPosition::Center,
            AlignItems::FlexEnd | AlignItems::End => OutOfFlowPosition::End,
        };
        if container_style.flex_wrap == FlexWrap::WrapReverse {
            cross_position = cross_position.reverse();
        }
        let cross = OutOfFlowAxisAlignment::new(cross_position, !reverse_cross);

        if is_row {
            OutOfFlowAlignment {
                horizontal: main,
                vertical: cross,
            }
        } else {
            OutOfFlowAlignment {
                horizontal: cross,
                vertical: main,
            }
        }
    }

    fn flex_justify(
        &self,
        justify_content: JustifyContent,
        free_space: f32,
        item_count: usize,
        gap: f32,
    ) -> (f32, f32) {
        let (start, interval) = self.flex_justify_interval(justify_content, free_space, item_count);
        (start, gap + interval)
    }

    fn flex_justify_interval(
        &self,
        justify_content: JustifyContent,
        free_space: f32,
        item_count: usize,
    ) -> (f32, f32) {
        if item_count == 0 {
            return (0.0, 0.0);
        }
        let negative_free_space = free_space < -self.epsilon;
        match justify_content {
            JustifyContent::Stretch | JustifyContent::FlexStart | JustifyContent::Start => {
                (0.0, 0.0)
            }
            JustifyContent::FlexEnd | JustifyContent::End => (free_space, 0.0),
            JustifyContent::Center => (free_space / 2.0, 0.0),
            JustifyContent::SpaceBetween => {
                if negative_free_space || item_count == 1 {
                    (0.0, 0.0)
                } else {
                    (0.0, free_space / (item_count - 1) as f32)
                }
            }
            JustifyContent::SpaceAround if negative_free_space || item_count == 1 => {
                (free_space / 2.0, 0.0)
            }
            JustifyContent::SpaceAround => {
                let space = free_space / item_count as f32;
                (space / 2.0, space)
            }
            JustifyContent::SpaceEvenly => {
                let space = free_space / (item_count + 1) as f32;
                (space, space)
            }
        }
    }

    fn flex_align_content(
        &self,
        align_content: AlignContent,
        free_space: f32,
        line_count: usize,
    ) -> (f32, f32) {
        if line_count == 0 {
            return (0.0, 0.0);
        }
        let negative_free_space = free_space < -self.epsilon;
        match align_content {
            AlignContent::FlexStart | AlignContent::Start | AlignContent::Stretch => (0.0, 0.0),
            AlignContent::FlexEnd | AlignContent::End => (free_space, 0.0),
            AlignContent::Center => (free_space / 2.0, 0.0),
            AlignContent::SpaceBetween => {
                if negative_free_space || line_count == 1 {
                    (0.0, 0.0)
                } else {
                    (0.0, free_space / (line_count - 1) as f32)
                }
            }
            AlignContent::SpaceAround if negative_free_space => (free_space / 2.0, 0.0),
            AlignContent::SpaceAround => {
                let space = free_space / line_count as f32;
                (space / 2.0, space)
            }
            AlignContent::SpaceEvenly => {
                let space = free_space / (line_count + 1) as f32;
                (space, space)
            }
        }
    }

    fn flex_main_start_margin_with_auto(
        &self,
        style: &Style,
        margin: Edges,
        horizontal: bool,
        reverse: bool,
        auto_margin: Option<f32>,
    ) -> f32 {
        if reverse {
            self.axis_end_margin_with_auto(style, margin, horizontal, auto_margin)
        } else {
            self.axis_start_margin_with_auto(style, margin, horizontal, auto_margin)
        }
    }

    fn flex_main_end_margin_with_auto(
        &self,
        style: &Style,
        margin: Edges,
        horizontal: bool,
        reverse: bool,
        auto_margin: Option<f32>,
    ) -> f32 {
        if reverse {
            self.axis_start_margin_with_auto(style, margin, horizontal, auto_margin)
        } else {
            self.axis_end_margin_with_auto(style, margin, horizontal, auto_margin)
        }
    }

    fn flex_auto_cross_offset(
        &self,
        style: &Style,
        margin: Edges,
        line_cross: f32,
        child_cross: f32,
        parent_is_row: bool,
        reverse_cross: bool,
    ) -> f32 {
        let (start_margin, _) = self.flex_resolved_auto_cross_margins(
            style,
            margin,
            line_cross,
            child_cross,
            parent_is_row,
            reverse_cross,
        );
        start_margin
    }

    fn flex_used_margin(
        &self,
        style: &Style,
        base_margin: Edges,
        context: FlexUsedMarginContext,
    ) -> Edges {
        let mut margin = base_margin;
        if self.axis_logical_start_margin_is_auto(style, context.is_row, context.reverse_main) {
            self.set_axis_logical_start_margin(
                &mut margin,
                context.is_row,
                context.reverse_main,
                context.auto_main_margin.unwrap_or(0.0),
            );
        }
        if self.axis_logical_end_margin_is_auto(style, context.is_row, context.reverse_main) {
            self.set_axis_logical_end_margin(
                &mut margin,
                context.is_row,
                context.reverse_main,
                context.auto_main_margin.unwrap_or(0.0),
            );
        }

        let cross_horizontal = !context.is_row;
        let start_auto =
            self.axis_logical_start_margin_is_auto(style, cross_horizontal, context.reverse_cross);
        let end_auto =
            self.axis_logical_end_margin_is_auto(style, cross_horizontal, context.reverse_cross);
        if start_auto || end_auto {
            let (resolved_start, resolved_end) = self.flex_resolved_auto_cross_margins(
                style,
                base_margin,
                context.line_cross,
                context.item_cross,
                context.is_row,
                context.reverse_cross,
            );
            self.set_axis_logical_start_margin(
                &mut margin,
                cross_horizontal,
                context.reverse_cross,
                resolved_start,
            );
            self.set_axis_logical_end_margin(
                &mut margin,
                cross_horizontal,
                context.reverse_cross,
                resolved_end,
            );
        }
        margin
    }

    fn flex_resolved_auto_cross_margins(
        &self,
        style: &Style,
        margin: Edges,
        line_cross: f32,
        item_cross: f32,
        parent_is_row: bool,
        reverse_cross: bool,
    ) -> (f32, f32) {
        let cross_horizontal = !parent_is_row;
        let start_auto =
            self.axis_logical_start_margin_is_auto(style, cross_horizontal, reverse_cross);
        let end_auto = self.axis_logical_end_margin_is_auto(style, cross_horizontal, reverse_cross);
        let mut start_margin = if start_auto {
            0.0
        } else {
            self.axis_logical_start_margin(margin, cross_horizontal, reverse_cross)
        };
        let mut end_margin = if end_auto {
            0.0
        } else {
            self.axis_logical_end_margin(margin, cross_horizontal, reverse_cross)
        };

        if !start_auto && !end_auto {
            return (start_margin, end_margin);
        }

        let outer_cross = item_cross + start_margin + end_margin;
        if outer_cross < line_cross {
            let free = line_cross - outer_cross;
            if start_auto && end_auto {
                start_margin = free / 2.0;
                end_margin = free / 2.0;
            } else if start_auto {
                start_margin = free;
            } else {
                end_margin = free;
            }
        } else if start_auto {
            start_margin = 0.0;
            end_margin = line_cross - item_cross;
        } else {
            end_margin = line_cross - item_cross - start_margin;
        }

        (start_margin, end_margin)
    }

    fn flex_align_cross_offset(&self, input: FlexAlignCrossOffsetInput) -> f32 {
        let child_cross_margin = self.axis_margin(input.margin, !input.parent_is_row);
        let child_cross_margin_bound = input.child_cross + child_cross_margin;
        let free = input.content_cross - child_cross_margin_bound;
        let start_margin =
            self.axis_logical_start_margin(input.margin, !input.parent_is_row, input.reverse_cross);
        match input.align_items {
            AlignItems::Stretch
            | AlignItems::FlexStart
            | AlignItems::Start
            | AlignItems::Baseline => input.line_cross_offset + start_margin,
            AlignItems::Center if input.reverse_cross => {
                input.line_cross_offset + start_margin + free / 2.0
            }
            AlignItems::Center => (input.line_cross_offset + free / 2.0) + start_margin,
            AlignItems::FlexEnd | AlignItems::End if input.reverse_cross => {
                input.line_cross_offset + start_margin + free
            }
            AlignItems::FlexEnd | AlignItems::End => {
                (input.line_cross_offset + free) + start_margin
            }
        }
    }

    fn flex_basis<T: LayoutTree>(
        &mut self,
        tree: &mut T,
        child: T::NodeId,
        style: &Style,
        context: FlexBasisContext,
    ) -> FlexBasisResult {
        let axis_length = if context.is_row {
            style.width
        } else {
            style.height
        };
        if let Some(value) =
            self.resolve_numeric_length(style.flex_basis, context.flex_basis_percent_base.value())
        {
            return FlexBasisResult::resolved_flex_basis(value.max(0.0));
        }

        if let Some(value) = self.resolve_axis_length(axis_length, context.content_main_limit) {
            let main = match style.box_sizing {
                BoxSizing::BorderBox => value.max(0.0),
                BoxSizing::ContentBox => {
                    let padding_border = if context.is_row {
                        context.edges.padding.horizontal() + context.edges.border.horizontal()
                    } else {
                        context.edges.padding.vertical() + context.edges.border.vertical()
                    };
                    (value + padding_border).max(0.0)
                }
            };
            return FlexBasisResult::derived(main);
        }

        if let Some(cross) = self.resolve_child_cross_size(
            style,
            context.is_row,
            context.content_cross_limit,
            context.edges,
        ) {
            let (main, _) = self.apply_aspect_ratio_to_main_cross(
                style,
                context.edges,
                None,
                Some(cross),
                context.is_row,
            );
            if let Some(main) = main {
                return FlexBasisResult::derived(main.max(0.0));
            }
        }

        let constraints = self.default_child_constraints_for_parent(
            style,
            context.parent_constraints,
            context.edges,
        );
        let measure_constraints =
            self.apply_min_max_to_constraints(style, constraints, context.edges);
        let measure_style =
            self.default_constraint_style_override(style, context.edges, measure_constraints);
        let main_percent_base_was_unresolved = self.flex_layout_main_percent_base_is_unresolved(
            style,
            context.edges,
            measure_constraints,
            context.is_row,
        );
        let measured = self.layout_node_with_edges(
            tree,
            child,
            Some(measure_style),
            NodeLayoutContext {
                constraints: measure_constraints,
                offset: Point::ZERO,
                sticky_constraints: measure_constraints,
                edges: context.edges,
                rounding: RoundingContext::root(),
                flex: FlexNodeContext::from_parts(FlexPercentPropagation::new(
                    main_percent_base_was_unresolved,
                    main_percent_base_was_unresolved,
                )),
            },
        );
        FlexBasisResult {
            main: self.main_size(measured.size, context.is_row),
            measure: Some(CachedLayoutMeasure {
                constraints: measure_constraints,
                size: measured.size,
                main_percent_base_was_unresolved,
            }),
            post_flex_main_defines_percent_base: false,
        }
    }

    fn content_constraints_from_optional_border_axes(
        &self,
        fixed_main: Option<f32>,
        fixed_cross: Option<f32>,
        main_constraint: SideConstraint,
        cross_constraint: SideConstraint,
        is_row: bool,
        edges: ResolvedEdges,
    ) -> Constraints {
        let main = self.content_axis_constraint_from_border_axis(
            fixed_main,
            main_constraint,
            is_row,
            edges,
        );
        let cross = self.content_axis_constraint_from_border_axis(
            fixed_cross,
            cross_constraint,
            !is_row,
            edges,
        );
        if is_row {
            Constraints::new(main, cross)
        } else {
            Constraints::new(cross, main)
        }
    }

    fn content_axis_constraint_from_border_axis(
        &self,
        fixed_border_size: Option<f32>,
        constraint: SideConstraint,
        horizontal: bool,
        edges: ResolvedEdges,
    ) -> SideConstraint {
        if let Some(size) = fixed_border_size {
            return SideConstraint::definite(self.inner_axis(size, horizontal, edges));
        }
        match constraint.mode {
            MeasureMode::Indefinite => SideConstraint::indefinite(),
            MeasureMode::Definite | MeasureMode::AtMost => SideConstraint {
                size: self.inner_axis(constraint.size, horizontal, edges),
                mode: constraint.mode,
            },
        }
    }

    fn default_child_constraints_for_parent(
        &self,
        style: &Style,
        parent_constraints: Constraints,
        edges: ResolvedEdges,
    ) -> Constraints {
        Constraints::new(
            self.default_child_axis_constraint_for_parent(
                style,
                Axis::Horizontal,
                parent_constraints,
                edges,
            ),
            self.default_child_axis_constraint_for_parent(
                style,
                Axis::Vertical,
                parent_constraints,
                edges,
            ),
        )
    }

    fn default_child_axis_constraint_for_parent(
        &self,
        style: &Style,
        axis: Axis,
        parent_constraints: Constraints,
        edges: ResolvedEdges,
    ) -> SideConstraint {
        let parent_constraint = match axis {
            Axis::Horizontal => parent_constraints.width,
            Axis::Vertical => parent_constraints.height,
        };
        let length = match axis {
            Axis::Horizontal => style.width,
            Axis::Vertical => style.height,
        };
        if let Some(preferred) =
            self.resolve_border_axis(length, axis, parent_constraints, edges, style.box_sizing)
        {
            return SideConstraint::definite(preferred);
        }

        let margin = if axis.is_horizontal() {
            edges.margin.horizontal()
        } else {
            edges.margin.vertical()
        };
        let mut constraint = parent_constraint
            .bounded_size()
            .map(|size| SideConstraint::at_most((size - margin).max(0.0)))
            .unwrap_or_else(SideConstraint::indefinite);

        if let Length::FitContent(base) = length {
            constraint = self.fit_content_owner_constraint(base, parent_constraint);
        }
        if matches!(length, Length::MinContent | Length::MaxContent) {
            constraint = SideConstraint::indefinite();
        }
        constraint
    }

    fn flex_child_cross_constraint(
        &self,
        style: &Style,
        explicit_cross: Option<f32>,
        parent_constraints: Constraints,
        edges: ResolvedEdges,
        is_row: bool,
    ) -> SideConstraint {
        if let Some(cross) = explicit_cross {
            return SideConstraint::definite(cross);
        }

        self.default_child_axis_constraint_for_parent(
            style,
            if is_row {
                Axis::Vertical
            } else {
                Axis::Horizontal
            },
            parent_constraints,
            edges,
        )
    }

    fn child_flex_constraints(
        &self,
        main_size: f32,
        cross_constraint: SideConstraint,
        is_row: bool,
    ) -> Constraints {
        if is_row {
            Constraints::new(SideConstraint::definite(main_size), cross_constraint)
        } else {
            Constraints::new(cross_constraint, SideConstraint::definite(main_size))
        }
    }

    fn child_flex_main_tracks_parent_main(&self, style: &Style, parent_is_row: bool) -> bool {
        style.display == Display::Flex && style.flex_direction.is_row() == parent_is_row
    }

    fn child_flex_main_inherits_unresolved_parent_main<T: LayoutTree>(
        &self,
        tree: &T,
        node: T::NodeId,
        style: &Style,
        parent_is_row: bool,
        parent_main_axis_is_definite: bool,
    ) -> bool {
        !parent_main_axis_is_definite
            && self.child_flex_main_tracks_parent_main(style, parent_is_row)
            && self.flex_main_axis_has_non_shrinking_percent_flex_basis_child(
                tree,
                node,
                style,
                parent_is_row,
            )
    }

    fn main_length_defines_percent_main_size_base(&self, length: Length, flex_shrink: f32) -> bool {
        match length {
            Length::Points(_) | Length::Fr(_) => true,
            Length::Percent(_) => flex_shrink > self.epsilon,
            Length::Calc { percent, .. } => {
                percent.abs() <= f32::EPSILON || flex_shrink > self.epsilon
            }
            Length::Auto | Length::MinContent | Length::MaxContent | Length::FitContent(_) => false,
        }
    }

    fn flex_layout_main_percent_base_is_unresolved(
        &self,
        style: &Style,
        edges: ResolvedEdges,
        constraints: Constraints,
        is_row: bool,
    ) -> bool {
        let main_axis = if is_row {
            Axis::Horizontal
        } else {
            Axis::Vertical
        };
        let cross_axis = if is_row {
            Axis::Vertical
        } else {
            Axis::Horizontal
        };
        let fixed_main = self.resolve_border_axis(
            if is_row { style.width } else { style.height },
            main_axis,
            constraints,
            edges,
            style.box_sizing,
        );
        let fixed_cross = self.resolve_border_axis(
            if is_row { style.height } else { style.width },
            cross_axis,
            constraints,
            edges,
            style.box_sizing,
        );
        let (fixed_main, _) =
            self.apply_aspect_ratio_to_main_cross(style, edges, fixed_main, fixed_cross, is_row);
        fixed_main.is_none() && !self.axis_constraint(constraints, is_row).is_definite()
    }

    fn cached_flex_basis_measure_matches<T: LayoutTree>(
        &self,
        tree: &T,
        node: T::NodeId,
        style: &Style,
        cached: CachedLayoutMeasure,
        constraints: Constraints,
    ) -> bool {
        if !tree.has_measure(node) && tree.children(node).next().is_none() {
            return false;
        }

        let horizontal = self.flex_cache_axis_matches(
            tree,
            node,
            style,
            FlexCacheAxisCheck {
                current: constraints.width,
                last: cached.constraints.width,
                last_size: cached.size.width,
                is_horizontal: true,
            },
        );
        let vertical = self.flex_cache_axis_matches(
            tree,
            node,
            style,
            FlexCacheAxisCheck {
                current: constraints.height,
                last: cached.constraints.height,
                last_size: cached.size.height,
                is_horizontal: false,
            },
        );
        horizontal && vertical
    }

    fn flex_cache_axis_matches<T: LayoutTree>(
        &self,
        tree: &T,
        node: T::NodeId,
        style: &Style,
        check: FlexCacheAxisCheck,
    ) -> bool {
        let can_reuse_percent_base = || {
            self.can_reuse_layout_with_same_size_as_given_constraint(
                tree,
                node,
                style,
                check.is_horizontal,
            )
        };
        match check.last.mode {
            MeasureMode::Indefinite => match check.current.mode {
                MeasureMode::Indefinite => true,
                MeasureMode::AtMost => check.current.size + self.epsilon >= check.last_size,
                MeasureMode::Definite => {
                    (check.current.size - check.last_size).abs() <= self.epsilon
                        && can_reuse_percent_base()
                }
            },
            MeasureMode::AtMost => match check.current.mode {
                MeasureMode::Indefinite => false,
                MeasureMode::AtMost => {
                    (check.last.size - check.current.size).abs() <= self.epsilon
                        || (check.last.size + self.epsilon >= check.current.size
                            && check.current.size + self.epsilon >= check.last_size)
                }
                MeasureMode::Definite => {
                    (check.last_size - check.current.size).abs() <= self.epsilon
                        && can_reuse_percent_base()
                }
            },
            MeasureMode::Definite => match check.current.mode {
                MeasureMode::Indefinite | MeasureMode::AtMost => false,
                MeasureMode::Definite => {
                    (check.last.size - check.current.size).abs() <= self.epsilon
                        || (check.last_size - check.current.size).abs() <= self.epsilon
                }
            },
        }
    }

    fn cached_layout_measure_matches<T: LayoutTree>(
        &self,
        tree: &T,
        node: T::NodeId,
        style: &Style,
        cached: LayoutBox,
        constraints: Constraints,
    ) -> bool {
        if !tree.has_measure(node) && tree.children(node).next().is_none() {
            return false;
        }

        let horizontal = self.flex_cache_axis_matches(
            tree,
            node,
            style,
            FlexCacheAxisCheck {
                current: constraints.width,
                last: cached.constraints.width,
                last_size: cached.size.width,
                is_horizontal: true,
            },
        );
        let vertical = self.flex_cache_axis_matches(
            tree,
            node,
            style,
            FlexCacheAxisCheck {
                current: constraints.height,
                last: cached.constraints.height,
                last_size: cached.size.height,
                is_horizontal: false,
            },
        );
        horizontal && vertical
    }

    fn box_info_depends_on_percent_base(style: &Style, is_horizontal: bool) -> bool {
        if is_horizontal
            && (Self::rect_contains_percentage(style.padding)
                || Self::rect_contains_percentage(style.margin))
        {
            return true;
        }
        if is_horizontal {
            Self::length_contains_percentage(style.min_width)
                || Self::length_contains_percentage(style.max_width)
        } else {
            Self::length_contains_percentage(style.min_height)
                || Self::length_contains_percentage(style.max_height)
        }
    }

    fn rect_contains_percentage(rect: Rect<Length>) -> bool {
        Self::length_contains_percentage(rect.left)
            || Self::length_contains_percentage(rect.right)
            || Self::length_contains_percentage(rect.top)
            || Self::length_contains_percentage(rect.bottom)
    }

    fn can_reuse_layout_with_same_size_as_given_constraint<T: LayoutTree>(
        &self,
        tree: &T,
        node: T::NodeId,
        style: &Style,
        is_horizontal: bool,
    ) -> bool {
        if Self::box_info_depends_on_percent_base(style, is_horizontal) {
            return false;
        }

        let effective_display = if style.display == Display::Block {
            Display::Linear
        } else {
            style.display
        };
        if effective_display == Display::Linear
            && is_horizontal != style.linear_orientation.is_row()
            && self.linear_cross_axis_can_change_under_definite_constraint(tree, node, style)
        {
            return false;
        }

        match style.display {
            Display::Flex if is_horizontal != style.flex_direction.is_row() => return false,
            Display::Linear if is_horizontal != style.linear_orientation.is_row() => return false,
            Display::Relative => return false,
            _ => {}
        }

        let flex_main_axis =
            style.display == Display::Flex && style.flex_direction.is_row() == is_horizontal;

        tree.children(node).all(|child| {
            let child_style = tree.style(child);
            if flex_main_axis && Self::length_contains_percentage(child_style.flex_basis) {
                return false;
            }
            if is_horizontal {
                !Self::length_contains_percentage(child_style.width)
                    && !Self::length_contains_percentage(child_style.min_width)
                    && !Self::length_contains_percentage(child_style.max_width)
                    && !Self::rect_contains_percentage(child_style.padding)
                    && !Self::rect_contains_percentage(child_style.margin)
            } else {
                !Self::length_contains_percentage(child_style.height)
                    && !Self::length_contains_percentage(child_style.min_height)
                    && !Self::length_contains_percentage(child_style.max_height)
            }
        })
    }

    fn linear_cross_axis_can_change_under_definite_constraint<T: LayoutTree>(
        &self,
        tree: &T,
        node: T::NodeId,
        style: &Style,
    ) -> bool {
        let is_row = style.linear_orientation.is_row();
        tree.children(node).any(|child| {
            let child_style = tree.style(child);
            if child_style.display == Display::None || is_out_of_flow(child_style) {
                return false;
            }

            let layout_gravity = self.computed_linear_layout_gravity(style, child_style);
            if self.linear_layout_gravity_forces_stretch(layout_gravity) {
                return true;
            }

            let cross_length = if is_row {
                child_style.height
            } else {
                child_style.width
            };
            layout_gravity == LinearLayoutGravity::None && cross_length == Length::Auto
        })
    }

    fn flex_main_axis_has_non_shrinking_percent_flex_basis_child<T: LayoutTree>(
        &self,
        tree: &T,
        node: T::NodeId,
        style: &Style,
        is_horizontal: bool,
    ) -> bool {
        style.display == Display::Flex
            && style.flex_direction.is_row() == is_horizontal
            && style.flex_grow <= self.epsilon
            && tree.children(node).any(|child| {
                let child_style = tree.style(child);
                Self::length_contains_percentage(child_style.flex_basis)
                    && child_style.flex_shrink <= self.epsilon
                    && child_style.align_self.is_none()
            })
    }

    fn flex_main_axis_has_non_shrinking_percent_flex_basis_descendant<T: LayoutTree>(
        &self,
        tree: &T,
        node: T::NodeId,
        style: &Style,
        is_horizontal: bool,
    ) -> bool {
        style.display == Display::Flex
            && style.flex_direction.is_row() == is_horizontal
            && tree.children(node).any(|child| {
                let child_style = tree.style(child);
                Self::length_contains_percentage(child_style.flex_basis)
                    && child_style.flex_shrink <= self.epsilon
                    && child_style.align_self.is_none()
            })
    }

    fn flex_main_axis_has_default_aligned_shrinking_percent_flex_basis_child<T: LayoutTree>(
        &self,
        tree: &T,
        node: T::NodeId,
        style: &Style,
        is_horizontal: bool,
    ) -> bool {
        style.display == Display::Flex
            && style.flex_direction.is_row() == is_horizontal
            && tree.children(node).any(|child| {
                let child_style = tree.style(child);
                Self::length_contains_percentage(child_style.flex_basis)
                    && child_style.flex_grow > self.epsilon
                    && child_style.flex_shrink > self.epsilon
                    && child_style.align_self.is_none()
            })
    }

    fn flex_child_style_override(
        &self,
        style: &Style,
        edges: ResolvedEdges,
        target_main: f32,
        explicit_cross: Option<f32>,
        is_row: bool,
        min_max_percent_constraints: Constraints,
    ) -> Style {
        let mut override_style = self.main_axis_size_override(style, edges, target_main, is_row);
        if let Some(cross) = explicit_cross {
            let axis = if is_row {
                Axis::Vertical
            } else {
                Axis::Horizontal
            };
            self.set_css_axis_size_from_border_size(&mut override_style, style, axis, cross, edges);
        }
        self.override_min_max_percent_lengths(
            style,
            min_max_percent_constraints,
            &mut override_style,
        );
        override_style
    }

    pub(super) fn clamp_flex_item_main(
        &self,
        style: &Style,
        edges: ResolvedEdges,
        value: f32,
        is_row: bool,
        percent_base: Option<f32>,
        suppress_flexible_min: bool,
    ) -> f32 {
        let axis = if is_row {
            Axis::Horizontal
        } else {
            Axis::Vertical
        };
        let padding_border = self.padding_border(axis, edges);
        let min_length = if is_row {
            style.min_width
        } else {
            style.min_height
        };
        let max_length = if is_row {
            style.max_width
        } else {
            style.max_height
        };

        let mut result = value.max(0.0);
        let suppress_flexible_min = suppress_flexible_min
            && min_length.is_flexible()
            && max_length.is_flexible()
            && Self::length_contains_percentage(style.flex_basis)
            && style.box_sizing == BoxSizing::BorderBox
            && style.display == Display::Block
            && style.flex_grow <= self.epsilon
            && style.flex_shrink > self.epsilon
            && style.align_self.is_none();
        if !suppress_flexible_min {
            if let Some(min) = self.resolve_min_max_length(min_length, percent_base) {
                let min = match style.box_sizing {
                    BoxSizing::BorderBox => min,
                    BoxSizing::ContentBox => min + padding_border,
                };
                result = result.max(min);
            }
        }
        if let Some(max) = self.resolve_min_max_length(max_length, percent_base) {
            let max = match style.box_sizing {
                BoxSizing::BorderBox => max,
                BoxSizing::ContentBox => max + padding_border,
            };
            result = result.min(max);
        }
        result = result.max(padding_border);
        result.max(0.0)
    }

    fn flex_main_front_is_reversed(&self, style: &Style, is_row: bool) -> bool {
        if is_row {
            style.flex_direction.is_reverse() ^ style.direction.is_any_rtl()
        } else {
            style.flex_direction.is_reverse()
        }
    }

    fn flex_cross_front_is_reversed(&self, style: &Style, is_row: bool) -> bool {
        !is_row && style.direction.is_any_rtl()
    }

    pub(super) fn flex_physical_offset_from_logical_margin_start(
        &self,
        logical_margin_offset: f32,
        content_size: f32,
        item_border_size: f32,
        logical_start_margin: f32,
        logical_end_margin: f32,
        reversed: bool,
    ) -> f32 {
        let physical_start_margin = if reversed {
            logical_end_margin
        } else {
            logical_start_margin
        };
        if reversed {
            let physical_end_margin = logical_start_margin;
            let margin_bound = Self::near_integer_bound(
                (item_border_size + physical_start_margin) + physical_end_margin,
            );
            ((content_size - margin_bound) - logical_margin_offset) + physical_start_margin
        } else {
            logical_margin_offset + physical_start_margin
        }
    }

    fn flex_physical_offset_from_logical_border_start(
        &self,
        logical_border_offset: f32,
        content_size: f32,
        item_border_size: f32,
        reversed: bool,
    ) -> f32 {
        let offset = if reversed {
            // Keep reversed bound writeback order stable at observable .5 pixel boundaries.
            (content_size - item_border_size) - logical_border_offset
        } else {
            logical_border_offset
        };
        Self::exported_zero_offset(offset)
    }

    fn flex_physical_cross_offset_from_logical_border_start(
        &self,
        input: FlexPhysicalCrossOffsetInput,
    ) -> f32 {
        let cross_horizontal = !input.parent_is_row;
        let logical_start_margin =
            self.axis_logical_start_margin(input.margin, cross_horizontal, input.reversed);
        let physical_start_margin = if cross_horizontal {
            input.margin.left
        } else {
            input.margin.top
        };
        let logical_margin_offset = input.logical_border_offset - logical_start_margin;
        if input.reversed {
            let margin_bound =
                input.item_border_size + self.axis_margin(input.margin, cross_horizontal);
            Self::exported_zero_offset(
                ((input.content_size - margin_bound) - logical_margin_offset)
                    + physical_start_margin,
            )
        } else {
            Self::exported_zero_offset(logical_margin_offset + physical_start_margin)
        }
    }

    fn collect_flex_lines<N>(
        &self,
        items: &[FlexItem<N>],
        flex_wrap: FlexWrap,
        content_main: f32,
        gap: f32,
        is_row: bool,
    ) -> Vec<FlexLine> {
        if items.is_empty() {
            return Vec::new();
        }
        if flex_wrap == FlexWrap::NoWrap {
            return vec![FlexLine::new(0, items.len())];
        }

        let mut lines = Vec::new();
        let mut line_start = 0;
        let mut line_outer_main = 0.0;
        for (idx, item) in items.iter().enumerate() {
            let item_outer_main = self.item_hypothetical_outer_main(item, is_row);
            let proposed_outer_main = if idx == line_start {
                item_outer_main
            } else if item.collapsed {
                line_outer_main
            } else {
                line_outer_main + gap + item_outer_main
            };

            if idx > line_start && proposed_outer_main > content_main + self.epsilon {
                lines.push(FlexLine::new(line_start, idx));
                line_start = idx;
                line_outer_main = item_outer_main;
            } else {
                line_outer_main = proposed_outer_main;
            }
        }
        lines.push(FlexLine::new(line_start, items.len()));
        lines
    }

    fn resolve_flexible_lengths<N>(
        &self,
        items: &mut [FlexItem<N>],
        content_main: f32,
        gap: f32,
        is_row: bool,
    ) -> f32 {
        if items.is_empty() {
            return content_main;
        }
        if items.iter().any(|item| item.collapsed) {
            return self.resolve_flexible_lengths_excluding_collapsed(
                items,
                content_main,
                gap,
                is_row,
            );
        }

        let is_grow = used_flex_factor_is_grow(
            self.line_hypothetical_main(items, gap, is_row),
            content_main,
            self.epsilon,
        );

        let resolve_items = items
            .iter()
            .map(|item| FlexResolveItem {
                flex_base_size: item.base_main,
                hypothetical_main_size: item.hypothetical_main,
                flex_grow: item.style.flex_grow,
                flex_shrink: item.style.flex_shrink,
                outer_non_main_size: self.axis_margin(item.edges.margin, is_row),
            })
            .collect::<Vec<_>>();

        let result = compute_elastic_item_sizes(
            FlexResolveInput {
                items: &resolve_items,
                available_main_space: content_main,
                main_axis_gap: gap,
                is_grow,
                epsilon: self.epsilon,
            },
            |idx, raw| {
                let item = &items[idx];
                self.clamp_flex_item_main(
                    &item.style,
                    item.edges,
                    raw,
                    is_row,
                    Some(content_main),
                    item.suppress_flexible_min,
                )
            },
        );

        for (item, target_main) in items.iter_mut().zip(result.target_main_sizes) {
            item.target_main = target_main;
        }

        result.remaining_free_space
    }

    fn resolve_flexible_lengths_excluding_collapsed<N>(
        &self,
        items: &mut [FlexItem<N>],
        content_main: f32,
        gap: f32,
        is_row: bool,
    ) -> f32 {
        let active_indices = items
            .iter()
            .enumerate()
            .filter_map(|(idx, item)| (!item.collapsed).then_some(idx))
            .collect::<Vec<_>>();
        if active_indices.is_empty() {
            for item in items {
                item.target_main = 0.0;
            }
            return content_main;
        }

        let active_outer_main = active_indices.iter().fold(0.0, |sum, &idx| {
            let item = &items[idx];
            sum + item.hypothetical_main + self.axis_margin(item.edges.margin, is_row)
        }) + self.gap_total(gap, active_indices.len());
        let is_grow = used_flex_factor_is_grow(active_outer_main, content_main, self.epsilon);
        let resolve_items = active_indices
            .iter()
            .map(|&idx| {
                let item = &items[idx];
                FlexResolveItem {
                    flex_base_size: item.base_main,
                    hypothetical_main_size: item.hypothetical_main,
                    flex_grow: item.style.flex_grow,
                    flex_shrink: item.style.flex_shrink,
                    outer_non_main_size: self.axis_margin(item.edges.margin, is_row),
                }
            })
            .collect::<Vec<_>>();

        let result = compute_elastic_item_sizes(
            FlexResolveInput {
                items: &resolve_items,
                available_main_space: content_main,
                main_axis_gap: gap,
                is_grow,
                epsilon: self.epsilon,
            },
            |active_idx, raw| {
                let item = &items[active_indices[active_idx]];
                self.clamp_flex_item_main(
                    &item.style,
                    item.edges,
                    raw,
                    is_row,
                    Some(content_main),
                    item.suppress_flexible_min,
                )
            },
        );

        for item in items.iter_mut() {
            if item.collapsed {
                item.target_main = 0.0;
            }
        }
        for (&idx, target_main) in active_indices.iter().zip(result.target_main_sizes) {
            items[idx].target_main = target_main;
        }

        result.remaining_free_space
    }

    fn line_hypothetical_main<N>(&self, items: &[FlexItem<N>], gap: f32, is_row: bool) -> f32 {
        items.iter().fold(0.0, |sum, item| {
            sum + self.item_hypothetical_outer_main(item, is_row)
        }) + self.gap_total(gap, self.flex_line_participating_len(items))
    }

    fn item_hypothetical_outer_main<N>(&self, item: &FlexItem<N>, is_row: bool) -> f32 {
        if item.collapsed {
            0.0
        } else {
            item.hypothetical_main + self.axis_margin(item.edges.margin, is_row)
        }
    }

    fn flex_line_participating_len<N>(&self, items: &[FlexItem<N>]) -> usize {
        items.iter().filter(|item| !item.collapsed).count()
    }

    fn total_line_cross(&self, lines: &[FlexLine], gap: f32) -> f32 {
        lines.iter().map(|line| line.cross).sum::<f32>() + self.gap_total(gap, lines.len())
    }

    fn flex_line_cross_considering_baseline<N>(
        &self,
        items: &[FlexItem<N>],
        container_align: AlignItems,
        is_row: bool,
    ) -> FlexBaselineCross {
        if !is_row {
            return FlexBaselineCross::default();
        }

        let baseline = items
            .iter()
            .filter(|item| {
                self.flex_item_align(&item.style, container_align) == AlignItems::Baseline
            })
            .map(|item| item.baseline_from_cross_start_margin_edge)
            .fold(0.0, f32::max);

        if baseline <= self.epsilon {
            return FlexBaselineCross::default();
        }

        let cross = items
            .iter()
            .filter(|item| {
                self.flex_item_align(&item.style, container_align) == AlignItems::Baseline
            })
            .map(|item| {
                item.cross + self.axis_margin(item.edges.margin, !is_row) + baseline
                    - item.baseline_from_cross_start_margin_edge
            })
            .fold(0.0, f32::max);

        FlexBaselineCross { baseline, cross }
    }

    fn flex_container_baseline<N>(
        &self,
        items: &[FlexItem<N>],
        lines: &[FlexLine],
        context: FlexContainerBaselineContext<'_>,
    ) -> Option<f32> {
        let first_line = lines.first()?;
        if first_line.start == first_line.end {
            return None;
        }

        if context.is_row {
            return self.exported_container_baseline(Some(first_line.baseline));
        }

        let line_items = &items[first_line.start..first_line.end];
        let first = &line_items[0];
        let remaining_main = first_line.remaining_main;
        let (main_start, _) = self.flex_justify(
            context.container_style.justify_content,
            remaining_main,
            first_line.len(),
            context.main_gap,
        );
        self.exported_container_baseline(Some(main_start + first.baseline_from_top_margin_edge))
    }

    fn flex_line_baseline<N>(
        &self,
        line_items: &[FlexItem<N>],
        line_cross: f32,
        container_align: AlignItems,
        is_row: bool,
    ) -> f32 {
        if !is_row || line_items.is_empty() {
            return 0.0;
        }

        let max_baseline = line_items
            .iter()
            .filter(|item| {
                self.flex_item_align(&item.style, container_align) == AlignItems::Baseline
            })
            .map(|item| item.baseline_from_cross_start_margin_edge)
            .fold(0.0, f32::max);
        if max_baseline > self.epsilon {
            return max_baseline;
        }

        let first = &line_items[0];
        let mut baseline = first.baseline_from_top_margin_edge;
        let cross_margin_bound = first.cross + self.axis_margin(first.edges.margin, false);
        match self.flex_item_align(&first.style, container_align) {
            AlignItems::FlexEnd | AlignItems::End => {
                baseline += line_cross - cross_margin_bound;
            }
            AlignItems::Center => {
                baseline += (line_cross - cross_margin_bound) / 2.0;
            }
            AlignItems::Stretch
            | AlignItems::FlexStart
            | AlignItems::Start
            | AlignItems::Baseline => {}
        }
        baseline
    }

    fn item_baseline_from_top_border_edge(&self, layout: LayoutBox, edges: ResolvedEdges) -> f32 {
        layout.baseline.map_or(layout.size.height, |baseline| {
            edges.border.top + edges.padding.top + baseline
        })
    }

    pub(super) fn item_baseline_from_top_margin_edge(
        &self,
        layout: LayoutBox,
        edges: ResolvedEdges,
    ) -> f32 {
        edges.margin.top + self.item_baseline_from_top_border_edge(layout, edges)
    }

    fn flex_item_baseline_offsets(
        &self,
        layout: LayoutBox,
        edges: ResolvedEdges,
        parent_is_row: bool,
    ) -> FlexItemBaselineOffsets {
        let cross_horizontal = !parent_is_row;
        let cross_size = self.cross_size(layout.size, parent_is_row);
        let from_cross_start_border_edge = layout.baseline.map_or(cross_size, |baseline| {
            self.axis_start_margin(edges.border, cross_horizontal)
                + self.axis_start_margin(edges.padding, cross_horizontal)
                + baseline
        });

        FlexItemBaselineOffsets {
            from_cross_start_margin_edge: self.axis_start_margin(edges.margin, cross_horizontal)
                + from_cross_start_border_edge,
            from_cross_start_border_edge,
        }
    }

    fn flex_item_align(&self, item_style: &Style, container_align: AlignItems) -> AlignItems {
        item_style.align_self.unwrap_or(container_align)
    }

    fn auto_margin_count<N>(&self, items: &[FlexItem<N>], horizontal: bool) -> usize {
        items
            .iter()
            .filter(|item| !item.collapsed)
            .map(|item| {
                usize::from(self.axis_start_margin_is_auto(&item.style, horizontal))
                    + usize::from(self.axis_end_margin_is_auto(&item.style, horizontal))
            })
            .sum()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::tree::{SimpleNode, SimpleTree};

    fn assert_close(actual: f32, expected: f32) {
        assert!(
            (actual - expected).abs() < 0.0001,
            "actual={actual}, expected={expected}"
        );
    }

    fn assert_axis_alignment(
        actual: OutOfFlowAxisAlignment,
        expected_position: OutOfFlowPosition,
        expected_front_is_physical_start: bool,
    ) {
        assert_eq!(
            actual.front_is_physical_start,
            expected_front_is_physical_start
        );
        assert!(
            matches!(
                (actual.position, expected_position),
                (OutOfFlowPosition::Start, OutOfFlowPosition::Start)
                    | (OutOfFlowPosition::Center, OutOfFlowPosition::Center)
                    | (OutOfFlowPosition::End, OutOfFlowPosition::End)
            ),
            "actual={:?}, expected={:?}",
            actual.position,
            expected_position
        );
    }

    fn resolved_edges(margin: Edges, padding: Edges, border: Edges) -> ResolvedEdges {
        ResolvedEdges {
            margin,
            padding,
            border,
        }
    }

    fn zero_edges() -> ResolvedEdges {
        resolved_edges(Rect::all(0.0), Rect::all(0.0), Rect::all(0.0))
    }

    fn flex_item(
        id: usize,
        style: Style,
        base_main: f32,
        hypothetical_main: f32,
    ) -> FlexItem<usize> {
        FlexItem {
            id,
            style,
            style_override: None,
            constraints: Constraints::indefinite(),
            edges: zero_edges(),
            base_main,
            hypothetical_main,
            target_main: hypothetical_main,
            post_flex_main_defines_percent_base: false,
            suppress_flexible_min: false,
            basis_measure: None,
            collapsed: false,
            collapse_strut_cross: None,
            cross: 0.0,
            measured: None,
            baseline_from_top_margin_edge: 0.0,
            baseline_from_cross_start_margin_edge: 0.0,
            baseline_from_cross_start_border_edge: 0.0,
        }
    }

    #[test]
    fn flex_percent_base_tracks_definite_and_suppressed_sources() {
        let definite = FlexPercentBase::from_layout_state(Some(42.0), false);
        assert_eq!(definite.value(), Some(42.0));
        assert_eq!(
            definite.source,
            FlexPercentBaseSource::DefiniteContainerMain
        );

        let missing = FlexPercentBase::from_layout_state(None, false);
        assert_eq!(missing.value(), None);
        assert_eq!(
            missing.source,
            FlexPercentBaseSource::NoDefiniteContainerMain
        );

        let suppressed = FlexPercentBase::from_layout_state(Some(42.0), true);
        assert_eq!(suppressed.value(), None);
        assert_eq!(
            suppressed.source,
            FlexPercentBaseSource::SuppressedForUnresolvedParent
        );
    }

    #[test]
    fn flex_justify_interval_covers_distribution_and_overflow_fallbacks() {
        let engine = LayoutEngine::new();

        assert_eq!(
            engine.flex_justify_interval(JustifyContent::FlexStart, 12.0, 3),
            (0.0, 0.0)
        );
        assert_eq!(
            engine.flex_justify_interval(JustifyContent::FlexEnd, 12.0, 3),
            (12.0, 0.0)
        );
        assert_eq!(
            engine.flex_justify_interval(JustifyContent::Center, 12.0, 3),
            (6.0, 0.0)
        );
        assert_eq!(
            engine.flex_justify_interval(JustifyContent::SpaceBetween, 12.0, 4),
            (0.0, 4.0)
        );
        assert_eq!(
            engine.flex_justify_interval(JustifyContent::SpaceBetween, -6.0, 3),
            (0.0, 0.0)
        );
        assert_eq!(
            engine.flex_justify_interval(JustifyContent::SpaceAround, 12.0, 3),
            (2.0, 4.0)
        );
        assert_eq!(
            engine.flex_justify_interval(JustifyContent::SpaceAround, -6.0, 3),
            (-3.0, 0.0)
        );
        assert_eq!(
            engine.flex_justify_interval(JustifyContent::SpaceEvenly, 12.0, 3),
            (3.0, 3.0)
        );
        assert_eq!(
            engine.flex_justify_interval(JustifyContent::Center, 12.0, 0),
            (0.0, 0.0)
        );
        assert_eq!(
            engine.flex_justify(JustifyContent::SpaceBetween, 12.0, 4, 2.0),
            (0.0, 6.0)
        );
    }

    #[test]
    fn flex_align_content_covers_negative_and_distribution_fallbacks() {
        let engine = LayoutEngine::new();

        assert_eq!(
            engine.flex_align_content(AlignContent::FlexStart, 12.0, 3),
            (0.0, 0.0)
        );
        assert_eq!(
            engine.flex_align_content(AlignContent::FlexEnd, 12.0, 3),
            (12.0, 0.0)
        );
        assert_eq!(
            engine.flex_align_content(AlignContent::Center, 12.0, 3),
            (6.0, 0.0)
        );
        assert_eq!(
            engine.flex_align_content(AlignContent::SpaceBetween, 12.0, 3),
            (0.0, 6.0)
        );
        assert_eq!(
            engine.flex_align_content(AlignContent::SpaceBetween, -6.0, 3),
            (0.0, 0.0)
        );
        assert_eq!(
            engine.flex_align_content(AlignContent::SpaceAround, 12.0, 3),
            (2.0, 4.0)
        );
        assert_eq!(
            engine.flex_align_content(AlignContent::SpaceAround, -6.0, 3),
            (-3.0, 0.0)
        );
        assert_eq!(
            engine.flex_align_content(AlignContent::SpaceEvenly, 12.0, 3),
            (3.0, 3.0)
        );
        assert_eq!(
            engine.flex_align_content(AlignContent::Center, 12.0, 0),
            (0.0, 0.0)
        );
    }

    #[test]
    fn flex_cross_axis_auto_margins_resolve_w3c_positive_and_overflow_cases() {
        let engine = LayoutEngine::new();
        let no_auto = Style::default();
        assert_eq!(
            engine.flex_resolved_auto_cross_margins(
                &no_auto,
                Rect::new(1.0, 3.0, 0.0, 0.0),
                30.0,
                10.0,
                false,
                false,
            ),
            (1.0, 3.0)
        );

        let both_auto = Style {
            margin: Rect::new(Length::Auto, Length::Auto, Length::ZERO, Length::ZERO),
            ..Style::default()
        };
        assert_eq!(
            engine.flex_resolved_auto_cross_margins(
                &both_auto,
                Rect::all(0.0),
                30.0,
                10.0,
                false,
                false,
            ),
            (10.0, 10.0)
        );

        let start_auto = Style {
            margin: Rect::new(
                Length::Auto,
                Length::points(2.0),
                Length::ZERO,
                Length::ZERO,
            ),
            ..Style::default()
        };
        assert_eq!(
            engine.flex_resolved_auto_cross_margins(
                &start_auto,
                Rect::new(0.0, 2.0, 0.0, 0.0),
                30.0,
                10.0,
                false,
                false,
            ),
            (18.0, 2.0)
        );
        assert_eq!(
            engine.flex_resolved_auto_cross_margins(
                &start_auto,
                Rect::new(0.0, 2.0, 0.0, 0.0),
                8.0,
                10.0,
                false,
                false,
            ),
            (0.0, -2.0)
        );
        let end_auto = Style {
            margin: Rect::new(
                Length::points(2.0),
                Length::Auto,
                Length::ZERO,
                Length::ZERO,
            ),
            ..Style::default()
        };
        assert_eq!(
            engine.flex_resolved_auto_cross_margins(
                &end_auto,
                Rect::new(2.0, 0.0, 0.0, 0.0),
                8.0,
                10.0,
                false,
                false,
            ),
            (2.0, -4.0)
        );
        assert_close(
            engine.flex_auto_cross_offset(&both_auto, Rect::all(0.0), 30.0, 10.0, false, false),
            10.0,
        );
    }

    #[test]
    fn flex_basis_and_default_constraints_cover_non_measured_paths() {
        let mut engine = LayoutEngine::new();
        let mut tree = SimpleTree::default();
        let child = tree.push(SimpleNode::new(Style::default()));
        let edges = resolved_edges(Rect::all(0.0), Rect::all(2.0), Rect::all(1.0));

        let border_box = Style {
            width: Length::points(20.0),
            box_sizing: BoxSizing::BorderBox,
            ..Style::default()
        };
        let border_box_basis = engine.flex_basis(
            &mut tree,
            child,
            &border_box,
            FlexBasisContext {
                is_row: true,
                flex_basis_percent_base: FlexPercentBase::from_layout_state(None, false),
                content_main_limit: Some(100.0),
                content_cross_limit: None,
                parent_constraints: Constraints::definite(100.0, 50.0),
                edges,
            },
        );
        assert_close(border_box_basis.main, 20.0);
        assert!(!border_box_basis.post_flex_main_defines_percent_base);

        let content_box = Style {
            width: Length::points(20.0),
            box_sizing: BoxSizing::ContentBox,
            ..Style::default()
        };
        let content_box_basis = engine.flex_basis(
            &mut tree,
            child,
            &content_box,
            FlexBasisContext {
                is_row: true,
                flex_basis_percent_base: FlexPercentBase::from_layout_state(None, false),
                content_main_limit: Some(100.0),
                content_cross_limit: None,
                parent_constraints: Constraints::definite(100.0, 50.0),
                edges,
            },
        );
        assert_close(content_box_basis.main, 26.0);

        let max_content = Style {
            width: Length::MaxContent,
            ..Style::default()
        };
        assert_eq!(
            engine.default_child_axis_constraint_for_parent(
                &max_content,
                Axis::Horizontal,
                Constraints::definite(100.0, 50.0),
                edges,
            ),
            SideConstraint::indefinite()
        );
    }

    #[test]
    fn flex_cache_reuse_helpers_cover_mode_matrix_and_guard_paths() {
        let engine = LayoutEngine::new();
        let mut tree = SimpleTree::default();
        let measured = tree.push(SimpleNode::with_measured_size(
            Style::default(),
            Size::new(10.0, 8.0),
        ));
        let empty = tree.push(SimpleNode::new(Style::default()));
        let style = Style::default();
        let cached_basis = CachedLayoutMeasure {
            constraints: Constraints::indefinite(),
            size: Size::new(10.0, 8.0),
            main_percent_base_was_unresolved: false,
        };
        assert!(!engine.cached_flex_basis_measure_matches(
            &tree,
            empty,
            &style,
            cached_basis,
            Constraints::indefinite()
        ));

        assert!(engine.flex_cache_axis_matches(
            &tree,
            measured,
            &style,
            FlexCacheAxisCheck {
                current: SideConstraint::at_most(12.0),
                last: SideConstraint::indefinite(),
                last_size: 10.0,
                is_horizontal: true,
            },
        ));
        assert!(!engine.flex_cache_axis_matches(
            &tree,
            measured,
            &style,
            FlexCacheAxisCheck {
                current: SideConstraint::indefinite(),
                last: SideConstraint::at_most(12.0),
                last_size: 10.0,
                is_horizontal: true,
            },
        ));
        assert!(engine.flex_cache_axis_matches(
            &tree,
            measured,
            &style,
            FlexCacheAxisCheck {
                current: SideConstraint::at_most(12.0),
                last: SideConstraint::at_most(12.0),
                last_size: 10.0,
                is_horizontal: true,
            },
        ));
        assert!(engine.flex_cache_axis_matches(
            &tree,
            measured,
            &style,
            FlexCacheAxisCheck {
                current: SideConstraint::at_most(10.0),
                last: SideConstraint::at_most(12.0),
                last_size: 9.0,
                is_horizontal: true,
            },
        ));
        assert!(engine.flex_cache_axis_matches(
            &tree,
            measured,
            &style,
            FlexCacheAxisCheck {
                current: SideConstraint::definite(10.0),
                last: SideConstraint::at_most(12.0),
                last_size: 10.0,
                is_horizontal: true,
            },
        ));
        assert!(!engine.flex_cache_axis_matches(
            &tree,
            measured,
            &style,
            FlexCacheAxisCheck {
                current: SideConstraint::at_most(10.0),
                last: SideConstraint::definite(10.0),
                last_size: 10.0,
                is_horizontal: true,
            },
        ));
        assert!(engine.flex_cache_axis_matches(
            &tree,
            measured,
            &style,
            FlexCacheAxisCheck {
                current: SideConstraint::definite(10.0),
                last: SideConstraint::definite(12.0),
                last_size: 10.0,
                is_horizontal: true,
            },
        ));

        let percent_padding = Style {
            padding: Rect::new(
                Length::percent(10.0),
                Length::ZERO,
                Length::ZERO,
                Length::ZERO,
            ),
            ..Style::default()
        };
        assert!(!engine.can_reuse_layout_with_same_size_as_given_constraint(
            &tree,
            measured,
            &percent_padding,
            true
        ));

        let flex_cross_axis = Style {
            display: Display::Flex,
            flex_direction: crate::style::FlexDirection::Column,
            ..Style::default()
        };
        assert!(!engine.can_reuse_layout_with_same_size_as_given_constraint(
            &tree,
            measured,
            &flex_cross_axis,
            true
        ));

        let linear_cross_axis = tree.push(SimpleNode::new(Style {
            display: Display::Block,
            ..Style::default()
        }));
        let linear_child = tree.push(SimpleNode::new(Style::default()));
        tree.append_child(linear_cross_axis, linear_child);
        assert!(!engine.can_reuse_layout_with_same_size_as_given_constraint(
            &tree,
            linear_cross_axis,
            &Style::default(),
            true
        ));

        let linear_display_cross_axis = Style {
            display: Display::Linear,
            linear_orientation: crate::style::LinearOrientation::Vertical,
            ..Style::default()
        };
        assert!(!engine.can_reuse_layout_with_same_size_as_given_constraint(
            &tree,
            measured,
            &linear_display_cross_axis,
            true
        ));

        let relative = Style {
            display: Display::Relative,
            ..Style::default()
        };
        assert!(!engine
            .can_reuse_layout_with_same_size_as_given_constraint(&tree, measured, &relative, true));

        let cached_layout = LayoutBox::new(
            Size::new(10.0, 8.0),
            Rect::all(0.0),
            None,
            Constraints::indefinite(),
        );
        assert!(engine.cached_layout_measure_matches(
            &tree,
            measured,
            &style,
            cached_layout,
            Constraints::indefinite()
        ));
    }

    #[test]
    fn linear_cross_axis_cache_guard_covers_ignored_stretch_and_auto_children() {
        let engine = LayoutEngine::new();

        let mut ignored_tree = SimpleTree::default();
        let ignored_parent = ignored_tree.push(SimpleNode::new(Style {
            display: Display::Linear,
            linear_orientation: crate::style::LinearOrientation::Horizontal,
            ..Style::default()
        }));
        let display_none_child = ignored_tree.push(SimpleNode::new(Style {
            display: Display::None,
            ..Style::default()
        }));
        let out_of_flow_child = ignored_tree.push(SimpleNode::new(Style {
            position: PositionType::Absolute,
            ..Style::default()
        }));
        ignored_tree.append_child(ignored_parent, display_none_child);
        ignored_tree.append_child(ignored_parent, out_of_flow_child);
        let ignored_parent_style = ignored_tree.style(ignored_parent).clone();
        assert!(
            !engine.linear_cross_axis_can_change_under_definite_constraint(
                &ignored_tree,
                ignored_parent,
                &ignored_parent_style,
            )
        );

        let mut stretch_tree = SimpleTree::default();
        let stretch_parent = stretch_tree.push(SimpleNode::new(Style {
            display: Display::Linear,
            linear_orientation: crate::style::LinearOrientation::Horizontal,
            ..Style::default()
        }));
        let stretch_child = stretch_tree.push(SimpleNode::new(Style {
            linear_layout_gravity: LinearLayoutGravity::Stretch,
            width: Length::points(20.0),
            height: Length::points(10.0),
            ..Style::default()
        }));
        stretch_tree.append_child(stretch_parent, stretch_child);
        let stretch_parent_style = stretch_tree.style(stretch_parent).clone();
        assert!(
            engine.linear_cross_axis_can_change_under_definite_constraint(
                &stretch_tree,
                stretch_parent,
                &stretch_parent_style,
            )
        );

        let mut auto_cross_tree = SimpleTree::default();
        let auto_cross_parent = auto_cross_tree.push(SimpleNode::new(Style {
            display: Display::Linear,
            linear_orientation: crate::style::LinearOrientation::Horizontal,
            ..Style::default()
        }));
        let auto_cross_child = auto_cross_tree.push(SimpleNode::new(Style::default()));
        auto_cross_tree.append_child(auto_cross_parent, auto_cross_child);
        let auto_cross_parent_style = auto_cross_tree.style(auto_cross_parent).clone();
        assert!(
            engine.linear_cross_axis_can_change_under_definite_constraint(
                &auto_cross_tree,
                auto_cross_parent,
                &auto_cross_parent_style,
            )
        );
    }

    #[test]
    fn flex_percent_base_and_min_clamp_helpers_cover_w3c_definiteness_branches() {
        let engine = LayoutEngine::new();
        let mut tree = SimpleTree::default();
        let parent = tree.push(SimpleNode::new(Style {
            display: Display::Flex,
            flex_direction: crate::style::FlexDirection::Row,
            ..Style::default()
        }));
        let non_shrinking_child = tree.push(SimpleNode::new(Style {
            flex_basis: Length::percent(50.0),
            flex_shrink: 0.0,
            ..Style::default()
        }));
        tree.append_child(parent, non_shrinking_child);

        let parent_style = tree.style(parent).clone();
        assert!(
            engine.flex_main_axis_has_non_shrinking_percent_flex_basis_child(
                &tree,
                parent,
                &parent_style,
                true,
            )
        );
        assert!(
            engine.flex_main_axis_has_non_shrinking_percent_flex_basis_descendant(
                &tree,
                parent,
                &parent_style,
                true,
            )
        );
        assert!(engine.child_flex_main_inherits_unresolved_parent_main(
            &tree,
            parent,
            &parent_style,
            true,
            false,
        ));

        let shrinking_parent = tree.push(SimpleNode::new(Style {
            display: Display::Flex,
            flex_direction: crate::style::FlexDirection::Row,
            ..Style::default()
        }));
        let shrinking_child = tree.push(SimpleNode::new(Style {
            flex_basis: Length::percent(50.0),
            flex_grow: 1.0,
            flex_shrink: 1.0,
            ..Style::default()
        }));
        tree.append_child(shrinking_parent, shrinking_child);
        let shrinking_parent_style = tree.style(shrinking_parent).clone();
        assert!(
            engine.flex_main_axis_has_default_aligned_shrinking_percent_flex_basis_child(
                &tree,
                shrinking_parent,
                &shrinking_parent_style,
                true,
            )
        );

        assert!(engine.main_length_defines_percent_main_size_base(Length::points(1.0), 0.0));
        assert!(!engine.main_length_defines_percent_main_size_base(Length::percent(50.0), 0.0));
        assert!(engine.main_length_defines_percent_main_size_base(Length::percent(50.0), 1.0));
        assert!(engine.main_length_defines_percent_main_size_base(Length::calc(1.0, 0.0), 0.0));
        assert!(!engine.main_length_defines_percent_main_size_base(Length::calc(1.0, 10.0), 0.0));

        let suppressible = Style {
            min_width: Length::fr(30.0),
            max_width: Length::fr(50.0),
            flex_basis: Length::percent(50.0),
            box_sizing: BoxSizing::BorderBox,
            flex_shrink: 1.0,
            ..Style::default()
        };
        assert_close(
            engine.clamp_flex_item_main(&suppressible, zero_edges(), 10.0, true, Some(100.0), true),
            10.0,
        );
        assert_close(
            engine.clamp_flex_item_main(
                &suppressible,
                zero_edges(),
                10.0,
                true,
                Some(100.0),
                false,
            ),
            30.0,
        );
    }

    #[test]
    fn flex_line_collection_and_collapsed_resolution_cover_empty_and_collapsed_paths() {
        let engine = LayoutEngine::new();
        let empty_items: Vec<FlexItem<usize>> = Vec::new();
        assert!(engine
            .collect_flex_lines(&empty_items, FlexWrap::Wrap, 100.0, 5.0, true)
            .is_empty());

        let first = flex_item(0, Style::default(), 10.0, 10.0);
        let mut collapsed = flex_item(1, Style::default(), 10.0, 10.0);
        collapsed.collapsed = true;
        let second = flex_item(2, Style::default(), 10.0, 10.0);
        let items = vec![first.clone(), collapsed.clone(), second.clone()];
        let lines = engine.collect_flex_lines(&items, FlexWrap::Wrap, 100.0, 5.0, true);
        assert_eq!(lines.len(), 1);
        assert_eq!(lines[0].len(), 3);
        assert_close(engine.line_hypothetical_main(&items, 5.0, true), 25.0);

        let mut no_items: Vec<FlexItem<usize>> = Vec::new();
        assert_close(
            engine.resolve_flexible_lengths(&mut no_items, 40.0, 0.0, true),
            40.0,
        );

        let mut all_collapsed = vec![collapsed.clone()];
        assert_close(
            engine.resolve_flexible_lengths(&mut all_collapsed, 40.0, 0.0, true),
            40.0,
        );
        assert_close(all_collapsed[0].target_main, 0.0);

        let mut active = flex_item(
            3,
            Style {
                flex_grow: 1.0,
                ..Style::default()
            },
            10.0,
            10.0,
        );
        active.suppress_flexible_min = false;
        let mut mixed = vec![active, collapsed];
        mixed[0].style.margin = Rect::new(Length::Auto, Length::Auto, Length::ZERO, Length::ZERO);
        assert_close(
            engine.resolve_flexible_lengths(&mut mixed, 40.0, 0.0, true),
            0.0,
        );
        assert_close(mixed[0].target_main, 40.0);
        assert_close(mixed[1].target_main, 0.0);

        assert_eq!(engine.auto_margin_count(&mixed, true), 2);
    }

    #[test]
    fn flex_baseline_helpers_cover_empty_line_and_no_row_cases() {
        let engine = LayoutEngine::new();
        let items = vec![flex_item(0, Style::default(), 10.0, 10.0)];
        assert_eq!(
            engine.flex_line_cross_considering_baseline(&items, AlignItems::Baseline, false),
            FlexBaselineCross::default()
        );
        assert_eq!(
            engine.flex_container_baseline(
                &items,
                &[FlexLine::new(0, 0)],
                FlexContainerBaselineContext {
                    container_style: &Style::default(),
                    main_gap: 0.0,
                    is_row: true,
                },
            ),
            None
        );
    }

    #[test]
    fn flex_used_margin_writes_main_and_cross_axis_auto_margins() {
        let engine = LayoutEngine::new();
        let style = Style {
            margin: Rect::new(
                Length::Auto,
                Length::points(2.0),
                Length::Auto,
                Length::Auto,
            ),
            ..Style::default()
        };
        let margin = engine.flex_used_margin(
            &style,
            Rect::new(1.0, 2.0, 3.0, 4.0),
            FlexUsedMarginContext {
                item_cross: 10.0,
                line_cross: 30.0,
                is_row: true,
                reverse_main: false,
                reverse_cross: false,
                auto_main_margin: Some(5.0),
            },
        );

        assert_eq!(margin, Rect::new(5.0, 2.0, 10.0, 10.0));
    }

    #[test]
    fn flex_align_cross_offset_accounts_for_alignment_margin_and_reverse_cross_axis() {
        let engine = LayoutEngine::new();
        let margin = Rect::new(1.0, 3.0, 2.0, 4.0);

        assert_close(
            engine.flex_align_cross_offset(FlexAlignCrossOffsetInput {
                align_items: AlignItems::FlexStart,
                line_cross_offset: 5.0,
                content_cross: 40.0,
                child_cross: 10.0,
                margin,
                parent_is_row: true,
                reverse_cross: false,
            }),
            7.0,
        );
        assert_close(
            engine.flex_align_cross_offset(FlexAlignCrossOffsetInput {
                align_items: AlignItems::Center,
                line_cross_offset: 5.0,
                content_cross: 40.0,
                child_cross: 10.0,
                margin,
                parent_is_row: true,
                reverse_cross: false,
            }),
            19.0,
        );
        assert_close(
            engine.flex_align_cross_offset(FlexAlignCrossOffsetInput {
                align_items: AlignItems::Center,
                line_cross_offset: 5.0,
                content_cross: 40.0,
                child_cross: 10.0,
                margin,
                parent_is_row: true,
                reverse_cross: true,
            }),
            21.0,
        );
        assert_close(
            engine.flex_align_cross_offset(FlexAlignCrossOffsetInput {
                align_items: AlignItems::FlexEnd,
                line_cross_offset: 5.0,
                content_cross: 40.0,
                child_cross: 10.0,
                margin,
                parent_is_row: true,
                reverse_cross: false,
            }),
            31.0,
        );
        assert_close(
            engine.flex_align_cross_offset(FlexAlignCrossOffsetInput {
                align_items: AlignItems::FlexEnd,
                line_cross_offset: 5.0,
                content_cross: 40.0,
                child_cross: 10.0,
                margin,
                parent_is_row: true,
                reverse_cross: true,
            }),
            33.0,
        );
    }

    #[test]
    fn flex_out_of_flow_alignment_maps_static_position_to_flex_axes() {
        let engine = LayoutEngine::new();
        let child_style = Style::default();

        let row = Style {
            display: Display::Flex,
            justify_content: JustifyContent::SpaceBetween,
            align_items: AlignItems::Baseline,
            ..Style::default()
        };
        let row_alignment = engine.flex_out_of_flow_alignment(&row, &child_style);
        assert_axis_alignment(row_alignment.horizontal, OutOfFlowPosition::Start, true);
        assert_axis_alignment(row_alignment.vertical, OutOfFlowPosition::Start, true);

        let row_reverse = Style {
            display: Display::Flex,
            flex_direction: crate::style::FlexDirection::RowReverse,
            flex_wrap: FlexWrap::WrapReverse,
            justify_content: JustifyContent::FlexEnd,
            align_items: AlignItems::Center,
            ..Style::default()
        };
        let row_reverse_alignment = engine.flex_out_of_flow_alignment(&row_reverse, &child_style);
        assert_axis_alignment(
            row_reverse_alignment.horizontal,
            OutOfFlowPosition::End,
            false,
        );
        assert_axis_alignment(
            row_reverse_alignment.vertical,
            OutOfFlowPosition::Center,
            true,
        );

        let column_child = Style {
            align_self: Some(AlignItems::FlexEnd),
            ..Style::default()
        };
        let column = Style {
            display: Display::Flex,
            flex_direction: crate::style::FlexDirection::Column,
            direction: crate::style::Direction::Rtl,
            justify_content: JustifyContent::Center,
            ..Style::default()
        };
        let column_alignment = engine.flex_out_of_flow_alignment(&column, &column_child);
        assert_axis_alignment(column_alignment.horizontal, OutOfFlowPosition::End, false);
        assert_axis_alignment(column_alignment.vertical, OutOfFlowPosition::Center, true);
    }
}
