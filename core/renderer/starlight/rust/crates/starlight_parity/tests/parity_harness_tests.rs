// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use std::fs;
use std::path::Path;

#[cfg(not(feature = "native-standalone"))]
use starlight_cpp::CppBaselineError;
use starlight_layout::{
    Constraints, LayoutResult, Length, Point, Rect, SideConstraint, SimpleNode, SimpleTree, Size,
    Style,
};
#[cfg(not(feature = "native-standalone"))]
use starlight_parity::run_head_to_head;
use starlight_parity::{
    collect_layout_snapshots, compare_layout_outputs, LayoutField, LayoutSnapshot, LayoutTolerance,
    ParityError,
};

#[test]
fn collect_layout_snapshots_walks_tree_in_preorder() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        padding: Rect::all(Length::points(2.0)),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(10.0, 5.0),
    ));
    let second = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(8.0, 4.0),
    ));
    tree.append_child(root, first);
    tree.append_child(root, second);

    starlight_parity::run_rust_layout(
        &mut tree,
        root,
        Constraints::new(SideConstraint::definite(50.0), SideConstraint::indefinite()),
    );
    let snapshots = collect_layout_snapshots(&tree, root);

    assert_eq!(
        snapshots
            .iter()
            .map(|snapshot| snapshot.node)
            .collect::<Vec<_>>(),
        vec![root, first, second]
    );
    assert_eq!(snapshots[1].result.size, Size::new(46.0, 5.0));
}

#[test]
fn compare_layout_outputs_accepts_values_within_tolerance() {
    let rust_result = LayoutResult {
        size: Size::new(10.0, 5.0),
        offset: Point::new(1.0, 2.0),
        baseline: Some(3.0),
        ..LayoutResult::default()
    };
    let cpp_result = LayoutResult {
        size: Size::new(10.004, 5.004),
        offset: Point::new(1.004, 2.004),
        baseline: Some(3.004),
        ..LayoutResult::default()
    };

    compare_layout_outputs(
        Size::new(10.0, 5.0),
        Size::new(10.004, 5.004),
        &[LayoutSnapshot {
            node: 0,
            result: rust_result,
        }],
        &[LayoutSnapshot {
            node: 0,
            result: cpp_result,
        }],
        LayoutTolerance::new(0.01),
    )
    .expect("values within tolerance should compare equal");
}

#[test]
fn compare_layout_outputs_accepts_missing_fallback_baseline() {
    let rust_result = LayoutResult {
        size: Size::new(10.0, 12.0),
        baseline: Some(12.0),
        ..LayoutResult::default()
    };
    let cpp_result = LayoutResult {
        size: Size::new(10.0, 12.0),
        baseline: None,
        ..LayoutResult::default()
    };

    compare_layout_outputs(
        Size::new(10.0, 12.0),
        Size::new(10.0, 12.0),
        &[LayoutSnapshot {
            node: 3,
            result: rust_result,
        }],
        &[LayoutSnapshot {
            node: 3,
            result: cpp_result,
        }],
        LayoutTolerance::new(0.01),
    )
    .expect("missing baseline should compare as border-box bottom fallback");
}

#[test]
fn compare_layout_outputs_reports_baseline_value_mismatch() {
    let rust_result = LayoutResult {
        size: Size::new(10.0, 10.0),
        baseline: Some(7.0),
        ..LayoutResult::default()
    };
    let cpp_result = LayoutResult {
        size: Size::new(10.0, 10.0),
        baseline: None,
        ..LayoutResult::default()
    };

    let error = compare_layout_outputs(
        Size::new(10.0, 10.0),
        Size::new(10.0, 10.0),
        &[LayoutSnapshot {
            node: 3,
            result: rust_result,
        }],
        &[LayoutSnapshot {
            node: 3,
            result: cpp_result,
        }],
        LayoutTolerance::new(0.01),
    )
    .expect_err("different baseline values should be reported");

    assert_eq!(
        error,
        ParityError::LayoutMismatch {
            node: 3,
            field: LayoutField::Baseline,
            rust: 7.0,
            cpp: 10.0,
        }
    );
}

#[test]
fn compare_layout_outputs_reports_exact_mismatched_field() {
    let rust_result = LayoutResult {
        offset: Point::new(2.0, 3.0),
        ..LayoutResult::default()
    };
    let cpp_result = LayoutResult {
        offset: Point::new(2.0, 9.0),
        ..LayoutResult::default()
    };

    let error = compare_layout_outputs(
        Size::ZERO,
        Size::ZERO,
        &[LayoutSnapshot {
            node: 7,
            result: rust_result,
        }],
        &[LayoutSnapshot {
            node: 7,
            result: cpp_result,
        }],
        LayoutTolerance::new(0.01),
    )
    .expect_err("offset y mismatch should be reported");

    assert_eq!(
        error,
        ParityError::LayoutMismatch {
            node: 7,
            field: LayoutField::OffsetY,
            rust: 3.0,
            cpp: 9.0,
        }
    );
}

#[test]
fn compare_layout_outputs_explicitly_compares_every_layout_result_field() {
    let manifest_dir = Path::new(env!("CARGO_MANIFEST_DIR"));
    let layout_types = fs::read_to_string(manifest_dir.join("../starlight_layout/src/types.rs"))
        .expect("starlight_layout types source is readable");
    let parity_source =
        fs::read_to_string(manifest_dir.join("src/lib.rs")).expect("parity source is readable");

    let fields = public_struct_field_names(&layout_types, "LayoutResult");
    assert!(
        !fields.is_empty(),
        "LayoutResult must expose fields for parity comparison"
    );

    let missing = fields
        .into_iter()
        .filter(|field| {
            !parity_source.contains(&format!("rust.{field}"))
                || !parity_source.contains(&format!("cpp.{field}"))
        })
        .collect::<Vec<_>>();

    assert!(
        missing.is_empty(),
        "head-to-head parity must explicitly compare every LayoutResult field; missing: {}",
        missing.join(", ")
    );
}

#[test]
#[cfg(not(feature = "native-standalone"))]
fn run_head_to_head_reports_native_feature_disabled_by_default() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style::default()));

    let error = run_head_to_head(
        tree,
        root,
        Constraints::indefinite(),
        LayoutTolerance::default(),
    )
    .expect_err("native C++ backend should be explicitly enabled");

    assert_eq!(
        error,
        ParityError::CppBaseline(CppBaselineError::NativeFeatureDisabled)
    );
}

fn public_struct_field_names(source: &str, struct_name: &str) -> Vec<String> {
    let marker = format!("pub struct {struct_name}");
    let start = source.find(&marker).expect("public struct exists");
    let body_start = start
        + source[start..]
            .find('{')
            .expect("public struct body starts")
        + 1;
    let body_end = body_start
        + source[body_start..]
            .find('}')
            .expect("public struct body ends");

    source[body_start..body_end]
        .lines()
        .map(str::trim)
        .filter_map(|line| {
            let field = line.strip_prefix("pub ")?;
            let (name, _) = field.split_once(':')?;
            Some(name.trim().to_owned())
        })
        .collect()
}
