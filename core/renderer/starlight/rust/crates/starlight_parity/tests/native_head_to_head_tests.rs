// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use starlight_cpp::{BaselineLayoutTree, CppBaselineError};
use starlight_layout::{
    AlignContent, AlignItems, BaseLength, BoxSizing, Constraints, Direction, Display,
    FlexDirection, FlexWrap, GridAutoFlow, JustifyContent, JustifyItems, LayoutResult, LayoutTree,
    Length, LinearCrossGravity, LinearGravity, LinearLayoutGravity, LinearOrientation, MeasureMode,
    PositionType, Rect, RelativeCenter, SideConstraint, SimpleNode, SimpleTree, Size, Style,
    Visibility, RELATIVE_ALIGN_PARENT,
};
use starlight_parity::{run_head_to_head, LayoutTolerance, ParityError};

#[test]
fn head_to_head_flex_grow_and_order() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(120.0),
        height: Length::points(20.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        order: 2,
        ..Style::default()
    })));
    let second = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 3.0,
        height: Length::points(10.0),
        order: 1,
        ..Style::default()
    })));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(120.0, 20.0));
}

#[test]
fn head_to_head_flex_growing_target_defines_percent_basis_and_main_size_child_base() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(92.0),
        height: Length::points(24.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        flex_shrink: 0.0,
        width: Length::points(20.0),
        height: Length::points(18.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let percent = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(50.0),
        width: Length::percent(40.0),
        height: Length::points(6.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(7.0),
        width: Length::points(7.0),
        height: Length::points(5.0),
        ..Style::default()
    })));
    let sibling = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(15.0),
        width: Length::points(15.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(root, sibling);
    tree.append_child(child, percent);
    tree.append_child(child, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(92.0, 24.0));
}

#[test]
fn head_to_head_flex_growing_percent_main_target_defines_child_main_size_base() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(91.0),
        height: Length::points(22.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        flex_shrink: 0.0,
        width: Length::percent(35.0),
        height: Length::points(16.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let percent = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(8.0),
        width: Length::percent(50.0),
        height: Length::points(6.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(6.0),
        width: Length::points(6.0),
        height: Length::points(5.0),
        ..Style::default()
    })));
    let sibling = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(14.0),
        width: Length::points(14.0),
        height: Length::points(9.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(root, sibling);
    tree.append_child(child, percent);
    tree.append_child(child, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(91.0, 22.0));
}

#[test]
fn head_to_head_flex_percent_main_length_parent_defines_growing_percent_basis_child_base() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(90.0),
        height: Length::points(22.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        flex_basis: Length::Auto,
        flex_grow: 0.0,
        flex_shrink: 1.0,
        width: Length::percent(50.0),
        height: Length::points(16.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let percent = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(50.0),
        flex_grow: 1.0,
        flex_shrink: 0.0,
        height: Length::points(6.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(5.0),
        width: Length::points(5.0),
        height: Length::points(5.0),
        ..Style::default()
    })));
    let sibling = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_shrink: 0.0,
        width: Length::points(20.0),
        height: Length::points(9.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(root, sibling);
    tree.append_child(child, percent);
    tree.append_child(child, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(90.0, 22.0));
}

#[test]
fn head_to_head_flex_point_basis_parent_defines_growing_percent_basis_child_base() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(76.0),
        height: Length::points(22.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        flex_basis: Length::points(36.0),
        flex_grow: 0.0,
        flex_shrink: 1.0,
        height: Length::points(16.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let percent = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(50.0),
        flex_grow: 1.0,
        flex_shrink: 0.0,
        height: Length::points(6.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(5.0),
        width: Length::points(5.0),
        height: Length::points(5.0),
        ..Style::default()
    })));
    let sibling = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(18.0),
        flex_shrink: 0.0,
        width: Length::points(18.0),
        height: Length::points(9.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(root, sibling);
    tree.append_child(child, percent);
    tree.append_child(child, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(76.0, 22.0));
}

#[test]
fn head_to_head_flex_own_percent_basis_and_main_size_define_percent_child_bases() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(24.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        flex_basis: Length::percent(50.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        width: Length::percent(40.0),
        height: Length::points(16.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let percent = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(50.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        width: Length::percent(40.0),
        height: Length::points(6.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(5.0),
        width: Length::points(5.0),
        height: Length::points(5.0),
        ..Style::default()
    })));
    let sibling = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(30.0),
        flex_shrink: 0.0,
        width: Length::points(30.0),
        height: Length::points(9.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(root, sibling);
    tree.append_child(child, percent);
    tree.append_child(child, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 24.0));
}

#[test]
fn head_to_head_flex_oversized_inflexible_fixed_child_with_percent_basis_sibling() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(24.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        flex_basis: Length::percent(40.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        width: Length::percent(40.0),
        height: Length::points(16.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let oversized = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(60.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        width: Length::points(60.0),
        height: Length::points(6.0),
        ..Style::default()
    })));
    let percent = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(50.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        height: Length::points(5.0),
        ..Style::default()
    })));
    let sibling = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(30.0),
        flex_shrink: 0.0,
        width: Length::points(30.0),
        height: Length::points(9.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(root, sibling);
    tree.append_child(child, oversized);
    tree.append_child(child, percent);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 24.0));
}

#[test]
fn head_to_head_flex_shrinking_target_defines_percent_main_size_child_base() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(42.0),
        height: Length::points(22.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        flex_basis: Length::points(36.0),
        flex_grow: 0.0,
        flex_shrink: 1.0,
        width: Length::points(36.0),
        height: Length::points(16.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let percent = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(8.0),
        width: Length::percent(50.0),
        height: Length::points(6.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(6.0),
        width: Length::points(6.0),
        height: Length::points(5.0),
        ..Style::default()
    })));
    let sibling = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(18.0),
        flex_shrink: 0.0,
        width: Length::points(18.0),
        height: Length::points(9.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(root, sibling);
    tree.append_child(child, percent);
    tree.append_child(child, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(42.0, 22.0));
}

#[test]
fn head_to_head_flex_shrunk_parent_target_defines_percent_basis_child_base() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(45.0),
        height: Length::points(22.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        flex_basis: Length::points(42.0),
        flex_grow: 0.0,
        flex_shrink: 1.0,
        width: Length::points(42.0),
        height: Length::points(16.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let percent = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(50.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        height: Length::points(6.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(5.0),
        width: Length::points(5.0),
        height: Length::points(5.0),
        ..Style::default()
    })));
    let sibling = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(28.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        width: Length::points(28.0),
        height: Length::points(9.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(root, sibling);
    tree.append_child(child, percent);
    tree.append_child(child, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(45.0, 22.0));
}

#[test]
fn head_to_head_flex_unchanged_main_defines_stretch_percent_basis_child_base() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(70.0),
        height: Length::points(22.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        flex_basis: Length::points(40.0),
        flex_grow: 0.0,
        flex_shrink: 1.0,
        width: Length::points(40.0),
        height: Length::points(16.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let stretched = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(50.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        align_self: Some(AlignItems::Stretch),
        height: Length::points(6.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(5.0),
        width: Length::points(5.0),
        height: Length::points(5.0),
        ..Style::default()
    })));
    let sibling = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(30.0),
        flex_shrink: 0.0,
        width: Length::points(30.0),
        height: Length::points(9.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(root, sibling);
    tree.append_child(child, stretched);
    tree.append_child(child, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(70.0, 22.0));
}

#[test]
fn head_to_head_flex_unchanged_main_defines_inflexible_percent_basis_child_base() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(69.0),
        height: Length::points(22.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        flex_basis: Length::points(39.0),
        flex_grow: 0.0,
        flex_shrink: 1.0,
        width: Length::points(39.0),
        height: Length::points(16.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let percent = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(50.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        height: Length::points(6.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(5.0),
        width: Length::points(5.0),
        height: Length::points(5.0),
        ..Style::default()
    })));
    let sibling = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(30.0),
        flex_shrink: 0.0,
        width: Length::points(30.0),
        height: Length::points(9.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(root, sibling);
    tree.append_child(child, percent);
    tree.append_child(child, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(69.0, 22.0));
}

#[test]
fn head_to_head_flex_preserved_percent_basis_parent_defines_growing_percent_child_base() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        height: Length::points(22.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        flex_basis: Length::percent(50.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        height: Length::points(16.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let percent = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(50.0),
        flex_grow: 1.0,
        flex_shrink: 0.0,
        height: Length::points(6.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(5.0),
        width: Length::points(5.0),
        height: Length::points(5.0),
        ..Style::default()
    })));
    let sibling = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(12.0),
        flex_shrink: 0.0,
        width: Length::points(12.0),
        height: Length::points(9.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(root, sibling);
    tree.append_child(child, percent);
    tree.append_child(child, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_flex_preserved_percent_basis_parent_defines_inflexible_percent_basis_and_main_size_child_base(
) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        height: Length::points(22.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        flex_basis: Length::percent(50.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        height: Length::points(16.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let percent = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(50.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        width: Length::percent(40.0),
        height: Length::points(6.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(5.0),
        width: Length::points(5.0),
        height: Length::points(5.0),
        ..Style::default()
    })));
    let sibling = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(12.0),
        flex_shrink: 0.0,
        width: Length::points(12.0),
        height: Length::points(9.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(root, sibling);
    tree.append_child(child, percent);
    tree.append_child(child, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_flex_aligned_growing_percent_basis_target_defines_child_basis_base() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(96.0),
        height: Length::points(24.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        flex_basis: Length::percent(30.0),
        flex_grow: 1.0,
        flex_shrink: 0.0,
        width: Length::points(20.0),
        height: Length::points(18.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let aligned = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(45.0),
        flex_grow: 1.0,
        flex_shrink: 0.0,
        align_self: Some(AlignItems::Center),
        height: Length::points(7.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(6.0),
        width: Length::points(6.0),
        height: Length::points(5.0),
        ..Style::default()
    })));
    let sibling = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(18.0),
        width: Length::points(18.0),
        height: Length::points(9.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(root, sibling);
    tree.append_child(child, aligned);
    tree.append_child(child, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(96.0, 24.0));
}

#[test]
fn head_to_head_flex_growing_percent_basis_target_defines_local_flexible_child_basis_base() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(94.0),
        height: Length::points(23.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        flex_basis: Length::percent(25.0),
        flex_grow: 1.0,
        flex_shrink: 0.0,
        width: Length::points(18.0),
        height: Length::points(17.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let percent = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(40.0),
        flex_grow: 1.0,
        flex_shrink: 0.0,
        height: Length::points(6.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(5.0),
        width: Length::points(5.0),
        height: Length::points(5.0),
        ..Style::default()
    })));
    let sibling = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(17.0),
        width: Length::points(17.0),
        height: Length::points(9.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(root, sibling);
    tree.append_child(child, percent);
    tree.append_child(child, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(94.0, 23.0));
}

#[test]
fn head_to_head_flex_growing_percent_basis_target_defines_local_inflexible_child_basis_base() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(93.0),
        height: Length::points(23.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        flex_basis: Length::percent(25.0),
        flex_grow: 1.0,
        flex_shrink: 0.0,
        width: Length::points(18.0),
        height: Length::points(17.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let percent = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(42.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        height: Length::points(6.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(5.0),
        width: Length::points(5.0),
        height: Length::points(5.0),
        ..Style::default()
    })));
    let sibling = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(16.0),
        width: Length::points(16.0),
        height: Length::points(9.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(root, sibling);
    tree.append_child(child, percent);
    tree.append_child(child, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(93.0, 23.0));
}

#[test]
fn head_to_head_flex_auto_main_preserves_intrinsic_percent_basis_child() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(82.0),
        height: Length::points(22.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        flex_basis: Length::Auto,
        flex_grow: 0.0,
        flex_shrink: 0.0,
        width: Length::Auto,
        height: Length::points(14.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let percent = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(65.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        width: Length::points(18.0),
        height: Length::points(7.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(6.0),
        width: Length::points(6.0),
        height: Length::points(5.0),
        ..Style::default()
    })));
    let sibling = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(11.0),
        width: Length::points(11.0),
        height: Length::points(9.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(root, sibling);
    tree.append_child(child, percent);
    tree.append_child(child, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(82.0, 22.0));
}

#[test]
fn head_to_head_wrap_gaps_and_align_content() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(50.0),
        height: Length::points(40.0),
        flex_wrap: FlexWrap::Wrap,
        row_gap: Length::points(2.0),
        column_gap: Length::points(1.0),
        align_items: AlignItems::FlexStart,
        align_content: AlignContent::Center,
        justify_content: JustifyContent::FlexStart,
        ..Style::default()
    })));

    for width in [18.0, 20.0, 22.0, 24.0] {
        let child = tree.push(SimpleNode::new(standalone_style(Style {
            flex_basis: Length::points(width),
            height: Length::points(8.0),
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 40.0));
}

#[test]
fn head_to_head_flex_justify_content_space_evenly() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        justify_content: JustifyContent::SpaceEvenly,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style {
            flex_basis: Length::points(10.0),
            height: Length::points(10.0),
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(110.0, 10.0));
}

#[test]
fn head_to_head_justify_content_space_evenly_single_item_equal_edge_spaces() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        justify_content: JustifyContent::SpaceEvenly,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let child = fixed_flex_child(&mut tree, 20.0, 10.0);
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_justify_content_space_between_single_item_fallback() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        justify_content: JustifyContent::SpaceBetween,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let child = fixed_flex_child(&mut tree, 20.0, 10.0);
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_justify_content_space_around_single_item_fallback() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        justify_content: JustifyContent::SpaceAround,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let child = fixed_flex_child(&mut tree, 20.0, 10.0);
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_main_axis_auto_margin_without_positive_free_space_zeroes_margins_then_justify_content(
) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        justify_content: JustifyContent::Center,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(30.0),
        flex_shrink: 0.0,
        height: Length::points(10.0),
        margin: Rect::new(Length::Auto, Length::ZERO, Length::ZERO, Length::ZERO),
        ..Style::default()
    })));
    let second = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(30.0),
        flex_shrink: 0.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 10.0));
}

#[test]
fn head_to_head_single_cross_axis_auto_margins_absorb_positive_free_space() {
    for (name, margin) in [
        (
            "cross-axis start auto margin",
            Rect::new(Length::ZERO, Length::ZERO, Length::Auto, Length::ZERO),
        ),
        (
            "cross-axis end auto margin",
            Rect::new(Length::ZERO, Length::ZERO, Length::ZERO, Length::Auto),
        ),
    ] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(standalone_style(Style {
            align_items: AlignItems::Center,
            width: Length::points(50.0),
            height: Length::points(40.0),
            ..Style::default()
        })));
        let child = tree.push(SimpleNode::new(standalone_style(Style {
            flex_basis: Length::points(10.0),
            height: Length::points(10.0),
            margin,
            ..Style::default()
        })));
        tree.append_child(root, child);

        assert_head_to_head_or_skip_with_name(name, tree, root, Constraints::definite(50.0, 40.0));
    }
}

#[test]
fn head_to_head_flex_column_container_baseline_uses_first_item_baseline_after_main_axis_alignment()
{
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Column,
        justify_content: JustifyContent::Center,
        align_items: AlignItems::FlexStart,
        width: Length::points(40.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size_and_baseline(
        standalone_style(Style::default()),
        Size::new(20.0, 20.0),
        6.0,
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(40.0, 100.0));
}

#[test]
fn head_to_head_root_at_most_shrink_wraps_flex_line() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    for width in [30.0, 20.0] {
        let child = tree.push(SimpleNode::new(standalone_style(Style {
            flex_basis: Length::points(width),
            height: Length::points(10.0),
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(SideConstraint::at_most(100.0), SideConstraint::indefinite()),
    );
}

#[test]
fn head_to_head_wrapped_root_at_most_shrink_wraps_largest_flex_line() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_wrap: FlexWrap::Wrap,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    for _ in 0..3 {
        let child = fixed_flex_child(&mut tree, 40.0, 10.0);
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(SideConstraint::at_most(100.0), SideConstraint::indefinite()),
    );
}

#[test]
fn head_to_head_display_none_child_is_laid_out_as_zero_and_skipped_by_flex() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(10.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let first = fixed_flex_child(&mut tree, 20.0, 10.0);
    let hidden = tree.push(SimpleNode::new(Style {
        display: Display::None,
        box_sizing: BoxSizing::ContentBox,
        flex_basis: Length::points(50.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let second = fixed_flex_child(&mut tree, 20.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, hidden);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_absolute_flex_child_uses_static_position_without_participating_in_flex_layout() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        justify_content: JustifyContent::Center,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let first = fixed_flex_child(&mut tree, 20.0, 10.0);
    let absolute = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Absolute,
        width: Length::points(10.0),
        height: Length::points(10.0),
        align_self: Some(AlignItems::FlexEnd),
        margin: Rect::all(Length::Auto),
        ..Style::default()
    })));
    let second = fixed_flex_child(&mut tree, 20.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, absolute);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_flex_stretch_reexports_cached_block_subtree() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::Stretch,
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style::default())));
    let grandchild = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(10.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, grandchild);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(30.0, 20.0));
}

#[test]
fn head_to_head_flex_stretch_reexports_cached_block_subtree_with_fractional_offsets() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::Stretch,
        padding: Rect::all(Length::points(0.4)),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style::default())));
    let grandchild = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(10.4),
        height: Length::points(5.4),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, grandchild);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(30.4, 20.4));
}

#[test]
fn head_to_head_orthogonal_flex_reuses_percent_basis_subtree_measure() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(80.0),
        height: Length::points(40.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Column,
        flex_shrink: 0.0,
        width: Length::points(20.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    let grandchild = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(50.0),
        flex_grow: 1.0,
        flex_shrink: 0.0,
        width: Length::points(8.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, grandchild);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(80.0, 40.0));
}

#[test]
fn head_to_head_flex_stretch_remeasures_aligned_inflexible_percent_basis_subtree() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::Stretch,
        width: Length::points(50.0),
        height: Length::points(24.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Column,
        flex_shrink: 1.0,
        flex_basis: Length::points(18.0),
        width: Length::points(18.0),
        ..Style::default()
    })));
    let grandchild = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(80.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        align_self: Some(AlignItems::Center),
        height: Length::points(12.0),
        ..Style::default()
    })));
    let leaf = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(7.0),
        height: Length::points(5.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, grandchild);
    tree.append_child(grandchild, leaf);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 24.0));
}

#[test]
fn head_to_head_flex_explicit_stretch_remeasures_shrinking_percent_basis_subtree() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(54.0),
        height: Length::points(26.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Column,
        flex_basis: Length::points(20.0),
        flex_shrink: 1.0,
        width: Length::points(20.0),
        align_self: Some(AlignItems::Stretch),
        ..Style::default()
    })));
    let grandchild = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(75.0),
        flex_shrink: 1.0,
        height: Length::points(9.0),
        ..Style::default()
    })));
    let leaf = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(6.0),
        height: Length::points(4.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, grandchild);
    tree.append_child(grandchild, leaf);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(54.0, 26.0));
}

#[test]
fn head_to_head_flex_explicit_shrinking_stretch_remeasures_aligned_inflexible_percent_basis_subtree(
) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(56.0),
        height: Length::points(26.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Column,
        flex_basis: Length::points(19.0),
        flex_grow: 0.0,
        flex_shrink: 1.0,
        width: Length::points(19.0),
        align_self: Some(AlignItems::Stretch),
        ..Style::default()
    })));
    let aligned = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(80.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        align_self: Some(AlignItems::Center),
        height: Length::points(11.0),
        ..Style::default()
    })));
    let leaf = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(6.0),
        height: Length::points(4.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, aligned);
    tree.append_child(aligned, leaf);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(56.0, 26.0));
}

#[test]
fn head_to_head_flex_growing_explicit_stretch_remeasures_shrinking_percent_basis_subtree() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(72.0),
        height: Length::points(28.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Column,
        flex_basis: Length::points(18.0),
        flex_grow: 1.0,
        flex_shrink: 0.0,
        width: Length::points(18.0),
        align_self: Some(AlignItems::Stretch),
        ..Style::default()
    })));
    let grandchild = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(70.0),
        flex_grow: 0.0,
        flex_shrink: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    let leaf = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(5.0),
        height: Length::points(4.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, grandchild);
    tree.append_child(grandchild, leaf);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(72.0, 28.0));
}

#[test]
fn head_to_head_flex_growing_explicit_stretch_handles_inflexible_percent_basis_subtree() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(76.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Column,
        flex_basis: Length::points(18.0),
        flex_grow: 1.0,
        flex_shrink: 0.0,
        width: Length::points(18.0),
        align_self: Some(AlignItems::Stretch),
        ..Style::default()
    })));
    let percent = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(55.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        height: Length::points(7.0),
        ..Style::default()
    })));
    let leaf = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(5.0),
        height: Length::points(3.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, percent);
    tree.append_child(percent, leaf);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(76.0, 30.0));
}

#[test]
#[ignore = "Length::fr is a Lynx extension outside the W3C flexbox algorithm audit"]
fn head_to_head_flex_column_stretch_with_fr_sibling_preserves_percent_basis() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        direction: Direction::Rtl,
        box_sizing: BoxSizing::BorderBox,
        width: Length::points(30.0),
        min_width: Length::points(20.0),
        min_height: Length::points(16.0),
        padding: Rect::new(
            Length::points(1.0),
            Length::ZERO,
            Length::points(3.0),
            Length::ZERO,
        ),
        border: Rect::new(1.0, 0.0, 0.0, 1.0),
        flex_direction: FlexDirection::ColumnReverse,
        flex_wrap: FlexWrap::Wrap,
        justify_content: JustifyContent::Center,
        align_items: AlignItems::Center,
        align_content: AlignContent::FlexEnd,
        row_gap: Length::points(1.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(block_standalone_style(Style {
        box_sizing: BoxSizing::BorderBox,
        width: Length::points(42.0),
        min_width: Length::points(8.0),
        margin: Rect::new(
            Length::points(7.0),
            Length::points(7.0),
            Length::ZERO,
            Length::points(7.0),
        ),
        padding: Rect::new(
            Length::points(7.0),
            Length::points(3.0),
            Length::points(7.0),
            Length::ZERO,
        ),
        border: Rect::all(1.0),
        flex_basis: Length::percent(50.0),
        flex_shrink: 0.0,
        order: -1,
        justify_self: JustifyItems::Center,
        ..Style::default()
    })));
    let second = tree.push(SimpleNode::new(block_standalone_style(Style {
        direction: Direction::Rtl,
        width: Length::points(54.0),
        height: Length::points(36.0),
        min_width: Length::points(16.0),
        max_width: Length::points(36.0),
        margin: Rect::new(
            Length::points(5.0),
            Length::points(3.0),
            Length::ZERO,
            Length::points(7.0),
        ),
        padding: Rect::new(
            Length::ZERO,
            Length::points(3.0),
            Length::ZERO,
            Length::ZERO,
        ),
        border: Rect::all(1.0),
        justify_self: JustifyItems::End,
        ..Style::default()
    })));
    let third = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        box_sizing: BoxSizing::BorderBox,
        width: Length::fr(1.0),
        height: Length::points(24.0),
        min_width: Length::fr(8.0),
        min_height: Length::points(16.0),
        max_width: Length::fr(26.0),
        max_height: Length::points(40.0),
        margin: Rect::new(
            Length::ZERO,
            Length::ZERO,
            Length::ZERO,
            Length::points(5.0),
        ),
        padding: Rect::new(
            Length::points(7.0),
            Length::ZERO,
            Length::points(5.0),
            Length::points(5.0),
        ),
        border: Rect::all(1.0),
        flex_basis: Length::percent(60.0),
        flex_grow: 1.0,
        order: 1,
        align_self: Some(AlignItems::End),
        justify_self: JustifyItems::Auto,
        column_gap: Length::points(1.0),
        ..Style::default()
    }));
    let fourth = tree.push(SimpleNode::new(block_standalone_style(Style {
        direction: Direction::Rtl,
        height: Length::points(66.0),
        min_width: Length::fr(4.0),
        min_height: Length::points(20.0),
        max_width: Length::fr(18.0),
        max_height: Length::points(44.0),
        margin: Rect::new(
            Length::ZERO,
            Length::ZERO,
            Length::points(7.0),
            Length::points(1.0),
        ),
        padding: Rect::new(
            Length::points(7.0),
            Length::ZERO,
            Length::points(1.0),
            Length::ZERO,
        ),
        border: Rect::all(1.0),
        flex_basis: Length::fr(2.0),
        flex_grow: 1.0,
        order: 2,
        align_self: Some(AlignItems::FlexStart),
        justify_self: JustifyItems::Center,
        row_gap: Length::points(3.0),
        column_gap: Length::points(5.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        direction: Direction::Rtl,
        box_sizing: BoxSizing::BorderBox,
        flex_direction: FlexDirection::Row,
        width: Length::Auto,
        height: Length::fr(3.0),
        min_width: Length::fr(10.0),
        min_height: Length::points(8.0),
        margin: Rect::new(
            Length::points(5.0),
            Length::points(5.0),
            Length::ZERO,
            Length::ZERO,
        ),
        padding: Rect::new(
            Length::ZERO,
            Length::ZERO,
            Length::points(3.0),
            Length::ZERO,
        ),
        border: Rect::all(1.0),
        flex_basis: Length::points(54.0),
        flex_grow: 1.0,
        flex_shrink: 0.0,
        order: 3,
        align_self: Some(AlignItems::Stretch),
        justify_self: JustifyItems::End,
        row_gap: Length::points(3.0),
        column_gap: Length::points(3.0),
        ..Style::default()
    }));
    let flexible = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(11.0),
        height: Length::percent(60.0),
        min_width: Length::points(8.0),
        max_width: Length::points(36.0),
        max_height: Length::points(40.0),
        margin: Rect::new(
            Length::points(1.0),
            Length::ZERO,
            Length::points(5.0),
            Length::ZERO,
        ),
        padding: Rect::new(
            Length::ZERO,
            Length::points(1.0),
            Length::points(5.0),
            Length::points(1.0),
        ),
        flex_basis: Length::fr(2.0),
        justify_self: JustifyItems::Start,
        row_gap: Length::points(1.0),
        ..Style::default()
    })));
    let percent = tree.push(SimpleNode::new(block_standalone_style(Style {
        direction: Direction::Rtl,
        width: Length::points(8.0),
        margin: Rect::new(
            Length::points(3.0),
            Length::points(5.0),
            Length::points(1.0),
            Length::ZERO,
        ),
        padding: Rect::new(
            Length::points(5.0),
            Length::ZERO,
            Length::ZERO,
            Length::points(1.0),
        ),
        border: Rect::all(1.0),
        flex_basis: Length::percent(60.0),
        flex_shrink: 0.0,
        justify_self: JustifyItems::Center,
        ..Style::default()
    })));
    tree.append_child(root, first);
    tree.append_child(root, second);
    tree.append_child(root, third);
    tree.append_child(root, fourth);
    tree.append_child(root, child);
    tree.append_child(child, flexible);
    tree.append_child(child, percent);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::at_most(180.0),
            SideConstraint::at_most(140.0),
        ),
    );
}

#[test]
fn head_to_head_flex_shrinking_explicit_stretch_handles_inflexible_percent_basis_subtree() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(60.0),
        height: Length::points(27.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Column,
        flex_basis: Length::points(20.0),
        flex_grow: 0.0,
        flex_shrink: 1.0,
        width: Length::points(20.0),
        align_self: Some(AlignItems::Stretch),
        ..Style::default()
    })));
    let percent = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(65.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        height: Length::points(8.0),
        ..Style::default()
    })));
    let leaf = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(4.0),
        height: Length::points(3.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, percent);
    tree.append_child(percent, leaf);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(60.0, 27.0));
}

#[test]
fn head_to_head_flex_explicit_stretch_remeasures_growing_percent_basis_subtree() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(78.0),
        height: Length::points(29.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Column,
        flex_basis: Length::points(18.0),
        flex_grow: 1.0,
        flex_shrink: 0.0,
        width: Length::points(18.0),
        align_self: Some(AlignItems::Stretch),
        ..Style::default()
    })));
    let percent = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(45.0),
        flex_grow: 1.0,
        flex_shrink: 0.0,
        height: Length::points(7.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(5.0),
        height: Length::points(5.0),
        ..Style::default()
    })));
    let leaf = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(4.0),
        height: Length::points(3.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, percent);
    tree.append_child(child, fixed);
    tree.append_child(percent, leaf);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(78.0, 29.0));
}

#[test]
fn head_to_head_flex_percent_basis_explicit_stretch_remeasures_flexible_percent_basis_subtree() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(80.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Column,
        flex_basis: Length::percent(50.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        width: Length::points(18.0),
        align_self: Some(AlignItems::Stretch),
        ..Style::default()
    })));
    let percent = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(45.0),
        flex_grow: 1.0,
        flex_shrink: 0.0,
        height: Length::points(7.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(5.0),
        height: Length::points(5.0),
        ..Style::default()
    })));
    let leaf = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(4.0),
        height: Length::points(3.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, percent);
    tree.append_child(child, fixed);
    tree.append_child(percent, leaf);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(80.0, 30.0));
}

#[test]
fn head_to_head_flex_implicit_growing_stretch_remeasures_aligned_growing_percent_basis_subtree() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::Stretch,
        width: Length::points(80.0),
        height: Length::points(32.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Column,
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        flex_shrink: 0.0,
        width: Length::points(20.0),
        ..Style::default()
    })));
    let percent = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(60.0),
        flex_grow: 1.0,
        flex_shrink: 0.0,
        align_self: Some(AlignItems::Center),
        height: Length::points(8.0),
        ..Style::default()
    })));
    let flexible = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(6.0),
        flex_grow: 1.0,
        height: Length::points(6.0),
        ..Style::default()
    })));
    let leaf = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(5.0),
        height: Length::points(4.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, percent);
    tree.append_child(child, flexible);
    tree.append_child(percent, leaf);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(80.0, 32.0));
}

#[test]
fn head_to_head_flex_implicit_stretch_remeasures_shared_growing_percent_basis_line() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::Stretch,
        width: Length::points(64.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Column,
        flex_basis: Length::points(22.0),
        width: Length::points(22.0),
        ..Style::default()
    })));
    let stretched_percent = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(45.0),
        align_self: Some(AlignItems::Stretch),
        height: Length::points(7.0),
        ..Style::default()
    })));
    let growing_percent = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(35.0),
        flex_grow: 1.0,
        flex_shrink: 0.0,
        height: Length::points(6.0),
        ..Style::default()
    })));
    let leaf = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(4.0),
        height: Length::points(3.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, stretched_percent);
    tree.append_child(child, growing_percent);
    tree.append_child(stretched_percent, leaf);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(64.0, 30.0));
}

#[test]
fn head_to_head_flex_implicit_stretch_remeasures_local_inflexible_percent_basis_subtree() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::Stretch,
        width: Length::points(58.0),
        height: Length::points(27.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Column,
        flex_basis: Length::points(19.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        width: Length::points(19.0),
        ..Style::default()
    })));
    let percent = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(65.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        width: Length::points(8.0),
        ..Style::default()
    })));
    let leaf = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(4.0),
        height: Length::points(3.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, percent);
    tree.append_child(percent, leaf);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(58.0, 27.0));
}

#[test]
fn head_to_head_flex_implicit_non_shrinking_stretch_remeasures_aligned_inflexible_percent_basis_subtree(
) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::Stretch,
        width: Length::points(59.0),
        height: Length::points(28.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Column,
        flex_basis: Length::points(20.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        width: Length::points(20.0),
        ..Style::default()
    })));
    let aligned = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(60.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        align_self: Some(AlignItems::Center),
        height: Length::points(8.0),
        ..Style::default()
    })));
    let leaf = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(4.0),
        height: Length::points(3.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, aligned);
    tree.append_child(aligned, leaf);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(59.0, 28.0));
}

#[test]
fn head_to_head_flex_implicit_shrinking_stretch_remeasures_mixed_percent_basis_subtree() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::Stretch,
        width: Length::points(66.0),
        height: Length::points(34.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Column,
        flex_basis: Length::points(20.0),
        flex_grow: 0.0,
        flex_shrink: 1.0,
        width: Length::points(20.0),
        ..Style::default()
    })));
    let inflexible = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(40.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        height: Length::points(8.0),
        ..Style::default()
    })));
    let growing = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(30.0),
        flex_grow: 1.0,
        flex_shrink: 0.0,
        height: Length::points(7.0),
        ..Style::default()
    })));
    let leaf = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(4.0),
        height: Length::points(3.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, inflexible);
    tree.append_child(child, growing);
    tree.append_child(inflexible, leaf);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(66.0, 34.0));
}

#[test]
fn head_to_head_flex_implicit_shrinking_stretch_remeasures_aligned_shrinking_percent_basis_subtree()
{
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::Stretch,
        width: Length::points(62.0),
        height: Length::points(29.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Column,
        flex_basis: Length::points(21.0),
        flex_grow: 0.0,
        flex_shrink: 1.0,
        width: Length::points(21.0),
        ..Style::default()
    })));
    let aligned = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(72.0),
        flex_grow: 0.0,
        flex_shrink: 1.0,
        align_self: Some(AlignItems::Center),
        height: Length::points(9.0),
        ..Style::default()
    })));
    let leaf = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(5.0),
        height: Length::points(3.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, aligned);
    tree.append_child(aligned, leaf);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(62.0, 29.0));
}

#[test]
fn head_to_head_flex_implicit_stretch_defines_percent_basis_for_non_shrinking_descendant() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::Stretch,
        width: Length::points(90.0),
        height: Length::points(33.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Column,
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        flex_shrink: 0.0,
        width: Length::points(20.0),
        ..Style::default()
    })));
    let percent = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(50.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        height: Length::points(8.0),
        ..Style::default()
    })));
    let leaf = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(5.0),
        height: Length::points(3.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, percent);
    tree.append_child(percent, leaf);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(90.0, 33.0));
}

#[test]
fn head_to_head_flex_implicit_stretch_remeasures_unresolved_percent_basis_descendant() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::Stretch,
        height: Length::points(31.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Column,
        flex_basis: Length::percent(60.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        width: Length::points(22.0),
        ..Style::default()
    })));
    let percent = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(50.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        height: Length::points(8.0),
        ..Style::default()
    })));
    let leaf = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(5.0),
        height: Length::points(3.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, percent);
    tree.append_child(percent, leaf);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_owner_definite_width_without_root_width_uses_root_at_most_width() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let child = fixed_flex_child(&mut tree, 30.0, 20.0);
    tree.append_child(root, child);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(50.0),
            SideConstraint::at_most(10.0),
        ),
    );
}

#[test]
fn head_to_head_owner_definite_height_without_root_height_uses_root_at_most_height() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        width: Length::points(100.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let child = fixed_linear_child(&mut tree, Length::points(20.0), Length::points(10.0));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
}

#[test]
fn head_to_head_axis_fr_lengths_are_imported_as_full_values() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::fr(30.0),
        height: Length::fr(12.0),
        ..Style::default()
    })));

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 80.0));
}

#[test]
fn head_to_head_min_max_fr_lengths_are_imported_as_full_values() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(10.0),
        min_width: Length::fr(30.0),
        height: Length::points(40.0),
        max_height: Length::fr(12.0),
        ..Style::default()
    })));

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 80.0));
}

#[test]
#[ignore = "Length::fr is a Lynx extension outside the W3C flexbox algorithm audit"]
fn head_to_head_flex_basis_fr_length_is_imported_as_full_value() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(20.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::fr(30.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_owner_definite_width_strips_root_horizontal_margins() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        margin: Rect::new(
            Length::points(5.0),
            Length::points(7.0),
            Length::ZERO,
            Length::ZERO,
        ),
        ..Style::default()
    })));
    let child = fixed_flex_child(&mut tree, 80.0, 10.0);
    tree.append_child(root, child);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );
}

#[test]
fn head_to_head_rtl_row_uses_right_main_front() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        direction: Direction::Rtl,
        width: Length::points(100.0),
        height: Length::points(10.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style {
            flex_basis: Length::points(10.0),
            height: Length::points(10.0),
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_rtl_row_reverse_uses_left_main_front() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        direction: Direction::Rtl,
        flex_direction: FlexDirection::RowReverse,
        width: Length::points(100.0),
        height: Length::points(10.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style {
            flex_basis: Length::points(10.0),
            height: Length::points(10.0),
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_explicit_ltr_no_wrap_mapping_keeps_single_flex_line() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        direction: Direction::Ltr,
        flex_wrap: FlexWrap::NoWrap,
        width: Length::points(50.0),
        height: Length::points(20.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style {
            flex_basis: Length::points(30.0),
            height: Length::points(10.0),
            flex_shrink: 0.0,
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 20.0));
}

#[test]
fn head_to_head_auto_margin_consumes_remaining_main_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(20.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        height: Length::points(10.0),
        margin: Rect::new(Length::Auto, Length::ZERO, Length::ZERO, Length::ZERO),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_multiple_main_axis_auto_margins_share_positive_free_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        justify_content: JustifyContent::Center,
        width: Length::points(100.0),
        height: Length::points(10.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        height: Length::points(10.0),
        margin: Rect::new(Length::Auto, Length::Auto, Length::ZERO, Length::ZERO),
        ..Style::default()
    })));
    let second = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        height: Length::points(10.0),
        margin: Rect::new(Length::Auto, Length::ZERO, Length::ZERO, Length::ZERO),
        ..Style::default()
    })));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_flex_wrap_collects_items_into_multiple_lines() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_wrap: FlexWrap::Wrap,
        width: Length::points(50.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    for _ in 0..3 {
        let child = fixed_flex_child(&mut tree, 30.0, 10.0);
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(SideConstraint::definite(50.0), SideConstraint::indefinite()),
    );
}

#[test]
fn head_to_head_flex_wrap_collects_zero_sized_item_after_exact_fit_on_same_line() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_wrap: FlexWrap::Wrap,
        width: Length::points(50.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let first = fixed_flex_child(&mut tree, 50.0, 10.0);
    let zero = fixed_flex_child(&mut tree, 0.0, 6.0);
    let next_line = fixed_flex_child(&mut tree, 10.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, zero);
    tree.append_child(root, next_line);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(SideConstraint::definite(50.0), SideConstraint::indefinite()),
    );
}

#[test]
fn head_to_head_flex_main_size_nowrap_collects_all_items_into_single_line_even_when_overflowing() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_wrap: FlexWrap::NoWrap,
        width: Length::points(50.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style {
            flex_basis: Length::points(40.0),
            flex_shrink: 0.0,
            height: Length::points(10.0),
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(SideConstraint::definite(50.0), SideConstraint::indefinite()),
    );
}

#[test]
fn head_to_head_flex_main_size_wrap_collects_oversized_first_item_alone() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_wrap: FlexWrap::Wrap,
        width: Length::points(30.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let oversized = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(50.0),
        flex_shrink: 0.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    let next = fixed_flex_child(&mut tree, 10.0, 10.0);
    tree.append_child(root, oversized);
    tree.append_child(root, next);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(SideConstraint::definite(30.0), SideConstraint::indefinite()),
    );
}

#[test]
fn head_to_head_flex_main_size_line_collection_uses_outer_hypothetical_main_with_negative_margin() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_wrap: FlexWrap::Wrap,
        width: Length::points(50.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(40.0),
        height: Length::points(10.0),
        margin: Rect::new(
            Length::ZERO,
            Length::points(-30.0),
            Length::ZERO,
            Length::ZERO,
        ),
        ..Style::default()
    })));
    let second = fixed_flex_child(&mut tree, 40.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(SideConstraint::definite(50.0), SideConstraint::indefinite()),
    );
}

