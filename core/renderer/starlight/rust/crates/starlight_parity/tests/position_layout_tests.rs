// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use starlight_layout::{
    AlignItems, BaseLength, BoxSizing, Constraints, Direction, Display, FlexDirection, FlexWrap,
    JustifyContent, LayoutResult, LayoutTree, Length, LinearGravity, LinearLayoutGravity,
    LinearOrientation, PositionType, Rect, SideConstraint, SimpleNode, SimpleTree, Size, Style,
};
use starlight_parity::run_rust_layout;

fn assert_close(actual: f32, expected: f32) {
    assert!(
        (actual - expected).abs() < 0.01,
        "expected {expected}, got {actual}"
    );
}

#[derive(Clone, Debug)]
struct MeasuringNode {
    style: Style,
    layout: LayoutResult,
    children: Vec<usize>,
    measured_size: Option<Size>,
    last_constraints: Option<Constraints>,
}

impl MeasuringNode {
    fn new(style: Style) -> Self {
        Self {
            style,
            layout: LayoutResult::default(),
            children: Vec::new(),
            measured_size: None,
            last_constraints: None,
        }
    }

    fn measured(style: Style, measured_size: Size) -> Self {
        Self {
            measured_size: Some(measured_size),
            ..Self::new(style)
        }
    }
}

#[derive(Clone, Debug, Default)]
struct MeasuringTree {
    nodes: Vec<MeasuringNode>,
}

impl MeasuringTree {
    fn push(&mut self, node: MeasuringNode) -> usize {
        let id = self.nodes.len();
        self.nodes.push(node);
        id
    }

    fn append_child(&mut self, parent: usize, child: usize) {
        self.nodes[parent].children.push(child);
    }
}

impl LayoutTree for MeasuringTree {
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

    fn measure(&mut self, node: Self::NodeId, constraints: Constraints) -> Option<Size> {
        let node = &mut self.nodes[node];
        node.last_constraints = Some(constraints);
        node.measured_size.map(|size| {
            Size::new(
                constraints.width.clamp(size.width),
                constraints.height.clamp(size.height),
            )
        })
    }

    fn has_measure(&self, node: Self::NodeId) -> bool {
        self.nodes[node].measured_size.is_some()
    }
}

#[test]
fn relative_position_offsets_visual_result_without_changing_flow() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style::default()));
    let relative = tree.push(SimpleNode::with_measured_size(
        Style {
            position: PositionType::Relative,
            left: Length::points(5.0),
            top: Length::points(3.0),
            ..Style::default()
        },
        Size::new(10.0, 10.0),
    ));
    let normal = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(10.0, 7.0),
    ));
    tree.append_child(root, relative);
    tree.append_child(root, normal);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );

    assert_close(size.height, 17.0);
    assert_close(tree.nodes[relative].layout.offset.x, 5.0);
    assert_close(tree.nodes[relative].layout.offset.y, 3.0);
    assert_close(tree.nodes[normal].layout.offset.y, 10.0);
}

#[test]
fn relative_position_percent_offsets_use_parent_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        height: Length::points(40.0),
        ..Style::default()
    }));
    let relative = tree.push(SimpleNode::new(Style {
        position: PositionType::Relative,
        width: Length::points(20.0),
        height: Length::points(10.0),
        left: Length::percent(10.0),
        top: Length::percent(50.0),
        ..Style::default()
    }));
    let normal = tree.push(SimpleNode::new(Style {
        width: Length::points(10.0),
        height: Length::points(7.0),
        ..Style::default()
    }));
    tree.append_child(root, relative);
    tree.append_child(root, normal);

    let size = run_rust_layout(&mut tree, root, Constraints::definite(100.0, 40.0));

    assert_close(size.height, 40.0);
    assert_close(tree.nodes[relative].layout.offset.x, 10.0);
    assert_close(tree.nodes[relative].layout.offset.y, 20.0);
    assert_close(tree.nodes[normal].layout.offset.y, 10.0);
}

#[test]
fn relative_position_calc_end_offsets_use_parent_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let relative = tree.push(SimpleNode::new(Style {
        position: PositionType::Relative,
        width: Length::points(20.0),
        height: Length::points(10.0),
        right: Length::calc(4.0, 5.0),
        bottom: Length::calc(3.0, 10.0),
        ..Style::default()
    }));
    let normal = tree.push(SimpleNode::new(Style {
        width: Length::points(10.0),
        height: Length::points(7.0),
        ..Style::default()
    }));
    tree.append_child(root, relative);
    tree.append_child(root, normal);

    let size = run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    assert_close(size.height, 100.0);
    assert_close(tree.nodes[relative].layout.offset.x, -14.0);
    assert_close(tree.nodes[relative].layout.offset.y, -13.0);
    assert_close(tree.nodes[normal].layout.offset.y, 10.0);
}

#[test]
fn static_position_ignores_offsets() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style::default()));
    let static_child = tree.push(SimpleNode::with_measured_size(
        Style {
            position: PositionType::Static,
            left: Length::points(5.0),
            top: Length::points(3.0),
            ..Style::default()
        },
        Size::new(10.0, 10.0),
    ));
    let normal = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(10.0, 7.0),
    ));
    tree.append_child(root, static_child);
    tree.append_child(root, normal);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );

    assert_close(size.height, 17.0);
    assert_close(tree.nodes[static_child].layout.offset.x, 0.0);
    assert_close(tree.nodes[static_child].layout.offset.y, 0.0);
    assert_close(tree.nodes[normal].layout.offset.y, 10.0);
}

