// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use std::collections::BTreeSet;

use super::*;
use crate::style::{RELATIVE_ALIGN_NONE, RELATIVE_ALIGN_PARENT};

impl LayoutEngine {
    pub(super) fn layout_relative<T: LayoutTree>(
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

        let definite_content_width = fixed_width
            .or_else(|| {
                constraints
                    .width
                    .is_definite()
                    .then_some(constraints.width.size)
            })
            .map(|width| self.inner_width(width, edges));
        let definite_content_height = fixed_height
            .or_else(|| {
                constraints
                    .height
                    .is_definite()
                    .then_some(constraints.height.size)
            })
            .map(|height| self.inner_height(height, edges));
        let available_content_width = fixed_width
            .or_else(|| constraints.width.bounded_size())
            .map(|width| self.inner_width(width, edges));
        let available_content_height = fixed_height
            .or_else(|| constraints.height.bounded_size())
            .map(|height| self.inner_height(height, edges));
        let relative_context = RelativeConstraintContext {
            available_content_width,
            available_content_height,
            child_percent_base_width: definite_content_width,
            child_percent_base_height: definite_content_height,
            definite_content_width,
            definite_content_height,
        };

        self.layout_display_none_children(tree, children, edges, rounding);
        let visible_children = self.ordered_in_flow_children(&*tree, children);
        let mut items = Vec::new();
        for child in visible_children {
            let child_style = tree.style(child).clone();
            let child_edges = self.resolve_edges_for_parent(
                &child_style,
                self.axis_constraint_from_optional(definite_content_width),
            );
            let child_measure_context = if style.relative_layout_once {
                relative_context
            } else {
                RelativeConstraintContext {
                    definite_content_height: None,
                    ..relative_context
                }
            };
            let child_constraints =
                self.relative_child_constraints(&child_style, child_edges, child_measure_context);
            let child_style_override = self.relative_child_style_override(
                &child_style,
                child_constraints,
                child_edges,
                self.relative_percent_constraints(child_measure_context),
                RelativeDefiniteConstraintOverride::None,
            );
            let measured_size = if style.relative_layout_once {
                Size::ZERO
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
                .size
            };
            items.push(RelativeItem {
                id: child,
                style: child_style,
                style_override: child_style_override,
                edges: child_edges,
                constraints: child_constraints,
                size: measured_size,
                left: 0.0,
                right: 0.0,
                top: 0.0,
                bottom: 0.0,
            });
        }

        if style.relative_layout_once {
            self.measure_and_position_relative_once(
                tree,
                &mut items,
                definite_content_width,
                definite_content_height,
            );
        } else {
            self.position_relative_items(&mut items, true, definite_content_width);
            self.position_relative_items(&mut items, false, definite_content_height);
            self.remeasure_relative_constrained_items(
                tree,
                &mut items,
                definite_content_width,
                definite_content_height,
            );
            self.position_relative_items(&mut items, true, definite_content_width);
            self.position_relative_items(&mut items, false, definite_content_height);
        }

        let raw_content_width = fixed_width
            .or_else(|| {
                constraints
                    .width
                    .is_definite()
                    .then_some(constraints.width.size)
            })
            .map(|width| self.inner_width(width, edges))
            .unwrap_or_else(|| self.relative_content_extent(&items, true));

        if definite_content_width.is_none() {
            self.update_relative_item_percent_edges(&mut items, raw_content_width);
        }

        if !style.relative_layout_once && definite_content_width.is_none() {
            let horizontal_order = self.relative_order(&items, true);
            self.recompute_relative_axis_positions(
                &mut items,
                &horizontal_order,
                true,
                raw_content_width,
            );
            self.remeasure_relative_horizontal_proposed_items(
                tree,
                &mut items,
                Some(raw_content_width),
                definite_content_height,
            );
            self.position_relative_items(&mut items, false, definite_content_height);
        }

        let raw_content_height = fixed_height
            .or_else(|| {
                constraints
                    .height
                    .is_definite()
                    .then_some(constraints.height.size)
            })
            .map(|height| self.inner_height(height, edges))
            .unwrap_or_else(|| {
                if style.relative_layout_once {
                    self.relative_content_extent(&items, false)
                } else {
                    self.relative_content_height_after_recompute(&items)
                }
            });

        let clamp_constraints = self.definite_container_clamp_constraints(constraints);
        let width = self.clamp_axis(
            style,
            Axis::Horizontal,
            fixed_width.unwrap_or_else(|| {
                raw_content_width + edges.padding.horizontal() + edges.border.horizontal()
            }),
            clamp_constraints,
            edges,
        );
        let height = self.clamp_axis(
            style,
            Axis::Vertical,
            fixed_height.unwrap_or_else(|| {
                raw_content_height + edges.padding.vertical() + edges.border.vertical()
            }),
            clamp_constraints,
            edges,
        );
        let content_width = self.inner_width(width, edges);
        let content_height = self.inner_height(height, edges);

        self.recompute_relative_final_positions(
            &mut items,
            content_width,
            content_height,
            style.relative_layout_once,
        );
        let sticky_constraints = Constraints::definite(content_width, content_height);

        let content_left = edges.border.left + edges.padding.left;
        let content_top = edges.border.top + edges.padding.top;
        for item in &items {
            let offset = Point::new(
                content_left + item.left + item.edges.margin.left,
                content_top + item.top + item.edges.margin.top,
            );
            self.layout_node_with_edges(
                tree,
                item.id,
                item.style_override.clone(),
                NodeLayoutContext {
                    constraints: item.constraints,
                    offset,
                    sticky_constraints,
                    edges: item.edges,
                    rounding,
                    flex: FlexNodeContext::default(),
                },
            );
        }

        let size = Size::new(width.max(0.0), height.max(0.0));
        self.layout_out_of_flow_children(tree, children, style, size, edges, rounding);
        LayoutOutput::new(size)
    }

