// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use starlight_layout::{
    AlignItems, BaseLength, BoxSizing, Constraints, Display, LayoutEngine, LayoutResult,
    LayoutTree, Length, MeasureMode, Rect, SideConstraint, SimpleNode, SimpleTree, Size, Style,
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
    measured_size: Size,
    last_constraints: Option<Constraints>,
}

#[derive(Clone, Debug, Default)]
struct MeasuringTree {
    nodes: Vec<MeasuringNode>,
}

impl MeasuringTree {
    fn push(&mut self, style: Style, measured_size: Size) -> usize {
        let id = self.nodes.len();
        self.nodes.push(MeasuringNode {
            style,
            layout: LayoutResult::default(),
            measured_size,
            last_constraints: None,
        });
        id
    }
}

impl LayoutTree for MeasuringTree {
    type NodeId = usize;
    type Children<'a> = std::iter::Empty<usize>;

    fn children(&self, _node: Self::NodeId) -> Self::Children<'_> {
        std::iter::empty()
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
        Some(Size::new(
            constraints.width.clamp(node.measured_size.width),
            constraints.height.clamp(node.measured_size.height),
        ))
    }

    fn has_measure(&self, _node: Self::NodeId) -> bool {
        true
    }
}

#[test]
fn block_leaf_at_most_width_does_not_fill_without_measure() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style::default()));

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(SideConstraint::at_most(100.0), SideConstraint::indefinite()),
    );

    assert_close(size.width, 0.0);
    assert_close(size.height, 0.0);
}

#[test]
fn measured_root_fit_content_percent_argument_caps_final_size() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(
        Style {
            width: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 50.0))),
            height: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 25.0))),
            ..Style::default()
        },
        Size::new(150.0, 70.0),
    );

    let size = LayoutEngine::new().layout_with_owner_constraints(
        &mut tree,
        root,
        Constraints::definite(200.0, 100.0),
    );
    let constraints = tree.nodes[root]
        .last_constraints
        .expect("root should have been measured");

    assert!(constraints.width.near(SideConstraint::at_most(100.0)));
    assert!(constraints.height.near(SideConstraint::at_most(25.0)));
    assert_close(size.width, 100.0);
    assert_close(size.height, 25.0);
}

#[test]
fn measured_root_fit_content_calc_argument_caps_final_size() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(
        Style {
            width: Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
            height: Length::fit_content(Some(BaseLength::fixed_and_percent(5.0, 25.0))),
            ..Style::default()
        },
        Size::new(150.0, 70.0),
    );

    let size = LayoutEngine::new().layout_with_owner_constraints(
        &mut tree,
        root,
        Constraints::definite(200.0, 100.0),
    );
    let constraints = tree.nodes[root]
        .last_constraints
        .expect("root should have been measured");

    assert!(constraints.width.near(SideConstraint::at_most(110.0)));
    assert!(constraints.height.near(SideConstraint::at_most(30.0)));
    assert_close(size.width, 110.0);
    assert_close(size.height, 30.0);
}

#[test]
fn root_block_fit_content_percent_argument_uses_linear_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 50.0))),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 25.0))),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        width: Length::points(120.0),
        height: Length::points(30.0),
        ..Style::default()
    }));
    tree.append_child(root, child);

    let size = LayoutEngine::new().layout_with_owner_constraints(
        &mut tree,
        root,
        Constraints::definite(200.0, 100.0),
    );

    assert_close(size.width, 120.0);
    assert_close(size.height, 30.0);
    assert_close(tree.nodes[root].layout.size.width, 120.0);
    assert_close(tree.nodes[root].layout.size.height, 30.0);
}

#[test]
fn root_block_fit_content_calc_argument_uses_linear_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(5.0, 25.0))),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        width: Length::points(120.0),
        height: Length::points(30.0),
        ..Style::default()
    }));
    tree.append_child(root, child);

    let size = LayoutEngine::new().layout_with_owner_constraints(
        &mut tree,
        root,
        Constraints::definite(200.0, 100.0),
    );

    assert_close(size.width, 120.0);
    assert_close(size.height, 30.0);
    assert_close(tree.nodes[root].layout.size.width, 120.0);
    assert_close(tree.nodes[root].layout.size.height, 30.0);
}