#[test]
fn absolute_position_is_removed_from_block_flow_and_uses_left_top_insets() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        padding: Rect::all(Length::points(2.0)),
        ..Style::default()
    }));
    let absolute = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        width: Length::points(20.0),
        height: Length::points(30.0),
        left: Length::points(7.0),
        top: Length::points(9.0),
        ..Style::default()
    }));
    let normal = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(10.0, 5.0),
    ));
    tree.append_child(root, absolute);
    tree.append_child(root, normal);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );

    assert_close(size.height, 9.0);
    assert_close(tree.nodes[absolute].layout.size.width, 20.0);
    assert_close(tree.nodes[absolute].layout.size.height, 30.0);
    assert_close(tree.nodes[absolute].layout.offset.x, 7.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 9.0);
    assert_close(tree.nodes[normal].layout.offset.y, 2.0);
}

#[test]
fn absolute_measured_percent_size_resolves_once_against_containing_block() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(120.0),
        height: Length::points(82.0),
        padding: Rect::new(
            Length::points(5.0),
            Length::points(7.0),
            Length::points(3.0),
            Length::points(4.0),
        ),
        border: Rect::new(2.0, 1.0, 3.0, 1.0),
        ..Style::default()
    }));
    let absolute = tree.push(SimpleNode::with_measured_size(
        Style {
            position: PositionType::Absolute,
            width: Length::percent(30.0),
            height: Length::percent(25.0),
            left: Length::points(9.0),
            top: Length::points(6.0),
            margin: Rect::new(
                Length::points(2.0),
                Length::points(3.0),
                Length::points(1.0),
                Length::points(4.0),
            ),
            ..Style::default()
        },
        Size::new(31.0, 17.0),
    ));
    tree.append_child(root, absolute);

    let size = run_rust_layout(&mut tree, root, Constraints::definite(180.0, 120.0));

    assert_close(size.width, 135.0);
    assert_close(size.height, 93.0);
    assert_close(tree.nodes[absolute].layout.size.width, 40.0);
    assert_close(tree.nodes[absolute].layout.size.height, 22.0);
    assert_close(tree.nodes[absolute].layout.offset.x, 13.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 10.0);
}

#[test]
fn absolute_measured_percent_border_box_size_resolves_once_against_containing_block() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(122.0),
        height: Length::points(89.0),
        padding: Rect::new(
            Length::points(5.0),
            Length::points(7.0),
            Length::points(3.0),
            Length::points(4.0),
        ),
        border: Rect::new(2.0, 1.0, 3.0, 1.0),
        ..Style::default()
    }));
    let absolute = tree.push(SimpleNode::with_measured_size(
        Style {
            box_sizing: BoxSizing::BorderBox,
            position: PositionType::Absolute,
            width: Length::percent(40.0),
            height: Length::percent(30.0),
            left: Length::points(9.0),
            top: Length::points(6.0),
            margin: Rect::new(
                Length::points(2.0),
                Length::points(3.0),
                Length::points(1.0),
                Length::points(4.0),
            ),
            padding: Rect::all(Length::points(3.0)),
            border: Rect::all(1.0),
            ..Style::default()
        },
        Size::new(72.0, 31.0),
    ));
    tree.append_child(root, absolute);

    run_rust_layout(&mut tree, root, Constraints::definite(180.0, 130.0));

    assert_close(tree.nodes[absolute].layout.size.width, 54.0);
    assert_close(tree.nodes[absolute].layout.size.height, 29.0);
    assert_close(tree.nodes[absolute].layout.offset.x, 13.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 10.0);
    assert_close(tree.nodes[absolute].layout.padding.left, 3.0);
    assert_close(tree.nodes[absolute].layout.border.left, 1.0);
}

#[test]
fn absolute_measured_auto_min_max_percent_calc_resolves_against_containing_block() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        width: Length::points(100.0),
        height: Length::points(50.0),
        ..Style::default()
    }));
    let absolute = tree.push(MeasuringNode::measured(
        Style {
            position: PositionType::Absolute,
            width: Length::Auto,
            height: Length::Auto,
            max_width: Length::percent(50.0),
            min_height: Length::calc(2.0, 40.0),
            left: Length::points(5.0),
            top: Length::points(6.0),
            padding: Rect::all(Length::points(1.0)),
            border: Rect::all(1.0),
            ..Style::default()
        },
        Size::new(80.0, 10.0),
    ));
    tree.append_child(root, absolute);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 50.0));

    let measure_constraints = tree.nodes[absolute]
        .last_constraints
        .expect("absolute child should have been measured");
    assert!(measure_constraints
        .width
        .near(SideConstraint::at_most(50.0)));
    assert!(measure_constraints
        .height
        .near(SideConstraint::at_most(40.0)));
    assert_close(tree.nodes[absolute].layout.size.width, 54.0);
    assert_close(tree.nodes[absolute].layout.size.height, 26.0);
    assert_close(tree.nodes[absolute].layout.offset.x, 5.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 6.0);
}

#[test]
fn absolute_block_fit_content_argument_uses_latest_linear_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let absolute = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        width: Length::fit_content(Some(BaseLength::fixed(80.0))),
        height: Length::fit_content(Some(BaseLength::fixed(20.0))),
        left: Length::points(7.0),
        top: Length::points(9.0),
        ..Style::default()
    }));
    let grandchild = tree.push(SimpleNode::new(Style {
        width: Length::points(120.0),
        height: Length::points(30.0),
        ..Style::default()
    }));
    tree.append_child(root, absolute);
    tree.append_child(absolute, grandchild);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    assert_close(tree.nodes[absolute].layout.size.width, 120.0);
    assert_close(tree.nodes[absolute].layout.size.height, 30.0);
    assert_close(tree.nodes[absolute].layout.offset.x, 7.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 9.0);
}

