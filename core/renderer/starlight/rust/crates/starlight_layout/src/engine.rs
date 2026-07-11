// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use std::fmt::Debug;

use crate::style::{
    AlignContent, AlignItems, BoxSizing, Display, FlexWrap, JustifyContent, JustifyItems,
    PositionType, Style, Visibility,
};
use crate::style_data::{LinearCrossGravity, LinearLayoutGravity};
use crate::types::{
    Constraints, Edges, LayoutResult, Length, MeasureMode, Point, Rect, SideConstraint, Size,
};

mod flex;
mod grid;
mod linear;
mod relative;

/// Adapter for running Starlight layout over external tree storage.
///
/// The engine only needs stable node ids, child iteration, style lookup,
/// optional content measurement, and a place to write the final layout result.
/// Implement this trait for an existing DOM/shadow/test tree to avoid copying
/// nodes into an engine-owned structure.
pub trait LayoutTree {
    type NodeId: Copy + Eq + Debug;
    type Children<'a>: Iterator<Item = Self::NodeId>
    where
        Self: 'a;

    /// Returns the current children in layout order.
    fn children(&self, node: Self::NodeId) -> Self::Children<'_>;

    /// Returns layout-facing style for `node`.
    fn style(&self, node: Self::NodeId) -> &Style;

    /// Returns whether the node's `direction` style was explicitly assigned.
    ///
    /// Generic external trees can rely on the default and treat their stored
    /// direction as explicit. Rust standalone overrides this so C++ standalone
    /// imports can preserve the public owner-direction inheritance contract for
    /// nodes that still carry the default direction value.
    fn has_explicit_direction_style(&self, _node: Self::NodeId) -> bool {
        true
    }

    /// Stores the computed border-box result for `node`.
    fn set_layout(&mut self, node: Self::NodeId, layout: LayoutResult);

    /// Stores the computed border-box result and the node constraints used to
    /// produce it.
    ///
    /// Embedders that do not need constraint-aware cache synchronization can
    /// rely on the default implementation.
    fn set_layout_with_constraints(
        &mut self,
        node: Self::NodeId,
        _constraints: Constraints,
        layout: LayoutResult,
    ) {
        self.set_layout(node, layout);
    }

    /// Returns the current exported layout for `node` when the tree stores it.
    ///
    /// This is optional because external embedders may be write-only. The
    /// layout engine uses it only to re-export cached subtrees after a parent
    /// offset changes without recomputing child algorithms.
    fn layout(&self, _node: Self::NodeId) -> Option<LayoutResult> {
        None
    }

    /// Measures external leaf content. The returned size is content-box size;
    /// Starlight adds padding and border around it.
    ///
    /// Text shaping and line breaking intentionally live outside this crate.
    /// Embedders should connect their text layout engine through this callback
    /// and expose `baseline` below when text participates in baseline
    /// alignment.
    fn measure(&mut self, _node: Self::NodeId, _constraints: Constraints) -> Option<Size> {
        None
    }

    /// Measures a node's min-content contribution as content-box size.
    ///
    /// External text engines should use this to expose their narrowest
    /// unoverflowing inline contribution without forcing Starlight to know how
    /// text is shaped or line-broken. The default preserves existing adapters:
    /// when this returns `None`, layout algorithms fall back to their current
    /// style-derived contribution rules.
    fn measure_min_content(
        &mut self,
        _node: Self::NodeId,
        _constraints: Constraints,
    ) -> Option<Size> {
        None
    }

    /// Measures a node's max-content contribution as content-box size.
    ///
    /// This is separate from `measure` because an embedder may need different
    /// text-layout modes for regular layout, min-content, and max-content
    /// contributions.
    fn measure_max_content(
        &mut self,
        _node: Self::NodeId,
        _constraints: Constraints,
    ) -> Option<Size> {
        None
    }

    /// Returns whether `measure` can produce content size for this node.
    ///
    /// The default keeps external tree adapters source-compatible. Native
    /// baseline adapters use this to avoid marking container nodes as custom
    /// measured leaves.
    fn has_measure(&self, _node: Self::NodeId) -> bool {
        false
    }

    /// Returns the physical pixel ratio used for Starlight's public layout
    /// result rounding. External trees that do not expose standalone config
    /// can rely on the C++ standalone default of `1.0`.
    fn physical_pixels_per_layout_unit(&self, _node: Self::NodeId) -> f32 {
        1.0
    }

    /// Returns an optional content-box baseline offset for externally measured
    /// content.
    ///
    /// The returned value is measured from the content-box top edge. When this
    /// is `None`, Starlight falls back to the border-box bottom edge, matching
    /// the C++ default baseline behavior.
    fn baseline(&self, _node: Self::NodeId, _content_size: Size) -> Option<f32> {
        None
    }
}

#[derive(Clone, Debug)]
pub struct LayoutEngine {
    epsilon: f32,
}

impl LayoutEngine {
    const STICKY_AUTO_INSET: f32 = -1e10;
    const LAYOUT_SETTER_ZERO_EPSILON: f32 = 0.01;
    const BOUND_INTEGER_EPSILON: f32 = 0.00001;

    pub fn new() -> Self {
        Self { epsilon: 0.0001 }
    }

    pub fn layout<T: LayoutTree>(
        &mut self,
        tree: &mut T,
        root: T::NodeId,
        constraints: Constraints,
    ) -> Size {
        let root_style = tree.style(root).clone();
        let root_edges = self.resolve_edges(&root_style, constraints);
        self.layout_root_with_edges(tree, root, constraints, root_edges)
    }

    /// Runs layout from C++ Starlight-style owner constraints.
    ///
    /// The standalone C API receives viewport/owner constraints first, then
    /// derives root node constraints from the root style. Keep `layout` for
    /// embedders that already have node constraints and use this entry point
    /// when comparing against the imported C++ standalone baseline.
    pub fn layout_with_owner_constraints<T: LayoutTree>(
        &mut self,
        tree: &mut T,
        root: T::NodeId,
        owner_constraints: Constraints,
    ) -> Size {
        let root_style = tree.style(root).clone();
        let root_edges = self.resolve_edges(&root_style, owner_constraints);
        let mut preferred_width = self.resolve_border_axis(
            root_style.width,
            Axis::Horizontal,
            owner_constraints,
            root_edges,
            root_style.box_sizing,
        );
        let mut preferred_height = self.resolve_border_axis(
            root_style.height,
            Axis::Vertical,
            owner_constraints,
            root_edges,
            root_style.box_sizing,
        );
        if !root_style.width.is_intrinsic() && !root_style.height.is_intrinsic() {
            self.apply_aspect_ratio_to_optional(
                &root_style,
                root_edges,
                &mut preferred_width,
                &mut preferred_height,
            );
        }
        let constraints = self.root_constraints_from_owner(&root_style, owner_constraints);
        let mut layout_root_style = root_style.clone();
        if preferred_width.is_some() && Self::length_needs_percent_override(root_style.width) {
            layout_root_style.width = Length::points(self.css_axis_size_from_border_size(
                &root_style,
                Axis::Horizontal,
                constraints.width.size,
                root_edges,
            ));
        }
        if preferred_height.is_some() && Self::length_needs_percent_override(root_style.height) {
            layout_root_style.height = Length::points(self.css_axis_size_from_border_size(
                &root_style,
                Axis::Vertical,
                constraints.height.size,
                root_edges,
            ));
        }
        self.override_root_min_max_owner_lengths(
            &root_style,
            owner_constraints,
            &mut layout_root_style,
        );
        self.layout_root_with_style_override(
            tree,
            root,
            constraints,
            root_edges,
            Some(layout_root_style),
        )
    }

    fn layout_root_with_edges<T: LayoutTree>(
        &mut self,
        tree: &mut T,
        root: T::NodeId,
        constraints: Constraints,
        root_edges: ResolvedEdges,
    ) -> Size {
        self.layout_root_with_style_override(tree, root, constraints, root_edges, None)
    }

    fn layout_root_with_style_override<T: LayoutTree>(
        &mut self,
        tree: &mut T,
        root: T::NodeId,
        constraints: Constraints,
        root_edges: ResolvedEdges,
        root_style_override: Option<Style>,
    ) -> Size {
        let mut root_box = self.layout_node_with_edges(
            tree,
            root,
            root_style_override,
            NodeLayoutContext {
                constraints,
                offset: Point::ZERO,
                sticky_constraints: constraints,
                edges: root_edges,
                rounding: RoundingContext::root(),
                flex: FlexNodeContext::default(),
            },
        );
        if tree.style(root).position == PositionType::Sticky {
            let mut raw_layout = root_box.layout;
            raw_layout.sticky_pos = Edges::default();
            let exported_layout = self.exported_layout_result(
                raw_layout,
                RoundingContext::root(),
                tree.physical_pixels_per_layout_unit(root),
            );
            tree.set_layout_with_constraints(root, constraints, exported_layout);
            root_box = LayoutBox::from_raw_and_exported(raw_layout, exported_layout, constraints);
        }
        self.layout_fixed_descendants(tree, root, root_box.size, root_edges);
        root_box.layout.size
    }

    fn root_constraints_from_owner(
        &self,
        style: &Style,
        owner_constraints: Constraints,
    ) -> Constraints {
        let owner_edges = self.resolve_edges(style, owner_constraints);
        let mut width = self.resolve_border_axis(
            style.width,
            Axis::Horizontal,
            owner_constraints,
            owner_edges,
            style.box_sizing,
        );
        let mut height = self.resolve_border_axis(
            style.height,
            Axis::Vertical,
            owner_constraints,
            owner_edges,
            style.box_sizing,
        );

        if !style.width.is_intrinsic() && !style.height.is_intrinsic() {
            self.apply_aspect_ratio_to_optional(style, owner_edges, &mut width, &mut height);
        }

        let mut constraints = Constraints::new(
            self.root_axis_constraint_from_owner(
                style,
                Axis::Horizontal,
                width,
                owner_constraints.width,
                owner_edges,
            ),
            self.root_axis_constraint_from_owner(
                style,
                Axis::Vertical,
                height,
                owner_constraints.height,
                owner_edges,
            ),
        );

        constraints.width = self.apply_root_min_max_to_constraint(
            style,
            Axis::Horizontal,
            constraints.width,
            owner_constraints,
            owner_edges,
        );
        constraints.height = self.apply_root_min_max_to_constraint(
            style,
            Axis::Vertical,
            constraints.height,
            owner_constraints,
            owner_edges,
        );
        constraints
    }

    fn root_axis_constraint_from_owner(
        &self,
        style: &Style,
        axis: Axis,
        preferred_size: Option<f32>,
        owner_constraint: SideConstraint,
        owner_edges: ResolvedEdges,
    ) -> SideConstraint {
        let length = match axis {
            Axis::Horizontal => style.width,
            Axis::Vertical => style.height,
        };
        if matches!(length, Length::MinContent | Length::MaxContent) {
            return SideConstraint::indefinite();
        }
        if let Length::FitContent(base) = length {
            return self.fit_content_owner_constraint(base, owner_constraint);
        }
        if let Some(size) = preferred_size {
            return SideConstraint::definite(size);
        }
        owner_constraint
            .bounded_size()
            .map_or_else(SideConstraint::indefinite, |size| {
                let margin = self.axis_margin(owner_edges.margin, axis.is_horizontal());
                SideConstraint::at_most((size - margin).max(0.0))
            })
    }

    fn length_needs_percent_override(length: Length) -> bool {
        match length {
            Length::Percent(_) => true,
            Length::Calc { percent, .. } => percent != 0.0,
            Length::Auto
            | Length::Points(_)
            | Length::Fr(_)
            | Length::MinContent
            | Length::MaxContent
            | Length::FitContent(_) => false,
        }
    }

    fn min_max_needs_owner_override(length: Length) -> bool {
        match length {
            Length::Percent(_) | Length::FitContent(Some(_)) => true,
            Length::Calc { percent, .. } => percent != 0.0,
            Length::Auto
            | Length::Points(_)
            | Length::Fr(_)
            | Length::MinContent
            | Length::MaxContent
            | Length::FitContent(None) => false,
        }
    }

    fn override_root_min_max_owner_lengths(
        &self,
        source: &Style,
        owner_constraints: Constraints,
        target: &mut Style,
    ) {
        self.override_min_max_percent_lengths(source, owner_constraints, target);
    }

    fn override_min_max_percent_lengths(
        &self,
        source: &Style,
        percent_constraints: Constraints,
        target: &mut Style,
    ) -> bool {
        let mut changed = false;
        if Self::min_max_needs_owner_override(source.min_width) {
            if let Some(value) = self
                .resolve_min_max_length(source.min_width, percent_constraints.width.percent_base())
            {
                target.min_width = Length::points(value);
                changed = true;
            }
        }
        if Self::min_max_needs_owner_override(source.max_width) {
            if let Some(value) = self
                .resolve_min_max_length(source.max_width, percent_constraints.width.percent_base())
            {
                target.max_width = Length::points(value);
                changed = true;
            }
        }
        if Self::min_max_needs_owner_override(source.min_height) {
            if let Some(value) = self.resolve_min_max_length(
                source.min_height,
                percent_constraints.height.percent_base(),
            ) {
                target.min_height = Length::points(value);
                changed = true;
            }
        }
        if Self::min_max_needs_owner_override(source.max_height) {
            if let Some(value) = self.resolve_min_max_length(
                source.max_height,
                percent_constraints.height.percent_base(),
            ) {
                target.max_height = Length::points(value);
                changed = true;
            }
        }
        changed
    }

