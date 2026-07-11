// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use super::*;

struct GridItem<N> {
    id: N,
    style: Style,
    edges: ResolvedEdges,
    row: usize,
    column: usize,
    row_span: usize,
    column_span: usize,
}

#[derive(Clone, Debug)]
struct GridPendingItem<N> {
    id: N,
    style: Style,
    row_placement: GridAxisPlacement,
    column_placement: GridAxisPlacement,
}

impl<N> GridPendingItem<N> {
    fn has_definite_area(&self) -> bool {
        self.row_placement.start.is_some() && self.column_placement.start.is_some()
    }

    fn is_locked_to_auto_placement_cross_axis(&self, is_column_flow: bool) -> bool {
        if is_column_flow {
            self.column_placement.start.is_some() && self.row_placement.start.is_none()
        } else {
            self.row_placement.start.is_some() && self.column_placement.start.is_none()
        }
    }
}

#[derive(Clone, Copy, Debug)]
struct GridMeasuredItem {
    row: usize,
    column: usize,
    row_span: usize,
    column_span: usize,
    outer_width: f32,
    minimum_width: f32,
    outer_height: f32,
    minimum_height: f32,
}

#[derive(Clone, Copy, Debug)]
struct GridFlexibleExpansionContext {
    gap: f32,
    min_content_limit: Option<f32>,
    max_content_limit: Option<f32>,
    min_content_constraint: bool,
    horizontal: bool,
}

struct GridIntrinsicGrowthContext<'a> {
    track_sizing: &'a [GridTrackSizing],
    growth_limits: &'a mut [Option<f32>],
    span: std::ops::Range<usize>,
    gap: f32,
    required_size: f32,
    minimum_size: f32,
    container_axis_is_definite: bool,
    container_axis_is_indefinite: bool,
    container_axis_is_min_content_constraint: bool,
    container_axis_has_max_limit: bool,
}

struct GridIntrinsicBaseRequiredSizeContext<'a> {
    track_sizing: &'a [GridTrackSizing],
    growth_limits: &'a [Option<f32>],
    span: std::ops::Range<usize>,
    gap: f32,
    required_size: f32,
    minimum_size: f32,
    container_axis_has_max_limit: bool,
}

struct GridIntrinsicGrowthGroupContext<'a> {
    track_sizing: &'a [GridTrackSizing],
    growth_limits: &'a mut [Option<f32>],
    contributions: &'a [GridIntrinsicContribution],
    gap: f32,
    container_axis_is_definite: bool,
    container_axis_is_indefinite: bool,
    container_axis_is_min_content_constraint: bool,
    container_axis_has_max_limit: bool,
}

#[derive(Clone, Copy, Debug)]
struct GridIntrinsicContribution {
    start: usize,
    span: usize,
    required_size: f32,
    minimum_size: f32,
}

#[derive(Clone, Copy, Debug, Default, PartialEq)]
struct GridAxisArea {
    start: f32,
    size: f32,
}

#[derive(Clone, Copy, Debug, Default, PartialEq)]
struct GridAbsoluteArea {
    origin: Point,
    size: Size,
    inline_start: f32,
    content_origin: Point,
    content_size: Size,
    inline_static_uses_content_edges: bool,
    block_static_uses_content_edges: bool,
}

#[derive(Clone, Copy, Debug)]
struct GridOutOfFlowContext<'a> {
    container_style: &'a Style,
    edges: ResolvedEdges,
    column_offsets: &'a [f32],
    row_offsets: &'a [f32],
    column_gap: f32,
    row_gap: f32,
    content_size: Size,
    scrollable_content_size: Size,
    grid_offsets: GridPlacementOffsets,
}

#[derive(Clone, Copy, Debug)]
struct GridAbsoluteAxisContext<'a> {
    explicit_track_count: usize,
    axis_offset: usize,
    line_offsets: &'a [f32],
    gap: f32,
    scrollable_content_size: f32,
    start_padding: f32,
    end_padding: f32,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