#[test]
fn absolute_subtree_fit_content_percent_argument_uses_latest_linear_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        width: Length::points(122.0),
        height: Length::points(89.0),
        ..Style::default()
    }));
    let absolute = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(5.0, 50.0))),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(3.0, 25.0))),
        left: Length::points(9.0),
        top: Length::points(6.0),
        ..Style::default()
    }));
    let grandchild = tree.push(SimpleNode::with_measured_size(
        Style {
            display: Display::Block,
            width: Length::points(74.0),
            height: Length::points(29.0),
            ..Style::default()
        },
        Size::new(74.0, 29.0),
    ));
    tree.append_child(root, absolute);
    tree.append_child(absolute, grandchild);

    run_rust_layout(&mut tree, root, Constraints::definite(180.0, 130.0));

    assert_close(tree.nodes[absolute].layout.size.width, 74.0);
    assert_close(tree.nodes[absolute].layout.size.height, 29.0);
    assert_close(tree.nodes[absolute].layout.offset.x, 9.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 6.0);
}

#[test]
fn absolute_measured_fit_content_argument_uses_measured_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let absolute = tree.push(SimpleNode::with_measured_size(
        Style {
            position: PositionType::Absolute,
            width: Length::fit_content(Some(BaseLength::fixed(80.0))),
            height: Length::fit_content(Some(BaseLength::fixed(20.0))),
            left: Length::points(7.0),
            top: Length::points(9.0),
            ..Style::default()
        },
        Size::new(120.0, 30.0),
    ));
    tree.append_child(root, absolute);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    assert_close(tree.nodes[absolute].layout.size.width, 120.0);
    assert_close(tree.nodes[absolute].layout.size.height, 30.0);
    assert_close(tree.nodes[absolute].layout.offset.x, 7.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 9.0);
}

#[test]
fn absolute_block_max_content_uses_latest_linear_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let absolute = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        width: Length::MaxContent,
        height: Length::MaxContent,
        left: Length::points(7.0),
        top: Length::points(9.0),
        ..Style::default()
    }));
    let grandchild = tree.push(SimpleNode::new(Style {
        width: Length::points(250.0),
        height: Length::points(130.0),
        ..Style::default()
    }));
    tree.append_child(root, absolute);
    tree.append_child(absolute, grandchild);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    assert_close(tree.nodes[absolute].layout.size.width, 250.0);
    assert_close(tree.nodes[absolute].layout.size.height, 130.0);
    assert_close(tree.nodes[absolute].layout.offset.x, 7.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 9.0);
}

#[test]
fn absolute_measured_max_content_uses_measured_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let absolute = tree.push(SimpleNode::with_measured_size(
        Style {
            position: PositionType::Absolute,
            width: Length::MaxContent,
            height: Length::MaxContent,
            left: Length::points(7.0),
            top: Length::points(9.0),
            ..Style::default()
        },
        Size::new(250.0, 130.0),
    ));
    tree.append_child(root, absolute);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    assert_close(tree.nodes[absolute].layout.size.width, 250.0);
    assert_close(tree.nodes[absolute].layout.size.height, 130.0);
    assert_close(tree.nodes[absolute].layout.offset.x, 7.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 9.0);
}

#[test]
fn absolute_position_can_use_right_bottom_insets() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let absolute = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        width: Length::points(20.0),
        height: Length::points(10.0),
        right: Length::points(5.0),
        bottom: Length::points(7.0),
        ..Style::default()
    }));
    let flex_child = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(15.0),
        ..Style::default()
    }));
    tree.append_child(root, absolute);
    tree.append_child(root, flex_child);

    let size = run_rust_layout(&mut tree, root, Constraints::definite(100.0, 80.0));

    assert_close(size.width, 100.0);
    assert_close(size.height, 80.0);
    assert_close(tree.nodes[absolute].layout.offset.x, 75.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 63.0);
    assert_close(tree.nodes[flex_child].layout.offset.x, 0.0);
}

#[test]
fn absolute_auto_width_strips_single_horizontal_inset_from_measure_constraint() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        width: Length::points(100.0),
        height: Length::points(40.0),
        ..Style::default()
    }));
    let absolute = tree.push(MeasuringNode::measured(
        Style {
            position: PositionType::Absolute,
            left: Length::points(10.0),
            ..Style::default()
        },
        Size::new(200.0, 10.0),
    ));
    tree.append_child(root, absolute);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 40.0));

    let constraints = tree.nodes[absolute]
        .last_constraints
        .expect("absolute child should have been measured");
    assert!(constraints.width.near(SideConstraint::at_most(90.0)));
    assert_close(tree.nodes[absolute].layout.size.width, 90.0);
    assert_close(tree.nodes[absolute].layout.offset.x, 10.0);
}

#[test]
fn absolute_auto_height_strips_single_vertical_inset_from_measure_constraint() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        width: Length::points(80.0),
        height: Length::points(50.0),
        ..Style::default()
    }));
    let absolute = tree.push(MeasuringNode::measured(
        Style {
            position: PositionType::Absolute,
            top: Length::points(15.0),
            ..Style::default()
        },
        Size::new(10.0, 100.0),
    ));
    tree.append_child(root, absolute);

    run_rust_layout(&mut tree, root, Constraints::definite(80.0, 50.0));

    let constraints = tree.nodes[absolute]
        .last_constraints
        .expect("absolute child should have been measured");
    assert!(constraints.height.near(SideConstraint::at_most(35.0)));
    assert_close(tree.nodes[absolute].layout.size.height, 35.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 15.0);
}