    fn update_relative_item_percent_edges<N>(&self, items: &mut [RelativeItem<N>], width: f32) {
        let parent_width_constraint = SideConstraint::definite(width);
        for item in items {
            item.edges =
                self.update_percent_box_edges(&item.style, item.edges, parent_width_constraint);
        }
    }

    fn relative_child_constraints(
        &self,
        style: &Style,
        edges: ResolvedEdges,
        context: RelativeConstraintContext,
    ) -> Constraints {
        let child_percent_constraints = Constraints::new(
            self.axis_constraint_from_optional(context.child_percent_base_width),
            self.axis_constraint_from_optional(context.child_percent_base_height),
        );
        let width = self
            .resolve_border_axis(
                style.width,
                Axis::Horizontal,
                child_percent_constraints,
                edges,
                style.box_sizing,
            )
            .map(SideConstraint::definite)
            .or_else(|| {
                self.relative_fit_content_axis_constraint(
                    style.width,
                    context.child_percent_base_width,
                    context.available_content_width,
                )
            })
            .or_else(|| {
                (self.relative_axis_stretches_to_parent(style, true))
                    .then(|| {
                        context.definite_content_width.map(|width| {
                            SideConstraint::definite((width - edges.margin.horizontal()).max(0.0))
                        })
                    })
                    .flatten()
            })
            .or_else(|| {
                context.available_content_width.map(|width| {
                    SideConstraint::at_most((width - edges.margin.horizontal()).max(0.0))
                })
            })
            .unwrap_or_else(SideConstraint::indefinite);

        let height = self
            .resolve_border_axis(
                style.height,
                Axis::Vertical,
                child_percent_constraints,
                edges,
                style.box_sizing,
            )
            .map(SideConstraint::definite)
            .or_else(|| {
                self.relative_fit_content_axis_constraint(
                    style.height,
                    context.child_percent_base_height,
                    context.available_content_height,
                )
            })
            .or_else(|| {
                (self.relative_axis_stretches_to_parent(style, false))
                    .then(|| {
                        context.definite_content_height.map(|height| {
                            SideConstraint::definite((height - edges.margin.vertical()).max(0.0))
                        })
                    })
                    .flatten()
            })
            .or_else(|| {
                context.available_content_height.map(|height| {
                    SideConstraint::at_most((height - edges.margin.vertical()).max(0.0))
                })
            })
            .unwrap_or_else(SideConstraint::indefinite);

        Constraints::new(width, height)
    }

    fn relative_fit_content_axis_constraint(
        &self,
        length: Length,
        percent_base: Option<f32>,
        available: Option<f32>,
    ) -> Option<SideConstraint> {
        let Length::FitContent(base) = length else {
            return None;
        };
        let owner_constraint = percent_base
            .map(SideConstraint::definite)
            .or_else(|| available.map(SideConstraint::at_most))
            .unwrap_or_else(SideConstraint::indefinite);
        Some(self.fit_content_owner_constraint(base, owner_constraint))
    }