#[test]
fn head_to_head_flex_main_size_resolves_flexible_lengths_per_line_independently() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_wrap: FlexWrap::Wrap,
        width: Length::points(100.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    for basis in [40.0, 40.0, 40.0, 20.0] {
        let child = tree.push(SimpleNode::new(standalone_style(Style {
            flex_basis: Length::points(basis),
            flex_grow: 1.0,
            height: Length::points(10.0),
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );
}

#[test]
fn head_to_head_flex_nowrap_cross_axis_at_most_does_not_clamp_latest_mode() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(50.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let child = fixed_flex_child(&mut tree, 30.0, 20.0);
    tree.append_child(root, child);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(50.0),
            SideConstraint::at_most(10.0),
        ),
    );
}

#[test]
fn head_to_head_single_line_min_cross_size_clamps_line_before_cross_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(50.0),
        min_height: Length::points(30.0),
        align_items: AlignItems::Center,
        ..Style::default()
    })));
    let child = fixed_flex_child(&mut tree, 10.0, 10.0);
    tree.append_child(root, child);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(SideConstraint::definite(50.0), SideConstraint::indefinite()),
    );
}

#[test]
fn head_to_head_flex_wrap_cross_axis_at_most_does_not_clamp_line_sum_latest_mode() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_wrap: FlexWrap::Wrap,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = fixed_flex_child(&mut tree, 30.0, 10.0);
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(50.0),
            SideConstraint::at_most(15.0),
        ),
    );
}

#[test]
fn head_to_head_align_self_overrides_container_align_items() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(50.0),
        height: Length::points(30.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let centered = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(10.0),
        height: Length::points(10.0),
        align_self: Some(AlignItems::Center),
        ..Style::default()
    })));
    let start = fixed_flex_child(&mut tree, 10.0, 10.0);
    tree.append_child(root, centered);
    tree.append_child(root, start);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 30.0));
}

#[test]
fn head_to_head_justify_content_start_end_variants() {
    for justify_content in [JustifyContent::Start, JustifyContent::End] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(standalone_style(Style {
            width: Length::points(60.0),
            height: Length::points(20.0),
            align_items: AlignItems::FlexStart,
            justify_content,
            ..Style::default()
        })));
        for _ in 0..2 {
            let child = fixed_flex_child(&mut tree, 10.0, 10.0);
            tree.append_child(root, child);
        }

        assert_head_to_head_or_skip(tree, root, Constraints::definite(60.0, 20.0));
    }
}

#[test]
fn head_to_head_align_start_end_variants_in_flex_cross_axis() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(50.0),
        height: Length::points(40.0),
        align_items: AlignItems::End,
        justify_content: JustifyContent::FlexStart,
        ..Style::default()
    })));
    let end_aligned = fixed_flex_child(&mut tree, 10.0, 10.0);
    let start_aligned = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(10.0),
        height: Length::points(10.0),
        align_self: Some(AlignItems::Start),
        ..Style::default()
    })));
    tree.append_child(root, end_aligned);
    tree.append_child(root, start_aligned);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 40.0));
}

#[test]
fn head_to_head_align_items_center_uses_negative_cross_space_when_item_overflows() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(50.0),
        height: Length::points(30.0),
        align_items: AlignItems::Center,
        ..Style::default()
    })));
    let child = fixed_flex_child(&mut tree, 10.0, 50.0);
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 30.0));
}

#[test]
fn head_to_head_align_items_flex_end_uses_negative_cross_space_when_item_overflows() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(50.0),
        height: Length::points(30.0),
        align_items: AlignItems::FlexEnd,
        ..Style::default()
    })));
    let child = fixed_flex_child(&mut tree, 10.0, 50.0);
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 30.0));
}

#[test]
fn head_to_head_align_content_centers_wrapped_lines_in_cross_axis() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_wrap: FlexWrap::Wrap,
        align_content: AlignContent::Center,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(70.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = fixed_flex_child(&mut tree, 30.0, 10.0);
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 70.0));
}

#[test]
fn head_to_head_align_content_stretch_expands_wrapped_line_cross_sizes() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_wrap: FlexWrap::Wrap,
        align_content: AlignContent::Stretch,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(50.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = fixed_flex_child(&mut tree, 30.0, 10.0);
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 50.0));
}

#[test]
fn head_to_head_stretched_flex_item_relayouts_percent_height_child_with_definite_cross_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::Stretch,
        width: Length::points(80.0),
        height: Length::points(40.0),
        ..Style::default()
    })));
    let stretched = tree.push(SimpleNode::new(block_standalone_style(Style {
        flex_basis: Length::points(20.0),
        width: Length::points(20.0),
        height: Length::Auto,
        ..Style::default()
    })));
    let percent_child = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(10.0),
        height: Length::percent(50.0),
        ..Style::default()
    })));
    tree.append_child(root, stretched);
    tree.append_child(stretched, percent_child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(80.0, 40.0));
}

#[test]
fn head_to_head_stretched_flex_item_cross_size_respects_min_max_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::Stretch,
        width: Length::points(120.0),
        height: Length::points(60.0),
        ..Style::default()
    })));
    let capped = tree.push(SimpleNode::new(block_standalone_style(Style {
        flex_basis: Length::points(20.0),
        width: Length::points(20.0),
        height: Length::Auto,
        max_height: Length::points(35.0),
        ..Style::default()
    })));
    let floored = tree.push(SimpleNode::new(block_standalone_style(Style {
        flex_basis: Length::points(20.0),
        width: Length::points(20.0),
        height: Length::Auto,
        min_height: Length::points(75.0),
        ..Style::default()
    })));
    tree.append_child(root, capped);
    tree.append_child(root, floored);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(120.0, 60.0));
}

#[test]
fn head_to_head_stretched_flex_item_with_aspect_ratio_keeps_flexed_main_size_and_uses_line_cross_size(
) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::Stretch,
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    })));
    let stretched = tree.push(SimpleNode::new(block_standalone_style(Style {
        flex_basis: Length::points(40.0),
        height: Length::Auto,
        aspect_ratio: Some(2.0),
        ..Style::default()
    })));
    tree.append_child(root, stretched);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 80.0));
}

#[test]
fn head_to_head_align_content_start_end_alias_flex_edges_for_wrapped_lines() {
    for align_content in [AlignContent::Start, AlignContent::End] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(standalone_style(Style {
            flex_wrap: FlexWrap::Wrap,
            align_content,
            align_items: AlignItems::FlexStart,
            width: Length::points(50.0),
            height: Length::points(70.0),
            ..Style::default()
        })));
        for _ in 0..2 {
            let child = fixed_flex_child(&mut tree, 30.0, 10.0);
            tree.append_child(root, child);
        }

        assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 70.0));
    }
}

#[test]
fn head_to_head_align_content_center_uses_negative_free_space_when_lines_overflow() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_wrap: FlexWrap::Wrap,
        align_content: AlignContent::Center,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(15.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = fixed_flex_child(&mut tree, 30.0, 10.0);
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 15.0));
}

#[test]
fn head_to_head_align_content_flex_end_places_wrapped_lines_at_cross_end() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_wrap: FlexWrap::Wrap,
        align_content: AlignContent::FlexEnd,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(70.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = fixed_flex_child(&mut tree, 30.0, 10.0);
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 70.0));
}

#[test]
fn head_to_head_explicit_stretch_justify_and_align_content_mapping() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_wrap: FlexWrap::Wrap,
        justify_content: JustifyContent::Stretch,
        align_content: AlignContent::Stretch,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(70.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = fixed_flex_child(&mut tree, 30.0, 10.0);
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 70.0));
}

#[test]
fn head_to_head_justify_content_stretch_behaves_like_flex_start_in_flex_layout() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        justify_content: JustifyContent::Stretch,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = fixed_flex_child(&mut tree, 10.0, 10.0);
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_align_content_space_between_keeps_row_gap_when_lines_overflow() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_wrap: FlexWrap::Wrap,
        align_content: AlignContent::SpaceBetween,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(15.0),
        row_gap: Length::points(10.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = fixed_flex_child(&mut tree, 30.0, 10.0);
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 15.0));
}

#[test]
fn head_to_head_justify_content_center_uses_negative_free_space_when_items_overflow() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        justify_content: JustifyContent::Center,
        align_items: AlignItems::FlexStart,
        width: Length::points(40.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style {
            flex_basis: Length::points(30.0),
            flex_shrink: 0.0,
            height: Length::points(10.0),
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(40.0, 10.0));
}

#[test]
fn head_to_head_align_content_space_around_centers_overflow_and_keeps_row_gap() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_wrap: FlexWrap::Wrap,
        align_content: AlignContent::SpaceAround,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(15.0),
        row_gap: Length::points(10.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = fixed_flex_child(&mut tree, 30.0, 10.0);
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 15.0));
}

#[test]
fn head_to_head_align_content_space_evenly_distributes_wrapped_lines() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_wrap: FlexWrap::Wrap,
        align_content: AlignContent::SpaceEvenly,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = fixed_flex_child(&mut tree, 30.0, 10.0);
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 100.0));
}

#[test]
fn head_to_head_align_content_space_evenly_uses_negative_space_when_lines_overflow() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_wrap: FlexWrap::Wrap,
        align_content: AlignContent::SpaceEvenly,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(15.0),
        row_gap: Length::points(10.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = fixed_flex_child(&mut tree, 30.0, 10.0);
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 15.0));
}

#[test]
fn head_to_head_justify_content_space_around_uses_edge_difference_width_rounding_when_overflowing()
{
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        justify_content: JustifyContent::SpaceAround,
        align_items: AlignItems::FlexStart,
        width: Length::points(55.0),
        height: Length::points(10.0),
        column_gap: Length::points(10.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style {
            flex_basis: Length::points(30.0),
            flex_shrink: 0.0,
            height: Length::points(10.0),
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(55.0, 10.0));
}

#[test]
fn head_to_head_flex_wrap_reverse_places_first_line_at_cross_end() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_wrap: FlexWrap::WrapReverse,
        align_content: AlignContent::FlexStart,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(70.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = fixed_flex_child(&mut tree, 30.0, 10.0);
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 70.0));
}

#[test]
fn head_to_head_flex_wrap_reverse_reverses_space_between_line_distribution() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_wrap: FlexWrap::WrapReverse,
        align_content: AlignContent::SpaceBetween,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(70.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = fixed_flex_child(&mut tree, 30.0, 10.0);
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 70.0));
}

#[test]
fn head_to_head_flex_wrap_reverse_stretched_line_uses_reversed_cross_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::RowReverse,
        flex_wrap: FlexWrap::WrapReverse,
        justify_content: JustifyContent::FlexEnd,
        align_items: AlignItems::FlexEnd,
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
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            width: Length::points(18.0),
            height: Length::points(11.0),
            ..Style::default()
        }),
        Size::new(18.0, 11.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(180.0, 120.0));
}

#[test]
fn head_to_head_flex_wrap_reverse_center_reexports_cached_block_subtree_with_fractional_offset() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_wrap: FlexWrap::WrapReverse,
        align_items: AlignItems::Center,
        width: Length::points(20.0),
        height: Length::points(9.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style::default())));
    let grandchild = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(10.0),
        height: Length::points(4.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, grandchild);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(20.0, 9.0));
}

#[test]
fn head_to_head_paired_main_axis_auto_margins_center_item() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        height: Length::points(10.0),
        margin: Rect::new(Length::Auto, Length::Auto, Length::ZERO, Length::ZERO),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_cross_axis_auto_margin_overrides_stretch_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::Stretch,
        width: Length::points(50.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            flex_basis: Length::points(10.0),
            margin: Rect::new(Length::ZERO, Length::ZERO, Length::Auto, Length::ZERO),
            ..Style::default()
        }),
        Size::new(10.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 30.0));
}

#[test]
fn head_to_head_paired_cross_axis_auto_margins_center_item() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(10.0),
        height: Length::points(20.0),
        margin: Rect::new(Length::ZERO, Length::ZERO, Length::Auto, Length::Auto),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 100.0));
}

#[test]
fn head_to_head_overflowing_cross_axis_auto_margins_place_overflow_at_cross_end() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(50.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(10.0),
        height: Length::points(70.0),
        margin: Rect::new(Length::ZERO, Length::ZERO, Length::Auto, Length::Auto),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 50.0));
}

#[test]
fn head_to_head_flex_item_fit_content_width_uses_natural_main_axis_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(200.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::fit_content(Some(BaseLength::fixed(80.0))),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let grandchild = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(120.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, grandchild);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 20.0));
}

#[test]
fn head_to_head_column_flex_item_fit_content_height_uses_natural_main_axis_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Column,
        align_items: AlignItems::FlexStart,
        width: Length::points(20.0),
        height: Length::points(200.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(10.0),
        height: Length::fit_content(Some(BaseLength::fixed(80.0))),
        ..Style::default()
    })));
    let grandchild = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(10.0),
        height: Length::points(120.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, grandchild);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(20.0, 200.0));
}

#[test]
fn head_to_head_column_flex_item_percent_cross_size_and_aspect_ratio_define_main_basis() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Column,
        align_items: AlignItems::FlexEnd,
        justify_content: JustifyContent::Center,
        width: Length::points(126.0),
        height: Length::points(92.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            box_sizing: BoxSizing::BorderBox,
            width: Length::percent(38.0),
            height: Length::Auto,
            aspect_ratio: Some(1.5),
            padding: Rect::all(Length::points(1.0)),
            border: Rect::all(1.0),
            ..Style::default()
        }),
        Size::new(44.0, 18.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(126.0, 92.0));
}

#[test]
fn head_to_head_flex_line_length_available_main_space_uses_inner_content_box_for_auto_basis() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        height: Length::points(20.0),
        padding: Rect::new(
            Length::points(5.0),
            Length::points(5.0),
            Length::points(0.0),
            Length::points(0.0),
        ),
        border: Rect::new(2.0, 2.0, 0.0, 0.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measure_func(
        block_standalone_style(Style {
            height: Length::points(10.0),
            ..Style::default()
        }),
        measure_available_width,
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_flex_line_length_definite_flex_basis_overrides_main_size_property() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        flex_basis: Length::points(30.0),
        width: Length::points(80.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_flex_line_length_aspect_ratio_uses_definite_cross_size_for_content_basis() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(40.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        height: Length::points(30.0),
        aspect_ratio: Some(2.0),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 40.0));
}

#[test]
fn head_to_head_flex_line_length_hypothetical_main_size_clamps_min_before_wrapping() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_wrap: FlexWrap::Wrap,
        align_items: AlignItems::FlexStart,
        width: Length::points(80.0),
        ..Style::default()
    })));
    let clamped = tree.push(SimpleNode::new(block_standalone_style(Style {
        flex_basis: Length::points(20.0),
        min_width: Length::points(50.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let sibling = fixed_flex_child(&mut tree, 40.0, 10.0);
    tree.append_child(root, clamped);
    tree.append_child(root, sibling);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(SideConstraint::definite(80.0), SideConstraint::indefinite()),
    );
}

#[test]
fn head_to_head_flex_line_length_hypothetical_main_size_clamps_max_before_wrapping() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_wrap: FlexWrap::Wrap,
        align_items: AlignItems::FlexStart,
        width: Length::points(70.0),
        ..Style::default()
    })));
    let clamped = tree.push(SimpleNode::new(block_standalone_style(Style {
        flex_basis: Length::points(80.0),
        max_width: Length::points(30.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let sibling = fixed_flex_child(&mut tree, 40.0, 10.0);
    tree.append_child(root, clamped);
    tree.append_child(root, sibling);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(SideConstraint::definite(70.0), SideConstraint::indefinite()),
    );
}

#[test]
fn head_to_head_flex_line_length_auto_container_main_size_uses_max_content_sum() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let first = fixed_flex_child(&mut tree, 30.0, 10.0);
    let second = fixed_flex_child(&mut tree, 20.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_root_flex_fit_content_percent_argument_caps_final_width() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 50.0))),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(70.0),
        flex_shrink: 0.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    let second = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(70.0),
        flex_shrink: 0.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 20.0));
}

#[test]
fn head_to_head_root_flex_fit_content_calc_argument_caps_final_width() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(70.0),
        flex_shrink: 0.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    let second = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(70.0),
        flex_shrink: 0.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 20.0));
}

#[test]
fn head_to_head_root_column_flex_fit_content_percent_argument_caps_final_height() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Column,
        align_items: AlignItems::FlexStart,
        width: Length::points(10.0),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 50.0))),
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(70.0),
        flex_shrink: 0.0,
        width: Length::points(10.0),
        ..Style::default()
    })));
    let second = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(70.0),
        flex_shrink: 0.0,
        width: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(20.0, 200.0));
}

#[test]
fn head_to_head_root_column_flex_fit_content_calc_argument_caps_final_height() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Column,
        align_items: AlignItems::FlexStart,
        width: Length::points(10.0),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(70.0),
        flex_shrink: 0.0,
        width: Length::points(10.0),
        ..Style::default()
    })));
    let second = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(70.0),
        flex_shrink: 0.0,
        width: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(20.0, 200.0));
}

#[test]
fn head_to_head_min_width_freezes_item_during_flex_shrink() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(40.0),
        min_width: Length::points(30.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let second = fixed_flex_child(&mut tree, 40.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 10.0));
}

#[test]
fn head_to_head_fit_content_min_width_freezes_item_during_flex_shrink() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(40.0),
        min_width: Length::fit_content(Some(BaseLength::fixed(30.0))),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let second = fixed_flex_child(&mut tree, 40.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 10.0));
}

#[test]
fn head_to_head_max_content_min_width_does_not_freeze_item_during_flex_shrink() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(40.0),
        min_width: Length::MaxContent,
        height: Length::points(10.0),
        ..Style::default()
    })));
    let second = fixed_flex_child(&mut tree, 40.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 10.0));
}

#[test]
fn head_to_head_percent_min_width_freezes_item_during_flex_shrink() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(80.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(60.0),
        min_width: Length::percent(50.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let second = fixed_flex_child(&mut tree, 60.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(80.0, 10.0));
}

#[test]
fn head_to_head_flex_min_target_defines_percent_flex_basis_descendant_base() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(78.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        flex_basis: Length::points(64.0),
        flex_grow: 0.0,
        flex_shrink: 1.0,
        min_width: Length::points(42.0),
        width: Length::points(64.0),
        height: Length::points(14.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let percent = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(50.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        height: Length::points(6.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(5.0),
        width: Length::points(5.0),
        height: Length::points(5.0),
        ..Style::default()
    })));
    let second = fixed_flex_child(&mut tree, 52.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);
    tree.append_child(first, percent);
    tree.append_child(first, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(78.0, 20.0));
}

#[test]
fn head_to_head_border_box_min_width_freezes_flex_item_without_adding_padding_border() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style {
        box_sizing: BoxSizing::BorderBox,
        flex_basis: Length::points(40.0),
        min_width: Length::points(30.0),
        height: Length::points(10.0),
        padding: Rect::new(
            Length::points(5.0),
            Length::points(5.0),
            Length::ZERO,
            Length::ZERO,
        ),
        border: Rect::new(1.0, 1.0, 0.0, 0.0),
        ..Style::default()
    })));
    let second = fixed_flex_child(&mut tree, 40.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 20.0));
}

#[test]
fn head_to_head_flex_multiple_min_width_violations_freeze_before_redistributing_flex_shrink_space()
{
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(180.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(100.0),
        min_width: Length::points(80.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let second = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(100.0),
        min_width: Length::points(70.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let third = fixed_flex_child(&mut tree, 100.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);
    tree.append_child(root, third);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(180.0, 10.0));
}

#[test]
fn head_to_head_flex_min_width_above_basis_freezes_shrinking_item_to_hypothetical_main_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let frozen = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_shrink: 1.0,
        min_width: Length::points(50.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let flexible = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(80.0),
        flex_shrink: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, frozen);
    tree.append_child(root, flexible);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_max_width_freezes_item_and_redistributes_flex_grow_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let capped = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        max_width: Length::points(30.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let flexible = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, capped);
    tree.append_child(root, flexible);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_flex_max_width_below_basis_freezes_growing_item_to_hypothetical_main_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(140.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let capped = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(80.0),
        flex_grow: 1.0,
        max_width: Length::points(50.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let flexible = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, capped);
    tree.append_child(root, flexible);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(140.0, 10.0));
}

#[test]
fn head_to_head_flex_zero_grow_freezes_item_before_distributing_positive_free_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let frozen = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 0.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    let flexible = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, frozen);
    tree.append_child(root, flexible);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_flex_all_zero_grow_items_leave_space_for_justify_content() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        justify_content: JustifyContent::Center,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 0.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    let second = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(30.0),
        flex_grow: 0.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_flex_min_width_violation_freezes_item_during_grow_and_restarts_distribution() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let clamped = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        min_width: Length::points(70.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let flexible = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, clamped);
    tree.append_child(root, flexible);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_flex_main_axis_gap_reduces_free_space_before_grow_distribution() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(110.0),
        height: Length::points(10.0),
        column_gap: Length::points(10.0),
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    let second = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 3.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(110.0, 10.0));
}

#[test]
fn head_to_head_flex_shrink_distribution_is_scaled_by_flex_base_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(120.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let large_base = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(100.0),
        flex_shrink: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    let small_base = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(50.0),
        flex_shrink: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, large_base);
    tree.append_child(root, small_base);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(120.0, 10.0));
}

#[test]
fn head_to_head_flex_shrink_negative_inner_size_is_floored_after_outer_margins() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(0.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style {
            flex_basis: Length::points(10.0),
            flex_shrink: 1.0,
            height: Length::points(10.0),
            margin: Rect::new(
                Length::points(10.0),
                Length::points(10.0),
                Length::ZERO,
                Length::ZERO,
            ),
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(0.0, 10.0));
}

#[test]
fn head_to_head_flex_multiple_max_width_violations_freeze_before_redistributing_flex_grow_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(180.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        max_width: Length::points(30.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let second = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        max_width: Length::points(50.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let third = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, first);
    tree.append_child(root, second);
    tree.append_child(root, third);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(180.0, 10.0));
}

#[test]
fn head_to_head_flex_grow_sum_below_one_leaves_remaining_space_for_justify_content() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        justify_content: JustifyContent::Center,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style {
            flex_basis: Length::points(10.0),
            flex_grow: 0.25,
            height: Length::points(10.0),
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_flex_shrink_sum_below_one_leaves_negative_space_for_justify_content() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        justify_content: JustifyContent::Center,
        width: Length::points(80.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style {
            flex_basis: Length::points(50.0),
            flex_shrink: 0.25,
            height: Length::points(10.0),
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(80.0, 10.0));
}

#[test]
fn head_to_head_flex_zero_shrink_freezes_item_before_distributing_negative_free_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(80.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let frozen = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(50.0),
        flex_shrink: 0.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    let flexible = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(50.0),
        flex_shrink: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, frozen);
    tree.append_child(root, flexible);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(80.0, 10.0));
}

#[test]
fn head_to_head_flex_max_width_violation_freezes_item_during_shrink_and_restarts_distribution() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(160.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let capped = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(100.0),
        flex_shrink: 1.0,
        max_width: Length::points(70.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let flexible = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(100.0),
        flex_shrink: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, capped);
    tree.append_child(root, flexible);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(160.0, 10.0));
}

#[test]
fn head_to_head_border_box_max_width_caps_flex_grow_without_adding_padding_border() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let capped = tree.push(SimpleNode::new(standalone_style(Style {
        box_sizing: BoxSizing::BorderBox,
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        max_width: Length::points(30.0),
        height: Length::points(10.0),
        padding: Rect::new(
            Length::points(5.0),
            Length::points(5.0),
            Length::ZERO,
            Length::ZERO,
        ),
        border: Rect::new(1.0, 1.0, 0.0, 0.0),
        ..Style::default()
    })));
    let flexible = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, capped);
    tree.append_child(root, flexible);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_fit_content_max_width_freezes_item_and_redistributes_flex_grow_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let capped = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        max_width: Length::fit_content(Some(BaseLength::fixed(30.0))),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let flexible = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, capped);
    tree.append_child(root, flexible);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_fit_content_max_width_without_argument_does_not_cap_flex_grow_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        max_width: Length::fit_content(None),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let second = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_percent_max_width_freezes_item_and_redistributes_flex_grow_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let capped = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        max_width: Length::percent(30.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let flexible = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, capped);
    tree.append_child(root, flexible);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_flex_max_target_defines_percent_flex_basis_descendant_base() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let capped = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        flex_shrink: 1.0,
        max_width: Length::points(34.0),
        width: Length::points(20.0),
        height: Length::points(14.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let percent = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(50.0),
        flex_grow: 1.0,
        flex_shrink: 0.0,
        height: Length::points(6.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(5.0),
        width: Length::points(5.0),
        height: Length::points(5.0),
        ..Style::default()
    })));
    let flexible = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, capped);
    tree.append_child(root, flexible);
    tree.append_child(capped, percent);
    tree.append_child(capped, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_flex_max_target_defines_inflexible_percent_basis_child_base() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(96.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let capped = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Row,
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        flex_shrink: 1.0,
        max_width: Length::points(32.0),
        width: Length::points(20.0),
        height: Length::points(14.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let percent = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::percent(50.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        height: Length::points(6.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(5.0),
        width: Length::points(5.0),
        height: Length::points(5.0),
        ..Style::default()
    })));
    let flexible = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, capped);
    tree.append_child(root, flexible);
    tree.append_child(capped, percent);
    tree.append_child(capped, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(96.0, 20.0));
}

#[test]
fn head_to_head_column_percent_min_height_freezes_item_during_flex_shrink() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Column,
        align_items: AlignItems::FlexStart,
        width: Length::points(10.0),
        height: Length::points(80.0),
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(60.0),
        min_height: Length::percent(50.0),
        width: Length::points(10.0),
        ..Style::default()
    })));
    let second = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(60.0),
        width: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(10.0, 80.0));
}

#[test]
fn head_to_head_column_fit_content_min_height_freezes_item_during_flex_shrink() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Column,
        align_items: AlignItems::FlexStart,
        width: Length::points(10.0),
        height: Length::points(80.0),
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(60.0),
        min_height: Length::fit_content(Some(BaseLength::fixed(40.0))),
        width: Length::points(10.0),
        ..Style::default()
    })));
    let second = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(60.0),
        width: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(10.0, 80.0));
}

#[test]
fn head_to_head_column_fit_content_min_height_without_argument_does_not_freeze_item() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Column,
        align_items: AlignItems::FlexStart,
        width: Length::points(10.0),
        height: Length::points(50.0),
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(40.0),
        min_height: Length::fit_content(None),
        width: Length::points(10.0),
        ..Style::default()
    })));
    let second = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(40.0),
        width: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(10.0, 50.0));
}

#[test]
fn head_to_head_column_percent_max_height_freezes_item_and_redistributes_flex_grow_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Column,
        align_items: AlignItems::FlexStart,
        width: Length::points(10.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let capped = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        max_height: Length::percent(30.0),
        width: Length::points(10.0),
        ..Style::default()
    })));
    let flexible = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        width: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, capped);
    tree.append_child(root, flexible);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(10.0, 100.0));
}

#[test]
fn head_to_head_column_fit_content_max_height_freezes_item_and_redistributes_flex_grow_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Column,
        align_items: AlignItems::FlexStart,
        width: Length::points(10.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let capped = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        max_height: Length::fit_content(Some(BaseLength::fixed(30.0))),
        width: Length::points(10.0),
        ..Style::default()
    })));
    let flexible = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        width: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, capped);
    tree.append_child(root, flexible);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(10.0, 100.0));
}

#[test]
fn head_to_head_column_max_content_max_height_does_not_cap_flex_grow_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Column,
        align_items: AlignItems::FlexStart,
        width: Length::points(10.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        max_height: Length::MaxContent,
        width: Length::points(10.0),
        ..Style::default()
    })));
    let second = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        width: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(10.0, 100.0));
}

#[test]
fn head_to_head_row_reverse_flex_grow_freeze_places_flexed_items_from_right_edge() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::RowReverse,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let capped = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        max_width: Length::points(30.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let flexible = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, capped);
    tree.append_child(root, flexible);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_column_reverse_flex_shrink_freeze_places_flexed_items_from_bottom_edge() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::ColumnReverse,
        align_items: AlignItems::FlexStart,
        width: Length::points(10.0),
        height: Length::points(80.0),
        ..Style::default()
    })));
    let frozen = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(50.0),
        flex_shrink: 0.0,
        width: Length::points(10.0),
        ..Style::default()
    })));
    let flexible = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(50.0),
        flex_shrink: 1.0,
        width: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, frozen);
    tree.append_child(root, flexible);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(10.0, 80.0));
}

#[test]
fn head_to_head_flexible_lengths_resolve_independently_per_wrapped_line() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_wrap: FlexWrap::Wrap,
        align_items: AlignItems::FlexStart,
        align_content: AlignContent::FlexStart,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    for (basis, grow) in [(50.0, 1.0), (40.0, 1.0), (20.0, 1.0), (20.0, 3.0)].into_iter() {
        let child = tree.push(SimpleNode::new(standalone_style(Style {
            flex_basis: Length::points(basis),
            flex_grow: grow,
            height: Length::points(10.0),
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_measured_flex_basis_grow_max_width_violation_restarts_distribution() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(120.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let capped = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            flex_grow: 1.0,
            max_width: Length::points(70.0),
            height: Length::points(10.0),
            ..Style::default()
        }),
        Size::new(60.0, 10.0),
    ));
    let flexible = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            flex_grow: 1.0,
            height: Length::points(10.0),
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, capped);
    tree.append_child(root, flexible);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(120.0, 10.0));
}

#[test]
fn head_to_head_measured_flex_basis_shrink_min_width_violation_restarts_distribution() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(80.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let floored = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            min_width: Length::points(50.0),
            height: Length::points(10.0),
            ..Style::default()
        }),
        Size::new(60.0, 10.0),
    ));
    let flexible = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            height: Length::points(10.0),
            ..Style::default()
        }),
        Size::new(60.0, 10.0),
    ));
    tree.append_child(root, floored);
    tree.append_child(root, flexible);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(80.0, 10.0));
}

#[test]
fn head_to_head_nested_intrinsic_flex_basis_grow_max_width_violation_restarts_distribution() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(120.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let capped = tree.push(SimpleNode::new(block_standalone_style(Style {
        max_width: Length::points(70.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    let nested_child = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(60.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let flexible = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, capped);
    tree.append_child(root, flexible);
    tree.append_child(capped, nested_child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(120.0, 10.0));
}

#[test]
fn head_to_head_nested_intrinsic_flex_basis_shrink_min_width_violation_restarts_distribution() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(80.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let floored = tree.push(SimpleNode::new(block_standalone_style(Style {
        min_width: Length::points(50.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let nested_child = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(60.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let flexible = fixed_flex_child(&mut tree, 60.0, 10.0);
    tree.append_child(root, floored);
    tree.append_child(root, flexible);
    tree.append_child(floored, nested_child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(80.0, 10.0));
}

#[test]
fn head_to_head_row_reverse_positions_items_from_right_edge_in_tree_order() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::RowReverse,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = fixed_flex_child(&mut tree, 10.0, 10.0);
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_row_reverse_flex_end_packs_items_at_left_edge() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::RowReverse,
        justify_content: JustifyContent::FlexEnd,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = fixed_flex_child(&mut tree, 10.0, 10.0);
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_column_reverse_positions_items_from_bottom_edge_in_tree_order() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::ColumnReverse,
        align_items: AlignItems::FlexStart,
        width: Length::points(10.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = fixed_flex_child(&mut tree, 10.0, 10.0);
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(10.0, 100.0));
}

#[test]
fn head_to_head_justify_content_main_axis_direction_matrix() {
    for direction_case in NATIVE_MAIN_AXIS_MATRIX {
        for justify_content in NATIVE_JUSTIFY_MATRIX {
            let mut tree = SimpleTree::default();
            let root = tree.push(SimpleNode::new(standalone_style(Style {
                flex_direction: direction_case.flex_direction,
                direction: direction_case.direction,
                justify_content,
                align_items: AlignItems::FlexStart,
                width: Length::points(100.0),
                height: Length::points(100.0),
                ..Style::default()
            })));
            let first = fixed_matrix_flex_child(&mut tree);
            let second = fixed_matrix_flex_child(&mut tree);
            tree.append_child(root, first);
            tree.append_child(root, second);

            assert_head_to_head_or_skip_with_name(
                &format!(
                    "{:?}/{:?}/{:?}",
                    direction_case.flex_direction, direction_case.direction, justify_content
                ),
                tree,
                root,
                Constraints::definite(100.0, 100.0),
            );
        }
    }
}

#[test]
fn head_to_head_main_axis_auto_margin_direction_matrix() {
    for direction_case in NATIVE_MAIN_AXIS_MATRIX {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(standalone_style(Style {
            flex_direction: direction_case.flex_direction,
            direction: direction_case.direction,
            justify_content: JustifyContent::Center,
            align_items: AlignItems::FlexStart,
            width: Length::points(100.0),
            height: Length::points(100.0),
            ..Style::default()
        })));
        let first = fixed_matrix_flex_child(&mut tree);
        let second = tree.push(SimpleNode::new(standalone_style(Style {
            flex_basis: Length::points(10.0),
            width: Length::points(10.0),
            height: Length::points(10.0),
            margin: native_main_start_auto_margin(direction_case),
            ..Style::default()
        })));
        tree.append_child(root, first);
        tree.append_child(root, second);

        assert_head_to_head_or_skip_with_name(
            &format!(
                "{:?}/{:?}/main-start-auto-margin",
                direction_case.flex_direction, direction_case.direction
            ),
            tree,
            root,
            Constraints::definite(100.0, 100.0),
        );
    }
}

#[test]
fn head_to_head_justify_content_gap_overflow_direction_matrix() {
    for direction_case in NATIVE_MAIN_AXIS_MATRIX {
        for justify_content in NATIVE_GAP_OVERFLOW_JUSTIFY_MATRIX {
            let mut tree = SimpleTree::default();
            let root = tree.push(SimpleNode::new(standalone_style(Style {
                flex_direction: direction_case.flex_direction,
                direction: direction_case.direction,
                justify_content,
                align_items: AlignItems::FlexStart,
                width: Length::points(50.0),
                height: Length::points(50.0),
                row_gap: Length::points(10.0),
                column_gap: Length::points(10.0),
                ..Style::default()
            })));
            let first = fixed_main_axis_matrix_flex_child(&mut tree, direction_case, 30.0, 10.0);
            let second = fixed_main_axis_matrix_flex_child(&mut tree, direction_case, 30.0, 10.0);
            tree.append_child(root, first);
            tree.append_child(root, second);

            assert_head_to_head_or_skip_with_name(
                &format!(
                    "{:?}/{:?}/{:?}/gap-overflow",
                    direction_case.flex_direction, direction_case.direction, justify_content
                ),
                tree,
                root,
                Constraints::definite(50.0, 50.0),
            );
        }
    }
}

#[test]
fn head_to_head_flexible_lengths_direction_matrix_places_resolved_main_sizes() {
    for direction_case in NATIVE_MAIN_AXIS_MATRIX {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(standalone_style(Style {
            flex_direction: direction_case.flex_direction,
            direction: direction_case.direction,
            align_items: AlignItems::FlexStart,
            width: Length::points(100.0),
            height: Length::points(100.0),
            ..Style::default()
        })));
        for grow in [1.0, 2.0] {
            let child = tree.push(SimpleNode::new(standalone_style(Style {
                flex_basis: Length::points(20.0),
                flex_grow: grow,
                width: Length::points(if direction_case.flex_direction.is_row() {
                    20.0
                } else {
                    10.0
                }),
                height: Length::points(if direction_case.flex_direction.is_row() {
                    10.0
                } else {
                    20.0
                }),
                ..Style::default()
            })));
            tree.append_child(root, child);
        }

        assert_head_to_head_or_skip_with_name(
            &format!(
                "{:?}/{:?}/flexible-lengths",
                direction_case.flex_direction, direction_case.direction
            ),
            tree,
            root,
            Constraints::definite(100.0, 100.0),
        );
    }
}

#[test]
fn head_to_head_rtl_column_uses_right_cross_start_for_flex_start() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        direction: Direction::Rtl,
        flex_direction: FlexDirection::Column,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(10.0),
        width: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
}

#[test]
fn head_to_head_percent_padding_gap_and_margin() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(120.0),
        height: Length::points(50.0),
        padding: Rect::all(Length::percent(10.0)),
        column_gap: Length::percent(5.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        height: Length::points(8.0),
        margin: Rect::new(
            Length::percent(5.0),
            Length::ZERO,
            Length::percent(2.0),
            Length::ZERO,
        ),
        ..Style::default()
    })));
    let second = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(18.0),
        height: Length::points(10.0),
        margin: Rect::new(
            Length::ZERO,
            Length::percent(4.0),
            Length::ZERO,
            Length::percent(3.0),
        ),
        ..Style::default()
    })));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(120.0, 50.0));
}

#[test]
fn head_to_head_border_box_explicit_size_includes_padding_and_border() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Block,
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
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );
}

#[test]
fn head_to_head_border_box_aspect_ratio_resolves_border_box_height() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Block,
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
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );
}

#[test]
fn head_to_head_linear_auto_main_uses_final_grid_aspect_ratio_child_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(360.0),
        padding: Rect::all(Length::points(2.0)),
        border: Rect::all(1.0),
        ..Style::default()
    })));
    let grid = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::percent(30.0),
        height: Length::Auto,
        min_width: Length::points(28.0),
        min_height: Length::points(12.0),
        max_width: Length::calc(44.0, 32.0),
        max_height: Length::calc(28.0, 45.0),
        aspect_ratio: Some(1.63),
        margin: Rect::new(
            Length::points(1.0),
            Length::ZERO,
            Length::ZERO,
            Length::ZERO,
        ),
        padding: Rect::new(
            Length::points(1.0),
            Length::points(3.0),
            Length::points(1.0),
            Length::points(1.0),
        ),
        border: Rect::new(1.0, 1.0, 1.0, 0.5),
        align_items: AlignItems::Center,
        justify_content: JustifyContent::Center,
        grid_template_columns: vec![Length::points(20.0), Length::Auto],
        grid_template_rows: vec![Length::points(12.0), Length::Auto],
        column_gap: Length::points(1.0),
        row_gap: Length::points(1.0),
        ..Style::default()
    })));
    let grid_child = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(22.0),
        height: Length::points(12.0),
        margin: Rect::new(
            Length::ZERO,
            Length::ZERO,
            Length::points(0.5),
            Length::ZERO,
        ),
        padding: Rect::all(Length::points(0.5)),
        ..Style::default()
    })));
    let sibling = tree.push(SimpleNode::new(block_standalone_style(Style {
        box_sizing: BoxSizing::BorderBox,
        width: Length::calc(8.0, 19.0),
        height: Length::points(25.0),
        min_width: Length::points(24.0),
        min_height: Length::points(13.0),
        max_width: Length::calc(45.0, 32.0),
        max_height: Length::calc(29.0, 45.0),
        margin: Rect::new(
            Length::points(2.0),
            Length::points(0.5),
            Length::points(1.0),
            Length::ZERO,
        ),
        padding: Rect::new(
            Length::points(2.0),
            Length::points(4.0),
            Length::points(1.5),
            Length::points(1.0),
        ),
        border: Rect::new(2.0, 1.5, 1.0, 1.5),
        align_items: AlignItems::Center,
        justify_content: JustifyContent::Center,
        ..Style::default()
    })));
    let sibling_child = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(23.0),
        height: Length::points(8.0),
        margin: Rect::new(
            Length::points(1.0),
            Length::ZERO,
            Length::points(1.0),
            Length::points(1.0),
        ),
        padding: Rect::all(Length::points(1.0)),
        border: Rect::all(1.0),
        ..Style::default()
    })));
    tree.append_child(root, grid);
    tree.append_child(grid, grid_child);
    tree.append_child(root, sibling);
    tree.append_child(sibling, sibling_child);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(SideConstraint::definite(8.0), SideConstraint::indefinite()),
    );
}