#[test]
fn child_block_fit_content_percent_argument_uses_linear_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 50.0))),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 25.0))),
        ..Style::default()
    }));
    let grandchild = tree.push(SimpleNode::new(Style {
        width: Length::points(120.0),
        height: Length::points(30.0),
        ..Style::default()
    }));
    tree.append_child(root, child);
    tree.append_child(child, grandchild);

    LayoutEngine::new().layout_with_owner_constraints(
        &mut tree,
        root,
        Constraints::definite(200.0, 100.0),
    );

    assert_close(tree.nodes[child].layout.size.width, 120.0);
    assert_close(tree.nodes[child].layout.size.height, 30.0);
}

#[test]
fn child_block_fit_content_calc_argument_uses_linear_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(5.0, 25.0))),
        ..Style::default()
    }));
    let grandchild = tree.push(SimpleNode::new(Style {
        width: Length::points(120.0),
        height: Length::points(30.0),
        ..Style::default()
    }));
    tree.append_child(root, child);
    tree.append_child(child, grandchild);

    LayoutEngine::new().layout_with_owner_constraints(
        &mut tree,
        root,
        Constraints::definite(200.0, 100.0),
    );

    assert_close(tree.nodes[child].layout.size.width, 120.0);
    assert_close(tree.nodes[child].layout.size.height, 30.0);
}

#[test]
fn border_box_explicit_size_includes_padding_and_border() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        box_sizing: BoxSizing::BorderBox,
        width: Length::points(100.0),
        height: Length::points(40.0),
        padding: Rect::new(
            Length::points(10.0),
            Length::points(5.0),
            Length::points(4.0),
            Length::points(6.0),
        ),
        border: Rect::new(2.0, 3.0, 1.0, 2.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, child);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );

    assert_close(size.width, 100.0);
    assert_close(size.height, 40.0);
    assert_close(tree.nodes[child].layout.offset.x, 12.0);
    assert_close(tree.nodes[child].layout.offset.y, 5.0);
    assert_close(tree.nodes[child].layout.size.width, 80.0);
    assert_close(tree.nodes[child].layout.size.height, 10.0);
}

#[test]
fn border_box_aspect_ratio_resolves_border_box_height() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        box_sizing: BoxSizing::BorderBox,
        width: Length::points(100.0),
        aspect_ratio: Some(2.0),
        padding: Rect::new(
            Length::points(10.0),
            Length::points(5.0),
            Length::points(4.0),
            Length::points(6.0),
        ),
        border: Rect::new(2.0, 3.0, 1.0, 2.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, child);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );

    assert_close(size.width, 100.0);
    assert_close(size.height, 50.0);
    assert_close(tree.nodes[root].layout.size.height, 50.0);
    assert_close(tree.nodes[child].layout.offset.y, 5.0);
}

#[test]
fn block_container_baseline_uses_first_child_fallback_baseline() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        width: Length::points(100.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        height: Length::points(10.0),
        margin: Rect::new(
            Length::points(2.0),
            Length::points(3.0),
            Length::points(1.0),
            Length::points(4.0),
        ),
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );

    assert_eq!(tree.nodes[root].layout.baseline, Some(11.0));
    assert_close(tree.nodes[child].layout.size.width, 95.0);
}

#[test]
fn measured_block_leaf_definite_constraints_override_measured_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(10.0, 5.0),
    ));

    let size = run_rust_layout(&mut tree, root, Constraints::definite(100.0, 30.0));

    assert_close(size.width, 100.0);
    assert_close(size.height, 30.0);
}

#[test]
fn measured_block_leaf_at_most_constraint_does_not_clamp_callback_result() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(120.0, 5.0),
    ));

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(SideConstraint::at_most(40.0), SideConstraint::indefinite()),
    );

    assert_close(size.width, 120.0);
    assert_close(size.height, 5.0);
}

#[test]
fn measured_block_leaf_indefinite_constraint_applies_max_before_measure() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(
        Style {
            max_width: Length::points(40.0),
            ..Style::default()
        },
        Size::new(100.0, 5.0),
    );

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    let constraints = tree.nodes[root].last_constraints.unwrap();
    assert_eq!(constraints.width.mode, MeasureMode::AtMost);
    assert_close(constraints.width.size, 40.0);
    assert_close(size.width, 40.0);
}