    fn relative_child_style_override(
        &self,
        style: &Style,
        constraints: Constraints,
        edges: ResolvedEdges,
        min_max_percent_constraints: Constraints,
        definite_constraint_override: RelativeDefiniteConstraintOverride,
    ) -> Option<Style> {
        let mut style_override = self
            .percent_resolved_style_override(style, constraints, edges)
            .unwrap_or_else(|| style.clone());
        let mut changed = style_override != *style;
        changed |= self.override_min_max_percent_lengths(
            style,
            min_max_percent_constraints,
            &mut style_override,
        );
        if definite_constraint_override != RelativeDefiniteConstraintOverride::None {
            changed |= self.override_relative_definite_constraints(
                constraints,
                edges,
                &mut style_override,
                definite_constraint_override,
            );
        }
        changed.then_some(style_override)
    }

    fn override_relative_definite_constraints(
        &self,
        constraints: Constraints,
        edges: ResolvedEdges,
        style_override: &mut Style,
        mode: RelativeDefiniteConstraintOverride,
    ) -> bool {
        let mut changed = false;
        if constraints.width.is_definite() {
            let width = if mode == RelativeDefiniteConstraintOverride::StyleSize {
                Length::points(self.css_axis_size_from_border_size(
                    style_override,
                    Axis::Horizontal,
                    constraints.width.size,
                    edges,
                ))
            } else {
                Length::Auto
            };
            if style_override.width != width {
                style_override.width = width;
                changed = true;
            }
        }
        if constraints.height.is_definite() {
            let height = if mode == RelativeDefiniteConstraintOverride::StyleSize {
                Length::points(self.css_axis_size_from_border_size(
                    style_override,
                    Axis::Vertical,
                    constraints.height.size,
                    edges,
                ))
            } else {
                Length::Auto
            };
            if style_override.height != height {
                style_override.height = height;
                changed = true;
            }
        }
        changed
    }

    fn relative_percent_constraints(&self, context: RelativeConstraintContext) -> Constraints {
        Constraints::new(
            self.axis_constraint_from_optional(context.child_percent_base_width),
            self.axis_constraint_from_optional(context.child_percent_base_height),
        )
    }

    fn remeasure_relative_constrained_items<T: LayoutTree>(
        &mut self,
        tree: &mut T,
        items: &mut [RelativeItem<T::NodeId>],
        definite_content_width: Option<f32>,
        definite_content_height: Option<f32>,
    ) {
        let constraints = items
            .iter()
            .enumerate()
            .map(|(idx, item)| {
                let width = self
                    .relative_axis_constraint_from_positions(
                        items,
                        idx,
                        true,
                        definite_content_width,
                        item.constraints.width,
                    )
                    .unwrap_or_else(|| self.relative_width_constraint_from_proposed_size(item));
                let height = self
                    .relative_axis_constraint_from_positions(
                        items,
                        idx,
                        false,
                        definite_content_height,
                        item.constraints.height,
                    )
                    .unwrap_or(item.constraints.height);
                Constraints::new(width, height)
            })
            .collect::<Vec<_>>();

        for (item, constraints) in items.iter_mut().zip(constraints) {
            if item.constraints == constraints {
                continue;
            }
            let min_max_percent_constraints = Constraints::new(
                self.axis_constraint_from_optional(definite_content_width),
                self.axis_constraint_from_optional(definite_content_height),
            );
            let style_override = self.relative_child_style_override(
                &item.style,
                constraints,
                item.edges,
                min_max_percent_constraints,
                relative_definite_constraint_override_for_item(tree, item.id),
            );
            let measured = self.layout_node_with_style_override_and_sticky_constraints(
                tree,
                item.id,
                style_override.clone(),
                constraints,
                Point::ZERO,
                constraints,
            );
            item.style_override = style_override;
            item.constraints = constraints;
            item.size = measured.size;
        }
    }

