// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use starlight_layout::{
    AlignItems, BaseLength, Constraints, Direction, Display, JustifyContent, LayoutResult,
    LayoutTree, Length, LinearCrossGravity, LinearGravity, LinearLayoutGravity, LinearOrientation,
    MeasureMode, PositionType, Rect, SideConstraint, SimpleNode, SimpleTree, Size, Style,
    Visibility,
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
fn vertical_linear_stacks_children_and_stretches_cross_axis() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        width: Length::points(100.0),
        ..Style::default()
    }));
    let first = fixed_linear_child(&mut tree, Length::Auto, Length::points(10.0));
    let second = fixed_linear_child(&mut tree, Length::Auto, Length::points(20.0));
    tree.append_child(root, first);
    tree.append_child(root, second);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );

    assert_close(size.width, 100.0);
    assert_close(size.height, 30.0);
    assert_close(tree.nodes[first].layout.offset.y, 0.0);
    assert_close(tree.nodes[second].layout.offset.y, 10.0);
    assert_close(tree.nodes[first].layout.size.width, 100.0);
    assert_close(tree.nodes[second].layout.size.width, 100.0);
}

#[test]
fn visibility_hidden_and_collapse_linear_children_participate_in_layout() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        width: Length::points(100.0),
        ..Style::default()
    }));
    let hidden = tree.push(SimpleNode::new(Style {
        width: Length::Auto,
        height: Length::points(10.0),
        visibility: Visibility::Hidden,
        ..Style::default()
    }));
    let collapsed = tree.push(SimpleNode::new(Style {
        width: Length::Auto,
        height: Length::points(20.0),
        visibility: Visibility::Collapse,
        ..Style::default()
    }));
    tree.append_child(root, hidden);
    tree.append_child(root, collapsed);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );

    assert_close(size.width, 100.0);
    assert_close(size.height, 30.0);
    assert_close(tree.nodes[hidden].layout.offset.y, 0.0);
    assert_close(tree.nodes[collapsed].layout.offset.y, 10.0);
    assert_close(tree.nodes[hidden].layout.size.width, 100.0);
    assert_close(tree.nodes[collapsed].layout.size.width, 100.0);
    assert_close(tree.nodes[hidden].layout.size.height, 10.0);
    assert_close(tree.nodes[collapsed].layout.size.height, 20.0);
}

#[test]
fn vertical_linear_measured_child_percent_calc_min_max_uses_container_content_base() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        width: Length::points(126.0),
        height: Length::points(92.0),
        justify_content: JustifyContent::Center,
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            width: Length::Auto,
            height: Length::Auto,
            min_width: Length::percent(25.0),
            max_width: Length::calc(8.0, 35.0),
            min_height: Length::calc(3.0, 18.0),
            max_height: Length::percent(70.0),
            padding: Rect::all(Length::points(1.0)),
            border: Rect::all(1.0),
            ..Style::default()
        },
        Size::new(80.0, 9.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(126.0, 92.0));

    assert_close(tree.nodes[child].layout.size.width, 56.0);
    assert_close(tree.nodes[child].layout.size.height, 24.0);
}

#[test]
fn vertical_linear_percent_cross_size_remeasures_final_constraint() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        width: Length::points(100.0),
        height: Length::points(40.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::percent(50.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 40.0));

    assert_close(tree.nodes[child].layout.size.width, 50.0);
    assert_close(tree.nodes[child].layout.size.height, 10.0);
}

#[test]
fn horizontal_linear_percent_cross_size_with_stretch_remeasures_final_constraint() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(20.0),
        height: Length::percent(50.0),
        linear_layout_gravity: LinearLayoutGravity::Stretch,
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 80.0));

    assert_close(tree.nodes[child].layout.size.width, 20.0);
    assert_close(tree.nodes[child].layout.size.height, 80.0);
}

#[test]
fn horizontal_linear_fit_content_cross_axis_argument_bounds_measured_child() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(100.0),
        height: Length::fit_content(Some(BaseLength::fixed(30.0))),
        ..Style::default()
    }));
    let child = tree.push(MeasuringNode::measured(
        Style::default(),
        Size::new(20.0, 50.0),
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

    assert_close(size.width, 100.0);
    assert_close(size.height, 30.0);
    assert_close(tree.nodes[child].layout.size.width, 20.0);
    assert_close(tree.nodes[child].layout.size.height, 30.0);
}

#[test]
fn display_none_child_is_laid_out_as_zero_and_skipped_by_linear_stack() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        width: Length::points(100.0),
        ..Style::default()
    }));
    let first = fixed_linear_child(&mut tree, Length::Auto, Length::points(10.0));
    let hidden = tree.push(SimpleNode::new(Style {
        display: Display::None,
        width: Length::points(100.0),
        height: Length::points(50.0),
        ..Style::default()
    }));
    let second = fixed_linear_child(&mut tree, Length::Auto, Length::points(20.0));
    tree.append_child(root, first);
    tree.append_child(root, hidden);
    tree.append_child(root, second);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );

    assert_close(size.width, 100.0);
    assert_close(size.height, 30.0);
    assert_close(tree.nodes[first].layout.offset.y, 0.0);
    assert_close(tree.nodes[second].layout.offset.y, 10.0);
    assert_eq!(tree.nodes[hidden].layout.size, Size::ZERO);
    assert_close(tree.nodes[hidden].layout.offset.x, 0.0);
    assert_close(tree.nodes[hidden].layout.offset.y, 0.0);
}

#[test]
fn display_none_child_uses_parent_padding_bound_origin() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        width: Length::points(100.0),
        border: Rect::new(2.0, 1.0, 3.0, 1.0),
        padding: Rect::all(Length::points(4.0)),
        ..Style::default()
    }));
    let hidden = tree.push(SimpleNode::new(Style {
        display: Display::None,
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, hidden);

    run_rust_layout(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );

    assert_eq!(tree.nodes[hidden].layout.size, Size::ZERO);
    assert_close(tree.nodes[hidden].layout.offset.x, 2.0);
    assert_close(tree.nodes[hidden].layout.offset.y, 3.0);
}

#[test]
fn display_none_parent_clears_descendant_layouts() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        width: Length::points(100.0),
        ..Style::default()
    }));
    let parent = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        width: Length::points(40.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        width: Length::points(12.0),
        height: Length::points(6.0),
        ..Style::default()
    }));
    tree.append_child(parent, child);
    tree.append_child(root, parent);

    run_rust_layout(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );
    assert_close(tree.nodes[child].layout.size.width, 12.0);
    assert_close(tree.nodes[child].layout.size.height, 6.0);

    tree.nodes[parent].style.display = Display::None;
    run_rust_layout(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );

    assert_eq!(tree.nodes[parent].layout.size, Size::ZERO);
    assert_eq!(tree.nodes[child].layout.size, Size::ZERO);
    assert_close(tree.nodes[child].layout.offset.x, 0.0);
    assert_close(tree.nodes[child].layout.offset.y, 0.0);
}

#[test]
fn horizontal_linear_center_uses_remaining_main_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        justify_content: JustifyContent::Center,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let first = fixed_linear_child(&mut tree, Length::points(10.0), Length::Auto);
    let second = fixed_linear_child(&mut tree, Length::points(20.0), Length::Auto);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[first].layout.offset.x, 35.0);
    assert_close(tree.nodes[second].layout.offset.x, 45.0);
    assert_close(tree.nodes[first].layout.size.height, 20.0);
    assert_close(tree.nodes[second].layout.size.height, 20.0);
}

#[test]
fn horizontal_linear_auto_cross_axis_uses_parent_height_constraint_for_stretch() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(100.0),
        height: Length::Auto,
        ..Style::default()
    }));
    let child = fixed_linear_child(&mut tree, Length::points(10.0), Length::Auto);
    tree.append_child(root, child);

    let size = run_rust_layout(&mut tree, root, Constraints::definite(100.0, 80.0));

    assert_close(size.width, 100.0);
    assert_close(size.height, 80.0);
    assert_close(tree.nodes[child].layout.size.width, 10.0);
    assert_close(tree.nodes[child].layout.size.height, 80.0);
}