    fn fit_content_owner_constraint(
        &self,
        base: Option<crate::BaseLength>,
        owner_constraint: SideConstraint,
    ) -> SideConstraint {
        match base {
            None => owner_constraint
                .bounded_size()
                .map_or_else(SideConstraint::indefinite, SideConstraint::at_most),
            Some(base) => SideConstraint::at_most(
                self.resolve_base_length(base, owner_constraint.percent_base())
                    .unwrap_or(0.0)
                    .max(0.0),
            ),
        }
    }

    fn apply_root_min_max_to_constraint(
        &self,
        style: &Style,
        axis: Axis,
        constraint: SideConstraint,
        owner_constraints: Constraints,
        owner_edges: ResolvedEdges,
    ) -> SideConstraint {
        let percent_base = match axis {
            Axis::Horizontal => owner_constraints.width.percent_base(),
            Axis::Vertical => owner_constraints.height.percent_base(),
        };
        let min = self.root_min_max_border_size(
            style,
            axis,
            match axis {
                Axis::Horizontal => style.min_width,
                Axis::Vertical => style.min_height,
            },
            percent_base,
            owner_edges,
        );
        let max = self.root_min_max_border_size(
            style,
            axis,
            match axis {
                Axis::Horizontal => style.max_width,
                Axis::Vertical => style.max_height,
            },
            percent_base,
            owner_edges,
        );

        match constraint.mode {
            MeasureMode::Indefinite => max.map_or(constraint, SideConstraint::at_most),
            MeasureMode::Definite | MeasureMode::AtMost => {
                let mut size = constraint.size;
                if let Some(min) = min {
                    size = size.max(min);
                }
                if let Some(max) = max {
                    size = size.min(max);
                }
                size = size.max(0.0);
                SideConstraint {
                    size,
                    mode: constraint.mode,
                }
            }
        }
    }

    fn root_min_max_border_size(
        &self,
        style: &Style,
        axis: Axis,
        length: Length,
        percent_base: Option<f32>,
        owner_edges: ResolvedEdges,
    ) -> Option<f32> {
        self.resolve_min_max_length(length, percent_base)
            .map(|size| match style.box_sizing {
                BoxSizing::BorderBox => size,
                BoxSizing::ContentBox => size + self.padding_border(axis, owner_edges),
            })
    }

    fn resolve_base_length(
        &self,
        base: crate::BaseLength,
        percent_base: Option<f32>,
    ) -> Option<f32> {
        if !base.has_value() {
            return None;
        }
        if base.contains_percentage() {
            percent_base.map(|value| base.fixed_part() + value * (base.percentage_part() / 100.0))
        } else {
            Some(base.fixed_part())
        }
    }

    fn resolve_numeric_length(&self, length: Length, percent_base: Option<f32>) -> Option<f32> {
        match length {
            Length::FitContent(Some(base)) => self.resolve_base_length(base, percent_base),
            Length::Fr(value) => Some(value),
            _ => length.resolve(percent_base),
        }
    }

    fn resolve_edge_length(&self, length: Length, percent_base: Option<f32>) -> Option<f32> {
        match length {
            Length::FitContent(Some(base)) => self.resolve_base_length(base, percent_base),
            Length::Fr(value) => Some(value),
            _ => length.resolve(percent_base),
        }
    }

    fn resolve_axis_length(&self, length: Length, percent_base: Option<f32>) -> Option<f32> {
        match length {
            Length::Fr(value) => Some(value),
            _ => length.resolve(percent_base),
        }
    }

    fn layout_node<T: LayoutTree>(
        &mut self,
        tree: &mut T,
        node: T::NodeId,
        constraints: Constraints,
        offset: Point,
    ) -> LayoutBox {
        self.layout_node_with_sticky_constraints(tree, node, constraints, offset, constraints)
    }

    fn layout_node_with_sticky_constraints<T: LayoutTree>(
        &mut self,
        tree: &mut T,
        node: T::NodeId,
        constraints: Constraints,
        offset: Point,
        sticky_constraints: Constraints,
    ) -> LayoutBox {
        self.layout_node_with_style_override_and_sticky_constraints(
            tree,
            node,
            None,
            constraints,
            offset,
            sticky_constraints,
        )
    }

    fn layout_node_with_style_override_and_sticky_constraints<T: LayoutTree>(
        &mut self,
        tree: &mut T,
        node: T::NodeId,
        style_override: Option<Style>,
        constraints: Constraints,
        offset: Point,
        sticky_constraints: Constraints,
    ) -> LayoutBox {
        let edges = self.resolve_edges(
            style_override.as_ref().unwrap_or_else(|| tree.style(node)),
            constraints,
        );
        self.layout_node_with_edges(
            tree,
            node,
            style_override,
            NodeLayoutContext {
                constraints,
                offset,
                sticky_constraints,
                edges,
                rounding: RoundingContext::root(),
                flex: FlexNodeContext::default(),
            },
        )
    }

    fn layout_node_with_edges<T: LayoutTree>(
        &mut self,
        tree: &mut T,
        node: T::NodeId,
        style_override: Option<Style>,
        context: NodeLayoutContext,
    ) -> LayoutBox {
        let NodeLayoutContext {
            constraints,
            offset,
            sticky_constraints,
            edges,
            rounding,
            flex,
        } = context;
        let style = style_override.unwrap_or_else(|| tree.style(node).clone());
        let physical_pixels_per_layout_unit = tree.physical_pixels_per_layout_unit(node);
        let children = tree.children(node).collect::<Vec<_>>();
        let final_offset = self.positioned_offset(&style, sticky_constraints, offset);
        let node_absolute = Point::new(
            rounding.container_absolute.x + final_offset.x,
            rounding.container_absolute.y + final_offset.y,
        );
        let child_rounding = RoundingContext {
            container_absolute: node_absolute,
            container_rounded: Point::new(
                Self::round_to_pixel_grid(node_absolute.x, physical_pixels_per_layout_unit),
                Self::round_to_pixel_grid(node_absolute.y, physical_pixels_per_layout_unit),
            ),
        };

        if style.display == Display::None {
            let layout = LayoutResult {
                offset: final_offset,
                border: edges.border,
                ..LayoutResult::default()
            };
            let exported_layout =
                self.exported_layout_result(layout, rounding, physical_pixels_per_layout_unit);
            tree.set_layout_with_constraints(node, constraints, exported_layout);
            self.layout_hidden_descendants(tree, &children, edges, child_rounding);
            return LayoutBox::from_raw_and_exported(layout, exported_layout, constraints);
        }

        let constraints = self.apply_min_max_to_constraints(&style, constraints, edges);
        let constraints = self.apply_aspect_ratio_to_constraints(&style, edges, constraints);
        let layout_style = self.effective_layout_style(&style);
        let output = if tree.has_measure(node) {
            self.layout_measured(tree, node, &style, constraints, edges)
        } else {
            match layout_style.display {
                Display::None => LayoutOutput::new(Size::ZERO),
                Display::Block => unreachable!("effective display never returns block"),
                Display::Flex => self.layout_flex(
                    tree,
                    &layout_style,
                    &children,
                    NodeLayoutContext {
                        constraints,
                        offset: final_offset,
                        sticky_constraints,
                        edges,
                        rounding: child_rounding,
                        flex,
                    },
                ),
                Display::Linear => self.layout_linear(
                    tree,
                    &layout_style,
                    &children,
                    constraints,
                    edges,
                    child_rounding,
                ),
                Display::Relative => self.layout_relative(
                    tree,
                    &layout_style,
                    &children,
                    constraints,
                    edges,
                    child_rounding,
                ),
                Display::Grid => self.layout_grid(
                    tree,
                    &layout_style,
                    &children,
                    constraints,
                    edges,
                    child_rounding,
                ),
            }
        };

        let raw_layout = LayoutResult {
            offset: final_offset,
            size: output.size,
            baseline: output.baseline,
            padding: edges.padding,
            border: edges.border,
            margin: edges.margin,
            sticky_pos: self.sticky_pos(&style, sticky_constraints),
        };
        let exported_layout =
            self.exported_layout_result(raw_layout, rounding, physical_pixels_per_layout_unit);
        tree.set_layout_with_constraints(node, constraints, exported_layout);

        LayoutBox::from_raw_and_exported(raw_layout, exported_layout, constraints)
    }

    fn effective_layout_style(&self, style: &Style) -> Style {
        let mut layout_style = style.clone();
        if layout_style.display == Display::Block {
            layout_style.display = Display::Linear;
        }
        layout_style
    }

    fn layout_measured<T: LayoutTree>(
        &mut self,
        tree: &mut T,
        node: T::NodeId,
        style: &Style,
        constraints: Constraints,
        edges: ResolvedEdges,
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

        let measure_constraints = self.content_constraints_for_measure(
            style,
            fixed_width,
            fixed_height,
            constraints,
            edges,
        );
        let measured = if fixed_width.is_some() && fixed_height.is_some() {
            Size::new(
                measure_constraints.width.size,
                measure_constraints.height.size,
            )
        } else {
            tree.measure(node, measure_constraints)
                .unwrap_or(Size::ZERO)
        };
        let physical_pixels_per_layout_unit = tree.physical_pixels_per_layout_unit(node);

        let mut border_width = if let Some(width) = fixed_width {
            width
        } else if constraints.width.is_definite() {
            constraints.width.size
        } else {
            self.measured_content_axis(
                measured.width,
                measure_constraints.width,
                physical_pixels_per_layout_unit,
            ) + edges.padding.horizontal()
                + edges.border.horizontal()
        };

        let mut border_height = if let Some(height) = fixed_height {
            height
        } else if constraints.height.is_definite() {
            constraints.height.size
        } else {
            self.measured_content_axis(
                measured.height,
                measure_constraints.height,
                physical_pixels_per_layout_unit,
            ) + edges.padding.vertical()
                + edges.border.vertical()
        };

        border_width = self.clamp_measured_axis(
            style,
            Axis::Horizontal,
            border_width,
            constraints.width.percent_base(),
            edges,
        );
        border_height = self.clamp_measured_axis(
            style,
            Axis::Vertical,
            border_height,
            constraints.height.percent_base(),
            edges,
        );

        let size = Size::new(border_width.max(0.0), border_height.max(0.0));
        let baseline = tree.baseline(node, measured);

        LayoutOutput { size, baseline }
    }

    fn measured_content_axis(
        &self,
        measured: f32,
        constraint: SideConstraint,
        physical_pixels_per_layout_unit: f32,
    ) -> f32 {
        if constraint.is_definite() {
            constraint.size
        } else {
            Self::ceil_to_pixel_grid(measured, physical_pixels_per_layout_unit)
        }
    }

    fn layout_display_none_children<T: LayoutTree>(
        &mut self,
        tree: &mut T,
        children: &[T::NodeId],
        parent_edges: ResolvedEdges,
        rounding: RoundingContext,
    ) {
        let offset = Point::new(parent_edges.border.left, parent_edges.border.top);
        for &child in children {
            if tree.style(child).display == Display::None {
                let constraints = Constraints::indefinite();
                let child_edges = self.resolve_edges(tree.style(child), constraints);
                self.layout_node_with_edges(
                    tree,
                    child,
                    None,
                    NodeLayoutContext {
                        constraints,
                        offset,
                        sticky_constraints: constraints,
                        edges: child_edges,
                        rounding,
                        flex: FlexNodeContext::default(),
                    },
                );
            }
        }
    }

    fn layout_hidden_descendants<T: LayoutTree>(
        &mut self,
        tree: &mut T,
        children: &[T::NodeId],
        parent_edges: ResolvedEdges,
        rounding: RoundingContext,
    ) {
        let offset = Point::new(parent_edges.border.left, parent_edges.border.top);
        for &child in children {
            self.layout_hidden_subtree(tree, child, offset, rounding);
        }
    }

    fn layout_hidden_subtree<T: LayoutTree>(
        &mut self,
        tree: &mut T,
        node: T::NodeId,
        offset: Point,
        rounding: RoundingContext,
    ) {
        let children = tree.children(node).collect::<Vec<_>>();
        let edges = self.resolve_edges(tree.style(node), Constraints::indefinite());
        let layout = LayoutResult {
            offset,
            border: edges.border,
            ..LayoutResult::default()
        };
        let physical_pixels_per_layout_unit = tree.physical_pixels_per_layout_unit(node);
        let exported_layout =
            self.exported_layout_result(layout, rounding, physical_pixels_per_layout_unit);
        tree.set_layout_with_constraints(node, Constraints::indefinite(), exported_layout);

        let node_absolute = Point::new(
            rounding.container_absolute.x + offset.x,
            rounding.container_absolute.y + offset.y,
        );
        let child_rounding = RoundingContext {
            container_absolute: node_absolute,
            container_rounded: Point::new(
                Self::round_to_pixel_grid(node_absolute.x, physical_pixels_per_layout_unit),
                Self::round_to_pixel_grid(node_absolute.y, physical_pixels_per_layout_unit),
            ),
        };
        self.layout_hidden_descendants(tree, &children, edges, child_rounding);
    }