    fn remeasure_relative_horizontal_proposed_items<T: LayoutTree>(
        &mut self,
        tree: &mut T,
        items: &mut [RelativeItem<T::NodeId>],
        content_width: Option<f32>,
        content_height: Option<f32>,
    ) {
        let constraints = items
            .iter()
            .map(|item| {
                Constraints::new(
                    self.relative_width_constraint_from_proposed_size(item),
                    item.constraints.height,
                )
            })
            .collect::<Vec<_>>();

        for (item, constraints) in items.iter_mut().zip(constraints) {
            if item.constraints == constraints {
                continue;
            }
            let min_max_percent_constraints = Constraints::new(
                self.axis_constraint_from_optional(content_width),
                self.axis_constraint_from_optional(content_height),
            );
            let style_override = self.relative_child_style_override(
                &item.style,
                constraints,
                item.edges,
                min_max_percent_constraints,
                relative_definite_constraint_override_for_item(tree, item.id),
            );
            let measured = self.layout_node_with_style_override_and_sticky_constraints(
                tree,
                item.id,
                style_override.clone(),
                constraints,
                Point::ZERO,
                constraints,
            );
            item.style_override = style_override;
            item.constraints = constraints;
            item.size = measured.size;
        }
    }

    fn relative_width_constraint_from_proposed_size<N>(
        &self,
        item: &RelativeItem<N>,
    ) -> SideConstraint {
        let outer_size = item.right - item.left;
        let margin = item.edges.margin.horizontal();
        SideConstraint::definite((outer_size - margin).max(0.0))
    }

    fn measure_and_position_relative_once<T: LayoutTree>(
        &mut self,
        tree: &mut T,
        items: &mut [RelativeItem<T::NodeId>],
        definite_content_width: Option<f32>,
        definite_content_height: Option<f32>,
    ) {
        let order = self.relative_order_for_scope(items, RelativeDependencyScope::Both);
        let mut horizontal_bounds = RelativeAxisBounds::new(definite_content_width);
        let mut vertical_bounds = RelativeAxisBounds::new(definite_content_height);

        for idx in order {
            let width = self
                .relative_axis_constraint_from_positions(
                    items,
                    idx,
                    true,
                    definite_content_width,
                    items[idx].constraints.width,
                )
                .unwrap_or(items[idx].constraints.width);
            let height = self
                .relative_axis_constraint_from_positions(
                    items,
                    idx,
                    false,
                    definite_content_height,
                    items[idx].constraints.height,
                )
                .unwrap_or(items[idx].constraints.height);
            let constraints = Constraints::new(width, height);
            let style_override = self.relative_child_style_override(
                &items[idx].style,
                constraints,
                items[idx].edges,
                Constraints::new(
                    self.axis_constraint_from_optional(definite_content_width),
                    self.axis_constraint_from_optional(definite_content_height),
                ),
                relative_definite_constraint_override_for_item(tree, items[idx].id),
            );
            let measured = self.layout_node_with_style_override_and_sticky_constraints(
                tree,
                items[idx].id,
                style_override.clone(),
                constraints,
                Point::ZERO,
                constraints,
            );
            items[idx].style_override = style_override;
            items[idx].constraints = constraints;
            items[idx].size = measured.size;

            let (left, right) = self.relative_axis_position(
                items,
                idx,
                true,
                horizontal_bounds,
                definite_content_width,
            );
            items[idx].left = left;
            items[idx].right = right;
            if definite_content_width.is_none() {
                horizontal_bounds.include(left, right);
            }

            let (top, bottom) = self.relative_axis_position(
                items,
                idx,
                false,
                vertical_bounds,
                definite_content_height,
            );
            items[idx].top = top;
            items[idx].bottom = bottom;
            if definite_content_height.is_none() {
                vertical_bounds.include(top, bottom);
            }
        }
    }

    fn recompute_relative_final_positions<N>(
        &self,
        items: &mut [RelativeItem<N>],
        content_width: f32,
        content_height: f32,
        combined_order: bool,
    ) {
        if combined_order {
            let order = self.relative_order_for_scope(items, RelativeDependencyScope::Both);
            self.recompute_relative_axis_positions(items, &order, true, content_width);
            self.recompute_relative_axis_positions(items, &order, false, content_height);
        } else {
            let horizontal_order = self.relative_order(items, true);
            self.recompute_relative_axis_positions(items, &horizontal_order, true, content_width);
            let vertical_order = self.relative_order(items, false);
            self.recompute_relative_axis_positions(items, &vertical_order, false, content_height);
        }
    }