#[test]
fn horizontal_linear_center_uses_negative_remaining_main_space_when_overflowing() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        justify_content: JustifyContent::Center,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let first = fixed_linear_child(&mut tree, Length::points(80.0), Length::Auto);
    let second = fixed_linear_child(&mut tree, Length::points(70.0), Length::Auto);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[first].layout.offset.x, -25.0);
    assert_close(tree.nodes[second].layout.offset.x, 55.0);
}

#[test]
fn rtl_horizontal_linear_positions_items_from_right_edge() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        direction: Direction::Rtl,
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let first = fixed_linear_child(&mut tree, Length::points(10.0), Length::Auto);
    let second = fixed_linear_child(&mut tree, Length::points(20.0), Length::Auto);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[first].layout.offset.x, 90.0);
    assert_close(tree.nodes[second].layout.offset.x, 70.0);
}

#[test]
fn vertical_linear_center_uses_negative_remaining_main_space_for_container_baseline() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        justify_content: JustifyContent::Center,
        width: Length::points(20.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::with_measured_size_and_baseline(
        Style::default(),
        Size::new(10.0, 80.0),
        10.0,
    ));
    let second = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(10.0, 70.0),
    ));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(20.0, 100.0));

    assert_close(tree.nodes[first].layout.offset.y, -25.0);
    assert_close(tree.nodes[second].layout.offset.y, 55.0);
    assert_close(tree.nodes[root].layout.baseline.unwrap(), -15.0);
}

#[test]
fn vertical_linear_end_gravity_offsets_container_baseline_by_remaining_main_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_gravity: LinearGravity::End,
        width: Length::points(20.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::with_measured_size_and_baseline(
        Style::default(),
        Size::new(10.0, 20.0),
        5.0,
    ));
    let second = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(10.0, 10.0),
    ));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(20.0, 100.0));

    assert_close(tree.nodes[first].layout.offset.y, 70.0);
    assert_close(tree.nodes[second].layout.offset.y, 90.0);
    assert_close(tree.nodes[root].layout.baseline.unwrap(), 75.0);
}

#[test]
fn horizontal_linear_empty_container_exports_no_baseline() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    }));

    let size = run_rust_layout(&mut tree, root, Constraints::definite(20.0, 10.0));

    assert_close(size.width, 20.0);
    assert_close(size.height, 10.0);
    assert_eq!(tree.nodes[root].layout.baseline, None);
}

#[test]
fn vertical_linear_empty_container_exports_no_baseline() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    }));

    let size = run_rust_layout(&mut tree, root, Constraints::definite(20.0, 10.0));

    assert_close(size.width, 20.0);
    assert_close(size.height, 10.0);
    assert_eq!(tree.nodes[root].layout.baseline, None);
}

#[test]
fn horizontal_linear_child_without_baseline_exports_fallback_baseline() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        ..Style::default()
    }));
    let child = fixed_linear_child(&mut tree, Length::points(20.0), Length::points(10.0));
    tree.append_child(root, child);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 20.0);
    assert_close(size.height, 10.0);
    assert_close(tree.nodes[root].layout.baseline.unwrap(), 10.0);
}

#[test]
fn horizontal_linear_container_baseline_uses_largest_child_baseline() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(100.0),
        height: Length::points(40.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::with_measured_size_and_baseline(
        Style::default(),
        Size::new(10.0, 30.0),
        5.0,
    ));
    let second = tree.push(SimpleNode::with_measured_size_and_baseline(
        Style::default(),
        Size::new(10.0, 20.0),
        15.0,
    ));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 40.0));

    assert_close(tree.nodes[root].layout.baseline.unwrap(), 15.0);
}

#[test]
fn vertical_linear_child_without_baseline_exports_fallback_baseline() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        ..Style::default()
    }));
    let child = fixed_linear_child(&mut tree, Length::points(20.0), Length::points(10.0));
    tree.append_child(root, child);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 20.0);
    assert_close(size.height, 10.0);
    assert_close(tree.nodes[root].layout.baseline.unwrap(), 10.0);
}

#[test]
fn horizontal_reverse_linear_positions_items_from_right_edge() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::HorizontalReverse,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let first = fixed_linear_child(&mut tree, Length::points(10.0), Length::Auto);
    let second = fixed_linear_child(&mut tree, Length::points(20.0), Length::Auto);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[first].layout.offset.x, 90.0);
    assert_close(tree.nodes[second].layout.offset.x, 70.0);
    assert_close(tree.nodes[first].layout.size.height, 20.0);
    assert_close(tree.nodes[second].layout.size.height, 20.0);
}

#[test]
fn rtl_horizontal_reverse_linear_positions_items_from_left_edge() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        direction: Direction::Rtl,
        linear_orientation: LinearOrientation::HorizontalReverse,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let first = fixed_linear_child(&mut tree, Length::points(10.0), Length::Auto);
    let second = fixed_linear_child(&mut tree, Length::points(20.0), Length::Auto);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[second].layout.offset.x, 10.0);
}

#[test]
fn horizontal_reverse_linear_gravity_left_packs_items_at_left_edge() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::HorizontalReverse,
        linear_gravity: LinearGravity::Left,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let first = fixed_linear_child(&mut tree, Length::points(10.0), Length::Auto);
    let second = fixed_linear_child(&mut tree, Length::points(20.0), Length::Auto);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[first].layout.offset.x, 20.0);
    assert_close(tree.nodes[second].layout.offset.x, 0.0);
}

#[test]
fn linear_absolute_child_without_insets_uses_linear_static_alignment_matrix() {
    let cases = [
        (
            LinearOrientation::Horizontal,
            LinearGravity::End,
            LinearLayoutGravity::End,
            90.0,
            42.0,
        ),
        (
            LinearOrientation::Horizontal,
            LinearGravity::Center,
            LinearLayoutGravity::Center,
            45.0,
            21.0,
        ),
        (
            LinearOrientation::Vertical,
            LinearGravity::Start,
            LinearLayoutGravity::Start,
            0.0,
            0.0,
        ),
    ];

    for (orientation, gravity, layout_gravity, expected_x, expected_y) in cases {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Linear,
            linear_orientation: orientation,
            linear_gravity: gravity,
            width: Length::points(100.0),
            height: Length::points(50.0),
            ..Style::default()
        }));
        let absolute = tree.push(SimpleNode::new(Style {
            position: PositionType::Absolute,
            linear_layout_gravity: layout_gravity,
            width: Length::points(10.0),
            height: Length::points(8.0),
            ..Style::default()
        }));
        tree.append_child(root, absolute);

        run_rust_layout(&mut tree, root, Constraints::definite(100.0, 50.0));

        assert_close(tree.nodes[absolute].layout.offset.x, expected_x);
        assert_close(tree.nodes[absolute].layout.offset.y, expected_y);
    }
}

#[test]
fn linear_absolute_static_position_with_margins_uses_margin_bound_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Center,
        width: Length::points(100.0),
        height: Length::points(50.0),
        ..Style::default()
    }));
    let absolute = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        linear_layout_gravity: LinearLayoutGravity::End,
        width: Length::points(10.0),
        height: Length::points(8.0),
        margin: Rect::new(
            Length::points(3.0),
            Length::points(7.0),
            Length::points(4.0),
            Length::points(6.0),
        ),
        ..Style::default()
    }));
    tree.append_child(root, absolute);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 50.0));

    assert_close(tree.nodes[absolute].layout.offset.x, 43.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 36.0);
    assert_close(tree.nodes[absolute].layout.size.width, 10.0);
    assert_close(tree.nodes[absolute].layout.size.height, 8.0);
}

#[test]
fn linear_absolute_rtl_static_position_with_margins_uses_reversed_front() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        direction: Direction::Rtl,
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::None,
        width: Length::points(100.0),
        height: Length::points(50.0),
        ..Style::default()
    }));
    let absolute = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        linear_layout_gravity: LinearLayoutGravity::Start,
        width: Length::points(10.0),
        height: Length::points(8.0),
        margin: Rect::new(
            Length::points(3.0),
            Length::points(7.0),
            Length::points(4.0),
            Length::points(6.0),
        ),
        ..Style::default()
    }));
    tree.append_child(root, absolute);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 50.0));

    assert_close(tree.nodes[absolute].layout.offset.x, 83.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 4.0);
    assert_close(tree.nodes[absolute].layout.size.width, 10.0);
    assert_close(tree.nodes[absolute].layout.size.height, 8.0);
}