    fn ordered_in_flow_children<T: LayoutTree>(
        &self,
        tree: &T,
        children: &[T::NodeId],
    ) -> Vec<T::NodeId> {
        let mut need_order = false;
        let mut in_flow = Vec::new();
        for &child in children {
            let style = tree.style(child);
            if style.display == Display::None || is_out_of_flow(style) {
                continue;
            }
            need_order |= style.order != 0;
            in_flow.push(child);
        }
        if need_order {
            in_flow.sort_by_key(|child| tree.style(*child).order);
        }
        in_flow
    }

    fn layout_out_of_flow_children<T: LayoutTree>(
        &mut self,
        tree: &mut T,
        children: &[T::NodeId],
        container_style: &Style,
        container_size: Size,
        edges: ResolvedEdges,
        rounding: RoundingContext,
    ) {
        let containing_width = (container_size.width - edges.border.horizontal()).max(0.0);
        let containing_height = (container_size.height - edges.border.vertical()).max(0.0);
        let containing_block = Constraints::definite(containing_width, containing_height);
        let context = OutOfFlowContext {
            containing_size: Size::new(containing_width, containing_height),
            container_origin: Point::new(edges.border.left, edges.border.top),
            containing_block,
        };

        for &child in children {
            let child_style = tree.style(child).clone();
            if child_style.display == Display::None
                || child_style.position != PositionType::Absolute
            {
                continue;
            }

            let child_edges = self
                .resolve_edges_for_parent(&child_style, SideConstraint::definite(containing_width));
            let child_constraints = self.out_of_flow_constraints(
                &child_style,
                child_edges,
                containing_block,
                containing_width,
                containing_height,
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
            let offset = self.out_of_flow_offset(
                container_style,
                &child_style,
                child_edges,
                measured.size,
                context,
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

    fn layout_fixed_descendants<T: LayoutTree>(
        &mut self,
        tree: &mut T,
        root: T::NodeId,
        root_size: Size,
        root_edges: ResolvedEdges,
    ) {
        let root_style = tree.style(root).clone();
        if root_style.display == Display::None {
            return;
        }
        let root_layout_style = self.effective_layout_style(&root_style);

        let containing_width = (root_size.width - root_edges.border.horizontal()).max(0.0);
        let containing_height = (root_size.height - root_edges.border.vertical()).max(0.0);
        let context = OutOfFlowContext {
            containing_size: Size::new(containing_width, containing_height),
            container_origin: Point::new(root_edges.border.left, root_edges.border.top),
            containing_block: Constraints::definite(containing_width, containing_height),
        };

        let mut fixed_nodes = Vec::new();
        self.collect_fixed_descendants(tree, root, true, false, &mut fixed_nodes);
        for fixed in fixed_nodes {
            let child_style = tree.style(fixed.node).clone();
            if child_style.display == Display::None {
                self.layout_node(
                    tree,
                    fixed.node,
                    Constraints::indefinite(),
                    context.container_origin,
                );
                continue;
            }

            let child_edges = self
                .resolve_edges_for_parent(&child_style, SideConstraint::definite(containing_width));
            if fixed.hidden_by_display_none_ancestor {
                let offset = self.out_of_flow_offset(
                    &root_layout_style,
                    &child_style,
                    child_edges,
                    Size::ZERO,
                    context,
                );
                self.layout_hidden_subtree(tree, fixed.node, offset, RoundingContext::root());
                continue;
            }

            let child_constraints = self.out_of_flow_constraints(
                &child_style,
                child_edges,
                context.containing_block,
                containing_width,
                containing_height,
            );
            let child_style_override = self.out_of_flow_style_override(
                &child_style,
                child_constraints,
                child_edges,
                context.containing_block,
            );
            let measured = self.layout_node_with_edges(
                tree,
                fixed.node,
                child_style_override.clone(),
                NodeLayoutContext {
                    constraints: child_constraints,
                    offset: Point::ZERO,
                    sticky_constraints: child_constraints,
                    edges: child_edges,
                    rounding: RoundingContext::root(),
                    flex: FlexNodeContext::default(),
                },
            );
            let offset = self.out_of_flow_offset(
                &root_layout_style,
                &child_style,
                child_edges,
                measured.size,
                context,
            );
            self.layout_node_with_edges(
                tree,
                fixed.node,
                child_style_override,
                NodeLayoutContext {
                    constraints: child_constraints,
                    offset,
                    sticky_constraints: child_constraints,
                    edges: child_edges,
                    rounding: RoundingContext::root(),
                    flex: FlexNodeContext::default(),
                },
            );
        }
    }

    fn out_of_flow_style_override(
        &self,
        style: &Style,
        constraints: Constraints,
        edges: ResolvedEdges,
        containing_block: Constraints,
    ) -> Option<Style> {
        let mut style_override = self
            .percent_resolved_style_override(style, constraints, edges)
            .unwrap_or_else(|| style.clone());
        let mut changed = style_override != *style;
        changed |=
            self.override_min_max_percent_lengths(style, containing_block, &mut style_override);

        changed.then_some(style_override)
    }

    fn percent_resolved_style_override(
        &self,
        style: &Style,
        constraints: Constraints,
        edges: ResolvedEdges,
    ) -> Option<Style> {
        let mut style_override = style.clone();
        let mut changed = false;

        if Self::length_needs_percent_override(style.width) && constraints.width.is_definite() {
            style_override.width = Length::points(self.css_axis_size_from_border_size(
                style,
                Axis::Horizontal,
                constraints.width.size,
                edges,
            ));
            changed = true;
        }
        if Self::length_needs_percent_override(style.height) && constraints.height.is_definite() {
            style_override.height = Length::points(self.css_axis_size_from_border_size(
                style,
                Axis::Vertical,
                constraints.height.size,
                edges,
            ));
            changed = true;
        }

        changed.then_some(style_override)
    }

    fn collect_fixed_descendants<T: LayoutTree>(
        &self,
        tree: &T,
        node: T::NodeId,
        node_is_root: bool,
        ancestor_display_none: bool,
        fixed_nodes: &mut Vec<FixedDescendant<T::NodeId>>,
    ) {
        let node_is_grid = tree.style(node).display == Display::Grid;
        for child in tree.children(node) {
            let child_style = tree.style(child);
            let child_hidden_by_display_none =
                ancestor_display_none || child_style.display == Display::None;
            if child_style.position == PositionType::Fixed && (!node_is_grid || !node_is_root) {
                fixed_nodes.push(FixedDescendant {
                    node: child,
                    hidden_by_display_none_ancestor: ancestor_display_none,
                });
            }
            self.collect_fixed_descendants(
                tree,
                child,
                false,
                child_hidden_by_display_none,
                fixed_nodes,
            );
        }
    }

    fn out_of_flow_constraints(
        &self,
        style: &Style,
        edges: ResolvedEdges,
        containing_block: Constraints,
        content_width: f32,
        content_height: f32,
    ) -> Constraints {
        let left = self.resolve_edge_length(style.left, containing_block.width.percent_base());
        let right = self.resolve_edge_length(style.right, containing_block.width.percent_base());
        let top = self.resolve_edge_length(style.top, containing_block.height.percent_base());
        let bottom = self.resolve_edge_length(style.bottom, containing_block.height.percent_base());

        let width = self
            .resolve_border_axis(
                style.width,
                Axis::Horizontal,
                containing_block,
                edges,
                style.box_sizing,
            )
            .map_or_else(
                || {
                    self.out_of_flow_auto_axis_constraint(
                        content_width,
                        edges.margin.horizontal(),
                        left,
                        right,
                    )
                },
                SideConstraint::definite,
            );

        let height = self
            .resolve_border_axis(
                style.height,
                Axis::Vertical,
                containing_block,
                edges,
                style.box_sizing,
            )
            .map_or_else(
                || {
                    self.out_of_flow_auto_axis_constraint(
                        content_height,
                        edges.margin.vertical(),
                        top,
                        bottom,
                    )
                },
                SideConstraint::definite,
            );

        Constraints::new(width, height)
    }

    fn out_of_flow_auto_axis_constraint(
        &self,
        content_size: f32,
        margin_size: f32,
        start_inset: Option<f32>,
        end_inset: Option<f32>,
    ) -> SideConstraint {
        let inset_size = start_inset.unwrap_or(0.0) + end_inset.unwrap_or(0.0);
        let available = content_size - margin_size - inset_size;
        if start_inset.is_some() && end_inset.is_some() {
            SideConstraint::definite(available)
        } else {
            SideConstraint::at_most(available.max(0.0))
        }
    }

    fn out_of_flow_offset(
        &self,
        container_style: &Style,
        style: &Style,
        child_edges: ResolvedEdges,
        child_size: Size,
        context: OutOfFlowContext,
    ) -> Point {
        let containing_block = context.containing_block;
        let left = self.resolve_edge_length(style.left, containing_block.width.percent_base());
        let right = self.resolve_edge_length(style.right, containing_block.width.percent_base());
        let top = self.resolve_edge_length(style.top, containing_block.height.percent_base());
        let bottom = self.resolve_edge_length(style.bottom, containing_block.height.percent_base());

        let alignment = self.out_of_flow_initial_alignment(container_style, style);
        let x = if let Some(left) = left {
            context.container_origin.x + left + child_edges.margin.left
        } else if let Some(right) = right {
            context.container_origin.x + context.containing_size.width
                - right
                - child_edges.margin.right
                - child_size.width
        } else {
            context.container_origin.x
                + self.out_of_flow_initial_axis_offset(
                    context.containing_size.width,
                    child_size.width,
                    child_edges.margin.left,
                    child_edges.margin.right,
                    alignment.horizontal,
                )
        };
        let y = if let Some(top) = top {
            context.container_origin.y + top + child_edges.margin.top
        } else if let Some(bottom) = bottom {
            context.container_origin.y + context.containing_size.height
                - bottom
                - child_edges.margin.bottom
                - child_size.height
        } else {
            context.container_origin.y
                + self.out_of_flow_initial_axis_offset(
                    context.containing_size.height,
                    child_size.height,
                    child_edges.margin.top,
                    child_edges.margin.bottom,
                    alignment.vertical,
                )
        };
        Point::new(x, y)
    }

    fn out_of_flow_initial_alignment(
        &self,
        container_style: &Style,
        child_style: &Style,
    ) -> OutOfFlowAlignment {
        match container_style.display {
            Display::Flex => self.flex_out_of_flow_alignment(container_style, child_style),
            Display::Linear => self.linear_out_of_flow_alignment(container_style, child_style),
            Display::Block | Display::Relative | Display::Grid | Display::None => {
                OutOfFlowAlignment {
                    horizontal: OutOfFlowAxisAlignment::start_with_front(
                        !container_style.direction.is_any_rtl(),
                    ),
                    vertical: OutOfFlowAxisAlignment::start_with_front(true),
                }
            }
        }
    }

    fn out_of_flow_initial_axis_offset(
        &self,
        content_size: f32,
        child_size: f32,
        start_margin: f32,
        end_margin: f32,
        alignment: OutOfFlowAxisAlignment,
    ) -> f32 {
        let free_space = content_size - child_size - start_margin - end_margin;
        let logical_offset = match alignment.position {
            OutOfFlowPosition::Start => 0.0,
            OutOfFlowPosition::Center => free_space / 2.0,
            OutOfFlowPosition::End => free_space,
        };
        let physical_offset = if alignment.front_is_physical_start {
            logical_offset
        } else {
            free_space - logical_offset
        };
        physical_offset + start_margin
    }

    fn default_constraint_style_override(
        &self,
        style: &Style,
        edges: ResolvedEdges,
        constraints: Constraints,
    ) -> Style {
        let mut override_style = style.clone();
        if constraints.width.is_definite() && Self::length_contains_percentage(style.width) {
            self.set_css_axis_size_from_border_size(
                &mut override_style,
                style,
                Axis::Horizontal,
                constraints.width.size,
                edges,
            );
        }
        if constraints.height.is_definite() && Self::length_contains_percentage(style.height) {
            self.set_css_axis_size_from_border_size(
                &mut override_style,
                style,
                Axis::Vertical,
                constraints.height.size,
                edges,
            );
        }
        override_style
    }

    fn length_contains_percentage(length: Length) -> bool {
        match length {
            Length::Percent(_) => true,
            Length::Calc { percent, .. } => percent != 0.0,
            Length::FitContent(Some(base)) => base.contains_percentage(),
            Length::Auto
            | Length::Points(_)
            | Length::Fr(_)
            | Length::MinContent
            | Length::MaxContent
            | Length::FitContent(None) => false,
        }
    }

    fn style_main_axis_length(style: &Style, is_row: bool) -> Length {
        if is_row {
            style.width
        } else {
            style.height
        }
    }

    fn main_axis_size_override(
        &self,
        style: &Style,
        edges: ResolvedEdges,
        target_main: f32,
        is_row: bool,
    ) -> Style {
        let mut style = style.clone();
        if is_row {
            style.width = Length::points(self.css_axis_size_from_border_size(
                &style,
                Axis::Horizontal,
                target_main,
                edges,
            ));
            style.min_width = Length::Auto;
            style.max_width = Length::Auto;
        } else {
            style.height = Length::points(self.css_axis_size_from_border_size(
                &style,
                Axis::Vertical,
                target_main,
                edges,
            ));
            style.min_height = Length::Auto;
            style.max_height = Length::Auto;
        }
        style
    }

    fn child_main_cross_constraints(
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

    fn content_constraints_from_main_cross(
        &self,
        main_size: f32,
        cross_size: f32,
        is_row: bool,
    ) -> Constraints {
        if is_row {
            Constraints::definite(main_size, cross_size)
        } else {
            Constraints::definite(cross_size, main_size)
        }
    }

    fn definite_container_clamp_constraints(&self, constraints: Constraints) -> Constraints {
        Constraints::new(
            if constraints.width.is_definite() {
                constraints.width
            } else {
                SideConstraint::indefinite()
            },
            if constraints.height.is_definite() {
                constraints.height
            } else {
                SideConstraint::indefinite()
            },
        )
    }

    fn child_cross_axis_is_content_keyword(&self, style: &Style, parent_is_row: bool) -> bool {
        if parent_is_row {
            matches!(style.height, Length::MinContent | Length::MaxContent)
        } else {
            matches!(style.width, Length::MinContent | Length::MaxContent)
        }
    }

    fn child_cross_axis_is_fit_content(&self, style: &Style, parent_is_row: bool) -> bool {
        if parent_is_row {
            matches!(style.height, Length::FitContent(_))
        } else {
            matches!(style.width, Length::FitContent(_))
        }
    }

    fn child_cross_axis_is_intrinsic(&self, style: &Style, parent_is_row: bool) -> bool {
        if parent_is_row {
            style.height.is_intrinsic()
        } else {
            style.width.is_intrinsic()
        }
    }

    fn resolve_child_cross_size(
        &self,
        style: &Style,
        parent_is_row: bool,
        content_cross_limit: Option<f32>,
        edges: ResolvedEdges,
    ) -> Option<f32> {
        let axis = if parent_is_row {
            Axis::Vertical
        } else {
            Axis::Horizontal
        };
        self.resolve_child_axis_size(style, axis, content_cross_limit, edges)
    }

    fn resolve_child_main_size(
        &self,
        style: &Style,
        parent_is_row: bool,
        content_main_limit: Option<f32>,
        edges: ResolvedEdges,
    ) -> Option<f32> {
        let axis = if parent_is_row {
            Axis::Horizontal
        } else {
            Axis::Vertical
        };
        self.resolve_child_axis_size(style, axis, content_main_limit, edges)
    }

    fn resolve_child_axis_size(
        &self,
        style: &Style,
        axis: Axis,
        percent_base: Option<f32>,
        edges: ResolvedEdges,
    ) -> Option<f32> {
        let length = match axis {
            Axis::Horizontal => style.width,
            Axis::Vertical => style.height,
        };
        self.resolve_axis_length(length, percent_base)
            .map(|value| match style.box_sizing {
                BoxSizing::BorderBox => value.max(0.0),
                BoxSizing::ContentBox => {
                    value
                        + match axis {
                            Axis::Horizontal => {
                                edges.padding.horizontal() + edges.border.horizontal()
                            }
                            Axis::Vertical => edges.padding.vertical() + edges.border.vertical(),
                        }
                }
            })
    }

    fn content_constraints_for_measure(
        &self,
        style: &Style,
        fixed_width: Option<f32>,
        fixed_height: Option<f32>,
        constraints: Constraints,
        edges: ResolvedEdges,
    ) -> Constraints {
        let mut width = fixed_width
            .map(|width| SideConstraint::definite(self.inner_width(width, edges)))
            .unwrap_or_else(|| {
                self.content_constraint_from_parent(constraints.width, Axis::Horizontal, edges)
            });
        let mut height = fixed_height
            .map(|height| SideConstraint::definite(self.inner_height(height, edges)))
            .unwrap_or_else(|| {
                self.content_constraint_from_parent(constraints.height, Axis::Vertical, edges)
            });
        width = self.apply_min_max_to_content_measure_constraint(
            style,
            Axis::Horizontal,
            width,
            constraints.width.percent_base(),
            edges,
        );
        height = self.apply_min_max_to_content_measure_constraint(
            style,
            Axis::Vertical,
            height,
            constraints.height.percent_base(),
            edges,
        );
        Constraints::new(width, height)
    }

    fn apply_min_max_to_content_measure_constraint(
        &self,
        style: &Style,
        axis: Axis,
        constraint: SideConstraint,
        percent_base: Option<f32>,
        edges: ResolvedEdges,
    ) -> SideConstraint {
        let min_length = match axis {
            Axis::Horizontal => style.min_width,
            Axis::Vertical => style.min_height,
        };
        let max_length = match axis {
            Axis::Horizontal => style.max_width,
            Axis::Vertical => style.max_height,
        };
        let min = self
            .resolve_min_max_length(min_length, percent_base)
            .map(|size| {
                self.inner_axis(
                    self.border_size_from_css_axis(style, axis, size, edges),
                    axis.is_horizontal(),
                    edges,
                )
            });
        let max = self
            .resolve_min_max_length(max_length, percent_base)
            .map(|size| {
                self.inner_axis(
                    self.border_size_from_css_axis(style, axis, size, edges),
                    axis.is_horizontal(),
                    edges,
                )
            });

        match constraint.mode {
            MeasureMode::Indefinite => max
                .map(SideConstraint::at_most)
                .unwrap_or_else(SideConstraint::indefinite),
            MeasureMode::Definite | MeasureMode::AtMost => {
                let mut size = constraint.size;
                if let Some(min) = min {
                    size = size.max(min);
                }
                if let Some(max) = max {
                    size = size.min(max);
                }
                SideConstraint {
                    size: size.max(0.0),
                    mode: constraint.mode,
                }
            }
        }
    }

    fn content_constraint_from_parent(
        &self,
        constraint: SideConstraint,
        axis: Axis,
        edges: ResolvedEdges,
    ) -> SideConstraint {
        let content_size = match axis {
            Axis::Horizontal => self.inner_width(constraint.size, edges),
            Axis::Vertical => self.inner_height(constraint.size, edges),
        };
        match constraint.mode {
            MeasureMode::Indefinite => SideConstraint::indefinite(),
            MeasureMode::Definite => SideConstraint::definite(content_size),
            MeasureMode::AtMost => SideConstraint::at_most(content_size),
        }
    }

    fn apply_aspect_ratio_to_constraints(
        &self,
        style: &Style,
        edges: ResolvedEdges,
        mut constraints: Constraints,
    ) -> Constraints {
        let Some(aspect_ratio) = self.valid_aspect_ratio(style) else {
            return constraints;
        };

        if constraints.width.is_definite() && !constraints.height.is_definite() {
            constraints.height = SideConstraint::definite(self.aspect_height_from_width(
                style,
                edges,
                constraints.width.size,
                aspect_ratio,
            ));
        } else if constraints.height.is_definite() && !constraints.width.is_definite() {
            constraints.width = SideConstraint::definite(self.aspect_width_from_height(
                style,
                edges,
                constraints.height.size,
                aspect_ratio,
            ));
        }
        constraints
    }

    fn apply_aspect_ratio_to_optional(
        &self,
        style: &Style,
        edges: ResolvedEdges,
        width: &mut Option<f32>,
        height: &mut Option<f32>,
    ) {
        let Some(aspect_ratio) = self.valid_aspect_ratio(style) else {
            return;
        };

        match (*width, *height) {
            (Some(width_value), None) => {
                *height =
                    Some(self.aspect_height_from_width(style, edges, width_value, aspect_ratio));
            }
            (None, Some(height_value)) => {
                *width =
                    Some(self.aspect_width_from_height(style, edges, height_value, aspect_ratio));
            }
            _ => {}
        }
    }

    fn apply_aspect_ratio_to_main_cross(
        &self,
        style: &Style,
        edges: ResolvedEdges,
        main: Option<f32>,
        cross: Option<f32>,
        is_row: bool,
    ) -> (Option<f32>, Option<f32>) {
        let (mut width, mut height) = if is_row { (main, cross) } else { (cross, main) };
        self.apply_aspect_ratio_to_optional(style, edges, &mut width, &mut height);
        if is_row {
            (width, height)
        } else {
            (height, width)
        }
    }

    fn aspect_height_from_width(
        &self,
        style: &Style,
        edges: ResolvedEdges,
        border_width: f32,
        aspect_ratio: f32,
    ) -> f32 {
        match style.box_sizing {
            BoxSizing::BorderBox => border_width / aspect_ratio,
            BoxSizing::ContentBox => {
                let content_width = self.inner_width(border_width, edges);
                content_width / aspect_ratio + edges.padding.vertical() + edges.border.vertical()
            }
        }
        .max(0.0)
    }

    fn aspect_width_from_height(
        &self,
        style: &Style,
        edges: ResolvedEdges,
        border_height: f32,
        aspect_ratio: f32,
    ) -> f32 {
        match style.box_sizing {
            BoxSizing::BorderBox => border_height * aspect_ratio,
            BoxSizing::ContentBox => {
                let content_height = self.inner_height(border_height, edges);
                content_height * aspect_ratio
                    + edges.padding.horizontal()
                    + edges.border.horizontal()
            }
        }
        .max(0.0)
    }

    fn valid_aspect_ratio(&self, style: &Style) -> Option<f32> {
        style
            .aspect_ratio
            .filter(|aspect_ratio| aspect_ratio.is_finite() && *aspect_ratio > self.epsilon)
    }

    fn exported_layout_result(
        &self,
        layout: LayoutResult,
        rounding: RoundingContext,
        physical_pixels_per_layout_unit: f32,
    ) -> LayoutResult {
        let absolute_left = rounding.container_absolute.x + layout.offset.x;
        let absolute_top = rounding.container_absolute.y + layout.offset.y;
        let absolute_right = absolute_left + layout.size.width;
        let absolute_bottom = absolute_top + layout.size.height;
        let rounded_absolute_left =
            Self::round_to_pixel_grid(absolute_left, physical_pixels_per_layout_unit);
        let rounded_absolute_top =
            Self::round_to_pixel_grid(absolute_top, physical_pixels_per_layout_unit);
        let rounded_absolute_right =
            Self::round_to_pixel_grid(absolute_right, physical_pixels_per_layout_unit);
        let rounded_absolute_bottom =
            Self::round_to_pixel_grid(absolute_bottom, physical_pixels_per_layout_unit);
        let rounded_offset = Point::new(
            rounded_absolute_left - rounding.container_rounded.x,
            rounded_absolute_top - rounding.container_rounded.y,
        );
        let rounded_size = Size::new(
            rounded_absolute_right - rounded_absolute_left,
            rounded_absolute_bottom - rounded_absolute_top,
        );
        let baseline = layout.baseline.or_else(|| {
            (layout.size.height.abs() > f32::EPSILON
                && (rounded_size.height - layout.size.height).abs() > f32::EPSILON)
                .then_some(layout.size.height)
        });
        let sticky_pos = self.exported_sticky_pos(
            layout,
            rounding,
            rounded_absolute_right,
            rounded_absolute_bottom,
            physical_pixels_per_layout_unit,
        );
        let rounded_border = Edges {
            left: Self::round_to_pixel_grid(layout.border.left, physical_pixels_per_layout_unit),
            right: Self::round_to_pixel_grid(layout.border.right, physical_pixels_per_layout_unit),
            top: Self::round_to_pixel_grid(layout.border.top, physical_pixels_per_layout_unit),
            bottom: Self::round_to_pixel_grid(
                layout.border.bottom,
                physical_pixels_per_layout_unit,
            ),
        };
        let content_left = Self::round_to_pixel_grid(
            absolute_left + layout.padding.left + layout.border.left,
            physical_pixels_per_layout_unit,
        );
        let content_top = Self::round_to_pixel_grid(
            absolute_top + layout.padding.top + layout.border.top,
            physical_pixels_per_layout_unit,
        );
        let content_right = Self::round_to_pixel_grid(
            absolute_right - layout.padding.right - layout.border.right,
            physical_pixels_per_layout_unit,
        );
        let content_bottom = Self::round_to_pixel_grid(
            absolute_bottom - layout.padding.bottom - layout.border.bottom,
            physical_pixels_per_layout_unit,
        );
        LayoutResult {
            offset: rounded_offset,
            size: rounded_size,
            padding: Edges {
                left: content_left - rounded_absolute_left - rounded_border.left,
                right: rounded_absolute_right - content_right - rounded_border.right,
                top: content_top - rounded_absolute_top - rounded_border.top,
                bottom: rounded_absolute_bottom - content_bottom - rounded_border.bottom,
            },
            border: rounded_border,
            sticky_pos,
            baseline,
            margin: layout.margin,
        }
    }

    fn reexport_cached_subtree<T: LayoutTree>(
        &self,
        tree: &mut T,
        node: T::NodeId,
        raw_layout: LayoutResult,
        rounding: RoundingContext,
    ) {
        let physical_pixels_per_layout_unit = tree.physical_pixels_per_layout_unit(node);
        let exported_layout =
            self.exported_layout_result(raw_layout, rounding, physical_pixels_per_layout_unit);
        tree.set_layout(node, exported_layout);

        let node_absolute = Point::new(
            rounding.container_absolute.x + raw_layout.offset.x,
            rounding.container_absolute.y + raw_layout.offset.y,
        );
        let child_rounding = RoundingContext {
            container_absolute: node_absolute,
            container_rounded: Point::new(
                Self::round_to_pixel_grid(node_absolute.x, physical_pixels_per_layout_unit),
                Self::round_to_pixel_grid(node_absolute.y, physical_pixels_per_layout_unit),
            ),
        };
        let children = tree.children(node).collect::<Vec<_>>();
        for child in children {
            if let Some(child_layout) = tree.layout(child) {
                self.reexport_cached_subtree(tree, child, child_layout, child_rounding);
            }
        }
    }

    fn exported_sticky_pos(
        &self,
        layout: LayoutResult,
        rounding: RoundingContext,
        rounded_absolute_right: f32,
        rounded_absolute_bottom: f32,
        physical_pixels_per_layout_unit: f32,
    ) -> Edges {
        let absolute_right = rounding.container_absolute.x + layout.offset.x + layout.size.width;
        let absolute_bottom = rounding.container_absolute.y + layout.offset.y + layout.size.height;
        Edges {
            left: Self::round_to_pixel_grid(
                layout.sticky_pos.left + rounding.container_absolute.x,
                physical_pixels_per_layout_unit,
            ) - rounding.container_rounded.x,
            right: rounded_absolute_right
                - Self::round_to_pixel_grid(
                    absolute_right - layout.sticky_pos.right,
                    physical_pixels_per_layout_unit,
                ),
            top: Self::round_to_pixel_grid(
                layout.sticky_pos.top + rounding.container_absolute.y,
                physical_pixels_per_layout_unit,
            ) - rounding.container_rounded.y,
            bottom: rounded_absolute_bottom
                - Self::round_to_pixel_grid(
                    absolute_bottom - layout.sticky_pos.bottom,
                    physical_pixels_per_layout_unit,
                ),
        }
    }

    fn round_to_pixel_grid(value: f32, physical_pixels_per_layout_unit: f32) -> f32 {
        let physical_pixels_per_layout_unit =
            Self::valid_physical_pixels_per_layout_unit(physical_pixels_per_layout_unit);
        (value * physical_pixels_per_layout_unit).round() / physical_pixels_per_layout_unit
    }

    fn ceil_to_pixel_grid(value: f32, physical_pixels_per_layout_unit: f32) -> f32 {
        let physical_pixels_per_layout_unit =
            Self::valid_physical_pixels_per_layout_unit(physical_pixels_per_layout_unit);
        (value * physical_pixels_per_layout_unit).ceil() / physical_pixels_per_layout_unit
    }

    fn valid_physical_pixels_per_layout_unit(value: f32) -> f32 {
        if value.is_finite() && value > 0.0 {
            value
        } else {
            1.0
        }
    }

    fn resolve_edges(&self, style: &Style, constraints: Constraints) -> ResolvedEdges {
        let base = constraints.width.percent_base();
        ResolvedEdges {
            margin: self.resolve_rect(style.margin, base, base),
            padding: self.resolve_padding_rect(style.padding, base, base),
            border: style.border,
        }
    }

    fn resolve_edges_for_parent(
        &self,
        style: &Style,
        parent_width_constraint: SideConstraint,
    ) -> ResolvedEdges {
        let base = parent_width_constraint.percent_base();
        ResolvedEdges {
            margin: self.resolve_rect(style.margin, base, base),
            padding: self.resolve_padding_rect(style.padding, base, base),
            border: style.border,
        }
    }

    fn update_percent_box_edges(
        &self,
        style: &Style,
        mut edges: ResolvedEdges,
        parent_width_constraint: SideConstraint,
    ) -> ResolvedEdges {
        let base = parent_width_constraint.percent_base();
        edges.margin = self.update_percent_rect(style.margin, edges.margin, base);
        edges.padding = self.update_percent_padding_rect(style.padding, edges.padding, base);
        edges
    }

    fn update_percent_rect(
        &self,
        rect: Rect<Length>,
        mut edges: Edges,
        percent_base: Option<f32>,
    ) -> Edges {
        edges.left = self.update_percent_edge(rect.left, edges.left, percent_base);
        edges.right = self.update_percent_edge(rect.right, edges.right, percent_base);
        edges.top = self.update_percent_edge(rect.top, edges.top, percent_base);
        edges.bottom = self.update_percent_edge(rect.bottom, edges.bottom, percent_base);
        edges
    }

    fn update_percent_padding_rect(
        &self,
        rect: Rect<Length>,
        mut edges: Edges,
        percent_base: Option<f32>,
    ) -> Edges {
        edges.left = self
            .update_percent_edge(rect.left, edges.left, percent_base)
            .max(0.0);
        edges.right = self
            .update_percent_edge(rect.right, edges.right, percent_base)
            .max(0.0);
        edges.top = self
            .update_percent_edge(rect.top, edges.top, percent_base)
            .max(0.0);
        edges.bottom = self
            .update_percent_edge(rect.bottom, edges.bottom, percent_base)
            .max(0.0);
        edges
    }

    fn update_percent_edge(&self, length: Length, current: f32, percent_base: Option<f32>) -> f32 {
        if matches!(length, Length::Percent(_)) {
            length.resolve(percent_base).unwrap_or(current)
        } else {
            current
        }
    }

    fn resolve_rect(
        &self,
        rect: Rect<Length>,
        horizontal_base: Option<f32>,
        vertical_base: Option<f32>,
    ) -> Edges {
        Edges {
            left: self
                .resolve_edge_length(rect.left, horizontal_base)
                .unwrap_or(0.0),
            right: self
                .resolve_edge_length(rect.right, horizontal_base)
                .unwrap_or(0.0),
            top: self
                .resolve_edge_length(rect.top, vertical_base)
                .unwrap_or(0.0),
            bottom: self
                .resolve_edge_length(rect.bottom, vertical_base)
                .unwrap_or(0.0),
        }
    }

    fn resolve_padding_rect(
        &self,
        rect: Rect<Length>,
        horizontal_base: Option<f32>,
        vertical_base: Option<f32>,
    ) -> Edges {
        let padding = self.resolve_rect(rect, horizontal_base, vertical_base);
        Edges {
            left: padding.left.max(0.0),
            right: padding.right.max(0.0),
            top: padding.top.max(0.0),
            bottom: padding.bottom.max(0.0),
        }
    }

    fn resolve_border_axis(
        &self,
        length: Length,
        axis: Axis,
        constraints: Constraints,
        edges: ResolvedEdges,
        box_sizing: BoxSizing,
    ) -> Option<f32> {
        let percent_base = match axis {
            Axis::Horizontal => constraints.width.percent_base(),
            Axis::Vertical => constraints.height.percent_base(),
        };
        self.resolve_axis_length(length, percent_base)
            .map(|value| match box_sizing {
                BoxSizing::BorderBox => value.max(0.0),
                BoxSizing::ContentBox => self.border_size_from_content_axis(axis, value, edges),
            })
    }

    fn resolve_min_max_border_axis(
        &self,
        style: &Style,
        axis: Axis,
        length: Length,
        constraints: Constraints,
        edges: ResolvedEdges,
    ) -> Option<f32> {
        let percent_base = match axis {
            Axis::Horizontal => constraints.width.percent_base(),
            Axis::Vertical => constraints.height.percent_base(),
        };
        self.resolve_min_max_length(length, percent_base)
            .map(|value| self.border_size_from_css_axis(style, axis, value, edges))
    }

    fn resolve_min_max_length(&self, length: Length, percent_base: Option<f32>) -> Option<f32> {
        self.resolve_numeric_length(length, percent_base)
    }

    fn apply_min_max_to_constraints(
        &self,
        style: &Style,
        constraints: Constraints,
        edges: ResolvedEdges,
    ) -> Constraints {
        Constraints::new(
            self.apply_min_max_to_axis_constraint(style, Axis::Horizontal, constraints, edges),
            self.apply_min_max_to_axis_constraint(style, Axis::Vertical, constraints, edges),
        )
    }

    fn apply_min_max_to_axis_constraint(
        &self,
        style: &Style,
        axis: Axis,
        constraints: Constraints,
        edges: ResolvedEdges,
    ) -> SideConstraint {
        let constraint = match axis {
            Axis::Horizontal => constraints.width,
            Axis::Vertical => constraints.height,
        };
        let min_length = match axis {
            Axis::Horizontal => style.min_width,
            Axis::Vertical => style.min_height,
        };
        let max_length = match axis {
            Axis::Horizontal => style.max_width,
            Axis::Vertical => style.max_height,
        };
        let min = self.resolve_min_max_border_axis(style, axis, min_length, constraints, edges);
        let max = self.resolve_min_max_border_axis(style, axis, max_length, constraints, edges);

        match constraint.mode {
            MeasureMode::Indefinite => max.map_or(constraint, SideConstraint::at_most),
            MeasureMode::Definite | MeasureMode::AtMost => {
                let mut size = constraint.size;
                if let Some(min) = min {
                    size = size.max(min);
                }
                if let Some(max) = max {
                    size = size.min(max);
                }
                SideConstraint {
                    size,
                    mode: constraint.mode,
                }
            }
        }
    }

    fn border_size_from_css_axis(
        &self,
        style: &Style,
        axis: Axis,
        value: f32,
        edges: ResolvedEdges,
    ) -> f32 {
        match style.box_sizing {
            BoxSizing::BorderBox => value.max(0.0),
            BoxSizing::ContentBox => self.border_size_from_content_axis(axis, value, edges),
        }
    }

    fn css_axis_size_from_border_size(
        &self,
        style: &Style,
        axis: Axis,
        border_size: f32,
        edges: ResolvedEdges,
    ) -> f32 {
        match style.box_sizing {
            BoxSizing::BorderBox => border_size.max(0.0),
            BoxSizing::ContentBox => (border_size - self.padding_border(axis, edges)).max(0.0),
        }
    }

    fn border_size_from_content_axis(&self, axis: Axis, value: f32, edges: ResolvedEdges) -> f32 {
        (value + self.padding_border(axis, edges)).max(0.0)
    }

    fn clamp_axis(
        &self,
        style: &Style,
        axis: Axis,
        value: f32,
        constraints: Constraints,
        edges: ResolvedEdges,
    ) -> f32 {
        let constraint = match axis {
            Axis::Horizontal => constraints.width,
            Axis::Vertical => constraints.height,
        };
        let percent_base = constraint.percent_base();
        let mut result = constraint.clamp(value);
        let padding_border = self.padding_border(axis, edges);

        let min_length = match axis {
            Axis::Horizontal => style.min_width,
            Axis::Vertical => style.min_height,
        };
        if let Some(min) = self.resolve_min_max_length(min_length, percent_base) {
            let min = match style.box_sizing {
                BoxSizing::BorderBox => min,
                BoxSizing::ContentBox => min + padding_border,
            };
            result = result.max(min);
        }

        let max_length = match axis {
            Axis::Horizontal => style.max_width,
            Axis::Vertical => style.max_height,
        };
        if let Some(max) = self.resolve_min_max_length(max_length, percent_base) {
            let max = match style.box_sizing {
                BoxSizing::BorderBox => max,
                BoxSizing::ContentBox => max + padding_border,
            };
            result = result.min(max);
        }
        result.max(padding_border)
    }

    fn clamp_content_axis(
        &self,
        style: &Style,
        axis: Axis,
        content_size: f32,
        constraints: Constraints,
        edges: ResolvedEdges,
    ) -> f32 {
        let border_size = content_size + self.padding_border(axis, edges);
        let clamped_border_size = self.clamp_axis(style, axis, border_size, constraints, edges);
        self.inner_axis(clamped_border_size.max(0.0), axis.is_horizontal(), edges)
    }

    fn clamp_measured_axis(
        &self,
        style: &Style,
        axis: Axis,
        value: f32,
        percent_base: Option<f32>,
        edges: ResolvedEdges,
    ) -> f32 {
        let padding_border = self.padding_border(axis, edges);
        let mut result = value;

        let min_length = match axis {
            Axis::Horizontal => style.min_width,
            Axis::Vertical => style.min_height,
        };
        if let Some(min) = self.resolve_min_max_length(min_length, percent_base) {
            let min = match style.box_sizing {
                BoxSizing::BorderBox => min,
                BoxSizing::ContentBox => min + padding_border,
            };
            result = result.max(min);
        }

        let max_length = match axis {
            Axis::Horizontal => style.max_width,
            Axis::Vertical => style.max_height,
        };
        if let Some(max) = self.resolve_min_max_length(max_length, percent_base) {
            let max = match style.box_sizing {
                BoxSizing::BorderBox => max,
                BoxSizing::ContentBox => max + padding_border,
            };
            result = result.min(max);
        }

        result.max(padding_border)
    }

    fn resolve_gap(
        &self,
        style: &Style,
        is_main_row: bool,
        constraints: Constraints,
    ) -> Option<f32> {
        let (gap, percent_base) = if is_main_row {
            (style.column_gap, constraints.width.percent_base())
        } else {
            (style.row_gap, constraints.height.percent_base())
        };
        self.resolve_gap_length(gap, percent_base)
    }

    fn resolve_gap_with_content_size(
        &self,
        style: &Style,
        is_column_gap: bool,
        content_size: f32,
        fallback: f32,
    ) -> f32 {
        let gap = if is_column_gap {
            style.column_gap
        } else {
            style.row_gap
        };
        self.resolve_gap_length(gap, Some(content_size))
            .unwrap_or(fallback)
    }

    fn resolve_gap_length(&self, gap: Length, percent_base: Option<f32>) -> Option<f32> {
        match gap {
            Length::FitContent(Some(base)) => self.resolve_base_length(base, percent_base),
            Length::Fr(value) => Some(value),
            Length::Auto | Length::MinContent | Length::MaxContent | Length::FitContent(None) => {
                None
            }
            Length::Points(_) | Length::Percent(_) | Length::Calc { .. } => {
                gap.resolve(percent_base)
            }
        }
    }

    fn positioned_offset(&self, style: &Style, constraints: Constraints, offset: Point) -> Point {
        if style.position != PositionType::Relative {
            return offset;
        }

        let x = if let Some(left) =
            self.resolve_edge_length(style.left, constraints.width.percent_base())
        {
            offset.x + left
        } else if let Some(right) =
            self.resolve_edge_length(style.right, constraints.width.percent_base())
        {
            offset.x - right
        } else {
            offset.x
        };
        let y = if let Some(top) =
            self.resolve_edge_length(style.top, constraints.height.percent_base())
        {
            offset.y + top
        } else if let Some(bottom) =
            self.resolve_edge_length(style.bottom, constraints.height.percent_base())
        {
            offset.y - bottom
        } else {
            offset.y
        };

        Point::new(x, y)
    }

    fn sticky_pos(&self, style: &Style, constraints: Constraints) -> Edges {
        if style.position != PositionType::Sticky {
            return Edges::default();
        }

        Edges {
            left: self.sticky_inset(style.left, constraints.width.percent_base()),
            right: self.sticky_inset(style.right, constraints.width.percent_base()),
            top: self.sticky_inset(style.top, constraints.height.percent_base()),
            bottom: self.sticky_inset(style.bottom, constraints.height.percent_base()),
        }
    }

    fn sticky_inset(&self, length: Length, percent_base: Option<f32>) -> f32 {
        self.resolve_edge_length(length, percent_base)
            .unwrap_or(Self::STICKY_AUTO_INSET)
    }

    fn justify(
        &self,
        justify_content: JustifyContent,
        free_space: f32,
        item_count: usize,
        gap: f32,
    ) -> (f32, f32) {
        let (start, interval) = self.justify_interval(justify_content, free_space, item_count);
        (start, gap + interval)
    }

    fn justify_interval(
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
            JustifyContent::SpaceBetween if negative_free_space || item_count == 1 => (0.0, 0.0),
            JustifyContent::SpaceBetween if item_count > 1 => {
                (0.0, free_space / (item_count - 1) as f32)
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
            JustifyContent::SpaceBetween => (0.0, 0.0),
        }
    }

    fn align_content(
        &self,
        align_content: AlignContent,
        free_space: f32,
        line_count: usize,
        gap: f32,
    ) -> (f32, f32) {
        if line_count == 0 {
            return (0.0, 0.0);
        }
        let negative_space_with_gap = free_space < -self.epsilon && gap > self.epsilon;
        match align_content {
            AlignContent::FlexStart | AlignContent::Start | AlignContent::Stretch => (0.0, 0.0),
            AlignContent::FlexEnd | AlignContent::End => (free_space, 0.0),
            AlignContent::Center => (free_space / 2.0, 0.0),
            AlignContent::SpaceBetween if negative_space_with_gap => (0.0, 0.0),
            AlignContent::SpaceBetween if line_count > 1 => {
                (0.0, free_space / (line_count - 1) as f32)
            }
            AlignContent::SpaceAround if negative_space_with_gap => (free_space / 2.0, 0.0),
            AlignContent::SpaceAround => {
                let space = free_space / line_count as f32;
                (space / 2.0, space)
            }
            AlignContent::SpaceEvenly => {
                let space = free_space / (line_count + 1) as f32;
                (space, space)
            }
            AlignContent::SpaceBetween => (0.0, 0.0),
        }
    }

    fn align_content_with_gap(
        &self,
        align_content: AlignContent,
        free_space: f32,
        line_count: usize,
        gap: f32,
    ) -> (f32, f32) {
        let (start, extra_interval) =
            self.align_content(align_content, free_space, line_count, gap);
        (start, gap + extra_interval)
    }

    fn physical_offset_from_logical_start(
        &self,
        logical_offset: f32,
        content_size: f32,
        item_size: f32,
        reversed: bool,
    ) -> f32 {
        if reversed {
            if logical_offset < 0.0 && (item_size - item_size.round()).abs() > f32::EPSILON {
                content_size - logical_offset - item_size
            } else {
                // Keep reversed bound writeback order stable at observable .5 pixel boundaries.
                (content_size - item_size) - logical_offset
            }
        } else {
            logical_offset
        }
    }

    fn exported_zero_offset(offset: f32) -> f32 {
        if offset.abs() < Self::LAYOUT_SETTER_ZERO_EPSILON {
            0.0
        } else {
            offset
        }
    }

    fn parent_border_offset(border: f32, padding: f32, content_offset: f32) -> f32 {
        border + Self::exported_zero_offset(padding + content_offset)
    }

    fn near_integer_bound(value: f32) -> f32 {
        let rounded = value.round();
        if (value - rounded).abs() <= Self::BOUND_INTEGER_EPSILON {
            rounded
        } else {
            value
        }
    }

    fn gap_total(&self, gap: f32, count: usize) -> f32 {
        if count > 1 {
            gap * (count - 1) as f32
        } else {
            0.0
        }
    }

    fn has_axis_auto_margin(&self, style: &Style, horizontal: bool) -> bool {
        self.axis_start_margin_is_auto(style, horizontal)
            || self.axis_end_margin_is_auto(style, horizontal)
    }

    fn axis_start_margin_with_auto(
        &self,
        style: &Style,
        margin: Edges,
        horizontal: bool,
        auto_margin: Option<f32>,
    ) -> f32 {
        if self.axis_start_margin_is_auto(style, horizontal) {
            auto_margin.unwrap_or(0.0)
        } else {
            self.axis_start_margin(margin, horizontal)
        }
    }

    fn axis_end_margin_with_auto(
        &self,
        style: &Style,
        margin: Edges,
        horizontal: bool,
        auto_margin: Option<f32>,
    ) -> f32 {
        if self.axis_end_margin_is_auto(style, horizontal) {
            auto_margin.unwrap_or(0.0)
        } else {
            self.axis_end_margin(margin, horizontal)
        }
    }

    fn computed_linear_layout_gravity(
        &self,
        container_style: &Style,
        item_style: &Style,
    ) -> LinearLayoutGravity {
        let mut gravity = item_style.linear_layout_gravity;
        if gravity == LinearLayoutGravity::None {
            if let Some(align_self) = item_style.align_self {
                gravity = self.align_to_linear_layout_gravity(align_self);
            }
        }
        if gravity == LinearLayoutGravity::None {
            gravity =
                self.cross_gravity_to_linear_layout_gravity(container_style.linear_cross_gravity);
        }
        if gravity == LinearLayoutGravity::None
            && container_style.align_items != AlignItems::Stretch
        {
            gravity = self.align_to_linear_layout_gravity(container_style.align_items);
        }
        if !container_style.linear_orientation.is_row() && container_style.direction.is_rtl() {
            gravity = match gravity {
                LinearLayoutGravity::Left => LinearLayoutGravity::Right,
                LinearLayoutGravity::Right => LinearLayoutGravity::Left,
                _ => gravity,
            };
        }
        gravity
    }

    fn align_to_linear_layout_gravity(&self, align_items: AlignItems) -> LinearLayoutGravity {
        match align_items {
            AlignItems::Stretch => LinearLayoutGravity::Stretch,
            AlignItems::FlexStart | AlignItems::Start => LinearLayoutGravity::Start,
            AlignItems::Center => LinearLayoutGravity::Center,
            AlignItems::FlexEnd | AlignItems::End => LinearLayoutGravity::End,
            AlignItems::Baseline => LinearLayoutGravity::None,
        }
    }

    fn cross_gravity_to_linear_layout_gravity(
        &self,
        cross_gravity: LinearCrossGravity,
    ) -> LinearLayoutGravity {
        match cross_gravity {
            LinearCrossGravity::None => LinearLayoutGravity::None,
            LinearCrossGravity::Start => LinearLayoutGravity::Start,
            LinearCrossGravity::End => LinearLayoutGravity::End,
            LinearCrossGravity::Center => LinearLayoutGravity::Center,
            LinearCrossGravity::Stretch => LinearLayoutGravity::Stretch,
        }
    }

    fn linear_layout_gravity_forces_stretch(&self, gravity: LinearLayoutGravity) -> bool {
        matches!(
            gravity,
            LinearLayoutGravity::FillHorizontal
                | LinearLayoutGravity::FillVertical
                | LinearLayoutGravity::Stretch
        )
    }

    fn set_css_axis_size_from_border_size(
        &self,
        target: &mut Style,
        source: &Style,
        axis: Axis,
        border_axis_size: f32,
        edges: ResolvedEdges,
    ) {
        let css_size = self.css_axis_size_from_border_size(source, axis, border_axis_size, edges);
        match axis {
            Axis::Horizontal => target.width = Length::points(css_size),
            Axis::Vertical => target.height = Length::points(css_size),
        }
    }

    fn exported_container_baseline(&self, baseline: Option<f32>) -> Option<f32> {
        baseline.filter(|value| value.abs() > self.epsilon)
    }

    fn axis_constraint(&self, constraints: Constraints, horizontal: bool) -> SideConstraint {
        if horizontal {
            constraints.width
        } else {
            constraints.height
        }
    }

    fn axis_constraint_from_optional(&self, size: Option<f32>) -> SideConstraint {
        size.map(SideConstraint::definite)
            .unwrap_or_else(SideConstraint::indefinite)
    }

    fn inner_axis(&self, border_size: f32, horizontal: bool, edges: ResolvedEdges) -> f32 {
        if horizontal {
            self.inner_width(border_size, edges)
        } else {
            self.inner_height(border_size, edges)
        }
    }

    fn inner_width(&self, border_width: f32, edges: ResolvedEdges) -> f32 {
        (border_width - edges.padding.horizontal() - edges.border.horizontal()).max(0.0)
    }

    fn inner_height(&self, border_height: f32, edges: ResolvedEdges) -> f32 {
        (border_height - edges.padding.vertical() - edges.border.vertical()).max(0.0)
    }

    fn main_size(&self, size: Size, is_row: bool) -> f32 {
        if is_row {
            size.width
        } else {
            size.height
        }
    }

    fn cross_size(&self, size: Size, is_row: bool) -> f32 {
        if is_row {
            size.height
        } else {
            size.width
        }
    }

    fn axis_margin(&self, margin: Edges, horizontal: bool) -> f32 {
        if horizontal {
            margin.horizontal()
        } else {
            margin.vertical()
        }
    }

    fn physical_margin_bound_axis_size(
        &self,
        border_size: f32,
        margin: Edges,
        horizontal: bool,
    ) -> f32 {
        if horizontal {
            (border_size + margin.left) + margin.right
        } else {
            (border_size + margin.top) + margin.bottom
        }
    }

    fn axis_start_margin(&self, margin: Edges, horizontal: bool) -> f32 {
        if horizontal {
            margin.left
        } else {
            margin.top
        }
    }

    fn axis_end_margin(&self, margin: Edges, horizontal: bool) -> f32 {
        if horizontal {
            margin.right
        } else {
            margin.bottom
        }
    }

    fn axis_logical_start_margin(&self, margin: Edges, horizontal: bool, reversed: bool) -> f32 {
        if reversed {
            self.axis_end_margin(margin, horizontal)
        } else {
            self.axis_start_margin(margin, horizontal)
        }
    }

    fn axis_logical_end_margin(&self, margin: Edges, horizontal: bool, reversed: bool) -> f32 {
        if reversed {
            self.axis_start_margin(margin, horizontal)
        } else {
            self.axis_end_margin(margin, horizontal)
        }
    }

    fn set_axis_start_margin(&self, margin: &mut Edges, horizontal: bool, value: f32) {
        if horizontal {
            margin.left = value;
        } else {
            margin.top = value;
        }
    }

    fn set_axis_end_margin(&self, margin: &mut Edges, horizontal: bool, value: f32) {
        if horizontal {
            margin.right = value;
        } else {
            margin.bottom = value;
        }
    }

    fn set_axis_logical_start_margin(
        &self,
        margin: &mut Edges,
        horizontal: bool,
        reversed: bool,
        value: f32,
    ) {
        if reversed {
            self.set_axis_end_margin(margin, horizontal, value);
        } else {
            self.set_axis_start_margin(margin, horizontal, value);
        }
    }

    fn set_axis_logical_end_margin(
        &self,
        margin: &mut Edges,
        horizontal: bool,
        reversed: bool,
        value: f32,
    ) {
        if reversed {
            self.set_axis_start_margin(margin, horizontal, value);
        } else {
            self.set_axis_end_margin(margin, horizontal, value);
        }
    }

    fn axis_start_margin_is_auto(&self, style: &Style, horizontal: bool) -> bool {
        matches!(
            if horizontal {
                style.margin.left
            } else {
                style.margin.top
            },
            Length::Auto
        )
    }

    fn axis_end_margin_is_auto(&self, style: &Style, horizontal: bool) -> bool {
        matches!(
            if horizontal {
                style.margin.right
            } else {
                style.margin.bottom
            },
            Length::Auto
        )
    }

    fn axis_logical_start_margin_is_auto(
        &self,
        style: &Style,
        horizontal: bool,
        reversed: bool,
    ) -> bool {
        if reversed {
            self.axis_end_margin_is_auto(style, horizontal)
        } else {
            self.axis_start_margin_is_auto(style, horizontal)
        }
    }

    fn axis_logical_end_margin_is_auto(
        &self,
        style: &Style,
        horizontal: bool,
        reversed: bool,
    ) -> bool {
        if reversed {
            self.axis_start_margin_is_auto(style, horizontal)
        } else {
            self.axis_end_margin_is_auto(style, horizontal)
        }
    }

    fn padding_border(&self, axis: Axis, edges: ResolvedEdges) -> f32 {
        match axis {
            Axis::Horizontal => edges.padding.horizontal() + edges.border.horizontal(),
            Axis::Vertical => edges.padding.vertical() + edges.border.vertical(),
        }
    }
}

fn is_out_of_flow(style: &Style) -> bool {
    matches!(style.position, PositionType::Absolute | PositionType::Fixed)
}

fn ranges_overlap(
    first_start: usize,
    first_end: usize,
    second_start: usize,
    second_end: usize,
) -> bool {
    first_start < second_end && second_start < first_end
}

impl Default for LayoutEngine {
    fn default() -> Self {
        Self::new()
    }
}

#[derive(Clone, Copy, Debug)]
struct ResolvedEdges {
    margin: Edges,
    padding: Edges,
    border: Edges,
}

#[derive(Clone, Copy, Debug)]
struct NodeLayoutContext {
    constraints: Constraints,
    offset: Point,
    sticky_constraints: Constraints,
    edges: ResolvedEdges,
    rounding: RoundingContext,
    flex: FlexNodeContext,
}

#[derive(Clone, Copy, Debug, Default)]
struct FlexNodeContext {
    percent: FlexPercentPropagation,
}

impl FlexNodeContext {
    const fn from_parts(percent: FlexPercentPropagation) -> Self {
        Self { percent }
    }
}

#[derive(Clone, Copy, Debug, Default)]
struct FlexPercentPropagation {
    suppress_flex_basis_percent_base: bool,
    suppress_main_size_percent_base: bool,
}

impl FlexPercentPropagation {
    const fn new(
        suppress_flex_basis_percent_base: bool,
        suppress_main_size_percent_base: bool,
    ) -> Self {
        Self {
            suppress_flex_basis_percent_base,
            suppress_main_size_percent_base,
        }
    }
}

#[derive(Clone, Copy, Debug)]
struct RoundingContext {
    container_absolute: Point,
    container_rounded: Point,
}

impl RoundingContext {
    const fn root() -> Self {
        Self {
            container_absolute: Point::ZERO,
            container_rounded: Point::ZERO,
        }
    }
}

#[derive(Clone, Copy, Debug)]
struct LayoutBox {
    size: Size,
    baseline: Option<f32>,
    layout: LayoutResult,
    constraints: Constraints,
}

impl LayoutBox {
    fn new(size: Size, margin: Edges, baseline: Option<f32>, constraints: Constraints) -> Self {
        let layout = LayoutResult {
            size,
            margin,
            baseline,
            ..LayoutResult::default()
        };
        Self {
            size,
            baseline,
            layout,
            constraints,
        }
    }

    fn from_raw_and_exported(
        raw_layout: LayoutResult,
        exported_layout: LayoutResult,
        constraints: Constraints,
    ) -> Self {
        Self {
            size: raw_layout.size,
            baseline: raw_layout.baseline,
            layout: exported_layout,
            constraints,
        }
    }
}

#[derive(Clone, Copy, Debug)]
struct LayoutOutput {
    size: Size,
    baseline: Option<f32>,
}

impl LayoutOutput {
    const fn new(size: Size) -> Self {
        Self {
            size,
            baseline: None,
        }
    }
}

#[derive(Clone, Copy, Debug)]
struct FixedDescendant<N> {
    node: N,
    hidden_by_display_none_ancestor: bool,
}

#[derive(Clone, Copy, Debug)]
struct OutOfFlowContext {
    containing_size: Size,
    container_origin: Point,
    containing_block: Constraints,
}

#[derive(Clone, Copy, Debug)]
struct OutOfFlowAlignment {
    horizontal: OutOfFlowAxisAlignment,
    vertical: OutOfFlowAxisAlignment,
}

#[derive(Clone, Copy, Debug)]
struct OutOfFlowAxisAlignment {
    position: OutOfFlowPosition,
    front_is_physical_start: bool,
}

impl OutOfFlowAxisAlignment {
    fn new(position: OutOfFlowPosition, front_is_physical_start: bool) -> Self {
        Self {
            position,
            front_is_physical_start,
        }
    }

    fn start_with_front(front_is_physical_start: bool) -> Self {
        Self::new(OutOfFlowPosition::Start, front_is_physical_start)
    }

    fn center() -> Self {
        Self::new(OutOfFlowPosition::Center, true)
    }

    fn end_with_front(front_is_physical_start: bool) -> Self {
        Self::new(OutOfFlowPosition::End, front_is_physical_start)
    }
}

#[derive(Clone, Copy, Debug)]
enum OutOfFlowPosition {
    Start,
    Center,
    End,
}

impl OutOfFlowPosition {
    fn reverse(self) -> Self {
        match self {
            Self::Start => Self::End,
            Self::Center => Self::Center,
            Self::End => Self::Start,
        }
    }
}

#[derive(Clone, Copy, Debug)]
enum Axis {
    Horizontal,
    Vertical,
}

impl Axis {
    fn is_horizontal(self) -> bool {
        matches!(self, Self::Horizontal)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::tree::{SimpleNode, SimpleTree};
    use std::cell::Cell;

    fn assert_close(actual: f32, expected: f32) {
        assert!(
            (actual - expected).abs() < 0.01,
            "expected {expected}, got {actual}"
        );
    }

    #[test]
    fn flex_node_context_carries_percent_propagation() {
        let flex = FlexNodeContext::from_parts(FlexPercentPropagation::new(true, false));

        assert!(flex.percent.suppress_flex_basis_percent_base);
        assert!(!flex.percent.suppress_main_size_percent_base);
    }

    #[derive(Debug)]
    struct TrackingNode {
        style: Style,
        children: Vec<usize>,
        layout: LayoutResult,
        layout_reads: Cell<usize>,
    }

    impl TrackingNode {
        fn new(style: Style) -> Self {
            Self {
                style,
                children: Vec::new(),
                layout: LayoutResult::default(),
                layout_reads: Cell::new(0),
            }
        }
    }

    #[derive(Default)]
    struct TrackingTree {
        nodes: Vec<TrackingNode>,
    }

    impl TrackingTree {
        fn push(&mut self, node: TrackingNode) -> usize {
            let id = self.nodes.len();
            self.nodes.push(node);
            id
        }

        fn append_child(&mut self, parent: usize, child: usize) {
            self.nodes[parent].children.push(child);
        }
    }

    impl LayoutTree for TrackingTree {
        type NodeId = usize;
        type Children<'a> = std::iter::Copied<std::slice::Iter<'a, usize>>;

        fn children(&self, node: Self::NodeId) -> Self::Children<'_> {
            self.nodes[node].children.iter().copied()
        }

        fn style(&self, node: Self::NodeId) -> &Style {
            &self.nodes[node].style
        }

        fn set_layout(&mut self, node: Self::NodeId, layout: LayoutResult) {
            self.nodes[node].layout = layout;
        }

        fn layout(&self, node: Self::NodeId) -> Option<LayoutResult> {
            let node = &self.nodes[node];
            node.layout_reads.set(node.layout_reads.get() + 1);
            Some(node.layout)
        }
    }

    #[test]
    fn block_layout_runs_on_external_tree() {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            padding: Rect::all(Length::points(2.0)),
            ..Style::default()
        }));
        let child = tree.push(SimpleNode::with_measured_size(
            Style::default(),
            Size::new(30.0, 20.0),
        ));
        tree.append_child(root, child);

        let mut engine = LayoutEngine::new();
        let size = engine.layout(
            &mut tree,
            root,
            Constraints::new(
                SideConstraint::definite(100.0),
                SideConstraint::indefinite(),
            ),
        );

        assert_close(size.width, 100.0);
        assert_close(size.height, 24.0);
        assert_close(tree.nodes[child].layout.offset.x, 2.0);
        assert_close(tree.nodes[child].layout.offset.y, 2.0);
        assert_close(tree.nodes[child].layout.size.width, 96.0);
        assert_close(tree.nodes[child].layout.size.height, 20.0);
    }

    #[test]
    fn flex_row_distributes_grow_space() {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Flex,
            height: Length::points(20.0),
            ..Style::default()
        }));
        let first = tree.push(SimpleNode::new(Style {
            flex_basis: Length::points(20.0),
            flex_grow: 1.0,
            ..Style::default()
        }));
        let second = tree.push(SimpleNode::new(Style {
            flex_basis: Length::points(20.0),
            flex_grow: 3.0,
            ..Style::default()
        }));
        tree.append_child(root, first);
        tree.append_child(root, second);

