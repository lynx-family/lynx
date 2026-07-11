// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use starlight_layout::{
    BaseLength, Constraints, Display, LayoutEngine, LayoutResult, LayoutTree, Length, PositionType,
    Rect, RelativeCenter, SimpleNode, SimpleTree, Size, Style, Visibility, RELATIVE_ALIGN_PARENT,
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
    measure: Option<MeasureBehavior>,
    last_constraints: Option<Constraints>,
}

#[derive(Clone, Copy, Debug)]
enum MeasureBehavior {
    Fixed(Size),
    WidthByHeightMode {
        at_most_width: f32,
        definite_width: f32,
        height: f32,
    },
}

impl MeasuringNode {
    fn new(style: Style) -> Self {
        Self {
            style,
            layout: LayoutResult::default(),
            children: Vec::new(),
            measure: None,
            last_constraints: None,
        }
    }

    fn measured(style: Style, measured_size: Size) -> Self {
        Self {
            measure: Some(MeasureBehavior::Fixed(measured_size)),
            ..Self::new(style)
        }
    }

    fn width_by_height_mode(
        style: Style,
        at_most_width: f32,
        definite_width: f32,
        height: f32,
    ) -> Self {
        Self {
            measure: Some(MeasureBehavior::WidthByHeightMode {
                at_most_width,
                definite_width,
                height,
            }),
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
        node.measure.map(|behavior| match behavior {
            MeasureBehavior::Fixed(size) => Size::new(
                constraints.width.clamp(size.width),
                constraints.height.clamp(size.height),
            ),
            MeasureBehavior::WidthByHeightMode {
                at_most_width,
                definite_width,
                height,
            } => {
                let width = if constraints.height.is_definite() {
                    definite_width
                } else {
                    at_most_width
                };
                Size::new(
                    constraints.width.clamp(width),
                    constraints.height.clamp(height),
                )
            }
        })
    }

    fn has_measure(&self, node: Self::NodeId) -> bool {
        self.nodes[node].measure.is_some()
    }
}

#[test]
fn relative_display_centers_child_in_definite_parent() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_center: RelativeCenter::Both,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 100.0);
    assert_close(size.height, 80.0);
    assert_close(tree.nodes[child].layout.offset.x, 40.0);
    assert_close(tree.nodes[child].layout.offset.y, 35.0);
}

#[test]
fn relative_wrap_content_center_recomputes_after_container_sizing() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        ..Style::default()
    }));
    let anchor = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(80.0, 30.0),
    ));
    let centered = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_center: RelativeCenter::Both,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, anchor);
    tree.append_child(root, centered);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 80.0);
    assert_close(size.height, 30.0);
    assert_close(tree.nodes[centered].layout.offset.x, 30.0);
    assert_close(tree.nodes[centered].layout.offset.y, 10.0);
}

#[test]
fn visibility_hidden_and_collapse_relative_children_participate_in_dependency_layout() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let hidden_anchor = tree.push(SimpleNode::new(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        relative_id: 10,
        visibility: Visibility::Hidden,
        ..Style::default()
    }));
    let collapsed_anchor = tree.push(SimpleNode::new(Style {
        width: Length::points(30.0),
        height: Length::points(12.0),
        relative_id: 20,
        relative_right_of: 10,
        visibility: Visibility::Collapse,
        ..Style::default()
    }));
    let follower = tree.push(SimpleNode::new(Style {
        width: Length::points(5.0),
        height: Length::points(5.0),
        relative_right_of: 20,
        relative_bottom_of: 20,
        ..Style::default()
    }));
    tree.append_child(root, hidden_anchor);
    tree.append_child(root, collapsed_anchor);
    tree.append_child(root, follower);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 80.0));

    assert_close(tree.nodes[hidden_anchor].layout.offset.x, 0.0);
    assert_close(tree.nodes[hidden_anchor].layout.offset.y, 0.0);
    assert_close(tree.nodes[collapsed_anchor].layout.offset.x, 20.0);
    assert_close(tree.nodes[collapsed_anchor].layout.offset.y, 0.0);
    assert_close(tree.nodes[follower].layout.offset.x, 50.0);
    assert_close(tree.nodes[follower].layout.offset.y, 12.0);
}

#[test]
fn relative_display_absolute_child_uses_static_start_without_participating() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        ..Style::default()
    }));
    let absolute = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, absolute);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 0.0);
    assert_close(size.height, 0.0);
    assert_close(tree.nodes[absolute].layout.offset.x, 0.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 0.0);
    assert_close(tree.nodes[absolute].layout.size.width, 20.0);
    assert_close(tree.nodes[absolute].layout.size.height, 10.0);
}

#[test]
fn relative_absolute_static_start_with_margins_positions_margin_box() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let absolute = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        width: Length::points(20.0),
        height: Length::points(10.0),
        margin: Rect::new(
            Length::points(3.0),
            Length::points(7.0),
            Length::points(4.0),
            Length::points(6.0),
        ),
        ..Style::default()
    }));
    tree.append_child(root, absolute);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    assert_close(tree.nodes[absolute].layout.offset.x, 3.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 4.0);
    assert_close(tree.nodes[absolute].layout.size.width, 20.0);
    assert_close(tree.nodes[absolute].layout.size.height, 10.0);
}

#[test]
fn relative_absolute_percent_insets_and_size_resolve_against_relative_containing_block() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let absolute = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        left: Length::percent(10.0),
        top: Length::percent(25.0),
        width: Length::percent(50.0),
        height: Length::percent(20.0),
        ..Style::default()
    }));
    tree.append_child(root, absolute);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    assert_close(tree.nodes[absolute].layout.offset.x, 20.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 25.0);
    assert_close(tree.nodes[absolute].layout.size.width, 100.0);
    assert_close(tree.nodes[absolute].layout.size.height, 20.0);
}

#[test]
fn relative_absolute_percent_end_insets_resolve_against_relative_containing_block() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let absolute = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        right: Length::percent(10.0),
        bottom: Length::percent(25.0),
        width: Length::percent(50.0),
        height: Length::percent(20.0),
        ..Style::default()
    }));
    tree.append_child(root, absolute);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    assert_close(tree.nodes[absolute].layout.offset.x, 80.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 55.0);
    assert_close(tree.nodes[absolute].layout.size.width, 100.0);
    assert_close(tree.nodes[absolute].layout.size.height, 20.0);
}

#[test]
fn relative_absolute_auto_size_stretches_between_start_and_end_insets() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let absolute = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        left: Length::points(10.0),
        right: Length::points(30.0),
        top: Length::points(20.0),
        bottom: Length::points(25.0),
        ..Style::default()
    }));
    tree.append_child(root, absolute);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    assert_close(tree.nodes[absolute].layout.offset.x, 10.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 20.0);
    assert_close(tree.nodes[absolute].layout.size.width, 160.0);
    assert_close(tree.nodes[absolute].layout.size.height, 55.0);
}