#[test]
fn absolute_auto_size_with_both_insets_fills_padding_box_minus_margins() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        width: Length::points(100.0),
        height: Length::points(50.0),
        padding: Rect::all(Length::points(10.0)),
        ..Style::default()
    }));
    let absolute = tree.push(MeasuringNode::measured(
        Style {
            position: PositionType::Absolute,
            left: Length::points(10.0),
            right: Length::points(15.0),
            top: Length::points(4.0),
            bottom: Length::points(6.0),
            margin: Rect::new(
                Length::points(2.0),
                Length::points(3.0),
                Length::points(1.0),
                Length::points(2.0),
            ),
            ..Style::default()
        },
        Size::new(200.0, 200.0),
    ));
    tree.append_child(root, absolute);

    run_rust_layout(&mut tree, root, Constraints::definite(120.0, 70.0));

    let constraints = tree.nodes[absolute]
        .last_constraints
        .expect("absolute child should have been measured");
    assert!(constraints.width.near(SideConstraint::definite(90.0)));
    assert!(constraints.height.near(SideConstraint::definite(57.0)));
    assert_close(tree.nodes[absolute].layout.size.width, 90.0);
    assert_close(tree.nodes[absolute].layout.size.height, 57.0);
    assert_close(tree.nodes[absolute].layout.offset.x, 12.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 5.0);
}

#[test]
fn absolute_auto_size_with_percent_and_calc_insets_fills_containing_block() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let absolute = tree.push(MeasuringNode::measured(
        Style {
            position: PositionType::Absolute,
            left: Length::percent(10.0),
            right: Length::calc(5.0, 20.0),
            top: Length::calc(2.0, 10.0),
            bottom: Length::percent(25.0),
            margin: Rect::new(
                Length::points(3.0),
                Length::points(7.0),
                Length::points(4.0),
                Length::points(6.0),
            ),
            ..Style::default()
        },
        Size::new(300.0, 200.0),
    ));
    tree.append_child(root, absolute);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    let constraints = tree.nodes[absolute]
        .last_constraints
        .expect("absolute child should have been measured");
    assert!(constraints.width.near(SideConstraint::definite(125.0)));
    assert!(constraints.height.near(SideConstraint::definite(53.0)));
    assert_close(tree.nodes[absolute].layout.size.width, 125.0);
    assert_close(tree.nodes[absolute].layout.size.height, 53.0);
    assert_close(tree.nodes[absolute].layout.offset.x, 23.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 16.0);
}

#[test]
fn absolute_flex_child_without_insets_uses_container_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        width: Length::points(100.0),
        height: Length::points(40.0),
        justify_content: JustifyContent::Center,
        align_items: AlignItems::FlexEnd,
        ..Style::default()
    }));
    let absolute = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, absolute);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 40.0));

    assert_close(tree.nodes[absolute].layout.offset.x, 40.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 30.0);
}

#[test]
fn absolute_flex_child_center_alignment_uses_negative_free_space_when_overflowing() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        width: Length::points(100.0),
        height: Length::points(40.0),
        justify_content: JustifyContent::Center,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let absolute = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        width: Length::points(140.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, absolute);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 40.0));

    assert_close(tree.nodes[absolute].layout.offset.x, -20.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 0.0);
}

#[test]
fn absolute_flex_child_wrap_reverse_reverses_cross_axis_initial_position() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        width: Length::points(100.0),
        height: Length::points(40.0),
        flex_wrap: FlexWrap::WrapReverse,
        align_items: AlignItems::FlexEnd,
        ..Style::default()
    }));
    let absolute = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, absolute);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 40.0));

    assert_close(tree.nodes[absolute].layout.offset.x, 0.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 0.0);
}

#[test]
fn absolute_rtl_flex_child_without_insets_uses_rtl_fronts() {
    let cases = [
        (
            Style {
                direction: Direction::Rtl,
                width: Length::points(100.0),
                height: Length::points(40.0),
                align_items: AlignItems::FlexStart,
                ..Style::default()
            },
            80.0,
            0.0,
        ),
        (
            Style {
                direction: Direction::Rtl,
                flex_direction: FlexDirection::Column,
                width: Length::points(100.0),
                height: Length::points(40.0),
                align_items: AlignItems::FlexStart,
                ..Style::default()
            },
            80.0,
            0.0,
        ),
        (
            Style {
                direction: Direction::Rtl,
                flex_direction: FlexDirection::Column,
                flex_wrap: FlexWrap::WrapReverse,
                width: Length::points(100.0),
                height: Length::points(40.0),
                align_items: AlignItems::FlexStart,
                ..Style::default()
            },
            0.0,
            0.0,
        ),
    ];

    for (style, expected_x, expected_y) in cases {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Flex,
            ..style
        }));
        let absolute = tree.push(SimpleNode::new(Style {
            position: PositionType::Absolute,
            width: Length::points(20.0),
            height: Length::points(10.0),
            ..Style::default()
        }));
        tree.append_child(root, absolute);

        run_rust_layout(&mut tree, root, Constraints::definite(100.0, 40.0));

        assert_close(tree.nodes[absolute].layout.offset.x, expected_x);
        assert_close(tree.nodes[absolute].layout.offset.y, expected_y);
    }
}

