// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#![forbid(unsafe_code)]

use starlight_layout::{
    AlignItems, BoxSizing, Constraints, Direction, Display, FlexDirection, LayoutEngine,
    LayoutResult, LayoutTree, Length, Rect, SideConstraint, Size, Style,
};

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
struct ExternalId(u32);

impl ExternalId {
    fn index(self) -> usize {
        self.0 as usize
    }
}

#[derive(Clone, Debug)]
struct ExternalNode {
    style: Style,
    children: Vec<ExternalId>,
    measured_size: Option<Size>,
    baseline: Option<f32>,
    physical_pixels_per_layout_unit: f32,
    layout: Option<LayoutResult>,
    layout_constraints: Option<Constraints>,
}

impl ExternalNode {
    fn new(style: Style) -> Self {
        Self {
            style,
            children: Vec::new(),
            measured_size: None,
            baseline: None,
            physical_pixels_per_layout_unit: 1.0,
            layout: None,
            layout_constraints: None,
        }
    }

    fn measured(style: Style, measured_size: Size, baseline: f32) -> Self {
        Self {
            measured_size: Some(measured_size),
            baseline: Some(baseline),
            ..Self::new(style)
        }
    }

    fn with_physical_pixels_per_layout_unit(mut self, value: f32) -> Self {
        self.physical_pixels_per_layout_unit = value;
        self
    }
}

#[derive(Default)]
struct ExternalTree {
    nodes: Vec<ExternalNode>,
}

impl ExternalTree {
    fn push(&mut self, node: ExternalNode) -> ExternalId {
        let id = ExternalId(self.nodes.len() as u32);
        self.nodes.push(node);
        id
    }

    fn append_child(&mut self, parent: ExternalId, child: ExternalId) {
        self.nodes[parent.index()].children.push(child);
    }

    fn node(&self, node: ExternalId) -> &ExternalNode {
        &self.nodes[node.index()]
    }
}

impl LayoutTree for ExternalTree {
    type NodeId = ExternalId;
    type Children<'a> = std::iter::Copied<std::slice::Iter<'a, ExternalId>>;

    fn children(&self, node: Self::NodeId) -> Self::Children<'_> {
        self.nodes[node.index()].children.iter().copied()
    }

    fn style(&self, node: Self::NodeId) -> &Style {
        &self.nodes[node.index()].style
    }

    fn set_layout(&mut self, node: Self::NodeId, layout: LayoutResult) {
        self.nodes[node.index()].layout = Some(layout);
    }

    fn set_layout_with_constraints(
        &mut self,
        node: Self::NodeId,
        constraints: Constraints,
        layout: LayoutResult,
    ) {
        let node = &mut self.nodes[node.index()];
        node.layout_constraints = Some(constraints);
        node.layout = Some(layout);
    }

    fn layout(&self, node: Self::NodeId) -> Option<LayoutResult> {
        self.nodes[node.index()].layout
    }

    fn measure(&mut self, node: Self::NodeId, _constraints: Constraints) -> Option<Size> {
        self.nodes[node.index()].measured_size
    }

    fn has_measure(&self, node: Self::NodeId) -> bool {
        self.nodes[node.index()].measured_size.is_some()
    }

    fn physical_pixels_per_layout_unit(&self, node: Self::NodeId) -> f32 {
        self.nodes[node.index()].physical_pixels_per_layout_unit
    }

    fn baseline(&self, node: Self::NodeId, _content_size: Size) -> Option<f32> {
        self.nodes[node.index()].baseline
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
struct MinimalId(usize);

#[derive(Clone, Debug)]
struct MinimalNode {
    style: Style,
    children: Vec<MinimalId>,
    layout: Option<LayoutResult>,
    layout_writes: usize,
}

impl MinimalNode {
    fn new(style: Style) -> Self {
        Self {
            style,
            children: Vec::new(),
            layout: None,
            layout_writes: 0,
        }
    }
}

#[derive(Default)]
struct MinimalTree {
    nodes: Vec<MinimalNode>,
}

impl MinimalTree {
    fn push(&mut self, node: MinimalNode) -> MinimalId {
        let id = MinimalId(self.nodes.len());
        self.nodes.push(node);
        id
    }

    fn append_child(&mut self, parent: MinimalId, child: MinimalId) {
        self.nodes[parent.0].children.push(child);
    }

    fn node(&self, node: MinimalId) -> &MinimalNode {
        &self.nodes[node.0]
    }
}

impl LayoutTree for MinimalTree {
    type NodeId = MinimalId;
    type Children<'a> = std::iter::Copied<std::slice::Iter<'a, MinimalId>>;

    fn children(&self, node: Self::NodeId) -> Self::Children<'_> {
        self.nodes[node.0].children.iter().copied()
    }

    fn style(&self, node: Self::NodeId) -> &Style {
        &self.nodes[node.0].style
    }

    fn set_layout(&mut self, node: Self::NodeId, layout: LayoutResult) {
        let node = &mut self.nodes[node.0];
        node.layout = Some(layout);
        node.layout_writes += 1;
    }
}