#[test]
fn head_to_head_calc_column_gap() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(120.0),
        height: Length::points(30.0),
        column_gap: Length::calc(2.0, 5.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let second = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(18.0),
        height: Length::points(12.0),
        ..Style::default()
    })));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(120.0, 30.0));
}

#[test]
fn head_to_head_full_value_edge_lengths_reach_cpp_baseline_import() {
    for edge_length in [
        Length::MaxContent,
        Length::FitContent(Some(BaseLength::fixed(4.0))),
        Length::fr(1.0),
    ] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(standalone_style(Style {
            width: Length::points(80.0),
            height: Length::points(20.0),
            align_items: AlignItems::FlexStart,
            ..Style::default()
        })));
        let first = tree.push(SimpleNode::new(standalone_style(Style {
            position: PositionType::Relative,
            left: edge_length,
            margin: Rect::new(edge_length, Length::ZERO, Length::ZERO, Length::ZERO),
            padding: Rect::new(edge_length, Length::ZERO, Length::ZERO, Length::ZERO),
            flex_basis: Length::points(10.0),
            height: Length::points(6.0),
            ..Style::default()
        })));
        let second = tree.push(SimpleNode::new(standalone_style(Style {
            flex_basis: Length::points(12.0),
            height: Length::points(8.0),
            ..Style::default()
        })));
        tree.append_child(root, first);
        tree.append_child(root, second);

        assert_head_to_head_or_skip(tree, root, Constraints::definite(80.0, 20.0));
    }
}

#[test]
fn head_to_head_grid_item_percent_edges_keep_cpp_box_data_update_order() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(128.0),
        height: Length::points(64.0),
        padding: Rect::new(
            Length::Auto,
            Length::fr(2.0),
            Length::MaxContent,
            Length::FitContent(None),
        ),
        border: Rect::new(2.0, 0.0, 1.5, 0.75),
        row_gap: Length::FitContent(Some(BaseLength::fixed(4.0))),
        column_gap: Length::FitContent(Some(BaseLength::fixed_and_percent(1.0, 11.0))),
        grid_template_columns: vec![Length::points(28.0), Length::points(34.0)],
        grid_template_rows: vec![Length::points(14.0), Length::points(16.0)],
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let calc_padding = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Relative,
        top: Length::fr(2.0),
        width: Length::points(18.0),
        height: Length::points(8.0),
        margin: Rect::new(
            Length::MaxContent,
            Length::FitContent(None),
            Length::FitContent(Some(BaseLength::fixed(4.0))),
            Length::FitContent(Some(BaseLength::fixed_and_percent(1.0, 11.0))),
        ),
        padding: Rect::new(
            Length::points(6.0),
            Length::percent(7.0),
            Length::calc(3.0, 4.0),
            Length::Auto,
        ),
        border: Rect::new(0.0, 0.5, 0.0, 1.0),
        ..Style::default()
    })));
    let percent_margin = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Relative,
        left: Length::FitContent(None),
        top: Length::FitContent(Some(BaseLength::fixed(4.0))),
        width: Length::points(21.0),
        height: Length::points(10.0),
        margin: Rect::new(
            Length::FitContent(Some(BaseLength::fixed_and_percent(1.0, 11.0))),
            Length::points(6.0),
            Length::percent(7.0),
            Length::calc(3.0, 4.0),
        ),
        padding: Rect::new(
            Length::Auto,
            Length::fr(2.0),
            Length::MaxContent,
            Length::FitContent(None),
        ),
        border: Rect::new(0.5, 1.5, 0.25, 1.0),
        ..Style::default()
    })));
    tree.append_child(root, calc_padding);
    tree.append_child(root, percent_margin);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(320.0, 80.0));
}

#[test]
fn head_to_head_full_value_column_gap_units() {
    for column_gap in [
        Length::MaxContent,
        Length::FitContent(Some(BaseLength::fixed(12.0))),
        Length::fr(1.0),
    ] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(standalone_style(Style {
            width: Length::points(120.0),
            height: Length::points(30.0),
            column_gap,
            align_items: AlignItems::FlexStart,
            ..Style::default()
        })));
        let first = tree.push(SimpleNode::new(standalone_style(Style {
            flex_basis: Length::points(20.0),
            height: Length::points(10.0),
            ..Style::default()
        })));
        let second = tree.push(SimpleNode::new(standalone_style(Style {
            flex_basis: Length::points(18.0),
            height: Length::points(12.0),
            ..Style::default()
        })));
        tree.append_child(root, first);
        tree.append_child(root, second);

        assert_head_to_head_or_skip(tree, root, Constraints::definite(120.0, 30.0));
    }
}

#[test]
fn head_to_head_full_value_row_gap_units() {
    for row_gap in [
        Length::MaxContent,
        Length::FitContent(Some(BaseLength::fixed(12.0))),
        Length::fr(1.0),
    ] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(standalone_style(Style {
            width: Length::points(30.0),
            height: Length::points(80.0),
            flex_wrap: FlexWrap::Wrap,
            row_gap,
            align_items: AlignItems::FlexStart,
            align_content: AlignContent::FlexStart,
            ..Style::default()
        })));
        let first = tree.push(SimpleNode::new(standalone_style(Style {
            flex_basis: Length::points(20.0),
            height: Length::points(10.0),
            ..Style::default()
        })));
        let second = tree.push(SimpleNode::new(standalone_style(Style {
            flex_basis: Length::points(20.0),
            height: Length::points(10.0),
            ..Style::default()
        })));
        tree.append_child(root, first);
        tree.append_child(root, second);

        assert_head_to_head_or_skip(tree, root, Constraints::definite(30.0, 80.0));
    }
}

#[test]
fn head_to_head_calc_size_lengths() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::calc(20.0, 50.0),
        height: Length::calc(10.0, 50.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::calc(10.0, 25.0),
        height: Length::points(12.0),
        min_width: Length::calc(20.0, 0.0),
        max_width: Length::calc(80.0, 0.0),
        min_height: Length::calc(8.0, 0.0),
        max_height: Length::calc(40.0, 0.0),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 80.0));
}

#[test]
fn head_to_head_flex_basis_fit_content_argument_resolves_before_measuring_item() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(10.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            flex_basis: Length::fit_content(Some(BaseLength::fixed(40.0))),
            height: Length::points(10.0),
            ..Style::default()
        }),
        Size::new(80.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_flex_basis_fit_content_percent_argument_resolves_against_main_axis() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(10.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            flex_basis: Length::fit_content(Some(BaseLength::fixed_and_percent(5.0, 50.0))),
            height: Length::points(10.0),
            ..Style::default()
        }),
        Size::new(80.0, 10.0),
    ));
    let second = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(10.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_flex_basis_max_content_uses_auto_measure_path() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(10.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            flex_basis: Length::MaxContent,
            height: Length::points(10.0),
            ..Style::default()
        }),
        Size::new(45.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_measured_fit_content_argument_size_lengths() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            width: Length::fit_content(Some(BaseLength::fixed(80.0))),
            height: Length::fit_content(Some(BaseLength::fixed(20.0))),
            ..Style::default()
        }),
        Size::new(24.0, 9.0),
    ));

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_measured_fit_content_percent_argument_size_lengths() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            width: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 50.0))),
            height: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 25.0))),
            ..Style::default()
        }),
        Size::new(150.0, 70.0),
    ));

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_measured_fit_content_calc_argument_size_lengths() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            width: Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
            height: Length::fit_content(Some(BaseLength::fixed_and_percent(5.0, 25.0))),
            ..Style::default()
        }),
        Size::new(150.0, 70.0),
    ));

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_root_block_fit_content_argument_uses_latest_linear_sizing() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::fit_content(Some(BaseLength::fixed(80.0))),
        height: Length::fit_content(Some(BaseLength::fixed(20.0))),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(120.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_root_block_fit_content_percent_argument_uses_latest_linear_sizing() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 50.0))),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 25.0))),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(120.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_root_block_fit_content_calc_argument_uses_latest_linear_sizing() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(5.0, 25.0))),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(120.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_child_block_fit_content_argument_uses_latest_linear_sizing() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::fit_content(Some(BaseLength::fixed(80.0))),
        height: Length::fit_content(Some(BaseLength::fixed(20.0))),
        ..Style::default()
    })));
    let grandchild = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(120.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, grandchild);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_child_block_fit_content_percent_argument_uses_latest_linear_sizing() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 50.0))),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 25.0))),
        ..Style::default()
    })));
    let grandchild = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(120.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, grandchild);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_child_block_fit_content_calc_argument_uses_latest_linear_sizing() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(5.0, 25.0))),
        ..Style::default()
    })));
    let grandchild = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(120.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, grandchild);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_absolute_block_fit_content_argument_uses_latest_linear_sizing() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Absolute,
        width: Length::fit_content(Some(BaseLength::fixed(80.0))),
        height: Length::fit_content(Some(BaseLength::fixed(20.0))),
        left: Length::points(7.0),
        top: Length::points(9.0),
        ..Style::default()
    })));
    let grandchild = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(120.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, grandchild);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_absolute_subtree_fit_content_percent_argument_uses_latest_linear_sizing() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(122.0),
        height: Length::points(89.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Absolute,
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(5.0, 50.0))),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(3.0, 25.0))),
        left: Length::points(9.0),
        top: Length::points(6.0),
        ..Style::default()
    })));
    let grandchild = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            width: Length::points(74.0),
            height: Length::points(29.0),
            ..Style::default()
        }),
        Size::new(74.0, 29.0),
    ));
    tree.append_child(root, child);
    tree.append_child(child, grandchild);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(180.0, 130.0));
}

#[test]
fn head_to_head_fixed_block_fit_content_argument_uses_latest_linear_sizing() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Fixed,
        width: Length::fit_content(Some(BaseLength::fixed(80.0))),
        height: Length::fit_content(Some(BaseLength::fixed(20.0))),
        left: Length::points(7.0),
        top: Length::points(9.0),
        ..Style::default()
    })));
    let grandchild = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(120.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);
    tree.append_child(fixed, grandchild);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_absolute_measured_fit_content_argument_uses_measured_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            position: PositionType::Absolute,
            width: Length::fit_content(Some(BaseLength::fixed(80.0))),
            height: Length::fit_content(Some(BaseLength::fixed(20.0))),
            left: Length::points(7.0),
            top: Length::points(9.0),
            ..Style::default()
        }),
        Size::new(120.0, 30.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_fixed_measured_fit_content_argument_uses_measured_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            position: PositionType::Fixed,
            width: Length::fit_content(Some(BaseLength::fixed(80.0))),
            height: Length::fit_content(Some(BaseLength::fixed(20.0))),
            left: Length::points(7.0),
            top: Length::points(9.0),
            ..Style::default()
        }),
        Size::new(120.0, 30.0),
    ));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_absolute_block_max_content_uses_latest_linear_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Absolute,
        width: Length::MaxContent,
        height: Length::MaxContent,
        left: Length::points(7.0),
        top: Length::points(9.0),
        ..Style::default()
    })));
    let grandchild = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(250.0),
        height: Length::points(130.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, grandchild);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_fixed_block_max_content_uses_latest_linear_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Fixed,
        width: Length::MaxContent,
        height: Length::MaxContent,
        left: Length::points(7.0),
        top: Length::points(9.0),
        ..Style::default()
    })));
    let grandchild = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(250.0),
        height: Length::points(130.0),
        ..Style::default()
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);
    tree.append_child(fixed, grandchild);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_absolute_measured_max_content_uses_measured_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            position: PositionType::Absolute,
            width: Length::MaxContent,
            height: Length::MaxContent,
            left: Length::points(7.0),
            top: Length::points(9.0),
            ..Style::default()
        }),
        Size::new(250.0, 130.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_fixed_measured_max_content_uses_measured_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            position: PositionType::Fixed,
            width: Length::MaxContent,
            height: Length::MaxContent,
            left: Length::points(7.0),
            top: Length::points(9.0),
            ..Style::default()
        }),
        Size::new(250.0, 130.0),
    ));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_fixed_auto_size_with_percent_and_calc_insets_uses_root_containing_block() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
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
        }),
        Size::new(300.0, 200.0),
    ));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_calc_padding_margin_and_position_edges() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(120.0),
        height: Length::points(60.0),
        padding: Rect::new(
            Length::calc(2.0, 10.0),
            Length::calc(3.0, 5.0),
            Length::calc(1.0, 5.0),
            Length::points(0.0),
        ),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let flow = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        height: Length::points(10.0),
        margin: Rect::new(
            Length::calc(1.0, 5.0),
            Length::calc(2.0, 5.0),
            Length::calc(3.0, 0.0),
            Length::calc(4.0, 0.0),
        ),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::new(standalone_style(Style {
        position: PositionType::Absolute,
        width: Length::points(10.0),
        height: Length::points(8.0),
        left: Length::calc(2.0, 10.0),
        top: Length::calc(3.0, 5.0),
        ..Style::default()
    })));
    tree.append_child(root, flow);
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(120.0, 60.0));
}

#[test]
fn head_to_head_relative_calc_end_offsets_use_parent_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let relative = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Relative,
        width: Length::points(20.0),
        height: Length::points(10.0),
        right: Length::calc(4.0, 5.0),
        bottom: Length::calc(3.0, 10.0),
        ..Style::default()
    })));
    let normal = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(10.0),
        height: Length::points(7.0),
        ..Style::default()
    })));
    tree.append_child(root, relative);
    tree.append_child(root, normal);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_measured_max_content_item() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(100.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            width: Length::max_content(),
            height: Length::max_content(),
            ..Style::default()
        }),
        Size::new(24.0, 9.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );
}

#[test]
fn head_to_head_measured_exact_item_uses_constraints_without_measure_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(100.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size_and_baseline(
        standalone_style(Style {
            width: Length::points(20.0),
            height: Length::points(7.0),
            ..Style::default()
        }),
        Size::new(99.0, 99.0),
        4.0,
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );
}

#[test]
fn head_to_head_absolute_child_with_edges() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(80.0),
        height: Length::points(60.0),
        padding: Rect::all(Length::points(2.0)),
        border: Rect::all(1.0),
        ..Style::default()
    })));
    let in_flow = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::new(standalone_style(Style {
        position: PositionType::Absolute,
        left: Length::points(5.0),
        top: Length::points(7.0),
        width: Length::points(16.0),
        height: Length::points(9.0),
        margin: Rect::all(Length::points(1.0)),
        ..Style::default()
    })));
    tree.append_child(root, in_flow);
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(80.0, 60.0));
}

#[test]
fn head_to_head_absolute_block_child_is_removed_from_flow_and_uses_left_top_insets() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(100.0),
        padding: Rect::all(Length::points(2.0)),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Absolute,
        width: Length::points(20.0),
        height: Length::points(30.0),
        left: Length::points(7.0),
        top: Length::points(9.0),
        ..Style::default()
    })));
    let normal = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(10.0),
        height: Length::points(5.0),
        ..Style::default()
    })));
    tree.append_child(root, absolute);
    tree.append_child(root, normal);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );
}

#[test]
fn head_to_head_absolute_measured_percent_size_resolves_once_against_containing_block() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
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
    })));
    let absolute = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
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
        }),
        Size::new(31.0, 17.0),
    ));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(180.0, 120.0));
}

#[test]
fn head_to_head_absolute_measured_percent_border_box_size_resolves_once_against_containing_block() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
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
    })));
    let absolute = tree.push(SimpleNode::with_measured_size(
        Style {
            display: Display::Block,
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

    assert_head_to_head_or_skip(tree, root, Constraints::definite(180.0, 130.0));
}

#[test]
fn head_to_head_relative_position_offsets_visual_result_without_changing_flow() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(40.0),
        ..Style::default()
    })));
    let relative = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Relative,
        width: Length::points(10.0),
        height: Length::points(10.0),
        left: Length::points(5.0),
        top: Length::points(3.0),
        ..Style::default()
    })));
    let normal = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(10.0),
        height: Length::points(7.0),
        ..Style::default()
    })));
    tree.append_child(root, relative);
    tree.append_child(root, normal);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );
}

#[test]
fn head_to_head_relative_position_percent_offsets_use_parent_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(40.0),
        ..Style::default()
    })));
    let relative = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Relative,
        width: Length::points(20.0),
        height: Length::points(10.0),
        left: Length::percent(10.0),
        top: Length::percent(50.0),
        ..Style::default()
    })));
    let normal = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(10.0),
        height: Length::points(7.0),
        ..Style::default()
    })));
    tree.append_child(root, relative);
    tree.append_child(root, normal);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 40.0));
}

#[test]
fn head_to_head_static_position_ignores_offsets() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(100.0),
        ..Style::default()
    })));
    let static_child = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Static,
        width: Length::points(10.0),
        height: Length::points(10.0),
        left: Length::points(5.0),
        top: Length::points(3.0),
        ..Style::default()
    })));
    let normal = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(10.0),
        height: Length::points(7.0),
        ..Style::default()
    })));
    tree.append_child(root, static_child);
    tree.append_child(root, normal);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );
}

#[test]
fn head_to_head_absolute_child_can_use_right_bottom_insets() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::new(standalone_style(Style {
        position: PositionType::Absolute,
        width: Length::points(20.0),
        height: Length::points(10.0),
        right: Length::points(5.0),
        bottom: Length::points(7.0),
        ..Style::default()
    })));
    let flex_child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(15.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, absolute);
    tree.append_child(root, flex_child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 80.0));
}

#[test]
fn head_to_head_absolute_auto_width_strips_single_horizontal_inset() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(40.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            position: PositionType::Absolute,
            left: Length::points(10.0),
            ..Style::default()
        }),
        Size::new(200.0, 10.0),
    ));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 40.0));
}

#[test]
fn head_to_head_absolute_auto_height_strips_single_vertical_inset() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(80.0),
        height: Length::points(50.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            position: PositionType::Absolute,
            top: Length::points(15.0),
            ..Style::default()
        }),
        Size::new(10.0, 100.0),
    ));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(80.0, 50.0));
}

#[test]
fn head_to_head_absolute_auto_size_with_both_insets_fills_padding_box_minus_margins() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(50.0),
        padding: Rect::all(Length::points(10.0)),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
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
        }),
        Size::new(200.0, 200.0),
    ));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(120.0, 70.0));
}

fn width_mode_sensitive_height_measure(constraints: Constraints) -> Size {
    let height = if constraints.width.is_definite() {
        17.0
    } else if constraints.width.is_at_most() {
        31.0
    } else {
        43.0
    };
    Size::new(11.0, height)
}

fn height_mode_sensitive_width_measure(constraints: Constraints) -> Size {
    let width = if constraints.height.is_definite() {
        19.0
    } else if constraints.height.is_at_most() {
        29.0
    } else {
        37.0
    };
    Size::new(width, 13.0)
}

#[test]
fn head_to_head_absolute_auto_width_with_oversized_paired_insets_keeps_definite_measure_mode() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(50.0),
        height: Length::points(40.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::with_measure_func(
        block_standalone_style(Style {
            position: PositionType::Absolute,
            left: Length::points(45.0),
            right: Length::points(25.0),
            top: Length::points(3.0),
            margin: Rect::new(
                Length::points(4.0),
                Length::points(6.0),
                Length::points(2.0),
                Length::points(1.0),
            ),
            ..Style::default()
        }),
        width_mode_sensitive_height_measure,
    ));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 40.0));
}

#[test]
fn head_to_head_fixed_auto_height_with_oversized_paired_insets_keeps_definite_measure_mode() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(50.0),
        height: Length::points(40.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(12.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::with_measure_func(
        block_standalone_style(Style {
            position: PositionType::Fixed,
            left: Length::points(5.0),
            top: Length::points(35.0),
            bottom: Length::points(18.0),
            margin: Rect::new(
                Length::points(2.0),
                Length::points(1.0),
                Length::points(3.0),
                Length::points(4.0),
            ),
            ..Style::default()
        }),
        height_mode_sensitive_width_measure,
    ));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 40.0));
}

#[test]
fn head_to_head_absolute_auto_size_with_percent_and_calc_insets_fills_containing_block() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
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
        }),
        Size::new(300.0, 200.0),
    ));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_absolute_flex_child_without_insets_uses_container_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        justify_content: JustifyContent::Center,
        align_items: AlignItems::FlexEnd,
        width: Length::points(100.0),
        height: Length::points(40.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::new(standalone_style(Style {
        position: PositionType::Absolute,
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 40.0));
}

#[test]
fn head_to_head_absolute_flex_child_center_alignment_allows_negative_free_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        justify_content: JustifyContent::Center,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(40.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::new(standalone_style(Style {
        position: PositionType::Absolute,
        width: Length::points(140.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 40.0));
}

#[test]
fn head_to_head_absolute_flex_child_wrap_reverse_reverses_cross_axis_initial_position() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        flex_wrap: FlexWrap::WrapReverse,
        align_items: AlignItems::FlexEnd,
        width: Length::points(100.0),
        height: Length::points(40.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::new(standalone_style(Style {
        position: PositionType::Absolute,
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 40.0));
}

#[test]
fn head_to_head_absolute_rtl_flex_child_without_insets_uses_rtl_fronts() {
    for style in [
        Style {
            direction: Direction::Rtl,
            width: Length::points(100.0),
            height: Length::points(40.0),
            align_items: AlignItems::FlexStart,
            ..Style::default()
        },
        Style {
            direction: Direction::Rtl,
            flex_direction: FlexDirection::Column,
            width: Length::points(100.0),
            height: Length::points(40.0),
            align_items: AlignItems::FlexStart,
            ..Style::default()
        },
        Style {
            direction: Direction::Rtl,
            flex_direction: FlexDirection::Column,
            flex_wrap: FlexWrap::WrapReverse,
            width: Length::points(100.0),
            height: Length::points(40.0),
            align_items: AlignItems::FlexStart,
            ..Style::default()
        },
    ] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(standalone_style(style)));
        let absolute = tree.push(SimpleNode::new(standalone_style(Style {
            position: PositionType::Absolute,
            width: Length::points(20.0),
            height: Length::points(10.0),
            ..Style::default()
        })));
        tree.append_child(root, absolute);

        assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 40.0));
    }
}

#[test]
fn head_to_head_flex_relative_child_percent_offsets_use_container_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(40.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let relative = tree.push(SimpleNode::new(standalone_style(Style {
        position: PositionType::Relative,
        width: Length::points(20.0),
        height: Length::points(10.0),
        left: Length::percent(10.0),
        top: Length::percent(25.0),
        ..Style::default()
    })));
    tree.append_child(root, relative);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 40.0));
}

#[test]
fn head_to_head_absolute_linear_child_without_insets_uses_linear_gravity() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Center,
        width: Length::points(100.0),
        height: Length::points(40.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::new(standalone_style(Style {
        position: PositionType::Absolute,
        width: Length::points(20.0),
        height: Length::points(10.0),
        linear_layout_gravity: LinearLayoutGravity::End,
        ..Style::default()
    })));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 40.0));
}

#[test]
fn head_to_head_linear_absolute_static_position_with_margins_uses_margin_bound_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Center,
        width: Length::points(100.0),
        height: Length::points(50.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::new(standalone_style(Style {
        position: PositionType::Absolute,
        width: Length::points(10.0),
        height: Length::points(8.0),
        linear_layout_gravity: LinearLayoutGravity::End,
        margin: Rect::new(
            Length::points(3.0),
            Length::points(7.0),
            Length::points(4.0),
            Length::points(6.0),
        ),
        ..Style::default()
    })));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 50.0));
}

#[test]
fn head_to_head_linear_absolute_rtl_static_position_with_margins_uses_reversed_front() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        direction: Direction::Rtl,
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::None,
        width: Length::points(100.0),
        height: Length::points(50.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::new(standalone_style(Style {
        position: PositionType::Absolute,
        width: Length::points(10.0),
        height: Length::points(8.0),
        linear_layout_gravity: LinearLayoutGravity::Start,
        margin: Rect::new(
            Length::points(3.0),
            Length::points(7.0),
            Length::points(4.0),
            Length::points(6.0),
        ),
        ..Style::default()
    })));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 50.0));
}

#[test]
fn head_to_head_absolute_rtl_horizontal_linear_child_without_insets_uses_rtl_main_front() {
    for gravity in [
        LinearGravity::None,
        LinearGravity::Left,
        LinearGravity::Right,
        LinearGravity::Center,
    ] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
            direction: Direction::Rtl,
            linear_orientation: LinearOrientation::Horizontal,
            linear_gravity: gravity,
            width: Length::points(100.0),
            height: Length::points(40.0),
            ..Style::default()
        })));
        let absolute = tree.push(SimpleNode::new(standalone_style(Style {
            position: PositionType::Absolute,
            width: Length::points(20.0),
            height: Length::points(10.0),
            linear_layout_gravity: LinearLayoutGravity::End,
            ..Style::default()
        })));
        tree.append_child(root, absolute);

        assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 40.0));
    }
}

#[test]
fn head_to_head_linear_absolute_child_layout_gravity_overrides_align_self_and_cross_gravity() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Vertical,
        linear_gravity: LinearGravity::Center,
        align_items: AlignItems::FlexStart,
        linear_cross_gravity: LinearCrossGravity::End,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::new(standalone_style(Style {
        position: PositionType::Absolute,
        width: Length::points(20.0),
        height: Length::points(10.0),
        align_self: Some(AlignItems::FlexEnd),
        linear_layout_gravity: LinearLayoutGravity::Left,
        ..Style::default()
    })));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
}

#[test]
fn head_to_head_linear_absolute_child_cross_axis_uses_cpp_computed_layout_gravity_order() {
    {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
            linear_orientation: LinearOrientation::Horizontal,
            align_items: AlignItems::FlexStart,
            linear_cross_gravity: LinearCrossGravity::End,
            width: Length::points(100.0),
            height: Length::points(50.0),
            ..Style::default()
        })));
        let absolute = tree.push(SimpleNode::new(standalone_style(Style {
            position: PositionType::Absolute,
            align_self: Some(AlignItems::Center),
            width: Length::points(20.0),
            height: Length::points(10.0),
            ..Style::default()
        })));
        tree.append_child(root, absolute);

        assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 50.0));
    }

    {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
            linear_orientation: LinearOrientation::Horizontal,
            align_items: AlignItems::FlexStart,
            linear_cross_gravity: LinearCrossGravity::End,
            width: Length::points(100.0),
            height: Length::points(50.0),
            ..Style::default()
        })));
        let absolute = tree.push(SimpleNode::new(standalone_style(Style {
            position: PositionType::Absolute,
            width: Length::points(20.0),
            height: Length::points(10.0),
            ..Style::default()
        })));
        tree.append_child(root, absolute);

        assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 50.0));
    }

    {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
            linear_orientation: LinearOrientation::Horizontal,
            align_items: AlignItems::FlexEnd,
            linear_cross_gravity: LinearCrossGravity::None,
            width: Length::points(100.0),
            height: Length::points(50.0),
            ..Style::default()
        })));
        let absolute = tree.push(SimpleNode::new(standalone_style(Style {
            position: PositionType::Absolute,
            width: Length::points(20.0),
            height: Length::points(10.0),
            ..Style::default()
        })));
        tree.append_child(root, absolute);

        assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 50.0));
    }

    {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
            linear_orientation: LinearOrientation::Horizontal,
            align_items: AlignItems::Stretch,
            linear_cross_gravity: LinearCrossGravity::None,
            width: Length::points(100.0),
            height: Length::points(50.0),
            ..Style::default()
        })));
        let absolute = tree.push(SimpleNode::new(standalone_style(Style {
            position: PositionType::Absolute,
            width: Length::points(20.0),
            height: Length::points(10.0),
            ..Style::default()
        })));
        tree.append_child(root, absolute);

        assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 50.0));
    }
}

#[test]
fn head_to_head_linear_absolute_vertical_child_uses_cpp_main_axis_static_position() {
    {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
            linear_orientation: LinearOrientation::Vertical,
            linear_gravity: LinearGravity::Center,
            width: Length::points(50.0),
            height: Length::points(100.0),
            ..Style::default()
        })));
        let absolute = tree.push(SimpleNode::new(standalone_style(Style {
            position: PositionType::Absolute,
            width: Length::points(20.0),
            height: Length::points(10.0),
            ..Style::default()
        })));
        tree.append_child(root, absolute);

        assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 100.0));
    }

    {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
            linear_orientation: LinearOrientation::Vertical,
            linear_gravity: LinearGravity::End,
            width: Length::points(50.0),
            height: Length::points(100.0),
            ..Style::default()
        })));
        let absolute = tree.push(SimpleNode::new(standalone_style(Style {
            position: PositionType::Absolute,
            width: Length::points(20.0),
            height: Length::points(10.0),
            ..Style::default()
        })));
        tree.append_child(root, absolute);

        assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 100.0));
    }

    {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
            linear_orientation: LinearOrientation::Vertical,
            linear_gravity: LinearGravity::Bottom,
            width: Length::points(50.0),
            height: Length::points(100.0),
            ..Style::default()
        })));
        let absolute = tree.push(SimpleNode::new(standalone_style(Style {
            position: PositionType::Absolute,
            width: Length::points(20.0),
            height: Length::points(10.0),
            ..Style::default()
        })));
        tree.append_child(root, absolute);

        assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 100.0));
    }

    {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
            linear_orientation: LinearOrientation::Vertical,
            linear_gravity: LinearGravity::Top,
            width: Length::points(50.0),
            height: Length::points(100.0),
            ..Style::default()
        })));
        let absolute = tree.push(SimpleNode::new(standalone_style(Style {
            position: PositionType::Absolute,
            width: Length::points(20.0),
            height: Length::points(10.0),
            ..Style::default()
        })));
        tree.append_child(root, absolute);

        assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 100.0));
    }
}

#[test]
fn head_to_head_linear_fixed_descendant_without_insets_uses_root_linear_static_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Center,
        width: Length::points(100.0),
        height: Length::points(50.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Fixed,
        linear_layout_gravity: LinearLayoutGravity::End,
        width: Length::points(10.0),
        height: Length::points(8.0),
        ..Style::default()
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 50.0));
}

#[test]
fn head_to_head_linear_fixed_static_position_with_margins_uses_margin_bound_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Center,
        width: Length::points(100.0),
        height: Length::points(50.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
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
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 50.0));
}

#[test]
fn head_to_head_linear_fixed_rtl_static_position_with_margins_uses_reversed_front() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        direction: Direction::Rtl,
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::None,
        width: Length::points(100.0),
        height: Length::points(50.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
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
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 50.0));
}

#[test]
fn head_to_head_linear_fixed_vertical_descendant_uses_center_main_axis_static_position() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Vertical,
        linear_gravity: LinearGravity::Center,
        width: Length::points(50.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Fixed,
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 100.0));
}

#[test]
fn head_to_head_linear_fixed_vertical_descendant_uses_end_main_axis_static_position() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Vertical,
        linear_gravity: LinearGravity::End,
        width: Length::points(50.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Fixed,
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 100.0));
}

#[test]
fn head_to_head_linear_fixed_vertical_descendant_uses_physical_bottom_main_axis_static_position() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Vertical,
        linear_gravity: LinearGravity::Bottom,
        width: Length::points(50.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Fixed,
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 100.0));
}

#[test]
fn head_to_head_linear_fixed_vertical_descendant_uses_physical_top_main_axis_static_position() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Vertical,
        linear_gravity: LinearGravity::Top,
        width: Length::points(50.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Fixed,
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 100.0));
}

#[test]
fn head_to_head_linear_fixed_start_insets_override_static_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Center,
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Fixed,
        left: Length::points(12.0),
        top: Length::points(9.0),
        width: Length::points(20.0),
        height: Length::points(10.0),
        linear_layout_gravity: LinearLayoutGravity::End,
        ..Style::default()
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_linear_fixed_paired_insets_with_explicit_size_use_start_insets() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Center,
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Fixed,
        left: Length::points(12.0),
        right: Length::points(30.0),
        top: Length::points(9.0),
        bottom: Length::points(25.0),
        width: Length::points(20.0),
        height: Length::points(10.0),
        linear_layout_gravity: LinearLayoutGravity::End,
        ..Style::default()
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_linear_fixed_end_insets_override_static_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Center,
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Fixed,
        right: Length::points(30.0),
        bottom: Length::points(25.0),
        width: Length::points(20.0),
        height: Length::points(10.0),
        linear_layout_gravity: LinearLayoutGravity::End,
        ..Style::default()
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_linear_fixed_end_insets_with_margins_position_margin_box() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Center,
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
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
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_linear_fixed_percent_insets_and_size_resolve_against_root_linear_containing_block()
{
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Fixed,
        left: Length::percent(10.0),
        top: Length::percent(25.0),
        width: Length::percent(50.0),
        height: Length::percent(20.0),
        ..Style::default()
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_linear_fixed_percent_end_insets_resolve_against_root_linear_containing_block() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Fixed,
        right: Length::percent(10.0),
        bottom: Length::percent(25.0),
        width: Length::percent(50.0),
        height: Length::percent(20.0),
        ..Style::default()
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_linear_fixed_auto_size_between_insets_strips_margins() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
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
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_linear_fixed_single_insets_strip_at_most_measure_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(50.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
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
        }),
        Size::new(200.0, 100.0),
    ));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 50.0));
}

#[test]
fn head_to_head_linear_fixed_descendant_uses_linear_root_padding_box_offset() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(80.0),
        padding: Rect::all(Length::points(3.0)),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Fixed,
        width: Length::points(10.0),
        height: Length::points(10.0),
        left: Length::points(5.0),
        top: Length::points(7.0),
        ..Style::default()
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 80.0));
}

#[test]
fn head_to_head_linear_absolute_percent_insets_and_size_resolve_against_linear_containing_block() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Absolute,
        left: Length::percent(10.0),
        top: Length::percent(25.0),
        width: Length::percent(50.0),
        height: Length::percent(20.0),
        ..Style::default()
    })));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_linear_absolute_percent_end_insets_resolve_against_linear_containing_block() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Absolute,
        right: Length::percent(10.0),
        bottom: Length::percent(25.0),
        width: Length::percent(50.0),
        height: Length::percent(20.0),
        ..Style::default()
    })));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_linear_absolute_auto_size_stretches_between_start_and_end_insets() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Center,
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Absolute,
        left: Length::points(10.0),
        right: Length::points(30.0),
        top: Length::points(20.0),
        bottom: Length::points(25.0),
        ..Style::default()
    })));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_linear_absolute_auto_size_between_insets_strips_margins() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Center,
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::new(block_standalone_style(Style {
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
    })));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_linear_absolute_auto_size_paired_insets_fill_padding_box_minus_margins() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Center,
        width: Length::points(100.0),
        height: Length::points(50.0),
        padding: Rect::all(Length::points(10.0)),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
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
        }),
        Size::new(200.0, 200.0),
    ));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(120.0, 70.0));
}

#[test]
fn head_to_head_linear_absolute_single_insets_strip_at_most_measure_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Center,
        width: Length::points(100.0),
        height: Length::points(50.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
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
        }),
        Size::new(200.0, 100.0),
    ));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 50.0));
}

#[test]
fn head_to_head_linear_absolute_start_insets_override_static_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Center,
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Absolute,
        left: Length::points(12.0),
        top: Length::points(9.0),
        width: Length::points(20.0),
        height: Length::points(10.0),
        linear_layout_gravity: LinearLayoutGravity::End,
        ..Style::default()
    })));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_linear_absolute_end_insets_with_margins_position_margin_box() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Center,
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::new(block_standalone_style(Style {
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
    })));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_linear_absolute_paired_insets_with_explicit_size_use_start_insets() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Center,
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Absolute,
        left: Length::points(12.0),
        right: Length::points(30.0),
        top: Length::points(9.0),
        bottom: Length::points(25.0),
        width: Length::points(20.0),
        height: Length::points(10.0),
        linear_layout_gravity: LinearLayoutGravity::End,
        ..Style::default()
    })));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_fixed_descendant_uses_root_containing_block() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(40.0),
        height: Length::points(30.0),
        padding: Rect::all(Length::points(2.0)),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Fixed,
        width: Length::points(20.0),
        height: Length::points(10.0),
        right: Length::points(5.0),
        bottom: Length::points(7.0),
        ..Style::default()
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 80.0));
}

#[test]
fn head_to_head_fixed_descendant_percent_insets_resolve_against_root_containing_block() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Fixed,
        width: Length::points(20.0),
        height: Length::points(10.0),
        left: Length::percent(10.0),
        top: Length::percent(25.0),
        ..Style::default()
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_fixed_descendant_calc_end_insets_resolve_against_root_containing_block() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Fixed,
        width: Length::points(20.0),
        height: Length::points(10.0),
        right: Length::calc(4.0, 5.0),
        bottom: Length::calc(3.0, 10.0),
        ..Style::default()
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_fixed_descendant_measured_aspect_ratio_from_percent_width_uses_root_containing_block(
) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
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
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(44.0),
        height: Length::points(26.0),
        padding: Rect::all(Length::points(2.0)),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
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
        }),
        Size::new(72.0, 31.0),
    ));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(180.0, 130.0));
}

#[test]
fn head_to_head_fixed_descendant_uses_root_padding_box_offset() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(80.0),
        padding: Rect::all(Length::points(3.0)),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Fixed,
        width: Length::points(10.0),
        height: Length::points(10.0),
        left: Length::points(5.0),
        top: Length::points(7.0),
        ..Style::default()
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 80.0));
}

#[test]
fn head_to_head_block_stacks_children_vertically() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(100.0),
        ..Style::default()
    })));

    for height in [10.0, 20.0] {
        let child = tree.push(SimpleNode::new(block_standalone_style(Style {
            height: Length::points(height),
            margin: Rect::new(
                Length::points(2.0),
                Length::points(3.0),
                Length::points(1.0),
                Length::points(4.0),
            ),
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );
}

#[test]
fn head_to_head_negative_padding_is_clamped_to_zero_in_latest_mode() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        padding: Rect::new(
            Length::points(-10.0),
            Length::points(-2.0),
            Length::points(-3.0),
            Length::points(-4.0),
        ),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style::default()),
        Size::new(10.0, 5.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );
}

#[test]
fn head_to_head_negative_margin_is_preserved_while_padding_is_clamped() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style::default())));
    let child = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            margin: Rect::new(
                Length::points(-3.0),
                Length::ZERO,
                Length::points(-2.0),
                Length::ZERO,
            ),
            ..Style::default()
        }),
        Size::new(10.0, 5.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );
}