#[test]
fn relative_absolute_auto_size_between_insets_strips_margins() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let absolute = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        left: Length::points(10.0),
        right: Length::points(30.0),
        top: Length::points(20.0),
        bottom: Length::points(25.0),
        margin: Rect::new(
            Length::points(3.0),
            Length::points(7.0),
            Length::points(4.0),
            Length::points(6.0),
        ),
        ..Style::default()
    }));
    tree.append_child(root, absolute);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    assert_close(tree.nodes[absolute].layout.offset.x, 13.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 24.0);
    assert_close(tree.nodes[absolute].layout.size.width, 150.0);
    assert_close(tree.nodes[absolute].layout.size.height, 45.0);
}

#[test]
fn relative_absolute_auto_size_paired_insets_fill_padding_box_minus_margins() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Relative,
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
    assert!(constraints
        .width
        .near(starlight_layout::SideConstraint::definite(90.0)));
    assert!(constraints
        .height
        .near(starlight_layout::SideConstraint::definite(57.0)));
    assert_close(tree.nodes[absolute].layout.size.width, 90.0);
    assert_close(tree.nodes[absolute].layout.size.height, 57.0);
    assert_close(tree.nodes[absolute].layout.offset.x, 12.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 5.0);
}

#[test]
fn relative_absolute_single_insets_strip_at_most_measure_constraints() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Relative,
        width: Length::points(100.0),
        height: Length::points(50.0),
        ..Style::default()
    }));
    let absolute = tree.push(MeasuringNode::measured(
        Style {
            position: PositionType::Absolute,
            left: Length::points(10.0),
            top: Length::points(15.0),
            margin: Rect::new(
                Length::points(3.0),
                Length::points(7.0),
                Length::points(4.0),
                Length::points(6.0),
            ),
            ..Style::default()
        },
        Size::new(200.0, 100.0),
    ));
    tree.append_child(root, absolute);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 50.0));

    let constraints = tree.nodes[absolute]
        .last_constraints
        .expect("absolute child should have been measured");
    assert!(constraints
        .width
        .near(starlight_layout::SideConstraint::at_most(80.0)));
    assert!(constraints
        .height
        .near(starlight_layout::SideConstraint::at_most(25.0)));
    assert_close(tree.nodes[absolute].layout.offset.x, 13.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 19.0);
    assert_close(tree.nodes[absolute].layout.size.width, 80.0);
    assert_close(tree.nodes[absolute].layout.size.height, 25.0);
}

#[test]
fn relative_absolute_end_insets_override_static_start_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let absolute = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        right: Length::points(30.0),
        bottom: Length::points(25.0),
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, absolute);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    assert_close(tree.nodes[absolute].layout.offset.x, 150.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 65.0);
    assert_close(tree.nodes[absolute].layout.size.width, 20.0);
    assert_close(tree.nodes[absolute].layout.size.height, 10.0);
}

#[test]
fn relative_absolute_end_insets_with_margins_position_margin_box() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let absolute = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        right: Length::points(30.0),
        bottom: Length::points(25.0),
        width: Length::points(20.0),
        height: Length::points(10.0),
        margin: Rect::new(
            Length::points(3.0),
            Length::points(7.0),
            Length::points(4.0),
            Length::points(6.0),
        ),
        ..Style::default()
    }));
    tree.append_child(root, absolute);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    assert_close(tree.nodes[absolute].layout.offset.x, 143.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 59.0);
    assert_close(tree.nodes[absolute].layout.size.width, 20.0);
    assert_close(tree.nodes[absolute].layout.size.height, 10.0);
}

#[test]
fn relative_absolute_start_insets_override_static_start_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let absolute = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        left: Length::points(12.0),
        top: Length::points(9.0),
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, absolute);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    assert_close(tree.nodes[absolute].layout.offset.x, 12.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 9.0);
    assert_close(tree.nodes[absolute].layout.size.width, 20.0);
    assert_close(tree.nodes[absolute].layout.size.height, 10.0);
}

#[test]
fn relative_absolute_paired_insets_with_explicit_size_use_start_insets() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let absolute = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        left: Length::points(12.0),
        right: Length::points(30.0),
        top: Length::points(9.0),
        bottom: Length::points(25.0),
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, absolute);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    assert_close(tree.nodes[absolute].layout.offset.x, 12.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 9.0);
    assert_close(tree.nodes[absolute].layout.size.width, 20.0);
    assert_close(tree.nodes[absolute].layout.size.height, 10.0);
}

#[test]
fn relative_fixed_descendant_uses_root_relative_containing_block() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let nested = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(40.0),
        height: Length::points(30.0),
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

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 80.0));

    assert_close(tree.nodes[fixed].layout.offset.x, 75.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 63.0);
    assert_close(tree.nodes[fixed].layout.size.width, 20.0);
    assert_close(tree.nodes[fixed].layout.size.height, 10.0);
}

#[test]
fn relative_fixed_descendant_uses_relative_root_padding_box_offset() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(100.0),
        height: Length::points(80.0),
        padding: Rect::all(Length::points(3.0)),
        ..Style::default()
    }));
    let nested = tree.push(SimpleNode::new(Style {
        display: Display::Block,
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
    assert_close(tree.nodes[fixed].layout.size.width, 10.0);
    assert_close(tree.nodes[fixed].layout.size.height, 10.0);
}

#[test]
fn relative_fixed_static_start_with_margins_positions_margin_box() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let nested = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    }));
    let fixed = tree.push(SimpleNode::new(Style {
        position: PositionType::Fixed,
        width: Length::points(20.0),
        height: Length::points(10.0),
        margin: Rect::new(
            Length::points(3.0),
            Length::points(7.0),
            Length::points(4.0),
            Length::points(6.0),
        ),
        ..Style::default()
    }));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    assert_close(tree.nodes[fixed].layout.offset.x, 3.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 4.0);
    assert_close(tree.nodes[fixed].layout.size.width, 20.0);
    assert_close(tree.nodes[fixed].layout.size.height, 10.0);
}

#[test]
fn relative_fixed_percent_insets_and_size_resolve_against_root_relative_containing_block() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let nested = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    }));
    let fixed = tree.push(SimpleNode::new(Style {
        position: PositionType::Fixed,
        left: Length::percent(10.0),
        top: Length::percent(25.0),
        width: Length::percent(50.0),
        height: Length::percent(20.0),
        ..Style::default()
    }));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    assert_close(tree.nodes[fixed].layout.offset.x, 20.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 25.0);
    assert_close(tree.nodes[fixed].layout.size.width, 100.0);
    assert_close(tree.nodes[fixed].layout.size.height, 20.0);
}