    fn recompute_relative_axis_positions<N>(
        &self,
        items: &mut [RelativeItem<N>],
        order: &[usize],
        horizontal: bool,
        extent: f32,
    ) {
        let bounds = RelativeAxisBounds::new(Some(extent));
        for &idx in order {
            let (start, end) =
                self.relative_axis_position(items, idx, horizontal, bounds, Some(extent));
            if horizontal {
                items[idx].left = start;
                items[idx].right = end;
            } else {
                items[idx].top = start;
                items[idx].bottom = end;
            }
        }
    }

    fn relative_axis_constraint_from_positions<N>(
        &self,
        items: &[RelativeItem<N>],
        idx: usize,
        horizontal: bool,
        definite_extent: Option<f32>,
        fallback: SideConstraint,
    ) -> Option<SideConstraint> {
        let start = self.relative_start_constraint(items, idx, horizontal, definite_extent);
        let end = self.relative_end_constraint(items, idx, horizontal, definite_extent);
        let margin = self.axis_margin(items[idx].edges.margin, horizontal);
        match (start, end) {
            (Some(start), Some(end)) => {
                Some(SideConstraint::definite((end - start - margin).max(0.0)))
            }
            (Some(start), None) if fallback.is_at_most() => {
                Some(SideConstraint::at_most((fallback.size - start).max(0.0)))
            }
            (None, Some(end)) if fallback.is_at_most() => {
                Some(SideConstraint::at_most(end.max(0.0)))
            }
            _ => None,
        }
    }

    fn position_relative_items<N>(
        &self,
        items: &mut [RelativeItem<N>],
        horizontal: bool,
        definite_extent: Option<f32>,
    ) {
        let order = self.relative_order(items, horizontal);
        let mut bounds = RelativeAxisBounds::new(definite_extent);
        for idx in order {
            let (start, end) =
                self.relative_axis_position(items, idx, horizontal, bounds, definite_extent);
            if horizontal {
                items[idx].left = start;
                items[idx].right = end;
            } else {
                items[idx].top = start;
                items[idx].bottom = end;
            }
            if definite_extent.is_none() {
                bounds.include(start, end);
            }
        }
    }

    fn relative_axis_position<N>(
        &self,
        items: &[RelativeItem<N>],
        idx: usize,
        horizontal: bool,
        bounds: RelativeAxisBounds,
        definite_extent: Option<f32>,
    ) -> (f32, f32) {
        let item = &items[idx];
        let start = self.relative_start_constraint(items, idx, horizontal, definite_extent);
        let end = self.relative_end_constraint(items, idx, horizontal, definite_extent);
        let outer_size = if horizontal {
            item.size.width + item.edges.margin.horizontal()
        } else {
            item.size.height + item.edges.margin.vertical()
        };

        match (start, end) {
            (Some(start), Some(end)) => (start, end.max(start)),
            (Some(start), None) => (start, start + outer_size),
            (None, Some(end)) => (end - outer_size, end),
            (None, None) => {
                let align_end_parent =
                    self.relative_align_end(&item.style, horizontal) == RELATIVE_ALIGN_PARENT;
                let align_start_parent =
                    self.relative_align_start(&item.style, horizontal) == RELATIVE_ALIGN_PARENT;
                let centered = if horizontal {
                    item.style.relative_center.is_horizontal()
                } else {
                    item.style.relative_center.is_vertical()
                };

                if align_end_parent {
                    let end = bounds.max;
                    (end - outer_size, end)
                } else if align_start_parent || !centered {
                    let start = bounds.min;
                    (start, start + outer_size)
                } else {
                    let start = bounds.min + (bounds.max - bounds.min - outer_size) / 2.0;
                    (start, start + outer_size)
                }
            }
        }
    }

    fn relative_start_constraint<N>(
        &self,
        items: &[RelativeItem<N>],
        idx: usize,
        horizontal: bool,
        definite_extent: Option<f32>,
    ) -> Option<f32> {
        let style = &items[idx].style;
        let align_start = self.relative_align_start(style, horizontal);
        if align_start != RELATIVE_ALIGN_NONE {
            if let Some(position) = self.relative_reference_position(
                items,
                align_start,
                horizontal,
                true,
                definite_extent,
            ) {
                return Some(position);
            }
        }

        let start_after = if horizontal {
            style.relative_right_of
        } else {
            style.relative_bottom_of
        };
        self.relative_reference_position(items, start_after, horizontal, false, definite_extent)
    }