#[test]
fn head_to_head_vertical_percentage_padding_and_margin_use_width_percent_base() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(120.0),
        padding: Rect::all(Length::percent(10.0)),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(10.0),
        height: Length::points(5.0),
        margin: Rect::new(
            Length::ZERO,
            Length::ZERO,
            Length::percent(5.0),
            Length::percent(2.0),
        ),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(120.0),
            SideConstraint::indefinite(),
        ),
    );
}

#[test]
fn head_to_head_block_layout_orders_in_flow_children_by_order() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style::default())));
    let later = tree.push(SimpleNode::new(block_standalone_style(Style {
        height: Length::points(10.0),
        order: 1,
        ..Style::default()
    })));
    let earlier = tree.push(SimpleNode::new(block_standalone_style(Style {
        height: Length::points(10.0),
        order: -1,
        ..Style::default()
    })));
    tree.append_child(root, later);
    tree.append_child(root, earlier);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_measured_block_leaf_definite_constraints_override_measured_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style::default()),
        Size::new(10.0, 5.0),
    ));

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 30.0));
}

#[test]
fn head_to_head_measured_block_leaf_at_most_constraint_does_not_clamp_callback_result() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style::default()),
        Size::new(120.0, 5.0),
    ));

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(SideConstraint::at_most(40.0), SideConstraint::indefinite()),
    );
}

fn simple_tree_callback_measure(constraints: Constraints) -> Size {
    let width = constraints
        .width
        .bounded_size()
        .map_or(17.0, |size| (size - 3.0).max(1.0));
    let height = constraints
        .height
        .bounded_size()
        .map_or(11.0, |size| (size - 2.0).max(1.0));
    Size::new(width, height)
}

fn simple_tree_callback_baseline(content_size: Size) -> f32 {
    (content_size.height - 2.0).max(0.0)
}

#[test]
fn head_to_head_simple_tree_measure_and_baseline_callbacks() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::Baseline,
        width: Length::points(80.0),
        ..Style::default()
    })));
    let callback_child = tree.push(SimpleNode::with_measure_func_and_baseline(
        standalone_style(Style::default()),
        simple_tree_callback_measure,
        simple_tree_callback_baseline,
    ));
    let static_child = tree.push(SimpleNode::with_measured_size_and_baseline(
        standalone_style(Style::default()),
        Size::new(12.0, 8.0),
        4.0,
    ));
    tree.append_child(root, callback_child);
    tree.append_child(root, static_child);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(SideConstraint::definite(80.0), SideConstraint::indefinite()),
    );
}

#[test]
fn head_to_head_block_fit_content_measured_callback_children_respects_natural_width() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(320.0),
        height: Length::points(80.0),
        ..Style::default()
    })));
    let container = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::fit_content(Some(BaseLength::fixed(126.0))),
        height: Length::fit_content(Some(BaseLength::fixed(44.0))),
        min_width: Length::points(72.0),
        max_width: Length::points(180.0),
        min_height: Length::points(28.0),
        max_height: Length::points(92.0),
        padding: Rect::all(Length::points(1.0)),
        margin: Rect::new(
            Length::points(1.0),
            Length::ZERO,
            Length::points(1.0),
            Length::ZERO,
        ),
        ..Style::default()
    })));
    tree.append_child(root, container);

    let measured_with_baseline = tree.push(SimpleNode::with_measure_func_and_baseline(
        block_standalone_style(Style {
            width: Length::fit_content(Some(BaseLength::fixed(36.0))),
            align_self: Some(AlignItems::Baseline),
            ..Style::default()
        }),
        simple_tree_callback_measure,
        simple_tree_callback_baseline,
    ));
    let measured = tree.push(SimpleNode::with_measure_func(
        block_standalone_style(Style {
            height: Length::fit_content(Some(BaseLength::fixed(18.0))),
            min_height: Length::points(10.0),
            margin: Rect::new(
                Length::points(1.0),
                Length::points(0.5),
                Length::ZERO,
                Length::ZERO,
            ),
            ..Style::default()
        }),
        simple_tree_callback_measure,
    ));
    let static_measured_with_baseline = tree.push(SimpleNode::with_measured_size_and_baseline(
        block_standalone_style(Style {
            min_width: Length::points(20.0),
            max_height: Length::points(32.0),
            align_self: Some(AlignItems::Baseline),
            ..Style::default()
        }),
        Size::new(24.0, 13.0),
        8.0,
    ));
    let static_measured = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            max_width: Length::points(54.0),
            margin: Rect::new(
                Length::points(1.0),
                Length::ZERO,
                Length::ZERO,
                Length::ZERO,
            ),
            ..Style::default()
        }),
        Size::new(27.0, 15.0),
    ));

    tree.append_child(container, measured_with_baseline);
    tree.append_child(container, measured);
    tree.append_child(container, static_measured_with_baseline);
    tree.append_child(container, static_measured);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(320.0, 80.0));
}

#[test]
fn head_to_head_wrapped_flex_measured_callback_baseline_exports_cpp_first_line_baseline() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(136.0),
        height: Length::points(58.0),
        min_width: Length::points(72.0),
        max_width: Length::points(180.0),
        min_height: Length::points(28.0),
        max_height: Length::points(92.0),
        padding: Rect::all(Length::points(1.0)),
        border: Rect::all(0.5),
        flex_wrap: FlexWrap::Wrap,
        align_items: AlignItems::Baseline,
        justify_content: JustifyContent::FlexStart,
        align_content: AlignContent::FlexStart,
        column_gap: Length::points(1.0),
        row_gap: Length::points(1.0),
        ..Style::default()
    })));

    let measured_with_baseline = tree.push(SimpleNode::with_measure_func_and_baseline(
        block_standalone_style(Style {
            width: Length::fit_content(Some(BaseLength::fixed(36.0))),
            align_self: Some(AlignItems::Baseline),
            margin: Rect::new(
                Length::ZERO,
                Length::ZERO,
                Length::points(0.5),
                Length::ZERO,
            ),
            ..Style::default()
        }),
        simple_tree_callback_measure,
        simple_tree_callback_baseline,
    ));
    let measured = tree.push(SimpleNode::with_measure_func(
        block_standalone_style(Style {
            height: Length::fit_content(Some(BaseLength::fixed(18.0))),
            min_height: Length::points(10.0),
            margin: Rect::new(
                Length::points(1.0),
                Length::points(0.5),
                Length::points(0.5),
                Length::ZERO,
            ),
            ..Style::default()
        }),
        simple_tree_callback_measure,
    ));
    let static_measured_with_baseline = tree.push(SimpleNode::with_measured_size_and_baseline(
        block_standalone_style(Style {
            min_width: Length::points(20.0),
            max_height: Length::points(32.0),
            align_self: Some(AlignItems::Baseline),
            margin: Rect::new(
                Length::ZERO,
                Length::points(1.0),
                Length::points(0.5),
                Length::ZERO,
            ),
            ..Style::default()
        }),
        Size::new(25.0, 14.0),
        8.0,
    ));
    let static_measured = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            max_width: Length::points(54.0),
            margin: Rect::new(
                Length::points(1.0),
                Length::ZERO,
                Length::points(0.5),
                Length::ZERO,
            ),
            ..Style::default()
        }),
        Size::new(28.0, 16.0),
    ));

    tree.append_child(root, measured_with_baseline);
    tree.append_child(root, measured);
    tree.append_child(root, static_measured_with_baseline);
    tree.append_child(root, static_measured);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(320.0, 80.0));
}

#[test]
fn head_to_head_wrapped_flex_fit_content_measured_callback_container_width() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(320.0),
        height: Length::points(120.0),
        ..Style::default()
    })));
    let container = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::fit_content(Some(BaseLength::fixed(126.0))),
        height: Length::points(58.0),
        min_width: Length::points(72.0),
        max_width: Length::points(180.0),
        min_height: Length::points(28.0),
        max_height: Length::points(92.0),
        padding: Rect::all(Length::points(1.0)),
        flex_wrap: FlexWrap::Wrap,
        align_items: AlignItems::Baseline,
        justify_content: JustifyContent::FlexStart,
        align_content: AlignContent::FlexStart,
        column_gap: Length::points(1.0),
        row_gap: Length::points(1.0),
        margin: Rect::new(
            Length::points(1.0),
            Length::ZERO,
            Length::points(1.0),
            Length::ZERO,
        ),
        ..Style::default()
    })));
    tree.append_child(root, container);

    let measured_with_baseline = tree.push(SimpleNode::with_measure_func_and_baseline(
        block_standalone_style(Style {
            width: Length::fit_content(Some(BaseLength::fixed(36.0))),
            align_self: Some(AlignItems::Baseline),
            ..Style::default()
        }),
        simple_tree_callback_measure,
        simple_tree_callback_baseline,
    ));
    let measured = tree.push(SimpleNode::with_measure_func(
        block_standalone_style(Style {
            height: Length::fit_content(Some(BaseLength::fixed(18.0))),
            min_height: Length::points(10.0),
            margin: Rect::new(
                Length::points(1.0),
                Length::points(0.5),
                Length::ZERO,
                Length::ZERO,
            ),
            ..Style::default()
        }),
        simple_tree_callback_measure,
    ));
    let static_measured_with_baseline = tree.push(SimpleNode::with_measured_size_and_baseline(
        block_standalone_style(Style {
            min_width: Length::points(20.0),
            max_height: Length::points(32.0),
            align_self: Some(AlignItems::Baseline),
            margin: Rect::new(
                Length::ZERO,
                Length::points(1.0),
                Length::ZERO,
                Length::ZERO,
            ),
            ..Style::default()
        }),
        Size::new(30.0, 14.0),
        8.0,
    ));
    let static_measured = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            max_width: Length::points(54.0),
            margin: Rect::new(
                Length::points(1.0),
                Length::ZERO,
                Length::ZERO,
                Length::ZERO,
            ),
            ..Style::default()
        }),
        Size::new(33.0, 16.0),
    ));

    tree.append_child(container, measured_with_baseline);
    tree.append_child(container, measured);
    tree.append_child(container, static_measured_with_baseline);
    tree.append_child(container, static_measured);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(320.0, 80.0));
}

#[test]
fn head_to_head_measured_block_leaf_indefinite_constraint_applies_max_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            max_width: Length::points(40.0),
            ..Style::default()
        }),
        Size::new(100.0, 5.0),
    ));

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_measured_block_leaf_definite_width_applies_max_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            width: Length::points(100.0),
            max_width: Length::points(40.0),
            ..Style::default()
        }),
        Size::new(10.0, 5.0),
    ));

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(120.0),
            SideConstraint::indefinite(),
        ),
    );
}

#[test]
fn head_to_head_measured_block_leaf_definite_width_applies_min_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            width: Length::points(20.0),
            min_width: Length::points(40.0),
            ..Style::default()
        }),
        Size::new(10.0, 5.0),
    ));

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_block_child_derives_auto_height_from_width_and_aspect_ratio() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(100.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(40.0),
        aspect_ratio: Some(2.0),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );
}

#[test]
fn head_to_head_aspect_ratio_uses_content_box_before_padding_and_border() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(100.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(40.0),
        aspect_ratio: Some(2.0),
        padding: Rect::all(Length::points(5.0)),
        border: Rect::all(1.0),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );
}

#[test]
fn head_to_head_sticky_exports_insets_without_leaving_flow() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(100.0),
        ..Style::default()
    })));
    let sticky = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Sticky,
        width: Length::points(20.0),
        height: Length::points(10.0),
        left: Length::points(3.0),
        right: Length::points(4.0),
        top: Length::points(5.0),
        bottom: Length::points(6.0),
        ..Style::default()
    })));
    let normal = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(7.0),
        ..Style::default()
    })));
    tree.append_child(root, sticky);
    tree.append_child(root, normal);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );
}

#[test]
fn head_to_head_sticky_auto_insets_use_starlight_sentinel() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(80.0),
        ..Style::default()
    })));
    let sticky = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Sticky,
        width: Length::points(20.0),
        height: Length::points(10.0),
        left: Length::percent(10.0),
        top: Length::percent(25.0),
        ..Style::default()
    })));
    tree.append_child(root, sticky);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 80.0));
}

#[test]
fn head_to_head_flex_sticky_child_percent_insets_resolve_against_container_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(40.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let sticky = tree.push(SimpleNode::new(standalone_style(Style {
        position: PositionType::Sticky,
        width: Length::points(20.0),
        height: Length::points(10.0),
        left: Length::percent(10.0),
        top: Length::percent(25.0),
        ..Style::default()
    })));
    tree.append_child(root, sticky);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 40.0));
}

#[test]
fn head_to_head_flex_sticky_child_end_percent_insets_resolve_against_container_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(40.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let sticky = tree.push(SimpleNode::new(standalone_style(Style {
        position: PositionType::Sticky,
        width: Length::points(20.0),
        height: Length::points(10.0),
        right: Length::percent(20.0),
        bottom: Length::percent(50.0),
        ..Style::default()
    })));
    tree.append_child(root, sticky);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 40.0));
}

#[test]
fn head_to_head_linear_sticky_child_percent_insets_resolve_against_container_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(40.0),
        linear_orientation: LinearOrientation::Horizontal,
        ..Style::default()
    })));
    let sticky = tree.push(SimpleNode::new(linear_standalone_style(Style {
        position: PositionType::Sticky,
        width: Length::points(20.0),
        height: Length::points(10.0),
        left: Length::percent(10.0),
        top: Length::percent(25.0),
        ..Style::default()
    })));
    tree.append_child(root, sticky);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 40.0));
}

#[test]
fn head_to_head_linear_sticky_child_end_percent_insets_resolve_against_container_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(40.0),
        linear_orientation: LinearOrientation::Horizontal,
        ..Style::default()
    })));
    let sticky = tree.push(SimpleNode::new(linear_standalone_style(Style {
        position: PositionType::Sticky,
        width: Length::points(20.0),
        height: Length::points(10.0),
        right: Length::percent(20.0),
        bottom: Length::percent(50.0),
        ..Style::default()
    })));
    tree.append_child(root, sticky);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 40.0));
}

#[test]
fn head_to_head_grid_sticky_child_percent_insets_resolve_against_container_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(40.0),
        grid_template_columns: vec![Length::points(100.0)],
        grid_template_rows: vec![Length::points(40.0)],
        ..Style::default()
    })));
    let sticky = tree.push(SimpleNode::new(grid_standalone_style(Style {
        position: PositionType::Sticky,
        width: Length::points(20.0),
        height: Length::points(10.0),
        left: Length::percent(10.0),
        top: Length::percent(25.0),
        ..Style::default()
    })));
    tree.append_child(root, sticky);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 40.0));
}

#[test]
fn head_to_head_grid_sticky_child_end_percent_insets_resolve_against_container_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(40.0),
        grid_template_columns: vec![Length::points(100.0)],
        grid_template_rows: vec![Length::points(40.0)],
        ..Style::default()
    })));
    let sticky = tree.push(SimpleNode::new(grid_standalone_style(Style {
        position: PositionType::Sticky,
        width: Length::points(20.0),
        height: Length::points(10.0),
        right: Length::percent(20.0),
        bottom: Length::percent(50.0),
        ..Style::default()
    })));
    tree.append_child(root, sticky);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 40.0));
}

#[test]
fn head_to_head_relative_sticky_child_percent_insets_resolve_against_container_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(40.0),
        ..Style::default()
    })));
    let sticky = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Sticky,
        width: Length::points(20.0),
        height: Length::points(10.0),
        left: Length::percent(10.0),
        top: Length::percent(25.0),
        ..Style::default()
    })));
    tree.append_child(root, sticky);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 40.0));
}

#[test]
fn head_to_head_relative_sticky_child_end_percent_insets_resolve_against_container_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(40.0),
        ..Style::default()
    })));
    let sticky = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Sticky,
        width: Length::points(20.0),
        height: Length::points(10.0),
        right: Length::percent(20.0),
        bottom: Length::percent(50.0),
        ..Style::default()
    })));
    tree.append_child(root, sticky);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 40.0));
}

#[test]
fn head_to_head_sticky_root_percent_insets_use_owner_constraints() {
    let mut tree = SimpleTree::default();
    let sticky = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Sticky,
        left: Length::percent(10.0),
        top: Length::percent(25.0),
        ..Style::default()
    })));

    assert_head_to_head_or_skip(tree, sticky, Constraints::definite(200.0, 80.0));
}

#[test]
fn head_to_head_sticky_calc_insets_resolve_against_container_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(80.0),
        ..Style::default()
    })));
    let sticky = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Sticky,
        width: Length::points(20.0),
        height: Length::points(10.0),
        left: Length::calc(3.0, 10.0),
        right: Length::calc(4.0, 5.0),
        top: Length::calc(2.0, 25.0),
        bottom: Length::calc(1.0, 50.0),
        ..Style::default()
    })));
    tree.append_child(root, sticky);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 80.0));
}

#[test]
fn head_to_head_measured_baseline_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(80.0),
        align_items: AlignItems::Baseline,
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::with_measured_size_and_baseline(
        standalone_style(Style {
            margin: Rect::new(
                Length::points(1.0),
                Length::points(2.0),
                Length::points(3.0),
                Length::points(4.0),
            ),
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
        6.0,
    ));
    let second = tree.push(SimpleNode::with_measured_size_and_baseline(
        standalone_style(Style {
            margin: Rect::new(
                Length::points(2.0),
                Length::points(1.0),
                Length::points(1.0),
                Length::points(2.0),
            ),
            ..Style::default()
        }),
        Size::new(16.0, 14.0),
        10.0,
    ));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(SideConstraint::definite(80.0), SideConstraint::indefinite()),
    );
}

#[test]
fn head_to_head_flex_row_align_self_baseline_triggers_baseline_line_sizing() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let early_baseline = tree.push(SimpleNode::with_measured_size_and_baseline(
        standalone_style(Style {
            align_self: Some(AlignItems::Baseline),
            ..Style::default()
        }),
        Size::new(10.0, 20.0),
        5.0,
    ));
    let late_baseline = tree.push(SimpleNode::with_measured_size_and_baseline(
        standalone_style(Style {
            align_self: Some(AlignItems::Baseline),
            ..Style::default()
        }),
        Size::new(10.0, 30.0),
        25.0,
    ));
    tree.append_child(root, early_baseline);
    tree.append_child(root, late_baseline);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_flex_row_baseline_uses_nested_flex_container_baseline() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::Baseline,
        ..Style::default()
    })));
    let reference = tree.push(SimpleNode::with_measured_size_and_baseline(
        standalone_style(Style::default()),
        Size::new(10.0, 30.0),
        25.0,
    ));
    let nested = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::Baseline,
        ..Style::default()
    })));
    let nested_early = tree.push(SimpleNode::with_measured_size_and_baseline(
        standalone_style(Style::default()),
        Size::new(10.0, 20.0),
        5.0,
    ));
    let nested_late = tree.push(SimpleNode::with_measured_size_and_baseline(
        standalone_style(Style::default()),
        Size::new(10.0, 30.0),
        15.0,
    ));
    tree.append_child(nested, nested_early);
    tree.append_child(nested, nested_late);
    tree.append_child(root, reference);
    tree.append_child(root, nested);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_flex_row_baseline_uses_nested_linear_container_baseline() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::Baseline,
        ..Style::default()
    })));
    let reference = tree.push(SimpleNode::with_measured_size_and_baseline(
        standalone_style(Style::default()),
        Size::new(10.0, 40.0),
        35.0,
    ));
    let nested = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        ..Style::default()
    })));
    let nested_early = tree.push(SimpleNode::with_measured_size_and_baseline(
        standalone_style(Style::default()),
        Size::new(10.0, 20.0),
        5.0,
    ));
    let nested_late = tree.push(SimpleNode::with_measured_size_and_baseline(
        standalone_style(Style::default()),
        Size::new(10.0, 30.0),
        25.0,
    ));
    tree.append_child(nested, nested_early);
    tree.append_child(nested, nested_late);
    tree.append_child(root, reference);
    tree.append_child(root, nested);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_flex_row_baseline_uses_nested_grid_container_baseline() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::Baseline,
        ..Style::default()
    })));
    let reference = tree.push(SimpleNode::with_measured_size_and_baseline(
        standalone_style(Style::default()),
        Size::new(10.0, 30.0),
        25.0,
    ));
    let nested = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_rows: vec![Length::points(10.0)],
        align_items: AlignItems::Baseline,
        ..Style::default()
    })));
    let nested_child = tree.push(SimpleNode::with_measured_size_and_baseline(
        standalone_style(Style {
            width: Length::MaxContent,
            height: Length::MaxContent,
            ..Style::default()
        }),
        Size::new(8.0, 6.0),
        4.0,
    ));
    tree.append_child(nested, nested_child);
    tree.append_child(root, reference);
    tree.append_child(root, nested);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_flex_row_baseline_can_expand_auto_cross_size_for_bottom_margin() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(100.0),
        align_items: AlignItems::Baseline,
        ..Style::default()
    })));
    let tall = fixed_flex_child(&mut tree, 10.0, 20.0);
    let bottom_heavy = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(10.0),
        height: Length::points(10.0),
        margin: Rect::new(
            Length::ZERO,
            Length::ZERO,
            Length::ZERO,
            Length::points(100.0),
        ),
        ..Style::default()
    })));
    tree.append_child(root, tall);
    tree.append_child(root, bottom_heavy);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );
}

#[test]
fn head_to_head_flex_cross_size_hypothetical_cross_layout_uses_used_main_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(100.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let measured = tree.push(SimpleNode::with_measure_func(
        block_standalone_style(Style {
            flex_basis: Length::points(20.0),
            flex_grow: 1.0,
            ..Style::default()
        }),
        measure_height_from_width,
    ));
    tree.append_child(root, measured);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );
}

#[test]
fn head_to_head_flex_cross_size_baseline_line_size_uses_largest_baseline_distances() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::Baseline,
        width: Length::points(60.0),
        ..Style::default()
    })));
    let high_baseline = tree.push(SimpleNode::with_measured_size_and_baseline(
        block_standalone_style(Style::default()),
        Size::new(20.0, 10.0),
        8.0,
    ));
    let deep_descent = tree.push(SimpleNode::with_measured_size_and_baseline(
        block_standalone_style(Style::default()),
        Size::new(20.0, 20.0),
        4.0,
    ));
    tree.append_child(root, high_baseline);
    tree.append_child(root, deep_descent);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(SideConstraint::definite(60.0), SideConstraint::indefinite()),
    );
}

#[test]
fn head_to_head_nested_column_flex() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(100.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let column = tree.push(SimpleNode::new(standalone_style(Style {
        flex_direction: FlexDirection::Column,
        flex_basis: Length::points(30.0),
        row_gap: Length::points(2.0),
        ..Style::default()
    })));
    let leaf_a = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(6.0),
        ..Style::default()
    })));
    let leaf_b = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(18.0),
        height: Length::points(8.0),
        ..Style::default()
    })));
    tree.append_child(root, column);
    tree.append_child(column, leaf_a);
    tree.append_child(column, leaf_b);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );
}

#[test]
fn head_to_head_flex_item_derives_cross_size_from_main_size_and_aspect_ratio() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(40.0),
        aspect_ratio: Some(2.0),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );
}

#[test]
fn head_to_head_vertical_linear_stacks_measured_children() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        width: Length::points(80.0),
        padding: Rect::all(Length::points(2.0)),
        align_items: AlignItems::FlexStart,
        justify_content: JustifyContent::Center,
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::with_measured_size(
        linear_standalone_style(Style {
            margin: Rect::new(
                Length::points(1.0),
                Length::points(2.0),
                Length::points(3.0),
                Length::points(4.0),
            ),
            ..Style::default()
        }),
        Size::new(20.0, 6.0),
    ));
    let second = tree.push(SimpleNode::new(linear_standalone_style(Style {
        width: Length::points(18.0),
        height: Length::points(8.0),
        margin: Rect::new(
            Length::points(2.0),
            Length::points(1.0),
            Length::points(1.0),
            Length::points(2.0),
        ),
        ..Style::default()
    })));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(80.0),
            SideConstraint::at_most(40.0),
        ),
    );
}

#[test]
fn head_to_head_vertical_linear_stacks_children_and_stretches_cross_axis() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        width: Length::points(100.0),
        ..Style::default()
    })));
    let first = fixed_linear_child(&mut tree, Length::Auto, Length::points(10.0));
    let second = fixed_linear_child(&mut tree, Length::Auto, Length::points(20.0));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );
}

#[test]
fn head_to_head_linear_visibility_hidden_and_collapse_participate_in_layout() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        width: Length::points(100.0),
        ..Style::default()
    })));
    let hidden = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::Auto,
        height: Length::points(10.0),
        visibility: Visibility::Hidden,
        ..Style::default()
    })));
    let collapsed = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::Auto,
        height: Length::points(20.0),
        visibility: Visibility::Collapse,
        ..Style::default()
    })));
    tree.append_child(root, hidden);
    tree.append_child(root, collapsed);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );
}

#[test]
fn head_to_head_display_none_child_is_laid_out_as_zero_and_skipped_by_linear_stack() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        width: Length::points(100.0),
        ..Style::default()
    })));
    let first = fixed_linear_child(&mut tree, Length::Auto, Length::points(10.0));
    let hidden = tree.push(SimpleNode::new(Style {
        display: Display::None,
        box_sizing: BoxSizing::ContentBox,
        width: Length::points(100.0),
        height: Length::points(50.0),
        ..Style::default()
    }));
    let second = fixed_linear_child(&mut tree, Length::Auto, Length::points(20.0));
    tree.append_child(root, first);
    tree.append_child(root, hidden);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );
}

#[test]
fn head_to_head_linear_layout_orders_in_flow_children_by_order() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style::default())));
    let later = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(10.0),
        height: Length::points(10.0),
        order: 1,
        ..Style::default()
    })));
    let earlier = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(10.0),
        height: Length::points(10.0),
        order: -1,
        ..Style::default()
    })));
    tree.append_child(root, later);
    tree.append_child(root, earlier);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_horizontal_linear_at_most_main_axis_shrink_wraps_content() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        height: Length::points(20.0),
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(10.0),
        height: Length::Auto,
        ..Style::default()
    })));
    let second = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::Auto,
        ..Style::default()
    })));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::at_most(100.0),
            SideConstraint::definite(20.0),
        ),
    );
}

#[test]
fn head_to_head_horizontal_linear_at_most_main_axis_keeps_overflow_content_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        height: Length::points(20.0),
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(80.0),
        height: Length::Auto,
        ..Style::default()
    })));
    let second = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(70.0),
        height: Length::Auto,
        ..Style::default()
    })));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::at_most(100.0),
            SideConstraint::definite(20.0),
        ),
    );
}

#[test]
fn head_to_head_horizontal_linear_container_min_width_and_max_height_clamp_wrap_content_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        min_width: Length::points(40.0),
        max_height: Length::points(25.0),
        ..Style::default()
    })));
    let child = fixed_linear_child(&mut tree, Length::points(20.0), Length::points(30.0));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_vertical_linear_container_max_width_and_min_height_clamp_wrap_content_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        max_width: Length::points(60.0),
        min_height: Length::points(40.0),
        ..Style::default()
    })));
    let child = fixed_linear_child(&mut tree, Length::points(100.0), Length::points(10.0));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_linear_container_padding_border_prevents_negative_content_size_under_tight_constraints(
) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        padding: Rect::new(
            Length::points(10.0),
            Length::points(15.0),
            Length::points(8.0),
            Length::points(9.0),
        ),
        border: Rect::new(2.0, 3.0, 1.0, 4.0),
        ..Style::default()
    })));

    assert_head_to_head_or_skip(tree, root, Constraints::definite(8.0, 7.0));
}

#[test]
fn head_to_head_horizontal_linear_auto_main_axis_keeps_initial_size_after_percent_main_margins_resolve(
) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        height: Length::points(10.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(10.0),
        margin: Rect::new(
            Length::percent(10.0),
            Length::percent(10.0),
            Length::ZERO,
            Length::ZERO,
        ),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_vertical_linear_auto_main_axis_keeps_initial_size_after_percent_main_margins_resolve(
) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style::default())));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(100.0),
        margin: Rect::new(
            Length::ZERO,
            Length::ZERO,
            Length::percent(10.0),
            Length::percent(10.0),
        ),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_horizontal_linear_at_most_main_axis_does_not_enable_linear_weight() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        height: Length::points(20.0),
        ..Style::default()
    })));
    let weighted = tree.push(SimpleNode::new(block_standalone_style(Style {
        linear_weight: 1.0,
        ..Style::default()
    })));
    tree.append_child(root, weighted);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::at_most(100.0),
            SideConstraint::definite(20.0),
        ),
    );
}

#[test]
fn head_to_head_vertical_linear_at_most_cross_axis_does_not_stretch_auto_child() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style::default())));
    let child = fixed_linear_child(&mut tree, Length::Auto, Length::points(10.0));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(SideConstraint::at_most(100.0), SideConstraint::indefinite()),
    );
}

#[test]
fn head_to_head_vertical_linear_at_most_cross_axis_min_width_growth_does_not_final_stretch_auto_child(
) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        direction: Direction::Rtl,
        min_width: Length::points(20.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::Auto,
        height: Length::percent(40.0),
        min_width: Length::points(12.0),
        ..Style::default()
    })));
    let wider_sibling = fixed_linear_child(&mut tree, Length::points(14.0), Length::points(1.0));
    tree.append_child(root, child);
    tree.append_child(root, wider_sibling);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(SideConstraint::at_most(100.0), SideConstraint::indefinite()),
    );
}

#[test]
fn head_to_head_vertical_linear_center_child_receives_bounded_cross_axis_measure_constraint() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(linear_standalone_style(Style {
        align_items: AlignItems::Center,
        width: Length::points(100.0),
        ..Style::default()
    })));
    let child = tree.push(MeasuringNode::measured(
        block_standalone_style(Style::default()),
        Size::new(150.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_measuring_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_vertical_linear_at_most_cross_axis_passes_bound_to_measured_child() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(
        linear_standalone_style(Style::default()),
    ));
    let child = tree.push(MeasuringNode::measured(
        block_standalone_style(Style::default()),
        Size::new(150.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_measuring_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(SideConstraint::at_most(100.0), SideConstraint::indefinite()),
    );
}

#[test]
fn head_to_head_horizontal_linear_auto_cross_axis_passes_parent_height_constraint_to_measured_child(
) {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(100.0),
        height: Length::Auto,
        ..Style::default()
    })));
    let child = tree.push(MeasuringNode::measured(
        block_standalone_style(Style {
            width: Length::points(10.0),
            ..Style::default()
        }),
        Size::new(10.0, 150.0),
    ));
    tree.append_child(root, child);

    assert_measuring_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 80.0));
}

#[test]
fn head_to_head_horizontal_linear_fit_content_cross_axis_argument_bounds_measured_child() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(100.0),
        height: Length::fit_content(Some(BaseLength::fixed(30.0))),
        ..Style::default()
    })));
    let child = tree.push(MeasuringNode::measured(
        block_standalone_style(Style::default()),
        Size::new(20.0, 50.0),
    ));
    tree.append_child(root, child);

    assert_measuring_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );
}

#[test]
fn head_to_head_vertical_linear_indefinite_cross_axis_keeps_narrow_measured_child() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(
        linear_standalone_style(Style::default()),
    ));
    let wide = tree.push(MeasuringNode::measured(
        block_standalone_style(Style::default()),
        Size::new(50.0, 10.0),
    ));
    let narrow = tree.push(MeasuringNode::measured(
        block_standalone_style(Style::default()),
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, wide);
    tree.append_child(root, narrow);

    assert_measuring_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_vertical_linear_default_stretch_does_not_override_max_content_cross_size() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(linear_standalone_style(Style {
        width: Length::points(100.0),
        ..Style::default()
    })));
    let child = tree.push(MeasuringNode::measured(
        block_standalone_style(Style {
            width: Length::MaxContent,
            ..Style::default()
        }),
        Size::new(150.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_measuring_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_horizontal_linear_splits_weighted_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(90.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style {
        linear_weight: 1.0,
        ..Style::default()
    })));
    let second = tree.push(SimpleNode::new(standalone_style(Style {
        linear_weight: 2.0,
        ..Style::default()
    })));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(90.0, 20.0));
}

#[test]
fn head_to_head_vertical_linear_gravity_packs_items_at_bottom() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_gravity: LinearGravity::Bottom,
        width: Length::points(20.0),
        height: Length::points(100.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(10.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let second = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(10.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(20.0, 100.0));
}

#[test]
fn head_to_head_vertical_linear_gravity_variants_match_cpp_mapping() {
    for gravity in [
        LinearGravity::None,
        LinearGravity::Top,
        LinearGravity::Bottom,
        LinearGravity::Left,
        LinearGravity::Right,
        LinearGravity::CenterVertical,
        LinearGravity::CenterHorizontal,
        LinearGravity::SpaceBetween,
        LinearGravity::Start,
        LinearGravity::End,
        LinearGravity::Center,
    ] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
            linear_gravity: gravity,
            width: Length::points(30.0),
            height: Length::points(100.0),
            ..Style::default()
        })));
        for height in [10.0, 20.0] {
            let child = fixed_linear_child(&mut tree, Length::points(10.0), Length::points(height));
            tree.append_child(root, child);
        }

        assert_head_to_head_or_skip(tree, root, Constraints::definite(30.0, 100.0));
    }
}

#[test]
fn head_to_head_horizontal_linear_center_uses_remaining_main_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        justify_content: JustifyContent::Center,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    for width in [10.0, 20.0] {
        let child = fixed_linear_child(&mut tree, Length::points(width), Length::Auto);
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_horizontal_linear_auto_cross_axis_uses_parent_height_constraint_for_stretch() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(100.0),
        height: Length::Auto,
        ..Style::default()
    })));
    let child = fixed_linear_child(&mut tree, Length::points(10.0), Length::Auto);
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 80.0));
}

#[test]
fn head_to_head_horizontal_linear_center_uses_negative_remaining_main_space_when_overflowing() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        justify_content: JustifyContent::Center,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    for width in [80.0, 70.0] {
        let child = fixed_linear_child(&mut tree, Length::points(width), Length::Auto);
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_vertical_linear_center_uses_negative_remaining_main_space_for_container_baseline() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        justify_content: JustifyContent::Center,
        width: Length::points(20.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::with_measured_size_and_baseline(
        block_standalone_style(Style::default()),
        Size::new(10.0, 80.0),
        10.0,
    ));
    let second = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style::default()),
        Size::new(10.0, 70.0),
    ));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(20.0, 100.0));
}

#[test]
fn head_to_head_vertical_linear_end_gravity_offsets_container_baseline_by_remaining_main_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_gravity: LinearGravity::End,
        width: Length::points(20.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::with_measured_size_and_baseline(
        block_standalone_style(Style::default()),
        Size::new(10.0, 20.0),
        5.0,
    ));
    let second = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style::default()),
        Size::new(10.0, 10.0),
    ));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(20.0, 100.0));
}

#[test]
fn head_to_head_horizontal_linear_empty_container_exports_no_baseline() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    })));

    assert_head_to_head_or_skip(tree, root, Constraints::definite(20.0, 10.0));
}

#[test]
fn head_to_head_vertical_linear_empty_container_exports_no_baseline() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    })));

    assert_head_to_head_or_skip(tree, root, Constraints::definite(20.0, 10.0));
}

#[test]
fn head_to_head_horizontal_linear_child_without_baseline_exports_fallback_baseline() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        ..Style::default()
    })));
    let child = fixed_linear_child(&mut tree, Length::points(20.0), Length::points(10.0));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_horizontal_linear_container_baseline_uses_largest_child_baseline() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(100.0),
        height: Length::points(40.0),
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::with_measured_size_and_baseline(
        standalone_style(Style::default()),
        Size::new(10.0, 30.0),
        5.0,
    ));
    let second = tree.push(SimpleNode::with_measured_size_and_baseline(
        standalone_style(Style::default()),
        Size::new(10.0, 20.0),
        15.0,
    ));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 40.0));
}

#[test]
fn head_to_head_vertical_linear_child_without_baseline_exports_fallback_baseline() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style::default())));
    let child = fixed_linear_child(&mut tree, Length::points(20.0), Length::points(10.0));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_rtl_horizontal_linear_positions_items_from_right_edge() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        direction: Direction::Rtl,
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    for width in [10.0, 20.0] {
        let child = fixed_linear_child(&mut tree, Length::points(width), Length::Auto);
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_rtl_horizontal_linear_gravity_swaps_physical_edges() {
    for gravity in [LinearGravity::Left, LinearGravity::Right] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
            direction: Direction::Rtl,
            linear_orientation: LinearOrientation::Horizontal,
            linear_gravity: gravity,
            width: Length::points(100.0),
            height: Length::points(20.0),
            ..Style::default()
        })));
        let child = fixed_linear_child(&mut tree, Length::points(20.0), Length::points(10.0));
        tree.append_child(root, child);

        assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
    }
}

#[test]
fn head_to_head_horizontal_reverse_linear_positions_items_from_right_edge() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::HorizontalReverse,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    for width in [10.0, 20.0] {
        let child = fixed_linear_child(&mut tree, Length::points(width), Length::Auto);
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_rtl_horizontal_reverse_linear_positions_items_from_left_edge() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        direction: Direction::Rtl,
        linear_orientation: LinearOrientation::HorizontalReverse,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    for width in [10.0, 20.0] {
        let child = fixed_linear_child(&mut tree, Length::points(width), Length::Auto);
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_linear_row_column_orientation_aliases_match_cpp_mapping() {
    for (orientation, is_row, reverse) in [
        (LinearOrientation::Row, true, false),
        (LinearOrientation::RowReverse, true, true),
        (LinearOrientation::Column, false, false),
        (LinearOrientation::ColumnReverse, false, true),
    ] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
            linear_orientation: orientation,
            justify_content: JustifyContent::Center,
            width: Length::points(100.0),
            height: Length::points(80.0),
            padding: Rect::new(
                Length::points(3.0),
                Length::points(5.0),
                Length::points(7.0),
                Length::points(11.0),
            ),
            ..Style::default()
        })));
        for main_size in if reverse { [20.0, 10.0] } else { [10.0, 20.0] } {
            let (width, height) = if is_row {
                (Length::points(main_size), Length::points(12.0))
            } else {
                (Length::points(12.0), Length::points(main_size))
            };
            let child = fixed_linear_child(&mut tree, width, height);
            tree.append_child(root, child);
        }

        assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 80.0));
    }
}