#[test]
fn relative_fixed_percent_end_insets_resolve_against_root_relative_containing_block() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let nested = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    }));
    let fixed = tree.push(SimpleNode::new(Style {
        position: PositionType::Fixed,
        right: Length::percent(10.0),
        bottom: Length::percent(25.0),
        width: Length::percent(50.0),
        height: Length::percent(20.0),
        ..Style::default()
    }));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    assert_close(tree.nodes[fixed].layout.offset.x, 80.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 55.0);
    assert_close(tree.nodes[fixed].layout.size.width, 100.0);
    assert_close(tree.nodes[fixed].layout.size.height, 20.0);
}

#[test]
fn relative_fixed_auto_size_stretches_between_start_and_end_insets() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let nested = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    }));
    let fixed = tree.push(SimpleNode::new(Style {
        position: PositionType::Fixed,
        left: Length::points(10.0),
        right: Length::points(30.0),
        top: Length::points(20.0),
        bottom: Length::points(25.0),
        ..Style::default()
    }));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    assert_close(tree.nodes[fixed].layout.offset.x, 10.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 20.0);
    assert_close(tree.nodes[fixed].layout.size.width, 160.0);
    assert_close(tree.nodes[fixed].layout.size.height, 55.0);
}

#[test]
fn relative_fixed_auto_size_between_insets_strips_margins() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let nested = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    }));
    let fixed = tree.push(SimpleNode::new(Style {
        position: PositionType::Fixed,
        left: Length::points(10.0),
        right: Length::points(30.0),
        top: Length::points(20.0),
        bottom: Length::points(25.0),
        margin: Rect::new(
            Length::points(3.0),
            Length::points(7.0),
            Length::points(4.0),
            Length::points(6.0),
        ),
        ..Style::default()
    }));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    assert_close(tree.nodes[fixed].layout.offset.x, 13.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 24.0);
    assert_close(tree.nodes[fixed].layout.size.width, 150.0);
    assert_close(tree.nodes[fixed].layout.size.height, 45.0);
}

#[test]
fn relative_fixed_single_insets_strip_at_most_measure_constraints() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Relative,
        width: Length::points(100.0),
        height: Length::points(50.0),
        ..Style::default()
    }));
    let nested = tree.push(MeasuringNode::new(Style {
        display: Display::Block,
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let fixed = tree.push(MeasuringNode::measured(
        Style {
            position: PositionType::Fixed,
            left: Length::points(10.0),
            top: Length::points(15.0),
            margin: Rect::new(
                Length::points(3.0),
                Length::points(7.0),
                Length::points(4.0),
                Length::points(6.0),
            ),
            ..Style::default()
        },
        Size::new(200.0, 100.0),
    ));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 50.0));

    let constraints = tree.nodes[fixed]
        .last_constraints
        .expect("fixed child should have been measured");
    assert!(constraints
        .width
        .near(starlight_layout::SideConstraint::at_most(80.0)));
    assert!(constraints
        .height
        .near(starlight_layout::SideConstraint::at_most(25.0)));
    assert_close(tree.nodes[fixed].layout.offset.x, 13.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 19.0);
    assert_close(tree.nodes[fixed].layout.size.width, 80.0);
    assert_close(tree.nodes[fixed].layout.size.height, 25.0);
}

#[test]
fn relative_fixed_start_insets_override_static_start_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let nested = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    }));
    let fixed = tree.push(SimpleNode::new(Style {
        position: PositionType::Fixed,
        left: Length::points(12.0),
        top: Length::points(9.0),
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    assert_close(tree.nodes[fixed].layout.offset.x, 12.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 9.0);
    assert_close(tree.nodes[fixed].layout.size.width, 20.0);
    assert_close(tree.nodes[fixed].layout.size.height, 10.0);
}

#[test]
fn relative_fixed_paired_insets_with_explicit_size_use_start_insets() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let nested = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    }));
    let fixed = tree.push(SimpleNode::new(Style {
        position: PositionType::Fixed,
        left: Length::points(12.0),
        right: Length::points(30.0),
        top: Length::points(9.0),
        bottom: Length::points(25.0),
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    assert_close(tree.nodes[fixed].layout.offset.x, 12.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 9.0);
    assert_close(tree.nodes[fixed].layout.size.width, 20.0);
    assert_close(tree.nodes[fixed].layout.size.height, 10.0);
}

#[test]
fn relative_fixed_end_insets_override_static_start_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let nested = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    }));
    let fixed = tree.push(SimpleNode::new(Style {
        position: PositionType::Fixed,
        right: Length::points(30.0),
        bottom: Length::points(25.0),
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    assert_close(tree.nodes[fixed].layout.offset.x, 150.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 65.0);
    assert_close(tree.nodes[fixed].layout.size.width, 20.0);
    assert_close(tree.nodes[fixed].layout.size.height, 10.0);
}

#[test]
fn relative_fixed_end_insets_with_margins_position_margin_box() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let nested = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    }));
    let fixed = tree.push(SimpleNode::new(Style {
        position: PositionType::Fixed,
        right: Length::points(30.0),
        bottom: Length::points(25.0),
        width: Length::points(20.0),
        height: Length::points(10.0),
        margin: Rect::new(
            Length::points(3.0),
            Length::points(7.0),
            Length::points(4.0),
            Length::points(6.0),
        ),
        ..Style::default()
    }));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 100.0));

    assert_close(tree.nodes[fixed].layout.offset.x, 143.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 59.0);
    assert_close(tree.nodes[fixed].layout.size.width, 20.0);
    assert_close(tree.nodes[fixed].layout.size.height, 10.0);
}

#[test]
fn relative_layout_once_parent_edge_stretch_resolves_initial_height_constraint() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        relative_layout_once: true,
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        relative_align_top: RELATIVE_ALIGN_PARENT,
        relative_align_bottom: RELATIVE_ALIGN_PARENT,
        margin: Rect::new(
            Length::points(0.0),
            Length::points(0.0),
            Length::points(3.0),
            Length::points(7.0),
        ),
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[child].layout.offset.y, 3.0);
    assert_close(tree.nodes[child].layout.size.height, 70.0);
}

#[test]
fn relative_layout_once_definite_parent_remeasures_two_sided_child_on_both_axes() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Relative,
        relative_layout_once: true,
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let child = tree.push(MeasuringNode::measured(
        Style {
            relative_align_left: RELATIVE_ALIGN_PARENT,
            relative_align_right: RELATIVE_ALIGN_PARENT,
            relative_align_top: RELATIVE_ALIGN_PARENT,
            relative_align_bottom: RELATIVE_ALIGN_PARENT,
            margin: Rect::new(
                Length::points(3.0),
                Length::points(5.0),
                Length::points(7.0),
                Length::points(11.0),
            ),
            ..Style::default()
        },
        Size::new(200.0, 200.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    let constraints = tree.nodes[child]
        .last_constraints
        .expect("child should have been measured");
    assert!(
        constraints
            .width
            .near(starlight_layout::SideConstraint::definite(92.0)),
        "expected width constraint definite(92), got {:?}",
        constraints.width
    );
    assert!(
        constraints
            .height
            .near(starlight_layout::SideConstraint::definite(62.0)),
        "expected height constraint definite(62), got {:?}",
        constraints.height
    );
    assert_close(tree.nodes[child].layout.offset.x, 3.0);
    assert_close(tree.nodes[child].layout.offset.y, 7.0);
    assert_close(tree.nodes[child].layout.size.width, 92.0);
    assert_close(tree.nodes[child].layout.size.height, 62.0);
}

#[test]
fn measured_relative_child_without_baseline_keeps_rounded_height_fallback_implicit() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(20.0, 23.5),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[child].layout.size.height, 24.0);
    assert_eq!(tree.nodes[child].layout.baseline, None);
}