#[test]
fn linear_absolute_child_layout_gravity_overrides_align_self_and_cross_gravity() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Vertical,
        linear_gravity: LinearGravity::Center,
        align_items: AlignItems::FlexStart,
        linear_cross_gravity: LinearCrossGravity::End,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let absolute = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        width: Length::points(20.0),
        height: Length::points(10.0),
        align_self: Some(AlignItems::FlexEnd),
        linear_layout_gravity: LinearLayoutGravity::Left,
        ..Style::default()
    }));
    tree.append_child(root, absolute);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

    assert_close(tree.nodes[absolute].layout.offset.x, 0.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 45.0);
}

#[test]
fn linear_absolute_child_cross_axis_uses_cpp_computed_layout_gravity_order() {
    {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Linear,
            linear_orientation: LinearOrientation::Horizontal,
            align_items: AlignItems::FlexStart,
            linear_cross_gravity: LinearCrossGravity::End,
            width: Length::points(100.0),
            height: Length::points(50.0),
            ..Style::default()
        }));
        let absolute = tree.push(SimpleNode::new(Style {
            position: PositionType::Absolute,
            align_self: Some(AlignItems::Center),
            width: Length::points(20.0),
            height: Length::points(10.0),
            ..Style::default()
        }));
        tree.append_child(root, absolute);

        run_rust_layout(&mut tree, root, Constraints::definite(100.0, 50.0));

        assert_close(tree.nodes[absolute].layout.offset.x, 0.0);
        assert_close(tree.nodes[absolute].layout.offset.y, 20.0);
    }

    {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Linear,
            linear_orientation: LinearOrientation::Horizontal,
            align_items: AlignItems::FlexStart,
            linear_cross_gravity: LinearCrossGravity::End,
            width: Length::points(100.0),
            height: Length::points(50.0),
            ..Style::default()
        }));
        let absolute = tree.push(SimpleNode::new(Style {
            position: PositionType::Absolute,
            width: Length::points(20.0),
            height: Length::points(10.0),
            ..Style::default()
        }));
        tree.append_child(root, absolute);

        run_rust_layout(&mut tree, root, Constraints::definite(100.0, 50.0));

        assert_close(tree.nodes[absolute].layout.offset.x, 0.0);
        assert_close(tree.nodes[absolute].layout.offset.y, 40.0);
    }

    {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Linear,
            linear_orientation: LinearOrientation::Horizontal,
            align_items: AlignItems::FlexEnd,
            linear_cross_gravity: LinearCrossGravity::None,
            width: Length::points(100.0),
            height: Length::points(50.0),
            ..Style::default()
        }));
        let absolute = tree.push(SimpleNode::new(Style {
            position: PositionType::Absolute,
            width: Length::points(20.0),
            height: Length::points(10.0),
            ..Style::default()
        }));
        tree.append_child(root, absolute);

        run_rust_layout(&mut tree, root, Constraints::definite(100.0, 50.0));

        assert_close(tree.nodes[absolute].layout.offset.x, 0.0);
        assert_close(tree.nodes[absolute].layout.offset.y, 40.0);
    }

    {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Linear,
            linear_orientation: LinearOrientation::Horizontal,
            align_items: AlignItems::Stretch,
            linear_cross_gravity: LinearCrossGravity::None,
            width: Length::points(100.0),
            height: Length::points(50.0),
            ..Style::default()
        }));
        let absolute = tree.push(SimpleNode::new(Style {
            position: PositionType::Absolute,
            width: Length::points(20.0),
            height: Length::points(10.0),
            ..Style::default()
        }));
        tree.append_child(root, absolute);

        run_rust_layout(&mut tree, root, Constraints::definite(100.0, 50.0));

        assert_close(tree.nodes[absolute].layout.offset.x, 0.0);
        assert_close(tree.nodes[absolute].layout.offset.y, 0.0);
    }
}

#[test]
fn linear_absolute_vertical_child_uses_cpp_main_axis_static_position() {
    {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Linear,
            linear_orientation: LinearOrientation::Vertical,
            linear_gravity: LinearGravity::Center,
            width: Length::points(50.0),
            height: Length::points(100.0),
            ..Style::default()
        }));
        let absolute = tree.push(SimpleNode::new(Style {
            position: PositionType::Absolute,
            width: Length::points(20.0),
            height: Length::points(10.0),
            ..Style::default()
        }));
        tree.append_child(root, absolute);

        run_rust_layout(&mut tree, root, Constraints::definite(50.0, 100.0));

        assert_close(tree.nodes[absolute].layout.offset.x, 0.0);
        assert_close(tree.nodes[absolute].layout.offset.y, 45.0);
    }

    {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Linear,
            linear_orientation: LinearOrientation::Vertical,
            linear_gravity: LinearGravity::End,
            width: Length::points(50.0),
            height: Length::points(100.0),
            ..Style::default()
        }));
        let absolute = tree.push(SimpleNode::new(Style {
            position: PositionType::Absolute,
            width: Length::points(20.0),
            height: Length::points(10.0),
            ..Style::default()
        }));
        tree.append_child(root, absolute);

        run_rust_layout(&mut tree, root, Constraints::definite(50.0, 100.0));

        assert_close(tree.nodes[absolute].layout.offset.x, 0.0);
        assert_close(tree.nodes[absolute].layout.offset.y, 90.0);
    }

    {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Linear,
            linear_orientation: LinearOrientation::Vertical,
            linear_gravity: LinearGravity::Bottom,
            width: Length::points(50.0),
            height: Length::points(100.0),
            ..Style::default()
        }));
        let absolute = tree.push(SimpleNode::new(Style {
            position: PositionType::Absolute,
            width: Length::points(20.0),
            height: Length::points(10.0),
            ..Style::default()
        }));
        tree.append_child(root, absolute);

        run_rust_layout(&mut tree, root, Constraints::definite(50.0, 100.0));

        assert_close(tree.nodes[absolute].layout.offset.x, 0.0);
        assert_close(tree.nodes[absolute].layout.offset.y, 90.0);
    }

    {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Linear,
            linear_orientation: LinearOrientation::Vertical,
            linear_gravity: LinearGravity::Top,
            width: Length::points(50.0),
            height: Length::points(100.0),
            ..Style::default()
        }));
        let absolute = tree.push(SimpleNode::new(Style {
            position: PositionType::Absolute,
            width: Length::points(20.0),
            height: Length::points(10.0),
            ..Style::default()
        }));
        tree.append_child(root, absolute);

        run_rust_layout(&mut tree, root, Constraints::definite(50.0, 100.0));

        assert_close(tree.nodes[absolute].layout.offset.x, 0.0);
        assert_close(tree.nodes[absolute].layout.offset.y, 0.0);
    }
}

#[test]
fn linear_fixed_descendant_without_insets_uses_root_linear_static_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Center,
        width: Length::points(100.0),
        height: Length::points(50.0),
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
        linear_layout_gravity: LinearLayoutGravity::End,
        width: Length::points(10.0),
        height: Length::points(8.0),
        ..Style::default()
    }));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 50.0));

    assert_close(tree.nodes[fixed].layout.offset.x, 45.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 42.0);
}

#[test]
fn linear_fixed_static_position_with_margins_uses_margin_bound_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Center,
        width: Length::points(100.0),
        height: Length::points(50.0),
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
        linear_layout_gravity: LinearLayoutGravity::End,
        width: Length::points(10.0),
        height: Length::points(8.0),
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

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 50.0));

    assert_close(tree.nodes[fixed].layout.offset.x, 43.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 36.0);
    assert_close(tree.nodes[fixed].layout.size.width, 10.0);
    assert_close(tree.nodes[fixed].layout.size.height, 8.0);
}

#[test]
fn linear_fixed_rtl_static_position_with_margins_uses_reversed_front() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        direction: Direction::Rtl,
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::None,
        width: Length::points(100.0),
        height: Length::points(50.0),
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
        linear_layout_gravity: LinearLayoutGravity::Start,
        width: Length::points(10.0),
        height: Length::points(8.0),
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

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 50.0));

    assert_close(tree.nodes[fixed].layout.offset.x, 83.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 4.0);
    assert_close(tree.nodes[fixed].layout.size.width, 10.0);
    assert_close(tree.nodes[fixed].layout.size.height, 8.0);
}