#[test]
fn head_to_head_linear_main_axis_orientation_direction_reverse_matrix() {
    let orientations = [
        LinearOrientation::Horizontal,
        LinearOrientation::HorizontalReverse,
        LinearOrientation::Vertical,
        LinearOrientation::VerticalReverse,
    ];
    let directions = [Direction::Ltr, Direction::Rtl];
    let justify_values = [
        JustifyContent::Stretch,
        JustifyContent::FlexStart,
        JustifyContent::Start,
        JustifyContent::Center,
        JustifyContent::FlexEnd,
        JustifyContent::End,
        JustifyContent::SpaceBetween,
        JustifyContent::SpaceAround,
        JustifyContent::SpaceEvenly,
    ];
    let gravity_values = [
        LinearGravity::None,
        LinearGravity::Top,
        LinearGravity::Bottom,
        LinearGravity::Left,
        LinearGravity::Right,
        LinearGravity::CenterVertical,
        LinearGravity::CenterHorizontal,
        LinearGravity::SpaceBetween,
        LinearGravity::Start,
        LinearGravity::End,
        LinearGravity::Center,
    ];

    for orientation in orientations {
        for direction in directions {
            for justify_content in justify_values {
                let mut tree = SimpleTree::default();
                let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
                    direction,
                    linear_orientation: orientation,
                    justify_content,
                    align_items: AlignItems::FlexStart,
                    width: Length::points(100.0),
                    height: Length::points(80.0),
                    ..Style::default()
                })));
                for main_size in [10.0, 20.0] {
                    let (width, height) = if orientation.is_row() {
                        (Length::points(main_size), Length::points(12.0))
                    } else {
                        (Length::points(12.0), Length::points(main_size))
                    };
                    let child = fixed_linear_child(&mut tree, width, height);
                    tree.append_child(root, child);
                }

                assert_head_to_head_or_skip_with_name(
                    &format!("{orientation:?} {direction:?} {justify_content:?}"),
                    tree,
                    root,
                    Constraints::definite(100.0, 80.0),
                );
            }

            for linear_gravity in gravity_values {
                let mut tree = SimpleTree::default();
                let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
                    direction,
                    linear_orientation: orientation,
                    linear_gravity,
                    align_items: AlignItems::FlexStart,
                    width: Length::points(100.0),
                    height: Length::points(80.0),
                    ..Style::default()
                })));
                for main_size in [10.0, 20.0] {
                    let (width, height) = if orientation.is_row() {
                        (Length::points(main_size), Length::points(12.0))
                    } else {
                        (Length::points(12.0), Length::points(main_size))
                    };
                    let child = fixed_linear_child(&mut tree, width, height);
                    tree.append_child(root, child);
                }

                assert_head_to_head_or_skip_with_name(
                    &format!("{orientation:?} {direction:?} {linear_gravity:?}"),
                    tree,
                    root,
                    Constraints::definite(100.0, 80.0),
                );
            }
        }
    }
}

#[test]
fn head_to_head_linear_cross_axis_orientation_direction_reverse_matrix() {
    let orientations = [
        LinearOrientation::Horizontal,
        LinearOrientation::HorizontalReverse,
        LinearOrientation::Vertical,
        LinearOrientation::VerticalReverse,
    ];
    let directions = [Direction::Ltr, Direction::Rtl];
    let align_values = [
        AlignItems::FlexStart,
        AlignItems::FlexEnd,
        AlignItems::Center,
        AlignItems::Stretch,
    ];
    let layout_gravity_values = [
        LinearLayoutGravity::None,
        LinearLayoutGravity::Top,
        LinearLayoutGravity::Bottom,
        LinearLayoutGravity::Left,
        LinearLayoutGravity::Right,
        LinearLayoutGravity::CenterVertical,
        LinearLayoutGravity::CenterHorizontal,
        LinearLayoutGravity::FillVertical,
        LinearLayoutGravity::FillHorizontal,
        LinearLayoutGravity::Center,
        LinearLayoutGravity::Stretch,
        LinearLayoutGravity::Start,
        LinearLayoutGravity::End,
    ];

    for orientation in orientations {
        for direction in directions {
            for align_items in align_values {
                let mut tree = SimpleTree::default();
                let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
                    direction,
                    linear_orientation: orientation,
                    align_items,
                    width: Length::points(100.0),
                    height: Length::points(80.0),
                    ..Style::default()
                })));
                let child =
                    fixed_linear_child(&mut tree, Length::points(20.0), Length::points(10.0));
                tree.append_child(root, child);

                assert_head_to_head_or_skip_with_name(
                    &format!("{orientation:?} {direction:?} {align_items:?}"),
                    tree,
                    root,
                    Constraints::definite(100.0, 80.0),
                );
            }

            for linear_layout_gravity in layout_gravity_values {
                let mut tree = SimpleTree::default();
                let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
                    direction,
                    linear_orientation: orientation,
                    align_items: AlignItems::FlexStart,
                    width: Length::points(100.0),
                    height: Length::points(80.0),
                    ..Style::default()
                })));
                let child = tree.push(SimpleNode::new(block_standalone_style(Style {
                    width: Length::points(20.0),
                    height: Length::points(10.0),
                    linear_layout_gravity,
                    ..Style::default()
                })));
                tree.append_child(root, child);

                assert_head_to_head_or_skip_with_name(
                    &format!("{orientation:?} {direction:?} {linear_layout_gravity:?}"),
                    tree,
                    root,
                    Constraints::definite(100.0, 80.0),
                );
            }
        }
    }
}

#[test]
fn head_to_head_horizontal_reverse_linear_gravity_left_packs_items_at_left_edge() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::HorizontalReverse,
        linear_gravity: LinearGravity::Left,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    for width in [10.0, 20.0] {
        let child = fixed_linear_child(&mut tree, Length::points(width), Length::Auto);
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_vertical_reverse_linear_positions_items_from_bottom_edge() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::VerticalReverse,
        width: Length::points(20.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    for height in [10.0, 20.0] {
        let child = fixed_linear_child(&mut tree, Length::Auto, Length::points(height));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(20.0, 100.0));
}

#[test]
fn head_to_head_vertical_reverse_linear_gravity_top_packs_items_at_top_edge() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::VerticalReverse,
        linear_gravity: LinearGravity::Top,
        width: Length::points(20.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    for height in [10.0, 20.0] {
        let child = fixed_linear_child(&mut tree, Length::Auto, Length::points(height));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(20.0, 100.0));
}

#[test]
fn head_to_head_vertical_linear_space_between_distributes_remaining_main_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        justify_content: JustifyContent::SpaceBetween,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = fixed_linear_child(&mut tree, Length::Auto, Length::points(10.0));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
}

#[test]
fn head_to_head_vertical_linear_space_between_single_item_uses_start_position() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        justify_content: JustifyContent::SpaceBetween,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let child = fixed_linear_child(&mut tree, Length::Auto, Length::points(10.0));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
}

#[test]
fn head_to_head_vertical_linear_space_between_keeps_items_adjacent_when_overflowing() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        justify_content: JustifyContent::SpaceBetween,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = fixed_linear_child(&mut tree, Length::Auto, Length::points(70.0));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
}

#[test]
fn head_to_head_horizontal_linear_gravity_right_overrides_justify_content() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        linear_gravity: LinearGravity::Right,
        justify_content: JustifyContent::FlexStart,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    for width in [10.0, 20.0] {
        let child = fixed_linear_child(&mut tree, Length::points(width), Length::Auto);
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_horizontal_linear_justify_content_distribution_values_map_to_start() {
    for justify_content in [
        JustifyContent::SpaceAround,
        JustifyContent::SpaceEvenly,
        JustifyContent::Stretch,
    ] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
            linear_orientation: LinearOrientation::Horizontal,
            justify_content,
            width: Length::points(100.0),
            height: Length::points(10.0),
            ..Style::default()
        })));
        for width in [10.0, 20.0] {
            let child = fixed_linear_child(&mut tree, Length::points(width), Length::Auto);
            tree.append_child(root, child);
        }

        assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
    }
}

#[test]
fn head_to_head_rtl_horizontal_linear_gravity_physical_left_and_right() {
    for gravity in [LinearGravity::Left, LinearGravity::Right] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
            direction: Direction::Rtl,
            linear_orientation: LinearOrientation::Horizontal,
            linear_gravity: gravity,
            width: Length::points(100.0),
            height: Length::points(20.0),
            ..Style::default()
        })));
        for width in [10.0, 20.0] {
            let child = fixed_linear_child(&mut tree, Length::points(width), Length::Auto);
            tree.append_child(root, child);
        }

        assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
    }
}

#[test]
fn head_to_head_linear_cross_axis_alignment_uses_align_items() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        align_items: AlignItems::Center,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let child = fixed_linear_child(&mut tree, Length::points(20.0), Length::points(10.0));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
}

#[test]
fn head_to_head_linear_cross_axis_center_uses_negative_space_when_item_overflows() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        align_items: AlignItems::Center,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let child = fixed_linear_child(&mut tree, Length::points(140.0), Length::points(10.0));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
}

#[test]
fn head_to_head_linear_cross_axis_end_uses_negative_space_when_item_overflows() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        align_items: AlignItems::FlexEnd,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let child = fixed_linear_child(&mut tree, Length::points(140.0), Length::points(10.0));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
}

#[test]
fn head_to_head_linear_baseline_align_items_keeps_default_cross_axis_stretch() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        align_items: AlignItems::Baseline,
        width: Length::points(100.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::Auto,
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
}

#[test]
fn head_to_head_linear_align_self_overrides_container_align_items() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        align_self: Some(AlignItems::FlexEnd),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
}

#[test]
fn head_to_head_linear_align_self_overrides_linear_cross_gravity() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        align_items: AlignItems::FlexStart,
        linear_cross_gravity: LinearCrossGravity::End,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        align_self: Some(AlignItems::Center),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
}

#[test]
fn head_to_head_vertical_linear_percent_cross_size_remeasures_final_constraint() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(40.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::percent(50.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 40.0));
}

#[test]
fn head_to_head_horizontal_linear_percent_cross_size_with_stretch_remeasures_final_constraint() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::percent(50.0),
        linear_layout_gravity: LinearLayoutGravity::Stretch,
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 80.0));
}

#[test]
fn head_to_head_linear_layout_gravity_end_overrides_container_stretch() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        linear_layout_gravity: LinearLayoutGravity::End,
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
}

#[test]
fn head_to_head_linear_layout_gravity_overrides_align_self_and_cross_gravity() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        align_items: AlignItems::FlexStart,
        linear_cross_gravity: LinearCrossGravity::End,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        align_self: Some(AlignItems::FlexEnd),
        linear_layout_gravity: LinearLayoutGravity::Left,
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
}

#[test]
fn head_to_head_linear_align_items_stretch_is_not_used_as_linear_layout_gravity_fallback() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        align_items: AlignItems::Stretch,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
}

#[test]
fn head_to_head_linear_layout_gravity_stretch_overrides_explicit_cross_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        linear_layout_gravity: LinearLayoutGravity::Stretch,
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
}

#[test]
fn head_to_head_linear_layout_gravity_stretch_overrides_weighted_explicit_cross_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(20.0),
        linear_weight: 1.0,
        linear_layout_gravity: LinearLayoutGravity::Stretch,
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
}

#[test]
fn head_to_head_linear_layout_gravity_physical_variants_match_cpp_groups() {
    for gravity in [
        LinearLayoutGravity::None,
        LinearLayoutGravity::Top,
        LinearLayoutGravity::Left,
        LinearLayoutGravity::Start,
        LinearLayoutGravity::Right,
        LinearLayoutGravity::Bottom,
        LinearLayoutGravity::End,
        LinearLayoutGravity::CenterHorizontal,
        LinearLayoutGravity::CenterVertical,
        LinearLayoutGravity::Center,
        LinearLayoutGravity::FillHorizontal,
        LinearLayoutGravity::FillVertical,
        LinearLayoutGravity::Stretch,
    ] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
            width: Length::points(100.0),
            height: Length::points(100.0),
            ..Style::default()
        })));
        let child = tree.push(SimpleNode::new(block_standalone_style(Style {
            width: Length::points(20.0),
            height: Length::points(10.0),
            linear_layout_gravity: gravity,
            ..Style::default()
        })));
        tree.append_child(root, child);

        assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
    }
}

#[test]
fn head_to_head_rtl_vertical_linear_layout_gravity_keeps_physical_left_and_right() {
    for gravity in [LinearLayoutGravity::Left, LinearLayoutGravity::Right] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
            direction: Direction::Rtl,
            width: Length::points(100.0),
            height: Length::points(100.0),
            ..Style::default()
        })));
        let child = tree.push(SimpleNode::new(block_standalone_style(Style {
            width: Length::points(20.0),
            height: Length::points(10.0),
            linear_layout_gravity: gravity,
            ..Style::default()
        })));
        tree.append_child(root, child);

        assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
    }
}

#[test]
fn head_to_head_horizontal_linear_layout_gravity_physical_variants_match_cpp_groups() {
    for gravity in [
        LinearLayoutGravity::None,
        LinearLayoutGravity::Top,
        LinearLayoutGravity::Left,
        LinearLayoutGravity::Start,
        LinearLayoutGravity::Right,
        LinearLayoutGravity::Bottom,
        LinearLayoutGravity::End,
        LinearLayoutGravity::CenterHorizontal,
        LinearLayoutGravity::CenterVertical,
        LinearLayoutGravity::Center,
        LinearLayoutGravity::FillHorizontal,
        LinearLayoutGravity::FillVertical,
        LinearLayoutGravity::Stretch,
    ] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
            linear_orientation: LinearOrientation::Horizontal,
            width: Length::points(100.0),
            height: Length::points(100.0),
            ..Style::default()
        })));
        let child = tree.push(SimpleNode::new(block_standalone_style(Style {
            width: Length::points(20.0),
            height: Length::points(10.0),
            linear_layout_gravity: gravity,
            ..Style::default()
        })));
        tree.append_child(root, child);

        assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
    }
}

#[test]
fn head_to_head_linear_cross_gravity_center_aligns_children() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_cross_gravity: LinearCrossGravity::Center,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let child = fixed_linear_child(&mut tree, Length::points(20.0), Length::points(10.0));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
}

#[test]
fn head_to_head_linear_cross_gravity_stretch_overrides_flex_start_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        align_items: AlignItems::FlexStart,
        linear_cross_gravity: LinearCrossGravity::Stretch,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let child = fixed_linear_child(&mut tree, Length::Auto, Length::points(10.0));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
}

#[test]
fn head_to_head_linear_cross_axis_auto_margins_override_cross_gravity() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_cross_gravity: LinearCrossGravity::End,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        margin: Rect::new(Length::Auto, Length::Auto, Length::ZERO, Length::ZERO),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
}

#[test]
fn head_to_head_horizontal_linear_cross_axis_start_auto_margin_pushes_item_to_end() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        margin: Rect::new(Length::ZERO, Length::ZERO, Length::Auto, Length::ZERO),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
}

#[test]
fn head_to_head_horizontal_linear_cross_axis_end_auto_margin_keeps_item_at_start() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        margin: Rect::new(Length::ZERO, Length::ZERO, Length::ZERO, Length::Auto),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
}

#[test]
fn head_to_head_horizontal_linear_overflowing_cross_axis_auto_margins_are_ignored() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(140.0),
        margin: Rect::new(Length::ZERO, Length::ZERO, Length::Auto, Length::Auto),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
}

#[test]
fn head_to_head_horizontal_linear_baseline_keeps_unresolved_start_auto_margin() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size_and_baseline(
        block_standalone_style(Style {
            margin: Rect::new(Length::ZERO, Length::ZERO, Length::Auto, Length::ZERO),
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
        4.0,
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
}

#[test]
fn head_to_head_horizontal_linear_baseline_uses_gravity_before_paired_auto_margins_resolve() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        linear_cross_gravity: LinearCrossGravity::End,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size_and_baseline(
        block_standalone_style(Style {
            margin: Rect::new(Length::ZERO, Length::ZERO, Length::Auto, Length::Auto),
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
        4.0,
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
}

#[test]
fn head_to_head_vertical_linear_cross_gravity_variants_override_align_items() {
    for cross_gravity in [
        LinearCrossGravity::None,
        LinearCrossGravity::Start,
        LinearCrossGravity::End,
        LinearCrossGravity::Center,
        LinearCrossGravity::Stretch,
    ] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
            align_items: AlignItems::FlexStart,
            linear_cross_gravity: cross_gravity,
            width: Length::points(100.0),
            height: Length::points(100.0),
            ..Style::default()
        })));
        let child = fixed_linear_child(&mut tree, Length::points(20.0), Length::points(10.0));
        tree.append_child(root, child);

        assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
    }
}

#[test]
fn head_to_head_horizontal_linear_cross_gravity_variants_override_align_items() {
    for cross_gravity in [
        LinearCrossGravity::None,
        LinearCrossGravity::Start,
        LinearCrossGravity::End,
        LinearCrossGravity::Center,
        LinearCrossGravity::Stretch,
    ] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
            linear_orientation: LinearOrientation::Horizontal,
            align_items: AlignItems::FlexStart,
            linear_cross_gravity: cross_gravity,
            width: Length::points(100.0),
            height: Length::points(100.0),
            ..Style::default()
        })));
        let child = fixed_linear_child(&mut tree, Length::points(20.0), Length::points(10.0));
        tree.append_child(root, child);

        assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
    }
}

#[test]
fn head_to_head_vertical_linear_weight_takes_remaining_main_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let fixed = fixed_linear_child(&mut tree, Length::Auto, Length::points(10.0));
    let weighted = tree.push(SimpleNode::new(standalone_style(Style {
        linear_weight: 1.0,
        ..Style::default()
    })));
    tree.append_child(root, fixed);
    tree.append_child(root, weighted);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
}

#[test]
fn head_to_head_vertical_linear_weight_gets_zero_when_main_space_is_exhausted() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let fixed = fixed_linear_child(&mut tree, Length::points(20.0), Length::points(30.0));
    let weighted = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(20.0),
        height: Length::Auto,
        linear_weight: 1.0,
        ..Style::default()
    })));
    tree.append_child(root, fixed);
    tree.append_child(root, weighted);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_horizontal_linear_weight_sub_epsilon_min_violations_do_not_freeze_items() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(block_standalone_style(Style {
            linear_weight: 1.0,
            min_width: Length::points(50.00006),
            height: Length::points(10.0),
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_horizontal_linear_weights_split_remaining_main_space_by_ratio() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(90.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(block_standalone_style(Style {
        linear_weight: 1.0,
        ..Style::default()
    })));
    let second = tree.push(SimpleNode::new(block_standalone_style(Style {
        linear_weight: 2.0,
        ..Style::default()
    })));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(90.0, 20.0));
}

#[test]
fn head_to_head_linear_weight_sum_can_leave_unallocated_main_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        linear_weight_sum: 4.0,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style {
            linear_weight: 1.0,
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_linear_total_weight_below_one_leaves_unallocated_main_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        linear_weight: 0.5,
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_linear_weight_max_size_freezes_and_redistributes_remaining_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let capped = tree.push(SimpleNode::new(standalone_style(Style {
        linear_weight: 1.0,
        max_width: Length::points(30.0),
        ..Style::default()
    })));
    let flexible = tree.push(SimpleNode::new(standalone_style(Style {
        linear_weight: 1.0,
        ..Style::default()
    })));
    tree.append_child(root, capped);
    tree.append_child(root, flexible);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_linear_weight_percent_max_size_freezes_and_redistributes_remaining_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let capped = tree.push(SimpleNode::new(standalone_style(Style {
        linear_weight: 1.0,
        max_width: Length::percent(30.0),
        ..Style::default()
    })));
    let flexible = tree.push(SimpleNode::new(standalone_style(Style {
        linear_weight: 1.0,
        ..Style::default()
    })));
    tree.append_child(root, capped);
    tree.append_child(root, flexible);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_linear_weight_min_size_freezes_and_redistributes_remaining_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let floor = tree.push(SimpleNode::new(standalone_style(Style {
        linear_weight: 1.0,
        min_width: Length::points(70.0),
        ..Style::default()
    })));
    let flexible = tree.push(SimpleNode::new(standalone_style(Style {
        linear_weight: 1.0,
        ..Style::default()
    })));
    tree.append_child(root, floor);
    tree.append_child(root, flexible);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_linear_weight_all_items_freeze_after_min_violations() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style {
            linear_weight: 1.0,
            min_width: Length::points(60.0),
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_linear_weight_percent_min_size_freezes_and_redistributes_remaining_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        linear_orientation: LinearOrientation::Horizontal,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let floor = tree.push(SimpleNode::new(standalone_style(Style {
        linear_weight: 1.0,
        min_width: Length::percent(70.0),
        ..Style::default()
    })));
    let flexible = tree.push(SimpleNode::new(standalone_style(Style {
        linear_weight: 1.0,
        ..Style::default()
    })));
    tree.append_child(root, floor);
    tree.append_child(root, flexible);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_vertical_linear_weight_percent_max_size_freezes_and_redistributes_remaining_space()
{
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let capped = tree.push(SimpleNode::new(standalone_style(Style {
        linear_weight: 1.0,
        max_height: Length::percent(30.0),
        ..Style::default()
    })));
    let flexible = tree.push(SimpleNode::new(standalone_style(Style {
        linear_weight: 1.0,
        ..Style::default()
    })));
    tree.append_child(root, capped);
    tree.append_child(root, flexible);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(20.0, 100.0));
}

#[test]
fn head_to_head_vertical_linear_weight_percent_min_size_freezes_and_redistributes_remaining_space()
{
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let floor = tree.push(SimpleNode::new(standalone_style(Style {
        linear_weight: 1.0,
        min_height: Length::percent(70.0),
        ..Style::default()
    })));
    let flexible = tree.push(SimpleNode::new(standalone_style(Style {
        linear_weight: 1.0,
        ..Style::default()
    })));
    tree.append_child(root, floor);
    tree.append_child(root, flexible);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(20.0, 100.0));
}

#[test]
fn head_to_head_relative_center_none_keeps_default_start_position() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_center: RelativeCenter::None,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_centers_child_in_definite_parent() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_center: RelativeCenter::Both,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_wrap_content_center_recomputes_after_container_sizing() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style::default())));
    let anchor = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style::default()),
        Size::new(80.0, 30.0),
    ));
    let centered = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_center: RelativeCenter::Both,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, anchor);
    tree.append_child(root, centered);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_visibility_hidden_and_collapse_participate_in_dependency_layout() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    })));
    let hidden_anchor = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        relative_id: 10,
        visibility: Visibility::Hidden,
        ..Style::default()
    })));
    let collapsed_anchor = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(30.0),
        height: Length::points(12.0),
        relative_id: 20,
        relative_right_of: 10,
        visibility: Visibility::Collapse,
        ..Style::default()
    })));
    let follower = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(5.0),
        height: Length::points(5.0),
        relative_right_of: 20,
        relative_bottom_of: 20,
        ..Style::default()
    })));
    tree.append_child(root, hidden_anchor);
    tree.append_child(root, collapsed_anchor);
    tree.append_child(root, follower);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 80.0));
}

#[test]
fn head_to_head_relative_absolute_child_uses_static_start_without_participating() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style::default())));
    let absolute = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Absolute,
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_absolute_static_start_with_margins_positions_margin_box() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::new(block_standalone_style(Style {
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
    })));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_relative_absolute_percent_insets_and_size_resolve_against_relative_containing_block(
) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Absolute,
        left: Length::percent(10.0),
        top: Length::percent(25.0),
        width: Length::percent(50.0),
        height: Length::percent(20.0),
        ..Style::default()
    })));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_relative_absolute_percent_end_insets_resolve_against_relative_containing_block() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Absolute,
        right: Length::percent(10.0),
        bottom: Length::percent(25.0),
        width: Length::percent(50.0),
        height: Length::percent(20.0),
        ..Style::default()
    })));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_relative_absolute_auto_size_stretches_between_start_and_end_insets() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Absolute,
        left: Length::points(10.0),
        right: Length::points(30.0),
        top: Length::points(20.0),
        bottom: Length::points(25.0),
        ..Style::default()
    })));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_relative_absolute_auto_size_between_insets_strips_margins() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::new(block_standalone_style(Style {
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
    })));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_relative_absolute_auto_size_paired_insets_fill_padding_box_minus_margins() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(50.0),
        padding: Rect::all(Length::points(10.0)),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
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
        }),
        Size::new(200.0, 200.0),
    ));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(120.0, 70.0));
}

#[test]
fn head_to_head_relative_absolute_single_insets_strip_at_most_measure_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(50.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
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
        }),
        Size::new(200.0, 100.0),
    ));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 50.0));
}

#[test]
fn head_to_head_relative_absolute_end_insets_override_static_start_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Absolute,
        right: Length::points(30.0),
        bottom: Length::points(25.0),
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_relative_absolute_end_insets_with_margins_position_margin_box() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::new(block_standalone_style(Style {
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
    })));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_relative_absolute_start_insets_override_static_start_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Absolute,
        left: Length::points(12.0),
        top: Length::points(9.0),
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_relative_absolute_paired_insets_with_explicit_size_use_start_insets() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Absolute,
        left: Length::points(12.0),
        right: Length::points(30.0),
        top: Length::points(9.0),
        bottom: Length::points(25.0),
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, absolute);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_relative_fixed_descendant_uses_root_relative_containing_block() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Fixed,
        width: Length::points(20.0),
        height: Length::points(10.0),
        right: Length::points(5.0),
        bottom: Length::points(7.0),
        ..Style::default()
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 80.0));
}

#[test]
fn head_to_head_relative_fixed_descendant_uses_relative_root_padding_box_offset() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(80.0),
        padding: Rect::all(Length::points(3.0)),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Fixed,
        width: Length::points(10.0),
        height: Length::points(10.0),
        left: Length::points(5.0),
        top: Length::points(7.0),
        ..Style::default()
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 80.0));
}

#[test]
fn head_to_head_relative_fixed_static_start_with_margins_positions_margin_box() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
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
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_relative_fixed_percent_insets_and_size_resolve_against_root_relative_containing_block(
) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Fixed,
        left: Length::percent(10.0),
        top: Length::percent(25.0),
        width: Length::percent(50.0),
        height: Length::percent(20.0),
        ..Style::default()
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_relative_fixed_percent_end_insets_resolve_against_root_relative_containing_block() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Fixed,
        right: Length::percent(10.0),
        bottom: Length::percent(25.0),
        width: Length::percent(50.0),
        height: Length::percent(20.0),
        ..Style::default()
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_relative_fixed_auto_size_stretches_between_start_and_end_insets() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Fixed,
        left: Length::points(10.0),
        right: Length::points(30.0),
        top: Length::points(20.0),
        bottom: Length::points(25.0),
        ..Style::default()
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_relative_fixed_auto_size_between_insets_strips_margins() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
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
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_relative_fixed_single_insets_strip_at_most_measure_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(50.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
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
        }),
        Size::new(200.0, 100.0),
    ));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 50.0));
}

#[test]
fn head_to_head_relative_fixed_start_insets_override_static_start_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Fixed,
        left: Length::points(12.0),
        top: Length::points(9.0),
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_relative_fixed_paired_insets_with_explicit_size_use_start_insets() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Fixed,
        left: Length::points(12.0),
        right: Length::points(30.0),
        top: Length::points(9.0),
        bottom: Length::points(25.0),
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_relative_fixed_end_insets_override_static_start_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Fixed,
        right: Length::points(30.0),
        bottom: Length::points(25.0),
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_relative_fixed_end_insets_with_margins_position_margin_box() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let nested = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(40.0),
        height: Length::points(30.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
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
    })));
    tree.append_child(root, nested);
    tree.append_child(nested, fixed);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_relative_centers_child_horizontally_only_in_definite_parent() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_center: RelativeCenter::Horizontal,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_centers_child_vertically_only_in_definite_parent() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_center: RelativeCenter::Vertical,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_missing_reference_resolves_to_no_constraint_before_centering() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_right_of: 999,
            relative_bottom_of: 999,
            relative_center: RelativeCenter::Both,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_missing_start_references_fall_back_to_after_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(120.0),
        height: Length::points(90.0),
        ..Style::default()
    })));
    let horizontal_anchor = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_id: 10,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    let vertical_anchor = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_id: 30,
            ..Style::default()
        }),
        Size::new(10.0, 15.0),
    ));
    let follower = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_align_left: 999,
            relative_right_of: 10,
            relative_align_top: 998,
            relative_bottom_of: 30,
            ..Style::default()
        }),
        Size::new(10.0, 8.0),
    ));
    tree.append_child(root, horizontal_anchor);
    tree.append_child(root, vertical_anchor);
    tree.append_child(root, follower);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_missing_end_references_fall_back_to_before_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(120.0),
        height: Length::points(90.0),
        ..Style::default()
    })));
    let right_anchor = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_id: 20,
            relative_align_right: RELATIVE_ALIGN_PARENT,
            ..Style::default()
        }),
        Size::new(30.0, 10.0),
    ));
    let bottom_anchor = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_id: 40,
            relative_align_bottom: RELATIVE_ALIGN_PARENT,
            ..Style::default()
        }),
        Size::new(10.0, 20.0),
    ));
    let follower = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_align_right: 999,
            relative_left_of: 20,
            relative_align_bottom: 998,
            relative_top_of: 40,
            ..Style::default()
        }),
        Size::new(10.0, 8.0),
    ));
    tree.append_child(root, right_anchor);
    tree.append_child(root, bottom_anchor);
    tree.append_child(root, follower);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_aligns_child_to_parent_end_edges() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_align_right: RELATIVE_ALIGN_PARENT,
            relative_align_bottom: RELATIVE_ALIGN_PARENT,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_parent_end_alignment_takes_precedence_over_centering() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_align_right: RELATIVE_ALIGN_PARENT,
            relative_align_bottom: RELATIVE_ALIGN_PARENT,
            relative_center: RelativeCenter::Both,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_parent_start_alignment_takes_precedence_over_centering() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_align_left: RELATIVE_ALIGN_PARENT,
            relative_align_top: RELATIVE_ALIGN_PARENT,
            relative_center: RelativeCenter::Both,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_non_once_wrap_content_height_uses_prefinal_vertical_recompute() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        padding: Rect::new(
            Length::points(3.0),
            Length::points(4.0),
            Length::points(5.0),
            Length::points(2.0),
        ),
        ..Style::default()
    })));
    let anchor = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
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
        }),
        Size::new(20.0, 10.0),
    ));
    let dependent = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
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
        }),
        Size::new(16.0, 12.0),
    ));
    let follower = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_left_of: 2,
            relative_top_of: 2,
            ..Style::default()
        }),
        Size::new(9.0, 7.0),
    ));
    tree.append_child(root, anchor);
    tree.append_child(root, dependent);
    tree.append_child(root, follower);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_wrap_content_width_remeasures_two_sided_child_after_horizontal_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        padding: Rect::new(
            Length::points(3.0),
            Length::points(4.0),
            Length::points(5.0),
            Length::points(2.0),
        ),
        ..Style::default()
    })));
    let anchor = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
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
        }),
        Size::new(20.0, 10.0),
    ));
    let dependent = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
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
        }),
        Size::new(16.0, 12.0),
    ));
    let follower = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_left_of: 2,
            relative_top_of: 2,
            ..Style::default()
        }),
        Size::new(9.0, 7.0),
    ));
    tree.append_child(root, anchor);
    tree.append_child(root, dependent);
    tree.append_child(root, follower);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_positions_child_after_referenced_sibling() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style::default())));
    let anchor = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_id: 10,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    let follower = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_right_of: 10,
            relative_bottom_of: 10,
            ..Style::default()
        }),
        Size::new(5.0, 7.0),
    ));
    tree.append_child(root, follower);
    tree.append_child(root, anchor);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_align_parent_start_takes_precedence_over_sibling_after_constraint() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(40.0),
        ..Style::default()
    })));
    let anchor = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_id: 10,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    let follower = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_align_left: RELATIVE_ALIGN_PARENT,
            relative_align_top: RELATIVE_ALIGN_PARENT,
            relative_right_of: 10,
            relative_bottom_of: 10,
            ..Style::default()
        }),
        Size::new(5.0, 7.0),
    ));
    tree.append_child(root, follower);
    tree.append_child(root, anchor);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 40.0));
}

#[test]
fn head_to_head_relative_align_parent_end_takes_precedence_over_sibling_before_constraint() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(40.0),
        ..Style::default()
    })));
    let anchor = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_id: 10,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    let follower = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_align_right: RELATIVE_ALIGN_PARENT,
            relative_align_bottom: RELATIVE_ALIGN_PARENT,
            relative_left_of: 10,
            relative_top_of: 10,
            ..Style::default()
        }),
        Size::new(5.0, 7.0),
    ));
    tree.append_child(root, follower);
    tree.append_child(root, anchor);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 40.0));
}

#[test]
fn head_to_head_relative_align_sibling_start_takes_precedence_over_sibling_after_constraint() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    })));
    let anchor = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_id: 10,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    let after_anchor = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_id: 20,
            relative_right_of: 10,
            relative_bottom_of: 10,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    let follower = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_align_left: 10,
            relative_align_top: 10,
            relative_right_of: 20,
            relative_bottom_of: 20,
            ..Style::default()
        }),
        Size::new(5.0, 7.0),
    ));
    tree.append_child(root, follower);
    tree.append_child(root, after_anchor);
    tree.append_child(root, anchor);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 80.0));
}

#[test]
fn head_to_head_relative_align_sibling_end_takes_precedence_over_sibling_before_constraint() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    })));
    let anchor = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_id: 10,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    let spacer = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_id: 30,
            ..Style::default()
        }),
        Size::new(40.0, 30.0),
    ));
    let before_anchor = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_id: 20,
            relative_right_of: 30,
            relative_bottom_of: 30,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    let follower = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_align_right: 10,
            relative_align_bottom: 10,
            relative_left_of: 20,
            relative_top_of: 20,
            ..Style::default()
        }),
        Size::new(5.0, 7.0),
    ));
    tree.append_child(root, follower);
    tree.append_child(root, before_anchor);
    tree.append_child(root, spacer);
    tree.append_child(root, anchor);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 80.0));
}

#[test]
fn head_to_head_relative_display_duplicate_ids_resolve_to_last_matching_sibling() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style::default())));
    let first_anchor = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_id: 10,
            ..Style::default()
        }),
        Size::new(60.0, 40.0),
    ));
    let follower = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_right_of: 10,
            relative_bottom_of: 10,
            ..Style::default()
        }),
        Size::new(5.0, 7.0),
    ));
    let last_anchor = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_id: 10,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, first_anchor);
    tree.append_child(root, follower);
    tree.append_child(root, last_anchor);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_display_order_affects_duplicate_id_resolution() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style::default())));
    let later_order_anchor = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_id: 10,
            order: 2,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    let follower = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_right_of: 10,
            relative_bottom_of: 10,
            order: 3,
            ..Style::default()
        }),
        Size::new(5.0, 7.0),
    ));
    let earlier_order_anchor = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_id: 10,
            order: 1,
            ..Style::default()
        }),
        Size::new(60.0, 40.0),
    ));
    tree.append_child(root, later_order_anchor);
    tree.append_child(root, follower);
    tree.append_child(root, earlier_order_anchor);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_display_skips_display_none_duplicate_id_for_dependency_lookup() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style::default())));
    let visible_anchor = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_id: 10,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    let follower = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_right_of: 10,
            relative_bottom_of: 10,
            ..Style::default()
        }),
        Size::new(5.0, 7.0),
    ));
    let hidden_anchor = tree.push(SimpleNode::new(Style {
        display: Display::None,
        box_sizing: BoxSizing::ContentBox,
        relative_id: 10,
        width: Length::points(80.0),
        height: Length::points(40.0),
        ..Style::default()
    }));
    tree.append_child(root, visible_anchor);
    tree.append_child(root, follower);
    tree.append_child(root, hidden_anchor);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_display_duplicate_ids_align_to_last_matching_sibling_edge() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(120.0),
        height: Length::points(80.0),
        ..Style::default()
    })));
    let first_anchor = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_id: 10,
            relative_align_right: RELATIVE_ALIGN_PARENT,
            relative_align_bottom: RELATIVE_ALIGN_PARENT,
            ..Style::default()
        }),
        Size::new(30.0, 20.0),
    ));
    let follower = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_align_left: 10,
            relative_align_bottom: 10,
            ..Style::default()
        }),
        Size::new(5.0, 7.0),
    ));
    let last_anchor = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_id: 10,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, first_anchor);
    tree.append_child(root, follower);
    tree.append_child(root, last_anchor);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_root_relative_fit_content_percent_argument_uses_wrap_content_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 50.0))),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 25.0))),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style::default()),
        Size::new(140.0, 70.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_root_relative_fit_content_calc_argument_uses_wrap_content_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(5.0, 25.0))),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style::default()),
        Size::new(140.0, 70.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_child_relative_fit_content_percent_argument_uses_wrap_content_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let relative = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 50.0))),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 25.0))),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style::default()),
        Size::new(140.0, 70.0),
    ));
    tree.append_child(root, relative);
    tree.append_child(relative, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_child_relative_fit_content_calc_argument_uses_wrap_content_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(100.0),
        ..Style::default()
    })));
    let relative = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(5.0, 25.0))),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style::default()),
        Size::new(140.0, 70.0),
    ));
    tree.append_child(root, relative);
    tree.append_child(relative, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 100.0));
}