#[test]
fn relative_display_centers_child_horizontally_only_in_definite_parent() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_center: RelativeCenter::Horizontal,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[child].layout.offset.x, 40.0);
    assert_close(tree.nodes[child].layout.offset.y, 0.0);
}

#[test]
fn relative_display_centers_child_vertically_only_in_definite_parent() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_center: RelativeCenter::Vertical,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[child].layout.offset.x, 0.0);
    assert_close(tree.nodes[child].layout.offset.y, 35.0);
}

#[test]
fn relative_display_missing_reference_resolves_to_no_constraint_before_centering() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_right_of: 999,
            relative_bottom_of: 999,
            relative_center: RelativeCenter::Both,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 100.0);
    assert_close(size.height, 80.0);
    assert_close(tree.nodes[child].layout.offset.x, 40.0);
    assert_close(tree.nodes[child].layout.offset.y, 35.0);
}

#[test]
fn relative_missing_start_references_fall_back_to_after_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(120.0),
        height: Length::points(90.0),
        ..Style::default()
    }));
    let horizontal_anchor = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 10,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    let vertical_anchor = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 30,
            ..Style::default()
        },
        Size::new(10.0, 15.0),
    ));
    let follower = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_align_left: 999,
            relative_right_of: 10,
            relative_align_top: 998,
            relative_bottom_of: 30,
            ..Style::default()
        },
        Size::new(10.0, 8.0),
    ));
    tree.append_child(root, horizontal_anchor);
    tree.append_child(root, vertical_anchor);
    tree.append_child(root, follower);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[follower].layout.offset.x, 20.0);
    assert_close(tree.nodes[follower].layout.offset.y, 15.0);
}

#[test]
fn relative_missing_end_references_fall_back_to_before_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(120.0),
        height: Length::points(90.0),
        ..Style::default()
    }));
    let right_anchor = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 20,
            relative_align_right: RELATIVE_ALIGN_PARENT,
            ..Style::default()
        },
        Size::new(30.0, 10.0),
    ));
    let bottom_anchor = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 40,
            relative_align_bottom: RELATIVE_ALIGN_PARENT,
            ..Style::default()
        },
        Size::new(10.0, 20.0),
    ));
    let follower = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_align_right: 999,
            relative_left_of: 20,
            relative_align_bottom: 998,
            relative_top_of: 40,
            ..Style::default()
        },
        Size::new(10.0, 8.0),
    ));
    tree.append_child(root, right_anchor);
    tree.append_child(root, bottom_anchor);
    tree.append_child(root, follower);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[follower].layout.offset.x, 80.0);
    assert_close(tree.nodes[follower].layout.offset.y, 62.0);
}

#[test]
fn relative_display_aligns_child_to_parent_end_edges() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_align_right: RELATIVE_ALIGN_PARENT,
            relative_align_bottom: RELATIVE_ALIGN_PARENT,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[child].layout.offset.x, 80.0);
    assert_close(tree.nodes[child].layout.offset.y, 70.0);
}

#[test]
fn relative_parent_end_alignment_takes_precedence_over_centering() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_align_right: RELATIVE_ALIGN_PARENT,
            relative_align_bottom: RELATIVE_ALIGN_PARENT,
            relative_center: RelativeCenter::Both,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[child].layout.offset.x, 80.0);
    assert_close(tree.nodes[child].layout.offset.y, 70.0);
}

#[test]
fn relative_parent_start_alignment_takes_precedence_over_centering() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_align_left: RELATIVE_ALIGN_PARENT,
            relative_align_top: RELATIVE_ALIGN_PARENT,
            relative_center: RelativeCenter::Both,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[child].layout.offset.x, 0.0);
    assert_close(tree.nodes[child].layout.offset.y, 0.0);
}

#[test]
fn relative_non_once_wrap_content_height_uses_prefinal_vertical_recompute() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        padding: Rect::new(
            Length::points(3.0),
            Length::points(4.0),
            Length::points(5.0),
            Length::points(2.0),
        ),
        ..Style::default()
    }));
    let anchor = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 1,
            relative_align_left: RELATIVE_ALIGN_PARENT,
            relative_align_top: RELATIVE_ALIGN_PARENT,
            margin: Rect::new(
                Length::points(2.0),
                Length::points(1.0),
                Length::points(3.0),
                Length::ZERO,
            ),
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    let dependent = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 2,
            relative_align_left: 1,
            relative_align_top: 1,
            margin: Rect::new(
                Length::points(1.0),
                Length::points(2.0),
                Length::points(1.0),
                Length::points(3.0),
            ),
            ..Style::default()
        },
        Size::new(16.0, 12.0),
    ));
    let follower = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_left_of: 2,
            relative_top_of: 2,
            ..Style::default()
        },
        Size::new(9.0, 7.0),
    ));
    tree.append_child(root, anchor);
    tree.append_child(root, dependent);
    tree.append_child(root, follower);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 39.0);
    assert_close(size.height, 37.0);
    assert_close(tree.nodes[anchor].layout.offset.y, 8.0);
    assert_close(tree.nodes[dependent].layout.offset.y, 6.0);
    assert_close(tree.nodes[follower].layout.offset.y, -2.0);
}

#[test]
fn relative_wrap_content_width_remeasures_two_sided_child_after_horizontal_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        padding: Rect::new(
            Length::points(3.0),
            Length::points(4.0),
            Length::points(5.0),
            Length::points(2.0),
        ),
        ..Style::default()
    }));
    let anchor = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 1,
            relative_align_left: RELATIVE_ALIGN_PARENT,
            relative_align_top: RELATIVE_ALIGN_PARENT,
            margin: Rect::new(
                Length::points(2.0),
                Length::points(1.0),
                Length::points(3.0),
                Length::ZERO,
            ),
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    let dependent = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 2,
            relative_align_left: 1,
            relative_align_right: RELATIVE_ALIGN_PARENT,
            relative_align_top: 1,
            relative_align_bottom: RELATIVE_ALIGN_PARENT,
            margin: Rect::new(
                Length::points(1.0),
                Length::points(2.0),
                Length::points(1.0),
                Length::points(3.0),
            ),
            ..Style::default()
        },
        Size::new(16.0, 12.0),
    ));
    let follower = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_left_of: 2,
            relative_top_of: 2,
            ..Style::default()
        },
        Size::new(9.0, 7.0),
    ));
    tree.append_child(root, anchor);
    tree.append_child(root, dependent);
    tree.append_child(root, follower);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 39.0);
    assert_close(size.height, 37.0);
    assert_close(tree.nodes[dependent].layout.offset.x, 4.0);
    assert_close(tree.nodes[dependent].layout.size.width, 29.0);
    assert_close(tree.nodes[follower].layout.offset.x, -6.0);
}