#[test]
fn measured_block_leaf_definite_width_applies_max_before_measure() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(
        Style {
            width: Length::points(100.0),
            max_width: Length::points(40.0),
            ..Style::default()
        },
        Size::new(10.0, 5.0),
    );

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::definite(120.0),
            SideConstraint::indefinite(),
        ),
    );

    let constraints = tree.nodes[root].last_constraints.unwrap();
    assert_eq!(constraints.width.mode, MeasureMode::Definite);
    assert_close(constraints.width.size, 40.0);
    assert_close(size.width, 40.0);
}

#[test]
fn measured_block_leaf_definite_width_applies_min_before_measure() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(
        Style {
            width: Length::points(20.0),
            min_width: Length::points(40.0),
            ..Style::default()
        },
        Size::new(10.0, 5.0),
    );

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    let constraints = tree.nodes[root].last_constraints.unwrap();
    assert_eq!(constraints.width.mode, MeasureMode::Definite);
    assert_close(constraints.width.size, 40.0);
    assert_close(size.width, 40.0);
}

#[test]
fn negative_padding_is_clamped_to_zero_in_latest_mode() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        padding: Rect::new(
            Length::points(-10.0),
            Length::points(-2.0),
            Length::points(-3.0),
            Length::points(-4.0),
        ),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(10.0, 5.0),
    ));
    tree.append_child(root, child);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );

    assert_close(size.height, 5.0);
    assert_close(tree.nodes[root].layout.padding.left, 0.0);
    assert_close(tree.nodes[root].layout.padding.right, 0.0);
    assert_close(tree.nodes[root].layout.padding.top, 0.0);
    assert_close(tree.nodes[root].layout.padding.bottom, 0.0);
    assert_close(tree.nodes[child].layout.offset.x, 0.0);
    assert_close(tree.nodes[child].layout.offset.y, 0.0);
}

#[test]
fn negative_margin_is_preserved_while_padding_is_clamped() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style::default()));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            margin: Rect::new(
                Length::points(-3.0),
                Length::ZERO,
                Length::points(-2.0),
                Length::ZERO,
            ),
            ..Style::default()
        },
        Size::new(10.0, 5.0),
    ));
    tree.append_child(root, child);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );

    assert_close(size.height, 3.0);
    assert_close(tree.nodes[child].layout.margin.left, -3.0);
    assert_close(tree.nodes[child].layout.margin.top, -2.0);
    assert_close(tree.nodes[child].layout.offset.x, -3.0);
    assert_close(tree.nodes[child].layout.offset.y, -2.0);
}

#[test]
fn vertical_percentage_padding_and_margin_use_width_percent_base() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        width: Length::points(120.0),
        padding: Rect::all(Length::percent(10.0)),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(10.0),
        height: Length::points(5.0),
        margin: Rect::new(
            Length::ZERO,
            Length::ZERO,
            Length::percent(5.0),
            Length::percent(2.0),
        ),
        ..Style::default()
    }));
    tree.append_child(root, child);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::definite(120.0),
            SideConstraint::indefinite(),
        ),
    );

    assert_close(tree.nodes[root].layout.padding.left, 12.0);
    assert_close(tree.nodes[root].layout.padding.top, 12.0);
    assert_close(tree.nodes[child].layout.margin.top, 6.0);
    assert_close(tree.nodes[child].layout.margin.bottom, 2.4);
    assert_close(size.height, 37.0);
}

#[test]
fn block_layout_orders_in_flow_children_by_order() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style::default()));
    let later = tree.push(SimpleNode::new(Style {
        height: Length::points(10.0),
        order: 1,
        ..Style::default()
    }));
    let earlier = tree.push(SimpleNode::new(Style {
        height: Length::points(10.0),
        order: -1,
        ..Style::default()
    }));
    tree.append_child(root, later);
    tree.append_child(root, earlier);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.height, 20.0);
    assert_close(tree.nodes[earlier].layout.offset.y, 0.0);
    assert_close(tree.nodes[later].layout.offset.y, 10.0);
}