#[test]
fn head_to_head_wrap_content_relative_recomputes_parent_end_alignment_after_sizing() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style::default())));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_align_right: RELATIVE_ALIGN_PARENT,
            relative_align_bottom: RELATIVE_ALIGN_PARENT,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_layout_once_uses_combined_dependency_order_for_cross_axis_cycle() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        relative_layout_once: true,
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_id: 1,
            relative_bottom_of: 2,
            ..Style::default()
        }),
        Size::new(10.0, 10.0),
    ));
    let second = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_id: 2,
            relative_right_of: 1,
            ..Style::default()
        }),
        Size::new(5.0, 7.0),
    ));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_layout_once_processes_initial_roots_before_dependents() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        relative_layout_once: true,
        ..Style::default()
    })));
    let dependent = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_right_of: 2,
            ..Style::default()
        }),
        Size::new(10.0, 10.0),
    ));
    let centered_root = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_center: RelativeCenter::Horizontal,
            ..Style::default()
        }),
        Size::new(10.0, 10.0),
    ));
    let anchor = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_id: 2,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, dependent);
    tree.append_child(root, centered_root);
    tree.append_child(root, anchor);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_layout_once_parent_edge_stretch_strips_child_margins() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        relative_layout_once: true,
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
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
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_layout_once_remeasures_two_sided_child_on_both_axes() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        relative_layout_once: true,
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
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
        }),
        Size::new(200.0, 200.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_display_stretches_child_between_parent_edges() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        relative_align_left: RELATIVE_ALIGN_PARENT,
        relative_align_right: RELATIVE_ALIGN_PARENT,
        relative_align_top: RELATIVE_ALIGN_PARENT,
        relative_align_bottom: RELATIVE_ALIGN_PARENT,
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_display_positions_child_before_referenced_sibling() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    })));
    let anchor = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_id: 20,
            relative_align_right: RELATIVE_ALIGN_PARENT,
            relative_align_bottom: RELATIVE_ALIGN_PARENT,
            ..Style::default()
        }),
        Size::new(20.0, 20.0),
    ));
    let before = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_left_of: 20,
            relative_top_of: 20,
            ..Style::default()
        }),
        Size::new(10.0, 10.0),
    ));
    tree.append_child(root, before);
    tree.append_child(root, anchor);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_display_aligns_child_to_sibling_edges() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    })));
    let anchor = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_id: 30,
            relative_align_right: RELATIVE_ALIGN_PARENT,
            relative_align_bottom: RELATIVE_ALIGN_PARENT,
            ..Style::default()
        }),
        Size::new(20.0, 20.0),
    ));
    let aligned = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_align_left: 30,
            relative_align_bottom: 30,
            ..Style::default()
        }),
        Size::new(5.0, 7.0),
    ));
    tree.append_child(root, aligned);
    tree.append_child(root, anchor);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_display_stretches_child_between_sibling_edges_and_strips_margins() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(40.0),
        ..Style::default()
    })));
    let left = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_id: 40,
            relative_align_left: RELATIVE_ALIGN_PARENT,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    let right = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            relative_id: 41,
            relative_align_right: RELATIVE_ALIGN_PARENT,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    let middle = tree.push(SimpleNode::new(block_standalone_style(Style {
        relative_right_of: 40,
        relative_left_of: 41,
        margin: Rect::new(
            Length::points(5.0),
            Length::points(5.0),
            Length::ZERO,
            Length::ZERO,
        ),
        ..Style::default()
    })));
    tree.append_child(root, middle);
    tree.append_child(root, right);
    tree.append_child(root, left);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_display_padding_border_content_origin_matrix() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
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
    })));
    let anchor = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            relative_id: 10,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    let parent_end = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            relative_align_right: RELATIVE_ALIGN_PARENT,
            relative_align_bottom: RELATIVE_ALIGN_PARENT,
            ..Style::default()
        }),
        Size::new(10.0, 8.0),
    ));
    let centered = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            relative_center: RelativeCenter::Both,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    let sibling_after = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            relative_right_of: 10,
            relative_bottom_of: 10,
            ..Style::default()
        }),
        Size::new(6.0, 4.0),
    ));
    for child in [anchor, parent_end, centered, sibling_after] {
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[derive(Clone, Copy, Debug)]
enum RelativeSiblingEdgeCase {
    RightOf,
    LeftOf,
    AlignLeft,
    AlignRight,
    BottomOf,
    TopOf,
    AlignTop,
    AlignBottom,
    AlignLeftToParentRight,
    AlignTopToParentBottom,
}

#[test]
fn head_to_head_relative_sibling_edge_position_matrix() {
    const CASES: [RelativeSiblingEdgeCase; 10] = [
        RelativeSiblingEdgeCase::RightOf,
        RelativeSiblingEdgeCase::LeftOf,
        RelativeSiblingEdgeCase::AlignLeft,
        RelativeSiblingEdgeCase::AlignRight,
        RelativeSiblingEdgeCase::BottomOf,
        RelativeSiblingEdgeCase::TopOf,
        RelativeSiblingEdgeCase::AlignTop,
        RelativeSiblingEdgeCase::AlignBottom,
        RelativeSiblingEdgeCase::AlignLeftToParentRight,
        RelativeSiblingEdgeCase::AlignTopToParentBottom,
    ];

    for case in CASES {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
            width: Length::points(120.0),
            height: Length::points(90.0),
            padding: Rect::new(
                Length::points(3.0),
                Length::points(5.0),
                Length::points(7.0),
                Length::points(11.0),
            ),
            ..Style::default()
        })));
        let anchor = tree.push(SimpleNode::new(block_standalone_style(Style {
            relative_id: 10,
            relative_align_left: RELATIVE_ALIGN_PARENT,
            relative_align_top: RELATIVE_ALIGN_PARENT,
            width: Length::points(30.0),
            height: Length::points(20.0),
            margin: Rect::new(
                Length::points(2.0),
                Length::points(4.0),
                Length::points(3.0),
                Length::points(5.0),
            ),
            ..Style::default()
        })));
        let dependent = tree.push(SimpleNode::new(relative_sibling_edge_style(case)));
        tree.append_child(root, dependent);
        tree.append_child(root, anchor);

        assert_head_to_head_or_skip_with_name(
            &format!("{case:?}"),
            tree,
            root,
            Constraints::indefinite(),
        );
    }
}

fn relative_sibling_edge_style(case: RelativeSiblingEdgeCase) -> Style {
    let mut style = block_standalone_style(Style {
        width: Length::points(10.0),
        height: Length::points(8.0),
        margin: Rect::new(
            Length::points(1.0),
            Length::points(2.0),
            Length::points(3.0),
            Length::points(4.0),
        ),
        ..Style::default()
    });

    match case {
        RelativeSiblingEdgeCase::RightOf => style.relative_right_of = 10,
        RelativeSiblingEdgeCase::LeftOf => style.relative_left_of = 10,
        RelativeSiblingEdgeCase::AlignLeft => style.relative_align_left = 10,
        RelativeSiblingEdgeCase::AlignRight => style.relative_align_right = 10,
        RelativeSiblingEdgeCase::BottomOf => style.relative_bottom_of = 10,
        RelativeSiblingEdgeCase::TopOf => style.relative_top_of = 10,
        RelativeSiblingEdgeCase::AlignTop => style.relative_align_top = 10,
        RelativeSiblingEdgeCase::AlignBottom => style.relative_align_bottom = 10,
        RelativeSiblingEdgeCase::AlignLeftToParentRight => {
            style.relative_align_left = 10;
            style.relative_align_right = RELATIVE_ALIGN_PARENT;
        }
        RelativeSiblingEdgeCase::AlignTopToParentBottom => {
            style.relative_align_top = 10;
            style.relative_align_bottom = RELATIVE_ALIGN_PARENT;
        }
    }

    style
}

#[test]
fn head_to_head_relative_display_single_start_constraint_reduces_at_most_measure_width() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(relative_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let anchor = tree.push(MeasuringNode::measured(
        standalone_style(Style {
            relative_id: 10,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    let follower = tree.push(MeasuringNode::measured(
        standalone_style(Style {
            relative_right_of: 10,
            ..Style::default()
        }),
        Size::new(200.0, 10.0),
    ));
    tree.append_child(root, follower);
    tree.append_child(root, anchor);

    assert_measuring_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_display_single_start_constraint_reduces_at_most_measure_height() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(relative_standalone_style(Style {
        width: Length::points(30.0),
        height: Length::points(80.0),
        ..Style::default()
    })));
    let anchor = tree.push(MeasuringNode::measured(
        standalone_style(Style {
            relative_id: 11,
            ..Style::default()
        }),
        Size::new(10.0, 20.0),
    ));
    let follower = tree.push(MeasuringNode::measured(
        standalone_style(Style {
            relative_bottom_of: 11,
            ..Style::default()
        }),
        Size::new(10.0, 200.0),
    ));
    tree.append_child(root, follower);
    tree.append_child(root, anchor);

    assert_measuring_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_display_single_end_constraint_preserves_margin_in_at_most_height() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(relative_standalone_style(Style {
        width: Length::points(30.0),
        height: Length::points(80.0),
        ..Style::default()
    })));
    let anchor = tree.push(MeasuringNode::measured(
        standalone_style(Style {
            relative_id: 12,
            relative_align_bottom: RELATIVE_ALIGN_PARENT,
            ..Style::default()
        }),
        Size::new(10.0, 20.0),
    ));
    let follower = tree.push(MeasuringNode::measured(
        standalone_style(Style {
            relative_top_of: 12,
            margin: Rect::new(
                Length::ZERO,
                Length::ZERO,
                Length::points(4.0),
                Length::points(6.0),
            ),
            ..Style::default()
        }),
        Size::new(10.0, 200.0),
    ));
    tree.append_child(root, follower);
    tree.append_child(root, anchor);

    assert_measuring_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_display_single_end_constraint_preserves_margin_in_at_most_width() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(relative_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let anchor = tree.push(MeasuringNode::measured(
        standalone_style(Style {
            relative_id: 20,
            relative_align_right: RELATIVE_ALIGN_PARENT,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    let follower = tree.push(MeasuringNode::measured(
        standalone_style(Style {
            relative_left_of: 20,
            margin: Rect::new(
                Length::points(5.0),
                Length::points(5.0),
                Length::ZERO,
                Length::ZERO,
            ),
            ..Style::default()
        }),
        Size::new(200.0, 10.0),
    ));
    tree.append_child(root, follower);
    tree.append_child(root, anchor);

    assert_measuring_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_two_pass_freezes_horizontal_size_before_vertical_stretch_remeasure() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(relative_standalone_style(Style {
        height: Length::points(100.0),
        ..Style::default()
    })));
    let child = tree.push(MeasuringNode::width_by_height_mode(
        standalone_style(Style {
            relative_align_top: RELATIVE_ALIGN_PARENT,
            relative_align_bottom: RELATIVE_ALIGN_PARENT,
            ..Style::default()
        }),
        20.0,
        60.0,
        10.0,
    ));
    tree.append_child(root, child);

    assert_measuring_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_container_min_width_and_max_height_clamp_wrap_content_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        min_width: Length::points(40.0),
        max_height: Length::points(25.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style::default()),
        Size::new(20.0, 30.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_container_max_width_and_min_height_clamp_wrap_content_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        max_width: Length::points(60.0),
        min_height: Length::points(40.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style::default()),
        Size::new(100.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_relative_container_padding_border_prevents_negative_content_size_under_tight_constraints(
) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_standalone_style(Style {
        padding: Rect::new(
            Length::points(10.0),
            Length::points(15.0),
            Length::points(8.0),
            Length::points(9.0),
        ),
        border: Rect::new(2.0, 3.0, 1.0, 4.0),
        ..Style::default()
    })));

    assert_head_to_head_or_skip(tree, root, Constraints::definite(8.0, 7.0));
}

#[test]
fn head_to_head_grid_auto_row_grows_from_child_aspect_ratio() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(80.0),
        grid_template_columns: vec![Length::points(80.0)],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(80.0),
        aspect_ratio: Some(2.0),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(SideConstraint::definite(80.0), SideConstraint::indefinite()),
    );
}

#[test]
fn head_to_head_grid_explicit_tracks_place_children_row_major() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(130.0),
        height: Length::points(70.0),
        grid_template_columns: vec![Length::points(50.0), Length::points(70.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(30.0)],
        column_gap: Length::points(10.0),
        row_gap: Length::points(5.0),
        ..Style::default()
    })));
    for _ in 0..3 {
        let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(130.0, 70.0));
}

#[test]
fn head_to_head_display_none_child_is_laid_out_as_zero_and_skipped_by_grid_auto_placement() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(50.0), Length::points(50.0)],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(block_standalone_style(Style::default())));
    let hidden = tree.push(SimpleNode::new(Style {
        display: Display::None,
        box_sizing: BoxSizing::ContentBox,
        width: Length::points(50.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(block_standalone_style(Style::default())));
    tree.append_child(root, first);
    tree.append_child(root, hidden);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_grid_visibility_hidden_and_collapse_participate_in_auto_placement() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(150.0),
        height: Length::points(20.0),
        grid_template_columns: vec![
            Length::points(50.0),
            Length::points(50.0),
            Length::points(50.0),
        ],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    })));
    let hidden = tree.push(SimpleNode::new(block_standalone_style(Style {
        visibility: Visibility::Hidden,
        width: Length::points(50.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let collapsed = tree.push(SimpleNode::new(block_standalone_style(Style {
        visibility: Visibility::Collapse,
        width: Length::points(50.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    let visible = tree.push(SimpleNode::new(block_standalone_style(Style::default())));
    tree.append_child(root, hidden);
    tree.append_child(root, collapsed);
    tree.append_child(root, visible);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(150.0, 20.0));
}

#[test]
fn head_to_head_grid_column_auto_flow_places_children_down_each_column() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(130.0),
        height: Length::points(70.0),
        grid_auto_flow: GridAutoFlow::Column,
        grid_template_columns: vec![Length::points(50.0), Length::points(70.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(30.0)],
        column_gap: Length::points(10.0),
        row_gap: Length::points(5.0),
        ..Style::default()
    })));
    for _ in 0..3 {
        let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(130.0, 70.0));
}

#[test]
fn head_to_head_grid_column_auto_flow_keeps_cursor_at_item_start_for_following_search() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(140.0),
        height: Length::points(140.0),
        grid_auto_flow: GridAutoFlow::Column,
        grid_template_columns: vec![
            Length::points(24.0),
            Length::points(24.0),
            Length::points(24.0),
        ],
        grid_template_rows: vec![
            Length::points(18.0),
            Length::points(18.0),
            Length::points(18.0),
        ],
        grid_auto_columns: vec![Length::points(20.0), Length::points(26.0)],
        grid_auto_rows: vec![Length::points(16.0), Length::points(22.0)],
        column_gap: Length::points(4.0),
        row_gap: Length::points(3.0),
        ..Style::default()
    })));
    let blocking = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(8.0, 8.0),
    ));
    let first_wide = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_span: 2,
            ..Style::default()
        }),
        Size::new(9.0, 7.0),
    ));
    let tall = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_row_span: 2,
            ..Style::default()
        }),
        Size::new(9.0, 7.0),
    ));
    let second_wide = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_span: 2,
            ..Style::default()
        }),
        Size::new(9.0, 7.0),
    ));
    tree.append_child(root, blocking);
    tree.append_child(root, first_wide);
    tree.append_child(root, tall);
    tree.append_child(root, second_wide);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(160.0, 160.0));
}

#[test]
fn head_to_head_auto_grid_item_skips_cell_occupied_by_explicit_item() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(50.0), Length::points(50.0)],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    })));
    let explicit = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    let auto = tree.push(SimpleNode::new(standalone_style(Style::default())));
    tree.append_child(root, explicit);
    tree.append_child(root, auto);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_auto_grid_item_skips_later_explicit_item() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(50.0), Length::points(50.0)],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    })));
    let auto = tree.push(SimpleNode::new(standalone_style(Style::default())));
    let explicit = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, auto);
    tree.append_child(root, explicit);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_later_locked_main_axis_item_expands_auto_placement_limit_before_auto_items() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(30.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::points(10.0)],
        grid_auto_columns: vec![Length::points(10.0)],
        grid_auto_columns_max: vec![Length::points(10.0)],
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
        tree.append_child(root, child);
    }
    let locked_column = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(3),
        ..Style::default()
    })));
    tree.append_child(root, locked_column);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(30.0, 10.0));
}

#[test]
fn head_to_head_grid_auto_placement_orders_in_flow_children_by_order() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(10.0), Length::points(10.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    let later = tree.push(SimpleNode::new(standalone_style(Style {
        order: 1,
        ..Style::default()
    })));
    let earlier = tree.push(SimpleNode::new(standalone_style(Style {
        order: -1,
        ..Style::default()
    })));
    tree.append_child(root, later);
    tree.append_child(root, earlier);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(20.0, 10.0));
}

#[test]
fn head_to_head_grid_dense_row_auto_flow_backfills_earlier_holes() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(30.0),
        height: Length::points(30.0),
        grid_auto_flow: GridAutoFlow::Dense,
        grid_template_columns: vec![
            Length::points(10.0),
            Length::points(10.0),
            Length::points(10.0),
        ],
        grid_template_rows: vec![
            Length::points(10.0),
            Length::points(10.0),
            Length::points(10.0),
        ],
        ..Style::default()
    })));
    for span in [2, 2, 1] {
        let child = tree.push(SimpleNode::new(standalone_style(Style {
            grid_column_span: span,
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(30.0, 30.0));
}

#[test]
fn head_to_head_grid_row_dense_auto_flow_explicit_mapping_backfills_earlier_holes() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(30.0),
        height: Length::points(30.0),
        grid_auto_flow: GridAutoFlow::RowDense,
        grid_template_columns: vec![
            Length::points(10.0),
            Length::points(10.0),
            Length::points(10.0),
        ],
        grid_template_rows: vec![
            Length::points(10.0),
            Length::points(10.0),
            Length::points(10.0),
        ],
        ..Style::default()
    })));
    for span in [2, 2, 1] {
        let child = tree.push(SimpleNode::new(standalone_style(Style {
            grid_column_span: span,
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(30.0, 30.0));
}

#[test]
fn head_to_head_grid_column_dense_auto_flow_backfills_earlier_holes() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(30.0),
        height: Length::points(30.0),
        grid_auto_flow: GridAutoFlow::ColumnDense,
        grid_template_columns: vec![
            Length::points(10.0),
            Length::points(10.0),
            Length::points(10.0),
        ],
        grid_template_rows: vec![
            Length::points(10.0),
            Length::points(10.0),
            Length::points(10.0),
        ],
        ..Style::default()
    })));
    for span in [2, 2, 1] {
        let child = tree.push(SimpleNode::new(standalone_style(Style {
            grid_row_span: span,
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(30.0, 30.0));
}

#[test]
fn head_to_head_grid_w3c_auto_placement_sparse_dense_matrix() {
    for case in [
        NativeGridAutoPlacementMatrixCase {
            name: "row sparse fully-auto cursor",
            flow: GridAutoFlow::Row,
            kind: NativeGridAutoPlacementMatrixKind::FullyAuto,
        },
        NativeGridAutoPlacementMatrixCase {
            name: "row dense fully-auto backfill",
            flow: GridAutoFlow::Dense,
            kind: NativeGridAutoPlacementMatrixKind::FullyAuto,
        },
        NativeGridAutoPlacementMatrixCase {
            name: "column sparse fully-auto cursor",
            flow: GridAutoFlow::Column,
            kind: NativeGridAutoPlacementMatrixKind::FullyAuto,
        },
        NativeGridAutoPlacementMatrixCase {
            name: "column dense fully-auto backfill",
            flow: GridAutoFlow::ColumnDense,
            kind: NativeGridAutoPlacementMatrixKind::FullyAuto,
        },
        NativeGridAutoPlacementMatrixCase {
            name: "row sparse locked-axis cursor",
            flow: GridAutoFlow::Row,
            kind: NativeGridAutoPlacementMatrixKind::LockedAxis,
        },
        NativeGridAutoPlacementMatrixCase {
            name: "row dense locked-axis backfill",
            flow: GridAutoFlow::Dense,
            kind: NativeGridAutoPlacementMatrixKind::LockedAxis,
        },
        NativeGridAutoPlacementMatrixCase {
            name: "column sparse locked-axis cursor",
            flow: GridAutoFlow::Column,
            kind: NativeGridAutoPlacementMatrixKind::LockedAxis,
        },
        NativeGridAutoPlacementMatrixCase {
            name: "column dense locked-axis backfill",
            flow: GridAutoFlow::ColumnDense,
            kind: NativeGridAutoPlacementMatrixKind::LockedAxis,
        },
        NativeGridAutoPlacementMatrixCase {
            name: "row sparse leading implicit cursor",
            flow: GridAutoFlow::Row,
            kind: NativeGridAutoPlacementMatrixKind::LeadingImplicit,
        },
        NativeGridAutoPlacementMatrixCase {
            name: "column sparse leading implicit cursor",
            flow: GridAutoFlow::Column,
            kind: NativeGridAutoPlacementMatrixKind::LeadingImplicit,
        },
    ] {
        let (tree, root, constraints) = native_grid_auto_placement_matrix_tree(case);
        assert_head_to_head_or_skip_with_name(case.name, tree, root, constraints);
    }
}

#[test]
#[ignore = "current C++ standalone places the row-dense leading implicit backfill one track earlier than W3C section 8.5 dense auto-placement over the full implicit grid"]
fn head_to_head_grid_w3c_auto_placement_row_dense_leading_implicit_backfill() {
    let case = NativeGridAutoPlacementMatrixCase {
        name: "row dense leading implicit backfill",
        flow: GridAutoFlow::Dense,
        kind: NativeGridAutoPlacementMatrixKind::LeadingImplicit,
    };
    let (tree, root, constraints) = native_grid_auto_placement_matrix_tree(case);
    assert_head_to_head_or_skip_with_name(case.name, tree, root, constraints);
}

#[test]
#[ignore = "current C++ standalone places the column-dense leading implicit backfill one track later than W3C section 8.5 dense auto-placement over the full implicit grid"]
fn head_to_head_grid_w3c_auto_placement_column_dense_leading_implicit_backfill() {
    let case = NativeGridAutoPlacementMatrixCase {
        name: "column dense leading implicit backfill",
        flow: GridAutoFlow::ColumnDense,
        kind: NativeGridAutoPlacementMatrixKind::LeadingImplicit,
    };
    let (tree, root, constraints) = native_grid_auto_placement_matrix_tree(case);
    assert_head_to_head_or_skip_with_name(case.name, tree, root, constraints);
}

#[derive(Clone, Copy)]
struct NativeGridAutoPlacementMatrixCase {
    name: &'static str,
    flow: GridAutoFlow,
    kind: NativeGridAutoPlacementMatrixKind,
}

#[derive(Clone, Copy)]
enum NativeGridAutoPlacementMatrixKind {
    FullyAuto,
    LockedAxis,
    LeadingImplicit,
}

fn native_grid_auto_placement_matrix_tree(
    case: NativeGridAutoPlacementMatrixCase,
) -> (SimpleTree, usize, Constraints) {
    match case.kind {
        NativeGridAutoPlacementMatrixKind::FullyAuto => {
            native_grid_fully_auto_placement_matrix_tree(case.flow)
        }
        NativeGridAutoPlacementMatrixKind::LockedAxis => {
            native_grid_locked_axis_auto_placement_matrix_tree(case.flow)
        }
        NativeGridAutoPlacementMatrixKind::LeadingImplicit => {
            native_grid_leading_implicit_auto_placement_matrix_tree(case.flow)
        }
    }
}

fn native_grid_fully_auto_placement_matrix_tree(
    flow: GridAutoFlow,
) -> (SimpleTree, usize, Constraints) {
    let mut tree = SimpleTree::default();
    let root = native_grid_auto_placement_matrix_root(&mut tree, flow);
    let is_column_flow = matches!(flow, GridAutoFlow::Column | GridAutoFlow::ColumnDense);

    for (index, span) in [2, 2, 1].into_iter().enumerate() {
        let child = tree.push(SimpleNode::with_measured_size(
            standalone_style(Style::default()),
            Size::new(6.0 + index as f32, 5.0),
        ));
        if is_column_flow {
            tree.nodes[child].style.grid_row_span = span;
        } else {
            tree.nodes[child].style.grid_column_span = span;
        }
        tree.append_child(root, child);
    }

    (tree, root, Constraints::definite(60.0, 60.0))
}

fn native_grid_locked_axis_auto_placement_matrix_tree(
    flow: GridAutoFlow,
) -> (SimpleTree, usize, Constraints) {
    let mut tree = SimpleTree::default();
    let root = native_grid_auto_placement_matrix_root(&mut tree, flow);
    let is_column_flow = matches!(flow, GridAutoFlow::Column | GridAutoFlow::ColumnDense);

    let blocker = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style::default()),
        Size::new(6.0, 6.0),
    ));
    let spanning = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style::default()),
        Size::new(8.0, 6.0),
    ));
    let backfill_candidate = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style::default()),
        Size::new(5.0, 5.0),
    ));

    if is_column_flow {
        tree.nodes[blocker].style.grid_column_start = Some(1);
        tree.nodes[blocker].style.grid_row_start = Some(2);
        tree.nodes[spanning].style.grid_column_start = Some(1);
        tree.nodes[spanning].style.grid_row_span = 2;
        tree.nodes[backfill_candidate].style.grid_column_start = Some(1);
    } else {
        tree.nodes[blocker].style.grid_column_start = Some(2);
        tree.nodes[blocker].style.grid_row_start = Some(1);
        tree.nodes[spanning].style.grid_row_start = Some(1);
        tree.nodes[spanning].style.grid_column_span = 2;
        tree.nodes[backfill_candidate].style.grid_row_start = Some(1);
    }

    tree.append_child(root, blocker);
    tree.append_child(root, spanning);
    tree.append_child(root, backfill_candidate);

    (tree, root, Constraints::definite(60.0, 60.0))
}

fn native_grid_leading_implicit_auto_placement_matrix_tree(
    flow: GridAutoFlow,
) -> (SimpleTree, usize, Constraints) {
    let mut tree = SimpleTree::default();
    let root = native_grid_auto_placement_matrix_root(&mut tree, flow);
    let is_column_flow = matches!(flow, GridAutoFlow::Column | GridAutoFlow::ColumnDense);

    let blocker = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style::default()),
        Size::new(6.0, 6.0),
    ));
    if is_column_flow {
        tree.nodes[blocker].style.grid_column_start = Some(1);
        tree.nodes[blocker].style.grid_row_start = Some(-5);
        tree.nodes[blocker].style.grid_row_end = Some(-4);
    } else {
        tree.nodes[blocker].style.grid_column_start = Some(-5);
        tree.nodes[blocker].style.grid_column_end = Some(-4);
        tree.nodes[blocker].style.grid_row_start = Some(1);
    }
    tree.append_child(root, blocker);

    for (index, span) in [2, 2, 1].into_iter().enumerate() {
        let child = tree.push(SimpleNode::with_measured_size(
            standalone_style(Style::default()),
            Size::new(7.0 + index as f32, 5.0),
        ));
        if is_column_flow {
            tree.nodes[child].style.grid_row_span = span;
        } else {
            tree.nodes[child].style.grid_column_span = span;
        }
        tree.append_child(root, child);
    }

    (tree, root, Constraints::definite(60.0, 60.0))
}

fn native_grid_auto_placement_matrix_root(tree: &mut SimpleTree, flow: GridAutoFlow) -> usize {
    tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(60.0),
        height: Length::points(60.0),
        grid_auto_flow: flow,
        grid_template_columns: vec![
            Length::points(10.0),
            Length::points(10.0),
            Length::points(10.0),
        ],
        grid_template_rows: vec![
            Length::points(10.0),
            Length::points(10.0),
            Length::points(10.0),
        ],
        grid_auto_columns: vec![Length::points(10.0)],
        grid_auto_columns_max: vec![Length::points(10.0)],
        grid_auto_rows: vec![Length::points(10.0)],
        grid_auto_rows_max: vec![Length::points(10.0)],
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })))
}

#[test]
fn head_to_head_grid_line_span_and_self_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(140.0),
        height: Length::points(50.0),
        grid_template_columns: vec![
            Length::points(40.0),
            Length::points(40.0),
            Length::points(40.0),
        ],
        grid_template_rows: vec![Length::points(20.0), Length::points(20.0)],
        column_gap: Length::points(5.0),
        row_gap: Length::points(5.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(10.0),
        height: Length::points(8.0),
        grid_column_start: Some(2),
        grid_row_start: Some(2),
        grid_column_span: 2,
        justify_self: JustifyItems::End,
        align_self: Some(AlignItems::FlexEnd),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(140.0, 50.0));
}

#[test]
fn head_to_head_grid_align_start_end_variants() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(50.0),
        height: Length::points(40.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_rows: vec![Length::points(30.0)],
        align_items: AlignItems::End,
        ..Style::default()
    })));
    let end_aligned = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(10.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    let start_aligned = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(10.0),
        height: Length::points(10.0),
        align_self: Some(AlignItems::Start),
        ..Style::default()
    })));
    tree.append_child(root, end_aligned);
    tree.append_child(root, start_aligned);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 40.0));
}

#[test]
fn head_to_head_grid_align_items_baseline_uses_start_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(80.0),
        grid_template_columns: vec![Length::points(100.0)],
        grid_template_rows: vec![Length::points(80.0)],
        align_items: AlignItems::Baseline,
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size_and_baseline(
        standalone_style(Style {
            width: Length::MaxContent,
            height: Length::MaxContent,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
        6.0,
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 80.0));
}

#[test]
fn head_to_head_grid_align_self_baseline_overrides_container_end_alignment_to_start() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(80.0),
        grid_template_columns: vec![Length::points(100.0)],
        grid_template_rows: vec![Length::points(80.0)],
        align_items: AlignItems::End,
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size_and_baseline(
        standalone_style(Style {
            width: Length::MaxContent,
            height: Length::MaxContent,
            align_self: Some(AlignItems::Baseline),
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
        6.0,
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 80.0));
}

#[test]
fn head_to_head_grid_container_baseline_uses_first_row_item_baseline() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(40.0),
        height: Length::points(50.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(20.0)],
        row_gap: Length::points(5.0),
        padding: Rect::new(
            Length::points(0.0),
            Length::points(3.0),
            Length::points(0.0),
            Length::points(0.0),
        ),
        border: Rect::new(0.0, 2.0, 0.0, 0.0),
        align_items: AlignItems::Start,
        ..Style::default()
    })));
    let later_row = tree.push(SimpleNode::with_measured_size_and_baseline(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(2),
            width: Length::MaxContent,
            height: Length::MaxContent,
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::Start),
            ..Style::default()
        }),
        Size::new(8.0, 10.0),
        7.0,
    ));
    let first_row = tree.push(SimpleNode::with_measured_size_and_baseline(
        standalone_style(Style {
            grid_column_start: Some(2),
            grid_row_start: Some(1),
            width: Length::MaxContent,
            height: Length::MaxContent,
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::Start),
            ..Style::default()
        }),
        Size::new(8.0, 10.0),
        4.0,
    ));
    tree.append_child(root, later_row);
    tree.append_child(root, first_row);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(40.0, 50.0));
}

#[test]
fn head_to_head_grid_container_baseline_uses_row_major_item_before_source_order() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(40.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_rows: vec![Length::points(20.0)],
        align_items: AlignItems::Start,
        ..Style::default()
    })));
    let second_column = tree.push(SimpleNode::with_measured_size_and_baseline(
        standalone_style(Style {
            grid_column_start: Some(2),
            grid_row_start: Some(1),
            width: Length::MaxContent,
            height: Length::MaxContent,
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::Start),
            ..Style::default()
        }),
        Size::new(8.0, 10.0),
        12.0,
    ));
    let first_column = tree.push(SimpleNode::with_measured_size_and_baseline(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            width: Length::MaxContent,
            height: Length::MaxContent,
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::Start),
            ..Style::default()
        }),
        Size::new(8.0, 10.0),
        5.0,
    ));
    tree.append_child(root, second_column);
    tree.append_child(root, first_column);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(40.0, 20.0));
}

#[test]
fn head_to_head_grid_items_center_child_with_justify_and_align_self() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(100.0),
        grid_template_columns: vec![Length::points(100.0)],
        grid_template_rows: vec![Length::points(100.0)],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        justify_self: JustifyItems::Center,
        align_self: Some(AlignItems::Center),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
}

#[test]
fn head_to_head_grid_container_alignment_applies_to_auto_self_children() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        justify_items: JustifyItems::End,
        align_items: AlignItems::FlexEnd,
        width: Length::points(100.0),
        height: Length::points(100.0),
        grid_template_columns: vec![Length::points(100.0)],
        grid_template_rows: vec![Length::points(100.0)],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 100.0));
}

#[test]
fn head_to_head_grid_justify_items_auto_and_stretch_mapping() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        justify_items: JustifyItems::Stretch,
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(50.0), Length::points(50.0)],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    })));
    let stretch_from_auto = tree.push(SimpleNode::new(standalone_style(Style {
        height: Length::points(10.0),
        justify_self: JustifyItems::Auto,
        ..Style::default()
    })));
    let explicit_stretch = tree.push(SimpleNode::new(standalone_style(Style {
        height: Length::points(10.0),
        justify_self: JustifyItems::Stretch,
        ..Style::default()
    })));
    tree.append_child(root, stretch_from_auto);
    tree.append_child(root, explicit_stretch);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_grid_justify_self_overrides_container_justify_items() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        justify_items: JustifyItems::End,
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(100.0)],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(20.0),
        justify_self: JustifyItems::Start,
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_grid_stretch_does_not_override_explicit_child_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(40.0),
        grid_template_columns: vec![Length::points(100.0)],
        grid_template_rows: vec![Length::points(40.0)],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 40.0));
}

#[test]
fn head_to_head_grid_stretch_does_not_override_max_content_child_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(40.0),
        grid_template_columns: vec![Length::points(100.0)],
        grid_template_rows: vec![Length::points(40.0)],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            width: Length::MaxContent,
            height: Length::MaxContent,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 40.0));
}

#[test]
fn head_to_head_grid_stretch_does_not_expand_max_content_track() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::MaxContent, Length::points(0.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    let marker = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    })));
    tree.append_child(root, intrinsic);
    tree.append_child(root, marker);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_grid_item_horizontal_auto_margins_override_justify_self() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(100.0)],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            margin: Rect::new(Length::Auto, Length::Auto, Length::ZERO, Length::ZERO),
            justify_self: JustifyItems::End,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_rtl_grid_places_inline_tracks_from_right_edge() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        direction: Direction::Rtl,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(10.0),
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(5.0, 10.0),
    ));
    let second = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(2),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(7.0, 10.0),
    ));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_rtl_grid_justify_self_end_uses_left_edge_of_cell() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        direction: Direction::Rtl,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(10.0),
        justify_items: JustifyItems::End,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(5.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_rtl_grid_justify_self_center_centers_item_in_cell() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        direction: Direction::Rtl,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(100.0)],
        grid_template_rows: vec![Length::points(10.0)],
        justify_items: JustifyItems::End,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            justify_self: JustifyItems::Center,
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_rtl_grid_auto_inline_margins_center_item_in_cell() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        direction: Direction::Rtl,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_rows: vec![Length::points(10.0)],
        justify_items: JustifyItems::End,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            margin: Rect::new(Length::Auto, Length::Auto, Length::ZERO, Length::ZERO),
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(6.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_rtl_grid_justify_content_center_offsets_track_group_from_right_edge() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        direction: Direction::Rtl,
        justify_content: JustifyContent::Center,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(10.0),
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(5.0, 10.0),
    ));
    let second = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(2),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(7.0, 10.0),
    ));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_rtl_grid_justify_content_start_variants_align_track_group_to_right_edge() {
    for justify_content in [JustifyContent::FlexStart, JustifyContent::Start] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
            direction: Direction::Rtl,
            justify_content,
            width: Length::points(100.0),
            height: Length::points(10.0),
            grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
            grid_template_rows: vec![Length::points(10.0)],
            column_gap: Length::points(10.0),
            justify_items: JustifyItems::Start,
            align_items: AlignItems::FlexStart,
            ..Style::default()
        })));
        let first = tree.push(SimpleNode::with_measured_size(
            standalone_style(Style {
                grid_column_start: Some(1),
                grid_row_start: Some(1),
                ..Style::default()
            }),
            Size::new(5.0, 10.0),
        ));
        let second = tree.push(SimpleNode::with_measured_size(
            standalone_style(Style {
                grid_column_start: Some(2),
                grid_row_start: Some(1),
                ..Style::default()
            }),
            Size::new(7.0, 10.0),
        ));
        tree.append_child(root, first);
        tree.append_child(root, second);

        assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
    }
}

#[test]
fn head_to_head_rtl_grid_justify_content_end_variants_align_track_group_to_left_edge() {
    for justify_content in [JustifyContent::FlexEnd, JustifyContent::End] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
            direction: Direction::Rtl,
            justify_content,
            width: Length::points(100.0),
            height: Length::points(10.0),
            grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
            grid_template_rows: vec![Length::points(10.0)],
            column_gap: Length::points(10.0),
            justify_items: JustifyItems::Start,
            align_items: AlignItems::FlexStart,
            ..Style::default()
        })));
        let first = tree.push(SimpleNode::with_measured_size(
            standalone_style(Style {
                grid_column_start: Some(1),
                grid_row_start: Some(1),
                ..Style::default()
            }),
            Size::new(5.0, 10.0),
        ));
        let second = tree.push(SimpleNode::with_measured_size(
            standalone_style(Style {
                grid_column_start: Some(2),
                grid_row_start: Some(1),
                ..Style::default()
            }),
            Size::new(7.0, 10.0),
        ));
        tree.append_child(root, first);
        tree.append_child(root, second);

        assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
    }
}

#[test]
fn head_to_head_rtl_grid_justify_content_space_between_keeps_right_origin_lines() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        direction: Direction::Rtl,
        justify_content: JustifyContent::SpaceBetween,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(10.0),
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(5.0, 10.0),
    ));
    let second = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(2),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(7.0, 10.0),
    ));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_rtl_grid_justify_content_space_evenly_offsets_track_group_from_right_edge() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        direction: Direction::Rtl,
        justify_content: JustifyContent::SpaceEvenly,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(10.0),
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(5.0, 10.0),
    ));
    let second = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(2),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(7.0, 10.0),
    ));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_rtl_grid_justify_content_space_around_offsets_track_group_from_right_edge() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        direction: Direction::Rtl,
        justify_content: JustifyContent::SpaceAround,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(10.0),
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(5.0, 10.0),
    ));
    let second = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(2),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(7.0, 10.0),
    ));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_rtl_grid_justify_content_space_evenly_falls_back_to_right_edge_when_tracks_overflow(
) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        direction: Direction::Rtl,
        justify_content: JustifyContent::SpaceEvenly,
        width: Length::points(50.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(40.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(10.0),
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(5.0, 10.0),
    ));
    let second = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(2),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(7.0, 10.0),
    ));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 10.0));
}

#[test]
fn head_to_head_rtl_grid_justify_content_space_around_falls_back_to_right_edge_when_tracks_overflow(
) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        direction: Direction::Rtl,
        justify_content: JustifyContent::SpaceAround,
        width: Length::points(50.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(40.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(10.0),
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(5.0, 10.0),
    ));
    let second = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(2),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(7.0, 10.0),
    ));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(50.0, 10.0));
}

#[test]
fn head_to_head_grid_item_vertical_auto_margins_override_align_self() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(100.0),
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_rows: vec![Length::points(100.0)],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            margin: Rect::new(Length::ZERO, Length::ZERO, Length::Auto, Length::Auto),
            align_self: Some(AlignItems::FlexEnd),
            ..Style::default()
        }),
        Size::new(10.0, 20.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(20.0, 100.0));
}

#[test]
fn head_to_head_grid_item_single_start_auto_margin_pushes_item_to_end() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(100.0)],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            margin: Rect::new(Length::Auto, Length::ZERO, Length::ZERO, Length::ZERO),
            justify_self: JustifyItems::Start,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_grid_item_single_end_auto_margin_keeps_item_at_start() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(100.0)],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            margin: Rect::new(Length::ZERO, Length::Auto, Length::ZERO, Length::ZERO),
            justify_self: JustifyItems::End,
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_grid_negative_lines_resolve_from_explicit_grid_end() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(70.0),
        height: Length::points(20.0),
        grid_template_columns: vec![
            Length::points(10.0),
            Length::points(20.0),
            Length::points(30.0),
        ],
        grid_template_rows: vec![Length::points(20.0)],
        column_gap: Length::points(5.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(-3),
        grid_column_end: Some(-1),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(70.0, 20.0));
}

#[test]
fn head_to_head_grid_negative_line_span_permutations() {
    let (tree, root, constraints) = native_grid_negative_line_span_permutation_tree();

    assert_head_to_head_or_skip(tree, root, constraints);
}

#[test]
#[ignore = "current C++ standalone truncates trailing explicit tracks when a positive end line plus span creates leading implicit tracks; Rust follows W3C sections 7.5, 7.6, and 8.5 and preserves the full explicit grid"]
fn head_to_head_records_cpp_gap_for_positive_end_span_leading_implicit_tracks() {
    let (tree, root, constraints) = native_positive_end_span_leading_implicit_tracks_tree();

    assert_head_to_head_or_skip(tree, root, constraints);
}