enum GridAxisAlignment {
    Start,
    Center,
    End,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
struct GridAxisPlacement {
    start: Option<usize>,
    span: usize,
}

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
struct GridPlacementOffsets {
    row: usize,
    column: usize,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
struct GridAxisLineInput {
    start: Option<i32>,
    end: Option<i32>,
    span: usize,
}

#[derive(Clone, Copy, Debug, PartialEq)]
enum GridTrackKind {
    Fixed,
    Auto,
    MinContent,
    MaxContent,
    FitContent(Option<f32>),
    Flexible(f32),
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
enum GridIntrinsicLimitPhase {
    MinContent,
    MaxContent,
}

#[derive(Clone, Copy, Debug)]
enum GridBeyondLimitDistribution {
    IntrinsicMaxOrAllAffected,
    MaxContentMaxOrAllAffected,
}

#[derive(Clone, Copy, Debug, PartialEq)]
struct GridTrackSizing {
    min_track: Length,
    max_track: Length,
    kind: GridTrackKind,
}

#[derive(Clone, Copy, Debug, PartialEq)]
struct GridBaselineCandidate {
    row: usize,
    column: usize,
    order: usize,
    baseline: f32,
}

#[derive(Clone, Copy, Debug)]
struct GridTrackDefinitions<'a> {
    explicit_min: &'a [Length],
    explicit_max: &'a [Length],
    implicit_min: &'a [Length],
    implicit_max: &'a [Length],
    leading_implicit: usize,
}

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
struct GridCursor {
    row: usize,
    column: usize,
}

impl GridCursor {
    const fn new(row: usize, column: usize) -> Self {
        Self { row, column }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
struct GridPlacementRequest {
    row_span: usize,
    column_span: usize,
    is_column_flow: bool,
    auto_flow_limit: usize,
}

fn normalize_grid_line(line: i32, explicit_end: i32) -> i32 {
    if line < 0 {
        line + explicit_end + 1
    } else {
        line
    }
}

fn grid_line_to_index(line: i32) -> Option<usize> {
    line.checked_sub(1).map(|line| line as usize)
}

fn leading_implicit_pattern_index(
    idx: usize,
    leading_implicit: usize,
    pattern_len: usize,
) -> usize {
    if pattern_len == 0 {
        return 0;
    }
    (idx + pattern_len - leading_implicit % pattern_len) % pattern_len
}

fn positive_implicit_pattern_index(idx: usize, pattern_len: usize) -> usize {
    if pattern_len == 0 {
        0
    } else {
        idx % pattern_len
    }
}

impl LayoutEngine {
    pub(super) fn layout_grid<T: LayoutTree>(
        &mut self,
        tree: &mut T,
        style: &Style,
        children: &[T::NodeId],
        constraints: Constraints,
        edges: ResolvedEdges,
        rounding: RoundingContext,
    ) -> LayoutOutput {
        let mut fixed_width = self.resolve_border_axis(
            style.width,
            Axis::Horizontal,
            constraints,
            edges,
            style.box_sizing,
        );
        let mut fixed_height = self.resolve_border_axis(
            style.height,
            Axis::Vertical,
            constraints,
            edges,
            style.box_sizing,
        );
        self.apply_aspect_ratio_to_optional(style, edges, &mut fixed_width, &mut fixed_height);
        let content_width_limit =
            self.grid_content_axis_limit(fixed_width, constraints.width, true, edges);
        let content_height_limit =
            self.grid_content_axis_limit(fixed_height, constraints.height, false, edges);
        let content_width_min_limit = self
            .resolve_min_max_border_axis(
                style,
                Axis::Horizontal,
                style.min_width,
                constraints,
                edges,
            )
            .map(|width| self.inner_width(width, edges));
        let content_height_min_limit = self
            .resolve_min_max_border_axis(
                style,
                Axis::Vertical,
                style.min_height,
                constraints,
                edges,
            )
            .map(|height| self.inner_height(height, edges));
        let content_width_max_limit = self
            .resolve_min_max_border_axis(
                style,
                Axis::Horizontal,
                style.max_width,
                constraints,
                edges,
            )
            .map(|width| self.inner_width(width, edges));
        let content_height_max_limit = self
            .resolve_min_max_border_axis(
                style,
                Axis::Vertical,
                style.max_height,
                constraints,
                edges,
            )
            .map(|height| self.inner_height(height, edges));

        self.layout_display_none_children(tree, children, edges, rounding);
        let visible_children = self.ordered_in_flow_children(&*tree, children);
        let grid_offsets = self.grid_placement_offsets(tree, &visible_children, style);
        let mut items = self.place_grid_items(
            tree,
            &visible_children,
            style,
            self.axis_constraint_from_optional(content_width_limit),
            grid_offsets,
        );
        let column_count = items
            .iter()
            .map(|item| item.column + item.column_span)
            .max()
            .unwrap_or(0)
            .max(grid_offsets.column + style.grid_template_columns.len())
            .max(1);
        let row_count = items
            .iter()
            .map(|item| item.row + item.row_span)
            .max()
            .unwrap_or(0)
            .max(grid_offsets.row + style.grid_template_rows.len())
            .max(1);
        let column_gap = self.resolve_gap(style, true, constraints).unwrap_or(0.0);
        let row_gap = self.resolve_gap(style, false, constraints).unwrap_or(0.0);
        let column_definitions = GridTrackDefinitions {
            explicit_min: &style.grid_template_columns,
            explicit_max: &style.grid_template_columns_max,
            implicit_min: &style.grid_auto_columns,
            implicit_max: &style.grid_auto_columns_max,
            leading_implicit: grid_offsets.column,
        };
        let row_definitions = GridTrackDefinitions {
            explicit_min: &style.grid_template_rows,
            explicit_max: &style.grid_template_rows_max,
            implicit_min: &style.grid_auto_rows,
            implicit_max: &style.grid_auto_rows_max,
            leading_implicit: grid_offsets.row,
        };
        let mut measured_grid_items = Vec::with_capacity(items.len());
        for item in &mut items {
            let child_constraints = self.grid_intrinsic_inline_child_constraints();
            let measured = self.layout_node(tree, item.id, child_constraints, Point::ZERO);
            let outer_width = measured.size.width + item.edges.margin.horizontal();
            let outer_height = measured.size.height + item.edges.margin.vertical();
            let max_content_outer_width = self.grid_item_max_content_contribution(
                tree,
                item.id,
                &item.style,
                item.edges,
                true,
                child_constraints,
                outer_width,
                content_width_limit,
            );
            let max_content_outer_height = self.grid_item_max_content_contribution(
                tree,
                item.id,
                &item.style,
                item.edges,
                false,
                child_constraints,
                outer_height,
                content_height_limit,
            );
            measured_grid_items.push(GridMeasuredItem {
                row: item.row,
                column: item.column,
                row_span: item.row_span,
                column_span: item.column_span,
                outer_width: max_content_outer_width,
                minimum_width: self.grid_item_minimum_contribution(
                    tree,
                    item.id,
                    &item.style,
                    item.edges,
                    true,
                    child_constraints,
                    content_width_limit,
                ),
                outer_height: max_content_outer_height,
                minimum_height: self.grid_item_minimum_contribution(
                    tree,
                    item.id,
                    &item.style,
                    item.edges,
                    false,
                    child_constraints,
                    content_height_limit,
                ),
            });
        }
        let (mut columns, mut column_track_sizing) = self.resolve_grid_tracks_for_axis(
            column_definitions,
            column_count,
            &measured_grid_items,
            true,
            content_width_limit,
            content_width_min_limit,
            content_width_max_limit,
            column_gap,
            constraints.width,
            style.width == Length::MinContent,
        );

        let content_width_for_block_contribution = self.grid_content_size_after_min_max(
            &columns,
            column_gap,
            content_width_limit,
            content_width_min_limit,
            content_width_max_limit,
        );
        self.stretch_grid_auto_tracks(
            &mut columns,
            &column_track_sizing,
            content_width_for_block_contribution,
            column_gap,
            style.justify_content == JustifyContent::Stretch,
        );
        let column_gap_for_block_contribution = self.grid_aligned_track_gap_for_contribution(
            style,
            true,
            &columns,
            column_gap,
            content_width_for_block_contribution,
        );

        self.update_grid_block_axis_contributions_after_column_sizing(
            tree,
            style,
            &items,
            &mut measured_grid_items,
            &columns,
            column_gap_for_block_contribution,
            content_height_limit,
        );

        let (mut rows, mut row_track_sizing) = self.resolve_grid_tracks_for_axis(
            row_definitions,
            row_count,
            &measured_grid_items,
            false,
            content_height_limit,
            content_height_min_limit,
            content_height_max_limit,
            row_gap,
            constraints.height,
            style.height == Length::MinContent,
        );

        let content_height_for_inline_contribution = self.grid_content_size_after_min_max(
            &rows,
            row_gap,
            content_height_limit,
            content_height_min_limit,
            content_height_max_limit,
        );
        self.stretch_grid_auto_tracks(
            &mut rows,
            &row_track_sizing,
            content_height_for_inline_contribution,
            row_gap,
            style.align_content == AlignContent::Stretch,
        );
        let row_gap_for_inline_contribution = self.grid_aligned_track_gap_for_contribution(
            style,
            false,
            &rows,
            row_gap,
            content_height_for_inline_contribution,
        );
        let inline_contribution_changed = self
            .update_grid_inline_axis_contributions_after_row_sizing(
                tree,
                style,
                &items,
                &mut measured_grid_items,
                &rows,
                row_gap_for_inline_contribution,
                content_width_limit,
            );
        if inline_contribution_changed {
            (columns, column_track_sizing) = self.resolve_grid_tracks_for_axis(
                column_definitions,
                column_count,
                &measured_grid_items,
                true,
                content_width_limit,
                content_width_min_limit,
                content_width_max_limit,
                column_gap,
                constraints.width,
                style.width == Length::MinContent,
            );
            let content_width_for_block_contribution = self.grid_content_size_after_min_max(
                &columns,
                column_gap,
                content_width_limit,
                content_width_min_limit,
                content_width_max_limit,
            );
            self.stretch_grid_auto_tracks(
                &mut columns,
                &column_track_sizing,
                content_width_for_block_contribution,
                column_gap,
                style.justify_content == JustifyContent::Stretch,
            );
            let column_gap_for_block_contribution = self.grid_aligned_track_gap_for_contribution(
                style,
                true,
                &columns,
                column_gap,
                content_width_for_block_contribution,
            );
            let block_contribution_changed = self
                .update_grid_block_axis_contributions_after_column_sizing(
                    tree,
                    style,
                    &items,
                    &mut measured_grid_items,
                    &columns,
                    column_gap_for_block_contribution,
                    content_height_limit,
                );
            if block_contribution_changed {
                (rows, row_track_sizing) = self.resolve_grid_tracks_for_axis(
                    row_definitions,
                    row_count,
                    &measured_grid_items,
                    false,
                    content_height_limit,
                    content_height_min_limit,
                    content_height_max_limit,
                    row_gap,
                    constraints.height,
                    style.height == Length::MinContent,
                );
                let content_height_for_inline_contribution = self.grid_content_size_after_min_max(
                    &rows,
                    row_gap,
                    content_height_limit,
                    content_height_min_limit,
                    content_height_max_limit,
                );
                self.stretch_grid_auto_tracks(
                    &mut rows,
                    &row_track_sizing,
                    content_height_for_inline_contribution,
                    row_gap,
                    style.align_content == AlignContent::Stretch,
                );
            }
        }

        let width_uses_constraint_as_container_size =
            fixed_width.is_some() || constraints.width.mode == MeasureMode::Definite;
        let height_uses_constraint_as_container_size =
            fixed_height.is_some() || constraints.height.mode == MeasureMode::Definite;
        let final_content_width_limit = width_uses_constraint_as_container_size
            .then_some(content_width_limit)
            .flatten();
        let final_content_height_limit = height_uses_constraint_as_container_size
            .then_some(content_height_limit)
            .flatten();
        let content_width = self.grid_content_size_after_min_max(
            &columns,
            column_gap,
            final_content_width_limit,
            content_width_min_limit,
            content_width_max_limit,
        );
        let content_height = self.grid_content_size_after_min_max(
            &rows,
            row_gap,
            final_content_height_limit,
            content_height_min_limit,
            content_height_max_limit,
        );

        self.stretch_grid_auto_tracks(
            &mut columns,
            &column_track_sizing,
            content_width,
            column_gap,
            style.justify_content == JustifyContent::Stretch,
        );
        self.stretch_grid_auto_tracks(
            &mut rows,
            &row_track_sizing,
            content_height,
            row_gap,
            style.align_content == AlignContent::Stretch,
        );

        let content_width = content_width
            .unwrap_or_else(|| self.track_total_size(&columns, column_gap))
            .max(0.0);
        let content_height = content_height
            .unwrap_or_else(|| self.track_total_size(&rows, row_gap))
            .max(0.0);
        let column_gap = self.resolve_gap_with_content_size(style, true, content_width, column_gap);
        let row_gap = self.resolve_gap_with_content_size(style, false, content_height, row_gap);
        let used_column_size = self.track_total_size(&columns, column_gap);
        let used_row_size = self.track_total_size(&rows, row_gap);
        let (column_start, resolved_column_gap) = self.justify(
            style.justify_content,
            (content_width - used_column_size).max(0.0),
            columns.len(),
            column_gap,
        );
        let (row_start, resolved_row_gap) = self.align_content_with_gap(
            style.align_content,
            (content_height - used_row_size).max(0.0),
            rows.len(),
            row_gap,
        );
        let column_offsets = self.grid_line_offsets(&columns, column_start, resolved_column_gap);
        let row_offsets = self.grid_line_offsets(&rows, row_start, resolved_row_gap);
        let scrollable_content_size = Size::new(
            content_width.max(column_offsets.last().copied().unwrap_or(0.0)),
            content_height.max(row_offsets.last().copied().unwrap_or(0.0)),
        );
        let sticky_constraints = Constraints::definite(content_width, content_height);

        let mut first_baseline = None;
        for (item_order, item) in items.into_iter().enumerate() {
            let cell_width = self.spanned_track_size(
                &columns,
                item.column,
                item.column_span,
                resolved_column_gap,
            );
            let cell_height =
                self.spanned_track_size(&rows, item.row, item.row_span, resolved_row_gap);
            let item_edges = self.grid_final_item_edges(&item.style, item.edges, cell_width);
            let mut child_constraints = self.grid_child_constraints(
                style,
                &item.style,
                cell_width,
                cell_height,
                item_edges,
            );
            if tree.has_measure(item.id) {
                child_constraints =
                    self.grid_measured_child_constraints(style, &item.style, child_constraints);
            }
            let mut child_style = self
                .percent_resolved_style_override(&item.style, child_constraints, item_edges)
                .unwrap_or_else(|| item.style.clone());
            let mut child_style_changed = child_style != item.style;
            child_style_changed |= self.override_min_max_percent_lengths(
                &item.style,
                Constraints::definite(cell_width, cell_height),
                &mut child_style,
            );
            let child_style_override = child_style_changed.then_some(child_style);
            let measured = self.layout_node_with_edges(
                tree,
                item.id,
                child_style_override.clone(),
                NodeLayoutContext {
                    constraints: child_constraints,
                    offset: Point::ZERO,
                    sticky_constraints: child_constraints,
                    edges: item_edges,
                    rounding,
                    flex: FlexNodeContext::default(),
                },
            );
            let inline_offset = self.grid_inline_item_offset(
                style,
                &item.style,
                cell_width,
                measured.size.width,
                item_edges.margin,
            );
            let block_offset = self.grid_block_item_offset(
                style,
                &item.style,
                cell_height,
                measured.size.height,
                item_edges.margin,
            );
            let inline_position = if style.direction.is_any_rtl() {
                content_width
                    - column_offsets[item.column]
                    - self.grid_inline_item_end_offset(
                        style,
                        &item.style,
                        cell_width,
                        measured.size.width,
                        item_edges.margin,
                    )
                    - measured.size.width
            } else {
                column_offsets[item.column] + inline_offset
            };
            let child_offset = Point::new(
                edges.border.left + edges.padding.left + inline_position,
                edges.border.top + edges.padding.top + row_offsets[item.row] + block_offset,
            );
            let final_layout = self.layout_node_with_edges(
                tree,
                item.id,
                child_style_override,
                NodeLayoutContext {
                    constraints: child_constraints,
                    offset: child_offset,
                    sticky_constraints,
                    edges: item_edges,
                    rounding,
                    flex: FlexNodeContext::default(),
                },
            );
            let mut child_layout = final_layout.layout;
            child_layout.margin = self.grid_item_used_margin(
                &item.style,
                cell_width,
                cell_height,
                measured.size,
                item_edges.margin,
            );
            let baseline = row_offsets[item.row]
                + block_offset
                + self.item_baseline_from_top_margin_edge(final_layout, item_edges);
            first_baseline = self.grid_first_baseline_candidate(
                first_baseline,
                GridBaselineCandidate {
                    row: item.row,
                    column: item.column,
                    order: item_order,
                    baseline,
                },
            );
            tree.set_layout_with_constraints(item.id, final_layout.constraints, child_layout);
        }

        let mut width = content_width + edges.padding.horizontal() + edges.border.horizontal();
        let mut height = content_height + edges.padding.vertical() + edges.border.vertical();
        let width_clamp_constraints = if width_uses_constraint_as_container_size {
            constraints
        } else {
            Constraints::new(SideConstraint::indefinite(), constraints.height)
        };
        let height_clamp_constraints = if height_uses_constraint_as_container_size {
            constraints
        } else {
            Constraints::new(constraints.width, SideConstraint::indefinite())
        };
        width = self.clamp_axis(
            style,
            Axis::Horizontal,
            width,
            width_clamp_constraints,
            edges,
        );
        height = self.clamp_axis(
            style,
            Axis::Vertical,
            height,
            height_clamp_constraints,
            edges,
        );
        let size = Size::new(width.max(0.0), height.max(0.0));
        self.layout_grid_out_of_flow_children(
            tree,
            children,
            GridOutOfFlowContext {
                container_style: style,
                edges,
                column_offsets: &column_offsets,
                row_offsets: &row_offsets,
                column_gap: resolved_column_gap,
                row_gap: resolved_row_gap,
                content_size: Size::new(content_width, content_height),
                scrollable_content_size,
                grid_offsets,
            },
            rounding,
        );
        LayoutOutput {
            size,
            baseline: first_baseline
                .and_then(|candidate| self.exported_container_baseline(Some(candidate.baseline))),
        }
    }

    fn grid_first_baseline_candidate(
        &self,
        current: Option<GridBaselineCandidate>,
        candidate: GridBaselineCandidate,
    ) -> Option<GridBaselineCandidate> {
        match current {
            None => Some(candidate),
            Some(current) => {
                let candidate_key = (candidate.row, candidate.column, candidate.order);
                let current_key = (current.row, current.column, current.order);
                Some(if candidate_key < current_key {
                    candidate
                } else {
                    current
                })
            }
        }
    }

    fn place_grid_items<T: LayoutTree>(
        &self,
        tree: &T,
        children: &[T::NodeId],
        container_style: &Style,
        parent_width_constraint: SideConstraint,
        offsets: GridPlacementOffsets,
    ) -> Vec<GridItem<T::NodeId>> {
        let is_column_flow = container_style.grid_auto_flow.is_column();
        let is_dense = container_style.grid_auto_flow.is_dense();
        let base_auto_flow_limit = if is_column_flow {
            (container_style.grid_template_rows.len() + offsets.row).max(1)
        } else {
            (container_style.grid_template_columns.len() + offsets.column).max(1)
        };
        let pending = children
            .iter()
            .copied()
            .map(|child| {
                let style = tree.style(child).clone();
                let column_placement = self.resolve_grid_axis_placement(
                    style.grid_column_start,
                    style.grid_column_end,
                    style.grid_column_span,
                    container_style.grid_template_columns.len(),
                    offsets.column,
                );
                let row_placement = self.resolve_grid_axis_placement(
                    style.grid_row_start,
                    style.grid_row_end,
                    style.grid_row_span,
                    container_style.grid_template_rows.len(),
                    offsets.row,
                );
                GridPendingItem {
                    id: child,
                    style,
                    row_placement,
                    column_placement,
                }
            })
            .collect::<Vec<_>>();
        let mut auto_flow_limit =
            self.initial_grid_auto_flow_limit(&pending, base_auto_flow_limit, is_column_flow);
        let mut items = Vec::with_capacity(children.len());

        for pending_item in pending.iter().filter(|item| item.has_definite_area()) {
            let placement = GridCursor::new(
                pending_item
                    .row_placement
                    .start
                    .expect("pre-placed grid item should have a row"),
                pending_item
                    .column_placement
                    .start
                    .expect("pre-placed grid item should have a column"),
            );
            self.update_grid_auto_flow_limit(
                &mut auto_flow_limit,
                placement,
                pending_item.row_placement.span,
                pending_item.column_placement.span,
                is_column_flow,
            );
            items.push(self.grid_item_from_pending(
                pending_item,
                placement,
                parent_width_constraint,
            ));
        }

        let mut locked_cross_axis_cache = Vec::new();
        for pending_item in pending.iter().filter(|item| {
            !item.has_definite_area() && item.is_locked_to_auto_placement_cross_axis(is_column_flow)
        }) {
            let row_span = pending_item.row_placement.span;
            let column_span = pending_item.column_placement.span;
            let request = GridPlacementRequest {
                row_span,
                column_span,
                is_column_flow,
                auto_flow_limit,
            };
            let placement = if is_column_flow {
                let column = pending_item
                    .column_placement
                    .start
                    .expect("column-flow locked cross-axis item should have a column");
                if locked_cross_axis_cache.len() <= column {
                    locked_cross_axis_cache.resize(column + 1, 0);
                }
                let row = if is_dense {
                    0
                } else {
                    locked_cross_axis_cache[column]
                };
                let placement = self.find_grid_position_in_column(&items, column, row, request);
                if !is_dense {
                    locked_cross_axis_cache[column] = placement.row + row_span;
                }
                placement
            } else {
                let row = pending_item
                    .row_placement
                    .start
                    .expect("row-flow locked cross-axis item should have a row");
                if locked_cross_axis_cache.len() <= row {
                    locked_cross_axis_cache.resize(row + 1, 0);
                }
                let column = if is_dense {
                    0
                } else {
                    locked_cross_axis_cache[row]
                };
                let placement = self.find_grid_position_in_row(&items, row, column, request);
                if !is_dense {
                    locked_cross_axis_cache[row] = placement.column + column_span;
                }
                placement
            };

            self.update_grid_auto_flow_limit(
                &mut auto_flow_limit,
                placement,
                row_span,
                column_span,
                is_column_flow,
            );
            items.push(self.grid_item_from_pending(
                pending_item,
                placement,
                parent_width_constraint,
            ));
        }

        let mut cursor = GridCursor::default();
        for pending_item in pending.iter().filter(|item| {
            !item.has_definite_area()
                && !item.is_locked_to_auto_placement_cross_axis(is_column_flow)
        }) {
            let column_span = pending_item.column_placement.span;
            let row_span = pending_item.row_placement.span;
            let request = GridPlacementRequest {
                row_span,
                column_span,
                is_column_flow,
                auto_flow_limit,
            };

            debug_assert!(!pending_item.has_definite_area());
            let placement = if let Some(row) = pending_item.row_placement.start {
                debug_assert!(is_column_flow);
                let previous_row = cursor.row;
                cursor.row = row;
                if is_dense {
                    cursor.column = 0;
                } else if cursor.row < previous_row {
                    cursor.column += 1;
                }
                self.find_grid_position_in_row(&items, cursor.row, cursor.column, request)
            } else if let Some(column) = pending_item.column_placement.start {
                debug_assert!(!is_column_flow);
                let previous_column = cursor.column;
                cursor.column = column;
                if is_dense {
                    cursor.row = 0;
                } else if cursor.column < previous_column {
                    cursor.row += 1;
                }
                self.find_grid_position_in_column(&items, cursor.column, cursor.row, request)
            } else {
                self.find_grid_position(
                    &items,
                    if is_dense {
                        GridCursor::default()
                    } else {
                        cursor
                    },
                    request,
                )
            };

            items.push(self.grid_item_from_pending(
                pending_item,
                placement,
                parent_width_constraint,
            ));

            cursor = placement;
        }

        items
    }

    fn grid_item_from_pending<N: Copy>(
        &self,
        item: &GridPendingItem<N>,
        placement: GridCursor,
        parent_width_constraint: SideConstraint,
    ) -> GridItem<N> {
        let edges = self.resolve_edges_for_parent(&item.style, parent_width_constraint);
        GridItem {
            id: item.id,
            style: item.style.clone(),
            edges,
            row: placement.row,
            column: placement.column,
            row_span: item.row_placement.span,
            column_span: item.column_placement.span,
        }
    }

    fn grid_final_item_edges(
        &self,
        style: &Style,
        mut edges: ResolvedEdges,
        cell_width: f32,
    ) -> ResolvedEdges {
        let cell_width_base = Some(cell_width);
        edges.margin = self.update_percent_rect(style.margin, edges.margin, cell_width_base);
        edges.padding =
            self.update_percent_padding_rect(style.padding, edges.padding, cell_width_base);

        let inline_percent_base = self
            .resolve_axis_length(style.width, Some(cell_width))
            .unwrap_or(cell_width)
            .max(0.0);
        let inline_percent_base = Some(inline_percent_base);
        edges.padding.right = self.update_percent_edge(
            style.padding.right,
            edges.padding.right,
            inline_percent_base,
        );
        edges
    }

    fn initial_grid_auto_flow_limit<N>(
        &self,
        pending: &[GridPendingItem<N>],
        base_limit: usize,
        is_column_flow: bool,
    ) -> usize {
        pending.iter().fold(base_limit, |limit, item| {
            let placement = if is_column_flow {
                item.row_placement
            } else {
                item.column_placement
            };
            let limit = limit.max(placement.span);
            placement
                .start
                .map_or(limit, |start| limit.max(start + placement.span))
        })
    }

    fn update_grid_auto_flow_limit(
        &self,
        auto_flow_limit: &mut usize,
        placement: GridCursor,
        row_span: usize,
        column_span: usize,
        is_column_flow: bool,
    ) {
        if is_column_flow {
            *auto_flow_limit = (*auto_flow_limit).max(placement.row + row_span);
        } else {
            *auto_flow_limit = (*auto_flow_limit).max(placement.column + column_span);
        }
    }

    fn grid_placement_offsets<T: LayoutTree>(
        &self,
        tree: &T,
        children: &[T::NodeId],
        container_style: &Style,
    ) -> GridPlacementOffsets {
        let column = self.grid_axis_offset(
            children.iter().map(|child| {
                let style = tree.style(*child);
                GridAxisLineInput {
                    start: style.grid_column_start,
                    end: style.grid_column_end,
                    span: style.grid_column_span,
                }
            }),
            container_style.grid_template_columns.len(),
        );
        let row = self.grid_axis_offset(
            children.iter().map(|child| {
                let style = tree.style(*child);
                GridAxisLineInput {
                    start: style.grid_row_start,
                    end: style.grid_row_end,
                    span: style.grid_row_span,
                }
            }),
            container_style.grid_template_rows.len(),
        );
        GridPlacementOffsets { row, column }
    }

    fn grid_axis_offset(
        &self,
        inputs: impl Iterator<Item = GridAxisLineInput>,
        explicit_track_count: usize,
    ) -> usize {
        let explicit_end = explicit_track_count as i32 + 1;
        let mut min_axis = 1;
        for input in inputs {
            debug_assert!(input.span > 0);
            let span = input.span as i32;
            let start = input.start;
            let end = input.end.filter(|line| Some(*line) != start);
            if let Some(line) = start.filter(|line| *line < 0) {
                min_axis = min_axis.min(normalize_grid_line(line, explicit_end));
            }
            if let Some(line) = end.filter(|line| *line < 0) {
                min_axis = min_axis.min(normalize_grid_line(line, explicit_end) - span);
            }
            if start.is_none() {
                if let Some(line) = end.filter(|line| *line > 0) {
                    min_axis = min_axis.min(line - span);
                }
            }
        }
        (1 - min_axis).max(0) as usize
    }

    fn resolve_grid_axis_placement(
        &self,
        start: Option<i32>,
        end: Option<i32>,
        span: usize,
        explicit_track_count: usize,
        axis_offset: usize,
    ) -> GridAxisPlacement {
        let explicit_end = explicit_track_count as i32 + 1;
        let raw_start = start;
        let raw_end = end.filter(|line| Some(*line) != raw_start);
        let axis_offset = axis_offset as i32;
        let mut start = raw_start.map(|line| normalize_grid_line(line, explicit_end) + axis_offset);
        let mut end = raw_end.map(|line| normalize_grid_line(line, explicit_end) + axis_offset);
        debug_assert!(span > 0);
        let mut span = span;

        match (start, end) {
            (Some(start_line), Some(end_line)) if start_line > end_line => {
                start = Some(end_line);
                end = Some(start_line);
            }
            _ => {}
        }

        match (start, end) {
            (Some(start_line), Some(end_line)) if end_line > start_line => {
                span = (end_line - start_line) as usize;
            }
            (Some(_), None) => {}
            (None, Some(end_line)) => {
                start = Some(end_line - span as i32);
            }
            (Some(_), Some(_)) | (None, None) => {}
        }

        GridAxisPlacement {
            start: start.and_then(grid_line_to_index),
            span,
        }
    }

    fn find_grid_position<N>(
        &self,
        items: &[GridItem<N>],
        mut cursor: GridCursor,
        request: GridPlacementRequest,
    ) -> GridCursor {
        loop {
            cursor = self.normalize_grid_cursor(cursor, request);
            if self.grid_area_is_free(items, cursor, request) {
                return cursor;
            }
            cursor = self.advance_grid_scan_cursor(cursor, request);
        }
    }

    fn find_grid_position_in_row<N>(
        &self,
        items: &[GridItem<N>],
        row: usize,
        mut column: usize,
        request: GridPlacementRequest,
    ) -> GridCursor {
        loop {
            let cursor = GridCursor::new(row, column);
            if self.grid_area_is_free(items, cursor, request) {
                return cursor;
            }
            column += 1;
        }
    }

    fn find_grid_position_in_column<N>(
        &self,
        items: &[GridItem<N>],
        column: usize,
        mut row: usize,
        request: GridPlacementRequest,
    ) -> GridCursor {
        loop {
            let cursor = GridCursor::new(row, column);
            if self.grid_area_is_free(items, cursor, request) {
                return cursor;
            }
            row += 1;
        }
    }

    fn grid_area_is_free<N>(
        &self,
        items: &[GridItem<N>],
        cursor: GridCursor,
        request: GridPlacementRequest,
    ) -> bool {
        items.iter().all(|item| {
            !ranges_overlap(
                cursor.row,
                cursor.row + request.row_span,
                item.row,
                item.row + item.row_span,
            ) || !ranges_overlap(
                cursor.column,
                cursor.column + request.column_span,
                item.column,
                item.column + item.column_span,
            )
        })
    }

    fn normalize_grid_cursor(
        &self,
        mut cursor: GridCursor,
        request: GridPlacementRequest,
    ) -> GridCursor {
        if request.is_column_flow {
            if cursor.row + request.row_span > request.auto_flow_limit {
                cursor.column += 1;
                cursor.row = 0;
            }
        } else if cursor.column + request.column_span > request.auto_flow_limit {
            cursor.row += 1;
            cursor.column = 0;
        }
        cursor
    }

    fn advance_grid_scan_cursor(
        &self,
        mut cursor: GridCursor,
        request: GridPlacementRequest,
    ) -> GridCursor {
        if request.is_column_flow {
            cursor.row += 1;
        } else {
            cursor.column += 1;
        }
        self.normalize_grid_cursor(cursor, request)
    }

    fn resolve_grid_tracks(
        &self,
        definitions: GridTrackDefinitions<'_>,
        count: usize,
        content_limit: Option<f32>,
        _max_content_limit: Option<f32>,
        _gap: f32,
    ) -> (Vec<f32>, Vec<GridTrackSizing>, Vec<Option<f32>>) {
        let mut tracks = Vec::with_capacity(count);
        let mut track_sizing = Vec::with_capacity(count);
        let mut growth_limits = Vec::with_capacity(count);
        for idx in 0..count {
            let (min_track, max_track) = self.grid_track_definition(definitions, idx);
            let base_size = min_track.resolve(content_limit).unwrap_or(0.0).max(0.0);
            tracks.push(base_size);
            let kind = self.grid_track_kind(min_track, max_track, content_limit);
            track_sizing.push(GridTrackSizing {
                min_track,
                max_track: max_track.unwrap_or(min_track),
                kind,
            });
            growth_limits.push(self.grid_track_fixed_growth_limit(
                max_track,
                content_limit,
                base_size,
            ));
        }

        (tracks, track_sizing, growth_limits)
    }

    fn resolve_grid_tracks_for_axis(
        &self,
        definitions: GridTrackDefinitions<'_>,
        count: usize,
        measured_items: &[GridMeasuredItem],
        horizontal: bool,
        content_limit: Option<f32>,
        min_content_limit: Option<f32>,
        max_content_limit: Option<f32>,
        gap: f32,
        axis_constraint: SideConstraint,
        min_content_constraint: bool,
    ) -> (Vec<f32>, Vec<GridTrackSizing>) {
        let (mut tracks, track_sizing, mut growth_limits) =
            self.resolve_grid_tracks(definitions, count, content_limit, max_content_limit, gap);
        for group in Self::grid_intrinsic_growth_groups(measured_items, &track_sizing, horizontal) {
            let contributions =
                Self::grid_intrinsic_group_contributions(measured_items, &group, horizontal);
            self.grow_grid_intrinsic_track_group(
                &mut tracks,
                GridIntrinsicGrowthGroupContext {
                    track_sizing: &track_sizing,
                    growth_limits: &mut growth_limits,
                    contributions: &contributions,
                    gap,
                    container_axis_is_definite: content_limit.is_some(),
                    container_axis_is_indefinite: content_limit.is_none()
                        && axis_constraint.mode == MeasureMode::Indefinite,
                    container_axis_is_min_content_constraint: min_content_constraint,
                    container_axis_has_max_limit: max_content_limit.is_some(),
                },
            );
        }

        self.finalize_grid_tracks(
            &mut tracks,
            &track_sizing,
            &growth_limits,
            content_limit,
            max_content_limit,
            min_content_constraint,
            gap,
        );

        if content_limit.is_none() {
            self.expand_grid_flexible_tracks_for_indefinite_content(
                &mut tracks,
                &track_sizing,
                measured_items,
                GridFlexibleExpansionContext {
                    gap,
                    min_content_limit,
                    max_content_limit,
                    min_content_constraint,
                    horizontal,
                },
            );
        }

        (tracks, track_sizing)
    }

    fn grid_aligned_track_gap_for_contribution(
        &self,
        style: &Style,
        horizontal: bool,
        tracks: &[f32],
        gap: f32,
        content_size: Option<f32>,
    ) -> f32 {
        let Some(content_size) = content_size else {
            return gap;
        };
        let gap = self.resolve_gap_with_content_size(style, horizontal, content_size, gap);
        let free_space = (content_size - self.track_total_size(tracks, gap)).max(0.0);
        if horizontal {
            self.justify(style.justify_content, free_space, tracks.len(), gap)
                .1
        } else {
            self.align_content_with_gap(style.align_content, free_space, tracks.len(), gap)
                .1
        }
    }

    fn update_grid_block_axis_contributions_after_column_sizing<T: LayoutTree>(
        &mut self,
        tree: &mut T,
        container_style: &Style,
        items: &[GridItem<T::NodeId>],
        measured_items: &mut [GridMeasuredItem],
        columns: &[f32],
        column_gap: f32,
        content_height_limit: Option<f32>,
    ) -> bool {
        let mut minimum_contribution_changed = false;
        for (item, measured_item) in items.iter().zip(measured_items.iter_mut()) {
            let cell_width =
                self.spanned_track_size(columns, item.column, item.column_span, column_gap);
            let child_constraints = self.grid_intrinsic_block_child_constraints(
                container_style,
                &item.style,
                cell_width,
                item.edges,
            );
            let measured = self.layout_node(tree, item.id, child_constraints, Point::ZERO);
            let outer_height = measured.size.height + item.edges.margin.vertical();
            measured_item.outer_height = self.grid_item_max_content_contribution(
                tree,
                item.id,
                &item.style,
                item.edges,
                false,
                child_constraints,
                outer_height,
                content_height_limit,
            );
            let minimum_height = self.grid_item_minimum_contribution(
                tree,
                item.id,
                &item.style,
                item.edges,
                false,
                child_constraints,
                content_height_limit,
            );
            minimum_contribution_changed |=
                self.grid_contribution_changed(measured_item.minimum_height, minimum_height);
            measured_item.minimum_height = minimum_height;
        }
        minimum_contribution_changed
    }

    fn update_grid_inline_axis_contributions_after_row_sizing<T: LayoutTree>(
        &mut self,
        tree: &mut T,
        container_style: &Style,
        items: &[GridItem<T::NodeId>],
        measured_items: &mut [GridMeasuredItem],
        rows: &[f32],
        row_gap: f32,
        content_width_limit: Option<f32>,
    ) -> bool {
        let mut minimum_contribution_changed = false;
        for (item, measured_item) in items.iter().zip(measured_items.iter_mut()) {
            let cell_height = self.spanned_track_size(rows, item.row, item.row_span, row_gap);
            let child_constraints = self.grid_intrinsic_inline_contribution_child_constraints(
                container_style,
                &item.style,
                cell_height,
                item.edges,
            );
            let measured = self.layout_node(tree, item.id, child_constraints, Point::ZERO);
            let outer_width = measured.size.width + item.edges.margin.horizontal();
            measured_item.outer_width = self.grid_item_max_content_contribution(
                tree,
                item.id,
                &item.style,
                item.edges,
                true,
                child_constraints,
                outer_width,
                content_width_limit,
            );
            let minimum_width = self.grid_item_minimum_contribution(
                tree,
                item.id,
                &item.style,
                item.edges,
                true,
                child_constraints,
                content_width_limit,
            );
            minimum_contribution_changed |=
                self.grid_contribution_changed(measured_item.minimum_width, minimum_width);
            measured_item.minimum_width = minimum_width;
        }
        minimum_contribution_changed
    }

    fn grid_contribution_changed(&self, previous: f32, next: f32) -> bool {
        (previous - next).abs() > self.epsilon
    }

    fn finalize_grid_tracks(
        &self,
        tracks: &mut [f32],
        track_sizing: &[GridTrackSizing],
        growth_limits: &[Option<f32>],
        content_limit: Option<f32>,
        max_content_limit: Option<f32>,
        min_content_constraint: bool,
        gap: f32,
    ) {
        if let Some(content_limit) = content_limit {
            self.maximize_grid_tracks(tracks, growth_limits, content_limit, gap);
            let occupied_size = self.track_total_size(tracks, gap);
            let remaining_size = (content_limit - occupied_size).max(0.0);
            if remaining_size > self.epsilon {
                self.expand_grid_flexible_tracks(tracks, track_sizing, content_limit, gap);
            }
        } else if min_content_constraint {
            return;
        } else {
            self.maximize_grid_tracks_for_indefinite_content(
                tracks,
                growth_limits,
                max_content_limit,
                gap,
            );
        }
    }

    fn grid_content_size_after_min_max(
        &self,
        tracks: &[f32],
        gap: f32,
        content_limit: Option<f32>,
        min_content_limit: Option<f32>,
        max_content_limit: Option<f32>,
    ) -> Option<f32> {
        let mut content_size = content_limit.unwrap_or_else(|| self.track_total_size(tracks, gap));
        let original_content_size = content_size;
        if let Some(min_content_limit) = min_content_limit {
            content_size = content_size.max(min_content_limit);
        }
        if let Some(max_content_limit) = max_content_limit {
            content_size = content_size.min(max_content_limit);
        }

        (content_limit.is_some() || (content_size - original_content_size).abs() > self.epsilon)
            .then_some(content_size.max(0.0))
    }

    fn grid_track_fixed_growth_limit(
        &self,
        max_track: Option<Length>,
        percent_base: Option<f32>,
        base_size: f32,
    ) -> Option<f32> {
        let max_track = max_track?;
        match max_track {
            Length::Points(_) | Length::Percent(_) | Length::Calc { .. } => max_track
                .resolve(percent_base)
                .map(|limit| limit.max(0.0).max(base_size)),
            Length::Auto
            | Length::Fr(_)
            | Length::MinContent
            | Length::MaxContent
            | Length::FitContent(_) => None,
        }
    }

    fn maximize_grid_tracks_for_indefinite_content(
        &self,
        tracks: &mut [f32],
        growth_limits: &[Option<f32>],
        max_content_limit: Option<f32>,
        _gap: f32,
    ) {
        let original_tracks = tracks.to_vec();
        for (track, limit) in tracks.iter_mut().zip(growth_limits.iter()) {
            if let Some(limit) = limit {
                if *limit > *track + self.epsilon {
                    *track = *limit;
                }
            }
        }

        let Some(max_content_limit) = max_content_limit else {
            return;
        };
        let grown_track_sum = tracks.iter().sum::<f32>();
        if grown_track_sum <= max_content_limit + self.epsilon {
            return;
        }

        tracks.copy_from_slice(&original_tracks);
        let free_space = max_content_limit - tracks.iter().sum::<f32>();
        self.maximize_grid_tracks_by_free_space(tracks, growth_limits, free_space);
    }

    fn maximize_grid_tracks(
        &self,
        tracks: &mut [f32],
        growth_limits: &[Option<f32>],
        content_limit: f32,
        gap: f32,
    ) {
        let free_space = content_limit - self.track_total_size(tracks, gap);
        self.maximize_grid_tracks_by_free_space(tracks, growth_limits, free_space);
    }

    fn maximize_grid_tracks_by_free_space(
        &self,
        tracks: &mut [f32],
        growth_limits: &[Option<f32>],
        mut free_space: f32,
    ) {
        let mut unfrozen_tracks = tracks.len();
        while free_space > self.epsilon && unfrozen_tracks > 0 {
            let space_per_track = free_space / unfrozen_tracks as f32;
            unfrozen_tracks = 0;
            let mut consumed = 0.0;
            for (track, limit) in tracks.iter_mut().zip(growth_limits.iter()) {
                let Some(limit) = *limit else {
                    continue;
                };
                if limit <= *track + self.epsilon {
                    continue;
                }

                let max_increment = limit - *track;
                let increase = max_increment.min(space_per_track);
                *track += increase;
                consumed += increase;
                free_space -= increase;
                if max_increment > space_per_track + self.epsilon {
                    unfrozen_tracks += 1;
                }
            }
            if consumed <= self.epsilon {
                break;
            }
        }
    }

    fn expand_grid_flexible_tracks(
        &self,
        tracks: &mut [f32],
        track_sizing: &[GridTrackSizing],
        content_limit: f32,
        gap: f32,
    ) {
        let flex_factors = self.grid_flex_factors(track_sizing);
        if !flex_factors.iter().any(|factor| *factor > self.epsilon) {
            return;
        }

        let space_to_fill = content_limit - self.gap_total(gap, tracks.len());
        let fr_size = self.find_grid_fr_size(tracks, &flex_factors, space_to_fill);
        debug_assert!(fr_size > self.epsilon);

        for (track, flex) in tracks.iter_mut().zip(flex_factors.iter()) {
            if *flex <= self.epsilon {
                continue;
            }
            *track = (*track).max(fr_size * *flex);
        }
    }

    fn expand_grid_flexible_tracks_for_indefinite_content(
        &self,
        tracks: &mut [f32],
        track_sizing: &[GridTrackSizing],
        measured_items: &[GridMeasuredItem],
        context: GridFlexibleExpansionContext,
    ) {
        let flex_factors = self.grid_flex_factors(track_sizing);
        if !flex_factors.iter().any(|factor| *factor > self.epsilon) {
            return;
        }
        if context.min_content_constraint {
            return;
        }

        let mut flex_fraction: f32 = 0.0;
        for (track, flex) in tracks.iter().zip(flex_factors.iter()) {
            if *flex > 1.0 {
                flex_fraction = flex_fraction.max(*track / *flex);
            } else if *flex > self.epsilon {
                flex_fraction = flex_fraction.max(*track);
            }
        }

        for item in measured_items {
            let (start, span, contribution) = if context.horizontal {
                (item.column, item.column_span, item.outer_width)
            } else {
                (item.row, item.row_span, item.outer_height)
            };
            let end = (start + span).min(tracks.len());
            debug_assert!(start < end);
            if !(start..end).any(|idx| flex_factors[idx] > self.epsilon) {
                continue;
            }

            let mut item_flex_factors = flex_factors.clone();
            for (idx, flex) in item_flex_factors.iter_mut().enumerate() {
                if idx < start || idx >= end {
                    *flex = -1.0;
                }
            }
            let space_to_fill = contribution - self.gap_total(context.gap, end - start);
            let item_fr_size = self.find_grid_fr_size(tracks, &item_flex_factors, space_to_fill);
            flex_fraction = flex_fraction.max(item_fr_size);
        }

        let mut hypothetical_grid_size = self.gap_total(context.gap, tracks.len());
        for (track, flex) in tracks.iter().zip(flex_factors.iter()) {
            hypothetical_grid_size += if *flex > self.epsilon {
                flex_fraction * *flex
            } else {
                *track
            };
        }
        let mut applied_grid_size = hypothetical_grid_size;
        if let Some(min_content_limit) = context.min_content_limit {
            applied_grid_size = applied_grid_size.max(min_content_limit);
        }
        if let Some(max_content_limit) = context.max_content_limit {
            applied_grid_size = applied_grid_size.min(max_content_limit);
        }
        if (applied_grid_size - hypothetical_grid_size).abs() > self.epsilon {
            let free_space = applied_grid_size - self.track_total_size(tracks, context.gap);
            if free_space > self.epsilon {
                flex_fraction = self.find_grid_fr_size(
                    tracks,
                    &flex_factors,
                    applied_grid_size - self.gap_total(context.gap, tracks.len()),
                );
            } else {
                flex_fraction = 0.0;
            }
        }

        if flex_fraction <= self.epsilon {
            return;
        }
        for (track, flex) in tracks.iter_mut().zip(flex_factors.iter()) {
            if *flex > self.epsilon {
                *track = (*track).max(flex_fraction * *flex);
            }
        }
    }

    fn grid_intrinsic_growth_groups(
        measured_items: &[GridMeasuredItem],
        track_sizing: &[GridTrackSizing],
        horizontal: bool,
    ) -> Vec<Vec<usize>> {
        let mut order = (0..measured_items.len()).collect::<Vec<_>>();
        order.sort_by_key(|idx| {
            let (_, span, _, _) = Self::grid_measured_item_axis(&measured_items[*idx], horizontal);
            span
        });

        let mut groups: Vec<Vec<usize>> = Vec::new();
        let mut current_span = None;
        let mut flexible = Vec::new();
        for item_index in order {
            let (start, span, _, _) =
                Self::grid_measured_item_axis(&measured_items[item_index], horizontal);
            if Self::grid_span_crosses_flexible_track(track_sizing, start, span) {
                flexible.push(item_index);
                continue;
            }

            if current_span != Some(span) {
                groups.push(Vec::new());
                current_span = Some(span);
            }
            groups
                .last_mut()
                .expect("a non-flexible grid growth group exists")
                .push(item_index);
        }
        if !flexible.is_empty() {
            groups.push(flexible);
        }
        groups
    }

    fn grid_intrinsic_group_contributions(
        measured_items: &[GridMeasuredItem],
        group: &[usize],
        horizontal: bool,
    ) -> Vec<GridIntrinsicContribution> {
        group
            .iter()
            .map(|idx| {
                let (start, span, required_size, minimum_size) =
                    Self::grid_measured_item_axis(&measured_items[*idx], horizontal);
                GridIntrinsicContribution {
                    start,
                    span,
                    required_size,
                    minimum_size,
                }
            })
            .collect()
    }

    const fn grid_measured_item_axis(
        item: &GridMeasuredItem,
        horizontal: bool,
    ) -> (usize, usize, f32, f32) {
        if horizontal {
            (
                item.column,
                item.column_span,
                item.outer_width,
                item.minimum_width,
            )
        } else {
            (
                item.row,
                item.row_span,
                item.outer_height,
                item.minimum_height,
            )
        }
    }

    fn grid_span_crosses_flexible_track(
        track_sizing: &[GridTrackSizing],
        start: usize,
        span: usize,
    ) -> bool {
        let end = (start + span).min(track_sizing.len());
        (start..end).any(|idx| matches!(track_sizing[idx].kind, GridTrackKind::Flexible(_)))
    }

    fn grid_flex_factors(&self, track_sizing: &[GridTrackSizing]) -> Vec<f32> {
        track_sizing
            .iter()
            .map(|sizing| match sizing.kind {
                GridTrackKind::Flexible(flex) if flex > self.epsilon => flex,
                GridTrackKind::Fixed
                | GridTrackKind::Auto
                | GridTrackKind::MinContent
                | GridTrackKind::MaxContent
                | GridTrackKind::Flexible(_)
                | GridTrackKind::FitContent(_) => 0.0,
            })
            .collect()
    }

    fn find_grid_fr_size(
        &self,
        base_sizes: &[f32],
        flex_factors: &[f32],
        space_to_fill: f32,
    ) -> f32 {
        let mut used_flex_factors = flex_factors.to_vec();
        loop {
            let mut leftover_space = space_to_fill;
            let mut flex_factor_sum: f32 = 0.0;
            for (base_size, flex) in base_sizes.iter().zip(used_flex_factors.iter()) {
                if flex.abs() <= self.epsilon {
                    leftover_space -= *base_size;
                } else if *flex > self.epsilon {
                    flex_factor_sum += *flex;
                }
            }
            flex_factor_sum = flex_factor_sum.max(1.0);
            let hypothetical_fr_size = leftover_space / flex_factor_sum;
            let mut froze_track = false;
            for (base_size, flex) in base_sizes.iter().zip(used_flex_factors.iter_mut()) {
                if *flex > self.epsilon && *base_size > hypothetical_fr_size * *flex + self.epsilon
                {
                    *flex = 0.0;
                    froze_track = true;
                }
            }
            if !froze_track {
                return hypothetical_fr_size;
            }
        }
    }

    fn stretch_grid_auto_tracks(
        &self,
        tracks: &mut [f32],
        track_sizing: &[GridTrackSizing],
        content_limit: Option<f32>,
        gap: f32,
        is_stretch: bool,
    ) {
        if !is_stretch {
            return;
        }
        let Some(content_limit) = content_limit else {
            return;
        };

        let auto_count = track_sizing
            .iter()
            .filter(|sizing| sizing.max_track == Length::Auto)
            .count();
        if auto_count == 0 {
            return;
        }

        let used_size = self.track_total_size(tracks, gap);
        let free_space = content_limit - used_size;
        if free_space <= self.epsilon {
            return;
        }

        let extra = free_space / auto_count as f32;
        for (track, sizing) in tracks.iter_mut().zip(track_sizing.iter()) {
            if sizing.max_track == Length::Auto {
                *track += extra;
            }
        }
    }

    fn grid_track_definition(
        &self,
        definitions: GridTrackDefinitions<'_>,
        idx: usize,
    ) -> (Length, Option<Length>) {
        if idx < definitions.leading_implicit {
            let pattern_idx = leading_implicit_pattern_index(
                idx,
                definitions.leading_implicit,
                definitions.implicit_min.len(),
            );
            return (
                definitions
                    .implicit_min
                    .get(pattern_idx)
                    .copied()
                    .unwrap_or(Length::Auto),
                definitions.implicit_max.get(pattern_idx).copied(),
            );
        }

        let explicit_idx = idx - definitions.leading_implicit;
        if explicit_idx < definitions.explicit_min.len() {
            return (
                definitions.explicit_min[explicit_idx],
                definitions.explicit_max.get(explicit_idx).copied(),
            );
        }

        let positive_implicit_idx = explicit_idx - definitions.explicit_min.len();
        let pattern_idx =
            positive_implicit_pattern_index(positive_implicit_idx, definitions.implicit_min.len());
        (
            definitions
                .implicit_min
                .get(pattern_idx)
                .copied()
                .unwrap_or(Length::Auto),
            definitions.implicit_max.get(pattern_idx).copied(),
        )
    }

    fn grid_track_kind(
        &self,
        min_track: Length,
        max_track: Option<Length>,
        percent_base: Option<f32>,
    ) -> GridTrackKind {
        match max_track {
            Some(Length::Fr(flex)) => {
                debug_assert!(flex >= 0.0);
                GridTrackKind::Flexible(flex)
            }
            Some(Length::FitContent(base)) => self.grid_track_fit_content_kind(base, percent_base),
            Some(Length::Auto) => GridTrackKind::Auto,
            Some(Length::MinContent) => GridTrackKind::MinContent,
            Some(Length::MaxContent) => GridTrackKind::MaxContent,
            None => self.grid_track_kind_from_single_length(min_track, percent_base),
            Some(track) => self.grid_track_kind_from_single_length(track, percent_base),
        }
    }

    fn grid_track_kind_from_single_length(
        &self,
        track: Length,
        percent_base: Option<f32>,
    ) -> GridTrackKind {
        match track {
            Length::Fr(flex) => {
                debug_assert!(flex >= 0.0);
                GridTrackKind::Flexible(flex)
            }
            Length::FitContent(base) => self.grid_track_fit_content_kind(base, percent_base),
            Length::Auto => GridTrackKind::Auto,
            Length::MinContent => GridTrackKind::MinContent,
            Length::MaxContent => GridTrackKind::MaxContent,
            Length::Points(_) | Length::Percent(_) | Length::Calc { .. } => GridTrackKind::Fixed,
        }
    }

    fn grid_track_fit_content_kind(
        &self,
        base: Option<crate::BaseLength>,
        percent_base: Option<f32>,
    ) -> GridTrackKind {
        let message = "grid track fit-content requires a <length-percentage> argument";
        let base = base.expect(message);
        GridTrackKind::FitContent(self.resolve_fit_content_limit(Some(base), percent_base))
    }

    fn resolve_fit_content_limit(
        &self,
        base: Option<crate::BaseLength>,
        percent_base: Option<f32>,
    ) -> Option<f32> {
        let base = base?;
        if base.contains_percentage() {
            percent_base.map(|percent_base| {
                (base.fixed_part() + percent_base * (base.percentage_part() / 100.0)).max(0.0)
            })
        } else {
            Some(base.fixed_part().max(0.0))
        }
    }

    fn grid_child_constraints(
        &self,
        container_style: &Style,
        child_style: &Style,
        cell_width: f32,
        cell_height: f32,
        edges: ResolvedEdges,
    ) -> Constraints {
        Constraints::new(
            self.grid_axis_child_constraint(container_style, child_style, true, cell_width, edges),
            self.grid_axis_child_constraint(
                container_style,
                child_style,
                false,
                cell_height,
                edges,
            ),
        )
    }

    fn grid_intrinsic_inline_child_constraints(&self) -> Constraints {
        Constraints::indefinite()
    }

    fn grid_intrinsic_inline_contribution_child_constraints(
        &self,
        container_style: &Style,
        child_style: &Style,
        cell_height: f32,
        edges: ResolvedEdges,
    ) -> Constraints {
        Constraints::new(
            SideConstraint::indefinite(),
            self.grid_axis_child_constraint(
                container_style,
                child_style,
                false,
                cell_height,
                edges,
            ),
        )
    }

    fn grid_intrinsic_block_child_constraints(
        &self,
        container_style: &Style,
        child_style: &Style,
        cell_width: f32,
        edges: ResolvedEdges,
    ) -> Constraints {
        Constraints::new(
            self.grid_axis_child_constraint(container_style, child_style, true, cell_width, edges),
            SideConstraint::indefinite(),
        )
    }

    fn grid_measured_child_constraints(
        &self,
        container_style: &Style,
        child_style: &Style,
        mut constraints: Constraints,
    ) -> Constraints {
        if child_style.width == Length::Auto
            && !self.grid_axis_stretches(container_style, child_style, true)
        {
            constraints.width = SideConstraint::indefinite();
        }
        if child_style.height == Length::Auto
            && !self.grid_axis_stretches(container_style, child_style, false)
        {
            constraints.height = SideConstraint::indefinite();
        }
        constraints
    }

    fn grid_axis_child_constraint(
        &self,
        container_style: &Style,
        child_style: &Style,
        horizontal: bool,
        cell_size: f32,
        edges: ResolvedEdges,
    ) -> SideConstraint {
        let constraint =
            self.grid_default_axis_child_constraint(child_style, horizontal, cell_size, edges);
        if constraint.is_at_most()
            && self.grid_axis_stretches(container_style, child_style, horizontal)
            && !self.has_axis_auto_margin(child_style, horizontal)
        {
            SideConstraint::definite(constraint.size)
        } else {
            constraint
        }
    }

    fn grid_default_axis_child_constraint(
        &self,
        child_style: &Style,
        horizontal: bool,
        cell_size: f32,
        edges: ResolvedEdges,
    ) -> SideConstraint {
        let axis = if horizontal {
            Axis::Horizontal
        } else {
            Axis::Vertical
        };
        let length = if horizontal {
            child_style.width
        } else {
            child_style.height
        };
        match length {
            Length::MinContent | Length::MaxContent => SideConstraint::indefinite(),
            Length::FitContent(base) => {
                let available = (cell_size - self.axis_margin(edges.margin, horizontal)).max(0.0);
                SideConstraint::at_most(
                    self.resolve_fit_content_limit(base, Some(cell_size))
                        .unwrap_or(available),
                )
            }
            _ => {
                let constraints = if horizontal {
                    Constraints::new(
                        SideConstraint::definite(cell_size),
                        SideConstraint::indefinite(),
                    )
                } else {
                    Constraints::new(
                        SideConstraint::indefinite(),
                        SideConstraint::definite(cell_size),
                    )
                };
                self.resolve_border_axis(length, axis, constraints, edges, child_style.box_sizing)
                    .map(SideConstraint::definite)
                    .unwrap_or_else(|| {
                        SideConstraint::at_most(
                            (cell_size - self.axis_margin(edges.margin, horizontal)).max(0.0),
                        )
                    })
            }
        }
    }

    fn grid_inline_item_offset(
        &self,
        container_style: &Style,
        child_style: &Style,
        cell_width: f32,
        child_width: f32,
        margin: Edges,
    ) -> f32 {
        let justify = if child_style.justify_self == JustifyItems::Auto {
            container_style.justify_items
        } else {
            child_style.justify_self
        };
        self.grid_axis_item_offset(
            child_style,
            true,
            cell_width,
            child_width,
            margin,
            self.grid_justify_to_position(justify),
        )
    }

    fn grid_inline_item_end_offset(
        &self,
        container_style: &Style,
        child_style: &Style,
        cell_width: f32,
        child_width: f32,
        margin: Edges,
    ) -> f32 {
        let (_, end_margin) =
            self.grid_axis_used_margins(child_style, true, cell_width, child_width, margin);
        let start_auto = self.axis_start_margin_is_auto(child_style, true);
        let end_auto = self.axis_end_margin_is_auto(child_style, true);
        if start_auto || end_auto {
            return end_margin;
        }
        end_margin
            + self.grid_inline_alignment_offset(
                container_style,
                child_style,
                cell_width,
                child_width,
                margin.horizontal(),
            )
    }

    fn grid_block_item_offset(
        &self,
        container_style: &Style,
        child_style: &Style,
        cell_height: f32,
        child_height: f32,
        margin: Edges,
    ) -> f32 {
        let align = child_style
            .align_self
            .unwrap_or(container_style.align_items);
        self.grid_axis_item_offset(
            child_style,
            false,
            cell_height,
            child_height,
            margin,
            self.grid_align_to_position(align),
        )
    }

    fn grid_axis_item_offset(
        &self,
        child_style: &Style,
        horizontal: bool,
        cell_size: f32,
        child_size: f32,
        margin: Edges,
        alignment: GridAxisAlignment,
    ) -> f32 {
        let start_auto = self.axis_start_margin_is_auto(child_style, horizontal);
        let end_auto = self.axis_end_margin_is_auto(child_style, horizontal);
        let start_margin = if start_auto {
            0.0
        } else {
            self.axis_start_margin(margin, horizontal)
        };
        let end_margin = if end_auto {
            0.0
        } else {
            self.axis_end_margin(margin, horizontal)
        };
        let free = cell_size - child_size - start_margin - end_margin;

        if free > self.epsilon && (start_auto || end_auto) {
            return match (start_auto, end_auto) {
                (true, true) => start_margin + free / 2.0,
                (true, false) => start_margin + free,
                (false, true) | (false, false) => start_margin,
            };
        }

        match alignment {
            GridAxisAlignment::Start => start_margin,
            GridAxisAlignment::Center => start_margin + free / 2.0,
            GridAxisAlignment::End => start_margin + free,
        }
    }

    fn grid_item_used_margin(
        &self,
        child_style: &Style,
        cell_width: f32,
        cell_height: f32,
        child_size: Size,
        margin: Edges,
    ) -> Edges {
        let mut used_margin = margin;
        let (inline_start, inline_end) =
            self.grid_axis_used_margins(child_style, true, cell_width, child_size.width, margin);
        self.set_axis_start_margin(&mut used_margin, true, inline_start);
        self.set_axis_end_margin(&mut used_margin, true, inline_end);

        let (block_start, block_end) =
            self.grid_axis_used_margins(child_style, false, cell_height, child_size.height, margin);
        self.set_axis_start_margin(&mut used_margin, false, block_start);
        self.set_axis_end_margin(&mut used_margin, false, block_end);
        used_margin
    }

    fn grid_axis_used_margins(
        &self,
        child_style: &Style,
        horizontal: bool,
        cell_size: f32,
        child_size: f32,
        margin: Edges,
    ) -> (f32, f32) {
        let start_auto = self.axis_start_margin_is_auto(child_style, horizontal);
        let end_auto = self.axis_end_margin_is_auto(child_style, horizontal);
        let start_margin = if start_auto {
            0.0
        } else {
            self.axis_start_margin(margin, horizontal)
        };
        let end_margin = if end_auto {
            0.0
        } else {
            self.axis_end_margin(margin, horizontal)
        };
        let free = cell_size - child_size - start_margin - end_margin;

        if free > self.epsilon {
            if start_auto && end_auto {
                return (free / 2.0, free / 2.0);
            }
            if start_auto {
                return (free, end_margin);
            }
            if end_auto {
                return (start_margin, free);
            }
        }
        (start_margin, end_margin)
    }

    fn grid_justify_to_position(&self, justify: JustifyItems) -> GridAxisAlignment {
        match justify {
            JustifyItems::Auto | JustifyItems::Stretch | JustifyItems::Start => {
                GridAxisAlignment::Start
            }
            JustifyItems::Center => GridAxisAlignment::Center,
            JustifyItems::End => GridAxisAlignment::End,
        }
    }

    fn grid_align_to_position(&self, align: AlignItems) -> GridAxisAlignment {
        match align {
            AlignItems::Stretch
            | AlignItems::FlexStart
            | AlignItems::Start
            | AlignItems::Baseline => GridAxisAlignment::Start,
            AlignItems::Center => GridAxisAlignment::Center,
            AlignItems::FlexEnd | AlignItems::End => GridAxisAlignment::End,
        }
    }

    fn grid_axis_stretches(
        &self,
        container_style: &Style,
        child_style: &Style,
        horizontal: bool,
    ) -> bool {
        if horizontal {
            let justify = if child_style.justify_self == JustifyItems::Auto {
                container_style.justify_items
            } else {
                child_style.justify_self
            };
            justify == JustifyItems::Stretch
        } else {
            child_style
                .align_self
                .unwrap_or(container_style.align_items)
                == AlignItems::Stretch
        }
    }

    fn grid_inline_alignment_offset(
        &self,
        container_style: &Style,
        child_style: &Style,
        cell_width: f32,
        child_width: f32,
        horizontal_margin: f32,
    ) -> f32 {
        let justify = if child_style.justify_self == JustifyItems::Auto {
            container_style.justify_items
        } else {
            child_style.justify_self
        };
        let free = cell_width - child_width - horizontal_margin;
        match justify {
            JustifyItems::Auto | JustifyItems::Stretch | JustifyItems::Start => 0.0,
            JustifyItems::Center => free / 2.0,
            JustifyItems::End => free,
        }
    }

    fn grid_item_minimum_contribution<T: LayoutTree>(
        &self,
        tree: &mut T,
        node: T::NodeId,
        style: &Style,
        edges: ResolvedEdges,
        horizontal: bool,
        constraints: Constraints,
        percent_base: Option<f32>,
    ) -> f32 {
        let preferred_size = if horizontal {
            style.width
        } else {
            style.height
        };
        let min_content_contribution = self.grid_item_min_content_contribution(
            tree,
            node,
            style,
            edges,
            horizontal,
            constraints,
            percent_base,
        );
        if !Self::grid_preferred_size_behaves_auto_or_depends_on_containing_block(preferred_size) {
            return min_content_contribution.unwrap_or(self.axis_margin(edges.margin, horizontal));
        }

        if let Some(contribution) = min_content_contribution {
            return contribution;
        }

        let min_size = if horizontal {
            style.min_width
        } else {
            style.min_height
        };
        let axis = if horizontal {
            Axis::Horizontal
        } else {
            Axis::Vertical
        };
        let min_border_size = self
            .resolve_min_max_length(min_size, percent_base)
            .map(|value| self.border_size_from_css_axis(style, axis, value, edges))
            .unwrap_or(0.0);
        min_border_size + self.axis_margin(edges.margin, horizontal)
    }

    fn grid_item_min_content_contribution<T: LayoutTree>(
        &self,
        tree: &mut T,
        node: T::NodeId,
        style: &Style,
        edges: ResolvedEdges,
        horizontal: bool,
        constraints: Constraints,
        percent_base: Option<f32>,
    ) -> Option<f32> {
        let content_size = tree.measure_min_content(node, constraints)?;
        self.grid_content_box_intrinsic_contribution(
            tree,
            node,
            style,
            edges,
            horizontal,
            constraints,
            percent_base,
            content_size,
        )
    }

    fn grid_item_max_content_contribution<T: LayoutTree>(
        &self,
        tree: &mut T,
        node: T::NodeId,
        style: &Style,
        edges: ResolvedEdges,
        horizontal: bool,
        constraints: Constraints,
        fallback: f32,
        percent_base: Option<f32>,
    ) -> f32 {
        let preferred_size = if horizontal {
            style.width
        } else {
            style.height
        };
        if !Self::grid_preferred_size_uses_max_content_contribution(preferred_size) {
            return fallback;
        }

        tree.measure_max_content(node, constraints)
            .and_then(|content_size| {
                self.grid_content_box_intrinsic_contribution(
                    tree,
                    node,
                    style,
                    edges,
                    horizontal,
                    constraints,
                    percent_base,
                    content_size,
                )
            })
            .unwrap_or(fallback)
    }

    fn grid_content_box_intrinsic_contribution<T: LayoutTree>(
        &self,
        tree: &T,
        node: T::NodeId,
        style: &Style,
        edges: ResolvedEdges,
        horizontal: bool,
        constraints: Constraints,
        percent_base: Option<f32>,
        content_size: Size,
    ) -> Option<f32> {
        let (axis, content_axis, constraint) = if horizontal {
            (Axis::Horizontal, content_size.width, constraints.width)
        } else {
            (Axis::Vertical, content_size.height, constraints.height)
        };
        if !content_axis.is_finite() {
            return None;
        }

        let physical_pixels_per_layout_unit = tree.physical_pixels_per_layout_unit(node);
        let mut border_size = self.measured_content_axis(
            content_axis.max(0.0),
            constraint,
            physical_pixels_per_layout_unit,
        );
        border_size = self.border_size_from_content_axis(axis, border_size, edges);
        border_size = self.clamp_measured_axis(style, axis, border_size, percent_base, edges);
        Some(border_size + self.axis_margin(edges.margin, horizontal))
    }

    const fn grid_preferred_size_behaves_auto_or_depends_on_containing_block(
        length: Length,
    ) -> bool {
        matches!(length, Length::Auto | Length::Percent(_))
            || matches!(length, Length::Calc { percent, .. } if percent != 0.0)
    }

    const fn grid_preferred_size_uses_max_content_contribution(length: Length) -> bool {
        matches!(
            length,
            Length::Auto | Length::MinContent | Length::MaxContent | Length::FitContent(_)
        )
    }

    fn grow_grid_intrinsic_track_group(
        &self,
        tracks: &mut [f32],
        context: GridIntrinsicGrowthGroupContext<'_>,
    ) {
        debug_assert!(!context.contributions.is_empty());

        let base_tracks = tracks.to_vec();
        let base_growth_limits = context.growth_limits.to_vec();
        let mut planned_track_increase = vec![0.0_f32; tracks.len()];
        let mut planned_growth_limit_increase = vec![0.0_f32; context.growth_limits.len()];

        for contribution in context.contributions {
            let mut item_tracks = base_tracks.clone();
            let mut item_growth_limits = base_growth_limits.clone();
            self.grow_grid_intrinsic_tracks(
                &mut item_tracks,
                GridIntrinsicGrowthContext {
                    track_sizing: context.track_sizing,
                    growth_limits: &mut item_growth_limits,
                    span: contribution.start..contribution.start + contribution.span,
                    gap: context.gap,
                    required_size: contribution.required_size,
                    minimum_size: contribution.minimum_size,
                    container_axis_is_definite: context.container_axis_is_definite,
                    container_axis_is_indefinite: context.container_axis_is_indefinite,
                    container_axis_is_min_content_constraint: context
                        .container_axis_is_min_content_constraint,
                    container_axis_has_max_limit: context.container_axis_has_max_limit,
                },
            );

            for (idx, (item_track, base_track)) in
                item_tracks.iter().zip(base_tracks.iter()).enumerate()
            {
                planned_track_increase[idx] =
                    planned_track_increase[idx].max(item_track - base_track);
            }

            for (idx, (item_limit, base_limit)) in item_growth_limits
                .iter()
                .zip(base_growth_limits.iter())
                .enumerate()
            {
                let item_limit = item_limit.unwrap_or(base_tracks[idx]);
                let base_limit = base_limit.unwrap_or(base_tracks[idx]);
                planned_growth_limit_increase[idx] =
                    planned_growth_limit_increase[idx].max(item_limit - base_limit);
            }
        }

        for (track, increase) in tracks.iter_mut().zip(planned_track_increase) {
            if increase > self.epsilon {
                *track += increase;
            }
        }
        for (idx, increase) in planned_growth_limit_increase.into_iter().enumerate() {
            if increase <= self.epsilon {
                continue;
            }

            let base_limit = base_growth_limits[idx].unwrap_or(base_tracks[idx]);
            context.growth_limits[idx] = Some(base_limit + increase);
        }

        self.update_grid_single_span_growth_limits(
            tracks,
            context.track_sizing,
            context.growth_limits,
            context.contributions,
        );
    }

    fn grow_grid_intrinsic_tracks(
        &self,
        tracks: &mut [f32],
        context: GridIntrinsicGrowthContext<'_>,
    ) {
        let track_sizing = context.track_sizing;
        let growth_limits = context.growth_limits;
        let span = context.span;
        let gap = context.gap;
        let mut required_size = context.required_size;
        let minimum_size = context.minimum_size;
        let container_axis_is_definite = context.container_axis_is_definite;
        let container_axis_is_indefinite = context.container_axis_is_indefinite;
        let container_axis_is_min_content_constraint =
            context.container_axis_is_min_content_constraint;
        let container_axis_has_max_limit = context.container_axis_has_max_limit;

        let start = span.start;
        let end = span.end;
        debug_assert!(start < end);
        debug_assert!(end <= tracks.len());
        debug_assert!(end <= track_sizing.len());
        debug_assert_eq!(tracks.len(), growth_limits.len());
        if container_axis_is_definite
            && !(start..end).any(|idx| {
                Self::grid_track_has_intrinsic_minimum(track_sizing[idx].min_track)
                    || Self::grid_track_updates_growth_limit_from_intrinsic(track_sizing[idx])
            })
        {
            return;
        }

        if container_axis_is_indefinite {
            self.update_grid_intrinsic_growth_limits(
                tracks,
                track_sizing,
                growth_limits,
                start..end,
                gap,
                minimum_size,
                GridIntrinsicLimitPhase::MinContent,
                &[],
            );
            if container_axis_is_min_content_constraint {
                required_size = minimum_size;
            } else {
                required_size = self.indefinite_grid_intrinsic_base_required_size(
                    GridIntrinsicBaseRequiredSizeContext {
                        track_sizing,
                        growth_limits,
                        span: start..end,
                        gap,
                        required_size,
                        minimum_size,
                        container_axis_has_max_limit,
                    },
                );
            }
        } else if !container_axis_is_definite {
            let infinitely_growable = self.update_grid_intrinsic_growth_limits(
                tracks,
                track_sizing,
                growth_limits,
                start..end,
                gap,
                minimum_size,
                GridIntrinsicLimitPhase::MinContent,
                &[],
            );
            self.update_grid_intrinsic_growth_limits(
                tracks,
                track_sizing,
                growth_limits,
                start..end,
                gap,
                required_size,
                GridIntrinsicLimitPhase::MaxContent,
                &infinitely_growable,
            );
            if !(start..end).any(|idx| track_sizing[idx].min_track == Length::MaxContent) {
                required_size = minimum_size;
            }
        }

        let current_size = self.spanned_track_size(tracks, start, end - start, gap);
        if container_axis_is_definite
            && Self::grid_span_crosses_flexible_track(track_sizing, start, end - start)
        {
            if minimum_size > current_size + self.epsilon {
                let flexible_tracks = (start..end)
                    .filter(|idx| matches!(track_sizing[*idx].kind, GridTrackKind::Flexible(_)))
                    .collect::<Vec<_>>();
                self.distribute_grid_flexible_intrinsic_growth(
                    tracks,
                    track_sizing,
                    flexible_tracks,
                    minimum_size - current_size,
                );
            }
            return;
        }

        if required_size <= current_size + self.epsilon {
            return;
        }
        let extra = required_size - current_size;

        if container_axis_is_definite {
            let infinitely_growable = self.update_grid_intrinsic_growth_limits(
                tracks,
                track_sizing,
                growth_limits,
                start..end,
                gap,
                minimum_size,
                GridIntrinsicLimitPhase::MinContent,
                &[],
            );
            self.update_grid_intrinsic_growth_limits(
                tracks,
                track_sizing,
                growth_limits,
                start..end,
                gap,
                required_size,
                GridIntrinsicLimitPhase::MaxContent,
                &infinitely_growable,
            );
            let current_size = self.spanned_track_size(tracks, start, end - start, gap);
            if minimum_size > current_size + self.epsilon {
                let intrinsic_minimum_tracks = (start..end)
                    .filter(|idx| {
                        Self::grid_track_has_intrinsic_minimum(track_sizing[*idx].min_track)
                    })
                    .collect::<Vec<_>>();
                if !intrinsic_minimum_tracks.is_empty() {
                    self.distribute_grid_intrinsic_growth(
                        tracks,
                        track_sizing,
                        growth_limits,
                        intrinsic_minimum_tracks,
                        start..end,
                        minimum_size - current_size,
                        true,
                        GridBeyondLimitDistribution::IntrinsicMaxOrAllAffected,
                    );
                }
            }
            let max_content_minimum_tracks = (start..end)
                .filter(|idx| track_sizing[*idx].min_track == Length::MaxContent)
                .collect::<Vec<_>>();
            if !max_content_minimum_tracks.is_empty() {
                let current_size = self.spanned_track_size(tracks, start, end - start, gap);
                self.distribute_grid_intrinsic_growth(
                    tracks,
                    track_sizing,
                    growth_limits,
                    max_content_minimum_tracks,
                    start..end,
                    (required_size - current_size).max(0.0),
                    true,
                    GridBeyondLimitDistribution::MaxContentMaxOrAllAffected,
                );
            }
            self.ensure_grid_growth_limits_cover_tracks(growth_limits, tracks, start..end);
            return;
        }

        let mut flexible_tracks = Vec::new();
        for idx in start..end.min(track_sizing.len()) {
            if matches!(track_sizing[idx].kind, GridTrackKind::Flexible(_)) {
                flexible_tracks.push(idx);
            }
        }
        if !flexible_tracks.is_empty() {
            self.distribute_grid_flexible_intrinsic_growth(
                tracks,
                track_sizing,
                flexible_tracks,
                extra,
            );
            return;
        }

        let growable_tracks = (start..end)
            .filter(|idx| {
                track_sizing
                    .get(*idx)
                    .is_some_and(|sizing| Self::grid_track_has_intrinsic_minimum(sizing.min_track))
            })
            .collect::<Vec<_>>();
        if growable_tracks.is_empty() {
            return;
        }

        self.distribute_grid_intrinsic_growth(
            tracks,
            track_sizing,
            growth_limits,
            growable_tracks,
            start..end,
            extra,
            !container_axis_is_indefinite,
            GridBeyondLimitDistribution::IntrinsicMaxOrAllAffected,
        );
    }

    fn indefinite_grid_intrinsic_base_required_size(
        &self,
        context: GridIntrinsicBaseRequiredSizeContext<'_>,
    ) -> f32 {
        let track_sizing = context.track_sizing;
        let growth_limits = context.growth_limits;
        let span = context.span;
        let gap = context.gap;
        let required_size = context.required_size;
        let minimum_size = context.minimum_size;
        let container_axis_has_max_limit = context.container_axis_has_max_limit;
        let start = span.start;
        let end = span.end.min(track_sizing.len());
        let has_max_content_minimum =
            (start..end).any(|idx| track_sizing[idx].min_track == Length::MaxContent);
        let crosses_flexible_track =
            (start..end).any(|idx| matches!(track_sizing[idx].kind, GridTrackKind::Flexible(_)));
        let single_fit_content_maximum =
            end == start + 1 && matches!(track_sizing[start].kind, GridTrackKind::FitContent(_));
        let single_span = end == start + 1;
        let all_tracks_have_finite_growth_limits =
            (start..end).all(|idx| growth_limits[idx].is_some());
        let base_required_size = if has_max_content_minimum
            || crosses_flexible_track
            || single_fit_content_maximum
            || ((!single_span || !container_axis_has_max_limit)
                && !all_tracks_have_finite_growth_limits)
        {
            required_size
        } else {
            minimum_size
        };
        if has_max_content_minimum {
            return base_required_size.max(minimum_size);
        }
        self.limited_grid_intrinsic_required_size(
            track_sizing,
            growth_limits,
            start..end,
            gap,
            base_required_size,
            minimum_size,
        )
    }

    fn update_grid_intrinsic_growth_limits(
        &self,
        tracks: &[f32],
        track_sizing: &[GridTrackSizing],
        growth_limits: &mut [Option<f32>],
        span: std::ops::Range<usize>,
        gap: f32,
        contribution_size: f32,
        phase: GridIntrinsicLimitPhase,
        infinitely_growable_tracks: &[usize],
    ) -> Vec<usize> {
        let start = span.start;
        let end = span.end;
        debug_assert!(start < end);
        debug_assert!(end <= tracks.len());
        debug_assert!(end <= track_sizing.len());
        debug_assert_eq!(tracks.len(), growth_limits.len());

        let current_limit_size = (start..end)
            .map(|idx| growth_limits[idx].unwrap_or(tracks[idx]))
            .sum::<f32>()
            + self.gap_total(gap, end - start);
        if contribution_size <= current_limit_size + self.epsilon {
            return Vec::new();
        }

        let affected_tracks = (start..end)
            .filter(|idx| {
                Self::grid_track_updates_growth_limit_for_phase(track_sizing[*idx].kind, phase)
            })
            .collect::<Vec<_>>();
        if affected_tracks.is_empty() {
            return Vec::new();
        }

        let infinite_before = growth_limits
            .iter()
            .map(Option::is_none)
            .collect::<Vec<_>>();
        let extra = contribution_size - current_limit_size;
        let growable_tracks = affected_tracks
            .iter()
            .copied()
            .filter(|idx| growth_limits[*idx].is_none() || infinitely_growable_tracks.contains(idx))
            .collect::<Vec<_>>();
        let extra = self.distribute_grid_growth_limits_to_tracks(
            tracks,
            track_sizing,
            growth_limits,
            growable_tracks,
            extra,
        );
        let extra = if extra > self.epsilon {
            let non_affected_tracks = (start..end)
                .filter(|idx| !affected_tracks.contains(idx) && growth_limits[*idx].is_none())
                .collect::<Vec<_>>();
            self.distribute_grid_growth_limits_to_tracks(
                tracks,
                track_sizing,
                growth_limits,
                non_affected_tracks,
                extra,
            )
        } else {
            extra
        };
        if extra > self.epsilon {
            self.distribute_grid_growth_limits_beyond_limits(
                tracks,
                track_sizing,
                growth_limits,
                affected_tracks.clone(),
                extra,
            );
        }

        affected_tracks
            .into_iter()
            .filter(|idx| infinite_before.get(*idx).copied().unwrap_or(false))
            .filter(|idx| growth_limits.get(*idx).is_some_and(Option::is_some))
            .collect()
    }

    fn distribute_grid_growth_limits_to_tracks(
        &self,
        tracks: &[f32],
        track_sizing: &[GridTrackSizing],
        growth_limits: &mut [Option<f32>],
        mut growable_tracks: Vec<usize>,
        mut extra: f32,
    ) -> f32 {
        debug_assert_eq!(tracks.len(), track_sizing.len());
        debug_assert_eq!(tracks.len(), growth_limits.len());
        while extra > self.epsilon && !growable_tracks.is_empty() {
            let share = extra / growable_tracks.len() as f32;
            let mut consumed = 0.0;
            let mut still_growable = Vec::new();
            for idx in growable_tracks {
                let current_limit = growth_limits[idx].unwrap_or(tracks[idx]);
                let cap = self.grid_track_growth_limit(track_sizing[idx].kind);
                let available = cap
                    .map(|cap| (cap - current_limit).max(0.0))
                    .unwrap_or(f32::INFINITY);
                if available <= self.epsilon {
                    continue;
                }

                let increase = share.min(available);
                growth_limits[idx] = Some(current_limit + increase);
                consumed += increase;
                if available > share + self.epsilon {
                    still_growable.push(idx);
                }
            }

            if consumed <= self.epsilon {
                break;
            }
            extra -= consumed;
            growable_tracks = still_growable;
        }
        extra
    }

    fn distribute_grid_growth_limits_beyond_limits(
        &self,
        tracks: &[f32],
        track_sizing: &[GridTrackSizing],
        growth_limits: &mut [Option<f32>],
        affected_tracks: Vec<usize>,
        mut extra: f32,
    ) {
        debug_assert_eq!(tracks.len(), track_sizing.len());
        debug_assert_eq!(tracks.len(), growth_limits.len());
        let mut growable_tracks = affected_tracks
            .into_iter()
            .filter(|idx| {
                self.grid_track_has_intrinsic_maximum_for_beyond_limit(
                    track_sizing[*idx],
                    growth_limits[*idx].unwrap_or(tracks[*idx]),
                )
            })
            .collect::<Vec<_>>();
        while extra > self.epsilon && !growable_tracks.is_empty() {
            let share = extra / growable_tracks.len() as f32;
            let mut consumed = 0.0;
            let mut still_growable = Vec::new();
            for idx in growable_tracks {
                let current_limit = growth_limits[idx].unwrap_or(tracks[idx]);
                let available = match track_sizing[idx].kind {
                    GridTrackKind::FitContent(Some(limit)) => (limit - current_limit).max(0.0),
                    GridTrackKind::Fixed
                    | GridTrackKind::Auto
                    | GridTrackKind::MinContent
                    | GridTrackKind::MaxContent
                    | GridTrackKind::Flexible(_)
                    | GridTrackKind::FitContent(None) => f32::INFINITY,
                };

                let increase = share.min(available);
                growth_limits[idx] = Some(current_limit + increase);
                consumed += increase;
                if available > share + self.epsilon {
                    still_growable.push(idx);
                }
            }

            extra -= consumed;
            growable_tracks = still_growable;
        }
    }

    fn update_grid_single_span_growth_limits(
        &self,
        tracks: &[f32],
        track_sizing: &[GridTrackSizing],
        growth_limits: &mut [Option<f32>],
        contributions: &[GridIntrinsicContribution],
    ) {
        debug_assert_eq!(tracks.len(), track_sizing.len());
        debug_assert_eq!(tracks.len(), growth_limits.len());
        for contribution in contributions {
            if contribution.span != 1 || contribution.start >= growth_limits.len() {
                continue;
            }

            let idx = contribution.start;
            let track = track_sizing[idx];
            if !Self::grid_track_updates_growth_limit_from_intrinsic(track) {
                continue;
            }

            let mut limit = if track.kind == GridTrackKind::MinContent {
                contribution.minimum_size
            } else {
                contribution.required_size
            };
            if let Some(track_limit) = self.grid_track_growth_limit(track.kind) {
                limit = limit.min(track_limit);
            }
            limit = limit.max(tracks[idx]);
            growth_limits[idx] =
                Some(growth_limits[idx].map_or(limit, |current| current.max(limit)));
        }
    }

    const fn grid_track_has_intrinsic_minimum(track: Length) -> bool {
        matches!(
            track,
            Length::Auto
                | Length::Fr(_)
                | Length::MinContent
                | Length::MaxContent
                | Length::FitContent(_)
        )
    }

    const fn grid_track_updates_growth_limit_from_intrinsic(sizing: GridTrackSizing) -> bool {
        matches!(
            sizing.kind,
            GridTrackKind::Auto
                | GridTrackKind::MinContent
                | GridTrackKind::MaxContent
                | GridTrackKind::FitContent(_)
        )
    }

    const fn grid_track_updates_growth_limit_for_phase(
        kind: GridTrackKind,
        phase: GridIntrinsicLimitPhase,
    ) -> bool {
        match phase {
            GridIntrinsicLimitPhase::MinContent => matches!(
                kind,
                GridTrackKind::Auto
                    | GridTrackKind::MinContent
                    | GridTrackKind::MaxContent
                    | GridTrackKind::FitContent(_)
            ),
            GridIntrinsicLimitPhase::MaxContent => matches!(
                kind,
                GridTrackKind::Auto | GridTrackKind::MaxContent | GridTrackKind::FitContent(_)
            ),
        }
    }

    fn grid_track_has_intrinsic_maximum_for_beyond_limit(
        &self,
        sizing: GridTrackSizing,
        track_size: f32,
    ) -> bool {
        match sizing.kind {
            GridTrackKind::Auto | GridTrackKind::MinContent | GridTrackKind::MaxContent => true,
            GridTrackKind::FitContent(None) => true,
            GridTrackKind::FitContent(Some(limit)) => track_size < limit - self.epsilon,
            GridTrackKind::Fixed | GridTrackKind::Flexible(_) => false,
        }
    }

    fn grid_track_has_max_content_maximum_for_beyond_limit(
        &self,
        sizing: GridTrackSizing,
        track_size: f32,
    ) -> bool {
        match sizing.kind {
            GridTrackKind::Auto | GridTrackKind::MaxContent => true,
            GridTrackKind::FitContent(None) => true,
            GridTrackKind::FitContent(Some(limit)) => track_size < limit - self.epsilon,
            GridTrackKind::Fixed | GridTrackKind::MinContent | GridTrackKind::Flexible(_) => false,
        }
    }

    fn limited_grid_intrinsic_required_size(
        &self,
        track_sizing: &[GridTrackSizing],
        growth_limits: &[Option<f32>],
        span: std::ops::Range<usize>,
        gap: f32,
        required_size: f32,
        minimum_size: f32,
    ) -> f32 {
        let start = span.start;
        let end = span.end.min(track_sizing.len()).min(growth_limits.len());
        debug_assert!(start < end);

        let mut upper_limit = self.gap_total(gap, end - start);
        for idx in start..end {
            let Some(limit) =
                self.grid_track_effective_growth_limit(track_sizing[idx], growth_limits[idx])
            else {
                return required_size;
            };
            upper_limit += limit;
        }

        required_size.min(upper_limit).max(minimum_size)
    }

    fn grid_track_effective_growth_limit(
        &self,
        sizing: GridTrackSizing,
        growth_limit: Option<f32>,
    ) -> Option<f32> {
        growth_limit.or_else(|| self.grid_track_growth_limit(sizing.kind))
    }

    fn ensure_grid_growth_limits_cover_tracks(
        &self,
        growth_limits: &mut [Option<f32>],
        tracks: &[f32],
        span: std::ops::Range<usize>,
    ) {
        let end = span.end.min(tracks.len()).min(growth_limits.len());
        let start = span.start.min(end);
        for idx in start..end {
            if growth_limits[idx].is_some_and(|limit| tracks[idx] > limit + self.epsilon) {
                growth_limits[idx] = Some(tracks[idx]);
            }
        }
    }

    fn distribute_grid_intrinsic_growth(
        &self,
        tracks: &mut [f32],
        track_sizing: &[GridTrackSizing],
        growth_limits: &[Option<f32>],
        growable_tracks: Vec<usize>,
        span: std::ops::Range<usize>,
        extra: f32,
        cap_unresolved_fit_content_tracks: bool,
        beyond_limit_distribution: GridBeyondLimitDistribution,
    ) {
        let extra = self.distribute_grid_intrinsic_growth_to_tracks(
            tracks,
            track_sizing,
            growth_limits,
            growable_tracks.clone(),
            extra,
            cap_unresolved_fit_content_tracks,
        );
        if extra <= self.epsilon {
            return;
        }

        let span_end = span.end.min(tracks.len()).min(track_sizing.len());
        let non_affected_tracks = (span.start.min(span_end)..span_end)
            .filter(|idx| !growable_tracks.contains(idx))
            .collect::<Vec<_>>();
        let extra = self.distribute_grid_intrinsic_growth_to_tracks(
            tracks,
            track_sizing,
            growth_limits,
            non_affected_tracks,
            extra,
            cap_unresolved_fit_content_tracks,
        );
        if extra <= self.epsilon {
            return;
        }

        self.distribute_grid_intrinsic_growth_beyond_limits(
            tracks,
            track_sizing,
            growable_tracks,
            extra,
            beyond_limit_distribution,
        );
    }

    fn distribute_grid_intrinsic_growth_beyond_limits(
        &self,
        tracks: &mut [f32],
        track_sizing: &[GridTrackSizing],
        affected_tracks: Vec<usize>,
        extra: f32,
        distribution: GridBeyondLimitDistribution,
    ) {
        debug_assert!(extra > self.epsilon);
        debug_assert!(!affected_tracks.is_empty());
        debug_assert_eq!(tracks.len(), track_sizing.len());
        debug_assert!(affected_tracks.iter().all(|idx| *idx < tracks.len()));

        let mut tracks_to_grow = affected_tracks
            .iter()
            .copied()
            .filter(|idx| {
                let sizing = track_sizing[*idx];
                match distribution {
                    GridBeyondLimitDistribution::IntrinsicMaxOrAllAffected => {
                        self.grid_track_has_intrinsic_maximum_for_beyond_limit(sizing, tracks[*idx])
                    }
                    GridBeyondLimitDistribution::MaxContentMaxOrAllAffected => self
                        .grid_track_has_max_content_maximum_for_beyond_limit(sizing, tracks[*idx]),
                }
            })
            .collect::<Vec<_>>();
        if tracks_to_grow.is_empty() {
            tracks_to_grow = affected_tracks;
        }

        let increase = extra / tracks_to_grow.len() as f32;
        for idx in tracks_to_grow {
            tracks[idx] += increase;
        }
    }

    fn distribute_grid_intrinsic_growth_to_tracks(
        &self,
        tracks: &mut [f32],
        track_sizing: &[GridTrackSizing],
        growth_limits: &[Option<f32>],
        mut growable_tracks: Vec<usize>,
        mut extra: f32,
        cap_unresolved_fit_content_tracks: bool,
    ) -> f32 {
        while extra > self.epsilon && !growable_tracks.is_empty() {
            let share = extra / growable_tracks.len() as f32;
            let mut consumed = 0.0;
            let mut still_growable = Vec::new();
            for idx in growable_tracks {
                let current_growth_limit = growth_limits.get(idx).copied().flatten();
                let fit_content_limit_is_unresolved = !cap_unresolved_fit_content_tracks
                    && current_growth_limit.is_none()
                    && matches!(track_sizing[idx].kind, GridTrackKind::FitContent(_));
                let available = if fit_content_limit_is_unresolved {
                    f32::INFINITY
                } else {
                    self.grid_track_effective_growth_limit(track_sizing[idx], current_growth_limit)
                        .map(|limit| (limit - tracks[idx]).max(0.0))
                        .unwrap_or(f32::INFINITY)
                };
                if available <= self.epsilon {
                    continue;
                }

                let increase = share.min(available);
                tracks[idx] += increase;
                consumed += increase;
                if available > share + self.epsilon {
                    still_growable.push(idx);
                }
            }

            if consumed <= self.epsilon {
                break;
            }
            extra -= consumed;
            growable_tracks = still_growable;
        }
        extra
    }

    fn distribute_grid_flexible_intrinsic_growth(
        &self,
        tracks: &mut [f32],
        track_sizing: &[GridTrackSizing],
        flexible_tracks: Vec<usize>,
        extra: f32,
    ) {
        debug_assert!(extra > self.epsilon);
        debug_assert!(!flexible_tracks.is_empty());

        let flex_sum = flexible_tracks
            .iter()
            .map(|idx| match track_sizing[*idx].kind {
                GridTrackKind::Flexible(flex) if flex > self.epsilon => flex,
                GridTrackKind::Fixed
                | GridTrackKind::Auto
                | GridTrackKind::MinContent
                | GridTrackKind::MaxContent
                | GridTrackKind::Flexible(_)
                | GridTrackKind::FitContent(_) => 0.0,
            })
            .sum::<f32>();
        let requested_fraction = flex_sum.min(1.0);
        let equal_remainder = extra * (1.0 - requested_fraction) / flexible_tracks.len() as f32;
        for idx in flexible_tracks {
            let weighted_share = match track_sizing[idx].kind {
                GridTrackKind::Flexible(flex) if flex_sum > self.epsilon => {
                    extra * requested_fraction * flex / flex_sum
                }
                GridTrackKind::Fixed
                | GridTrackKind::Auto
                | GridTrackKind::MinContent
                | GridTrackKind::MaxContent
                | GridTrackKind::Flexible(_)
                | GridTrackKind::FitContent(_) => 0.0,
            };
            tracks[idx] += weighted_share + equal_remainder;
        }
    }

    fn grid_track_growth_limit(&self, kind: GridTrackKind) -> Option<f32> {
        match kind {
            GridTrackKind::FitContent(Some(limit)) => Some(limit),
            GridTrackKind::Fixed
            | GridTrackKind::Auto
            | GridTrackKind::MinContent
            | GridTrackKind::MaxContent
            | GridTrackKind::Flexible(_)
            | GridTrackKind::FitContent(None) => None,
        }
    }

    fn spanned_track_size(&self, tracks: &[f32], start: usize, span: usize, gap: f32) -> f32 {
        let end = (start + span).min(tracks.len());
        debug_assert!(start < end);
        tracks[start..end].iter().sum::<f32>() + self.gap_total(gap, end - start)
    }

    fn track_total_size(&self, tracks: &[f32], gap: f32) -> f32 {
        tracks.iter().sum::<f32>() + self.gap_total(gap, tracks.len())
    }

    fn grid_line_offsets(&self, tracks: &[f32], start: f32, gap: f32) -> Vec<f32> {
        let mut offsets = Vec::with_capacity(tracks.len() + 1);
        let mut cursor = start;
        offsets.push(cursor);
        for (idx, track) in tracks.iter().enumerate() {
            cursor += *track;
            if idx + 1 < tracks.len() {
                cursor += gap;
            }
            offsets.push(cursor);
        }
        offsets
    }

    fn layout_grid_out_of_flow_children<T: LayoutTree>(
        &mut self,
        tree: &mut T,
        children: &[T::NodeId],
        context: GridOutOfFlowContext<'_>,
        rounding: RoundingContext,
    ) {
        for &child in children {
            let child_style = tree.style(child).clone();
            if child_style.display == Display::None
                || !matches!(
                    child_style.position,
                    PositionType::Absolute | PositionType::Fixed
                )
            {
                continue;
            }

            let area = self.grid_absolute_area(&child_style, context);
            let child_edges = self
                .resolve_edges_for_parent(&child_style, SideConstraint::definite(area.size.width));
            let containing_block = Constraints::definite(area.size.width, area.size.height);
            let child_constraints = self.out_of_flow_constraints(
                &child_style,
                child_edges,
                containing_block,
                area.size.width,
                area.size.height,
            );
            let child_style_override = self.out_of_flow_style_override(
                &child_style,
                child_constraints,
                child_edges,
                containing_block,
            );
            let measured = self.layout_node_with_edges(
                tree,
                child,
                child_style_override.clone(),
                NodeLayoutContext {
                    constraints: child_constraints,
                    offset: Point::ZERO,
                    sticky_constraints: child_constraints,
                    edges: child_edges,
                    rounding,
                    flex: FlexNodeContext::default(),
                },
            );
            let offset = self.grid_out_of_flow_offset(
                context.container_style,
                &child_style,
                child_edges,
                measured.size,
                area,
                containing_block,
            );
            self.layout_node_with_edges(
                tree,
                child,
                child_style_override,
                NodeLayoutContext {
                    constraints: child_constraints,
                    offset,
                    sticky_constraints: child_constraints,
                    edges: child_edges,
                    rounding,
                    flex: FlexNodeContext::default(),
                },
            );
        }
    }

    fn grid_absolute_area(
        &self,
        child_style: &Style,
        context: GridOutOfFlowContext<'_>,
    ) -> GridAbsoluteArea {
        let columns = self.grid_absolute_axis_area(
            child_style.grid_column_start,
            child_style.grid_column_end,
            GridAbsoluteAxisContext {
                explicit_track_count: context.container_style.grid_template_columns.len(),
                axis_offset: context.grid_offsets.column,
                line_offsets: context.column_offsets,
                gap: context.column_gap,
                scrollable_content_size: context.scrollable_content_size.width,
                start_padding: if context.container_style.direction.is_any_rtl() {
                    context.edges.padding.right
                } else {
                    context.edges.padding.left
                },
                end_padding: if context.container_style.direction.is_any_rtl() {
                    context.edges.padding.left
                } else {
                    context.edges.padding.right
                },
            },
        );
        let rows = self.grid_absolute_axis_area(
            child_style.grid_row_start,
            child_style.grid_row_end,
            GridAbsoluteAxisContext {
                explicit_track_count: context.container_style.grid_template_rows.len(),
                axis_offset: context.grid_offsets.row,
                line_offsets: context.row_offsets,
                gap: context.row_gap,
                scrollable_content_size: context.scrollable_content_size.height,
                start_padding: context.edges.padding.top,
                end_padding: context.edges.padding.bottom,
            },
        );
        GridAbsoluteArea {
            origin: Point::new(
                context.edges.border.left + context.edges.padding.left + columns.start,
                context.edges.border.top + context.edges.padding.top + rows.start,
            ),
            size: Size::new(columns.size, rows.size),
            inline_start: columns.start,
            content_origin: Point::new(
                context.edges.border.left + context.edges.padding.left,
                context.edges.border.top + context.edges.padding.top,
            ),
            content_size: context.content_size,
            inline_static_uses_content_edges: child_style.grid_column_start.is_none()
                && child_style.grid_column_end.is_none(),
            block_static_uses_content_edges: child_style.grid_row_start.is_none()
                && child_style.grid_row_end.is_none(),
        }
    }

    fn grid_absolute_axis_area(
        &self,
        raw_start: Option<i32>,
        raw_end: Option<i32>,
        context: GridAbsoluteAxisContext<'_>,
    ) -> GridAxisArea {
        debug_assert!(context.line_offsets.len() >= 2);

        let last_real_line = context.line_offsets.len() - 1;
        let start_line = raw_start
            .and_then(|line| {
                self.resolve_grid_line_offset_index(
                    line,
                    context.explicit_track_count,
                    context.axis_offset,
                )
            })
            .filter(|line| *line <= last_real_line);
        let end_line = raw_end
            .and_then(|line| {
                self.resolve_grid_line_offset_index(
                    line,
                    context.explicit_track_count,
                    context.axis_offset,
                )
            })
            .filter(|line| *line <= last_real_line);

        let start = start_line
            .and_then(|line| context.line_offsets.get(line).copied())
            .unwrap_or(-context.start_padding);
        let end = end_line
            .and_then(|line| context.line_offsets.get(line).copied())
            .unwrap_or(context.scrollable_content_size + context.end_padding);

        if start >= end {
            return GridAxisArea { start, size: 0.0 };
        }

        let trailing_internal_gutter =
            if end_line.is_some_and(|line| line > 0 && line < last_real_line) {
                context.gap
            } else {
                0.0
            };
        GridAxisArea {
            start,
            size: (end - start - trailing_internal_gutter).max(0.0),
        }
    }

    fn resolve_grid_line_offset_index(
        &self,
        line: i32,
        explicit_track_count: usize,
        axis_offset: usize,
    ) -> Option<usize> {
        let explicit_end = explicit_track_count as i32 + 1;
        let line = normalize_grid_line(line, explicit_end) + axis_offset as i32;
        (line >= 1).then_some(line as usize - 1)
    }

    fn grid_out_of_flow_offset(
        &self,
        container_style: &Style,
        child_style: &Style,
        child_edges: ResolvedEdges,
        child_size: Size,
        area: GridAbsoluteArea,
        containing_block: Constraints,
    ) -> Point {
        let left = child_style
            .left
            .resolve(containing_block.width.percent_base());
        let right = child_style
            .right
            .resolve(containing_block.width.percent_base());
        let top = child_style
            .top
            .resolve(containing_block.height.percent_base());
        let bottom = child_style
            .bottom
            .resolve(containing_block.height.percent_base());

        let area_origin_x = if container_style.direction.is_any_rtl() {
            let content_left = area.origin.x - area.inline_start;
            content_left + area.content_size.width - area.inline_start - area.size.width
        } else {
            area.origin.x
        };
        let static_origin_x = if area.inline_static_uses_content_edges {
            area.content_origin.x
        } else {
            area_origin_x
        };
        let static_width = if area.inline_static_uses_content_edges {
            area.content_size.width
        } else {
            area.size.width
        };
        let static_origin_y = if area.block_static_uses_content_edges {
            area.content_origin.y
        } else {
            area.origin.y
        };
        let static_height = if area.block_static_uses_content_edges {
            area.content_size.height
        } else {
            area.size.height
        };
        let x = if let Some(left) = left {
            area_origin_x + left + child_edges.margin.left
        } else if let Some(right) = right {
            area_origin_x + area.size.width - right - child_edges.margin.right - child_size.width
        } else if container_style.direction.is_any_rtl() {
            static_origin_x + static_width
                - self.grid_inline_item_end_offset(
                    container_style,
                    child_style,
                    static_width,
                    child_size.width,
                    child_edges.margin,
                )
                - child_size.width
        } else {
            static_origin_x
                + self.grid_inline_item_offset(
                    container_style,
                    child_style,
                    static_width,
                    child_size.width,
                    child_edges.margin,
                )
        };
        let y = if let Some(top) = top {
            area.origin.y + top + child_edges.margin.top
        } else if let Some(bottom) = bottom {
            area.origin.y + area.size.height
                - bottom
                - child_edges.margin.bottom
                - child_size.height
        } else {
            static_origin_y
                + self.grid_block_item_offset(
                    container_style,
                    child_style,
                    static_height,
                    child_size.height,
                    child_edges.margin,
                )
        };
        Point::new(x, y)
    }

    fn grid_content_axis_limit(
        &self,
        fixed_border_size: Option<f32>,
        constraint: SideConstraint,
        horizontal: bool,
        edges: ResolvedEdges,
    ) -> Option<f32> {
        fixed_border_size
            .or_else(|| constraint.is_definite().then_some(constraint.size))
            .map(|size| self.inner_axis(size, horizontal, edges))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn assert_close(actual: f32, expected: f32) {
        assert!((actual - expected).abs() < 0.01);
    }

    #[test]
    fn grid_intrinsic_distribution_uses_non_affected_tracks_before_exceeding_limits() {
        let engine = LayoutEngine::new();
        let mut tracks = vec![0.0, 0.0, 0.0];
        let track_sizing = vec![
            GridTrackSizing {
                min_track: Length::Auto,
                max_track: Length::points(20.0),
                kind: GridTrackKind::Fixed,
            },
            GridTrackSizing {
                min_track: Length::points(0.0),
                max_track: Length::points(50.0),
                kind: GridTrackKind::Fixed,
            },
            GridTrackSizing {
                min_track: Length::Auto,
                max_track: Length::points(20.0),
                kind: GridTrackKind::Fixed,
            },
        ];
        let growth_limits = vec![Some(20.0), Some(50.0), Some(20.0)];

        engine.distribute_grid_intrinsic_growth(
            &mut tracks,
            &track_sizing,
            &growth_limits,
            vec![0, 2],
            0..3,
            70.0,
            true,
            GridBeyondLimitDistribution::IntrinsicMaxOrAllAffected,
        );

        assert_close(tracks[0], 20.0);
        assert_close(tracks[1], 30.0);
        assert_close(tracks[2], 20.0);
    }

    #[test]
    fn grid_intrinsic_distribution_continues_beyond_limits_when_needed() {
        let engine = LayoutEngine::new();
        let mut tracks = vec![0.0, 0.0];
        let track_sizing = vec![
            GridTrackSizing {
                min_track: Length::Auto,
                max_track: Length::points(20.0),
                kind: GridTrackKind::Fixed,
            },
            GridTrackSizing {
                min_track: Length::Auto,
                max_track: Length::points(20.0),
                kind: GridTrackKind::Fixed,
            },
        ];
        let growth_limits = vec![Some(20.0), Some(20.0)];

        engine.distribute_grid_intrinsic_growth(
            &mut tracks,
            &track_sizing,
            &growth_limits,
            vec![0, 1],
            0..2,
            70.0,
            true,
            GridBeyondLimitDistribution::IntrinsicMaxOrAllAffected,
        );

        assert_close(tracks[0], 35.0);
        assert_close(tracks[1], 35.0);
    }

    #[test]
    fn grid_intrinsic_distribution_prioritizes_max_content_maximum_beyond_limits() {
        let engine = LayoutEngine::new();
        let mut tracks = vec![20.0, 20.0, 20.0];
        let track_sizing = vec![
            GridTrackSizing {
                min_track: Length::MaxContent,
                max_track: Length::Auto,
                kind: GridTrackKind::Auto,
            },
            GridTrackSizing {
                min_track: Length::MaxContent,
                max_track: Length::MaxContent,
                kind: GridTrackKind::MaxContent,
            },
            GridTrackSizing {
                min_track: Length::MaxContent,
                max_track: Length::points(20.0),
                kind: GridTrackKind::Fixed,
            },
        ];
        let growth_limits = vec![Some(20.0), Some(20.0), Some(20.0)];

        engine.distribute_grid_intrinsic_growth(
            &mut tracks,
            &track_sizing,
            &growth_limits,
            vec![0, 1, 2],
            0..3,
            30.0,
            true,
            GridBeyondLimitDistribution::MaxContentMaxOrAllAffected,
        );

        assert_close(tracks[0], 35.0);
        assert_close(tracks[1], 35.0);
        assert_close(tracks[2], 20.0);
    }

    #[test]
    fn grid_intrinsic_distribution_keeps_unresolved_fit_content_intrinsic_beyond_limits() {
        let engine = LayoutEngine::new();
        let mut tracks = vec![20.0, 20.0];
        let track_sizing = vec![
            GridTrackSizing {
                min_track: Length::Auto,
                max_track: Length::fit_content(Some(crate::BaseLength::fixed_and_percent(
                    0.0, 50.0,
                ))),
                kind: GridTrackKind::FitContent(None),
            },
            GridTrackSizing {
                min_track: Length::Auto,
                max_track: Length::points(20.0),
                kind: GridTrackKind::Fixed,
            },
        ];
        let growth_limits = vec![Some(20.0), Some(20.0)];

        engine.distribute_grid_intrinsic_growth(
            &mut tracks,
            &track_sizing,
            &growth_limits,
            vec![0, 1],
            0..2,
            30.0,
            true,
            GridBeyondLimitDistribution::IntrinsicMaxOrAllAffected,
        );

        assert_close(tracks[0], 50.0);
        assert_close(tracks[1], 20.0);
    }

    #[test]
    fn grid_intrinsic_distribution_treats_fit_content_as_max_content_until_argument() {
        let engine = LayoutEngine::new();
        let mut tracks = vec![20.0, 20.0, 20.0];
        let track_sizing = vec![
            GridTrackSizing {
                min_track: Length::MaxContent,
                max_track: Length::fit_content(Some(crate::BaseLength::fixed_and_percent(
                    0.0, 50.0,
                ))),
                kind: GridTrackKind::FitContent(None),
            },
            GridTrackSizing {
                min_track: Length::MaxContent,
                max_track: Length::fit_content(Some(crate::BaseLength::fixed(60.0))),
                kind: GridTrackKind::FitContent(Some(60.0)),
            },
            GridTrackSizing {
                min_track: Length::MaxContent,
                max_track: Length::points(20.0),
                kind: GridTrackKind::Fixed,
            },
        ];
        let growth_limits = vec![Some(20.0), Some(20.0), Some(20.0)];

        engine.distribute_grid_intrinsic_growth(
            &mut tracks,
            &track_sizing,
            &growth_limits,
            vec![0, 1, 2],
            0..3,
            30.0,
            true,
            GridBeyondLimitDistribution::MaxContentMaxOrAllAffected,
        );

        assert_close(tracks[0], 35.0);
        assert_close(tracks[1], 35.0);
        assert_close(tracks[2], 20.0);
    }

    #[test]
    fn grid_intrinsic_distribution_treats_fit_content_at_limit_as_fixed_beyond_limits() {
        let engine = LayoutEngine::new();
        let mut tracks = vec![40.0, 20.0];
        let track_sizing = vec![
            GridTrackSizing {
                min_track: Length::Auto,
                max_track: Length::fit_content(Some(crate::BaseLength::fixed(40.0))),
                kind: GridTrackKind::FitContent(Some(40.0)),
            },
            GridTrackSizing {
                min_track: Length::Auto,
                max_track: Length::Auto,
                kind: GridTrackKind::Auto,
            },
        ];
        let growth_limits = vec![Some(40.0), Some(20.0)];

        engine.distribute_grid_intrinsic_growth(
            &mut tracks,
            &track_sizing,
            &growth_limits,
            vec![0, 1],
            0..2,
            20.0,
            true,
            GridBeyondLimitDistribution::IntrinsicMaxOrAllAffected,
        );

        assert_close(tracks[0], 40.0);
        assert_close(tracks[1], 40.0);
    }

    #[test]
    fn grid_intrinsic_distribution_caps_unresolved_fit_content_only_when_requested() {
        let engine = LayoutEngine::new();
        let track_sizing = vec![
            GridTrackSizing {
                min_track: Length::Auto,
                max_track: Length::fit_content(Some(crate::BaseLength::fixed(30.0))),
                kind: GridTrackKind::FitContent(Some(30.0)),
            },
            GridTrackSizing {
                min_track: Length::Auto,
                max_track: Length::Auto,
                kind: GridTrackKind::Auto,
            },
        ];
        let growth_limits = vec![None, Some(20.0)];

        let mut uncapped_tracks = vec![10.0, 10.0];
        let uncapped_extra = engine.distribute_grid_intrinsic_growth_to_tracks(
            &mut uncapped_tracks,
            &track_sizing,
            &growth_limits,
            vec![0, 1],
            40.0,
            false,
        );
        assert_close(uncapped_extra, 0.0);
        assert_close(uncapped_tracks[0], 40.0);
        assert_close(uncapped_tracks[1], 20.0);

        let mut capped_tracks = vec![10.0, 10.0];
        let capped_extra = engine.distribute_grid_intrinsic_growth_to_tracks(
            &mut capped_tracks,
            &track_sizing,
            &growth_limits,
            vec![0, 1],
            40.0,
            true,
        );
        assert_close(capped_extra, 10.0);
        assert_close(capped_tracks[0], 30.0);
        assert_close(capped_tracks[1], 20.0);
    }

    #[test]
    fn grid_intrinsic_growth_non_definite_non_flexible_tracks_use_intrinsic_distribution() {
        let engine = LayoutEngine::new();
        let auto_track = GridTrackSizing {
            min_track: Length::Auto,
            max_track: Length::Auto,
            kind: GridTrackKind::Auto,
        };
        let mut tracks = vec![10.0, 20.0];
        let mut growth_limits = vec![Some(100.0), Some(100.0)];

        engine.grow_grid_intrinsic_tracks(
            &mut tracks,
            GridIntrinsicGrowthContext {
                track_sizing: &[auto_track, auto_track],
                growth_limits: &mut growth_limits,
                span: 0..2,
                gap: 0.0,
                required_size: 90.0,
                minimum_size: 70.0,
                container_axis_is_definite: false,
                container_axis_is_indefinite: false,
                container_axis_is_min_content_constraint: false,
                container_axis_has_max_limit: false,
            },
        );

        assert_close(tracks[0], 30.0);
        assert_close(tracks[1], 40.0);
        assert_eq!(growth_limits, vec![Some(100.0), Some(100.0)]);
    }

    #[test]
    fn grid_intrinsic_growth_indefinite_non_flexible_tracks_continue_to_intrinsic_distribution() {
        let engine = LayoutEngine::new();
        let auto_track = GridTrackSizing {
            min_track: Length::Auto,
            max_track: Length::Auto,
            kind: GridTrackKind::Auto,
        };
        let mut tracks = vec![10.0, 20.0];
        let mut growth_limits = vec![Some(100.0), Some(100.0)];

        engine.grow_grid_intrinsic_tracks(
            &mut tracks,
            GridIntrinsicGrowthContext {
                track_sizing: &[auto_track, auto_track],
                growth_limits: &mut growth_limits,
                span: 0..2,
                gap: 0.0,
                required_size: 90.0,
                minimum_size: 70.0,
                container_axis_is_definite: false,
                container_axis_is_indefinite: true,
                container_axis_is_min_content_constraint: false,
                container_axis_has_max_limit: false,
            },
        );

        assert_close(tracks[0], 30.0);
        assert_close(tracks[1], 40.0);
        assert_eq!(growth_limits, vec![Some(100.0), Some(100.0)]);
    }

    #[test]
    fn grid_intrinsic_growth_definite_minimum_continues_past_growth_limits() {
        let engine = LayoutEngine::new();
        let fixed_max_auto_min_track = GridTrackSizing {
            min_track: Length::Auto,
            max_track: Length::points(25.0),
            kind: GridTrackKind::Fixed,
        };
        let mut tracks = vec![20.0, 20.0];
        let mut growth_limits = vec![Some(25.0), Some(25.0)];

        engine.grow_grid_intrinsic_tracks(
            &mut tracks,
            GridIntrinsicGrowthContext {
                track_sizing: &[fixed_max_auto_min_track, fixed_max_auto_min_track],
                growth_limits: &mut growth_limits,
                span: 0..2,
                gap: 0.0,
                required_size: 100.0,
                minimum_size: 80.0,
                container_axis_is_definite: true,
                container_axis_is_indefinite: false,
                container_axis_is_min_content_constraint: false,
                container_axis_has_max_limit: false,
            },
        );

        assert_close(tracks[0], 40.0);
        assert_close(tracks[1], 40.0);
        assert_eq!(growth_limits, vec![Some(40.0), Some(40.0)]);
    }

    #[test]
    fn grid_intrinsic_growth_definite_minimum_skips_fixed_minimum_tracks() {
        let engine = LayoutEngine::new();
        let fixed_track = GridTrackSizing {
            min_track: Length::points(20.0),
            max_track: Length::points(25.0),
            kind: GridTrackKind::Fixed,
        };
        let mut tracks = vec![20.0, 20.0];
        let mut growth_limits = vec![Some(25.0), Some(25.0)];

        engine.grow_grid_intrinsic_tracks(
            &mut tracks,
            GridIntrinsicGrowthContext {
                track_sizing: &[fixed_track, fixed_track],
                growth_limits: &mut growth_limits,
                span: 0..2,
                gap: 0.0,
                required_size: 100.0,
                minimum_size: 80.0,
                container_axis_is_definite: true,
                container_axis_is_indefinite: false,
                container_axis_is_min_content_constraint: false,
                container_axis_has_max_limit: false,
            },
        );

        assert_close(tracks[0], 20.0);
        assert_close(tracks[1], 20.0);
        assert_eq!(growth_limits, vec![Some(25.0), Some(25.0)]);
    }

    #[test]
    fn grid_growth_limit_distribution_uses_non_affected_tracks_before_beyond_limits() {
        let engine = LayoutEngine::new();
        let tracks = vec![10.0, 10.0, 10.0];
        let track_sizing = vec![
            GridTrackSizing {
                min_track: Length::points(10.0),
                max_track: Length::Auto,
                kind: GridTrackKind::Auto,
            },
            GridTrackSizing {
                min_track: Length::points(10.0),
                max_track: Length::MinContent,
                kind: GridTrackKind::MinContent,
            },
            GridTrackSizing {
                min_track: Length::points(10.0),
                max_track: Length::Auto,
                kind: GridTrackKind::Auto,
            },
        ];
        let mut growth_limits = vec![Some(20.0), None, Some(20.0)];

        engine.update_grid_intrinsic_growth_limits(
            &tracks,
            &track_sizing,
            &mut growth_limits,
            0..3,
            0.0,
            80.0,
            GridIntrinsicLimitPhase::MaxContent,
            &[],
        );

        assert_eq!(growth_limits, vec![Some(20.0), Some(40.0), Some(20.0)]);
    }

    #[test]
    fn grid_growth_limit_distribution_marks_newly_finite_tracks_as_infinitely_growable() {
        let engine = LayoutEngine::new();
        let tracks = vec![10.0, 10.0];
        let track_sizing = vec![
            GridTrackSizing {
                min_track: Length::points(10.0),
                max_track: Length::Auto,
                kind: GridTrackKind::Auto,
            },
            GridTrackSizing {
                min_track: Length::points(10.0),
                max_track: Length::Auto,
                kind: GridTrackKind::Auto,
            },
        ];
        let mut growth_limits = vec![None, Some(10.0)];

        let infinitely_growable = engine.update_grid_intrinsic_growth_limits(
            &tracks,
            &track_sizing,
            &mut growth_limits,
            0..2,
            0.0,
            40.0,
            GridIntrinsicLimitPhase::MinContent,
            &[],
        );
        assert_eq!(infinitely_growable, vec![0]);
        assert_eq!(growth_limits, vec![Some(30.0), Some(10.0)]);

        engine.update_grid_intrinsic_growth_limits(
            &tracks,
            &track_sizing,
            &mut growth_limits,
            0..2,
            0.0,
            80.0,
            GridIntrinsicLimitPhase::MaxContent,
            &infinitely_growable,
        );

        assert_eq!(growth_limits, vec![Some(70.0), Some(10.0)]);
    }

    #[test]
    fn grid_growth_limit_distribution_continues_beyond_limits_when_needed() {
        let engine = LayoutEngine::new();
        let tracks = vec![10.0, 10.0];
        let track_sizing = vec![
            GridTrackSizing {
                min_track: Length::points(10.0),
                max_track: Length::Auto,
                kind: GridTrackKind::Auto,
            },
            GridTrackSizing {
                min_track: Length::points(10.0),
                max_track: Length::Auto,
                kind: GridTrackKind::Auto,
            },
        ];
        let mut growth_limits = vec![Some(20.0), Some(20.0)];

        engine.update_grid_intrinsic_growth_limits(
            &tracks,
            &track_sizing,
            &mut growth_limits,
            0..2,
            0.0,
            70.0,
            GridIntrinsicLimitPhase::MaxContent,
            &[],
        );

        assert_eq!(growth_limits, vec![Some(35.0), Some(35.0)]);
    }

    #[test]
    fn grid_growth_limit_distribution_treats_fit_content_at_limit_as_fixed_beyond_limits() {
        let engine = LayoutEngine::new();
        let tracks = vec![10.0, 10.0];
        let track_sizing = vec![
            GridTrackSizing {
                min_track: Length::points(10.0),
                max_track: Length::fit_content(Some(crate::BaseLength::fixed(20.0))),
                kind: GridTrackKind::FitContent(Some(20.0)),
            },
            GridTrackSizing {
                min_track: Length::points(10.0),
                max_track: Length::Auto,
                kind: GridTrackKind::Auto,
            },
        ];
        let mut growth_limits = vec![Some(20.0), Some(20.0)];

        engine.update_grid_intrinsic_growth_limits(
            &tracks,
            &track_sizing,
            &mut growth_limits,
            0..2,
            0.0,
            60.0,
            GridIntrinsicLimitPhase::MaxContent,
            &[],
        );

        assert_eq!(growth_limits, vec![Some(20.0), Some(40.0)]);
    }

    #[test]
    fn grid_growth_limit_distribution_stops_when_fit_content_argument_already_reached() {
        let engine = LayoutEngine::new();
        let tracks = vec![20.0];
        let track_sizing = vec![GridTrackSizing {
            min_track: Length::points(20.0),
            max_track: Length::fit_content(Some(crate::BaseLength::fixed(20.0))),
            kind: GridTrackKind::FitContent(Some(20.0)),
        }];
        let mut growth_limits = vec![None];

        let infinitely_growable = engine.update_grid_intrinsic_growth_limits(
            &tracks,
            &track_sizing,
            &mut growth_limits,
            0..1,
            0.0,
            40.0,
            GridIntrinsicLimitPhase::MaxContent,
            &[],
        );

        assert!(infinitely_growable.is_empty());
        assert_eq!(growth_limits, vec![None]);
    }

    #[test]
    fn grid_inline_alignment_offset_uses_child_center_override() {
        let engine = LayoutEngine::new();
        let mut container_style = Style::default();
        container_style.justify_items = JustifyItems::End;
        let mut child_style = Style::default();
        child_style.justify_self = JustifyItems::Center;

        assert_close(
            engine.grid_inline_alignment_offset(&container_style, &child_style, 100.0, 40.0, 10.0),
            25.0,
        );
    }

    #[test]
    fn grid_growth_limit_distribution_returns_without_applicable_tracks() {
        let engine = LayoutEngine::new();
        let auto_track = GridTrackSizing {
            min_track: Length::Auto,
            max_track: Length::Auto,
            kind: GridTrackKind::Auto,
        };
        let fixed_track = GridTrackSizing {
            min_track: Length::points(10.0),
            max_track: Length::points(10.0),
            kind: GridTrackKind::Fixed,
        };
        let tracks = vec![10.0];
        let mut growth_limits = vec![Some(10.0)];

        let infinitely_growable = engine.update_grid_intrinsic_growth_limits(
            &tracks,
            &[auto_track],
            &mut growth_limits,
            0..1,
            0.0,
            10.0,
            GridIntrinsicLimitPhase::MaxContent,
            &[],
        );
        assert!(infinitely_growable.is_empty());
        assert_eq!(growth_limits, vec![Some(10.0)]);

        let infinitely_growable = engine.update_grid_intrinsic_growth_limits(
            &tracks,
            &[fixed_track],
            &mut growth_limits,
            0..1,
            0.0,
            50.0,
            GridIntrinsicLimitPhase::MaxContent,
            &[],
        );
        assert!(infinitely_growable.is_empty());
        assert_eq!(growth_limits, vec![Some(10.0)]);
    }

    #[test]
    fn grid_growth_limit_distribution_keeps_fit_content_fixed_after_reaching_argument() {
        let engine = LayoutEngine::new();
        let tracks = vec![10.0, 10.0];
        let track_sizing = vec![
            GridTrackSizing {
                min_track: Length::points(10.0),
                max_track: Length::fit_content(Some(crate::BaseLength::fixed(10.0))),
                kind: GridTrackKind::FitContent(Some(10.0)),
            },
            GridTrackSizing {
                min_track: Length::Auto,
                max_track: Length::Auto,
                kind: GridTrackKind::Auto,
            },
        ];
        let mut growth_limits = vec![Some(10.0), Some(10.0)];

        let remaining = engine.distribute_grid_growth_limits_to_tracks(
            &tracks,
            &track_sizing,
            &mut growth_limits,
            vec![0, 1],
            30.0,
        );
        assert_close(remaining, 0.0);
        assert_eq!(growth_limits, vec![Some(10.0), Some(40.0)]);
    }

    #[test]
    fn grid_growth_limit_beyond_limits_stops_fit_content_at_argument() {
        let engine = LayoutEngine::new();
        let tracks = vec![10.0, 10.0];
        let track_sizing = vec![
            GridTrackSizing {
                min_track: Length::points(10.0),
                max_track: Length::fit_content(Some(crate::BaseLength::fixed(20.0))),
                kind: GridTrackKind::FitContent(Some(20.0)),
            },
            GridTrackSizing {
                min_track: Length::Auto,
                max_track: Length::Auto,
                kind: GridTrackKind::Auto,
            },
        ];
        let mut growth_limits = vec![Some(10.0), Some(10.0)];

        engine.distribute_grid_growth_limits_beyond_limits(
            &tracks,
            &track_sizing,
            &mut growth_limits,
            vec![0, 1],
            30.0,
        );
        assert_eq!(growth_limits, vec![Some(20.0), Some(30.0)]);
    }
}