#[test]
fn relative_display_positions_child_after_referenced_sibling() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        ..Style::default()
    }));
    let anchor = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 10,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    let follower = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_right_of: 10,
            relative_bottom_of: 10,
            ..Style::default()
        },
        Size::new(5.0, 7.0),
    ));
    tree.append_child(root, follower);
    tree.append_child(root, anchor);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 25.0);
    assert_close(size.height, 17.0);
    assert_close(tree.nodes[anchor].layout.offset.x, 0.0);
    assert_close(tree.nodes[anchor].layout.offset.y, 0.0);
    assert_close(tree.nodes[follower].layout.offset.x, 20.0);
    assert_close(tree.nodes[follower].layout.offset.y, 10.0);
}

#[test]
fn relative_align_parent_start_takes_precedence_over_sibling_after_constraint() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(100.0),
        height: Length::points(40.0),
        ..Style::default()
    }));
    let anchor = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 10,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    let follower = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_align_left: RELATIVE_ALIGN_PARENT,
            relative_align_top: RELATIVE_ALIGN_PARENT,
            relative_right_of: 10,
            relative_bottom_of: 10,
            ..Style::default()
        },
        Size::new(5.0, 7.0),
    ));
    tree.append_child(root, follower);
    tree.append_child(root, anchor);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 40.0));

    assert_close(tree.nodes[follower].layout.offset.x, 0.0);
    assert_close(tree.nodes[follower].layout.offset.y, 0.0);
    assert_close(tree.nodes[follower].layout.size.width, 5.0);
    assert_close(tree.nodes[follower].layout.size.height, 7.0);
}

#[test]
fn relative_align_parent_end_takes_precedence_over_sibling_before_constraint() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(100.0),
        height: Length::points(40.0),
        ..Style::default()
    }));
    let anchor = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 10,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    let follower = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_align_right: RELATIVE_ALIGN_PARENT,
            relative_align_bottom: RELATIVE_ALIGN_PARENT,
            relative_left_of: 10,
            relative_top_of: 10,
            ..Style::default()
        },
        Size::new(5.0, 7.0),
    ));
    tree.append_child(root, follower);
    tree.append_child(root, anchor);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 40.0));

    assert_close(tree.nodes[follower].layout.offset.x, 95.0);
    assert_close(tree.nodes[follower].layout.offset.y, 33.0);
    assert_close(tree.nodes[follower].layout.size.width, 5.0);
    assert_close(tree.nodes[follower].layout.size.height, 7.0);
}

#[test]
fn relative_align_sibling_start_takes_precedence_over_sibling_after_constraint() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let anchor = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 10,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    let after_anchor = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 20,
            relative_right_of: 10,
            relative_bottom_of: 10,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    let follower = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_align_left: 10,
            relative_align_top: 10,
            relative_right_of: 20,
            relative_bottom_of: 20,
            ..Style::default()
        },
        Size::new(5.0, 7.0),
    ));
    tree.append_child(root, follower);
    tree.append_child(root, after_anchor);
    tree.append_child(root, anchor);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 80.0));

    assert_close(tree.nodes[follower].layout.offset.x, 0.0);
    assert_close(tree.nodes[follower].layout.offset.y, 0.0);
    assert_close(tree.nodes[follower].layout.size.width, 5.0);
    assert_close(tree.nodes[follower].layout.size.height, 7.0);
}

#[test]
fn relative_align_sibling_end_takes_precedence_over_sibling_before_constraint() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let anchor = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 10,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    let spacer = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 30,
            ..Style::default()
        },
        Size::new(40.0, 30.0),
    ));
    let before_anchor = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 20,
            relative_right_of: 30,
            relative_bottom_of: 30,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    let follower = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_align_right: 10,
            relative_align_bottom: 10,
            relative_left_of: 20,
            relative_top_of: 20,
            ..Style::default()
        },
        Size::new(5.0, 7.0),
    ));
    tree.append_child(root, follower);
    tree.append_child(root, before_anchor);
    tree.append_child(root, spacer);
    tree.append_child(root, anchor);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 80.0));

    assert_close(tree.nodes[follower].layout.offset.x, 15.0);
    assert_close(tree.nodes[follower].layout.offset.y, 3.0);
    assert_close(tree.nodes[follower].layout.size.width, 5.0);
    assert_close(tree.nodes[follower].layout.size.height, 7.0);
}

#[test]
fn relative_display_duplicate_ids_resolve_to_last_matching_sibling() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        ..Style::default()
    }));
    let first_anchor = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 10,
            ..Style::default()
        },
        Size::new(60.0, 40.0),
    ));
    let follower = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_right_of: 10,
            relative_bottom_of: 10,
            ..Style::default()
        },
        Size::new(5.0, 7.0),
    ));
    let last_anchor = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 10,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, first_anchor);
    tree.append_child(root, follower);
    tree.append_child(root, last_anchor);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 60.0);
    assert_close(size.height, 40.0);
    assert_close(tree.nodes[first_anchor].layout.offset.x, 0.0);
    assert_close(tree.nodes[first_anchor].layout.offset.y, 0.0);
    assert_close(tree.nodes[last_anchor].layout.offset.x, 0.0);
    assert_close(tree.nodes[last_anchor].layout.offset.y, 0.0);
    assert_close(tree.nodes[follower].layout.offset.x, 20.0);
    assert_close(tree.nodes[follower].layout.offset.y, 10.0);
}

#[test]
fn relative_display_order_affects_duplicate_id_resolution() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        ..Style::default()
    }));
    let later_order_anchor = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 10,
            order: 2,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    let follower = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_right_of: 10,
            relative_bottom_of: 10,
            order: 3,
            ..Style::default()
        },
        Size::new(5.0, 7.0),
    ));
    let earlier_order_anchor = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 10,
            order: 1,
            ..Style::default()
        },
        Size::new(60.0, 40.0),
    ));
    tree.append_child(root, later_order_anchor);
    tree.append_child(root, follower);
    tree.append_child(root, earlier_order_anchor);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 60.0);
    assert_close(size.height, 40.0);
    assert_close(tree.nodes[earlier_order_anchor].layout.offset.x, 0.0);
    assert_close(tree.nodes[earlier_order_anchor].layout.offset.y, 0.0);
    assert_close(tree.nodes[later_order_anchor].layout.offset.x, 0.0);
    assert_close(tree.nodes[later_order_anchor].layout.offset.y, 0.0);
    assert_close(tree.nodes[follower].layout.offset.x, 20.0);
    assert_close(tree.nodes[follower].layout.offset.y, 10.0);
}