fn native_grid_negative_line_span_permutation_tree() -> (SimpleTree, usize, Constraints) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(54.0),
        grid_template_columns: vec![
            Length::points(10.0),
            Length::points(20.0),
            Length::points(30.0),
        ],
        grid_template_rows: vec![
            Length::points(8.0),
            Length::points(12.0),
            Length::points(16.0),
        ],
        grid_auto_columns: vec![Length::points(40.0)],
        grid_auto_columns_max: vec![Length::points(40.0)],
        grid_auto_rows: vec![Length::points(18.0)],
        grid_auto_rows_max: vec![Length::points(18.0)],
        justify_content: JustifyContent::FlexStart,
        align_content: AlignContent::FlexStart,
        justify_items: JustifyItems::Stretch,
        align_items: AlignItems::Stretch,
        ..Style::default()
    })));
    let end_span = tree.push(SimpleNode::new(block_standalone_style(Style {
        grid_column_end: Some(-1),
        grid_column_span: 2,
        grid_row_end: Some(-1),
        grid_row_span: 2,
        ..Style::default()
    })));
    let start_span = tree.push(SimpleNode::new(block_standalone_style(Style {
        grid_column_start: Some(-2),
        grid_column_span: 2,
        grid_row_start: Some(-2),
        grid_row_span: 2,
        ..Style::default()
    })));
    let positive_start_negative_end = tree.push(SimpleNode::new(block_standalone_style(Style {
        grid_column_start: Some(2),
        grid_column_end: Some(-1),
        grid_row_start: Some(2),
        grid_row_end: Some(-1),
        ..Style::default()
    })));
    let negative_start_positive_end = tree.push(SimpleNode::new(block_standalone_style(Style {
        grid_column_start: Some(-1),
        grid_column_end: Some(2),
        grid_row_start: Some(-1),
        grid_row_end: Some(2),
        ..Style::default()
    })));
    let positive_end_span = tree.push(SimpleNode::new(block_standalone_style(Style {
        grid_column_end: Some(1),
        grid_column_span: 2,
        grid_row_end: Some(1),
        grid_row_span: 2,
        ..Style::default()
    })));
    tree.append_child(root, end_span);
    tree.append_child(root, start_span);
    tree.append_child(root, positive_start_negative_end);
    tree.append_child(root, negative_start_positive_end);
    tree.append_child(root, positive_end_span);

    (tree, root, Constraints::definite(100.0, 54.0))
}

fn native_positive_end_span_leading_implicit_tracks_tree() -> (SimpleTree, usize, Constraints) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![
            Length::points(10.0),
            Length::points(20.0),
            Length::points(30.0),
        ],
        grid_template_rows: vec![
            Length::points(8.0),
            Length::points(12.0),
            Length::points(16.0),
        ],
        grid_auto_columns: vec![Length::points(6.0)],
        grid_auto_columns_max: vec![Length::points(6.0)],
        grid_auto_rows: vec![Length::points(4.0)],
        grid_auto_rows_max: vec![Length::points(4.0)],
        justify_content: JustifyContent::FlexStart,
        align_content: AlignContent::FlexStart,
        justify_items: JustifyItems::Stretch,
        align_items: AlignItems::Stretch,
        ..Style::default()
    })));
    let leading = tree.push(SimpleNode::new(block_standalone_style(Style {
        grid_column_end: Some(1),
        grid_column_span: 2,
        grid_row_end: Some(1),
        grid_row_span: 2,
        ..Style::default()
    })));
    let explicit_marker = tree.push(SimpleNode::new(block_standalone_style(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, leading);
    tree.append_child(root, explicit_marker);

    (tree, root, Constraints::indefinite())
}

#[test]
fn head_to_head_grid_line_conflict_handling_swaps_reversed_lines_and_drops_equal_end() {
    let (tree, root, constraints) = native_grid_line_conflict_handling_tree();

    assert_head_to_head_or_skip(tree, root, constraints);
}

fn native_grid_line_conflict_handling_tree() -> (SimpleTree, usize, Constraints) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(60.0),
        height: Length::points(36.0),
        grid_template_columns: vec![
            Length::points(10.0),
            Length::points(20.0),
            Length::points(30.0),
        ],
        grid_template_rows: vec![
            Length::points(8.0),
            Length::points(12.0),
            Length::points(16.0),
        ],
        justify_items: JustifyItems::Stretch,
        align_items: AlignItems::Stretch,
        ..Style::default()
    })));
    let reversed = tree.push(SimpleNode::new(block_standalone_style(Style {
        grid_column_start: Some(4),
        grid_column_end: Some(2),
        grid_row_start: Some(3),
        grid_row_end: Some(1),
        ..Style::default()
    })));
    let equal = tree.push(SimpleNode::new(block_standalone_style(Style {
        grid_column_start: Some(2),
        grid_column_end: Some(2),
        grid_row_start: Some(2),
        grid_row_end: Some(2),
        ..Style::default()
    })));
    tree.append_child(root, reversed);
    tree.append_child(root, equal);

    (tree, root, Constraints::definite(60.0, 36.0))
}

#[test]
fn head_to_head_grid_positive_implicit_columns_repeat_auto_track_pattern() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::points(10.0)],
        grid_auto_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_auto_columns_max: vec![Length::points(20.0), Length::points(30.0)],
        ..Style::default()
    })));
    for column in 1..=3 {
        let child = tree.push(SimpleNode::new(standalone_style(Style {
            grid_column_start: Some(column),
            grid_row_start: Some(1),
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_grid_leading_implicit_columns_align_auto_track_pattern() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::points(10.0)],
        grid_auto_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_auto_columns_max: vec![Length::points(20.0), Length::points(30.0)],
        ..Style::default()
    })));
    let leading_span = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(-4),
        grid_column_end: Some(-2),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    let explicit = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, leading_span);
    tree.append_child(root, explicit);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_grid_positive_implicit_rows_repeat_auto_track_pattern() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::points(10.0)],
        grid_auto_rows: vec![Length::points(20.0), Length::points(30.0)],
        grid_auto_rows_max: vec![Length::points(20.0), Length::points(30.0)],
        ..Style::default()
    })));
    for row in 1..=3 {
        let child = tree.push(SimpleNode::new(standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(row),
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_grid_leading_implicit_rows_align_auto_track_pattern() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::points(10.0)],
        grid_auto_rows: vec![Length::points(20.0), Length::points(30.0)],
        grid_auto_rows_max: vec![Length::points(20.0), Length::points(30.0)],
        ..Style::default()
    })));
    let leading_span = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(-4),
        grid_row_end: Some(-2),
        ..Style::default()
    })));
    let explicit = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, leading_span);
    tree.append_child(root, explicit);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_definite_grid_auto_fit_content_percent_column_max_caps_track() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(120.0),
        height: Length::points(10.0),
        grid_auto_columns: vec![Length::points(20.0), Length::points(10.0)],
        grid_auto_columns_max: vec![
            Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 50.0))),
            Length::points(60.0),
        ],
        grid_auto_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(90.0, 10.0),
    ));
    let following = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, intrinsic);
    tree.append_child(root, following);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(120.0, 10.0));
}

#[test]
fn head_to_head_definite_grid_auto_fit_content_calc_column_max_caps_track() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(120.0),
        height: Length::points(10.0),
        grid_auto_columns: vec![Length::points(20.0), Length::points(10.0)],
        grid_auto_columns_max: vec![
            Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
            Length::points(50.0),
        ],
        grid_auto_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(90.0, 10.0),
    ));
    let following = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, intrinsic);
    tree.append_child(root, following);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(120.0, 10.0));
}

#[test]
fn head_to_head_definite_grid_auto_fit_content_percent_row_max_caps_track() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(10.0),
        height: Length::points(120.0),
        grid_auto_columns: vec![Length::points(10.0)],
        grid_auto_rows: vec![Length::points(20.0), Length::points(10.0)],
        grid_auto_rows_max: vec![
            Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 50.0))),
            Length::points(60.0),
        ],
        ..Style::default()
    })));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(10.0, 90.0),
    ));
    let following = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(2),
        ..Style::default()
    })));
    tree.append_child(root, intrinsic);
    tree.append_child(root, following);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(10.0, 120.0));
}

#[test]
fn head_to_head_definite_grid_auto_fit_content_calc_row_max_caps_track() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(10.0),
        height: Length::points(120.0),
        grid_auto_columns: vec![Length::points(10.0)],
        grid_auto_rows: vec![Length::points(20.0), Length::points(10.0)],
        grid_auto_rows_max: vec![
            Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
            Length::points(50.0),
        ],
        ..Style::default()
    })));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(10.0, 90.0),
    ));
    let following = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(2),
        ..Style::default()
    })));
    tree.append_child(root, intrinsic);
    tree.append_child(root, following);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(10.0, 120.0));
}

#[test]
fn head_to_head_indefinite_grid_auto_fit_content_column_max_caps_track() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_auto_columns: vec![Length::points(20.0), Length::points(10.0)],
        grid_auto_columns_max: vec![
            Length::fit_content(Some(BaseLength::fixed(40.0))),
            Length::points(10.0),
        ],
        grid_auto_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(70.0, 10.0),
    ));
    let following = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, intrinsic);
    tree.append_child(root, following);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_indefinite_grid_auto_fit_content_row_max_caps_track() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_auto_columns: vec![Length::points(10.0)],
        grid_auto_rows: vec![Length::points(20.0), Length::points(10.0)],
        grid_auto_rows_max: vec![
            Length::fit_content(Some(BaseLength::fixed(40.0))),
            Length::points(10.0),
        ],
        ..Style::default()
    })));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(10.0, 70.0),
    ));
    let following = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(2),
        ..Style::default()
    })));
    tree.append_child(root, intrinsic);
    tree.append_child(root, following);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
#[ignore = "current C++ standalone truncates trailing explicit column tracks when a negative line creates leading implicit tracks; Rust follows W3C sections 7.5 and 7.6 and preserves the full explicit grid"]
fn head_to_head_records_cpp_gap_for_negative_column_line_before_explicit_grid() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![
            Length::points(10.0),
            Length::points(20.0),
            Length::points(30.0),
        ],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    let leading = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(-5),
            grid_column_end: Some(-4),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(12.0, 10.0),
    ));
    let explicit = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, leading);
    tree.append_child(root, explicit);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
#[ignore = "current C++ standalone truncates trailing explicit row tracks when a negative line creates leading implicit tracks; Rust follows W3C sections 7.5 and 7.6 and preserves the full explicit grid"]
fn head_to_head_records_cpp_gap_for_negative_row_line_before_explicit_grid() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::points(10.0), Length::points(20.0)],
        ..Style::default()
    })));
    let leading = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(-4),
            grid_row_end: Some(-3),
            ..Style::default()
        }),
        Size::new(10.0, 8.0),
    ));
    let explicit = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, leading);
    tree.append_child(root, explicit);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_absolute_grid_item_uses_grid_area_as_containing_block_for_insets() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0), Length::points(20.0)],
        column_gap: Length::points(5.0),
        row_gap: Length::points(5.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        position: PositionType::Absolute,
        grid_column_start: Some(2),
        grid_column_end: Some(3),
        grid_row_start: Some(1),
        grid_row_end: Some(3),
        left: Length::points(2.0),
        right: Length::points(3.0),
        top: Length::points(1.0),
        bottom: Length::points(4.0),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_absolute_grid_item_percent_calc_oversized_paired_insets_keep_definite_measure_mode()
{
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::points(30.0), Length::points(20.0)],
        grid_template_rows: vec![Length::points(20.0)],
        column_gap: Length::points(4.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measure_func(
        block_standalone_style(Style {
            position: PositionType::Absolute,
            grid_column_start: Some(1),
            grid_column_end: Some(2),
            grid_row_start: Some(1),
            grid_row_end: Some(2),
            left: Length::percent(90.0),
            right: Length::calc(4.0, 50.0),
            top: Length::points(2.0),
            margin: Rect::new(
                Length::points(2.0),
                Length::points(3.0),
                Length::points(1.0),
                Length::points(1.0),
            ),
            ..Style::default()
        }),
        width_mode_sensitive_height_measure,
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_absolute_grid_item_uses_grid_alignment_when_insets_are_auto() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::points(50.0)],
        grid_template_rows: vec![Length::points(40.0)],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            position: PositionType::Absolute,
            grid_column_start: Some(1),
            grid_column_end: Some(2),
            grid_row_start: Some(1),
            grid_row_end: Some(2),
            justify_self: JustifyItems::End,
            align_self: Some(AlignItems::Center),
            ..Style::default()
        }),
        Size::new(10.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_absolute_grid_item_excludes_trailing_gutter_from_internal_end_line() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(5.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        position: PositionType::Absolute,
        grid_column_start: Some(1),
        grid_column_end: Some(2),
        grid_row_start: Some(1),
        grid_row_end: Some(2),
        left: Length::ZERO,
        right: Length::ZERO,
        top: Length::ZERO,
        bottom: Length::ZERO,
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_rtl_absolute_grid_item_uses_right_origin_grid_area() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        direction: Direction::Rtl,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(10.0),
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            position: PositionType::Absolute,
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(5.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_rtl_absolute_grid_item_tracks_justify_content_end_area_shift() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        direction: Direction::Rtl,
        justify_content: JustifyContent::End,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(10.0),
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            position: PositionType::Absolute,
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(5.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_rtl_absolute_grid_item_left_inset_remains_physical_left_offset() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        direction: Direction::Rtl,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(10.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            position: PositionType::Absolute,
            grid_column_start: Some(1),
            grid_column_end: Some(2),
            grid_row_start: Some(1),
            left: Length::points(2.0),
            ..Style::default()
        }),
        Size::new(5.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_rtl_absolute_grid_item_right_inset_uses_physical_right_offset() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        direction: Direction::Rtl,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(10.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            position: PositionType::Absolute,
            grid_column_start: Some(1),
            grid_column_end: Some(2),
            grid_row_start: Some(1),
            right: Length::points(2.0),
            ..Style::default()
        }),
        Size::new(5.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_fixed_grid_item_uses_grid_area_without_root_fixed_pass_overwrite() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(5.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        position: PositionType::Fixed,
        grid_column_start: Some(2),
        grid_column_end: Some(3),
        grid_row_start: Some(1),
        grid_row_end: Some(2),
        left: Length::points(2.0),
        right: Length::points(3.0),
        top: Length::points(1.0),
        bottom: Length::points(4.0),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_fixed_grid_item_under_non_root_grid_uses_root_fixed_containing_block() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(340.0),
        height: Length::points(568.0),
        ..Style::default()
    })));
    let grid = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(320.0),
        height: Length::points(64.0),
        padding: Rect::new(
            Length::points(4.0),
            Length::points(5.0),
            Length::points(6.0),
            Length::points(7.0),
        ),
        border: Rect::new(1.0, 2.0, 3.0, 4.0),
        grid_template_columns: vec![Length::points(24.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(18.0), Length::points(20.0)],
        column_gap: Length::points(5.0),
        row_gap: Length::points(3.0),
        justify_content: JustifyContent::Center,
        align_content: AlignContent::Center,
        justify_items: JustifyItems::Center,
        align_items: AlignItems::Center,
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Fixed,
        grid_column_start: Some(1),
        grid_column_end: Some(2),
        grid_row_start: Some(1),
        grid_row_end: Some(2),
        left: Length::percent(10.0),
        right: Length::calc(2.0, 15.0),
        top: Length::calc(1.0, 20.0),
        bottom: Length::percent(12.0),
        margin: Rect::new(
            Length::points(1.0),
            Length::ZERO,
            Length::ZERO,
            Length::points(1.0),
        ),
        padding: Rect::all(Length::points(1.0)),
        border: Rect::all(1.0),
        ..Style::default()
    })));
    tree.append_child(root, grid);
    tree.append_child(grid, fixed);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(340.0),
            SideConstraint::indefinite(),
        ),
    );
}

#[test]
fn head_to_head_absolute_grid_item_max_content_uses_grid_area_and_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::points(50.0), Length::points(50.0)],
        grid_template_rows: vec![Length::points(40.0)],
        column_gap: Length::points(5.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Absolute,
        grid_column_start: Some(1),
        grid_column_end: Some(2),
        grid_row_start: Some(1),
        grid_row_end: Some(2),
        width: Length::MaxContent,
        height: Length::MaxContent,
        left: Length::points(3.0),
        top: Length::points(4.0),
        ..Style::default()
    })));
    let grandchild = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(70.0),
        height: Length::points(25.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, grandchild);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_fixed_grid_item_max_content_uses_grid_area_and_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::points(50.0), Length::points(50.0)],
        grid_template_rows: vec![Length::points(40.0)],
        column_gap: Length::points(5.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Fixed,
        grid_column_start: Some(2),
        grid_column_end: Some(3),
        grid_row_start: Some(1),
        grid_row_end: Some(2),
        width: Length::MaxContent,
        height: Length::MaxContent,
        left: Length::points(3.0),
        top: Length::points(4.0),
        ..Style::default()
    })));
    let grandchild = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(70.0),
        height: Length::points(25.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, grandchild);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_absolute_grid_item_fit_content_uses_grid_area_and_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::points(50.0), Length::points(50.0)],
        grid_template_rows: vec![Length::points(40.0)],
        column_gap: Length::points(5.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Absolute,
        grid_column_start: Some(1),
        grid_column_end: Some(2),
        grid_row_start: Some(1),
        grid_row_end: Some(2),
        width: Length::fit_content(Some(BaseLength::fixed(40.0))),
        height: Length::fit_content(Some(BaseLength::fixed(15.0))),
        left: Length::points(3.0),
        top: Length::points(4.0),
        ..Style::default()
    })));
    let grandchild = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(70.0),
        height: Length::points(25.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, grandchild);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_fixed_grid_item_fit_content_uses_grid_area_and_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::points(50.0), Length::points(50.0)],
        grid_template_rows: vec![Length::points(40.0)],
        column_gap: Length::points(5.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Fixed,
        grid_column_start: Some(2),
        grid_column_end: Some(3),
        grid_row_start: Some(1),
        grid_row_end: Some(2),
        width: Length::fit_content(Some(BaseLength::fixed(40.0))),
        height: Length::fit_content(Some(BaseLength::fixed(15.0))),
        left: Length::points(3.0),
        top: Length::points(4.0),
        ..Style::default()
    })));
    let grandchild = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(70.0),
        height: Length::points(25.0),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(child, grandchild);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_absolute_measured_grid_item_max_content_uses_grid_area_and_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::points(50.0), Length::points(50.0)],
        grid_template_rows: vec![Length::points(40.0)],
        column_gap: Length::points(5.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            position: PositionType::Absolute,
            grid_column_start: Some(1),
            grid_column_end: Some(2),
            grid_row_start: Some(1),
            grid_row_end: Some(2),
            width: Length::MaxContent,
            height: Length::MaxContent,
            left: Length::points(3.0),
            top: Length::points(4.0),
            ..Style::default()
        }),
        Size::new(70.0, 25.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_fixed_measured_grid_item_max_content_uses_grid_area_and_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::points(50.0), Length::points(50.0)],
        grid_template_rows: vec![Length::points(40.0)],
        column_gap: Length::points(5.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            position: PositionType::Fixed,
            grid_column_start: Some(2),
            grid_column_end: Some(3),
            grid_row_start: Some(1),
            grid_row_end: Some(2),
            width: Length::MaxContent,
            height: Length::MaxContent,
            left: Length::points(3.0),
            top: Length::points(4.0),
            ..Style::default()
        }),
        Size::new(70.0, 25.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_absolute_measured_grid_item_fit_content_uses_grid_area_and_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::points(50.0), Length::points(50.0)],
        grid_template_rows: vec![Length::points(40.0)],
        column_gap: Length::points(5.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            position: PositionType::Absolute,
            grid_column_start: Some(1),
            grid_column_end: Some(2),
            grid_row_start: Some(1),
            grid_row_end: Some(2),
            width: Length::fit_content(Some(BaseLength::fixed(40.0))),
            height: Length::fit_content(Some(BaseLength::fixed(15.0))),
            left: Length::points(3.0),
            top: Length::points(4.0),
            ..Style::default()
        }),
        Size::new(70.0, 25.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_fixed_measured_grid_item_fit_content_uses_grid_area_and_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::points(50.0), Length::points(50.0)],
        grid_template_rows: vec![Length::points(40.0)],
        column_gap: Length::points(5.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            position: PositionType::Fixed,
            grid_column_start: Some(2),
            grid_column_end: Some(3),
            grid_row_start: Some(1),
            grid_row_end: Some(2),
            width: Length::fit_content(Some(BaseLength::fixed(40.0))),
            height: Length::fit_content(Some(BaseLength::fixed(15.0))),
            left: Length::points(3.0),
            top: Length::points(4.0),
            ..Style::default()
        }),
        Size::new(70.0, 25.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_absolute_grid_item_auto_grid_lines_use_container_padding_edges() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        justify_content: JustifyContent::Center,
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        position: PositionType::Absolute,
        left: Length::ZERO,
        right: Length::ZERO,
        top: Length::ZERO,
        bottom: Length::ZERO,
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_absolute_grid_item_auto_lines_use_scrollable_overflow_padding_edges() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(80.0),
        height: Length::points(30.0),
        padding: Rect::new(
            Length::points(3.0),
            Length::points(5.0),
            Length::points(2.0),
            Length::points(4.0),
        ),
        grid_template_columns: vec![Length::points(60.0), Length::points(60.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(20.0)],
        column_gap: Length::points(10.0),
        row_gap: Length::points(5.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        position: PositionType::Absolute,
        left: Length::ZERO,
        right: Length::ZERO,
        top: Length::ZERO,
        bottom: Length::ZERO,
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(80.0, 30.0));
}

#[test]
fn head_to_head_rtl_absolute_grid_item_auto_lines_use_scrollable_overflow_padding_edges() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        direction: Direction::Rtl,
        width: Length::points(80.0),
        height: Length::points(30.0),
        padding: Rect::new(
            Length::points(3.0),
            Length::points(5.0),
            Length::points(2.0),
            Length::points(4.0),
        ),
        grid_template_columns: vec![Length::points(60.0), Length::points(60.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(20.0)],
        column_gap: Length::points(10.0),
        row_gap: Length::points(5.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        position: PositionType::Absolute,
        left: Length::ZERO,
        right: Length::ZERO,
        top: Length::ZERO,
        bottom: Length::ZERO,
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(80.0, 30.0));
}

#[test]
fn head_to_head_absolute_grid_item_last_real_start_line_can_span_to_auto_end_edge() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        justify_content: JustifyContent::Center,
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        position: PositionType::Absolute,
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        grid_row_end: Some(2),
        left: Length::ZERO,
        right: Length::ZERO,
        top: Length::ZERO,
        bottom: Length::ZERO,
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_absolute_grid_item_auto_end_line_uses_padding_edge_for_fit_content_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(120.0),
        height: Length::points(80.0),
        padding: Rect::all(Length::points(4.0)),
        grid_template_columns: vec![Length::points(30.0), Length::points(40.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(30.0)],
        column_gap: Length::points(5.0),
        row_gap: Length::points(3.0),
        justify_content: JustifyContent::Center,
        align_content: AlignContent::Center,
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::new(block_standalone_style(Style {
        position: PositionType::Absolute,
        grid_column_start: Some(2),
        grid_row_start: Some(2),
        width: Length::fit_content(Some(BaseLength::fixed(35.0))),
        height: Length::fit_content(Some(BaseLength::fixed(22.0))),
        justify_self: JustifyItems::Center,
        align_self: Some(AlignItems::Center),
        ..Style::default()
    })));
    let content = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(50.0),
        height: Length::points(20.0),
        ..Style::default()
    })));
    tree.append_child(root, absolute);
    tree.append_child(absolute, content);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(120.0, 80.0));
}

#[test]
fn head_to_head_indefinite_grid_justify_content_uses_container_min_width() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        justify_content: JustifyContent::Center,
        min_width: Length::points(100.0),
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_grid_auto_column_justify_center_offsets_intrinsic_track_group() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        justify_content: JustifyContent::Center,
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::Auto],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_definite_grid_auto_track_caps_intrinsic_growth_to_available_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        justify_content: JustifyContent::Center,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::Auto, Length::points(0.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(120.0, 10.0),
    ));
    let marker = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(root, marker);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_indefinite_grid_stretch_auto_track_uses_container_min_width() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        min_width: Length::points(100.0),
        grid_template_columns: vec![Length::Auto],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_definite_grid_stretch_distributes_free_space_to_auto_max_tracks_only() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(60.0),
        justify_content: JustifyContent::Stretch,
        align_content: AlignContent::Stretch,
        grid_template_columns: vec![
            Length::points(20.0),
            Length::points(20.0),
            Length::points(20.0),
        ],
        grid_template_columns_max: vec![Length::Auto, Length::points(20.0), Length::Auto],
        grid_template_rows: vec![
            Length::points(10.0),
            Length::points(10.0),
            Length::points(10.0),
        ],
        grid_template_rows_max: vec![Length::Auto, Length::points(10.0), Length::Auto],
        ..Style::default()
    })));
    let first_auto = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(2),
        ..Style::default()
    })));
    let second_auto = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(3),
        grid_row_start: Some(3),
        ..Style::default()
    })));
    tree.append_child(root, first_auto);
    tree.append_child(root, fixed);
    tree.append_child(root, second_auto);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 60.0));
}

#[test]
fn head_to_head_indefinite_grid_percentage_column_gap_resolves_after_container_min_width() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        min_width: Length::points(100.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::percent(10.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_indefinite_grid_percentage_row_gap_resolves_after_container_min_height() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        min_height: Length::points(100.0),
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(20.0)],
        row_gap: Length::percent(10.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_grid_align_content_distributes_extra_row_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        align_content: AlignContent::SpaceBetween,
        width: Length::points(20.0),
        height: Length::points(100.0),
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(20.0)],
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(20.0, 100.0));
}

#[test]
fn head_to_head_grid_align_content_start_end_alias_flex_edges() {
    for align_content in [AlignContent::Start, AlignContent::End] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
            align_content,
            width: Length::points(20.0),
            height: Length::points(100.0),
            grid_template_columns: vec![Length::points(20.0)],
            grid_template_rows: vec![Length::points(20.0), Length::points(20.0)],
            ..Style::default()
        })));
        for _ in 0..2 {
            let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
            tree.append_child(root, child);
        }

        assert_head_to_head_or_skip(tree, root, Constraints::definite(20.0, 100.0));
    }
}

#[test]
fn head_to_head_grid_align_content_space_around_offsets_track_group() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        align_content: AlignContent::SpaceAround,
        width: Length::points(20.0),
        height: Length::points(100.0),
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(20.0)],
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(20.0, 100.0));
}

#[test]
fn head_to_head_grid_align_content_space_evenly_offsets_track_group() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        align_content: AlignContent::SpaceEvenly,
        width: Length::points(20.0),
        height: Length::points(100.0),
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(20.0)],
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(20.0, 100.0));
}

#[test]
fn head_to_head_grid_align_content_space_between_keeps_row_gap_when_tracks_overflow() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        align_content: AlignContent::SpaceBetween,
        width: Length::points(20.0),
        height: Length::points(30.0),
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(20.0)],
        row_gap: Length::points(10.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(20.0, 30.0));
}

#[test]
fn head_to_head_grid_align_content_space_around_falls_back_to_start_when_tracks_overflow() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        align_content: AlignContent::SpaceAround,
        width: Length::points(20.0),
        height: Length::points(30.0),
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(20.0)],
        row_gap: Length::points(10.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(20.0, 30.0));
}

#[test]
fn head_to_head_grid_align_content_space_evenly_falls_back_to_start_when_tracks_overflow() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        align_content: AlignContent::SpaceEvenly,
        width: Length::points(20.0),
        height: Length::points(30.0),
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(20.0)],
        row_gap: Length::points(10.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(20.0, 30.0));
}

#[test]
fn head_to_head_grid_justify_content_space_evenly_offsets_track_group() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        justify_content: JustifyContent::SpaceEvenly,
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style::default())));
    let second = tree.push(SimpleNode::new(standalone_style(Style::default())));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_grid_justify_content_space_between_offsets_track_group() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        justify_content: JustifyContent::SpaceBetween,
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style::default())));
    let second = tree.push(SimpleNode::new(standalone_style(Style::default())));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_grid_justify_content_space_around_offsets_track_group() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        justify_content: JustifyContent::SpaceAround,
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style::default())));
    let second = tree.push(SimpleNode::new(standalone_style(Style::default())));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_grid_justify_content_space_between_keeps_column_gap_when_tracks_overflow() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        justify_content: JustifyContent::SpaceBetween,
        width: Length::points(30.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_rows: vec![Length::points(20.0)],
        column_gap: Length::points(10.0),
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style::default())));
    let second = tree.push(SimpleNode::new(standalone_style(Style::default())));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(30.0, 20.0));
}

#[test]
fn head_to_head_grid_justify_content_space_evenly_falls_back_to_start_when_tracks_overflow() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        justify_content: JustifyContent::SpaceEvenly,
        width: Length::points(30.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_rows: vec![Length::points(20.0)],
        column_gap: Length::points(10.0),
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style::default())));
    let second = tree.push(SimpleNode::new(standalone_style(Style::default())));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(30.0, 20.0));
}

#[test]
fn head_to_head_grid_justify_content_space_around_falls_back_to_start_when_tracks_overflow() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        justify_content: JustifyContent::SpaceAround,
        width: Length::points(30.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_rows: vec![Length::points(20.0)],
        column_gap: Length::points(10.0),
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::new(standalone_style(Style::default())));
    let second = tree.push(SimpleNode::new(standalone_style(Style::default())));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(30.0, 20.0));
}

#[test]
fn head_to_head_grid_auto_row_align_center_offsets_intrinsic_track_group() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        align_content: AlignContent::Center,
        width: Length::points(20.0),
        height: Length::points(100.0),
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_rows: vec![Length::Auto],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(20.0, 100.0));
}

#[test]
fn head_to_head_definite_grid_auto_column_caps_track_growth_not_measured_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        justify_content: JustifyContent::Center,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::Auto, Length::points(0.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(120.0, 10.0),
    ));
    let marker = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(root, marker);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_definite_grid_auto_track_uses_definite_child_minimum_contribution() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(30.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::Auto, Length::points(0.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    let definite = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(50.0),
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    })));
    let marker = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    })));
    tree.append_child(root, definite);
    tree.append_child(root, marker);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(30.0, 10.0));
}

#[test]
fn head_to_head_definite_grid_fixed_min_intrinsic_max_updates_growth_limit_without_base_growth() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        justify_content: JustifyContent::Start,
        grid_template_columns: vec![Length::points(10.0), Length::points(0.0)],
        grid_template_columns_max: vec![Length::MaxContent, Length::points(0.0)],
        grid_template_rows: vec![Length::points(10.0)],
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let intrinsic_max = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(40.0),
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    let marker = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    })));
    tree.append_child(root, intrinsic_max);
    tree.append_child(root, marker);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(20.0, 10.0));
}

#[test]
fn head_to_head_definite_grid_minmax_auto_fixed_track_uses_intrinsic_minimum() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(30.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::Auto, Length::points(0.0)],
        grid_template_columns_max: vec![Length::points(50.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    let definite = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(45.0),
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    })));
    let marker = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    })));
    tree.append_child(root, definite);
    tree.append_child(root, marker);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(30.0, 10.0));
}

#[test]
fn head_to_head_grid_root_at_most_does_not_cap_intrinsic_fixed_max_track_growth() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        justify_content: JustifyContent::Start,
        align_content: AlignContent::FlexStart,
        align_items: AlignItems::FlexStart,
        justify_items: JustifyItems::Start,
        grid_template_columns: vec![Length::Auto, Length::Auto, Length::Auto],
        grid_template_columns_max: vec![Length::points(70.0), Length::points(90.0), Length::Auto],
        grid_template_rows: vec![Length::Auto, Length::points(18.0)],
        grid_template_rows_max: vec![Length::Auto, Length::points(18.0)],
        column_gap: Length::points(7.0),
        row_gap: Length::points(5.0),
        padding: Rect::new(
            Length::points(4.0),
            Length::points(6.0),
            Length::points(3.0),
            Length::points(2.0),
        ),
        border: Rect::new(1.0, 2.0, 1.0, 2.0),
        ..Style::default()
    })));
    let first_span = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            grid_column_start: Some(1),
            grid_column_span: 2,
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(108.0, 46.0),
    ));
    let second_span = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            grid_column_start: Some(2),
            grid_column_span: 2,
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(96.0, 54.0),
    ));
    let full_span = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            grid_column_start: Some(1),
            grid_column_span: 3,
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(176.0, 72.0),
    ));
    let definite = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(64.0),
        height: Length::points(28.0),
        margin: Rect::new(
            Length::points(3.0),
            Length::points(5.0),
            Length::points(2.0),
            Length::points(4.0),
        ),
        grid_column_start: Some(3),
        grid_row_start: Some(1),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    })));
    tree.append_child(root, first_span);
    tree.append_child(root, second_span);
    tree.append_child(root, full_span);
    tree.append_child(root, definite);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::at_most(190.0),
            SideConstraint::at_most(110.0),
        ),
    );
}

#[test]
fn head_to_head_definite_grid_auto_row_caps_track_growth_not_measured_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        align_content: AlignContent::Center,
        width: Length::points(10.0),
        height: Length::points(100.0),
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::Auto, Length::points(0.0)],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(10.0, 120.0),
    ));
    let marker = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(2),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    })));
    tree.append_child(root, child);
    tree.append_child(root, marker);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(10.0, 100.0));
}

#[test]
fn head_to_head_grid_span_uses_combined_tracks_and_gap() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(120.0),
        height: Length::points(40.0),
        grid_template_columns: vec![Length::percent(50.0), Length::Auto],
        grid_template_rows: vec![Length::points(40.0)],
        column_gap: Length::points(10.0),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_span: 2,
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(120.0, 40.0));
}

#[test]
fn head_to_head_auto_grid_rows_grow_from_measured_children_when_container_height_is_indefinite() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(100.0),
        grid_template_columns: vec![Length::points(50.0), Length::points(50.0)],
        ..Style::default()
    })));
    let first = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style::default()),
        Size::new(30.0, 12.0),
    ));
    let second = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style::default()),
        Size::new(20.0, 25.0),
    ));
    tree.append_child(root, first);
    tree.append_child(root, second);

    assert_head_to_head_or_skip(
        tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );
}

#[test]
#[ignore = "current C++ standalone keeps the initial measured block contribution after column sizing; Rust follows W3C section 11.1 and recomputes it from the resolved inline space"]
fn head_to_head_records_cpp_gap_for_grid_auto_rows_use_column_sized_measured_block_contribution() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::Auto],
        grid_template_rows: vec![Length::Auto],
        ..Style::default()
    })));
    let child = tree.push(MeasuringNode::height_from_width(
        standalone_style(Style::default()),
        80.0,
        10.0,
        0.5,
    ));
    tree.append_child(root, child);

    assert_measuring_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_grid_intrinsic_growth_processes_shorter_spans_before_longer_spans() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::Auto, Length::Auto, Length::Auto],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    let wide_span = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_column_span: 3,
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(90.0, 10.0),
    ));
    let single_span = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(50.0, 10.0),
    ));
    tree.append_child(root, wide_span);
    tree.append_child(root, single_span);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_grid_intrinsic_growth_batches_equal_span_planned_increases() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::Auto, Length::Auto, Length::Auto],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    let first_span = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_column_span: 2,
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(100.0, 10.0),
    ));
    let second_span = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(2),
            grid_column_span: 2,
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(100.0, 10.0),
    ));
    let marker = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(3),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::ZERO,
    ));
    tree.append_child(root, first_span);
    tree.append_child(root, second_span);
    tree.append_child(root, marker);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_grid_intrinsic_growth_planned_increases_are_source_order_independent() {
    for first_then_second in [true, false] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
            grid_template_columns: vec![Length::Auto, Length::Auto, Length::Auto],
            grid_template_rows: vec![Length::points(10.0)],
            ..Style::default()
        })));
        let first_span = tree.push(SimpleNode::with_measured_size(
            standalone_style(Style {
                grid_column_start: Some(1),
                grid_column_span: 2,
                grid_row_start: Some(1),
                justify_self: JustifyItems::Start,
                align_self: Some(AlignItems::FlexStart),
                ..Style::default()
            }),
            Size::new(80.0, 10.0),
        ));
        let second_span = tree.push(SimpleNode::with_measured_size(
            standalone_style(Style {
                grid_column_start: Some(2),
                grid_column_span: 2,
                grid_row_start: Some(1),
                justify_self: JustifyItems::Start,
                align_self: Some(AlignItems::FlexStart),
                ..Style::default()
            }),
            Size::new(100.0, 10.0),
        ));
        if first_then_second {
            tree.append_child(root, first_span);
            tree.append_child(root, second_span);
        } else {
            tree.append_child(root, second_span);
            tree.append_child(root, first_span);
        }

        assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
    }
}

#[test]
fn head_to_head_grid_row_intrinsic_growth_planned_increases_are_source_order_independent() {
    for first_then_second in [true, false] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
            grid_template_columns: vec![Length::points(10.0)],
            grid_template_rows: vec![Length::Auto, Length::Auto, Length::Auto],
            ..Style::default()
        })));
        let first_span = tree.push(SimpleNode::with_measured_size(
            standalone_style(Style {
                grid_column_start: Some(1),
                grid_row_start: Some(1),
                grid_row_span: 2,
                justify_self: JustifyItems::Start,
                align_self: Some(AlignItems::FlexStart),
                ..Style::default()
            }),
            Size::new(10.0, 80.0),
        ));
        let second_span = tree.push(SimpleNode::with_measured_size(
            standalone_style(Style {
                grid_column_start: Some(1),
                grid_row_start: Some(2),
                grid_row_span: 2,
                justify_self: JustifyItems::Start,
                align_self: Some(AlignItems::FlexStart),
                ..Style::default()
            }),
            Size::new(10.0, 100.0),
        ));
        if first_then_second {
            tree.append_child(root, first_span);
            tree.append_child(root, second_span);
        } else {
            tree.append_child(root, second_span);
            tree.append_child(root, first_span);
        }

        assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
    }
}

#[test]
fn head_to_head_grid_spanning_auto_minimum_redistributes_after_fixed_growth_limit() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::Auto, Length::Auto],
        grid_template_columns_max: vec![Length::points(40.0), Length::Auto],
        grid_template_rows: vec![Length::points(10.0), Length::points(10.0)],
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let spanning = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_column_span: 2,
            grid_row_start: Some(1),
            min_width: Length::points(100.0),
            ..Style::default()
        }),
        Size::new(100.0, 10.0),
    ));
    let capped_marker = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(2),
        width: Length::percent(100.0),
        ..Style::default()
    })));
    let uncapped_marker = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(2),
        width: Length::percent(100.0),
        ..Style::default()
    })));
    tree.append_child(root, spanning);
    tree.append_child(root, capped_marker);
    tree.append_child(root, uncapped_marker);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
#[ignore = "current C++ standalone skips W3C section 11.5.1 non-affected track redistribution and grows affected tracks beyond their limits"]
fn head_to_head_grid_spanning_minimum_uses_non_affected_track_before_exceeding_limits() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(40.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::Auto, Length::points(0.0), Length::Auto],
        grid_template_columns_max: vec![
            Length::points(20.0),
            Length::points(50.0),
            Length::points(20.0),
        ],
        grid_template_rows: vec![Length::points(10.0), Length::points(10.0)],
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let spanning = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_column_span: 3,
            grid_row_start: Some(1),
            min_width: Length::points(70.0),
            ..Style::default()
        }),
        Size::new(70.0, 10.0),
    ));
    let middle_marker = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(2),
        width: Length::percent(100.0),
        ..Style::default()
    })));
    let trailing_marker = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(3),
        grid_row_start: Some(2),
        width: Length::percent(100.0),
        ..Style::default()
    })));
    for child in [spanning, middle_marker, trailing_marker] {
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(40.0, 20.0));
}

