// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#[cfg(not(feature = "native-standalone"))]
use starlight_cpp::{CppBaselineError, CppStarlightEngine};
use starlight_layout::{
    Constraints, Length, Rect, SideConstraint, SimpleNode, SimpleTree, Size, Style,
};
use starlight_parity::run_rust_layout;

fn assert_close(actual: f32, expected: f32) {
    assert!(
        (actual - expected).abs() < 0.01,
        "expected {expected}, got {actual}"
    );
}

#[test]
fn rust_layout_smoke_uses_workspace_crate() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        padding: Rect::all(Length::points(2.0)),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(12.0, 8.0),
    ));
    tree.append_child(root, child);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(SideConstraint::definite(40.0), SideConstraint::indefinite()),
    );

    assert_close(size.width, 40.0);
    assert_close(size.height, 12.0);
    assert_close(tree.nodes[child].layout.offset.x, 2.0);
    assert_close(tree.nodes[child].layout.offset.y, 2.0);
}

#[test]
#[cfg(not(feature = "native-standalone"))]
fn cpp_baseline_reports_native_feature_disabled_by_default() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style::default()));
    let mut cpp = CppStarlightEngine::new();

    let error = cpp
        .layout(&mut tree, root, Constraints::indefinite())
        .expect_err("native C++ backend should be explicitly enabled");

    assert_eq!(error, CppBaselineError::NativeFeatureDisabled);
}