#[test]
fn flex_relative_child_percent_offsets_use_container_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        width: Length::points(100.0),
        height: Length::points(40.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let relative = tree.push(SimpleNode::new(Style {
        position: PositionType::Relative,
        width: Length::points(20.0),
        height: Length::points(10.0),
        left: Length::percent(10.0),
        top: Length::percent(25.0),
        ..Style::default()
    }));
    tree.append_child(root, relative);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 40.0));

    assert_close(tree.nodes[relative].layout.offset.x, 10.0);
    assert_close(tree.nodes[relative].layout.offset.y, 10.0);
}

#[test]
fn absolute_linear_child_without_insets_uses_linear_gravity() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Center,
        width: Length::points(100.0),
        height: Length::points(40.0),
        ..Style::default()
    }));
    let absolute = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        width: Length::points(20.0),
        height: Length::points(10.0),
        linear_layout_gravity: LinearLayoutGravity::End,
        ..Style::default()
    }));
    tree.append_child(root, absolute);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 40.0));

    assert_close(tree.nodes[absolute].layout.offset.x, 40.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 30.0);
}

#[test]
fn absolute_rtl_horizontal_linear_child_without_insets_uses_rtl_main_front() {
    let cases = [
        (LinearGravity::None, 80.0),
        (LinearGravity::Left, 0.0),
        (LinearGravity::Right, 80.0),
        (LinearGravity::Center, 40.0),
    ];

    for (gravity, expected_x) in cases {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Linear,
            direction: Direction::Rtl,
            linear_orientation: LinearOrientation::Horizontal,
            linear_gravity: gravity,
            width: Length::points(100.0),
            height: Length::points(40.0),
            ..Style::default()
        }));
        let absolute = tree.push(SimpleNode::new(Style {
            position: PositionType::Absolute,
            width: Length::points(20.0),
            height: Length::points(10.0),
            linear_layout_gravity: LinearLayoutGravity::End,
            ..Style::default()
        }));
        tree.append_child(root, absolute);

        run_rust_layout(&mut tree, root, Constraints::definite(100.0, 40.0));

        assert_close(tree.nodes[absolute].layout.offset.x, expected_x);
        assert_close(tree.nodes[absolute].layout.offset.y, 30.0);
    }
}

#[test]
fn fixed_descendant_uses_root_containing_block_not_parent_container() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let nested = tree.push(SimpleNode::new(Style {
        width: Length::points(40.0),
        height: Length::points(30.0),
        padding: Rect::all(Length::points(2.0)),
        ..Style::default()
    }));
    let fixed = tree.push(SimpleNode::new(Style {
        position: PositionType::Fixed,
        width: Length::points(20.0),
        height: Length::points(10.0),
        right: Length::points(5.0),
        bottom: Length::points(7.0),
        ..Style::default()
    }));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    let size = run_rust_layout(&mut tree, root, Constraints::definite(100.0, 80.0));

    assert_close(size.width, 100.0);
    assert_close(size.height, 80.0);
    assert_close(tree.nodes[nested].layout.size.width, 44.0);
    assert_close(tree.nodes[nested].layout.size.height, 34.0);
    assert_close(tree.nodes[fixed].layout.offset.x, 75.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 63.0);
}

#[test]
fn fixed_descendant_percent_insets_resolve_against_root_containing_block() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let nested = tree.push(SimpleNode::new(Style {
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    }));
    let fixed = tree.push(SimpleNode::new(Style {
        position: PositionType::Fixed,
        width: Length::points(20.0),
        height: Length::points(10.0),
        left: Length::percent(10.0),
        top: Length::percent(25.0),
        ..Style::default()
    }));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    assert_close(tree.nodes[fixed].layout.offset.x, 20.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 25.0);
}

#[test]
fn fixed_descendant_calc_end_insets_resolve_against_root_containing_block() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let nested = tree.push(SimpleNode::new(Style {
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    }));
    let fixed = tree.push(SimpleNode::new(Style {
        position: PositionType::Fixed,
        width: Length::points(20.0),
        height: Length::points(10.0),
        right: Length::calc(4.0, 5.0),
        bottom: Length::calc(3.0, 10.0),
        ..Style::default()
    }));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    assert_close(tree.nodes[fixed].layout.offset.x, 166.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 77.0);
}

#[test]
fn fixed_descendant_measured_aspect_ratio_from_percent_width_uses_root_containing_block() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(122.0),
        height: Length::points(89.0),
        padding: Rect::new(
            Length::points(5.0),
            Length::points(7.0),
            Length::points(3.0),
            Length::points(4.0),
        ),
        border: Rect::new(2.0, 1.0, 3.0, 1.0),
        ..Style::default()
    }));
    let nested = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(44.0),
        height: Length::points(26.0),
        padding: Rect::all(Length::points(2.0)),
        ..Style::default()
    }));
    let fixed = tree.push(SimpleNode::with_measured_size(
        Style {
            position: PositionType::Fixed,
            width: Length::percent(40.0),
            aspect_ratio: Some(2.0),
            left: Length::points(9.0),
            top: Length::points(6.0),
            margin: Rect::new(
                Length::points(2.0),
                Length::points(3.0),
                Length::points(1.0),
                Length::points(4.0),
            ),
            ..Style::default()
        },
        Size::new(72.0, 31.0),
    ));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    run_rust_layout(&mut tree, root, Constraints::definite(180.0, 130.0));

    assert_close(tree.nodes[fixed].layout.size.width, 54.0);
    assert_close(tree.nodes[fixed].layout.size.height, 27.0);
    assert_close(tree.nodes[fixed].layout.offset.x, 13.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 10.0);
}

