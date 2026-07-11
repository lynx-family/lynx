// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

//! Safe Rust standalone Starlight tree API.
//!
//! This crate owns tree storage and delegates layout to `starlight_layout`.
//! It is the Rust-side landing zone for standalone API tests that should not
//! depend on the C++ standalone container model.

#![forbid(unsafe_code)]

use std::fmt;

use starlight_layout::{
    AlignContent, AlignItems, BoxSizing, Constraints, Direction, Display, Edges, FlexDirection,
    FlexWrap, GridAutoFlow, JustifyContent, JustifyItems, LayoutEngine, LayoutResult, LayoutTree,
    Length, LinearCrossGravity, LinearGravity, LinearLayoutGravity, LinearOrientation,
    ListComponentType, PositionType, Rect, RelativeCenter, Size, Style, Visibility,
};

#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
pub struct NodeId(usize);

impl NodeId {
    pub const fn index(self) -> usize {
        self.0
    }
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum TreeError {
    MissingNode(NodeId),
    ReferenceNotChild { parent: NodeId, reference: NodeId },
    CannotParentNodeToItself(NodeId),
}

impl fmt::Display for TreeError {
    fn fmt(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::MissingNode(node) => write!(formatter, "missing standalone node {node:?}"),
            Self::ReferenceNotChild { parent, reference } => {
                write!(
                    formatter,
                    "reference {reference:?} is not attached to parent {parent:?}"
                )
            }
            Self::CannotParentNodeToItself(node) => {
                write!(formatter, "node {node:?} cannot be parented to itself")
            }
        }
    }
}