fn assert_close(actual: f32, expected: f32) {
    assert!(
        (actual - expected).abs() < 0.01,
        "expected {expected}, got {actual}"
    );
}

#[test]
fn layout_engine_runs_over_custom_external_tree_ids_and_writeback() {
    let mut tree = ExternalTree::default();
    let root = tree.push(ExternalNode::new(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        box_sizing: BoxSizing::ContentBox,
        width: Length::points(80.0),
        height: Length::points(40.0),
        padding: Rect::all(Length::points(2.0)),
        direction: Direction::Ltr,
        ..Style::default()
    }));
    let child = tree.push(
        ExternalNode::measured(
            Style {
                box_sizing: BoxSizing::ContentBox,
                ..Style::default()
            },
            Size::new(10.2, 5.2),
            4.0,
        )
        .with_physical_pixels_per_layout_unit(2.0),
    );
    tree.append_child(root, child);

    let owner_constraints = Constraints::new(
        SideConstraint::definite(100.0),
        SideConstraint::definite(80.0),
    );
    let root_size =
        LayoutEngine::new().layout_with_owner_constraints(&mut tree, root, owner_constraints);

    assert_close(root_size.width, 84.0);
    assert_close(root_size.height, 44.0);

    let root_layout = tree.node(root).layout.expect("root layout writeback");
    assert_close(root_layout.size.width, 84.0);
    assert_close(root_layout.size.height, 44.0);
    assert_eq!(
        tree.node(root)
            .layout_constraints
            .expect("root constraints writeback"),
        Constraints::definite(84.0, 44.0)
    );

    let child_layout = tree.node(child).layout.expect("child layout writeback");
    assert_close(child_layout.offset.x, 2.0);
    assert_close(child_layout.offset.y, 2.0);
    assert_close(child_layout.size.width, 10.5);
    assert_close(child_layout.size.height, 5.5);
    assert_close(child_layout.baseline.expect("child baseline"), 4.0);
    let child_constraints = tree
        .node(child)
        .layout_constraints
        .expect("child constraints writeback");
    assert!(
        child_constraints.width.bounded_size().is_some(),
        "child width constraints should be written back with a bounded mode"
    );
    assert!(
        child_constraints.height.bounded_size().is_some(),
        "child height constraints should be written back with a bounded mode"
    );
}

#[test]
fn layout_engine_runs_with_minimal_write_only_external_tree_adapter() {
    let mut tree = MinimalTree::default();
    let root = tree.push(MinimalNode::new(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(60.0),
        height: Length::points(20.0),
        direction: Direction::Ltr,
        ..Style::default()
    }));
    let child = tree.push(MinimalNode::new(Style {
        width: Length::points(11.0),
        height: Length::points(7.0),
        ..Style::default()
    }));
    tree.append_child(root, child);

    let root_size = LayoutEngine::new().layout_with_owner_constraints(
        &mut tree,
        root,
        Constraints::definite(100.0, 80.0),
    );

    assert_close(root_size.width, 60.0);
    assert_close(root_size.height, 20.0);

    let root_node = tree.node(root);
    assert!(
        root_node.layout_writes >= 1,
        "root should be written through the default constraint-aware callback"
    );
    let root_layout = root_node.layout.expect("root layout writeback");
    assert_close(root_layout.size.width, 60.0);
    assert_close(root_layout.size.height, 20.0);

    let child_node = tree.node(child);
    assert!(
        child_node.layout_writes >= 1,
        "child should be written through the default constraint-aware callback"
    );
    let child_layout = child_node.layout.expect("child layout writeback");
    assert_close(child_layout.offset.x, 0.0);
    assert_close(child_layout.offset.y, 0.0);
    assert_close(child_layout.size.width, 11.0);
    assert_close(child_layout.size.height, 7.0);
}