#[test]
fn linear_fixed_start_insets_override_static_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Center,
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
        linear_layout_gravity: LinearLayoutGravity::End,
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
fn linear_fixed_paired_insets_with_explicit_size_use_start_insets() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Center,
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
        linear_layout_gravity: LinearLayoutGravity::End,
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
fn linear_fixed_end_insets_override_static_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Center,
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
        linear_layout_gravity: LinearLayoutGravity::End,
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
fn linear_fixed_end_insets_with_margins_position_margin_box() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Center,
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
        linear_layout_gravity: LinearLayoutGravity::End,
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
fn linear_fixed_vertical_descendant_uses_center_main_axis_static_position() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Vertical,
        linear_gravity: LinearGravity::Center,
        width: Length::points(50.0),
        height: Length::points(100.0),
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
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 100.0));

    assert_close(tree.nodes[fixed].layout.offset.x, 0.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 45.0);
}

#[test]
fn linear_fixed_vertical_descendant_uses_end_main_axis_static_position() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Vertical,
        linear_gravity: LinearGravity::End,
        width: Length::points(50.0),
        height: Length::points(100.0),
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
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 100.0));

    assert_close(tree.nodes[fixed].layout.offset.x, 0.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 90.0);
}

#[test]
fn linear_fixed_vertical_descendant_uses_physical_bottom_main_axis_static_position() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Vertical,
        linear_gravity: LinearGravity::Bottom,
        width: Length::points(50.0),
        height: Length::points(100.0),
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
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 100.0));

    assert_close(tree.nodes[fixed].layout.offset.x, 0.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 90.0);
}

#[test]
fn linear_fixed_vertical_descendant_uses_physical_top_main_axis_static_position() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Vertical,
        linear_gravity: LinearGravity::Top,
        width: Length::points(50.0),
        height: Length::points(100.0),
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
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 100.0));

    assert_close(tree.nodes[fixed].layout.offset.x, 0.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 0.0);
}

#[test]
fn linear_fixed_percent_insets_and_size_resolve_against_root_linear_containing_block() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
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
fn linear_fixed_percent_end_insets_resolve_against_root_linear_containing_block() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
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
fn linear_fixed_auto_size_between_insets_strips_margins() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
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
fn linear_fixed_single_insets_strip_at_most_measure_constraints() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Linear,
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
    assert!(constraints.width.near(SideConstraint::at_most(80.0)));
    assert!(constraints.height.near(SideConstraint::at_most(25.0)));
    assert_close(tree.nodes[fixed].layout.offset.x, 13.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 19.0);
    assert_close(tree.nodes[fixed].layout.size.width, 80.0);
    assert_close(tree.nodes[fixed].layout.size.height, 25.0);
}

#[test]
fn linear_fixed_descendant_uses_linear_root_padding_box_offset() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
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
fn linear_absolute_percent_insets_and_size_resolve_against_linear_containing_block() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
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
fn linear_absolute_percent_end_insets_resolve_against_linear_containing_block() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
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
fn linear_absolute_auto_size_stretches_between_start_and_end_insets() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Center,
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
fn linear_absolute_auto_size_between_insets_strips_margins() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Center,
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
fn linear_absolute_auto_size_paired_insets_fill_padding_box_minus_margins() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Center,
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
fn linear_absolute_single_insets_strip_at_most_measure_constraints() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Center,
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
    assert!(constraints.width.near(SideConstraint::at_most(80.0)));
    assert!(constraints.height.near(SideConstraint::at_most(25.0)));
    assert_close(tree.nodes[absolute].layout.offset.x, 13.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 19.0);
    assert_close(tree.nodes[absolute].layout.size.width, 80.0);
    assert_close(tree.nodes[absolute].layout.size.height, 25.0);
}

#[test]
fn linear_absolute_start_insets_override_static_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Center,
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
        linear_layout_gravity: LinearLayoutGravity::End,
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
fn linear_absolute_end_insets_with_margins_position_margin_box() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Center,
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
        linear_layout_gravity: LinearLayoutGravity::End,
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
fn linear_absolute_paired_insets_with_explicit_size_use_start_insets() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Center,
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
        linear_layout_gravity: LinearLayoutGravity::End,
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
fn horizontal_linear_at_most_main_axis_shrink_wraps_content() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        height: Length::points(20.0),
        ..Style::default()
    }));
    let first = fixed_linear_child(&mut tree, Length::points(10.0), Length::Auto);
    let second = fixed_linear_child(&mut tree, Length::points(20.0), Length::Auto);
    tree.append_child(root, first);
    tree.append_child(root, second);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::at_most(100.0),
            SideConstraint::definite(20.0),
        ),
    );

    assert_close(size.width, 30.0);
    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[second].layout.offset.x, 10.0);
}

#[test]
fn horizontal_linear_at_most_main_axis_keeps_overflow_content_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        height: Length::points(20.0),
        ..Style::default()
    }));
    let first = fixed_linear_child(&mut tree, Length::points(80.0), Length::Auto);
    let second = fixed_linear_child(&mut tree, Length::points(70.0), Length::Auto);
    tree.append_child(root, first);
    tree.append_child(root, second);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::at_most(100.0),
            SideConstraint::definite(20.0),
        ),
    );

    assert_close(size.width, 150.0);
    assert_close(tree.nodes[second].layout.offset.x, 80.0);
}

#[test]
fn horizontal_linear_container_min_width_and_max_height_clamp_wrap_content_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        min_width: Length::points(40.0),
        max_height: Length::points(25.0),
        ..Style::default()
    }));
    let child = fixed_linear_child(&mut tree, Length::points(20.0), Length::points(30.0));
    tree.append_child(root, child);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 40.0);
    assert_close(size.height, 25.0);
}

#[test]
fn vertical_linear_container_max_width_and_min_height_clamp_wrap_content_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        max_width: Length::points(60.0),
        min_height: Length::points(40.0),
        ..Style::default()
    }));
    let child = fixed_linear_child(&mut tree, Length::points(100.0), Length::points(10.0));
    tree.append_child(root, child);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 60.0);
    assert_close(size.height, 40.0);
}

#[test]
fn linear_container_padding_border_prevents_negative_content_size_under_tight_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
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
fn horizontal_linear_auto_main_axis_keeps_initial_size_after_percent_main_margins_resolve() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        height: Length::points(10.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(100.0),
        height: Length::points(10.0),
        margin: Rect::new(
            Length::percent(10.0),
            Length::percent(10.0),
            Length::ZERO,
            Length::ZERO,
        ),
        ..Style::default()
    }));
    tree.append_child(root, child);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 100.0);
    assert_close(size.height, 10.0);
    assert_close(tree.nodes[child].layout.offset.x, 10.0);
    assert_close(tree.nodes[child].layout.size.width, 100.0);
    assert_close(tree.nodes[child].layout.margin.left, 10.0);
    assert_close(tree.nodes[child].layout.margin.right, 10.0);
}

#[test]
fn vertical_linear_auto_main_axis_keeps_initial_size_after_percent_main_margins_resolve() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(100.0),
        height: Length::points(100.0),
        margin: Rect::new(
            Length::ZERO,
            Length::ZERO,
            Length::percent(10.0),
            Length::percent(10.0),
        ),
        ..Style::default()
    }));
    tree.append_child(root, child);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 100.0);
    assert_close(size.height, 100.0);
    assert_close(tree.nodes[child].layout.offset.y, 10.0);
    assert_close(tree.nodes[child].layout.size.height, 100.0);
    assert_close(tree.nodes[child].layout.margin.top, 10.0);
    assert_close(tree.nodes[child].layout.margin.bottom, 10.0);
}

#[test]
fn horizontal_linear_at_most_main_axis_does_not_enable_linear_weight() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        height: Length::points(20.0),
        ..Style::default()
    }));
    let weighted = tree.push(SimpleNode::new(Style {
        linear_weight: 1.0,
        ..Style::default()
    }));
    tree.append_child(root, weighted);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::at_most(100.0),
            SideConstraint::definite(20.0),
        ),
    );

    assert_close(size.width, 0.0);
    assert_close(tree.nodes[weighted].layout.size.width, 0.0);
}