    fn relative_end_constraint<N>(
        &self,
        items: &[RelativeItem<N>],
        idx: usize,
        horizontal: bool,
        definite_extent: Option<f32>,
    ) -> Option<f32> {
        let style = &items[idx].style;
        let align_end = self.relative_align_end(style, horizontal);
        if align_end != RELATIVE_ALIGN_NONE {
            if let Some(position) = self.relative_reference_position(
                items,
                align_end,
                horizontal,
                false,
                definite_extent,
            ) {
                return Some(position);
            }
        }

        let end_before = if horizontal {
            style.relative_left_of
        } else {
            style.relative_top_of
        };
        self.relative_reference_position(items, end_before, horizontal, true, definite_extent)
    }

    fn relative_reference_position<N>(
        &self,
        items: &[RelativeItem<N>],
        reference_id: i32,
        horizontal: bool,
        start_side: bool,
        definite_extent: Option<f32>,
    ) -> Option<f32> {
        match reference_id {
            RELATIVE_ALIGN_NONE => None,
            RELATIVE_ALIGN_PARENT => {
                if start_side {
                    definite_extent.map(|_| 0.0)
                } else {
                    definite_extent
                }
            }
            _ => self
                .relative_id_index(items, reference_id)
                .map(|idx| self.relative_item_side(&items[idx], horizontal, start_side)),
        }
    }

    fn relative_item_side<N>(
        &self,
        item: &RelativeItem<N>,
        horizontal: bool,
        start_side: bool,
    ) -> f32 {
        match (horizontal, start_side) {
            (true, true) => item.left,
            (true, false) => item.right,
            (false, true) => item.top,
            (false, false) => item.bottom,
        }
    }

    fn relative_order<N>(&self, items: &[RelativeItem<N>], horizontal: bool) -> Vec<usize> {
        let scope = if horizontal {
            RelativeDependencyScope::Horizontal
        } else {
            RelativeDependencyScope::Vertical
        };
        self.relative_order_for_scope(items, scope)
    }

    fn relative_order_for_scope<N>(
        &self,
        items: &[RelativeItem<N>],
        scope: RelativeDependencyScope,
    ) -> Vec<usize> {
        let mut unsorted = vec![true; items.len()];
        let mut dependencies = vec![BTreeSet::new(); items.len()];
        let mut reverse_dependencies = vec![BTreeSet::new(); items.len()];
        let mut order = Vec::with_capacity(items.len());

        for idx in 0..items.len() {
            for id in self.relative_dependency_ids(&items[idx].style, scope) {
                if let Some(dependency_idx) = self.relative_id_index(items, id) {
                    dependencies[idx].insert(dependency_idx);
                    reverse_dependencies[dependency_idx].insert(idx);
                }
            }
        }

        let mut current_start = dependencies
            .iter()
            .enumerate()
            .filter_map(|(idx, deps)| deps.is_empty().then_some(idx))
            .collect::<Vec<_>>();
        let mut current_start_idx = 0;

        for _ in 0..items.len() {
            let current = loop {
                if let Some(&candidate) = current_start.get(current_start_idx) {
                    current_start_idx += 1;
                    break candidate;
                }

                break unsorted
                    .iter()
                    .position(|is_unsorted| *is_unsorted)
                    .expect("relative order should have an unsorted item");
            };

            unsorted[current] = false;
            order.push(current);

            for dependent in &reverse_dependencies[current] {
                dependencies[*dependent].remove(&current);
                if dependencies[*dependent].is_empty() && unsorted[*dependent] {
                    current_start.push(*dependent);
                }
            }
        }

        order
    }

    fn relative_dependency_ids(&self, style: &Style, scope: RelativeDependencyScope) -> [i32; 8] {
        match scope {
            RelativeDependencyScope::Horizontal => [
                style.relative_right_of,
                style.relative_left_of,
                style.relative_align_left,
                style.relative_align_right,
                RELATIVE_ALIGN_NONE,
                RELATIVE_ALIGN_NONE,
                RELATIVE_ALIGN_NONE,
                RELATIVE_ALIGN_NONE,
            ],
            RelativeDependencyScope::Vertical => [
                RELATIVE_ALIGN_NONE,
                RELATIVE_ALIGN_NONE,
                RELATIVE_ALIGN_NONE,
                RELATIVE_ALIGN_NONE,
                style.relative_top_of,
                style.relative_bottom_of,
                style.relative_align_top,
                style.relative_align_bottom,
            ],
            RelativeDependencyScope::Both => [
                style.relative_right_of,
                style.relative_left_of,
                style.relative_align_left,
                style.relative_align_right,
                style.relative_top_of,
                style.relative_bottom_of,
                style.relative_align_top,
                style.relative_align_bottom,
            ],
        }
    }

