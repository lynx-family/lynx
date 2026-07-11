// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use starlight_layout::{
    AlignItems, Constraints, Display, Length, Rect, SideConstraint, SimpleNode, SimpleTree, Style,
};
use starlight_parity::run_rust_layout;

fn assert_close(actual: f32, expected: f32) {
    assert!(
        (actual - expected).abs() < 0.01,
        "expected {expected}, got {actual}"
    );
}

#[test]
fn block_child_derives_auto_height_from_width_and_aspect_ratio() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        width: Length::points(100.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        width: Length::points(40.0),
        aspect_ratio: Some(2.0),
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

    assert_close(size.height, 20.0);
    assert_close(tree.nodes[child].layout.size.width, 40.0);
    assert_close(tree.nodes[child].layout.size.height, 20.0);
}

#[test]
fn aspect_ratio_uses_content_box_before_padding_and_border() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        width: Length::points(100.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        width: Length::points(40.0),
        aspect_ratio: Some(2.0),
        padding: Rect::all(Length::points(5.0)),
        border: Rect::all(1.0),
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

    assert_close(tree.nodes[child].layout.size.width, 52.0);
    assert_close(tree.nodes[child].layout.size.height, 32.0);
}

#[test]
fn flex_item_derives_cross_size_from_main_size_and_aspect_ratio() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(40.0),
        aspect_ratio: Some(2.0),
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

    assert_close(size.height, 20.0);
    assert_close(tree.nodes[child].layout.size.width, 40.0);
    assert_close(tree.nodes[child].layout.size.height, 20.0);
}

#[test]
fn grid_auto_row_grows_from_child_aspect_ratio() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(80.0),
        grid_template_columns: vec![Length::points(80.0)],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        aspect_ratio: Some(2.0),
        ..Style::default()
    }));
    tree.append_child(root, child);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(SideConstraint::definite(80.0), SideConstraint::indefinite()),
    );

    assert_close(size.height, 40.0);
    assert_close(tree.nodes[child].layout.size.width, 80.0);
    assert_close(tree.nodes[child].layout.size.height, 40.0);
}
