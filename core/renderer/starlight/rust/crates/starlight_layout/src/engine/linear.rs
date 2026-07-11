// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use super::*;
use crate::style_data::LinearGravity;

impl LayoutEngine {
    pub(super) fn layout_linear<T: LayoutTree>(
        &mut self,
        tree: &mut T,
        style: &Style,
        children: &[T::NodeId],
        constraints: Constraints,
        edges: ResolvedEdges,
        rounding: RoundingContext,
    ) -> LayoutOutput {
        let is_row = style.linear_orientation.is_row();
        let reverse_main = self.linear_main_front_is_reversed(style, is_row);
        let reverse_cross = self.linear_cross_front_is_reversed(style, is_row);
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
        let cross_axis_constraint = self.axis_constraint(constraints, !is_row);
        let fit_content_cross_axis_constraint = if fixed_cross.is_none() {
            self.linear_fit_content_axis_constraint(style, cross_axis, cross_axis_constraint)
        } else {
            cross_axis_constraint
        };
        let content_main_limit = fixed_main
            .or_else(|| {
                main_axis_constraint
                    .is_definite()
                    .then_some(main_axis_constraint.size)
            })
            .map(|size| self.inner_axis(size, is_row, edges));
        let content_cross_limit = fixed_cross
            .or_else(|| {
                cross_axis_constraint
                    .is_definite()
                    .then_some(cross_axis_constraint.size)
            })
            .map(|size| self.inner_axis(size, !is_row, edges));
        let content_cross_constraint = fixed_cross
            .map(|size| SideConstraint::definite(self.inner_axis(size, !is_row, edges)))
            .unwrap_or_else(|| {
                self.content_constraint_from_parent(
                    fit_content_cross_axis_constraint,
                    if is_row {
                        Axis::Vertical
                    } else {
                        Axis::Horizontal
                    },
                    edges,
                )
            });
        let content_width_limit = fixed_main
            .filter(|_| is_row)
            .or_else(|| fixed_cross.filter(|_| !is_row))
            .or_else(|| {
                constraints
                    .width
                    .is_definite()
                    .then_some(constraints.width.size)
            })
            .map(|width| self.inner_width(width, edges));
        self.layout_display_none_children(tree, children, edges, rounding);
        let visible_children = self.ordered_in_flow_children(&*tree, children);
        let mut items = Vec::with_capacity(visible_children.len());

        for child in visible_children {
            let child_style = tree.style(child).clone();
            let child_content_cross_constraint =
                self.linear_child_cross_constraint(style, &child_style, content_cross_constraint);
            let child_parent_width_limit = if is_row {
                content_width_limit
            } else {
                child_content_cross_constraint.percent_base()
            };
            let child_edges = self.resolve_edges_for_parent(
                &child_style,
                self.axis_constraint_from_optional(child_parent_width_limit),
            );
            let layout_gravity = self.computed_linear_layout_gravity(style, &child_style);
            let child_main =
                self.resolve_child_main_size(&child_style, is_row, content_main_limit, child_edges);
            let child_cross = self.resolve_child_cross_size(
                &child_style,
                is_row,
                child_content_cross_constraint.percent_base(),
                child_edges,
            );
            let child_percent_constraints = if is_row {
                Constraints::new(
                    self.axis_constraint_from_optional(content_main_limit),
                    child_content_cross_constraint,
                )
            } else {
                Constraints::new(
                    child_content_cross_constraint,
                    self.axis_constraint_from_optional(content_main_limit),
                )
            };
            let child_cross_intrinsic = self.child_cross_axis_is_intrinsic(&child_style, is_row);
            let force_stretch = self.linear_layout_gravity_forces_stretch(layout_gravity);
            let auto_stretch = layout_gravity == LinearLayoutGravity::None
                && child_cross.is_none()
                && !child_cross_intrinsic;
            let stretch_cross =
                content_cross_constraint.is_definite() && (force_stretch || auto_stretch);
            let cross_constraint = if stretch_cross {
                SideConstraint::definite(
                    (content_cross_constraint.size - self.axis_margin(child_edges.margin, !is_row))
                        .max(0.0),
                )
            } else {
                self.default_linear_child_cross_constraint(
                    &child_style,
                    child_cross,
                    child_content_cross_constraint,
                    child_edges,
                    is_row,
                )
            };
            let child_style_override = self.linear_child_style_override(
                &child_style,
                child_main,
                if cross_constraint.is_definite() && (force_stretch || child_cross.is_some()) {
                    Some(cross_constraint.size)
                } else {
                    None
                },
                is_row,
                child_edges,
                child_percent_constraints,
            );
            let child_main_constraint = child_main;
            let child_constraints =
                self.child_linear_constraints(child_main_constraint, cross_constraint, is_row);
            let measured =
                if child_style.linear_weight > self.epsilon && content_main_limit.is_some() {
                    LayoutBox::new(Size::ZERO, child_edges.margin, None, child_constraints)
                } else {
                    self.layout_node_with_edges(
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
                    )
                };
            let main = self.main_size(measured.size, is_row);
            let cross = self.cross_size(measured.size, is_row);
            items.push(LinearItem {
                id: child,
                style: child_style,
                style_override: child_style_override,
                constraints: child_constraints,
                edges: child_edges,
                measured,
                main,
                cross,
                cross_constraint,
                baseline_from_top_margin_edge: self
                    .item_baseline_from_top_margin_edge(measured, child_edges),
            });
        }

        self.resolve_linear_weights(
            tree,
            &mut items,
            LinearLayoutContext {
                container_style: style,
                content_main_limit,
                is_row,
            },
        );
        let total_main = self.linear_total_main(&items, is_row, reverse_main);
        let max_cross = self.linear_max_cross(&items, is_row);
        let natural_content_main = content_main_limit.unwrap_or(total_main).max(0.0);
        let natural_content_cross = content_cross_limit.unwrap_or(max_cross).max(0.0);
        let update_container_content_width = if is_row {
            natural_content_main
        } else {
            natural_content_cross
        };
        let mut width = if is_row {
            natural_content_main
        } else {
            natural_content_cross
        };
        let mut height = if is_row {
            natural_content_cross
        } else {
            natural_content_main
        };
        width += edges.padding.horizontal() + edges.border.horizontal();
        height += edges.padding.vertical() + edges.border.vertical();
        let clamp_constraints = self.definite_container_clamp_constraints(constraints);
        width = self.clamp_axis(style, Axis::Horizontal, width, clamp_constraints, edges);
        height = self.clamp_axis(style, Axis::Vertical, height, clamp_constraints, edges);
        let size = Size::new(width.max(0.0), height.max(0.0));
        let content_main = if is_row {
            self.inner_width(size.width, edges)
        } else {
            self.inner_height(size.height, edges)
        };
        let content_cross = if is_row {
            self.inner_height(size.height, edges)
        } else {
            self.inner_width(size.width, edges)
        };
        let remaining_main = content_main - total_main;
        let (mut main_cursor, item_gap) =
            self.linear_main_alignment(style, remaining_main, items.len(), is_row, reverse_main);
        let sticky_constraints =
            self.content_constraints_from_main_cross(content_main, content_cross, is_row);
        let container_cross_length = if is_row { style.height } else { style.width };

        let item_count = items.len();
        let mut final_row_baseline: Option<f32> = None;
        let mut final_first_baseline_from_top_margin_edge: Option<f32> = None;
        for (item_index, item) in items.into_iter().enumerate() {
            let final_edges = self.update_percent_box_edges(
                &item.style,
                item.edges,
                SideConstraint::definite(update_container_content_width),
            );
            let layout_gravity = self.computed_linear_layout_gravity(style, &item.style);
            let cross_length = if is_row {
                item.style.height
            } else {
                item.style.width
            };
            let child_has_subtree = tree.children(item.id).next().is_some();
            let child_is_leaf_or_measured = tree.has_measure(item.id) || !child_has_subtree;
            let definite_cross_allows_percent_remeasure = content_cross_constraint.is_definite()
                && !(container_cross_length == Length::Auto && child_is_leaf_or_measured);
            let should_remeasure_final_cross = Self::length_needs_percent_override(cross_length)
                && definite_cross_allows_percent_remeasure;
            let (final_style_override, final_child_constraints, final_cross) =
                if should_remeasure_final_cross {
                    let final_child_content_cross_constraint = self.linear_child_cross_constraint(
                        style,
                        &item.style,
                        SideConstraint::definite(content_cross),
                    );
                    let final_child_percent_constraints = if is_row {
                        Constraints::new(
                            self.axis_constraint_from_optional(Some(content_main)),
                            final_child_content_cross_constraint,
                        )
                    } else {
                        Constraints::new(
                            final_child_content_cross_constraint,
                            self.axis_constraint_from_optional(Some(content_main)),
                        )
                    };
                    let final_child_cross = self.resolve_child_cross_size(
                        &item.style,
                        is_row,
                        final_child_content_cross_constraint.percent_base(),
                        final_edges,
                    );
                    let force_stretch = self.linear_layout_gravity_forces_stretch(layout_gravity);
                    let stretch_cross =
                        final_child_content_cross_constraint.is_definite() && force_stretch;
                    let final_cross_constraint = if stretch_cross {
                        SideConstraint::definite(
                            (final_child_content_cross_constraint.size
                                - self.axis_margin(final_edges.margin, !is_row))
                            .max(0.0),
                        )
                    } else {
                        self.default_linear_child_cross_constraint(
                            &item.style,
                            final_child_cross,
                            final_child_content_cross_constraint,
                            final_edges,
                            is_row,
                        )
                    };
                    let final_child_constraints = if is_row {
                        Constraints::new(item.constraints.width, final_cross_constraint)
                    } else {
                        Constraints::new(final_cross_constraint, item.constraints.height)
                    };
                    let mut final_style_override = item
                        .style_override
                        .clone()
                        .unwrap_or_else(|| item.style.clone());
                    let axis = if is_row {
                        Axis::Vertical
                    } else {
                        Axis::Horizontal
                    };
                    self.set_css_axis_size_from_border_size(
                        &mut final_style_override,
                        &item.style,
                        axis,
                        final_cross_constraint.size,
                        final_edges,
                    );
                    self.override_min_max_percent_lengths(
                        &item.style,
                        final_child_percent_constraints,
                        &mut final_style_override,
                    );
                    let final_style_override = Some(final_style_override);
                    let final_measured = self.layout_node_with_edges(
                        tree,
                        item.id,
                        final_style_override.clone(),
                        NodeLayoutContext {
                            constraints: final_child_constraints,
                            offset: Point::ZERO,
                            sticky_constraints: final_child_constraints,
                            edges: final_edges,
                            rounding,
                            flex: FlexNodeContext::default(),
                        },
                    );
                    (
                        final_style_override,
                        final_child_constraints,
                        self.cross_size(final_measured.size, is_row),
                    )
                } else {
                    (item.style_override.clone(), item.constraints, item.cross)
                };
            let main_start_margin = self.main_start_margin_with_auto(
                &item.style,
                final_edges.margin,
                is_row,
                reverse_main,
                None,
            );
            let main_end_margin = self.main_end_margin_with_auto(
                &item.style,
                final_edges.margin,
                is_row,
                reverse_main,
                None,
            );
            let has_auto_cross_margin = self.has_axis_auto_margin(&item.style, !is_row);
            let child_cross_offset = if has_auto_cross_margin {
                let cross_offset = self.auto_cross_offset(
                    &item.style,
                    final_edges.margin,
                    content_cross,
                    final_cross,
                    is_row,
                    reverse_cross,
                );
                self.physical_offset_from_logical_start(
                    cross_offset,
                    content_cross,
                    final_cross,
                    reverse_cross,
                )
            } else {
                let margin_bound_offset = self.linear_cross_margin_bound_offset(
                    layout_gravity,
                    content_cross,
                    final_cross,
                    final_edges.margin,
                    is_row,
                );
                self.linear_physical_cross_offset_from_margin_bound(
                    margin_bound_offset,
                    content_cross,
                    final_cross,
                    final_edges.margin,
                    is_row,
                    reverse_cross,
                )
            };
            let child_main_offset = self.flex_physical_offset_from_logical_margin_start(
                main_cursor,
                content_main,
                item.main,
                main_start_margin,
                main_end_margin,
                reverse_main,
            );
            let child_offset = if is_row {
                Point::new(
                    Self::parent_border_offset(
                        edges.border.left,
                        edges.padding.left,
                        child_main_offset,
                    ),
                    Self::parent_border_offset(
                        edges.border.top,
                        edges.padding.top,
                        child_cross_offset,
                    ),
                )
            } else {
                Point::new(
                    Self::parent_border_offset(
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
            let used_margin = self.linear_used_margin(
                &item.style,
                final_edges.margin,
                LinearUsedMarginContext {
                    item_cross: final_cross,
                    line_cross: content_cross,
                    is_row,
                    reverse_cross,
                },
            );
            let mut child_box = self.layout_node_with_edges(
                tree,
                item.id,
                final_style_override,
                NodeLayoutContext {
                    constraints: final_child_constraints,
                    offset: child_offset,
                    sticky_constraints,
                    edges: final_edges,
                    rounding,
                    flex: FlexNodeContext::default(),
                },
            );
            child_box.layout.margin = used_margin;
            let final_main = self.main_size(child_box.size, is_row);
            let final_item_baseline =
                self.item_baseline_from_top_margin_edge(child_box, final_edges);
            if is_row {
                let final_cross = self.cross_size(child_box.size, is_row);
                let cross_margin_bound = final_cross + self.axis_margin(final_edges.margin, false);
                let final_baseline_with_cross_offset = final_item_baseline
                    + if self.linear_layout_gravity_is_after(layout_gravity) {
                        content_cross - cross_margin_bound
                    } else if self.linear_layout_gravity_is_center(layout_gravity) {
                        (content_cross - cross_margin_bound) / 2.0
                    } else {
                        0.0
                    };
                final_row_baseline = Some(
                    final_row_baseline
                        .unwrap_or(0.0)
                        .max(final_baseline_with_cross_offset),
                );
            } else if item_index == 0 {
                final_first_baseline_from_top_margin_edge = Some(final_item_baseline);
            }
            tree.set_layout_with_constraints(item.id, child_box.constraints, child_box.layout);
            let final_outer_main =
                self.physical_margin_bound_axis_size(final_main, final_edges.margin, is_row);
            main_cursor += final_outer_main + item_gap;
        }
        let final_content_main = if is_row {
            self.inner_width(size.width, edges)
        } else {
            self.inner_height(size.height, edges)
        };
        let baseline = if is_row {
            self.exported_container_baseline(final_row_baseline)
        } else {
            final_first_baseline_from_top_margin_edge.and_then(|first_baseline| {
                let remaining_main = final_content_main - total_main;
                let (main_start, _) = self.linear_main_alignment(
                    style,
                    remaining_main,
                    item_count,
                    is_row,
                    reverse_main,
                );
                self.exported_container_baseline(Some(main_start + first_baseline))
            })
        };

        self.layout_out_of_flow_children(tree, children, style, size, edges, rounding);
        LayoutOutput { size, baseline }
    }
    fn linear_child_cross_constraint(
        &self,
        container_style: &Style,
        child_style: &Style,
        content_cross_constraint: SideConstraint,
    ) -> SideConstraint {
        let is_row = container_style.linear_orientation.is_row();
        let child_cross_length = if is_row {
            child_style.height
        } else {
            child_style.width
        };
        if let Length::FitContent(base) = child_cross_length {
            self.fit_content_owner_constraint(base, content_cross_constraint)
        } else {
            content_cross_constraint
        }
    }
    fn resolve_linear_weights<T: LayoutTree>(
        &mut self,
        tree: &mut T,
        items: &mut [LinearItem<T::NodeId>],
        context: LinearLayoutContext<'_>,
    ) {
        let Some(content_main) = context.content_main_limit else {
            return;
        };
        let weighted_indices = items
            .iter()
            .enumerate()
            .filter_map(|(idx, item)| (item.style.linear_weight > self.epsilon).then_some(idx))
            .collect::<Vec<_>>();
        if weighted_indices.is_empty() {
            return;
        }

        let initial_total_weight = weighted_indices
            .iter()
            .map(|&idx| items[idx].style.linear_weight.max(0.0))
            .sum::<f32>();
        let weight_sum_override = context.container_style.linear_weight_sum.max(0.0);

        for &idx in &weighted_indices {
            items[idx].main = 0.0;
        }

        let mut frozen = vec![false; weighted_indices.len()];
        let initial_free_space = self.remaining_linear_weight_space(
            items,
            &weighted_indices,
            &frozen,
            content_main,
            context.is_row,
        );

        loop {
            let active_weight = weighted_indices
                .iter()
                .zip(frozen.iter())
                .filter_map(|(&idx, is_frozen)| {
                    (!*is_frozen).then_some(items[idx].style.linear_weight.max(0.0))
                })
                .sum::<f32>();

            let remaining_free_space = self.remaining_linear_weight_space(
                items,
                &weighted_indices,
                &frozen,
                content_main,
                context.is_row,
            );
            let adjusted_free_space = if weight_sum_override > self.epsilon {
                initial_free_space * initial_total_weight / weight_sum_override
            } else {
                initial_free_space * active_weight
            };
            let free_space = if adjusted_free_space.abs() < remaining_free_space.abs() {
                adjusted_free_space
            } else {
                remaining_free_space
            };

            let mut total_violation = 0.0;
            let mut min_violations = Vec::new();
            let mut max_violations = Vec::new();

            for (weighted_idx, &idx) in weighted_indices.iter().enumerate() {
                if frozen[weighted_idx] {
                    continue;
                }

                let raw = if free_space > self.epsilon {
                    free_space * items[idx].style.linear_weight.max(0.0) / active_weight
                } else {
                    0.0
                };
                let clamped = self.clamp_flex_item_main(
                    &items[idx].style,
                    items[idx].edges,
                    raw,
                    context.is_row,
                    Some(content_main),
                    false,
                );
                items[idx].main = clamped;
                let violation = clamped - raw;
                total_violation += violation;
                if violation > self.epsilon {
                    min_violations.push(weighted_idx);
                } else if violation < -self.epsilon {
                    max_violations.push(weighted_idx);
                }
            }

            if total_violation.abs() <= self.epsilon {
                break;
            }

            let violations = if total_violation > 0.0 {
                min_violations
            } else {
                max_violations
            };
            if violations.is_empty() {
                break;
            }
            for idx in violations {
                frozen[idx] = true;
            }
            if frozen.iter().all(|is_frozen| *is_frozen) {
                break;
            }
        }

        for &idx in &weighted_indices {
            let item = &mut items[idx];
            let child_constraints =
                self.child_main_cross_constraints(item.main, item.cross_constraint, context.is_row);
            let child_style = self.main_axis_size_override(
                item.style_override.as_ref().unwrap_or(&item.style),
                item.edges,
                item.main,
                context.is_row,
            );
            let measured = self.layout_node_with_style_override_and_sticky_constraints(
                tree,
                item.id,
                Some(child_style.clone()),
                child_constraints,
                Point::ZERO,
                child_constraints,
            );
            item.style_override = Some(child_style);
            item.constraints = child_constraints;
            item.measured = measured;
            item.main = self.main_size(measured.size, context.is_row);
            item.cross = self.cross_size(measured.size, context.is_row);
            item.baseline_from_top_margin_edge =
                self.item_baseline_from_top_margin_edge(measured, item.edges);
        }
    }
    fn remaining_linear_weight_space<N>(
        &self,
        items: &[LinearItem<N>],
        weighted_indices: &[usize],
        frozen: &[bool],
        content_main: f32,
        is_row: bool,
    ) -> f32 {
        let mut occupied = 0.0;
        let mut weighted_cursor = 0;
        for (idx, item) in items.iter().enumerate() {
            let weighted_idx = if weighted_cursor < weighted_indices.len()
                && weighted_indices[weighted_cursor] == idx
            {
                let weighted_idx = Some(weighted_cursor);
                weighted_cursor += 1;
                weighted_idx
            } else {
                None
            };
            let main = match weighted_idx {
                Some(weighted_idx) if !frozen[weighted_idx] => 0.0,
                _ => item.main,
            };
            occupied += main + self.axis_margin(item.edges.margin, is_row);
        }
        content_main - occupied
    }
    fn linear_total_main<N>(
        &self,
        items: &[LinearItem<N>],
        is_row: bool,
        reverse_main: bool,
    ) -> f32 {
        items.iter().fold(0.0, |sum, item| {
            let main_front_margin =
                self.axis_logical_start_margin(item.edges.margin, is_row, reverse_main);
            let main_back_margin =
                self.axis_logical_end_margin(item.edges.margin, is_row, reverse_main);
            let outer_main = item.main + main_front_margin + main_back_margin;
            sum + outer_main
        })
    }
    fn linear_max_cross<N>(&self, items: &[LinearItem<N>], is_row: bool) -> f32 {
        items.iter().fold(0.0, |max_cross, item| {
            max_cross.max(item.cross + self.axis_margin(item.edges.margin, !is_row))
        })
    }
    pub(super) fn linear_out_of_flow_alignment(
        &self,
        container_style: &Style,
        child_style: &Style,
    ) -> OutOfFlowAlignment {
        let is_row = container_style.linear_orientation.is_row();
        let reverse_main = self.linear_main_front_is_reversed(container_style, is_row);
        let reverse_cross = self.linear_cross_front_is_reversed(container_style, is_row);
        let main = match self.logic_linear_gravity(container_style, is_row, reverse_main) {
            LinearGravity::End => OutOfFlowAxisAlignment::end_with_front(!reverse_main),
            LinearGravity::Center
            | LinearGravity::CenterHorizontal
            | LinearGravity::CenterVertical => OutOfFlowAxisAlignment::center(),
            LinearGravity::None
            | LinearGravity::Top
            | LinearGravity::Bottom
            | LinearGravity::Left
            | LinearGravity::Right
            | LinearGravity::Start
            | LinearGravity::SpaceBetween => {
                OutOfFlowAxisAlignment::start_with_front(!reverse_main)
            }
        };
        let layout_gravity = self.computed_linear_layout_gravity(container_style, child_style);
        let cross = if self.linear_layout_gravity_is_after(layout_gravity) {
            OutOfFlowAxisAlignment::end_with_front(!reverse_cross)
        } else if self.linear_layout_gravity_is_center(layout_gravity) {
            OutOfFlowAxisAlignment::center()
        } else {
            OutOfFlowAxisAlignment::start_with_front(!reverse_cross)
        };

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
    fn child_linear_constraints(
        &self,
        main_size: Option<f32>,
        cross_constraint: SideConstraint,
        is_row: bool,
    ) -> Constraints {
        if is_row {
            Constraints::new(
                main_size
                    .map(SideConstraint::definite)
                    .unwrap_or_else(SideConstraint::indefinite),
                cross_constraint,
            )
        } else {
            Constraints::new(
                cross_constraint,
                main_size
                    .map(SideConstraint::definite)
                    .unwrap_or_else(SideConstraint::indefinite),
            )
        }
    }
    fn linear_fit_content_axis_constraint(
        &self,
        style: &Style,
        axis: Axis,
        parent_constraint: SideConstraint,
    ) -> SideConstraint {
        let length = match axis {
            Axis::Horizontal => style.width,
            Axis::Vertical => style.height,
        };
        if let Length::FitContent(base) = length {
            self.fit_content_owner_constraint(base, parent_constraint)
        } else {
            parent_constraint
        }
    }
    fn default_linear_child_cross_constraint(
        &self,
        style: &Style,
        explicit_cross: Option<f32>,
        available_cross: SideConstraint,
        edges: ResolvedEdges,
        parent_is_row: bool,
    ) -> SideConstraint {
        if let Some(cross) = explicit_cross {
            return SideConstraint::definite(cross);
        }
        if self.child_cross_axis_is_content_keyword(style, parent_is_row) {
            return SideConstraint::indefinite();
        }
        if available_cross.mode == MeasureMode::Indefinite {
            return SideConstraint::indefinite();
        }
        if self.child_cross_axis_is_fit_content(style, parent_is_row) {
            return available_cross;
        }
        SideConstraint::at_most(
            (available_cross.size - self.axis_margin(edges.margin, !parent_is_row)).max(0.0),
        )
    }
    fn linear_main_alignment(
        &self,
        style: &Style,
        free_space: f32,
        item_count: usize,
        is_row: bool,
        reverse_main: bool,
    ) -> (f32, f32) {
        match self.logic_linear_gravity(style, is_row, reverse_main) {
            LinearGravity::End => (free_space, 0.0),
            LinearGravity::Center
            | LinearGravity::CenterHorizontal
            | LinearGravity::CenterVertical => (free_space / 2.0, 0.0),
            LinearGravity::SpaceBetween if item_count > 1 => {
                (0.0, free_space.max(0.0) / (item_count - 1) as f32)
            }
            LinearGravity::None
            | LinearGravity::Top
            | LinearGravity::Bottom
            | LinearGravity::Left
            | LinearGravity::Right
            | LinearGravity::Start
            | LinearGravity::SpaceBetween => (0.0, 0.0),
        }
    }
    fn logic_linear_gravity(
        &self,
        style: &Style,
        is_row: bool,
        reverse_main: bool,
    ) -> LinearGravity {
        let gravity = if style.linear_gravity == LinearGravity::None {
            self.linear_gravity_from_justify_content(style.justify_content)
        } else {
            style.linear_gravity
        };
        if self.linear_gravity_is_physical(gravity) {
            self.physical_linear_gravity_to_logic(gravity, is_row, reverse_main)
        } else {
            gravity
        }
    }
    fn linear_gravity_from_justify_content(
        &self,
        justify_content: JustifyContent,
    ) -> LinearGravity {
        match justify_content {
            JustifyContent::FlexEnd | JustifyContent::End => LinearGravity::End,
            JustifyContent::Center => LinearGravity::Center,
            JustifyContent::SpaceBetween => LinearGravity::SpaceBetween,
            JustifyContent::Stretch
            | JustifyContent::FlexStart
            | JustifyContent::Start
            | JustifyContent::SpaceAround
            | JustifyContent::SpaceEvenly => LinearGravity::Start,
        }
    }
    fn linear_gravity_is_physical(&self, gravity: LinearGravity) -> bool {
        matches!(
            gravity,
            LinearGravity::Left | LinearGravity::Right | LinearGravity::Top | LinearGravity::Bottom
        )
    }
    fn physical_linear_gravity_to_logic(
        &self,
        gravity: LinearGravity,
        is_row: bool,
        reverse_main: bool,
    ) -> LinearGravity {
        let points_to_main_back = matches!(
            (is_row, reverse_main, gravity),
            (true, false, LinearGravity::Right)
                | (true, true, LinearGravity::Left)
                | (false, false, LinearGravity::Bottom)
                | (false, true, LinearGravity::Top)
        );
        if points_to_main_back {
            LinearGravity::End
        } else {
            LinearGravity::Start
        }
    }
    fn linear_main_front_is_reversed(&self, style: &Style, is_row: bool) -> bool {
        if is_row {
            style.linear_orientation.is_reverse() ^ style.direction.is_any_rtl()
        } else {
            style.linear_orientation.is_reverse()
        }
    }
    fn linear_cross_front_is_reversed(&self, style: &Style, is_row: bool) -> bool {
        !is_row && style.direction.is_any_rtl()
    }
    fn main_start_margin_with_auto(
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
    fn main_end_margin_with_auto(
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
    fn auto_cross_offset(
        &self,
        style: &Style,
        margin: Edges,
        line_cross: f32,
        child_cross: f32,
        parent_is_row: bool,
        reverse_cross: bool,
    ) -> f32 {
        let (start_margin, _) = self.resolved_auto_cross_margins(
            style,
            margin,
            line_cross,
            child_cross,
            parent_is_row,
            reverse_cross,
        );
        start_margin
    }
    fn linear_used_margin(
        &self,
        style: &Style,
        base_margin: Edges,
        context: LinearUsedMarginContext,
    ) -> Edges {
        let mut margin = base_margin;
        let cross_horizontal = !context.is_row;
        let start_auto =
            self.axis_logical_start_margin_is_auto(style, cross_horizontal, context.reverse_cross);
        let end_auto =
            self.axis_logical_end_margin_is_auto(style, cross_horizontal, context.reverse_cross);
        if start_auto || end_auto {
            let (resolved_start, resolved_end) = self.resolved_auto_cross_margins(
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
    fn resolved_auto_cross_margins(
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
        }

        (start_margin, end_margin)
    }
    fn linear_child_style_override(
        &self,
        style: &Style,
        border_main_size: Option<f32>,
        border_cross_size: Option<f32>,
        parent_is_row: bool,
        edges: ResolvedEdges,
        min_max_percent_constraints: Constraints,
    ) -> Option<Style> {
        let mut override_style = style.clone();
        let mut changed = false;
        if let Some(main_size) = border_main_size {
            let axis = if parent_is_row {
                Axis::Horizontal
            } else {
                Axis::Vertical
            };
            self.set_css_axis_size_from_border_size(
                &mut override_style,
                style,
                axis,
                main_size,
                edges,
            );
            changed = true;
        }
        if let Some(cross_size) = border_cross_size {
            let axis = if parent_is_row {
                Axis::Vertical
            } else {
                Axis::Horizontal
            };
            self.set_css_axis_size_from_border_size(
                &mut override_style,
                style,
                axis,
                cross_size,
                edges,
            );
            changed = true;
        }
        changed |= self.override_min_max_percent_lengths(
            style,
            min_max_percent_constraints,
            &mut override_style,
        );
        changed.then_some(override_style)
    }
    fn linear_cross_margin_bound_offset(
        &self,
        layout_gravity: LinearLayoutGravity,
        content_cross: f32,
        child_cross: f32,
        margin: Edges,
        parent_is_row: bool,
    ) -> f32 {
        let child_cross_margin_bound = child_cross + self.axis_margin(margin, !parent_is_row);
        let free = content_cross - child_cross_margin_bound;
        if self.linear_layout_gravity_is_after(layout_gravity) {
            free
        } else if self.linear_layout_gravity_is_center(layout_gravity) {
            free / 2.0
        } else {
            0.0
        }
    }
    fn linear_physical_cross_offset_from_margin_bound(
        &self,
        margin_bound_offset: f32,
        content_cross: f32,
        child_cross: f32,
        margin: Edges,
        parent_is_row: bool,
        reverse_cross: bool,
    ) -> f32 {
        let cross_horizontal = !parent_is_row;
        let physical_start_margin = self.axis_start_margin(margin, cross_horizontal);
        if reverse_cross {
            let margin_bound = child_cross + self.axis_margin(margin, cross_horizontal);
            ((content_cross - margin_bound) - margin_bound_offset) + physical_start_margin
        } else {
            margin_bound_offset + physical_start_margin
        }
    }
    fn linear_layout_gravity_is_after(&self, gravity: LinearLayoutGravity) -> bool {
        matches!(
            gravity,
            LinearLayoutGravity::Right | LinearLayoutGravity::Bottom | LinearLayoutGravity::End
        )
    }
    fn linear_layout_gravity_is_center(&self, gravity: LinearLayoutGravity) -> bool {
        matches!(
            gravity,
            LinearLayoutGravity::CenterHorizontal
                | LinearLayoutGravity::CenterVertical
                | LinearLayoutGravity::Center
        )
    }
}

#[derive(Clone, Copy, Debug)]
struct LinearUsedMarginContext {
    item_cross: f32,
    line_cross: f32,
    is_row: bool,
    reverse_cross: bool,
}

#[derive(Clone, Debug)]
struct LinearItem<N> {
    id: N,
    style: Style,
    style_override: Option<Style>,
    constraints: Constraints,
    edges: ResolvedEdges,
    measured: LayoutBox,
    main: f32,
    cross: f32,
    cross_constraint: SideConstraint,
    baseline_from_top_margin_edge: f32,
}

#[derive(Clone, Copy, Debug)]
struct LinearLayoutContext<'a> {
    container_style: &'a Style,
    content_main_limit: Option<f32>,
    is_row: bool,
}