#[test]
fn vertical_linear_at_most_zero_first_baseline_exports_fallback_baseline() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_gravity: LinearGravity::End,
        linear_weight_sum: 4.0,
        ..Style::default()
    }));
    let weighted_first = tree.push(SimpleNode::new(Style {
        width: Length::points(10.0),
        linear_weight: 1.0,
        order: 0,
        ..Style::default()
    }));
    let fixed = tree.push(SimpleNode::new(Style {
        width: Length::points(11.0),
        height: Length::points(12.0),
        order: 1,
        ..Style::default()
    }));
    let weighted_last = tree.push(SimpleNode::new(Style {
        width: Length::points(14.0),
        linear_weight: 1.0,
        order: 2,
        ..Style::default()
    }));
    tree.append_child(root, fixed);
    tree.append_child(root, weighted_first);
    tree.append_child(root, weighted_last);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::at_most(142.0),
            SideConstraint::at_most(96.0),
        ),
    );

    assert_close(size.height, 12.0);
    assert_eq!(tree.nodes[root].layout.baseline, None);
    assert_close(
        tree.nodes[root].layout.baseline.unwrap_or(size.height),
        12.0,
    );
}

#[test]
fn vertical_linear_at_most_cross_axis_does_not_stretch_auto_child() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        ..Style::default()
    }));
    let child = fixed_linear_child(&mut tree, Length::Auto, Length::points(10.0));
    tree.append_child(root, child);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(SideConstraint::at_most(100.0), SideConstraint::indefinite()),
    );

    assert_close(size.width, 0.0);
    assert_close(size.height, 10.0);
    assert_close(tree.nodes[child].layout.size.width, 0.0);
}

#[test]
fn vertical_linear_at_most_cross_axis_min_width_growth_does_not_final_stretch_auto_child() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        direction: Direction::Rtl,
        min_width: Length::points(20.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        width: Length::Auto,
        height: Length::percent(40.0),
        min_width: Length::points(12.0),
        ..Style::default()
    }));
    let wider_sibling = fixed_linear_child(&mut tree, Length::points(14.0), Length::points(1.0));
    tree.append_child(root, child);
    tree.append_child(root, wider_sibling);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(SideConstraint::at_most(100.0), SideConstraint::indefinite()),
    );

    assert_close(size.width, 20.0);
    assert_close(tree.nodes[child].layout.size.width, 12.0);
    assert_close(tree.nodes[child].layout.offset.x, 8.0);
}

#[test]
fn vertical_linear_center_child_gets_at_most_cross_constraint() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Linear,
        align_items: AlignItems::Center,
        width: Length::points(100.0),
        ..Style::default()
    }));
    let child = tree.push(MeasuringNode::measured(
        Style::default(),
        Size::new(150.0, 10.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(
        &mut tree,
        root,
        Constraints::new(SideConstraint::indefinite(), SideConstraint::indefinite()),
    );

    let constraints = tree.nodes[child].last_constraints.unwrap();
    assert_eq!(constraints.width.mode, MeasureMode::AtMost);
    assert_close(constraints.width.size, 100.0);
    assert_close(tree.nodes[child].layout.size.width, 100.0);
}

#[test]
fn vertical_linear_at_most_cross_axis_passes_at_most_to_measured_child() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Linear,
        ..Style::default()
    }));
    let child = tree.push(MeasuringNode::measured(
        Style::default(),
        Size::new(150.0, 10.0),
    ));
    tree.append_child(root, child);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(SideConstraint::at_most(100.0), SideConstraint::indefinite()),
    );

    let constraints = tree.nodes[child].last_constraints.unwrap();
    assert_eq!(constraints.width.mode, MeasureMode::AtMost);
    assert_close(constraints.width.size, 100.0);
    assert_close(size.width, 100.0);
    assert_close(tree.nodes[child].layout.size.width, 100.0);
}

#[test]
fn horizontal_linear_auto_cross_axis_passes_parent_height_constraint_to_measured_child() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(100.0),
        height: Length::Auto,
        ..Style::default()
    }));
    let child = tree.push(MeasuringNode::measured(
        Style {
            width: Length::points(10.0),
            ..Style::default()
        },
        Size::new(10.0, 150.0),
    ));
    tree.append_child(root, child);

    let size = run_rust_layout(&mut tree, root, Constraints::definite(100.0, 80.0));

    let constraints = tree.nodes[child].last_constraints.unwrap();
    assert_eq!(constraints.height.mode, MeasureMode::Definite);
    assert_close(constraints.height.size, 80.0);
    assert_close(size.height, 80.0);
    assert_close(tree.nodes[child].layout.size.height, 80.0);
}

#[test]
fn vertical_linear_indefinite_cross_axis_does_not_stretch_narrow_measured_child() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Linear,
        ..Style::default()
    }));
    let wide = tree.push(MeasuringNode::measured(
        Style::default(),
        Size::new(50.0, 10.0),
    ));
    let narrow = tree.push(MeasuringNode::measured(
        Style::default(),
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, wide);
    tree.append_child(root, narrow);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(SideConstraint::indefinite(), SideConstraint::indefinite()),
    );

    let constraints = tree.nodes[narrow].last_constraints.unwrap();
    assert_eq!(constraints.width.mode, MeasureMode::Indefinite);
    assert_close(size.width, 50.0);
    assert_close(tree.nodes[narrow].layout.size.width, 20.0);
}

#[test]
fn vertical_linear_default_stretch_does_not_override_max_content_cross_size() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Linear,
        width: Length::points(100.0),
        ..Style::default()
    }));
    let child = tree.push(MeasuringNode::measured(
        Style {
            width: Length::MaxContent,
            ..Style::default()
        },
        Size::new(150.0, 10.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(
        &mut tree,
        root,
        Constraints::new(SideConstraint::indefinite(), SideConstraint::indefinite()),
    );

    let constraints = tree.nodes[child].last_constraints.unwrap();
    assert_eq!(constraints.width.mode, MeasureMode::Indefinite);
    assert_close(tree.nodes[child].layout.size.width, 150.0);
}

#[test]
fn vertical_reverse_linear_positions_items_from_bottom_edge() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::VerticalReverse,
        width: Length::points(20.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let first = fixed_linear_child(&mut tree, Length::Auto, Length::points(10.0));
    let second = fixed_linear_child(&mut tree, Length::Auto, Length::points(20.0));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(20.0, 100.0));

    assert_close(tree.nodes[first].layout.offset.y, 90.0);
    assert_close(tree.nodes[second].layout.offset.y, 70.0);
    assert_close(tree.nodes[first].layout.size.width, 20.0);
    assert_close(tree.nodes[second].layout.size.width, 20.0);
}

#[test]
fn vertical_reverse_linear_gravity_top_packs_items_at_top_edge() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::VerticalReverse,
        linear_gravity: LinearGravity::Top,
        width: Length::points(20.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let first = fixed_linear_child(&mut tree, Length::Auto, Length::points(10.0));
    let second = fixed_linear_child(&mut tree, Length::Auto, Length::points(20.0));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(20.0, 100.0));

    assert_close(tree.nodes[first].layout.offset.y, 20.0);
    assert_close(tree.nodes[second].layout.offset.y, 0.0);
}

#[test]
fn vertical_linear_space_between_distributes_remaining_main_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        justify_content: JustifyContent::SpaceBetween,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let first = fixed_linear_child(&mut tree, Length::Auto, Length::points(10.0));
    let second = fixed_linear_child(&mut tree, Length::Auto, Length::points(10.0));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

    assert_close(tree.nodes[first].layout.offset.y, 0.0);
    assert_close(tree.nodes[second].layout.offset.y, 90.0);
}

#[test]
fn vertical_linear_space_between_single_item_uses_start_position() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        justify_content: JustifyContent::SpaceBetween,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let child = fixed_linear_child(&mut tree, Length::Auto, Length::points(10.0));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

    assert_close(tree.nodes[child].layout.offset.y, 0.0);
}

#[test]
fn vertical_linear_space_between_keeps_items_adjacent_when_overflowing() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        justify_content: JustifyContent::SpaceBetween,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let first = fixed_linear_child(&mut tree, Length::Auto, Length::points(70.0));
    let second = fixed_linear_child(&mut tree, Length::Auto, Length::points(70.0));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

    assert_close(tree.nodes[first].layout.offset.y, 0.0);
    assert_close(tree.nodes[second].layout.offset.y, 70.0);
}