#[test]
fn fixed_descendant_uses_root_padding_box_offset() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        padding: Rect::all(Length::points(3.0)),
        ..Style::default()
    }));
    let nested = tree.push(SimpleNode::new(Style {
        width: Length::points(20.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let fixed = tree.push(SimpleNode::new(Style {
        position: PositionType::Fixed,
        width: Length::points(10.0),
        height: Length::points(10.0),
        left: Length::points(5.0),
        top: Length::points(7.0),
        ..Style::default()
    }));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 80.0));

    assert_close(tree.nodes[fixed].layout.offset.x, 5.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 7.0);
}

#[test]
fn fixed_descendant_block_fit_content_argument_uses_latest_linear_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let nested = tree.push(SimpleNode::new(Style {
        width: Length::points(20.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let fixed = tree.push(SimpleNode::new(Style {
        position: PositionType::Fixed,
        width: Length::fit_content(Some(BaseLength::fixed(80.0))),
        height: Length::fit_content(Some(BaseLength::fixed(20.0))),
        left: Length::points(7.0),
        top: Length::points(9.0),
        ..Style::default()
    }));
    let grandchild = tree.push(SimpleNode::new(Style {
        width: Length::points(120.0),
        height: Length::points(30.0),
        ..Style::default()
    }));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);
    tree.append_child(fixed, grandchild);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    assert_close(tree.nodes[fixed].layout.size.width, 120.0);
    assert_close(tree.nodes[fixed].layout.size.height, 30.0);
    assert_close(tree.nodes[fixed].layout.offset.x, 7.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 9.0);
}

#[test]
fn fixed_descendant_measured_fit_content_argument_uses_measured_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let nested = tree.push(SimpleNode::new(Style {
        width: Length::points(20.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let fixed = tree.push(SimpleNode::with_measured_size(
        Style {
            position: PositionType::Fixed,
            width: Length::fit_content(Some(BaseLength::fixed(80.0))),
            height: Length::fit_content(Some(BaseLength::fixed(20.0))),
            left: Length::points(7.0),
            top: Length::points(9.0),
            ..Style::default()
        },
        Size::new(120.0, 30.0),
    ));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    assert_close(tree.nodes[fixed].layout.size.width, 120.0);
    assert_close(tree.nodes[fixed].layout.size.height, 30.0);
    assert_close(tree.nodes[fixed].layout.offset.x, 7.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 9.0);
}

#[test]
fn fixed_descendant_block_max_content_uses_latest_linear_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let nested = tree.push(SimpleNode::new(Style {
        width: Length::points(20.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let fixed = tree.push(SimpleNode::new(Style {
        position: PositionType::Fixed,
        width: Length::MaxContent,
        height: Length::MaxContent,
        left: Length::points(7.0),
        top: Length::points(9.0),
        ..Style::default()
    }));
    let grandchild = tree.push(SimpleNode::new(Style {
        width: Length::points(250.0),
        height: Length::points(130.0),
        ..Style::default()
    }));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);
    tree.append_child(fixed, grandchild);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    assert_close(tree.nodes[fixed].layout.size.width, 250.0);
    assert_close(tree.nodes[fixed].layout.size.height, 130.0);
    assert_close(tree.nodes[fixed].layout.offset.x, 7.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 9.0);
}

#[test]
fn fixed_descendant_measured_max_content_uses_measured_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let nested = tree.push(SimpleNode::new(Style {
        width: Length::points(20.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let fixed = tree.push(SimpleNode::with_measured_size(
        Style {
            position: PositionType::Fixed,
            width: Length::MaxContent,
            height: Length::MaxContent,
            left: Length::points(7.0),
            top: Length::points(9.0),
            ..Style::default()
        },
        Size::new(250.0, 130.0),
    ));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    assert_close(tree.nodes[fixed].layout.size.width, 250.0);
    assert_close(tree.nodes[fixed].layout.size.height, 130.0);
    assert_close(tree.nodes[fixed].layout.offset.x, 7.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 9.0);
}

#[test]
fn fixed_auto_size_with_percent_and_calc_insets_uses_root_containing_block() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let nested = tree.push(MeasuringNode::new(Style {
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    }));
    let fixed = tree.push(MeasuringNode::measured(
        Style {
            position: PositionType::Fixed,
            left: Length::percent(10.0),
            right: Length::calc(5.0, 20.0),
            top: Length::calc(2.0, 10.0),
            bottom: Length::percent(25.0),
            margin: Rect::new(
                Length::points(3.0),
                Length::points(7.0),
                Length::points(4.0),
                Length::points(6.0),
            ),
            ..Style::default()
        },
        Size::new(300.0, 200.0),
    ));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    let constraints = tree.nodes[fixed]
        .last_constraints
        .expect("fixed child should have been measured");
    assert!(constraints.width.near(SideConstraint::definite(125.0)));
    assert!(constraints.height.near(SideConstraint::definite(53.0)));
    assert_close(tree.nodes[fixed].layout.size.width, 125.0);
    assert_close(tree.nodes[fixed].layout.size.height, 53.0);
    assert_close(tree.nodes[fixed].layout.offset.x, 23.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 16.0);
}

#[test]
fn sticky_position_exports_insets_without_leaving_flow() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style::default()));
    let sticky = tree.push(SimpleNode::with_measured_size(
        Style {
            position: PositionType::Sticky,
            left: Length::points(3.0),
            right: Length::points(4.0),
            top: Length::points(5.0),
            bottom: Length::points(6.0),
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    let normal = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(20.0, 7.0),
    ));
    tree.append_child(root, sticky);
    tree.append_child(root, normal);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );

    assert_close(size.height, 17.0);
    assert_close(tree.nodes[sticky].layout.offset.x, 0.0);
    assert_close(tree.nodes[sticky].layout.offset.y, 0.0);
    assert_close(tree.nodes[normal].layout.offset.y, 10.0);
    assert_close(tree.nodes[sticky].layout.sticky_pos.left, 3.0);
    assert_close(tree.nodes[sticky].layout.sticky_pos.right, 4.0);
    assert_close(tree.nodes[sticky].layout.sticky_pos.top, 5.0);
    assert_close(tree.nodes[sticky].layout.sticky_pos.bottom, 6.0);
}

#[test]
fn sticky_auto_insets_use_starlight_sentinel() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        width: Length::points(200.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let sticky = tree.push(SimpleNode::new(Style {
        position: PositionType::Sticky,
        width: Length::points(20.0),
        height: Length::points(10.0),
        left: Length::percent(10.0),
        top: Length::percent(25.0),
        ..Style::default()
    }));
    tree.append_child(root, sticky);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 80.0));

    assert_close(tree.nodes[sticky].layout.sticky_pos.left, 20.0);
    assert_close(tree.nodes[sticky].layout.sticky_pos.top, 20.0);
    assert_close(tree.nodes[sticky].layout.sticky_pos.right, -1e10);
    assert_close(tree.nodes[sticky].layout.sticky_pos.bottom, -1e10);
}

#[test]
fn flex_sticky_child_percent_insets_resolve_against_container_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        width: Length::points(100.0),
        height: Length::points(40.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let sticky = tree.push(SimpleNode::new(Style {
        position: PositionType::Sticky,
        width: Length::points(20.0),
        height: Length::points(10.0),
        left: Length::percent(10.0),
        top: Length::percent(25.0),
        ..Style::default()
    }));
    tree.append_child(root, sticky);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 40.0));

    assert_close(tree.nodes[sticky].layout.sticky_pos.left, 10.0);
    assert_close(tree.nodes[sticky].layout.sticky_pos.top, 10.0);
    assert_close(tree.nodes[sticky].layout.sticky_pos.right, -1e10);
    assert_close(tree.nodes[sticky].layout.sticky_pos.bottom, -1e10);
}

#[test]
fn flex_sticky_child_end_percent_insets_resolve_against_container_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        width: Length::points(100.0),
        height: Length::points(40.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let sticky = tree.push(SimpleNode::new(Style {
        position: PositionType::Sticky,
        width: Length::points(20.0),
        height: Length::points(10.0),
        right: Length::percent(20.0),
        bottom: Length::percent(50.0),
        ..Style::default()
    }));
    tree.append_child(root, sticky);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 40.0));

    assert_close(tree.nodes[sticky].layout.sticky_pos.left, -1e10);
    assert_close(tree.nodes[sticky].layout.sticky_pos.top, -1e10);
    assert_close(tree.nodes[sticky].layout.sticky_pos.right, 20.0);
    assert_close(tree.nodes[sticky].layout.sticky_pos.bottom, 20.0);
}

#[test]
fn linear_sticky_child_percent_insets_resolve_against_container_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        width: Length::points(100.0),
        height: Length::points(40.0),
        linear_orientation: LinearOrientation::Horizontal,
        ..Style::default()
    }));
    let sticky = tree.push(SimpleNode::new(Style {
        position: PositionType::Sticky,
        width: Length::points(20.0),
        height: Length::points(10.0),
        left: Length::percent(10.0),
        top: Length::percent(25.0),
        ..Style::default()
    }));
    tree.append_child(root, sticky);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 40.0));

    assert_close(tree.nodes[sticky].layout.sticky_pos.left, 10.0);
    assert_close(tree.nodes[sticky].layout.sticky_pos.top, 10.0);
    assert_close(tree.nodes[sticky].layout.sticky_pos.right, -1e10);
    assert_close(tree.nodes[sticky].layout.sticky_pos.bottom, -1e10);
}

