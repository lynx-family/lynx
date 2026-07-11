// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use crate::engine::LayoutTree;
use crate::style::Style;
use crate::types::{Constraints, LayoutResult, Size};

pub type SimpleMeasureFunc = fn(Constraints) -> Size;
pub type SimpleBaselineFunc = fn(Size) -> f32;

pub trait LayoutNode {
    fn style(&self) -> &Style;
    fn style_mut(&mut self) -> &mut Style;
    fn layout(&self) -> LayoutResult;
    fn set_layout(&mut self, layout: LayoutResult);

    fn measure(&mut self, _constraints: Constraints) -> Option<Size> {
        None
    }

    fn baseline(&self, _content_size: Size) -> Option<f32> {
        None
    }
}

#[derive(Clone, Debug)]
pub struct SimpleNode {
    pub style: Style,
    pub has_explicit_direction_style: bool,
    pub layout: LayoutResult,
    pub children: Vec<usize>,
    pub measured_size: Option<Size>,
    pub measure_func: Option<SimpleMeasureFunc>,
    pub baseline: Option<f32>,
    pub baseline_func: Option<SimpleBaselineFunc>,
}

impl SimpleNode {
    pub fn new(style: Style) -> Self {
        Self {
            style,
            has_explicit_direction_style: true,
            layout: LayoutResult::default(),
            children: Vec::new(),
            measured_size: None,
            measure_func: None,
            baseline: None,
            baseline_func: None,
        }
    }

    pub fn with_measured_size(style: Style, measured_size: Size) -> Self {
        Self {
            measured_size: Some(measured_size),
            ..Self::new(style)
        }
    }

    pub fn with_inherited_direction_style(style: Style) -> Self {
        Self {
            has_explicit_direction_style: false,
            ..Self::new(style)
        }
    }

    pub fn with_measure_func(style: Style, measure_func: SimpleMeasureFunc) -> Self {
        Self {
            measure_func: Some(measure_func),
            ..Self::new(style)
        }
    }

    pub fn with_measured_size_and_baseline(
        style: Style,
        measured_size: Size,
        baseline: f32,
    ) -> Self {
        Self {
            measured_size: Some(measured_size),
            baseline: Some(baseline),
            ..Self::new(style)
        }
    }

    pub fn with_measure_func_and_baseline(
        style: Style,
        measure_func: SimpleMeasureFunc,
        baseline_func: SimpleBaselineFunc,
    ) -> Self {
        Self {
            measure_func: Some(measure_func),
            baseline_func: Some(baseline_func),
            ..Self::new(style)
        }
    }
}

impl LayoutNode for SimpleNode {
    fn style(&self) -> &Style {
        &self.style
    }

    fn style_mut(&mut self) -> &mut Style {
        &mut self.style
    }

    fn layout(&self) -> LayoutResult {
        self.layout
    }

    fn set_layout(&mut self, layout: LayoutResult) {
        self.layout = layout;
    }

    fn measure(&mut self, constraints: Constraints) -> Option<Size> {
        match (self.measured_size, self.measure_func) {
            (Some(size), _) => Some(size),
            (None, Some(measure_func)) => Some(measure_func(constraints)),
            (None, None) => None,
        }
    }

    fn baseline(&self, content_size: Size) -> Option<f32> {
        match (self.baseline, self.baseline_func) {
            (Some(baseline), _) => Some(baseline),
            (None, Some(baseline_func)) => Some(baseline_func(content_size)),
            (None, None) => None,
        }
    }
}

#[derive(Clone, Debug, Default)]
pub struct SimpleTree {
    pub nodes: Vec<SimpleNode>,
}

impl SimpleTree {
    pub fn push(&mut self, node: SimpleNode) -> usize {
        let id = self.nodes.len();
        self.nodes.push(node);
        id
    }

    pub fn append_child(&mut self, parent: usize, child: usize) {
        self.nodes[parent].children.push(child);
    }
}

impl LayoutTree for SimpleTree {
    type NodeId = usize;
    type Children<'a> = std::iter::Copied<std::slice::Iter<'a, usize>>;

    fn children(&self, node: Self::NodeId) -> Self::Children<'_> {
        self.nodes[node].children.iter().copied()
    }

    fn style(&self, node: Self::NodeId) -> &Style {
        self.nodes[node].style()
    }

    fn has_explicit_direction_style(&self, node: Self::NodeId) -> bool {
        self.nodes[node].has_explicit_direction_style
    }

    fn measure(&mut self, node: Self::NodeId, constraints: Constraints) -> Option<Size> {
        self.nodes[node].measure(constraints)
    }

    fn has_measure(&self, node: Self::NodeId) -> bool {
        let node = &self.nodes[node];
        node.measured_size.is_some() || node.measure_func.is_some()
    }

    fn baseline(&self, node: Self::NodeId, content_size: Size) -> Option<f32> {
        self.nodes[node].baseline(content_size)
    }

    fn set_layout(&mut self, node: Self::NodeId, layout: LayoutResult) {
        self.nodes[node].set_layout(layout);
    }

    fn layout(&self, node: Self::NodeId) -> Option<LayoutResult> {
        Some(self.nodes[node].layout())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::types::{MeasureMode, SideConstraint};

    fn callback_measure(constraints: Constraints) -> Size {
        let width = match constraints.width.mode {
            MeasureMode::AtMost => constraints.width.size - 3.0,
            MeasureMode::Definite => constraints.width.size + 5.0,
            MeasureMode::Indefinite => 11.0,
        };
        let height = match constraints.height.mode {
            MeasureMode::AtMost => constraints.height.size - 2.0,
            MeasureMode::Definite => constraints.height.size + 7.0,
            MeasureMode::Indefinite => 13.0,
        };
        Size::new(width, height)
    }

    fn callback_baseline(content_size: Size) -> f32 {
        content_size.height - 4.0
    }

    #[test]
    fn simple_tree_direction_metadata_flows_through_layout_tree() {
        let mut tree = SimpleTree::default();
        let explicit = tree.push(SimpleNode::new(Style::default()));
        let inherited = tree.push(SimpleNode::with_inherited_direction_style(Style::default()));

        assert!(LayoutTree::has_explicit_direction_style(&tree, explicit));
        assert!(!LayoutTree::has_explicit_direction_style(&tree, inherited));
    }

    #[test]
    fn simple_tree_measure_and_baseline_callbacks_flow_through_layout_tree() {
        let mut tree = SimpleTree::default();
        let node = tree.push(SimpleNode::with_measure_func_and_baseline(
            Style::default(),
            callback_measure,
            callback_baseline,
        ));
        let constraints =
            Constraints::new(SideConstraint::at_most(30.0), SideConstraint::indefinite());

        assert!(tree.has_measure(node));
        let measured = tree
            .measure(node, constraints)
            .expect("callback measured size");
        assert_eq!(measured, Size::new(27.0, 13.0));
        assert_eq!(tree.baseline(node, measured), Some(9.0));
    }
}