#[test]
fn horizontal_linear_gravity_right_overrides_justify_content() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Right,
        justify_content: JustifyContent::FlexStart,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let first = fixed_linear_child(&mut tree, Length::points(10.0), Length::Auto);
    let second = fixed_linear_child(&mut tree, Length::points(20.0), Length::Auto);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[first].layout.offset.x, 70.0);
    assert_close(tree.nodes[second].layout.offset.x, 80.0);
}

#[test]
fn horizontal_linear_justify_content_flex_end_maps_to_linear_end() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        justify_content: JustifyContent::FlexEnd,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let first = fixed_linear_child(&mut tree, Length::points(10.0), Length::Auto);
    let second = fixed_linear_child(&mut tree, Length::points(20.0), Length::Auto);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[first].layout.offset.x, 70.0);
    assert_close(tree.nodes[second].layout.offset.x, 80.0);
}

#[test]
fn horizontal_linear_justify_content_distribution_values_map_to_start() {
    for justify_content in [
        JustifyContent::SpaceAround,
        JustifyContent::SpaceEvenly,
        JustifyContent::Stretch,
    ] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Linear,
            linear_orientation: LinearOrientation::Horizontal,
            justify_content,
            width: Length::points(100.0),
            height: Length::points(10.0),
            ..Style::default()
        }));
        let first = fixed_linear_child(&mut tree, Length::points(10.0), Length::Auto);
        let second = fixed_linear_child(&mut tree, Length::points(20.0), Length::Auto);
        tree.append_child(root, first);
        tree.append_child(root, second);

        run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

        assert_close(tree.nodes[first].layout.offset.x, 0.0);
        assert_close(tree.nodes[second].layout.offset.x, 10.0);
    }
}

#[test]
fn rtl_horizontal_linear_gravity_uses_rtl_main_front() {
    let cases = [
        (LinearGravity::Left, 20.0, 0.0),
        (LinearGravity::Right, 90.0, 70.0),
    ];

    for (gravity, first_x, second_x) in cases {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Linear,
            direction: Direction::Rtl,
            linear_orientation: LinearOrientation::Horizontal,
            linear_gravity: gravity,
            width: Length::points(100.0),
            height: Length::points(20.0),
            ..Style::default()
        }));
        let first = fixed_linear_child(&mut tree, Length::points(10.0), Length::Auto);
        let second = fixed_linear_child(&mut tree, Length::points(20.0), Length::Auto);
        tree.append_child(root, first);
        tree.append_child(root, second);

        run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

        assert_close(tree.nodes[first].layout.offset.x, first_x);
        assert_close(tree.nodes[second].layout.offset.x, second_x);
    }
}

#[test]
fn vertical_linear_gravity_bottom_packs_items_at_end() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_gravity: LinearGravity::Bottom,
        width: Length::points(20.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let first = fixed_linear_child(&mut tree, Length::Auto, Length::points(10.0));
    let second = fixed_linear_child(&mut tree, Length::Auto, Length::points(10.0));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(20.0, 100.0));

    assert_close(tree.nodes[first].layout.offset.y, 80.0);
    assert_close(tree.nodes[second].layout.offset.y, 90.0);
}

#[test]
fn linear_cross_axis_alignment_uses_align_items() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        align_items: AlignItems::Center,
        width: Length::points(100.0),
        ..Style::default()
    }));
    let child = fixed_linear_child(&mut tree, Length::points(20.0), Length::points(10.0));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

    assert_close(tree.nodes[child].layout.offset.x, 40.0);
}

#[test]
fn linear_cross_axis_center_uses_negative_space_when_item_overflows() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        align_items: AlignItems::Center,
        width: Length::points(100.0),
        ..Style::default()
    }));
    let child = fixed_linear_child(&mut tree, Length::points(140.0), Length::points(10.0));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

    assert_close(tree.nodes[child].layout.offset.x, -20.0);
}

#[test]
fn linear_cross_axis_end_uses_negative_space_when_item_overflows() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        align_items: AlignItems::FlexEnd,
        width: Length::points(100.0),
        ..Style::default()
    }));
    let child = fixed_linear_child(&mut tree, Length::points(140.0), Length::points(10.0));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

    assert_close(tree.nodes[child].layout.offset.x, -40.0);
}

#[test]
fn linear_baseline_align_items_keeps_default_cross_axis_stretch() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        align_items: AlignItems::Baseline,
        width: Length::points(100.0),
        ..Style::default()
    }));
    let child = fixed_linear_child(&mut tree, Length::Auto, Length::points(10.0));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

    assert_close(tree.nodes[child].layout.offset.x, 0.0);
    assert_close(tree.nodes[child].layout.size.width, 100.0);
}

#[test]
fn linear_align_self_overrides_container_align_items() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        align_self: Some(AlignItems::FlexEnd),
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

    assert_close(tree.nodes[child].layout.offset.x, 80.0);
}

#[test]
fn linear_align_self_overrides_linear_cross_gravity() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        align_items: AlignItems::FlexStart,
        linear_cross_gravity: LinearCrossGravity::End,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        align_self: Some(AlignItems::Center),
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

    assert_close(tree.nodes[child].layout.offset.x, 40.0);
    assert_close(tree.nodes[child].layout.size.width, 20.0);
}

#[test]
fn linear_layout_gravity_end_overrides_container_stretch() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        width: Length::points(100.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        linear_layout_gravity: LinearLayoutGravity::End,
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

    assert_close(tree.nodes[child].layout.offset.x, 80.0);
    assert_close(tree.nodes[child].layout.size.width, 20.0);
}

#[test]
fn linear_layout_gravity_overrides_align_self_and_cross_gravity() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        align_items: AlignItems::FlexStart,
        linear_cross_gravity: LinearCrossGravity::End,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        align_self: Some(AlignItems::FlexEnd),
        linear_layout_gravity: LinearLayoutGravity::Left,
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

    assert_close(tree.nodes[child].layout.offset.x, 0.0);
    assert_close(tree.nodes[child].layout.size.width, 20.0);
}

#[test]
fn linear_align_items_stretch_is_not_used_as_linear_layout_gravity_fallback() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        align_items: AlignItems::Stretch,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

    assert_close(tree.nodes[child].layout.offset.x, 0.0);
    assert_close(tree.nodes[child].layout.size.width, 20.0);
}

#[test]
fn linear_layout_gravity_stretch_overrides_explicit_cross_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        width: Length::points(100.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        linear_layout_gravity: LinearLayoutGravity::Stretch,
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

    assert_close(tree.nodes[child].layout.size.width, 100.0);
    assert_close(tree.nodes[child].layout.size.height, 10.0);
}

#[test]
fn linear_layout_gravity_stretch_overrides_weighted_explicit_cross_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        width: Length::points(20.0),
        linear_weight: 1.0,
        linear_layout_gravity: LinearLayoutGravity::Stretch,
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

    assert_close(tree.nodes[child].layout.size.width, 100.0);
    assert_close(tree.nodes[child].layout.size.height, 100.0);
}

#[test]
fn linear_layout_gravity_physical_variants_match_cpp_groups() {
    let cases = [
        (LinearLayoutGravity::None, 0.0, 20.0),
        (LinearLayoutGravity::Top, 0.0, 20.0),
        (LinearLayoutGravity::Left, 0.0, 20.0),
        (LinearLayoutGravity::Start, 0.0, 20.0),
        (LinearLayoutGravity::Right, 80.0, 20.0),
        (LinearLayoutGravity::Bottom, 80.0, 20.0),
        (LinearLayoutGravity::End, 80.0, 20.0),
        (LinearLayoutGravity::CenterHorizontal, 40.0, 20.0),
        (LinearLayoutGravity::CenterVertical, 40.0, 20.0),
        (LinearLayoutGravity::Center, 40.0, 20.0),
        (LinearLayoutGravity::FillHorizontal, 0.0, 100.0),
        (LinearLayoutGravity::FillVertical, 0.0, 100.0),
        (LinearLayoutGravity::Stretch, 0.0, 100.0),
    ];

    for (gravity, expected_x, expected_width) in cases {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Linear,
            width: Length::points(100.0),
            height: Length::points(100.0),
            ..Style::default()
        }));
        let child = tree.push(SimpleNode::new(Style {
            width: Length::points(20.0),
            height: Length::points(10.0),
            linear_layout_gravity: gravity,
            ..Style::default()
        }));
        tree.append_child(root, child);

        run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

        assert_close(tree.nodes[child].layout.offset.x, expected_x);
        assert_close(tree.nodes[child].layout.size.width, expected_width);
        assert_close(tree.nodes[child].layout.size.height, 10.0);
    }
}