#[test]
fn linear_sticky_child_end_percent_insets_resolve_against_container_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        width: Length::points(100.0),
        height: Length::points(40.0),
        linear_orientation: LinearOrientation::Horizontal,
        ..Style::default()
    }));
    let sticky = tree.push(SimpleNode::new(Style {
        position: PositionType::Sticky,
        width: Length::points(20.0),
        height: Length::points(10.0),
        right: Length::percent(20.0),
        bottom: Length::percent(50.0),
        ..Style::default()
    }));
    tree.append_child(root, sticky);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 40.0));

    assert_close(tree.nodes[sticky].layout.sticky_pos.left, -1e10);
    assert_close(tree.nodes[sticky].layout.sticky_pos.top, -1e10);
    assert_close(tree.nodes[sticky].layout.sticky_pos.right, 20.0);
    assert_close(tree.nodes[sticky].layout.sticky_pos.bottom, 20.0);
}

#[test]
fn grid_sticky_child_percent_insets_resolve_against_container_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(40.0),
        grid_template_columns: vec![Length::points(100.0)],
        grid_template_rows: vec![Length::points(40.0)],
        ..Style::default()
    }));
    let sticky = tree.push(SimpleNode::new(Style {
        position: PositionType::Sticky,
        width: Length::points(20.0),
        height: Length::points(10.0),
        left: Length::percent(10.0),
        top: Length::percent(25.0),
        ..Style::default()
    }));
    tree.append_child(root, sticky);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 40.0));

    assert_close(tree.nodes[sticky].layout.sticky_pos.left, 10.0);
    assert_close(tree.nodes[sticky].layout.sticky_pos.top, 10.0);
    assert_close(tree.nodes[sticky].layout.sticky_pos.right, -1e10);
    assert_close(tree.nodes[sticky].layout.sticky_pos.bottom, -1e10);
}