impl std::error::Error for TreeError {}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum StandaloneEdge {
    Left,
    Right,
    Top,
    Bottom,
    Start,
    End,
    Horizontal,
    Vertical,
    All,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum StandaloneGap {
    Column,
    Row,
    All,
}

pub fn standalone_default_style() -> Style {
    Style {
        display: Display::Flex,
        position: PositionType::Relative,
        box_sizing: BoxSizing::ContentBox,
        ..Style::default()
    }
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub struct StandaloneConfig {
    physical_pixels_per_layout_unit: f32,
}

impl StandaloneConfig {
    pub const DEFAULT_PHYSICAL_PIXELS_PER_LAYOUT_UNIT: f32 = 1.0;

    pub const fn new() -> Self {
        Self {
            physical_pixels_per_layout_unit: Self::DEFAULT_PHYSICAL_PIXELS_PER_LAYOUT_UNIT,
        }
    }

    pub const fn with_physical_pixels_per_layout_unit(
        physical_pixels_per_layout_unit: f32,
    ) -> Self {
        Self {
            physical_pixels_per_layout_unit,
        }
    }

    pub const fn physical_pixels_per_layout_unit(self) -> f32 {
        self.physical_pixels_per_layout_unit
    }

    pub fn set_physical_pixels_per_layout_unit(&mut self, physical_pixels_per_layout_unit: f32) {
        self.physical_pixels_per_layout_unit = physical_pixels_per_layout_unit;
    }
}

impl Default for StandaloneConfig {
    fn default() -> Self {
        Self::new()
    }
}

pub type StandaloneMeasureFunc = fn(Constraints) -> Size;
pub type StandaloneBaselineFunc = fn(Size) -> f32;

#[derive(Clone, Copy, Debug)]
enum StandaloneMeasurement {
    Static(Size),
    Callback(StandaloneMeasureFunc),
}

#[derive(Clone, Copy, Debug)]
enum StandaloneBaseline {
    Static(f32),
    Callback(StandaloneBaselineFunc),
}

#[derive(Clone, Debug)]
pub struct StandaloneNode {
    config: StandaloneConfig,
    style: Style,
    has_explicit_direction_style: bool,
    layout: LayoutResult,
    parent: Option<NodeId>,
    children: Vec<NodeId>,
    measurement: Option<StandaloneMeasurement>,
    baseline: Option<StandaloneBaseline>,
    dirty: bool,
}

impl StandaloneNode {
    pub fn default_standalone() -> Self {
        Self::new(standalone_default_style())
    }

    pub fn new(style: Style) -> Self {
        Self::new_with_config(style, StandaloneConfig::default())
    }

    pub fn new_with_config(style: Style, config: StandaloneConfig) -> Self {
        Self {
            config,
            has_explicit_direction_style: style.direction != Direction::Ltr,
            style,
            layout: LayoutResult::default(),
            parent: None,
            children: Vec::new(),
            measurement: None,
            baseline: None,
            dirty: false,
        }
    }

    pub fn measured(style: Style, measured_size: Size) -> Self {
        Self::measured_with_config(style, measured_size, StandaloneConfig::default())
    }

    pub fn measured_with_config(
        style: Style,
        measured_size: Size,
        config: StandaloneConfig,
    ) -> Self {
        Self {
            measurement: Some(StandaloneMeasurement::Static(measured_size)),
            ..Self::new_with_config(style, config)
        }
    }

    pub fn measured_with_baseline(style: Style, measured_size: Size, baseline: f32) -> Self {
        Self {
            measurement: Some(StandaloneMeasurement::Static(measured_size)),
            baseline: Some(StandaloneBaseline::Static(baseline)),
            ..Self::new(style)
        }
    }

    pub fn style(&self) -> &Style {
        &self.style
    }

    pub fn style_mut(&mut self) -> &mut Style {
        &mut self.style
    }

    pub fn has_explicit_direction_style(&self) -> bool {
        self.has_explicit_direction_style
    }

    pub fn config(&self) -> StandaloneConfig {
        self.config
    }

    pub fn physical_pixels_per_layout_unit(&self) -> f32 {
        self.config.physical_pixels_per_layout_unit()
    }

    pub fn layout(&self) -> LayoutResult {
        self.layout
    }

    pub fn parent(&self) -> Option<NodeId> {
        self.parent
    }

    pub fn children(&self) -> &[NodeId] {
        &self.children
    }

    pub fn is_dirty(&self) -> bool {
        self.dirty
    }

    pub fn set_measured_size(&mut self, measured_size: Option<Size>) {
        self.measurement = measured_size.map(StandaloneMeasurement::Static);
        self.dirty = true;
    }

    pub fn measured_size(&self) -> Option<Size> {
        match self.measurement {
            Some(StandaloneMeasurement::Static(size)) => Some(size),
            Some(StandaloneMeasurement::Callback(_)) | None => None,
        }
    }

    pub fn set_measure_func(&mut self, measure_func: Option<StandaloneMeasureFunc>) {
        self.measurement = measure_func.map(StandaloneMeasurement::Callback);
        self.dirty = true;
    }

    pub fn measure_func(&self) -> Option<StandaloneMeasureFunc> {
        match self.measurement {
            Some(StandaloneMeasurement::Callback(measure_func)) => Some(measure_func),
            Some(StandaloneMeasurement::Static(_)) | None => None,
        }
    }

    pub fn has_measure_func(&self) -> bool {
        self.measurement.is_some()
    }

    fn measure(&self, constraints: Constraints) -> Option<Size> {
        match self.measurement {
            Some(StandaloneMeasurement::Static(size)) => Some(size),
            Some(StandaloneMeasurement::Callback(measure_func)) => Some(measure_func(constraints)),
            None => None,
        }
    }

    pub fn set_baseline(&mut self, baseline: Option<f32>) {
        self.baseline = baseline.map(StandaloneBaseline::Static);
        self.dirty = true;
    }

    pub fn baseline(&self) -> Option<f32> {
        match self.baseline {
            Some(StandaloneBaseline::Static(baseline)) => Some(baseline),
            Some(StandaloneBaseline::Callback(_)) | None => None,
        }
    }

    pub fn set_baseline_func(&mut self, baseline_func: Option<StandaloneBaselineFunc>) {
        self.baseline = baseline_func.map(StandaloneBaseline::Callback);
        self.dirty = true;
    }

    pub fn baseline_func(&self) -> Option<StandaloneBaselineFunc> {
        match self.baseline {
            Some(StandaloneBaseline::Callback(baseline_func)) => Some(baseline_func),
            Some(StandaloneBaseline::Static(_)) | None => None,
        }
    }

    fn baseline_for_content_size(&self, content_size: Size) -> Option<f32> {
        match self.baseline {
            Some(StandaloneBaseline::Static(baseline)) => Some(baseline),
            Some(StandaloneBaseline::Callback(baseline_func)) => Some(baseline_func(content_size)),
            None => None,
        }
    }
}

#[derive(Clone, Debug, Default)]
pub struct StandaloneTree {
    nodes: Vec<StandaloneNode>,
}

impl StandaloneTree {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn create_node(&mut self, style: Style) -> NodeId {
        self.push(StandaloneNode::new(style))
    }

    pub fn create_node_with_config(&mut self, style: Style, config: StandaloneConfig) -> NodeId {
        self.push(StandaloneNode::new_with_config(style, config))
    }

    pub fn create_default_node(&mut self) -> NodeId {
        self.push(StandaloneNode::default_standalone())
    }

    pub fn create_default_node_with_config(&mut self, config: StandaloneConfig) -> NodeId {
        self.push(StandaloneNode::new_with_config(
            standalone_default_style(),
            config,
        ))
    }

    pub fn create_measured_node(&mut self, style: Style, measured_size: Size) -> NodeId {
        self.push(StandaloneNode::measured(style, measured_size))
    }

    pub fn create_default_measured_node(&mut self, measured_size: Size) -> NodeId {
        self.push(StandaloneNode::measured(
            standalone_default_style(),
            measured_size,
        ))
    }

    pub fn create_measured_node_with_baseline(
        &mut self,
        style: Style,
        measured_size: Size,
        baseline: f32,
    ) -> NodeId {
        self.push(StandaloneNode::measured_with_baseline(
            style,
            measured_size,
            baseline,
        ))
    }

    pub fn push(&mut self, node: StandaloneNode) -> NodeId {
        let id = NodeId(self.nodes.len());
        self.nodes.push(node);
        id
    }

    pub fn node(&self, node: NodeId) -> Result<&StandaloneNode, TreeError> {
        self.nodes.get(node.0).ok_or(TreeError::MissingNode(node))
    }

    pub fn node_mut(&mut self, node: NodeId) -> Result<&mut StandaloneNode, TreeError> {
        self.mark_dirty(node)?;
        self.nodes
            .get_mut(node.0)
            .ok_or(TreeError::MissingNode(node))
    }

    pub fn style(&self, node: NodeId) -> Result<&Style, TreeError> {
        Ok(self.node(node)?.style())
    }

    pub fn style_mut(&mut self, node: NodeId) -> Result<&mut Style, TreeError> {
        Ok(self.node_mut(node)?.style_mut())
    }

    pub fn set_measured_size(
        &mut self,
        node: NodeId,
        measured_size: Option<Size>,
    ) -> Result<(), TreeError> {
        self.ensure_node(node)?;
        self.nodes[node.0].measurement = measured_size.map(StandaloneMeasurement::Static);
        self.mark_dirty_internal(node);
        Ok(())
    }

    pub fn measured_size(&self, node: NodeId) -> Result<Option<Size>, TreeError> {
        Ok(self.node(node)?.measured_size())
    }

    pub fn set_measure_func(
        &mut self,
        node: NodeId,
        measure_func: Option<StandaloneMeasureFunc>,
    ) -> Result<(), TreeError> {
        self.ensure_node(node)?;
        self.nodes[node.0].measurement = measure_func.map(StandaloneMeasurement::Callback);
        self.mark_dirty_internal(node);
        Ok(())
    }

    pub fn measure_func(&self, node: NodeId) -> Result<Option<StandaloneMeasureFunc>, TreeError> {
        Ok(self.node(node)?.measure_func())
    }

    pub fn has_measure_func(&self, node: NodeId) -> Result<bool, TreeError> {
        Ok(self.node(node)?.has_measure_func())
    }

    pub fn set_baseline(&mut self, node: NodeId, baseline: Option<f32>) -> Result<(), TreeError> {
        self.ensure_node(node)?;
        self.nodes[node.0].baseline = baseline.map(StandaloneBaseline::Static);
        self.mark_dirty_internal(node);
        Ok(())
    }

    pub fn baseline(&self, node: NodeId) -> Result<Option<f32>, TreeError> {
        Ok(self.node(node)?.baseline())
    }

    pub fn set_baseline_func(
        &mut self,
        node: NodeId,
        baseline_func: Option<StandaloneBaselineFunc>,
    ) -> Result<(), TreeError> {
        self.ensure_node(node)?;
        self.nodes[node.0].baseline = baseline_func.map(StandaloneBaseline::Callback);
        self.mark_dirty_internal(node);
        Ok(())
    }

    pub fn baseline_func(&self, node: NodeId) -> Result<Option<StandaloneBaselineFunc>, TreeError> {
        Ok(self.node(node)?.baseline_func())
    }

    pub fn node_config(&self, node: NodeId) -> Result<StandaloneConfig, TreeError> {
        Ok(self.node(node)?.config())
    }

    pub fn physical_pixels_per_layout_unit(&self, node: NodeId) -> Result<f32, TreeError> {
        Ok(self.node(node)?.physical_pixels_per_layout_unit())
    }

    pub fn style_display(&self, node: NodeId) -> Result<Display, TreeError> {
        Ok(self.style(node)?.display)
    }

    pub fn style_direction(&self, node: NodeId) -> Result<Direction, TreeError> {
        Ok(self.style(node)?.direction)
    }

    pub fn style_visibility(&self, node: NodeId) -> Result<Visibility, TreeError> {
        Ok(self.style(node)?.visibility)
    }

    pub fn style_position_type(&self, node: NodeId) -> Result<PositionType, TreeError> {
        Ok(self.style(node)?.position)
    }

    pub fn style_box_sizing(&self, node: NodeId) -> Result<BoxSizing, TreeError> {
        Ok(self.style(node)?.box_sizing)
    }

    pub fn style_flex_direction(&self, node: NodeId) -> Result<FlexDirection, TreeError> {
        Ok(self.style(node)?.flex_direction)
    }

    pub fn style_flex_wrap(&self, node: NodeId) -> Result<FlexWrap, TreeError> {
        Ok(self.style(node)?.flex_wrap)
    }

    pub fn style_justify_content(&self, node: NodeId) -> Result<JustifyContent, TreeError> {
        Ok(self.style(node)?.justify_content)
    }

    pub fn style_align_content(&self, node: NodeId) -> Result<AlignContent, TreeError> {
        Ok(self.style(node)?.align_content)
    }

    pub fn style_align_items(&self, node: NodeId) -> Result<AlignItems, TreeError> {
        Ok(self.style(node)?.align_items)
    }

    pub fn style_align_self(&self, node: NodeId) -> Result<Option<AlignItems>, TreeError> {
        Ok(self.style(node)?.align_self)
    }

    pub fn style_justify_items(&self, node: NodeId) -> Result<JustifyItems, TreeError> {
        Ok(self.style(node)?.justify_items)
    }

    pub fn style_justify_self(&self, node: NodeId) -> Result<JustifyItems, TreeError> {
        Ok(self.style(node)?.justify_self)
    }

    pub fn style_aspect_ratio(&self, node: NodeId) -> Result<Option<f32>, TreeError> {
        Ok(self.style(node)?.aspect_ratio)
    }

    pub fn style_order(&self, node: NodeId) -> Result<i32, TreeError> {
        Ok(self.style(node)?.order)
    }

    pub fn style_flex_grow(&self, node: NodeId) -> Result<f32, TreeError> {
        Ok(self.style(node)?.flex_grow)
    }

    pub fn style_flex_shrink(&self, node: NodeId) -> Result<f32, TreeError> {
        Ok(self.style(node)?.flex_shrink)
    }

    pub fn style_width(&self, node: NodeId) -> Result<Length, TreeError> {
        Ok(self.style(node)?.width)
    }

    pub fn style_height(&self, node: NodeId) -> Result<Length, TreeError> {
        Ok(self.style(node)?.height)
    }

    pub fn style_min_width(&self, node: NodeId) -> Result<Length, TreeError> {
        Ok(self.style(node)?.min_width)
    }

    pub fn style_min_height(&self, node: NodeId) -> Result<Length, TreeError> {
        Ok(self.style(node)?.min_height)
    }

    pub fn style_max_width(&self, node: NodeId) -> Result<Length, TreeError> {
        Ok(self.style(node)?.max_width)
    }

    pub fn style_max_height(&self, node: NodeId) -> Result<Length, TreeError> {
        Ok(self.style(node)?.max_height)
    }

    pub fn style_flex_basis(&self, node: NodeId) -> Result<Length, TreeError> {
        Ok(self.style(node)?.flex_basis)
    }

    pub fn style_position(&self, node: NodeId, edge: StandaloneEdge) -> Result<Length, TreeError> {
        let style = self.style(node)?;
        Ok(resolve_style_length_edge(
            Rect::new(style.left, style.right, style.top, style.bottom),
            edge,
            style.direction.is_rtl(),
        ))
    }

    pub fn style_margin(&self, node: NodeId, edge: StandaloneEdge) -> Result<Length, TreeError> {
        let style = self.style(node)?;
        Ok(resolve_style_length_edge(
            style.margin,
            edge,
            style.direction.is_rtl(),
        ))
    }

    pub fn style_padding(&self, node: NodeId, edge: StandaloneEdge) -> Result<Length, TreeError> {
        let style = self.style(node)?;
        Ok(resolve_style_length_edge(
            style.padding,
            edge,
            style.direction.is_rtl(),
        ))
    }

    pub fn style_border(&self, node: NodeId, edge: StandaloneEdge) -> Result<f32, TreeError> {
        let style = self.style(node)?;
        Ok(resolve_style_edge(
            style.border,
            edge,
            style.direction.is_rtl(),
        ))
    }

    pub fn style_gap(&self, node: NodeId, gap: StandaloneGap) -> Result<Length, TreeError> {
        let style = self.style(node)?;
        Ok(match gap {
            StandaloneGap::Column => style.column_gap,
            StandaloneGap::Row | StandaloneGap::All => style.row_gap,
        })
    }

    pub fn style_row_gap(&self, node: NodeId) -> Result<Length, TreeError> {
        Ok(self.style(node)?.row_gap)
    }

    pub fn style_column_gap(&self, node: NodeId) -> Result<Length, TreeError> {
        Ok(self.style(node)?.column_gap)
    }

    pub fn style_linear_orientation(&self, node: NodeId) -> Result<LinearOrientation, TreeError> {
        Ok(self.style(node)?.linear_orientation)
    }

    pub fn style_linear_gravity(&self, node: NodeId) -> Result<LinearGravity, TreeError> {
        Ok(self.style(node)?.linear_gravity)
    }

    pub fn style_linear_layout_gravity(
        &self,
        node: NodeId,
    ) -> Result<LinearLayoutGravity, TreeError> {
        Ok(self.style(node)?.linear_layout_gravity)
    }

    pub fn style_linear_cross_gravity(
        &self,
        node: NodeId,
    ) -> Result<LinearCrossGravity, TreeError> {
        Ok(self.style(node)?.linear_cross_gravity)
    }

    pub fn style_linear_weight(&self, node: NodeId) -> Result<f32, TreeError> {
        Ok(self.style(node)?.linear_weight)
    }

    pub fn style_linear_weight_sum(&self, node: NodeId) -> Result<f32, TreeError> {
        Ok(self.style(node)?.linear_weight_sum)
    }

    pub fn style_linear_column_count(&self, node: NodeId) -> Result<Option<usize>, TreeError> {
        Ok(self.style(node)?.linear_column_count)
    }

    pub fn style_list_main_axis_gap(&self, node: NodeId) -> Result<Length, TreeError> {
        Ok(self.style(node)?.list_main_axis_gap)
    }

    pub fn style_list_cross_axis_gap(&self, node: NodeId) -> Result<Length, TreeError> {
        Ok(self.style(node)?.list_cross_axis_gap)
    }

    pub fn style_list_component_type(
        &self,
        node: NodeId,
    ) -> Result<Option<ListComponentType>, TreeError> {
        Ok(self.style(node)?.list_component_type)
    }

    pub fn style_grid_template_columns(&self, node: NodeId) -> Result<&[Length], TreeError> {
        Ok(self.style(node)?.grid_template_columns.as_slice())
    }

    pub fn style_grid_template_rows(&self, node: NodeId) -> Result<&[Length], TreeError> {
        Ok(self.style(node)?.grid_template_rows.as_slice())
    }

    pub fn style_grid_template_columns_max(&self, node: NodeId) -> Result<&[Length], TreeError> {
        Ok(self.style(node)?.grid_template_columns_max.as_slice())
    }

    pub fn style_grid_template_rows_max(&self, node: NodeId) -> Result<&[Length], TreeError> {
        Ok(self.style(node)?.grid_template_rows_max.as_slice())
    }

    pub fn style_grid_auto_columns(&self, node: NodeId) -> Result<&[Length], TreeError> {
        Ok(self.style(node)?.grid_auto_columns.as_slice())
    }

    pub fn style_grid_auto_rows(&self, node: NodeId) -> Result<&[Length], TreeError> {
        Ok(self.style(node)?.grid_auto_rows.as_slice())
    }

    pub fn style_grid_auto_columns_max(&self, node: NodeId) -> Result<&[Length], TreeError> {
        Ok(self.style(node)?.grid_auto_columns_max.as_slice())
    }

    pub fn style_grid_auto_rows_max(&self, node: NodeId) -> Result<&[Length], TreeError> {
        Ok(self.style(node)?.grid_auto_rows_max.as_slice())
    }

    pub fn style_grid_auto_flow(&self, node: NodeId) -> Result<GridAutoFlow, TreeError> {
        Ok(self.style(node)?.grid_auto_flow)
    }

    pub fn style_grid_column_start(&self, node: NodeId) -> Result<Option<i32>, TreeError> {
        Ok(self.style(node)?.grid_column_start)
    }

    pub fn style_grid_column_end(&self, node: NodeId) -> Result<Option<i32>, TreeError> {
        Ok(self.style(node)?.grid_column_end)
    }

    pub fn style_grid_row_start(&self, node: NodeId) -> Result<Option<i32>, TreeError> {
        Ok(self.style(node)?.grid_row_start)
    }

    pub fn style_grid_row_end(&self, node: NodeId) -> Result<Option<i32>, TreeError> {
        Ok(self.style(node)?.grid_row_end)
    }

    pub fn style_grid_column_span(&self, node: NodeId) -> Result<usize, TreeError> {
        Ok(self.style(node)?.grid_column_span)
    }

    pub fn style_grid_row_span(&self, node: NodeId) -> Result<usize, TreeError> {
        Ok(self.style(node)?.grid_row_span)
    }

    pub fn style_relative_id(&self, node: NodeId) -> Result<i32, TreeError> {
        Ok(self.style(node)?.relative_id)
    }

    pub fn style_relative_align_top(&self, node: NodeId) -> Result<i32, TreeError> {
        Ok(self.style(node)?.relative_align_top)
    }

    pub fn style_relative_align_right(&self, node: NodeId) -> Result<i32, TreeError> {
        Ok(self.style(node)?.relative_align_right)
    }

    pub fn style_relative_align_bottom(&self, node: NodeId) -> Result<i32, TreeError> {
        Ok(self.style(node)?.relative_align_bottom)
    }

    pub fn style_relative_align_left(&self, node: NodeId) -> Result<i32, TreeError> {
        Ok(self.style(node)?.relative_align_left)
    }

    pub fn style_relative_top_of(&self, node: NodeId) -> Result<i32, TreeError> {
        Ok(self.style(node)?.relative_top_of)
    }

    pub fn style_relative_right_of(&self, node: NodeId) -> Result<i32, TreeError> {
        Ok(self.style(node)?.relative_right_of)
    }

    pub fn style_relative_bottom_of(&self, node: NodeId) -> Result<i32, TreeError> {
        Ok(self.style(node)?.relative_bottom_of)
    }

    pub fn style_relative_left_of(&self, node: NodeId) -> Result<i32, TreeError> {
        Ok(self.style(node)?.relative_left_of)
    }

    pub fn style_relative_layout_once(&self, node: NodeId) -> Result<bool, TreeError> {
        Ok(self.style(node)?.relative_layout_once)
    }

    pub fn style_relative_center(&self, node: NodeId) -> Result<RelativeCenter, TreeError> {
        Ok(self.style(node)?.relative_center)
    }

    pub fn set_display(&mut self, node: NodeId, value: Display) -> Result<(), TreeError> {
        self.update_style(node, |style| style.display = value)
    }

    pub fn set_direction(&mut self, node: NodeId, value: Direction) -> Result<(), TreeError> {
        self.ensure_node(node)?;
        self.nodes[node.0].style.direction = value;
        self.nodes[node.0].has_explicit_direction_style = true;
        self.mark_dirty_internal(node);
        Ok(())
    }

    pub fn clear_direction(&mut self, node: NodeId) -> Result<(), TreeError> {
        self.ensure_node(node)?;
        self.nodes[node.0].style.direction = Direction::Ltr;
        self.nodes[node.0].has_explicit_direction_style = false;
        self.mark_dirty_internal(node);
        Ok(())
    }

    pub fn has_explicit_direction_style(&self, node: NodeId) -> Result<bool, TreeError> {
        Ok(self.node(node)?.has_explicit_direction_style())
    }

    pub fn set_position_type(
        &mut self,
        node: NodeId,
        value: PositionType,
    ) -> Result<(), TreeError> {
        self.update_style(node, |style| style.position = value)
    }

    pub fn set_box_sizing(&mut self, node: NodeId, value: BoxSizing) -> Result<(), TreeError> {
        self.update_style(node, |style| style.box_sizing = value)
    }

    pub fn set_flex_direction(
        &mut self,
        node: NodeId,
        value: FlexDirection,
    ) -> Result<(), TreeError> {
        self.update_style(node, |style| style.flex_direction = value)
    }

    pub fn set_flex_wrap(&mut self, node: NodeId, value: FlexWrap) -> Result<(), TreeError> {
        self.update_style(node, |style| style.flex_wrap = value)
    }

    pub fn set_justify_content(
        &mut self,
        node: NodeId,
        value: JustifyContent,
    ) -> Result<(), TreeError> {
        self.update_style(node, |style| style.justify_content = value)
    }

    pub fn set_align_content(
        &mut self,
        node: NodeId,
        value: AlignContent,
    ) -> Result<(), TreeError> {
        self.update_style(node, |style| style.align_content = value)
    }

    pub fn set_align_items(&mut self, node: NodeId, value: AlignItems) -> Result<(), TreeError> {
        self.update_style(node, |style| style.align_items = value)
    }

    pub fn set_align_self(
        &mut self,
        node: NodeId,
        value: Option<AlignItems>,
    ) -> Result<(), TreeError> {
        self.update_style(node, |style| style.align_self = value)
    }

    pub fn set_justify_items(
        &mut self,
        node: NodeId,
        value: JustifyItems,
    ) -> Result<(), TreeError> {
        self.update_style(node, |style| style.justify_items = value)
    }

    pub fn set_justify_self(&mut self, node: NodeId, value: JustifyItems) -> Result<(), TreeError> {
        self.update_style(node, |style| style.justify_self = value)
    }

    pub fn set_aspect_ratio(&mut self, node: NodeId, value: Option<f32>) -> Result<(), TreeError> {
        self.update_style(node, |style| style.aspect_ratio = value)
    }

    pub fn set_order(&mut self, node: NodeId, value: i32) -> Result<(), TreeError> {
        self.update_style(node, |style| style.order = value)
    }

    pub fn set_flex_grow(&mut self, node: NodeId, value: f32) -> Result<(), TreeError> {
        self.update_style(node, |style| style.flex_grow = value)
    }

    pub fn set_flex_shrink(&mut self, node: NodeId, value: f32) -> Result<(), TreeError> {
        self.update_style(node, |style| style.flex_shrink = value)
    }

    pub fn set_flex(&mut self, node: NodeId, value: f32) -> Result<(), TreeError> {
        self.update_style(node, |style| {
            style.flex_grow = value;
            style.flex_shrink = 1.0;
            style.flex_basis = Length::ZERO;
        })
    }

    pub fn set_row_gap(&mut self, node: NodeId, value: Length) -> Result<(), TreeError> {
        self.update_style(node, |style| style.row_gap = value)
    }

    pub fn set_column_gap(&mut self, node: NodeId, value: Length) -> Result<(), TreeError> {
        self.update_style(node, |style| style.column_gap = value)
    }

    pub fn set_gap(
        &mut self,
        node: NodeId,
        gap: StandaloneGap,
        value: Length,
    ) -> Result<(), TreeError> {
        self.update_style(node, |style| match gap {
            StandaloneGap::Column => style.column_gap = value,
            StandaloneGap::Row => style.row_gap = value,
            StandaloneGap::All => {
                style.column_gap = value;
                style.row_gap = value;
            }
        })
    }

    pub fn set_linear_orientation(
        &mut self,
        node: NodeId,
        value: LinearOrientation,
    ) -> Result<(), TreeError> {
        self.update_style(node, |style| style.linear_orientation = value)
    }

    pub fn set_linear_gravity(
        &mut self,
        node: NodeId,
        value: LinearGravity,
    ) -> Result<(), TreeError> {
        self.update_style(node, |style| style.linear_gravity = value)
    }

    pub fn set_linear_layout_gravity(
        &mut self,
        node: NodeId,
        value: LinearLayoutGravity,
    ) -> Result<(), TreeError> {
        self.update_style(node, |style| style.linear_layout_gravity = value)
    }

    pub fn set_linear_cross_gravity(
        &mut self,
        node: NodeId,
        value: LinearCrossGravity,
    ) -> Result<(), TreeError> {
        self.update_style(node, |style| style.linear_cross_gravity = value)
    }

    pub fn set_linear_weight(&mut self, node: NodeId, value: f32) -> Result<(), TreeError> {
        self.update_style(node, |style| style.linear_weight = value)
    }

    pub fn set_linear_weight_sum(&mut self, node: NodeId, value: f32) -> Result<(), TreeError> {
        self.update_style(node, |style| style.linear_weight_sum = value)
    }

    pub fn set_linear_column_count(
        &mut self,
        node: NodeId,
        value: Option<usize>,
    ) -> Result<(), TreeError> {
        self.update_style(node, |style| style.linear_column_count = value)
    }

    pub fn set_list_main_axis_gap(&mut self, node: NodeId, value: Length) -> Result<(), TreeError> {
        self.update_style(node, |style| style.list_main_axis_gap = value)
    }

    pub fn set_list_cross_axis_gap(
        &mut self,
        node: NodeId,
        value: Length,
    ) -> Result<(), TreeError> {
        self.update_style(node, |style| style.list_cross_axis_gap = value)
    }

    pub fn set_list_component_type(
        &mut self,
        node: NodeId,
        value: Option<ListComponentType>,
    ) -> Result<(), TreeError> {
        self.update_style(node, |style| style.list_component_type = value)
    }

    pub fn set_grid_template_columns(
        &mut self,
        node: NodeId,
        values: impl Into<Vec<Length>>,
    ) -> Result<(), TreeError> {
        self.update_style(node, |style| style.grid_template_columns = values.into())
    }

    pub fn set_grid_template_rows(
        &mut self,
        node: NodeId,
        values: impl Into<Vec<Length>>,
    ) -> Result<(), TreeError> {
        self.update_style(node, |style| style.grid_template_rows = values.into())
    }

    pub fn set_grid_template_columns_max(
        &mut self,
        node: NodeId,
        values: impl Into<Vec<Length>>,
    ) -> Result<(), TreeError> {
        self.update_style(node, |style| {
            style.grid_template_columns_max = values.into();
        })
    }

    pub fn set_grid_template_rows_max(
        &mut self,
        node: NodeId,
        values: impl Into<Vec<Length>>,
    ) -> Result<(), TreeError> {
        self.update_style(node, |style| style.grid_template_rows_max = values.into())
    }

    pub fn set_grid_auto_columns(
        &mut self,
        node: NodeId,
        values: impl Into<Vec<Length>>,
    ) -> Result<(), TreeError> {
        self.update_style(node, |style| style.grid_auto_columns = values.into())
    }

    pub fn set_grid_auto_rows(
        &mut self,
        node: NodeId,
        values: impl Into<Vec<Length>>,
    ) -> Result<(), TreeError> {
        self.update_style(node, |style| style.grid_auto_rows = values.into())
    }

    pub fn set_grid_auto_columns_max(
        &mut self,
        node: NodeId,
        values: impl Into<Vec<Length>>,
    ) -> Result<(), TreeError> {
        self.update_style(node, |style| style.grid_auto_columns_max = values.into())
    }

    pub fn set_grid_auto_rows_max(
        &mut self,
        node: NodeId,
        values: impl Into<Vec<Length>>,
    ) -> Result<(), TreeError> {
        self.update_style(node, |style| style.grid_auto_rows_max = values.into())
    }

    pub fn set_grid_auto_flow(
        &mut self,
        node: NodeId,
        value: GridAutoFlow,
    ) -> Result<(), TreeError> {
        self.update_style(node, |style| style.grid_auto_flow = value)
    }

    pub fn set_grid_column_start(
        &mut self,
        node: NodeId,
        value: Option<i32>,
    ) -> Result<(), TreeError> {
        self.update_style(node, |style| style.grid_column_start = value)
    }

    pub fn set_grid_column_end(
        &mut self,
        node: NodeId,
        value: Option<i32>,
    ) -> Result<(), TreeError> {
        self.update_style(node, |style| style.grid_column_end = value)
    }

    pub fn set_grid_row_start(
        &mut self,
        node: NodeId,
        value: Option<i32>,
    ) -> Result<(), TreeError> {
        self.update_style(node, |style| style.grid_row_start = value)
    }

    pub fn set_grid_row_end(&mut self, node: NodeId, value: Option<i32>) -> Result<(), TreeError> {
        self.update_style(node, |style| style.grid_row_end = value)
    }

    pub fn set_grid_column_span(&mut self, node: NodeId, value: usize) -> Result<(), TreeError> {
        self.update_style(node, |style| style.grid_column_span = value)
    }

    pub fn set_grid_row_span(&mut self, node: NodeId, value: usize) -> Result<(), TreeError> {
        self.update_style(node, |style| style.grid_row_span = value)
    }

    pub fn set_relative_id(&mut self, node: NodeId, value: i32) -> Result<(), TreeError> {
        self.update_style(node, |style| style.relative_id = value)
    }

    pub fn set_relative_align_top(&mut self, node: NodeId, value: i32) -> Result<(), TreeError> {
        self.update_style(node, |style| style.relative_align_top = value)
    }

    pub fn set_relative_align_right(&mut self, node: NodeId, value: i32) -> Result<(), TreeError> {
        self.update_style(node, |style| style.relative_align_right = value)
    }

    pub fn set_relative_align_bottom(&mut self, node: NodeId, value: i32) -> Result<(), TreeError> {
        self.update_style(node, |style| style.relative_align_bottom = value)
    }

    pub fn set_relative_align_left(&mut self, node: NodeId, value: i32) -> Result<(), TreeError> {
        self.update_style(node, |style| style.relative_align_left = value)
    }

    pub fn set_relative_top_of(&mut self, node: NodeId, value: i32) -> Result<(), TreeError> {
        self.update_style(node, |style| style.relative_top_of = value)
    }

    pub fn set_relative_right_of(&mut self, node: NodeId, value: i32) -> Result<(), TreeError> {
        self.update_style(node, |style| style.relative_right_of = value)
    }

    pub fn set_relative_bottom_of(&mut self, node: NodeId, value: i32) -> Result<(), TreeError> {
        self.update_style(node, |style| style.relative_bottom_of = value)
    }

    pub fn set_relative_left_of(&mut self, node: NodeId, value: i32) -> Result<(), TreeError> {
        self.update_style(node, |style| style.relative_left_of = value)
    }

    pub fn set_relative_layout_once(&mut self, node: NodeId, value: bool) -> Result<(), TreeError> {
        self.update_style(node, |style| style.relative_layout_once = value)
    }

    pub fn set_relative_center(
        &mut self,
        node: NodeId,
        value: RelativeCenter,
    ) -> Result<(), TreeError> {
        self.update_style(node, |style| style.relative_center = value)
    }

    pub fn set_position(
        &mut self,
        node: NodeId,
        edge: StandaloneEdge,
        value: Length,
    ) -> Result<(), TreeError> {
        self.ensure_node(node)?;
        let is_rtl = self.nodes[node.0].style.direction.is_rtl();
        apply_position_edge(&mut self.nodes[node.0].style, edge, value, is_rtl);
        self.mark_dirty_internal(node);
        Ok(())
    }

    pub fn set_margin(
        &mut self,
        node: NodeId,
        edge: StandaloneEdge,
        value: Length,
    ) -> Result<(), TreeError> {
        self.ensure_node(node)?;
        let is_rtl = self.nodes[node.0].style.direction.is_rtl();
        apply_length_edge(&mut self.nodes[node.0].style.margin, edge, value, is_rtl);
        self.mark_dirty_internal(node);
        Ok(())
    }

    pub fn set_padding(
        &mut self,
        node: NodeId,
        edge: StandaloneEdge,
        value: Length,
    ) -> Result<(), TreeError> {
        self.ensure_node(node)?;
        let is_rtl = self.nodes[node.0].style.direction.is_rtl();
        apply_length_edge(&mut self.nodes[node.0].style.padding, edge, value, is_rtl);
        self.mark_dirty_internal(node);
        Ok(())
    }

    pub fn set_border(
        &mut self,
        node: NodeId,
        edge: StandaloneEdge,
        value: f32,
    ) -> Result<(), TreeError> {
        self.ensure_node(node)?;
        let is_rtl = self.nodes[node.0].style.direction.is_rtl();
        apply_f32_edge(&mut self.nodes[node.0].style.border, edge, value, is_rtl);
        self.mark_dirty_internal(node);
        Ok(())
    }

    pub fn set_width(&mut self, node: NodeId, value: Length) -> Result<(), TreeError> {
        self.ensure_node(node)?;
        self.nodes[node.0].style.width = value;
        self.mark_dirty_internal(node);
        Ok(())
    }

    pub fn set_height(&mut self, node: NodeId, value: Length) -> Result<(), TreeError> {
        self.ensure_node(node)?;
        self.nodes[node.0].style.height = value;
        self.mark_dirty_internal(node);
        Ok(())
    }

    pub fn set_min_width(&mut self, node: NodeId, value: Length) -> Result<(), TreeError> {
        self.ensure_node(node)?;
        self.nodes[node.0].style.min_width = value;
        self.mark_dirty_internal(node);
        Ok(())
    }

    pub fn set_min_height(&mut self, node: NodeId, value: Length) -> Result<(), TreeError> {
        self.ensure_node(node)?;
        self.nodes[node.0].style.min_height = value;
        self.mark_dirty_internal(node);
        Ok(())
    }

    pub fn set_max_width(&mut self, node: NodeId, value: Length) -> Result<(), TreeError> {
        self.ensure_node(node)?;
        self.nodes[node.0].style.max_width = value;
        self.mark_dirty_internal(node);
        Ok(())
    }

    pub fn set_max_height(&mut self, node: NodeId, value: Length) -> Result<(), TreeError> {
        self.ensure_node(node)?;
        self.nodes[node.0].style.max_height = value;
        self.mark_dirty_internal(node);
        Ok(())
    }

    pub fn set_flex_basis(&mut self, node: NodeId, value: Length) -> Result<(), TreeError> {
        self.ensure_node(node)?;
        self.nodes[node.0].style.flex_basis = value;
        self.mark_dirty_internal(node);
        Ok(())
    }

    pub fn layout(&self, node: NodeId) -> Result<LayoutResult, TreeError> {
        Ok(self.node(node)?.layout())
    }

    pub fn layout_left(&self, node: NodeId) -> Result<f32, TreeError> {
        Ok(self.node(node)?.layout().offset.x)
    }

    pub fn layout_top(&self, node: NodeId) -> Result<f32, TreeError> {
        Ok(self.node(node)?.layout().offset.y)
    }

    pub fn layout_width(&self, node: NodeId) -> Result<f32, TreeError> {
        Ok(self.node(node)?.layout().size.width)
    }

    pub fn layout_height(&self, node: NodeId) -> Result<f32, TreeError> {
        Ok(self.node(node)?.layout().size.height)
    }

    pub fn layout_baseline(&self, node: NodeId) -> Result<f32, TreeError> {
        let layout = self.node(node)?.layout();
        Ok(layout.baseline.unwrap_or(layout.size.height))
    }

    pub fn layout_margin(&self, node: NodeId, edge: StandaloneEdge) -> Result<f32, TreeError> {
        self.layout_edge(node, edge, |layout| layout.margin)
    }

    pub fn layout_padding(&self, node: NodeId, edge: StandaloneEdge) -> Result<f32, TreeError> {
        self.layout_edge(node, edge, |layout| layout.padding)
    }

    pub fn layout_border(&self, node: NodeId, edge: StandaloneEdge) -> Result<f32, TreeError> {
        self.layout_edge(node, edge, |layout| layout.border)
    }

    pub fn layout_sticky_position(
        &self,
        node: NodeId,
        edge: StandaloneEdge,
    ) -> Result<f32, TreeError> {
        self.layout_edge(node, edge, |layout| layout.sticky_pos)
    }

    pub fn parent(&self, node: NodeId) -> Result<Option<NodeId>, TreeError> {
        Ok(self.node(node)?.parent())
    }

    pub fn children(&self, node: NodeId) -> Result<&[NodeId], TreeError> {
        Ok(self.node(node)?.children())
    }

    pub fn child_count(&self, node: NodeId) -> Result<usize, TreeError> {
        Ok(self.node(node)?.children().len())
    }

    pub fn child_at(&self, node: NodeId, index: usize) -> Result<Option<NodeId>, TreeError> {
        Ok(self.node(node)?.children().get(index).copied())
    }

    pub fn child_at_standalone_index(
        &self,
        node: NodeId,
        index: i32,
    ) -> Result<Option<NodeId>, TreeError> {
        if index < 0 {
            Ok(None)
        } else {
            self.child_at(node, index as usize)
        }
    }

    pub fn is_rtl(&self, node: NodeId) -> Result<bool, TreeError> {
        Ok(self.node(node)?.style().direction.is_rtl())
    }

    pub fn is_dirty(&self, node: NodeId) -> Result<bool, TreeError> {
        Ok(self.node(node)?.is_dirty())
    }

    pub fn mark_dirty(&mut self, node: NodeId) -> Result<(), TreeError> {
        self.ensure_node(node)?;
        self.mark_dirty_internal(node);
        Ok(())
    }

    pub fn append_child(&mut self, parent: NodeId, child: NodeId) -> Result<(), TreeError> {
        let index = self.children(parent)?.len();
        self.insert_child(parent, child, index)
    }

    pub fn insert_child_or_append(
        &mut self,
        parent: NodeId,
        child: NodeId,
        index: Option<usize>,
    ) -> Result<(), TreeError> {
        match index {
            Some(index) => self.insert_child(parent, child, index),
            None => self.insert_child_at_standalone_index(parent, child, -1),
        }
    }

    pub fn insert_child_at_standalone_index(
        &mut self,
        parent: NodeId,
        child: NodeId,
        index: i32,
    ) -> Result<(), TreeError> {
        if index < 0 {
            self.append_child(parent, child)
        } else {
            self.insert_child(parent, child, index as usize)
        }
    }

    pub fn insert_child(
        &mut self,
        parent: NodeId,
        child: NodeId,
        index: usize,
    ) -> Result<(), TreeError> {
        if parent == child {
            return Err(TreeError::CannotParentNodeToItself(parent));
        }
        self.ensure_node(parent)?;
        self.ensure_node(child)?;
        self.detach_from_parent(child);

        let insertion_index = index.min(self.nodes[parent.0].children.len());
        self.nodes[parent.0].children.insert(insertion_index, child);
        self.nodes[child.0].parent = Some(parent);
        self.mark_dirty_internal(parent);
        Ok(())
    }

    pub fn insert_child_before(
        &mut self,
        parent: NodeId,
        child: NodeId,
        reference: NodeId,
    ) -> Result<(), TreeError> {
        if parent == child {
            return Err(TreeError::CannotParentNodeToItself(parent));
        }
        self.ensure_node(parent)?;
        self.ensure_node(child)?;
        self.ensure_node(reference)?;
        if !self.nodes[parent.0].children.contains(&reference) {
            return Err(TreeError::ReferenceNotChild { parent, reference });
        }
        if child == reference {
            return Ok(());
        }

        self.detach_from_parent(child);
        let index = self.nodes[parent.0]
            .children
            .iter()
            .position(|candidate| *candidate == reference)
            .expect("reference remains attached after moving a different child");
        self.nodes[parent.0].children.insert(index, child);
        self.nodes[child.0].parent = Some(parent);
        self.mark_dirty_internal(parent);
        Ok(())
    }

    pub fn insert_child_before_or_append(
        &mut self,
        parent: NodeId,
        child: NodeId,
        reference: Option<NodeId>,
    ) -> Result<(), TreeError> {
        match reference {
            Some(reference) => self.insert_child_before(parent, child, reference),
            None => self.append_child(parent, child),
        }
    }

    pub fn remove_child(&mut self, parent: NodeId, child: NodeId) -> Result<(), TreeError> {
        self.ensure_node(parent)?;
        self.ensure_node(child)?;
        let Some(index) = self.nodes[parent.0]
            .children
            .iter()
            .position(|candidate| *candidate == child)
        else {
            return Ok(());
        };

        self.nodes[parent.0].children.remove(index);
        self.nodes[child.0].parent = None;
        self.mark_dirty_internal(parent);
        Ok(())
    }

    pub fn remove_all_children(&mut self, parent: NodeId) -> Result<(), TreeError> {
        self.ensure_node(parent)?;
        let children = std::mem::take(&mut self.nodes[parent.0].children);
        for child in children {
            self.nodes[child.0].parent = None;
        }
        self.mark_dirty_internal(parent);
        Ok(())
    }

    pub fn reset_node(&mut self, node: NodeId) -> Result<(), TreeError> {
        self.ensure_node(node)?;
        let children = std::mem::take(&mut self.nodes[node.0].children);
        for child in children {
            self.nodes[child.0].parent = None;
        }
        let parent = self.nodes[node.0].parent;
        let config = self.nodes[node.0].config;
        self.nodes[node.0] = StandaloneNode {
            config,
            parent,
            dirty: false,
            ..StandaloneNode::default_standalone()
        };
        Ok(())
    }

    pub fn calculate_layout(
        &mut self,
        root: NodeId,
        owner_size: Size,
        owner_direction: Direction,
    ) -> Result<Size, TreeError> {
        self.calculate_layout_with_mode(
            root,
            Constraints::definite(owner_size.width, owner_size.height),
            owner_direction,
        )
    }

    pub fn calculate_layout_with_mode(
        &mut self,
        root: NodeId,
        owner_constraints: Constraints,
        owner_direction: Direction,
    ) -> Result<Size, TreeError> {
        self.ensure_node(root)?;
        let previous_directions =
            self.apply_owner_direction_to_unset_subtree(root, owner_direction);
        let result =
            LayoutEngine::new().layout_with_owner_constraints(self, root, owner_constraints);
        self.restore_directions(previous_directions);
        self.mark_subtree_clean(root);
        Ok(result)
    }

    fn ensure_node(&self, node: NodeId) -> Result<(), TreeError> {
        self.nodes
            .get(node.0)
            .map(|_| ())
            .ok_or(TreeError::MissingNode(node))
    }

    fn update_style(
        &mut self,
        node: NodeId,
        update: impl FnOnce(&mut Style),
    ) -> Result<(), TreeError> {
        self.ensure_node(node)?;
        update(&mut self.nodes[node.0].style);
        self.mark_dirty_internal(node);
        Ok(())
    }

    fn layout_edge(
        &self,
        node: NodeId,
        edge: StandaloneEdge,
        select_edges: impl FnOnce(LayoutResult) -> Edges,
    ) -> Result<f32, TreeError> {
        let node = self.node(node)?;
        let edges = select_edges(node.layout());
        Ok(resolve_edge(edges, edge, node.style().direction.is_rtl()))
    }

    fn detach_from_parent(&mut self, child: NodeId) {
        let Some(existing_parent) = self.nodes[child.0].parent else {
            return;
        };
        if let Some(index) = self.nodes[existing_parent.0]
            .children
            .iter()
            .position(|candidate| *candidate == child)
        {
            self.nodes[existing_parent.0].children.remove(index);
        }
        self.nodes[child.0].parent = None;
    }

    fn mark_dirty_internal(&mut self, node: NodeId) {
        if !self.nodes[node.0].dirty {
            self.nodes[node.0].dirty = true;
            if let Some(parent) = self.nodes[node.0].parent {
                self.mark_dirty_internal(parent);
            }
        }
    }

    fn mark_subtree_clean(&mut self, node: NodeId) {
        self.nodes[node.0].dirty = false;
        let children = self.nodes[node.0].children.clone();
        for child in children {
            self.mark_subtree_clean(child);
        }
    }

    fn apply_owner_direction_to_unset_subtree(
        &mut self,
        node: NodeId,
        owner_direction: Direction,
    ) -> Vec<(NodeId, Direction)> {
        let mut previous_directions = Vec::new();
        self.apply_owner_direction_to_unset_subtree_inner(
            node,
            standalone_layout_direction(owner_direction),
            &mut previous_directions,
        );
        previous_directions
    }

    fn apply_owner_direction_to_unset_subtree_inner(
        &mut self,
        node: NodeId,
        layout_direction: Direction,
        previous_directions: &mut Vec<(NodeId, Direction)>,
    ) {
        if !self.nodes[node.0].has_explicit_direction_style {
            previous_directions.push((node, self.nodes[node.0].style.direction));
            self.nodes[node.0].style.direction = layout_direction;
        }

        let children = self.nodes[node.0].children.clone();
        for child in children {
            self.apply_owner_direction_to_unset_subtree_inner(
                child,
                layout_direction,
                previous_directions,
            );
        }
    }

    fn restore_directions(&mut self, previous_directions: Vec<(NodeId, Direction)>) {
        for (node, direction) in previous_directions {
            self.nodes[node.0].style.direction = direction;
        }
    }
}

fn standalone_layout_direction(owner_direction: Direction) -> Direction {
    if owner_direction.is_any_rtl() {
        Direction::Rtl
    } else {
        Direction::Ltr
    }
}

fn resolve_edge(edges: Edges, edge: StandaloneEdge, is_rtl: bool) -> f32 {
    match edge {
        StandaloneEdge::Left
        | StandaloneEdge::Horizontal
        | StandaloneEdge::Vertical
        | StandaloneEdge::All => edges.left,
        StandaloneEdge::Right => edges.right,
        StandaloneEdge::Top => edges.top,
        StandaloneEdge::Bottom => edges.bottom,
        StandaloneEdge::Start => {
            if is_rtl {
                edges.right
            } else {
                edges.left
            }
        }
        StandaloneEdge::End => {
            if is_rtl {
                edges.left
            } else {
                edges.right
            }
        }
    }
}

fn resolve_style_length_edge(edges: Rect<Length>, edge: StandaloneEdge, is_rtl: bool) -> Length {
    match edge {
        StandaloneEdge::Left => edges.left,
        StandaloneEdge::Right => edges.right,
        StandaloneEdge::Top => edges.top,
        StandaloneEdge::Bottom => edges.bottom,
        StandaloneEdge::Start => {
            if is_rtl {
                edges.right
            } else {
                edges.left
            }
        }
        StandaloneEdge::End => {
            if is_rtl {
                edges.left
            } else {
                edges.right
            }
        }
        StandaloneEdge::Horizontal | StandaloneEdge::Vertical | StandaloneEdge::All => Length::ZERO,
    }
}

fn resolve_style_edge(edges: Edges, edge: StandaloneEdge, is_rtl: bool) -> f32 {
    match edge {
        StandaloneEdge::Left => edges.left,
        StandaloneEdge::Right => edges.right,
        StandaloneEdge::Top => edges.top,
        StandaloneEdge::Bottom => edges.bottom,
        StandaloneEdge::Start => {
            if is_rtl {
                edges.right
            } else {
                edges.left
            }
        }
        StandaloneEdge::End => {
            if is_rtl {
                edges.left
            } else {
                edges.right
            }
        }
        StandaloneEdge::Horizontal | StandaloneEdge::Vertical | StandaloneEdge::All => 0.0,
    }
}

fn apply_position_edge(style: &mut Style, edge: StandaloneEdge, value: Length, is_rtl: bool) {
    match edge {
        StandaloneEdge::Left => style.left = value,
        StandaloneEdge::Right => style.right = value,
        StandaloneEdge::Top => style.top = value,
        StandaloneEdge::Bottom => style.bottom = value,
        StandaloneEdge::Start => {
            if is_rtl {
                style.right = value;
            } else {
                style.left = value;
            }
        }
        StandaloneEdge::End => {
            if is_rtl {
                style.left = value;
            } else {
                style.right = value;
            }
        }
        StandaloneEdge::Horizontal => {
            style.left = value;
            style.right = value;
        }
        StandaloneEdge::Vertical => {
            style.top = value;
            style.bottom = value;
        }
        StandaloneEdge::All => {
            style.left = value;
            style.right = value;
            style.top = value;
            style.bottom = value;
        }
    }
}

fn apply_length_edge(edges: &mut Rect<Length>, edge: StandaloneEdge, value: Length, is_rtl: bool) {
    match edge {
        StandaloneEdge::Left => edges.left = value,
        StandaloneEdge::Right => edges.right = value,
        StandaloneEdge::Top => edges.top = value,
        StandaloneEdge::Bottom => edges.bottom = value,
        StandaloneEdge::Start => {
            if is_rtl {
                edges.right = value;
            } else {
                edges.left = value;
            }
        }
        StandaloneEdge::End => {
            if is_rtl {
                edges.left = value;
            } else {
                edges.right = value;
            }
        }
        StandaloneEdge::Horizontal => {
            edges.left = value;
            edges.right = value;
        }
        StandaloneEdge::Vertical => {
            edges.top = value;
            edges.bottom = value;
        }
        StandaloneEdge::All => *edges = Rect::all(value),
    }
}

fn apply_f32_edge(edges: &mut Edges, edge: StandaloneEdge, value: f32, is_rtl: bool) {
    match edge {
        StandaloneEdge::Left => edges.left = value,
        StandaloneEdge::Right => edges.right = value,
        StandaloneEdge::Top => edges.top = value,
        StandaloneEdge::Bottom => edges.bottom = value,
        StandaloneEdge::Start => {
            if is_rtl {
                edges.right = value;
            } else {
                edges.left = value;
            }
        }
        StandaloneEdge::End => {
            if is_rtl {
                edges.left = value;
            } else {
                edges.right = value;
            }
        }
        StandaloneEdge::Horizontal => {
            edges.left = value;
            edges.right = value;
        }
        StandaloneEdge::Vertical => {
            edges.top = value;
            edges.bottom = value;
        }
        StandaloneEdge::All => *edges = Rect::all(value),
    }
}

impl LayoutTree for StandaloneTree {
    type NodeId = NodeId;
    type Children<'a> = std::iter::Copied<std::slice::Iter<'a, NodeId>>;

    fn children(&self, node: Self::NodeId) -> Self::Children<'_> {
        self.nodes[node.0].children.iter().copied()
    }

    fn style(&self, node: Self::NodeId) -> &Style {
        &self.nodes[node.0].style
    }

    fn has_explicit_direction_style(&self, node: Self::NodeId) -> bool {
        self.nodes[node.0].has_explicit_direction_style()
    }

    fn set_layout(&mut self, node: Self::NodeId, layout: LayoutResult) {
        self.nodes[node.0].layout = layout;
    }

    fn layout(&self, node: Self::NodeId) -> Option<LayoutResult> {
        Some(self.nodes[node.0].layout)
    }

    fn measure(&mut self, node: Self::NodeId, constraints: Constraints) -> Option<Size> {
        self.nodes[node.0].measure(constraints)
    }

    fn has_measure(&self, node: Self::NodeId) -> bool {
        self.nodes[node.0].has_measure_func()
    }

    fn physical_pixels_per_layout_unit(&self, node: Self::NodeId) -> f32 {
        self.nodes[node.0].physical_pixels_per_layout_unit()
    }

    fn baseline(&self, node: Self::NodeId, content_size: Size) -> Option<f32> {
        self.nodes[node.0].baseline_for_content_size(content_size)
    }
}