#[test]
fn relative_display_skips_display_none_duplicate_id_for_dependency_lookup() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        ..Style::default()
    }));
    let visible_anchor = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 10,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    let follower = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_right_of: 10,
            relative_bottom_of: 10,
            ..Style::default()
        },
        Size::new(5.0, 7.0),
    ));
    let hidden_anchor = tree.push(SimpleNode::new(Style {
        display: Display::None,
        relative_id: 10,
        width: Length::points(80.0),
        height: Length::points(40.0),
        ..Style::default()
    }));
    tree.append_child(root, visible_anchor);
    tree.append_child(root, follower);
    tree.append_child(root, hidden_anchor);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 25.0);
    assert_close(size.height, 17.0);
    assert_close(tree.nodes[visible_anchor].layout.offset.x, 0.0);
    assert_close(tree.nodes[visible_anchor].layout.offset.y, 0.0);
    assert_close(tree.nodes[follower].layout.offset.x, 20.0);
    assert_close(tree.nodes[follower].layout.offset.y, 10.0);
    assert_eq!(tree.nodes[hidden_anchor].layout.size, Size::ZERO);
    assert_close(tree.nodes[hidden_anchor].layout.offset.x, 0.0);
    assert_close(tree.nodes[hidden_anchor].layout.offset.y, 0.0);
}

#[test]
fn relative_display_duplicate_ids_align_to_last_matching_sibling_edge() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(120.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let first_anchor = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 10,
            relative_align_right: RELATIVE_ALIGN_PARENT,
            relative_align_bottom: RELATIVE_ALIGN_PARENT,
            ..Style::default()
        },
        Size::new(30.0, 20.0),
    ));
    let follower = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_align_left: 10,
            relative_align_bottom: 10,
            ..Style::default()
        },
        Size::new(5.0, 7.0),
    ));
    let last_anchor = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 10,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, first_anchor);
    tree.append_child(root, follower);
    tree.append_child(root, last_anchor);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 120.0);
    assert_close(size.height, 80.0);
    assert_close(tree.nodes[first_anchor].layout.offset.x, 90.0);
    assert_close(tree.nodes[first_anchor].layout.offset.y, 60.0);
    assert_close(tree.nodes[last_anchor].layout.offset.x, 0.0);
    assert_close(tree.nodes[last_anchor].layout.offset.y, 0.0);
    assert_close(tree.nodes[follower].layout.offset.x, 0.0);
    assert_close(tree.nodes[follower].layout.offset.y, 3.0);
}

#[test]
fn root_relative_fit_content_percent_argument_uses_wrap_content_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 50.0))),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 25.0))),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(140.0, 70.0),
    ));
    tree.append_child(root, child);

    let size = LayoutEngine::new().layout_with_owner_constraints(
        &mut tree,
        root,
        Constraints::definite(200.0, 100.0),
    );

    assert_close(size.width, 140.0);
    assert_close(size.height, 70.0);
    assert_close(tree.nodes[root].layout.size.width, 140.0);
    assert_close(tree.nodes[root].layout.size.height, 70.0);
}

#[test]
fn root_relative_fit_content_calc_argument_uses_wrap_content_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(5.0, 25.0))),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(140.0, 70.0),
    ));
    tree.append_child(root, child);

    let size = LayoutEngine::new().layout_with_owner_constraints(
        &mut tree,
        root,
        Constraints::definite(200.0, 100.0),
    );

    assert_close(size.width, 140.0);
    assert_close(size.height, 70.0);
    assert_close(tree.nodes[root].layout.size.width, 140.0);
    assert_close(tree.nodes[root].layout.size.height, 70.0);
}

#[test]
fn child_relative_fit_content_percent_argument_uses_wrap_content_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let relative = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 50.0))),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 25.0))),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(140.0, 70.0),
    ));
    tree.append_child(root, relative);
    tree.append_child(relative, child);

    LayoutEngine::new().layout_with_owner_constraints(
        &mut tree,
        root,
        Constraints::definite(200.0, 100.0),
    );

    assert_close(tree.nodes[relative].layout.size.width, 140.0);
    assert_close(tree.nodes[relative].layout.size.height, 70.0);
    assert_close(tree.nodes[child].layout.size.width, 140.0);
    assert_close(tree.nodes[child].layout.size.height, 70.0);
}

#[test]
fn child_relative_fit_content_calc_argument_uses_wrap_content_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let relative = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(5.0, 25.0))),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(140.0, 70.0),
    ));
    tree.append_child(root, relative);
    tree.append_child(relative, child);

    LayoutEngine::new().layout_with_owner_constraints(
        &mut tree,
        root,
        Constraints::definite(200.0, 100.0),
    );

    assert_close(tree.nodes[relative].layout.size.width, 140.0);
    assert_close(tree.nodes[relative].layout.size.height, 70.0);
    assert_close(tree.nodes[child].layout.size.width, 140.0);
    assert_close(tree.nodes[child].layout.size.height, 70.0);
}

#[test]
fn relative_child_fit_content_fixed_argument_uses_indefinite_owner_constraint() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Relative,
        ..Style::default()
    }));
    let child = tree.push(MeasuringNode::measured(
        Style {
            width: Length::fit_content(Some(BaseLength::fixed(50.0))),
            height: Length::fit_content(Some(BaseLength::fixed(30.0))),
            ..Style::default()
        },
        Size::new(80.0, 40.0),
    ));
    tree.append_child(root, child);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 50.0);
    assert_close(size.height, 30.0);
    assert_close(tree.nodes[child].layout.size.width, 50.0);
    assert_close(tree.nodes[child].layout.size.height, 30.0);
}

#[test]
fn wrap_content_relative_recomputes_parent_end_alignment_after_sizing() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_align_right: RELATIVE_ALIGN_PARENT,
            relative_align_bottom: RELATIVE_ALIGN_PARENT,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 20.0);
    assert_close(size.height, 10.0);
    assert_close(tree.nodes[child].layout.offset.x, 0.0);
    assert_close(tree.nodes[child].layout.offset.y, 0.0);
}

#[test]
fn relative_layout_once_uses_combined_dependency_order_for_cross_axis_cycle() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        relative_layout_once: true,
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 1,
            relative_bottom_of: 2,
            ..Style::default()
        },
        Size::new(10.0, 10.0),
    ));
    let second = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 2,
            relative_right_of: 1,
            ..Style::default()
        },
        Size::new(5.0, 7.0),
    ));
    tree.append_child(root, first);
    tree.append_child(root, second);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 15.0);
    assert_close(size.height, 10.0);
    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[first].layout.offset.y, 7.0);
    assert_close(tree.nodes[second].layout.offset.x, 10.0);
    assert_close(tree.nodes[second].layout.offset.y, 0.0);
}