#[test]
fn grid_sticky_child_end_percent_insets_resolve_against_container_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(40.0),
        grid_template_columns: vec![Length::points(100.0)],
        grid_template_rows: vec![Length::points(40.0)],
        ..Style::default()
    }));
    let sticky = tree.push(SimpleNode::new(Style {
        position: PositionType::Sticky,
        width: Length::points(20.0),
        height: Length::points(10.0),
        right: Length::percent(20.0),
        bottom: Length::percent(50.0),
        ..Style::default()
    }));
    tree.append_child(root, sticky);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 40.0));

    assert_close(tree.nodes[sticky].layout.sticky_pos.left, -1e10);
    assert_close(tree.nodes[sticky].layout.sticky_pos.top, -1e10);
    assert_close(tree.nodes[sticky].layout.sticky_pos.right, 20.0);
    assert_close(tree.nodes[sticky].layout.sticky_pos.bottom, 20.0);
}

#[test]
fn relative_sticky_child_percent_insets_resolve_against_container_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(100.0),
        height: Length::points(40.0),
        ..Style::default()
    }));
    let sticky = tree.push(SimpleNode::new(Style {
        position: PositionType::Sticky,
        width: Length::points(20.0),
        height: Length::points(10.0),
        left: Length::percent(10.0),
        top: Length::percent(25.0),
        ..Style::default()
    }));
    tree.append_child(root, sticky);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 40.0));

    assert_close(tree.nodes[sticky].layout.sticky_pos.left, 10.0);
    assert_close(tree.nodes[sticky].layout.sticky_pos.top, 10.0);
    assert_close(tree.nodes[sticky].layout.sticky_pos.right, -1e10);
    assert_close(tree.nodes[sticky].layout.sticky_pos.bottom, -1e10);
}

#[test]
fn relative_sticky_child_end_percent_insets_resolve_against_container_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(100.0),
        height: Length::points(40.0),
        ..Style::default()
    }));
    let sticky = tree.push(SimpleNode::new(Style {
        position: PositionType::Sticky,
        width: Length::points(20.0),
        height: Length::points(10.0),
        right: Length::percent(20.0),
        bottom: Length::percent(50.0),
        ..Style::default()
    }));
    tree.append_child(root, sticky);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 40.0));

    assert_close(tree.nodes[sticky].layout.sticky_pos.left, -1e10);
    assert_close(tree.nodes[sticky].layout.sticky_pos.top, -1e10);
    assert_close(tree.nodes[sticky].layout.sticky_pos.right, 20.0);
    assert_close(tree.nodes[sticky].layout.sticky_pos.bottom, 20.0);
}

#[test]
fn sticky_root_exports_default_insets() {
    let mut tree = SimpleTree::default();
    let sticky = tree.push(SimpleNode::new(Style {
        position: PositionType::Sticky,
        left: Length::percent(10.0),
        top: Length::percent(25.0),
        ..Style::default()
    }));

    run_rust_layout(&mut tree, sticky, Constraints::definite(200.0, 80.0));

    assert_close(tree.nodes[sticky].layout.sticky_pos.left, 0.0);
    assert_close(tree.nodes[sticky].layout.sticky_pos.top, 0.0);
    assert_close(tree.nodes[sticky].layout.sticky_pos.right, 0.0);
    assert_close(tree.nodes[sticky].layout.sticky_pos.bottom, 0.0);
}

#[test]
fn sticky_child_percent_insets_resolve_against_parent_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        width: Length::points(200.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let sticky = tree.push(SimpleNode::new(Style {
        position: PositionType::Sticky,
        width: Length::points(20.0),
        height: Length::points(10.0),
        left: Length::percent(10.0),
        top: Length::percent(25.0),
        ..Style::default()
    }));
    tree.append_child(root, sticky);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 80.0));

    assert_close(tree.nodes[sticky].layout.sticky_pos.left, 20.0);
    assert_close(tree.nodes[sticky].layout.sticky_pos.top, 20.0);
    assert_close(tree.nodes[sticky].layout.sticky_pos.right, -1e10);
    assert_close(tree.nodes[sticky].layout.sticky_pos.bottom, -1e10);
}

#[test]
fn sticky_child_calc_insets_resolve_against_parent_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        width: Length::points(200.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let sticky = tree.push(SimpleNode::new(Style {
        position: PositionType::Sticky,
        width: Length::points(20.0),
        height: Length::points(10.0),
        left: Length::calc(3.0, 10.0),
        right: Length::calc(4.0, 5.0),
        top: Length::calc(2.0, 25.0),
        bottom: Length::calc(1.0, 50.0),
        ..Style::default()
    }));
    tree.append_child(root, sticky);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 80.0));

    assert_close(tree.nodes[sticky].layout.sticky_pos.left, 23.0);
    assert_close(tree.nodes[sticky].layout.sticky_pos.right, 14.0);
    assert_close(tree.nodes[sticky].layout.sticky_pos.top, 22.0);
    assert_close(tree.nodes[sticky].layout.sticky_pos.bottom, 41.0);
}