        let mut engine = LayoutEngine::new();
        engine.layout(&mut tree, root, Constraints::definite(120.0, 20.0));

        assert_close(tree.nodes[first].layout.size.width, 40.0);
        assert_close(tree.nodes[second].layout.size.width, 80.0);
        assert_close(tree.nodes[first].layout.offset.x, 0.0);
        assert_close(tree.nodes[second].layout.offset.x, 40.0);
    }

    #[test]
    fn measured_flex_leaf_uses_measure_delegate_before_display_algorithm() {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Flex,
            align_items: AlignItems::FlexStart,
            ..Style::default()
        }));
        let child = tree.push(SimpleNode::with_measured_size_and_baseline(
            Style {
                display: Display::Flex,
                ..Style::default()
            },
            Size::new(12.0, 8.0),
            5.0,
        ));
        tree.append_child(root, child);

        let mut engine = LayoutEngine::new();
        engine.layout(
            &mut tree,
            root,
            Constraints::new(SideConstraint::definite(50.0), SideConstraint::indefinite()),
        );

        assert_close(tree.nodes[child].layout.size.width, 12.0);
        assert_close(tree.nodes[child].layout.size.height, 8.0);
        assert_eq!(tree.nodes[child].layout.baseline, Some(5.0));
    }

    #[test]
    fn display_none_is_laid_out_as_zero() {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style::default()));
        let hidden = tree.push(SimpleNode::new(Style::display_none()));
        tree.append_child(root, hidden);

        let mut engine = LayoutEngine::new();
        let size = engine.layout(&mut tree, root, Constraints::indefinite());

        assert_eq!(size, Size::ZERO);
        assert_eq!(tree.nodes[hidden].layout.size, Size::ZERO);
    }

    #[test]
    fn flex_stretch_reuse_reexports_cached_subtree_through_layout_tree_readback() {
        let mut tree = TrackingTree::default();
        let root = tree.push(TrackingNode::new(Style {
            display: Display::Flex,
            flex_direction: crate::style::FlexDirection::Row,
            align_items: AlignItems::Stretch,
            ..Style::default()
        }));
        let child = tree.push(TrackingNode::new(Style {
            display: Display::Block,
            ..Style::default()
        }));
        let grandchild = tree.push(TrackingNode::new(Style {
            width: Length::points(10.0),
            height: Length::points(20.0),
            ..Style::default()
        }));
        tree.append_child(root, child);
        tree.append_child(child, grandchild);

        LayoutEngine::new().layout(&mut tree, root, Constraints::definite(30.0, 20.0));

        assert!(
            tree.nodes[grandchild].layout_reads.get() > 0,
            "cached flex stretch subtree re-export should read descendant layouts through LayoutTree::layout"
        );
        assert_close(tree.nodes[child].layout.offset.x, 0.0);
        assert_close(tree.nodes[child].layout.offset.y, 0.0);
        assert_close(tree.nodes[child].layout.size.width, 10.0);
        assert_close(tree.nodes[child].layout.size.height, 20.0);
        assert_close(tree.nodes[grandchild].layout.offset.x, 0.0);
        assert_close(tree.nodes[grandchild].layout.offset.y, 0.0);
        assert_close(tree.nodes[grandchild].layout.size.width, 10.0);
        assert_close(tree.nodes[grandchild].layout.size.height, 20.0);
    }

    #[test]
    fn cached_subtree_reexport_reads_external_cached_descendant_layouts() {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style::default()));
        let child = tree.push(SimpleNode::new(Style::default()));
        let grandchild = tree.push(SimpleNode::new(Style::default()));
        tree.append_child(root, child);
        tree.append_child(child, grandchild);

        tree.nodes[child].layout = LayoutResult {
            offset: Point::new(1.4, 2.4),
            size: Size::new(3.4, 4.4),
            ..LayoutResult::default()
        };
        tree.nodes[grandchild].layout = LayoutResult {
            offset: Point::new(0.4, 0.4),
            size: Size::new(1.4, 1.4),
            ..LayoutResult::default()
        };

        LayoutEngine::new().reexport_cached_subtree(
            &mut tree,
            root,
            LayoutResult {
                offset: Point::new(0.4, 0.4),
                size: Size::new(10.4, 10.4),
                ..LayoutResult::default()
            },
            RoundingContext::root(),
        );

        assert_close(tree.nodes[root].layout.offset.x, 0.0);
        assert_close(tree.nodes[root].layout.offset.y, 0.0);
        assert_close(tree.nodes[root].layout.size.width, 11.0);
        assert_close(tree.nodes[root].layout.size.height, 11.0);

        assert_close(tree.nodes[child].layout.offset.x, 2.0);
        assert_close(tree.nodes[child].layout.offset.y, 3.0);
        assert_close(tree.nodes[child].layout.size.width, 3.0);
        assert_close(tree.nodes[child].layout.size.height, 4.0);

        assert_close(tree.nodes[grandchild].layout.offset.x, 0.0);
        assert_close(tree.nodes[grandchild].layout.offset.y, 0.0);
        assert_close(tree.nodes[grandchild].layout.size.width, 2.0);
        assert_close(tree.nodes[grandchild].layout.size.height, 2.0);
    }

    #[test]
    fn out_of_flow_auto_axis_constraint_preserves_negative_definite_both_insets() {
        let engine = LayoutEngine::new();

        assert_eq!(
            engine.out_of_flow_auto_axis_constraint(50.0, 0.0, Some(30.0), Some(40.0)),
            SideConstraint::definite(-20.0)
        );
        assert_eq!(
            engine.out_of_flow_auto_axis_constraint(50.0, 0.0, Some(60.0), None),
            SideConstraint::at_most(0.0)
        );
    }

    #[test]
    fn percentage_padding_resolves_against_definite_parent_width() {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            padding: Rect::new(
                Length::percent(10.0),
                Length::percent(10.0),
                Length::ZERO,
                Length::ZERO,
            ),
            ..Style::default()
        }));
        let child = tree.push(SimpleNode::with_measured_size(
            Style::default(),
            Size::new(10.0, 10.0),
        ));
        tree.append_child(root, child);

        let mut engine = LayoutEngine::new();
        engine.layout(
            &mut tree,
            root,
            Constraints::new(
                SideConstraint::definite(100.0),
                SideConstraint::indefinite(),
            ),
        );

        assert_close(tree.nodes[root].layout.padding.left, 10.0);
        assert_close(tree.nodes[child].layout.offset.x, 10.0);
    }

    #[test]
    fn flex_justify_center_offsets_items() {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Flex,
            justify_content: JustifyContent::Center,
            ..Style::default()
        }));
        let first = tree.push(SimpleNode::new(Style {
            flex_basis: Length::points(20.0),
            ..Style::default()
        }));
        let second = tree.push(SimpleNode::new(Style {
            flex_basis: Length::points(20.0),
            ..Style::default()
        }));
        tree.append_child(root, first);
        tree.append_child(root, second);

        let mut engine = LayoutEngine::new();
        engine.layout(
            &mut tree,
            root,
            Constraints::new(
                SideConstraint::definite(100.0),
                SideConstraint::indefinite(),
            ),
        );

        assert_close(tree.nodes[first].layout.offset.x, 30.0);
        assert_close(tree.nodes[second].layout.offset.x, 50.0);
    }

    #[test]
    fn edge_lengths_use_starlight_numeric_length_resolution() {
        for (edge_length, expected) in [
            (Length::MaxContent, 0.0),
            (Length::FitContent(Some(crate::BaseLength::fixed(4.0))), 4.0),
            (Length::fr(1.0), 1.0),
        ] {
            let mut tree = SimpleTree::default();
            let root = tree.push(SimpleNode::new(Style {
                display: Display::Flex,
                width: Length::points(80.0),
                height: Length::points(20.0),
                align_items: AlignItems::FlexStart,
                ..Style::default()
            }));
            let child = tree.push(SimpleNode::new(Style {
                position: PositionType::Relative,
                left: edge_length,
                margin: Rect::new(edge_length, Length::ZERO, Length::ZERO, Length::ZERO),
                padding: Rect::new(edge_length, Length::ZERO, Length::ZERO, Length::ZERO),
                flex_basis: Length::points(10.0),
                height: Length::points(6.0),
                box_sizing: BoxSizing::ContentBox,
                ..Style::default()
            }));
            tree.append_child(root, child);

            let mut engine = LayoutEngine::new();
            engine.layout(&mut tree, root, Constraints::definite(80.0, 20.0));

            assert_close(tree.nodes[child].layout.padding.left, expected);
            assert_close(tree.nodes[child].layout.margin.left, expected);
            assert_close(tree.nodes[child].layout.offset.x, expected * 2.0);
        }
    }

    #[test]
    fn axis_lengths_use_starlight_numeric_fr_resolution() {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Block,
            width: Length::fr(30.0),
            height: Length::fr(12.0),
            ..Style::default()
        }));

        let mut engine = LayoutEngine::new();
        let size = engine.layout(&mut tree, root, Constraints::definite(100.0, 80.0));

        assert_close(size.width, 30.0);
        assert_close(size.height, 12.0);
    }

    #[test]
    fn min_max_lengths_use_starlight_numeric_fr_resolution() {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Block,
            width: Length::points(10.0),
            min_width: Length::fr(30.0),
            height: Length::points(40.0),
            max_height: Length::fr(12.0),
            ..Style::default()
        }));

        let mut engine = LayoutEngine::new();
        let size = engine.layout(&mut tree, root, Constraints::definite(100.0, 80.0));

        assert_close(size.width, 30.0);
        assert_close(size.height, 12.0);
    }

    #[test]
    fn flex_basis_uses_starlight_numeric_fr_resolution() {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Flex,
            width: Length::points(100.0),
            height: Length::points(20.0),
            align_items: AlignItems::FlexStart,
            ..Style::default()
        }));
        let child = tree.push(SimpleNode::new(Style {
            flex_basis: Length::fr(30.0),
            height: Length::points(10.0),
            ..Style::default()
        }));
        tree.append_child(root, child);

        let mut engine = LayoutEngine::new();
        engine.layout(&mut tree, root, Constraints::definite(100.0, 20.0));

        assert_close(tree.nodes[child].layout.size.width, 30.0);
    }

    #[test]
    fn flex_column_gap_uses_starlight_numeric_length_resolution() {
        for (column_gap, expected_gap) in [
            (Length::MaxContent, 0.0),
            (
                Length::FitContent(Some(crate::BaseLength::fixed(12.0))),
                12.0,
            ),
            (Length::fr(1.0), 1.0),
        ] {
            let mut tree = SimpleTree::default();
            let root = tree.push(SimpleNode::new(Style {
                display: Display::Flex,
                width: Length::points(120.0),
                height: Length::points(30.0),
                column_gap,
                align_items: AlignItems::FlexStart,
                ..Style::default()
            }));
            let first = tree.push(SimpleNode::new(Style {
                flex_basis: Length::points(20.0),
                height: Length::points(10.0),
                ..Style::default()
            }));
            let second = tree.push(SimpleNode::new(Style {
                flex_basis: Length::points(18.0),
                height: Length::points(12.0),
                ..Style::default()
            }));
            tree.append_child(root, first);
            tree.append_child(root, second);

            let mut engine = LayoutEngine::new();
            engine.layout(&mut tree, root, Constraints::definite(120.0, 30.0));

            assert_close(tree.nodes[second].layout.offset.x, 20.0 + expected_gap);
        }
    }

    #[test]
    fn flex_row_gap_uses_starlight_numeric_length_resolution() {
        for (row_gap, expected_gap) in [
            (Length::MaxContent, 0.0),
            (
                Length::FitContent(Some(crate::BaseLength::fixed(12.0))),
                12.0,
            ),
            (Length::fr(1.0), 1.0),
        ] {
            let mut tree = SimpleTree::default();
            let root = tree.push(SimpleNode::new(Style {
                display: Display::Flex,
                width: Length::points(30.0),
                height: Length::points(80.0),
                flex_wrap: FlexWrap::Wrap,
                row_gap,
                align_items: AlignItems::FlexStart,
                align_content: AlignContent::FlexStart,
                ..Style::default()
            }));
            let first = tree.push(SimpleNode::new(Style {
                flex_basis: Length::points(20.0),
                height: Length::points(10.0),
                ..Style::default()
            }));
            let second = tree.push(SimpleNode::new(Style {
                flex_basis: Length::points(20.0),
                height: Length::points(10.0),
                ..Style::default()
            }));
            tree.append_child(root, first);
            tree.append_child(root, second);

            let mut engine = LayoutEngine::new();
            engine.layout(&mut tree, root, Constraints::definite(30.0, 80.0));

            assert_close(tree.nodes[second].layout.offset.y, 10.0 + expected_gap);
        }
    }

    #[derive(Default)]
    struct ForeignTree {
        styles: Vec<Style>,
        children: Vec<Vec<usize>>,
        layouts: Vec<LayoutResult>,
        measured: Vec<Option<Size>>,
        measure_calls: Vec<usize>,
    }

    impl ForeignTree {
        fn push(&mut self, style: Style, measured: Option<Size>) -> usize {
            let id = self.styles.len();
            self.styles.push(style);
            self.children.push(Vec::new());
            self.layouts.push(LayoutResult::default());
            self.measured.push(measured);
            self.measure_calls.push(0);
            id
        }
    }

    impl LayoutTree for ForeignTree {
        type NodeId = usize;
        type Children<'a> = std::iter::Copied<std::slice::Iter<'a, usize>>;

        fn children(&self, node: Self::NodeId) -> Self::Children<'_> {
            self.children[node].iter().copied()
        }

        fn style(&self, node: Self::NodeId) -> &Style {
            &self.styles[node]
        }

        fn set_layout(&mut self, node: Self::NodeId, layout: LayoutResult) {
            self.layouts[node] = layout;
        }

        fn measure(&mut self, node: Self::NodeId, _constraints: Constraints) -> Option<Size> {
            self.measure_calls[node] += 1;
            self.measured[node]
        }

        fn has_measure(&self, node: Self::NodeId) -> bool {
            self.measured[node].is_some()
        }
    }

    #[test]
    fn layout_tree_trait_supports_foreign_storage() {
        let mut tree = ForeignTree::default();
        let root = tree.push(
            Style {
                padding: Rect::all(Length::points(1.0)),
                ..Style::default()
            },
            None,
        );
        let child = tree.push(Style::default(), Some(Size::new(8.0, 9.0)));
        tree.children[root].push(child);

        let mut engine = LayoutEngine::new();
        let size = engine.layout(&mut tree, root, Constraints::indefinite());

        assert_close(size.width, 10.0);
        assert_close(size.height, 11.0);
        assert_close(tree.layouts[child].offset.x, 1.0);
        assert_close(tree.layouts[child].offset.y, 1.0);
    }

    #[test]
    fn fixed_size_measured_node_skips_measure_callback_when_both_axes_are_exact() {
        let mut tree = ForeignTree::default();
        let root = tree.push(
            Style {
                width: Length::points(30.0),
                height: Length::points(20.0),
                ..Style::default()
            },
            Some(Size::new(8.0, 9.0)),
        );

        let mut engine = LayoutEngine::new();
        let size = engine.layout(&mut tree, root, Constraints::indefinite());

        assert_close(size.width, 30.0);
        assert_close(size.height, 20.0);
        assert_close(tree.layouts[root].size.width, 30.0);
        assert_close(tree.layouts[root].size.height, 20.0);
        assert_eq!(tree.measure_calls[root], 0);
    }

    #[test]
    fn auto_measured_node_still_calls_measure_under_definite_owner_constraints() {
        let mut tree = ForeignTree::default();
        let root = tree.push(Style::default(), Some(Size::new(8.0, 9.0)));

        let mut engine = LayoutEngine::new();
        let size = engine.layout(&mut tree, root, Constraints::definite(30.0, 20.0));

        assert_close(size.width, 30.0);
        assert_close(size.height, 20.0);
        assert_eq!(tree.measure_calls[root], 1);
    }
}