#[test]
fn relative_layout_once_processes_all_initial_dependency_roots_before_dependents() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        relative_layout_once: true,
        ..Style::default()
    }));
    let dependent = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_right_of: 2,
            ..Style::default()
        },
        Size::new(10.0, 10.0),
    ));
    let centered_root = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_center: RelativeCenter::Horizontal,
            ..Style::default()
        },
        Size::new(10.0, 10.0),
    ));
    let anchor = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 2,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, dependent);
    tree.append_child(root, centered_root);
    tree.append_child(root, anchor);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 30.0);
    assert_close(size.height, 10.0);
    assert_close(tree.nodes[centered_root].layout.offset.x, 10.0);
    assert_close(tree.nodes[anchor].layout.offset.x, 0.0);
    assert_close(tree.nodes[dependent].layout.offset.x, 20.0);
}

#[test]
fn relative_display_single_start_constraint_reduces_at_most_measure_width() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Relative,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let anchor = tree.push(MeasuringNode::measured(
        Style {
            relative_id: 10,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    let follower = tree.push(MeasuringNode::measured(
        Style {
            relative_right_of: 10,
            ..Style::default()
        },
        Size::new(200.0, 10.0),
    ));
    tree.append_child(root, follower);
    tree.append_child(root, anchor);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    let constraints = tree.nodes[follower]
        .last_constraints
        .expect("follower should have been measured");
    assert!(constraints
        .width
        .near(starlight_layout::SideConstraint::at_most(80.0)));
    assert_close(tree.nodes[follower].layout.offset.x, 20.0);
    assert_close(tree.nodes[follower].layout.size.width, 80.0);
}

#[test]
fn relative_display_single_start_constraint_reduces_at_most_measure_height() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Relative,
        width: Length::points(30.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let anchor = tree.push(MeasuringNode::measured(
        Style {
            relative_id: 11,
            ..Style::default()
        },
        Size::new(10.0, 20.0),
    ));
    let follower = tree.push(MeasuringNode::measured(
        Style {
            relative_bottom_of: 11,
            ..Style::default()
        },
        Size::new(10.0, 200.0),
    ));
    tree.append_child(root, follower);
    tree.append_child(root, anchor);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    let constraints = tree.nodes[follower]
        .last_constraints
        .expect("follower should have been measured");
    assert!(constraints
        .height
        .near(starlight_layout::SideConstraint::at_most(60.0)));
    assert_close(tree.nodes[follower].layout.offset.y, 20.0);
    assert_close(tree.nodes[follower].layout.size.height, 60.0);
}

#[test]
fn relative_display_single_end_constraint_preserves_margin_in_at_most_width() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Relative,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let anchor = tree.push(MeasuringNode::measured(
        Style {
            relative_id: 20,
            relative_align_right: RELATIVE_ALIGN_PARENT,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    let follower = tree.push(MeasuringNode::measured(
        Style {
            relative_left_of: 20,
            margin: Rect::new(
                Length::points(5.0),
                Length::points(5.0),
                Length::ZERO,
                Length::ZERO,
            ),
            ..Style::default()
        },
        Size::new(200.0, 10.0),
    ));
    tree.append_child(root, follower);
    tree.append_child(root, anchor);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    let constraints = tree.nodes[follower]
        .last_constraints
        .expect("follower should have been measured");
    assert!(constraints
        .width
        .near(starlight_layout::SideConstraint::at_most(80.0)));
    assert_close(tree.nodes[follower].layout.offset.x, -5.0);
    assert_close(tree.nodes[follower].layout.size.width, 80.0);
}

#[test]
fn relative_display_single_end_constraint_preserves_margin_in_at_most_height() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Relative,
        width: Length::points(30.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let anchor = tree.push(MeasuringNode::measured(
        Style {
            relative_id: 12,
            relative_align_bottom: RELATIVE_ALIGN_PARENT,
            ..Style::default()
        },
        Size::new(10.0, 20.0),
    ));
    let follower = tree.push(MeasuringNode::measured(
        Style {
            relative_top_of: 12,
            margin: Rect::new(
                Length::ZERO,
                Length::ZERO,
                Length::points(4.0),
                Length::points(6.0),
            ),
            ..Style::default()
        },
        Size::new(10.0, 200.0),
    ));
    tree.append_child(root, follower);
    tree.append_child(root, anchor);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    let constraints = tree.nodes[follower]
        .last_constraints
        .expect("follower should have been measured");
    assert!(constraints
        .height
        .near(starlight_layout::SideConstraint::at_most(60.0)));
    assert_close(tree.nodes[follower].layout.offset.y, -6.0);
    assert_close(tree.nodes[follower].layout.size.height, 60.0);
}

#[test]
fn relative_display_stretches_child_between_parent_edges() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        relative_align_left: RELATIVE_ALIGN_PARENT,
        relative_align_right: RELATIVE_ALIGN_PARENT,
        relative_align_top: RELATIVE_ALIGN_PARENT,
        relative_align_bottom: RELATIVE_ALIGN_PARENT,
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[child].layout.offset.x, 0.0);
    assert_close(tree.nodes[child].layout.offset.y, 0.0);
    assert_close(tree.nodes[child].layout.size.width, 100.0);
    assert_close(tree.nodes[child].layout.size.height, 80.0);
}

#[test]
fn relative_two_pass_freezes_horizontal_size_before_vertical_stretch_remeasure() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Relative,
        height: Length::points(100.0),
        ..Style::default()
    }));
    let child = tree.push(MeasuringNode::width_by_height_mode(
        Style {
            relative_align_top: RELATIVE_ALIGN_PARENT,
            relative_align_bottom: RELATIVE_ALIGN_PARENT,
            ..Style::default()
        },
        20.0,
        60.0,
        10.0,
    ));
    tree.append_child(root, child);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    let constraints = tree.nodes[child]
        .last_constraints
        .expect("child should have been measured");
    assert!(
        constraints
            .width
            .near(starlight_layout::SideConstraint::definite(20.0)),
        "expected width constraint definite(20), got {:?}",
        constraints.width
    );
    assert!(
        constraints
            .height
            .near(starlight_layout::SideConstraint::definite(100.0)),
        "expected height constraint definite(100), got {:?}",
        constraints.height
    );
    assert_close(size.width, 20.0);
    assert_close(size.height, 100.0);
    assert_close(tree.nodes[child].layout.size.width, 20.0);
    assert_close(tree.nodes[child].layout.size.height, 100.0);
}

#[test]
fn relative_container_min_width_and_max_height_clamp_wrap_content_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        min_width: Length::points(40.0),
        max_height: Length::points(25.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(20.0, 30.0),
    ));
    tree.append_child(root, child);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 40.0);
    assert_close(size.height, 25.0);
}