#[test]
fn rtl_vertical_linear_layout_gravity_keeps_physical_left_and_right() {
    let cases = [
        (LinearLayoutGravity::Left, 0.0),
        (LinearLayoutGravity::Right, 80.0),
    ];

    for (gravity, expected_x) in cases {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Linear,
            direction: Direction::Rtl,
            width: Length::points(100.0),
            height: Length::points(100.0),
            ..Style::default()
        }));
        let child = tree.push(SimpleNode::new(Style {
            width: Length::points(20.0),
            height: Length::points(10.0),
            linear_layout_gravity: gravity,
            ..Style::default()
        }));
        tree.append_child(root, child);

        run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

        assert_close(tree.nodes[child].layout.offset.x, expected_x);
        assert_close(tree.nodes[child].layout.size.width, 20.0);
        assert_close(tree.nodes[child].layout.size.height, 10.0);
    }
}

#[test]
fn rtl_horizontal_linear_gravity_keeps_physical_left_and_right() {
    let cases = [
        (Direction::Rtl, LinearGravity::Left, 0.0),
        (Direction::Rtl, LinearGravity::Right, 80.0),
    ];

    for (direction, gravity, expected_x) in cases {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Linear,
            direction,
            linear_orientation: LinearOrientation::Horizontal,
            linear_gravity: gravity,
            width: Length::points(100.0),
            height: Length::points(20.0),
            ..Style::default()
        }));
        let child = tree.push(SimpleNode::new(Style {
            width: Length::points(20.0),
            height: Length::points(10.0),
            ..Style::default()
        }));
        tree.append_child(root, child);

        run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

        assert_close(tree.nodes[child].layout.offset.x, expected_x);
        assert_close(tree.nodes[child].layout.size.width, 20.0);
        assert_close(tree.nodes[child].layout.size.height, 10.0);
    }
}

#[test]
fn horizontal_linear_layout_gravity_physical_variants_match_cpp_groups() {
    let cases = [
        (LinearLayoutGravity::None, 0.0, 10.0),
        (LinearLayoutGravity::Top, 0.0, 10.0),
        (LinearLayoutGravity::Left, 0.0, 10.0),
        (LinearLayoutGravity::Start, 0.0, 10.0),
        (LinearLayoutGravity::Right, 90.0, 10.0),
        (LinearLayoutGravity::Bottom, 90.0, 10.0),
        (LinearLayoutGravity::End, 90.0, 10.0),
        (LinearLayoutGravity::CenterHorizontal, 45.0, 10.0),
        (LinearLayoutGravity::CenterVertical, 45.0, 10.0),
        (LinearLayoutGravity::Center, 45.0, 10.0),
        (LinearLayoutGravity::FillHorizontal, 0.0, 100.0),
        (LinearLayoutGravity::FillVertical, 0.0, 100.0),
        (LinearLayoutGravity::Stretch, 0.0, 100.0),
    ];

    for (gravity, expected_y, expected_height) in cases {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Linear,
            linear_orientation: LinearOrientation::Horizontal,
            width: Length::points(100.0),
            height: Length::points(100.0),
            ..Style::default()
        }));
        let child = tree.push(SimpleNode::new(Style {
            width: Length::points(20.0),
            height: Length::points(10.0),
            linear_layout_gravity: gravity,
            ..Style::default()
        }));
        tree.append_child(root, child);

        run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

        assert_close(tree.nodes[child].layout.offset.y, expected_y);
        assert_close(tree.nodes[child].layout.size.width, 20.0);
        assert_close(tree.nodes[child].layout.size.height, expected_height);
    }
}

#[test]
fn linear_cross_gravity_center_aligns_children() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_cross_gravity: LinearCrossGravity::Center,
        width: Length::points(100.0),
        ..Style::default()
    }));
    let child = fixed_linear_child(&mut tree, Length::points(20.0), Length::points(10.0));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

    assert_close(tree.nodes[child].layout.offset.x, 40.0);
}

#[test]
fn linear_cross_gravity_stretch_overrides_flex_start_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        align_items: AlignItems::FlexStart,
        linear_cross_gravity: LinearCrossGravity::Stretch,
        width: Length::points(100.0),
        ..Style::default()
    }));
    let child = fixed_linear_child(&mut tree, Length::Auto, Length::points(10.0));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

    assert_close(tree.nodes[child].layout.size.width, 100.0);
}

#[test]
fn linear_cross_axis_auto_margins_override_cross_gravity_and_export_used_margins() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_cross_gravity: LinearCrossGravity::End,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        margin: Rect::new(Length::Auto, Length::Auto, Length::ZERO, Length::ZERO),
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

    assert_close(tree.nodes[child].layout.offset.x, 40.0);
    assert_close(tree.nodes[child].layout.size.width, 20.0);
    assert_close(tree.nodes[child].layout.margin.left, 40.0);
    assert_close(tree.nodes[child].layout.margin.right, 40.0);
}

#[test]
fn horizontal_linear_cross_axis_start_auto_margin_pushes_item_to_end() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        margin: Rect::new(Length::ZERO, Length::ZERO, Length::Auto, Length::ZERO),
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

    assert_close(tree.nodes[child].layout.offset.y, 90.0);
    assert_close(tree.nodes[child].layout.size.height, 10.0);
    assert_close(tree.nodes[child].layout.margin.top, 90.0);
    assert_close(tree.nodes[child].layout.margin.bottom, 0.0);
}

#[test]
fn horizontal_linear_cross_axis_end_auto_margin_keeps_item_at_start() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        margin: Rect::new(Length::ZERO, Length::ZERO, Length::ZERO, Length::Auto),
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

    assert_close(tree.nodes[child].layout.offset.y, 0.0);
    assert_close(tree.nodes[child].layout.size.height, 10.0);
    assert_close(tree.nodes[child].layout.margin.top, 0.0);
    assert_close(tree.nodes[child].layout.margin.bottom, 90.0);
}

#[test]
fn horizontal_linear_overflowing_cross_axis_auto_margins_are_ignored() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(20.0),
        height: Length::points(140.0),
        margin: Rect::new(Length::ZERO, Length::ZERO, Length::Auto, Length::Auto),
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

    assert_close(tree.nodes[child].layout.offset.y, 0.0);
    assert_close(tree.nodes[child].layout.size.height, 140.0);
    assert_close(tree.nodes[child].layout.margin.top, 0.0);
    assert_close(tree.nodes[child].layout.margin.bottom, 0.0);
}

#[test]
fn horizontal_linear_baseline_keeps_unresolved_start_auto_margin() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size_and_baseline(
        Style {
            margin: Rect::new(Length::ZERO, Length::ZERO, Length::Auto, Length::ZERO),
            ..Style::default()
        },
        Size::new(20.0, 10.0),
        4.0,
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

    assert_close(tree.nodes[child].layout.offset.y, 90.0);
    assert_close(tree.nodes[child].layout.margin.top, 90.0);
    assert_close(tree.nodes[root].layout.baseline.unwrap(), 4.0);
}

#[test]
fn horizontal_linear_baseline_uses_gravity_before_paired_auto_margins_resolve() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        linear_cross_gravity: LinearCrossGravity::End,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size_and_baseline(
        Style {
            margin: Rect::new(Length::ZERO, Length::ZERO, Length::Auto, Length::Auto),
            ..Style::default()
        },
        Size::new(20.0, 10.0),
        4.0,
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

    assert_close(tree.nodes[child].layout.offset.y, 45.0);
    assert_close(tree.nodes[child].layout.margin.top, 45.0);
    assert_close(tree.nodes[child].layout.margin.bottom, 45.0);
    assert_close(tree.nodes[root].layout.baseline.unwrap(), 94.0);
}