    fn relative_id_index<N>(&self, items: &[RelativeItem<N>], reference_id: i32) -> Option<usize> {
        if matches!(reference_id, RELATIVE_ALIGN_NONE | RELATIVE_ALIGN_PARENT) {
            return None;
        }
        items
            .iter()
            .enumerate()
            .rev()
            .find_map(|(idx, item)| (item.style.relative_id == reference_id).then_some(idx))
    }

    fn relative_axis_stretches_to_parent(&self, style: &Style, horizontal: bool) -> bool {
        self.relative_align_start(style, horizontal) == RELATIVE_ALIGN_PARENT
            && self.relative_align_end(style, horizontal) == RELATIVE_ALIGN_PARENT
    }

    fn relative_align_start(&self, style: &Style, horizontal: bool) -> i32 {
        if horizontal {
            style.relative_align_left
        } else {
            style.relative_align_top
        }
    }

    fn relative_align_end(&self, style: &Style, horizontal: bool) -> i32 {
        if horizontal {
            style.relative_align_right
        } else {
            style.relative_align_bottom
        }
    }

    fn relative_content_extent<N>(&self, items: &[RelativeItem<N>], horizontal: bool) -> f32 {
        let mut min_position = 0.0f32;
        let mut max_position = 0.0f32;
        for item in items {
            if horizontal {
                min_position = min_position.min(item.left);
                max_position = max_position.max(item.right);
            } else {
                min_position = min_position.min(item.top);
                max_position = max_position.max(item.bottom);
            }
        }
        (max_position - min_position).max(0.0)
    }

    fn relative_content_height_after_recompute<N: Clone>(&self, items: &[RelativeItem<N>]) -> f32 {
        let mut recomputed = items.to_vec();
        let order = self.relative_order(&recomputed, false);
        let mut bounds = self.relative_vertical_bounds(&recomputed);
        for idx in order {
            let (start, end) = self.relative_axis_position(&recomputed, idx, false, bounds, None);
            recomputed[idx].top = start;
            recomputed[idx].bottom = end;
            bounds.include(start, end);
        }
        (bounds.max - bounds.min).max(0.0)
    }

    fn relative_vertical_bounds<N>(&self, items: &[RelativeItem<N>]) -> RelativeAxisBounds {
        let mut bounds = RelativeAxisBounds::new(None);
        for item in items {
            bounds.include(item.top, item.bottom);
        }
        bounds
    }
}

#[derive(Clone, Debug)]
struct RelativeItem<N> {
    id: N,
    style: Style,
    style_override: Option<Style>,
    edges: ResolvedEdges,
    constraints: Constraints,
    size: Size,
    left: f32,
    right: f32,
    top: f32,
    bottom: f32,
}

#[derive(Clone, Copy, Debug)]
struct RelativeConstraintContext {
    available_content_width: Option<f32>,
    available_content_height: Option<f32>,
    child_percent_base_width: Option<f32>,
    child_percent_base_height: Option<f32>,
    definite_content_width: Option<f32>,
    definite_content_height: Option<f32>,
}

#[derive(Clone, Copy, Debug)]
struct RelativeAxisBounds {
    min: f32,
    max: f32,
}

impl RelativeAxisBounds {
    fn new(definite_extent: Option<f32>) -> Self {
        Self {
            min: 0.0,
            max: definite_extent.unwrap_or(0.0),
        }
    }

    fn include(&mut self, start: f32, end: f32) {
        self.min = self.min.min(start);
        self.max = self.max.max(end);
    }
}

#[derive(Clone, Copy, Debug)]
enum RelativeDependencyScope {
    Horizontal,
    Vertical,
    Both,
}

#[derive(Clone, Copy, Eq, PartialEq)]
enum RelativeDefiniteConstraintOverride {
    None,
    StyleSize,
    MeasureConstraint,
}

fn relative_definite_constraint_override_for_item<T: LayoutTree>(
    tree: &T,
    node: T::NodeId,
) -> RelativeDefiniteConstraintOverride {
    if tree.has_measure(node) {
        RelativeDefiniteConstraintOverride::MeasureConstraint
    } else {
        RelativeDefiniteConstraintOverride::StyleSize
    }
}