#[test]
fn relative_container_max_width_and_min_height_clamp_wrap_content_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        max_width: Length::points(60.0),
        min_height: Length::points(40.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(100.0, 10.0),
    ));
    tree.append_child(root, child);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 60.0);
    assert_close(size.height, 40.0);
}

#[test]
fn relative_container_padding_border_prevents_negative_content_size_under_tight_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        padding: Rect::new(
            Length::points(10.0),
            Length::points(15.0),
            Length::points(8.0),
            Length::points(9.0),
        ),
        border: Rect::new(2.0, 3.0, 1.0, 4.0),
        ..Style::default()
    }));

    let size = run_rust_layout(&mut tree, root, Constraints::definite(8.0, 7.0));

    assert_close(size.width, 30.0);
    assert_close(size.height, 22.0);
}

#[test]
fn relative_display_positions_child_before_referenced_sibling() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let anchor = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 20,
            relative_align_right: RELATIVE_ALIGN_PARENT,
            relative_align_bottom: RELATIVE_ALIGN_PARENT,
            ..Style::default()
        },
        Size::new(20.0, 20.0),
    ));
    let before = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_left_of: 20,
            relative_top_of: 20,
            ..Style::default()
        },
        Size::new(10.0, 10.0),
    ));
    tree.append_child(root, before);
    tree.append_child(root, anchor);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[anchor].layout.offset.x, 80.0);
    assert_close(tree.nodes[anchor].layout.offset.y, 60.0);
    assert_close(tree.nodes[before].layout.offset.x, 70.0);
    assert_close(tree.nodes[before].layout.offset.y, 50.0);
}

#[test]
fn relative_display_aligns_child_to_sibling_edges() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let anchor = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 30,
            relative_align_right: RELATIVE_ALIGN_PARENT,
            relative_align_bottom: RELATIVE_ALIGN_PARENT,
            ..Style::default()
        },
        Size::new(20.0, 20.0),
    ));
    let aligned = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_align_left: 30,
            relative_align_bottom: 30,
            ..Style::default()
        },
        Size::new(5.0, 7.0),
    ));
    tree.append_child(root, aligned);
    tree.append_child(root, anchor);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[aligned].layout.offset.x, 80.0);
    assert_close(tree.nodes[aligned].layout.offset.y, 73.0);
}

#[test]
fn relative_display_stretches_child_between_sibling_edges_and_strips_margins() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(100.0),
        height: Length::points(40.0),
        ..Style::default()
    }));
    let left = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 40,
            relative_align_left: RELATIVE_ALIGN_PARENT,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    let right = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 41,
            relative_align_right: RELATIVE_ALIGN_PARENT,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    let middle = tree.push(SimpleNode::new(Style {
        relative_right_of: 40,
        relative_left_of: 41,
        margin: Rect::new(
            Length::points(5.0),
            Length::points(5.0),
            Length::ZERO,
            Length::ZERO,
        ),
        ..Style::default()
    }));
    tree.append_child(root, middle);
    tree.append_child(root, right);
    tree.append_child(root, left);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[left].layout.offset.x, 0.0);
    assert_close(tree.nodes[right].layout.offset.x, 80.0);
    assert_close(tree.nodes[middle].layout.offset.x, 25.0);
    assert_close(tree.nodes[middle].layout.size.width, 50.0);
}

#[test]
fn relative_sibling_start_to_parent_end_overrides_explicit_width() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(100.0),
        height: Length::points(40.0),
        ..Style::default()
    }));
    let anchor = tree.push(SimpleNode::new(Style {
        relative_id: 50,
        relative_align_left: RELATIVE_ALIGN_PARENT,
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let stretched = tree.push(SimpleNode::new(Style {
        relative_align_left: 50,
        relative_align_right: RELATIVE_ALIGN_PARENT,
        width: Length::points(10.0),
        height: Length::points(8.0),
        margin: Rect::new(
            Length::points(1.0),
            Length::points(2.0),
            Length::ZERO,
            Length::ZERO,
        ),
        ..Style::default()
    }));
    tree.append_child(root, stretched);
    tree.append_child(root, anchor);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[stretched].layout.offset.x, 1.0);
    assert_close(tree.nodes[stretched].layout.size.width, 97.0);
}

#[test]
fn relative_sibling_top_to_parent_bottom_overrides_explicit_height() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(40.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let anchor = tree.push(SimpleNode::new(Style {
        relative_id: 51,
        relative_align_top: RELATIVE_ALIGN_PARENT,
        width: Length::points(10.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let stretched = tree.push(SimpleNode::new(Style {
        relative_align_top: 51,
        relative_align_bottom: RELATIVE_ALIGN_PARENT,
        width: Length::points(10.0),
        height: Length::points(8.0),
        margin: Rect::new(
            Length::ZERO,
            Length::ZERO,
            Length::points(3.0),
            Length::points(4.0),
        ),
        ..Style::default()
    }));
    tree.append_child(root, stretched);
    tree.append_child(root, anchor);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[stretched].layout.offset.y, 3.0);
    assert_close(tree.nodes[stretched].layout.size.height, 73.0);
}

#[test]
fn relative_display_padding_border_content_origin_matrix() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(100.0),
        height: Length::points(80.0),
        padding: Rect::new(
            Length::points(3.0),
            Length::points(7.0),
            Length::points(5.0),
            Length::points(11.0),
        ),
        border: Rect::new(2.0, 1.0, 4.0, 6.0),
        ..Style::default()
    }));
    let anchor = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_id: 10,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    let parent_end = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_align_right: RELATIVE_ALIGN_PARENT,
            relative_align_bottom: RELATIVE_ALIGN_PARENT,
            ..Style::default()
        },
        Size::new(10.0, 8.0),
    ));
    let centered = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_center: RelativeCenter::Both,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    let sibling_after = tree.push(SimpleNode::with_measured_size(
        Style {
            relative_right_of: 10,
            relative_bottom_of: 10,
            ..Style::default()
        },
        Size::new(6.0, 4.0),
    ));
    for child in [anchor, parent_end, centered, sibling_after] {
        tree.append_child(root, child);
    }

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 113.0);
    assert_close(size.height, 106.0);
    assert_close(tree.nodes[anchor].layout.offset.x, 5.0);
    assert_close(tree.nodes[anchor].layout.offset.y, 9.0);
    assert_close(tree.nodes[parent_end].layout.offset.x, 95.0);
    assert_close(tree.nodes[parent_end].layout.offset.y, 81.0);
    assert_close(tree.nodes[centered].layout.offset.x, 45.0);
    assert_close(tree.nodes[centered].layout.offset.y, 44.0);
    assert_close(tree.nodes[sibling_after].layout.offset.x, 25.0);
    assert_close(tree.nodes[sibling_after].layout.offset.y, 19.0);
}