#[test]
fn head_to_head_grid_spanning_minimum_continues_beyond_fixed_growth_limits() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(40.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::Auto, Length::Auto],
        grid_template_columns_max: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_rows: vec![Length::points(10.0), Length::points(10.0)],
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let spanning = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_column_span: 2,
            grid_row_start: Some(1),
            min_width: Length::points(70.0),
            ..Style::default()
        }),
        Size::new(70.0, 10.0),
    ));
    let first_marker = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(2),
        width: Length::percent(100.0),
        ..Style::default()
    })));
    let second_marker = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(2),
        width: Length::percent(100.0),
        ..Style::default()
    })));
    for child in [spanning, first_marker, second_marker] {
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(40.0, 20.0));
}

#[test]
fn head_to_head_grid_fr_tracks_freeze_large_base_sizes_when_finding_fr_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(80.0), Length::fr(1.0)],
        grid_template_columns_max: vec![Length::fr(1.0)],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_grid_fr_size_restarts_after_each_large_base_freeze() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(180.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(80.0), Length::points(70.0), Length::Auto],
        grid_template_columns_max: vec![Length::fr(1.0), Length::fr(1.0), Length::fr(1.0)],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    })));
    for _ in 0..3 {
        let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(180.0, 20.0));
}

#[test]
fn head_to_head_grid_fr_size_uses_spanning_intrinsic_growth_as_base_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(180.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::Auto, Length::Auto, Length::Auto],
        grid_template_columns_max: vec![Length::fr(1.0), Length::fr(1.0), Length::fr(1.0)],
        grid_template_rows: vec![Length::points(10.0), Length::points(10.0)],
        ..Style::default()
    })));
    let spanning = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_column_end: Some(3),
            grid_row_start: Some(1),
            min_width: Length::points(160.0),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(160.0, 10.0),
    ));
    let first = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(2),
        ..Style::default()
    })));
    let second = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(2),
        ..Style::default()
    })));
    let third = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(3),
        grid_row_start: Some(2),
        ..Style::default()
    })));
    tree.append_child(root, spanning);
    tree.append_child(root, first);
    tree.append_child(root, second);
    tree.append_child(root, third);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(180.0, 20.0));
}

#[test]
fn head_to_head_grid_fr_tracks_flex_factor_sum_below_one_leaves_remaining_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::fr(0.25), Length::fr(0.25)],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 20.0));
}

#[test]
fn head_to_head_indefinite_grid_fr_columns_expand_to_container_min_width() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        min_width: Length::points(120.0),
        grid_template_columns: vec![Length::fr(1.0), Length::fr(2.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    for column in 1..=2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style {
            grid_column_start: Some(column),
            grid_row_start: Some(1),
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_max_content_grid_width_expands_fr_tracks_from_item_contribution() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::MaxContent,
        grid_template_columns: vec![Length::fr(1.0), Length::fr(1.0), Length::points(0.0)],
        grid_template_rows: vec![Length::points(10.0)],
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            min_width: Length::points(30.0),
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(80.0, 10.0),
    ));
    let trailing_marker = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(3),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, intrinsic);
    tree.append_child(root, trailing_marker);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_indefinite_grid_fr_columns_redo_flex_fraction_with_container_max_width() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        max_width: Length::points(100.0),
        grid_template_columns: vec![Length::fr(1.0), Length::fr(1.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    let spanning = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_column_span: 2,
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(200.0, 10.0),
    ));
    tree.append_child(root, spanning);
    for column in 1..=2 {
        let marker = tree.push(SimpleNode::new(standalone_style(Style {
            grid_column_start: Some(column),
            grid_row_start: Some(1),
            ..Style::default()
        })));
        tree.append_child(root, marker);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_indefinite_grid_fr_rows_expand_to_container_min_height() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        min_height: Length::points(90.0),
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::fr(1.0), Length::fr(2.0)],
        ..Style::default()
    })));
    for row in 1..=2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(row),
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_indefinite_grid_fr_tracks_expand_from_existing_flex_base() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::fr(2.0), Length::fr(1.0)],
        grid_template_rows: vec![Length::points(10.0), Length::points(10.0)],
        ..Style::default()
    })));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(100.0, 10.0),
    ));
    let sibling_fr = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(2),
        ..Style::default()
    })));
    tree.append_child(root, intrinsic);
    tree.append_child(root, sibling_fr);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_indefinite_grid_spanning_fr_item_distributes_growth_by_flex_factor() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::fr(1.0), Length::fr(2.0)],
        grid_template_rows: vec![Length::points(10.0), Length::points(10.0)],
        ..Style::default()
    })));
    let spanning = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_column_end: Some(3),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(90.0, 10.0),
    ));
    let one_fr = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(2),
        ..Style::default()
    })));
    let two_fr = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(2),
        ..Style::default()
    })));
    tree.append_child(root, spanning);
    tree.append_child(root, one_fr);
    tree.append_child(root, two_fr);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
#[ignore = "current C++ standalone distributes all flexible spanning extra space by flex/sum when the spanned flex factor sum is below one, instead of distributing the remaining space equally per W3C section 11.5.1"]
fn head_to_head_indefinite_grid_spanning_fr_item_with_flex_sum_below_one_distributes_remainder_equally(
) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::fr(0.1), Length::fr(0.3)],
        grid_template_rows: vec![Length::points(10.0), Length::points(10.0)],
        ..Style::default()
    })));
    let spanning = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_column_end: Some(3),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(100.0, 10.0),
    ));
    let one_fr = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(2),
        ..Style::default()
    })));
    let three_fr = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(2),
        ..Style::default()
    })));
    tree.append_child(root, spanning);
    tree.append_child(root, one_fr);
    tree.append_child(root, three_fr);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
#[ignore = "current C++ standalone leaves this indefinite mixed fr/fixed intrinsic span at the non-flexible intrinsic width; Rust expands the spanned flexible tracks per W3C sections 11.5.1 and 11.7"]
fn head_to_head_indefinite_grid_mixed_fr_fixed_intrinsic_spans() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        justify_content: JustifyContent::Start,
        align_content: AlignContent::FlexStart,
        align_items: AlignItems::FlexStart,
        justify_items: JustifyItems::Start,
        min_height: Length::points(90.0),
        max_height: Length::points(150.0),
        grid_template_columns: vec![Length::Auto, Length::Auto, Length::Auto],
        grid_template_columns_max: vec![Length::fr(1.0), Length::fr(2.0), Length::Auto],
        grid_template_rows: vec![Length::Auto, Length::points(18.0)],
        grid_template_rows_max: vec![Length::Auto, Length::points(18.0)],
        column_gap: Length::points(7.0),
        row_gap: Length::points(5.0),
        padding: Rect::new(
            Length::points(4.0),
            Length::points(6.0),
            Length::points(3.0),
            Length::points(2.0),
        ),
        border: Rect::new(1.0, 2.0, 1.0, 2.0),
        ..Style::default()
    })));
    let first_span = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            grid_column_start: Some(1),
            grid_column_span: 2,
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(108.0, 46.0),
    ));
    let second_span = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            grid_column_start: Some(2),
            grid_column_span: 2,
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(96.0, 54.0),
    ));
    let full_span = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            grid_column_start: Some(1),
            grid_column_span: 3,
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(176.0, 72.0),
    ));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(64.0),
        height: Length::points(28.0),
        margin: Rect::new(
            Length::points(3.0),
            Length::points(5.0),
            Length::points(2.0),
            Length::points(4.0),
        ),
        grid_column_start: Some(3),
        grid_row_start: Some(1),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    })));
    let auto_marker = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(18.0),
        height: Length::points(9.0),
        grid_row_start: Some(2),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    })));
    tree.append_child(root, first_span);
    tree.append_child(root, second_span);
    tree.append_child(root, full_span);
    tree.append_child(root, fixed);
    tree.append_child(root, auto_marker);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
#[ignore = "current C++ standalone lets spanning fit-content max tracks keep receiving growth-limit space after reaching the fit-content argument; Rust follows W3C section 11.5.1 and treats them as fixed at that point"]
fn head_to_head_indefinite_grid_fit_content_max_spanning_items_grow_base_tracks() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        justify_content: JustifyContent::Start,
        align_content: AlignContent::FlexStart,
        align_items: AlignItems::FlexStart,
        justify_items: JustifyItems::Start,
        grid_template_columns: vec![Length::Auto, Length::Auto, Length::Auto],
        grid_template_columns_max: vec![
            Length::fit_content(Some(BaseLength::fixed(70.0))),
            Length::fit_content(Some(BaseLength::fixed(90.0))),
            Length::Auto,
        ],
        grid_template_rows: vec![Length::Auto, Length::points(18.0)],
        grid_template_rows_max: vec![Length::Auto, Length::points(18.0)],
        column_gap: Length::points(7.0),
        row_gap: Length::points(5.0),
        padding: Rect::new(
            Length::points(4.0),
            Length::points(6.0),
            Length::points(3.0),
            Length::points(2.0),
        ),
        border: Rect::new(1.0, 2.0, 1.0, 2.0),
        ..Style::default()
    })));
    let first_span = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            grid_column_start: Some(1),
            grid_column_span: 2,
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(108.0, 46.0),
    ));
    let second_span = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            grid_column_start: Some(2),
            grid_column_span: 2,
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(96.0, 54.0),
    ));
    let full_span = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            grid_column_start: Some(1),
            grid_column_span: 3,
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(176.0, 72.0),
    ));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(64.0),
        height: Length::points(28.0),
        margin: Rect::new(
            Length::points(3.0),
            Length::points(5.0),
            Length::points(2.0),
            Length::points(4.0),
        ),
        grid_column_start: Some(3),
        grid_row_start: Some(1),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    })));
    let auto_marker = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(18.0),
        height: Length::points(9.0),
        grid_row_start: Some(2),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    })));
    tree.append_child(root, first_span);
    tree.append_child(root, second_span);
    tree.append_child(root, full_span);
    tree.append_child(root, fixed);
    tree.append_child(root, auto_marker);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_indefinite_grid_row_fixed_max_spanning_items_respect_max_height() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        justify_content: JustifyContent::Start,
        align_content: AlignContent::FlexStart,
        align_items: AlignItems::FlexStart,
        justify_items: JustifyItems::Start,
        min_width: Length::points(150.0),
        max_width: Length::points(280.0),
        min_height: Length::points(90.0),
        max_height: Length::points(180.0),
        grid_template_columns: vec![Length::Auto, Length::points(18.0)],
        grid_template_columns_max: vec![Length::Auto, Length::points(18.0)],
        grid_template_rows: vec![Length::Auto, Length::Auto, Length::Auto],
        grid_template_rows_max: vec![Length::points(70.0), Length::points(90.0), Length::Auto],
        column_gap: Length::points(7.0),
        row_gap: Length::points(5.0),
        padding: Rect::new(
            Length::points(4.0),
            Length::points(6.0),
            Length::points(3.0),
            Length::points(2.0),
        ),
        border: Rect::new(1.0, 2.0, 1.0, 2.0),
        ..Style::default()
    })));
    let first_span = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            grid_row_start: Some(1),
            grid_row_span: 2,
            grid_column_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(46.0, 108.0),
    ));
    let second_span = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            grid_row_start: Some(2),
            grid_row_span: 2,
            grid_column_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(54.0, 96.0),
    ));
    let full_span = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            grid_row_start: Some(1),
            grid_row_span: 3,
            grid_column_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(72.0, 176.0),
    ));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(28.0),
        height: Length::points(64.0),
        margin: Rect::new(
            Length::points(3.0),
            Length::points(5.0),
            Length::points(2.0),
            Length::points(4.0),
        ),
        grid_row_start: Some(3),
        grid_column_start: Some(1),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    })));
    let auto_marker = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(9.0),
        height: Length::points(18.0),
        grid_column_start: Some(2),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    })));
    tree.append_child(root, first_span);
    tree.append_child(root, second_span);
    tree.append_child(root, full_span);
    tree.append_child(root, fixed);
    tree.append_child(root, auto_marker);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_grid_minmax_tracks_use_fr_size_from_available_track_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(140.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_columns_max: vec![Length::fr(1.0), Length::fr(2.0)],
        grid_template_rows: vec![Length::points(20.0)],
        column_gap: Length::points(10.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(140.0, 20.0));
}

#[test]
fn head_to_head_grid_minmax_fixed_max_tracks_share_definite_free_space_up_to_limits() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_columns_max: vec![Length::points(50.0), Length::points(60.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_grid_maximize_tracks_resolves_percent_and_calc_growth_limits_before_redistribution()
{
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(150.0),
        height: Length::points(10.0),
        grid_template_columns: vec![
            Length::points(20.0),
            Length::points(20.0),
            Length::points(20.0),
        ],
        grid_template_columns_max: vec![
            Length::calc(10.0, 20.0),
            Length::percent(50.0),
            Length::points(100.0),
        ],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    for _ in 0..3 {
        let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(150.0, 10.0));
}

#[test]
fn head_to_head_grid_maximize_tracks_does_not_grow_indefinite_growth_limits() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(10.0),
        justify_content: JustifyContent::Start,
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_columns_max: vec![Length::Auto, Length::Auto],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    for column in 1..=2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style {
            grid_column_start: Some(column),
            width: Length::percent(100.0),
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_grid_maximize_tracks_subtracts_gaps_from_definite_free_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_columns_max: vec![Length::points(100.0), Length::points(100.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(10.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_grid_maximize_tracks_redistributes_after_fixed_growth_limit_freezes() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_columns_max: vec![Length::points(30.0), Length::points(100.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_indefinite_grid_minmax_fixed_max_tracks_grow_to_limits() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_template_columns_max: vec![Length::points(50.0), Length::points(40.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_indefinite_grid_minmax_fixed_max_tracks_respect_container_max_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        max_width: Length::points(70.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_columns_max: vec![Length::points(60.0), Length::points(60.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_indefinite_grid_max_size_track_redistribution_does_not_subtract_gaps() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        max_width: Length::points(210.0),
        grid_template_columns: vec![Length::Auto, Length::Auto, Length::Auto],
        grid_template_columns_max: vec![Length::points(70.0), Length::points(90.0), Length::Auto],
        grid_template_rows: vec![Length::Auto],
        column_gap: Length::points(7.0),
        ..Style::default()
    })));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(64.0),
        height: Length::points(28.0),
        margin: Rect::new(
            Length::points(3.0),
            Length::points(5.0),
            Length::points(2.0),
            Length::points(4.0),
        ),
        grid_column_start: Some(3),
        grid_row_start: Some(1),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    })));
    let first_marker = tree.push(SimpleNode::new(block_standalone_style(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    })));
    let second_marker = tree.push(SimpleNode::new(block_standalone_style(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    })));
    let third_marker = tree.push(SimpleNode::new(block_standalone_style(Style {
        grid_column_start: Some(3),
        grid_row_start: Some(1),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    })));
    tree.append_child(root, fixed);
    tree.append_child(root, first_marker);
    tree.append_child(root, second_marker);
    tree.append_child(root, third_marker);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_indefinite_grid_auto_minimum_spanning_items_feed_growth_limits_not_base_sizes() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        justify_content: JustifyContent::Start,
        align_content: AlignContent::FlexStart,
        align_items: AlignItems::FlexStart,
        justify_items: JustifyItems::Start,
        min_width: Length::points(150.0),
        max_width: Length::points(210.0),
        min_height: Length::points(90.0),
        max_height: Length::points(150.0),
        grid_template_columns: vec![Length::Auto, Length::Auto, Length::Auto],
        grid_template_columns_max: vec![Length::points(70.0), Length::points(90.0), Length::Auto],
        grid_template_rows: vec![Length::Auto, Length::points(18.0)],
        grid_template_rows_max: vec![Length::Auto, Length::points(18.0)],
        column_gap: Length::points(7.0),
        row_gap: Length::points(5.0),
        padding: Rect::new(
            Length::points(4.0),
            Length::points(6.0),
            Length::points(3.0),
            Length::points(2.0),
        ),
        border: Rect::new(1.0, 2.0, 1.0, 2.0),
        ..Style::default()
    })));
    let first_span = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            grid_column_start: Some(1),
            grid_column_span: 2,
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(108.0, 46.0),
    ));
    let second_span = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            grid_column_start: Some(2),
            grid_column_span: 2,
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(96.0, 54.0),
    ));
    let full_span = tree.push(SimpleNode::with_measured_size(
        block_standalone_style(Style {
            grid_column_start: Some(1),
            grid_column_span: 3,
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(176.0, 72.0),
    ));
    let fixed = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(64.0),
        height: Length::points(28.0),
        margin: Rect::new(
            Length::points(3.0),
            Length::points(5.0),
            Length::points(2.0),
            Length::points(4.0),
        ),
        grid_column_start: Some(3),
        grid_row_start: Some(1),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    })));
    let auto_marker = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(18.0),
        height: Length::points(9.0),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    })));
    tree.append_child(root, first_span);
    tree.append_child(root, second_span);
    tree.append_child(root, full_span);
    tree.append_child(root, fixed);
    tree.append_child(root, auto_marker);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_grid_max_content_track_grows_from_measured_child_when_container_is_indefinite() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::max_content()],
        grid_template_rows: vec![Length::max_content()],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style::default()),
        Size::new(45.0, 18.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_grid_minmax_fixed_auto_track_grows_to_measured_child_when_indefinite() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_columns_max: vec![Length::Auto],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style::default()),
        Size::new(45.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_grid_minmax_max_content_minimum_can_exceed_fixed_maximum() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::MaxContent],
        grid_template_columns_max: vec![Length::points(40.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style::default()),
        Size::new(70.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_grid_spanning_max_content_minimum_track_can_exceed_fixed_maximum() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::MaxContent, Length::points(10.0)],
        grid_template_columns_max: vec![Length::points(40.0), Length::points(10.0)],
        grid_template_rows: vec![Length::points(10.0), Length::points(10.0)],
        column_gap: Length::points(5.0),
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let spanning = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_span: 2,
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(100.0, 10.0),
    ));
    let marker = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(2),
        ..Style::default()
    })));
    tree.append_child(root, spanning);
    tree.append_child(root, marker);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_definite_grid_max_content_minimum_floors_fixed_maximum_before_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(120.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::MaxContent, Length::points(10.0)],
        grid_template_columns_max: vec![Length::points(40.0), Length::points(10.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(5.0),
        justify_content: JustifyContent::Center,
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(70.0, 10.0),
    ));
    let marker = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, intrinsic);
    tree.append_child(root, marker);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(120.0, 10.0));
}

#[test]
fn head_to_head_grid_max_content_minimum_floors_fit_content_maximum() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::MaxContent, Length::points(10.0)],
        grid_template_columns_max: vec![
            Length::fit_content(Some(BaseLength::fixed(40.0))),
            Length::points(10.0),
        ],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(5.0),
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    })));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(70.0, 10.0),
    ));
    let marker = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, intrinsic);
    tree.append_child(root, marker);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_definite_grid_minmax_auto_maximum_updates_growth_limit() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        justify_content: JustifyContent::Center,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_columns_max: vec![Length::Auto],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        }),
        Size::new(45.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
#[ignore = "current C++ standalone lets a spanning item push a fit-content max track past its argument for intrinsic growth-limit sizing; Rust follows W3C section 11.5.1"]
fn head_to_head_definite_grid_spanning_fit_content_max_track_contributes_to_content_alignment_size()
{
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(140.0),
        height: Length::points(90.0),
        grid_template_columns: vec![Length::Auto, Length::points(20.0)],
        grid_template_columns_max: vec![
            Length::fit_content(Some(BaseLength::fixed(40.0))),
            Length::points(20.0),
        ],
        grid_template_rows: vec![Length::Auto, Length::Auto],
        grid_template_rows_max: vec![
            Length::fit_content(Some(BaseLength::fixed_and_percent(4.0, 50.0))),
            Length::MaxContent,
        ],
        column_gap: Length::points(3.0),
        row_gap: Length::points(2.0),
        justify_content: JustifyContent::Center,
        align_items: AlignItems::FlexStart,
        justify_items: JustifyItems::Start,
        ..Style::default()
    })));
    let spanning = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_span: 2,
            ..Style::default()
        }),
        Size::new(80.0, 12.0),
    ));
    let marker = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(2),
            margin: Rect::new(
                Length::points(1.0),
                Length::points(2.0),
                Length::points(1.0),
                Length::ZERO,
            ),
            ..Style::default()
        }),
        Size::new(30.0, 18.0),
    ));
    tree.append_child(root, spanning);
    tree.append_child(root, marker);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(140.0, 90.0));
}

#[test]
fn head_to_head_definite_grid_spanning_flexible_max_tracks_expand_before_max_content_sibling() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(150.0),
        height: Length::points(96.0),
        padding: Rect::all(Length::points(2.0)),
        grid_template_columns: vec![Length::Auto, Length::points(20.0), Length::Auto],
        grid_template_columns_max: vec![Length::fr(1.0), Length::fr(1.0), Length::MaxContent],
        grid_template_rows: vec![Length::Auto, Length::points(18.0)],
        grid_template_rows_max: vec![
            Length::fit_content(Some(BaseLength::fixed_and_percent(4.0, 40.0))),
            Length::MaxContent,
        ],
        column_gap: Length::points(4.0),
        row_gap: Length::points(3.0),
        justify_content: JustifyContent::FlexStart,
        align_content: AlignContent::FlexStart,
        align_items: AlignItems::FlexStart,
        justify_items: JustifyItems::Start,
        ..Style::default()
    })));
    let spanning = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_column_span: 3,
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(116.0, 14.0),
    ));
    let marker = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(2),
            margin: Rect::new(
                Length::points(1.0),
                Length::points(2.0),
                Length::points(1.0),
                Length::ZERO,
            ),
            ..Style::default()
        }),
        Size::new(38.0, 16.0),
    ));
    let tail = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(3),
            grid_row_start: Some(2),
            ..Style::default()
        }),
        Size::new(52.0, 12.0),
    ));
    tree.append_child(root, spanning);
    tree.append_child(root, marker);
    tree.append_child(root, tail);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(160.0, 110.0));
}

#[test]
fn head_to_head_dense_grid_spanning_auto_rows_prefer_indefinite_growth_limits() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(140.0),
        height: Length::points(90.0),
        grid_template_columns: vec![Length::points(18.0), Length::Auto],
        grid_template_columns_max: vec![Length::points(18.0), Length::fr(1.0)],
        grid_template_rows: vec![Length::points(12.0), Length::Auto],
        grid_template_rows_max: vec![Length::points(12.0), Length::MaxContent],
        grid_auto_columns: vec![
            Length::Auto,
            Length::fit_content(Some(BaseLength::fixed(22.0))),
        ],
        grid_auto_columns_max: vec![Length::MaxContent, Length::fr(1.0)],
        grid_auto_rows: vec![Length::Auto, Length::points(9.0)],
        grid_auto_rows_max: vec![Length::MaxContent, Length::fr(1.0)],
        grid_auto_flow: GridAutoFlow::Dense,
        column_gap: Length::points(2.0),
        row_gap: Length::points(3.0),
        justify_content: JustifyContent::Center,
        align_content: AlignContent::Center,
        ..Style::default()
    })));
    let explicit_late = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(2),
            grid_row_start: Some(2),
            grid_row_span: 2,
            ..Style::default()
        }),
        Size::new(34.0, 40.0),
    ));
    let span = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_span: 2,
            ..Style::default()
        }),
        Size::new(60.0, 18.0),
    ));
    let filler = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style::default()),
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, explicit_late);
    tree.append_child(root, span);
    tree.append_child(root, filler);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(140.0, 90.0));
}

#[test]
fn head_to_head_grid_fit_content_track_caps_intrinsic_growth_in_definite_container() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![
            Length::fit_content(Some(BaseLength::fixed(40.0))),
            Length::points(10.0),
        ],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(70.0, 10.0),
    ));
    let following = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, intrinsic);
    tree.append_child(root, following);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(100.0, 10.0));
}

#[test]
fn head_to_head_grid_fit_content_percent_track_caps_intrinsic_growth_in_definite_container() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(120.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::fit_content(Some(BaseLength::fixed_and_percent(
            0.0, 50.0,
        )))],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style::default()),
        Size::new(90.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(120.0, 10.0));
}

#[test]
fn head_to_head_grid_fit_content_calc_track_caps_intrinsic_growth_in_definite_container() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(120.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::fit_content(Some(BaseLength::fixed_and_percent(
            10.0, 50.0,
        )))],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style::default()),
        Size::new(90.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(120.0, 10.0));
}

#[test]
fn head_to_head_root_grid_fit_content_percent_argument_preserves_larger_track_width() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 50.0))),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(140.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 20.0));
}

#[test]
fn head_to_head_root_grid_fit_content_calc_argument_preserves_larger_track_width() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(140.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 20.0));
}

#[test]
fn head_to_head_root_grid_fit_content_percent_argument_preserves_larger_track_height() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(10.0),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 50.0))),
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::points(140.0)],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(20.0, 200.0));
}

#[test]
fn head_to_head_root_grid_fit_content_calc_argument_preserves_larger_track_height() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(10.0),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::points(140.0)],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(20.0, 200.0));
}

#[test]
fn head_to_head_child_grid_fit_content_percent_argument_preserves_larger_track_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(160.0),
        ..Style::default()
    })));
    let child_grid = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 50.0))),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 25.0))),
        grid_template_columns: vec![Length::points(140.0)],
        grid_template_rows: vec![Length::points(60.0)],
        ..Style::default()
    })));
    let item = tree.push(SimpleNode::new(standalone_style(Style::default())));
    tree.append_child(root, child_grid);
    tree.append_child(child_grid, item);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 160.0));
}

#[test]
fn head_to_head_child_grid_fit_content_calc_argument_preserves_larger_track_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(block_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(160.0),
        ..Style::default()
    })));
    let child_grid = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(5.0, 25.0))),
        grid_template_columns: vec![Length::points(140.0)],
        grid_template_rows: vec![Length::points(60.0)],
        ..Style::default()
    })));
    let item = tree.push(SimpleNode::new(standalone_style(Style::default())));
    tree.append_child(root, child_grid);
    tree.append_child(child_grid, item);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 160.0));
}

#[test]
fn head_to_head_grid_fr_tracks_share_remaining_content_width() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(110.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::fr(1.0), Length::fr(2.0)],
        grid_template_rows: vec![Length::points(20.0)],
        column_gap: Length::points(10.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(110.0, 20.0));
}

#[test]
fn head_to_head_grid_fr_tracks_reserve_fixed_tracks_and_gaps() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(120.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(20.0), Length::fr(1.0), Length::fr(2.0)],
        grid_template_rows: vec![Length::points(20.0)],
        column_gap: Length::points(10.0),
        ..Style::default()
    })));
    for _ in 0..3 {
        let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(120.0, 20.0));
}

#[test]
fn head_to_head_grid_calc_track_resolves_against_definite_content_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(200.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::calc(10.0, 25.0), Length::fr(1.0)],
        grid_template_rows: vec![Length::points(20.0)],
        column_gap: Length::points(10.0),
        ..Style::default()
    })));
    for _ in 0..2 {
        let child = tree.push(SimpleNode::new(standalone_style(Style::default())));
        tree.append_child(root, child);
    }

    assert_head_to_head_or_skip(tree, root, Constraints::definite(200.0, 20.0));
}

#[test]
fn head_to_head_grid_fit_content_track_caps_fixed_item_growth() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::fit_content(Some(BaseLength::fixed(40.0)))],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::new(standalone_style(Style {
        width: Length::points(70.0),
        height: Length::points(10.0),
        ..Style::default()
    })));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_grid_fit_content_track_clamps_measured_intrinsic_growth() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::fit_content(Some(BaseLength::fixed(40.0)))],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style::default()),
        Size::new(70.0, 10.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_grid_fit_content_percent_row_track_clamps_measured_intrinsic_growth() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(10.0),
        height: Length::points(120.0),
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::fit_content(Some(BaseLength::fixed_and_percent(
            0.0, 50.0,
        )))],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style::default()),
        Size::new(10.0, 90.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(10.0, 120.0));
}

#[test]
fn head_to_head_grid_fit_content_calc_row_track_clamps_measured_intrinsic_growth() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(10.0),
        height: Length::points(120.0),
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::fit_content(Some(BaseLength::fixed_and_percent(
            10.0, 50.0,
        )))],
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style::default()),
        Size::new(10.0, 90.0),
    ));
    tree.append_child(root, child);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(10.0, 120.0));
}

#[test]
fn head_to_head_grid_minmax_fit_content_max_caps_track() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        grid_template_columns: vec![Length::points(20.0), Length::points(10.0)],
        grid_template_columns_max: vec![Length::fit_content(Some(BaseLength::fixed(40.0)))],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(70.0, 10.0),
    ));
    let following = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, intrinsic);
    tree.append_child(root, following);

    assert_head_to_head_or_skip(tree, root, Constraints::indefinite());
}

#[test]
fn head_to_head_definite_grid_minmax_fit_content_percent_max_caps_track() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(120.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(10.0)],
        grid_template_columns_max: vec![Length::fit_content(Some(BaseLength::fixed_and_percent(
            0.0, 50.0,
        )))],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(90.0, 10.0),
    ));
    let following = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, intrinsic);
    tree.append_child(root, following);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(120.0, 10.0));
}

#[test]
fn head_to_head_definite_grid_minmax_fit_content_calc_max_caps_track() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(120.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(10.0)],
        grid_template_columns_max: vec![Length::fit_content(Some(BaseLength::fixed_and_percent(
            10.0, 50.0,
        )))],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    })));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(90.0, 10.0),
    ));
    let following = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, intrinsic);
    tree.append_child(root, following);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(120.0, 10.0));
}

#[test]
fn head_to_head_definite_grid_minmax_fit_content_percent_row_max_caps_track() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(10.0),
        height: Length::points(120.0),
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(10.0)],
        grid_template_rows_max: vec![Length::fit_content(Some(BaseLength::fixed_and_percent(
            0.0, 50.0,
        )))],
        ..Style::default()
    })));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(10.0, 90.0),
    ));
    let following = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(2),
        ..Style::default()
    })));
    tree.append_child(root, intrinsic);
    tree.append_child(root, following);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(10.0, 120.0));
}

#[test]
fn head_to_head_definite_grid_minmax_fit_content_calc_row_max_caps_track() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_standalone_style(Style {
        width: Length::points(10.0),
        height: Length::points(120.0),
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(10.0)],
        grid_template_rows_max: vec![Length::fit_content(Some(BaseLength::fixed_and_percent(
            10.0, 50.0,
        )))],
        ..Style::default()
    })));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        standalone_style(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(10.0, 90.0),
    ));
    let following = tree.push(SimpleNode::new(standalone_style(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(2),
        ..Style::default()
    })));
    tree.append_child(root, intrinsic);
    tree.append_child(root, following);

    assert_head_to_head_or_skip(tree, root, Constraints::definite(10.0, 120.0));
}

#[derive(Clone, Debug)]
struct MeasuringNode {
    style: Style,
    layout: LayoutResult,
    children: Vec<usize>,
    measure: Option<MeasureBehavior>,
}

#[derive(Clone, Copy, Debug)]
enum MeasureBehavior {
    Fixed(Size),
    HeightFromWidth {
        intrinsic_width: f32,
        fallback_height: f32,
        height_ratio: f32,
    },
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
        }
    }

    fn height_from_width(
        style: Style,
        intrinsic_width: f32,
        fallback_height: f32,
        height_ratio: f32,
    ) -> Self {
        Self {
            measure: Some(MeasureBehavior::HeightFromWidth {
                intrinsic_width,
                fallback_height,
                height_ratio,
            }),
            ..Self::new(style)
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
        self.nodes[node].measure.map(|behavior| match behavior {
            MeasureBehavior::Fixed(size) => Size::new(
                constraints.width.clamp(size.width),
                constraints.height.clamp(size.height),
            ),
            MeasureBehavior::HeightFromWidth {
                intrinsic_width,
                fallback_height,
                height_ratio,
            } => {
                let resolved_width = constraints.width.bounded_size().unwrap_or(intrinsic_width);
                let resolved_height = if constraints.width.bounded_size().is_some() {
                    resolved_width * height_ratio
                } else {
                    fallback_height
                };
                Size::new(
                    constraints.width.clamp(resolved_width),
                    constraints.height.clamp(resolved_height),
                )
            }
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

impl BaselineLayoutTree for MeasuringTree {
    fn layout_result(&self, node: Self::NodeId) -> LayoutResult {
        self.nodes[node].layout
    }
}

fn fixed_flex_child(tree: &mut SimpleTree, width: f32, height: f32) -> usize {
    tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(width),
        height: Length::points(height),
        ..Style::default()
    })))
}

#[derive(Clone, Copy, Debug)]
struct NativeMainAxisDirectionCase {
    flex_direction: FlexDirection,
    direction: Direction,
}

const NATIVE_MAIN_AXIS_MATRIX: [NativeMainAxisDirectionCase; 8] = [
    NativeMainAxisDirectionCase {
        flex_direction: FlexDirection::Row,
        direction: Direction::Ltr,
    },
    NativeMainAxisDirectionCase {
        flex_direction: FlexDirection::Row,
        direction: Direction::Rtl,
    },
    NativeMainAxisDirectionCase {
        flex_direction: FlexDirection::RowReverse,
        direction: Direction::Ltr,
    },
    NativeMainAxisDirectionCase {
        flex_direction: FlexDirection::RowReverse,
        direction: Direction::Rtl,
    },
    NativeMainAxisDirectionCase {
        flex_direction: FlexDirection::Column,
        direction: Direction::Ltr,
    },
    NativeMainAxisDirectionCase {
        flex_direction: FlexDirection::Column,
        direction: Direction::Rtl,
    },
    NativeMainAxisDirectionCase {
        flex_direction: FlexDirection::ColumnReverse,
        direction: Direction::Ltr,
    },
    NativeMainAxisDirectionCase {
        flex_direction: FlexDirection::ColumnReverse,
        direction: Direction::Rtl,
    },
];

const NATIVE_JUSTIFY_MATRIX: [JustifyContent; 9] = [
    JustifyContent::Stretch,
    JustifyContent::FlexStart,
    JustifyContent::Start,
    JustifyContent::Center,
    JustifyContent::FlexEnd,
    JustifyContent::End,
    JustifyContent::SpaceBetween,
    JustifyContent::SpaceAround,
    JustifyContent::SpaceEvenly,
];

const NATIVE_GAP_OVERFLOW_JUSTIFY_MATRIX: [JustifyContent; 3] = [
    JustifyContent::Center,
    JustifyContent::SpaceBetween,
    JustifyContent::SpaceAround,
];

fn fixed_matrix_flex_child(tree: &mut SimpleTree) -> usize {
    tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(10.0),
        width: Length::points(10.0),
        height: Length::points(10.0),
        ..Style::default()
    })))
}

fn fixed_main_axis_matrix_flex_child(
    tree: &mut SimpleTree,
    case: NativeMainAxisDirectionCase,
    main_size: f32,
    cross_size: f32,
) -> usize {
    tree.push(SimpleNode::new(standalone_style(Style {
        flex_basis: Length::points(main_size),
        flex_shrink: 0.0,
        width: Length::points(if case.flex_direction.is_row() {
            main_size
        } else {
            cross_size
        }),
        height: Length::points(if case.flex_direction.is_row() {
            cross_size
        } else {
            main_size
        }),
        ..Style::default()
    })))
}

fn native_main_start_auto_margin(case: NativeMainAxisDirectionCase) -> Rect<Length> {
    match case.flex_direction {
        FlexDirection::Row => {
            if case.direction.is_rtl() {
                Rect::new(Length::ZERO, Length::Auto, Length::ZERO, Length::ZERO)
            } else {
                Rect::new(Length::Auto, Length::ZERO, Length::ZERO, Length::ZERO)
            }
        }
        FlexDirection::RowReverse => {
            if case.direction.is_rtl() {
                Rect::new(Length::Auto, Length::ZERO, Length::ZERO, Length::ZERO)
            } else {
                Rect::new(Length::ZERO, Length::Auto, Length::ZERO, Length::ZERO)
            }
        }
        FlexDirection::Column => Rect::new(Length::ZERO, Length::ZERO, Length::Auto, Length::ZERO),
        FlexDirection::ColumnReverse => {
            Rect::new(Length::ZERO, Length::ZERO, Length::ZERO, Length::Auto)
        }
    }
}

fn measure_available_width(constraints: Constraints) -> Size {
    let width = match constraints.width.mode {
        MeasureMode::Indefinite => 7.0,
        MeasureMode::Definite | MeasureMode::AtMost => constraints.width.size,
    };
    Size::new(width, 10.0)
}

fn measure_height_from_width(constraints: Constraints) -> Size {
    let width = match constraints.width.mode {
        MeasureMode::Indefinite => 20.0,
        MeasureMode::Definite | MeasureMode::AtMost => constraints.width.size,
    };
    Size::new(width, width / 4.0)
}

fn fixed_linear_child(tree: &mut SimpleTree, width: Length, height: Length) -> usize {
    tree.push(SimpleNode::new(standalone_style(Style {
        width,
        height,
        ..Style::default()
    })))
}

fn standalone_style(style: Style) -> Style {
    Style {
        display: Display::Flex,
        box_sizing: BoxSizing::ContentBox,
        ..style
    }
}

fn linear_standalone_style(style: Style) -> Style {
    Style {
        display: Display::Linear,
        box_sizing: BoxSizing::ContentBox,
        ..style
    }
}

fn relative_standalone_style(style: Style) -> Style {
    Style {
        display: Display::Relative,
        box_sizing: BoxSizing::ContentBox,
        ..style
    }
}

fn grid_standalone_style(style: Style) -> Style {
    Style {
        display: Display::Grid,
        box_sizing: BoxSizing::ContentBox,
        ..style
    }
}

fn block_standalone_style(style: Style) -> Style {
    Style {
        display: Display::Block,
        box_sizing: BoxSizing::ContentBox,
        ..style
    }
}

fn assert_head_to_head_or_skip(tree: SimpleTree, root: usize, constraints: Constraints) {
    match run_head_to_head(tree, root, constraints, LayoutTolerance::default()) {
        Ok(_) => {}
        Err(ParityError::CppBaseline(
            CppBaselineError::NativeFeatureDisabled | CppBaselineError::NativeLinkUnavailable,
        )) => {}
        Err(error) => panic!("head-to-head parity failed: {error}"),
    }
}

fn assert_head_to_head_or_skip_with_name(
    name: &str,
    tree: SimpleTree,
    root: usize,
    constraints: Constraints,
) {
    match run_head_to_head(tree, root, constraints, LayoutTolerance::default()) {
        Ok(_) => {}
        Err(ParityError::CppBaseline(
            CppBaselineError::NativeFeatureDisabled | CppBaselineError::NativeLinkUnavailable,
        )) => {}
        Err(error) => panic!("{name} head-to-head parity failed: {error}"),
    }
}

fn assert_measuring_head_to_head_or_skip(
    tree: MeasuringTree,
    root: usize,
    constraints: Constraints,
) {
    match run_head_to_head(tree, root, constraints, LayoutTolerance::default()) {
        Ok(_) => {}
        Err(ParityError::CppBaseline(
            CppBaselineError::NativeFeatureDisabled | CppBaselineError::NativeLinkUnavailable,
        )) => {}
        Err(error) => panic!("head-to-head parity failed: {error}"),
    }
}