#[test]
fn vertical_linear_cross_gravity_variants_override_align_items() {
    let cases = [
        (LinearCrossGravity::Start, 0.0, 20.0),
        (LinearCrossGravity::End, 80.0, 20.0),
        (LinearCrossGravity::Center, 40.0, 20.0),
        (LinearCrossGravity::Stretch, 0.0, 100.0),
    ];

    for (cross_gravity, expected_x, expected_width) in cases {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Linear,
            align_items: AlignItems::FlexStart,
            linear_cross_gravity: cross_gravity,
            width: Length::points(100.0),
            height: Length::points(100.0),
            ..Style::default()
        }));
        let child = fixed_linear_child(&mut tree, Length::points(20.0), Length::points(10.0));
        tree.append_child(root, child);

        run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

        assert_close(tree.nodes[child].layout.offset.x, expected_x);
        assert_close(tree.nodes[child].layout.size.width, expected_width);
        assert_close(tree.nodes[child].layout.size.height, 10.0);
    }
}

#[test]
fn horizontal_linear_cross_gravity_variants_override_align_items() {
    let cases = [
        (LinearCrossGravity::Start, 0.0, 10.0),
        (LinearCrossGravity::End, 90.0, 10.0),
        (LinearCrossGravity::Center, 45.0, 10.0),
        (LinearCrossGravity::Stretch, 0.0, 100.0),
    ];

    for (cross_gravity, expected_y, expected_height) in cases {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Linear,
            linear_orientation: LinearOrientation::Horizontal,
            align_items: AlignItems::FlexStart,
            linear_cross_gravity: cross_gravity,
            width: Length::points(100.0),
            height: Length::points(100.0),
            ..Style::default()
        }));
        let child = fixed_linear_child(&mut tree, Length::points(20.0), Length::points(10.0));
        tree.append_child(root, child);

        run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

        assert_close(tree.nodes[child].layout.offset.y, expected_y);
        assert_close(tree.nodes[child].layout.size.width, 20.0);
        assert_close(tree.nodes[child].layout.size.height, expected_height);
    }
}

#[test]
fn vertical_linear_weight_takes_remaining_main_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let fixed = fixed_linear_child(&mut tree, Length::Auto, Length::points(10.0));
    let weighted = tree.push(SimpleNode::new(Style {
        linear_weight: 1.0,
        ..Style::default()
    }));
    tree.append_child(root, fixed);
    tree.append_child(root, weighted);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

    assert_close(tree.nodes[fixed].layout.size.height, 10.0);
    assert_close(tree.nodes[weighted].layout.offset.y, 10.0);
    assert_close(tree.nodes[weighted].layout.size.height, 90.0);
    assert_close(tree.nodes[weighted].layout.size.width, 100.0);
}

#[test]
fn vertical_linear_weight_gets_zero_when_main_space_is_exhausted() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let fixed = fixed_linear_child(&mut tree, Length::points(20.0), Length::points(30.0));
    let weighted = tree.push(SimpleNode::new(Style {
        width: Length::points(20.0),
        height: Length::Auto,
        linear_weight: 1.0,
        ..Style::default()
    }));
    tree.append_child(root, fixed);
    tree.append_child(root, weighted);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[fixed].layout.size.height, 30.0);
    assert_close(tree.nodes[weighted].layout.offset.y, 30.0);
    assert_close(tree.nodes[weighted].layout.size.height, 0.0);
}

#[test]
fn horizontal_linear_weight_sub_epsilon_min_violations_do_not_freeze_items() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(Style {
            display: Display::Block,
            linear_weight: 1.0,
            min_width: Length::points(50.00006),
            height: Length::points(10.0),
            ..Style::default()
        }));
        tree.append_child(root, child);
    }

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[1].layout.size.width, 50.00006);
    assert_close(tree.nodes[2].layout.size.width, 50.00006);
}

#[test]
fn horizontal_linear_weights_split_remaining_main_space_by_ratio() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(90.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        linear_weight: 1.0,
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        linear_weight: 2.0,
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(90.0, 20.0));

    assert_close(tree.nodes[first].layout.size.width, 30.0);
    assert_close(tree.nodes[second].layout.size.width, 60.0);
    assert_close(tree.nodes[second].layout.offset.x, 30.0);
}

#[test]
fn linear_weight_sum_can_leave_unallocated_main_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        linear_weight_sum: 4.0,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        linear_weight: 1.0,
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        linear_weight: 1.0,
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[first].layout.size.width, 25.0);
    assert_close(tree.nodes[second].layout.size.width, 25.0);
    assert_close(tree.nodes[second].layout.offset.x, 25.0);
}

#[test]
fn linear_weight_max_size_freezes_and_redistributes_remaining_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        linear_weight: 1.0,
        max_width: Length::points(30.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        linear_weight: 1.0,
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[first].layout.size.width, 30.0);
    assert_close(tree.nodes[second].layout.size.width, 70.0);
    assert_close(tree.nodes[second].layout.offset.x, 30.0);
}

#[test]
fn linear_weight_percent_max_size_freezes_and_redistributes_remaining_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        linear_weight: 1.0,
        max_width: Length::percent(30.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        linear_weight: 1.0,
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[first].layout.size.width, 30.0);
    assert_close(tree.nodes[second].layout.size.width, 70.0);
    assert_close(tree.nodes[second].layout.offset.x, 30.0);
}

#[test]
fn linear_weight_min_size_freezes_and_redistributes_remaining_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        linear_weight: 1.0,
        min_width: Length::points(70.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        linear_weight: 1.0,
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[first].layout.size.width, 70.0);
    assert_close(tree.nodes[second].layout.size.width, 30.0);
    assert_close(tree.nodes[second].layout.offset.x, 70.0);
}

#[test]
fn linear_weight_all_items_freeze_after_min_violations() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        linear_weight: 1.0,
        min_width: Length::points(60.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        linear_weight: 1.0,
        min_width: Length::points(60.0),
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[first].layout.size.width, 60.0);
    assert_close(tree.nodes[second].layout.size.width, 60.0);
    assert_close(tree.nodes[second].layout.offset.x, 60.0);
}

#[test]
fn linear_weight_percent_min_size_freezes_and_redistributes_remaining_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        linear_weight: 1.0,
        min_width: Length::percent(70.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        linear_weight: 1.0,
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[first].layout.size.width, 70.0);
    assert_close(tree.nodes[second].layout.size.width, 30.0);
    assert_close(tree.nodes[second].layout.offset.x, 70.0);
}

#[test]
fn vertical_linear_weight_percent_max_size_freezes_and_redistributes_remaining_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        width: Length::points(20.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        linear_weight: 1.0,
        max_height: Length::percent(30.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        linear_weight: 1.0,
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(20.0, 100.0));

    assert_close(tree.nodes[first].layout.size.height, 30.0);
    assert_close(tree.nodes[second].layout.size.height, 70.0);
    assert_close(tree.nodes[second].layout.offset.y, 30.0);
}

#[test]
fn vertical_linear_weight_percent_min_size_freezes_and_redistributes_remaining_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        width: Length::points(20.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        linear_weight: 1.0,
        min_height: Length::percent(70.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        linear_weight: 1.0,
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(20.0, 100.0));

    assert_close(tree.nodes[first].layout.size.height, 70.0);
    assert_close(tree.nodes[second].layout.size.height, 30.0);
    assert_close(tree.nodes[second].layout.offset.y, 70.0);
}

#[test]
fn linear_total_weight_below_one_leaves_unallocated_main_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        linear_weight: 0.5,
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[child].layout.size.width, 50.0);
}

#[test]
fn linear_layout_orders_in_flow_children_by_order() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        ..Style::default()
    }));
    let later = fixed_linear_child(&mut tree, Length::points(10.0), Length::points(10.0));
    tree.nodes[later].style.order = 1;
    let earlier = fixed_linear_child(&mut tree, Length::points(10.0), Length::points(10.0));
    tree.nodes[earlier].style.order = -1;
    tree.append_child(root, later);
    tree.append_child(root, earlier);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.height, 20.0);
    assert_close(tree.nodes[earlier].layout.offset.y, 0.0);
    assert_close(tree.nodes[later].layout.offset.y, 10.0);
}

fn fixed_linear_child(tree: &mut SimpleTree, width: Length, height: Length) -> usize {
    tree.push(SimpleNode::new(Style {
        width,
        height,
        ..Style::default()
    }))
}
