// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

//! Head-to-head Starlight parity harness.
//!
//! This crate owns Rust translations of Starlight tests and the comparison
//! harness that will run the pure Rust implementation against the imported C++
//! baseline once `starlight_cpp` links native glue.

#![forbid(unsafe_code)]

use std::error::Error;
use std::fmt::{Debug, Display, Formatter};

use starlight_cpp::{BaselineLayoutTree, CppBaselineError, CppStarlightEngine};
use starlight_layout::{Constraints, Edges, LayoutEngine, LayoutResult, LayoutTree, Point, Size};

#[derive(Clone, Debug, PartialEq)]
pub struct LayoutSnapshot<N> {
    pub node: N,
    pub result: LayoutResult,
}

#[derive(Clone, Debug, PartialEq)]
pub struct LayoutComparison<N> {
    pub rust_size: Size,
    pub cpp_size: Size,
    pub rust_snapshots: Vec<LayoutSnapshot<N>>,
    pub cpp_snapshots: Vec<LayoutSnapshot<N>>,
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub struct LayoutTolerance {
    pub epsilon: f32,
}

impl LayoutTolerance {
    #[must_use]
    pub const fn new(epsilon: f32) -> Self {
        Self { epsilon }
    }
}

impl Default for LayoutTolerance {
    fn default() -> Self {
        Self { epsilon: 0.01 }
    }
}

#[derive(Clone, Debug, PartialEq)]
pub enum ParityError<N> {
    CppBaseline(CppBaselineError),
    SizeMismatch {
        rust: Size,
        cpp: Size,
    },
    SnapshotLengthMismatch {
        rust_len: usize,
        cpp_len: usize,
    },
    NodeMismatch {
        rust: N,
        cpp: N,
    },
    LayoutMismatch {
        node: N,
        field: LayoutField,
        rust: f32,
        cpp: f32,
    },
    LayoutOptionalMismatch {
        node: N,
        field: LayoutField,
        rust: Option<f32>,
        cpp: Option<f32>,
    },
}

impl<N: Debug> Display for ParityError<N> {
    fn fmt(&self, formatter: &mut Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::CppBaseline(error) => write!(formatter, "{error}"),
            Self::SizeMismatch { rust, cpp } => {
                write!(formatter, "root size mismatch: rust={rust:?}, cpp={cpp:?}")
            }
            Self::SnapshotLengthMismatch { rust_len, cpp_len } => write!(
                formatter,
                "snapshot length mismatch: rust={rust_len}, cpp={cpp_len}"
            ),
            Self::NodeMismatch { rust, cpp } => {
                write!(
                    formatter,
                    "snapshot node mismatch: rust={rust:?}, cpp={cpp:?}"
                )
            }
            Self::LayoutMismatch {
                node,
                field,
                rust,
                cpp,
            } => write!(
                formatter,
                "layout mismatch at node {node:?} field {field:?}: rust={rust}, cpp={cpp}"
            ),
            Self::LayoutOptionalMismatch {
                node,
                field,
                rust,
                cpp,
            } => write!(
                formatter,
                "layout optional mismatch at node {node:?} field {field:?}: rust={rust:?}, cpp={cpp:?}"
            ),
        }
    }
}

impl<N: Debug> Error for ParityError<N> {}

impl<N> From<CppBaselineError> for ParityError<N> {
    fn from(value: CppBaselineError) -> Self {
        Self::CppBaseline(value)
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum LayoutField {
    OffsetX,
    OffsetY,
    Width,
    Height,
    Baseline,
    PaddingLeft,
    PaddingRight,
    PaddingTop,
    PaddingBottom,
    BorderLeft,
    BorderRight,
    BorderTop,
    BorderBottom,
    MarginLeft,
    MarginRight,
    MarginTop,
    MarginBottom,
    StickyLeft,
    StickyRight,
    StickyTop,
    StickyBottom,
}

pub fn run_rust_layout<T: LayoutTree>(
    tree: &mut T,
    root: T::NodeId,
    constraints: Constraints,
) -> Size {
    LayoutEngine::new().layout(tree, root, constraints)
}

pub fn run_head_to_head<T>(
    tree: T,
    root: T::NodeId,
    constraints: Constraints,
    tolerance: LayoutTolerance,
) -> Result<LayoutComparison<T::NodeId>, ParityError<T::NodeId>>
where
    T: BaselineLayoutTree + Clone,
    T::NodeId: Debug,
{
    let mut rust_tree = tree.clone();
    let mut cpp_tree = tree;

    let rust_size =
        LayoutEngine::new().layout_with_owner_constraints(&mut rust_tree, root, constraints);
    let rust_snapshots = collect_layout_snapshots(&rust_tree, root);

    let mut cpp_engine = CppStarlightEngine::new();
    let cpp_size = cpp_engine.layout(&mut cpp_tree, root, constraints)?;
    let cpp_snapshots = collect_layout_snapshots(&cpp_tree, root);

    compare_layout_outputs(
        rust_size,
        cpp_size,
        &rust_snapshots,
        &cpp_snapshots,
        tolerance,
    )?;

    Ok(LayoutComparison {
        rust_size,
        cpp_size,
        rust_snapshots,
        cpp_snapshots,
    })
}

pub fn collect_layout_snapshots<T: BaselineLayoutTree>(
    tree: &T,
    root: T::NodeId,
) -> Vec<LayoutSnapshot<T::NodeId>> {
    let mut snapshots = Vec::new();
    collect_layout_snapshots_inner(tree, root, &mut snapshots);
    snapshots
}

fn collect_layout_snapshots_inner<T: BaselineLayoutTree>(
    tree: &T,
    node: T::NodeId,
    snapshots: &mut Vec<LayoutSnapshot<T::NodeId>>,
) {
    snapshots.push(LayoutSnapshot {
        node,
        result: tree.layout_result(node),
    });
    for child in tree.children(node) {
        collect_layout_snapshots_inner(tree, child, snapshots);
    }
}

pub fn compare_layout_outputs<N: Copy + Eq>(
    rust_size: Size,
    cpp_size: Size,
    rust_snapshots: &[LayoutSnapshot<N>],
    cpp_snapshots: &[LayoutSnapshot<N>],
    tolerance: LayoutTolerance,
) -> Result<(), ParityError<N>> {
    if !size_close(rust_size, cpp_size, tolerance) {
        return Err(ParityError::SizeMismatch {
            rust: rust_size,
            cpp: cpp_size,
        });
    }

    if rust_snapshots.len() != cpp_snapshots.len() {
        return Err(ParityError::SnapshotLengthMismatch {
            rust_len: rust_snapshots.len(),
            cpp_len: cpp_snapshots.len(),
        });
    }

    for (rust, cpp) in rust_snapshots.iter().zip(cpp_snapshots) {
        if rust.node != cpp.node {
            return Err(ParityError::NodeMismatch {
                rust: rust.node,
                cpp: cpp.node,
            });
        }
        compare_layout_result(rust.node, rust.result, cpp.result, tolerance)?;
    }

    Ok(())
}

fn compare_layout_result<N: Copy>(
    node: N,
    rust: LayoutResult,
    cpp: LayoutResult,
    tolerance: LayoutTolerance,
) -> Result<(), ParityError<N>> {
    compare_point(node, rust.offset, cpp.offset, tolerance)?;
    compare_size_fields(node, rust.size, cpp.size, tolerance)?;
    compare_baseline(node, rust, cpp, tolerance)?;
    compare_edges(
        node,
        rust.padding,
        cpp.padding,
        [
            LayoutField::PaddingLeft,
            LayoutField::PaddingRight,
            LayoutField::PaddingTop,
            LayoutField::PaddingBottom,
        ],
        tolerance,
    )?;
    compare_edges(
        node,
        rust.border,
        cpp.border,
        [
            LayoutField::BorderLeft,
            LayoutField::BorderRight,
            LayoutField::BorderTop,
            LayoutField::BorderBottom,
        ],
        tolerance,
    )?;
    compare_edges(
        node,
        rust.margin,
        cpp.margin,
        [
            LayoutField::MarginLeft,
            LayoutField::MarginRight,
            LayoutField::MarginTop,
            LayoutField::MarginBottom,
        ],
        tolerance,
    )?;
    compare_edges(
        node,
        rust.sticky_pos,
        cpp.sticky_pos,
        [
            LayoutField::StickyLeft,
            LayoutField::StickyRight,
            LayoutField::StickyTop,
            LayoutField::StickyBottom,
        ],
        tolerance,
    )
}

fn compare_point<N: Copy>(
    node: N,
    rust: Point,
    cpp: Point,
    tolerance: LayoutTolerance,
) -> Result<(), ParityError<N>> {
    compare_float(node, LayoutField::OffsetX, rust.x, cpp.x, tolerance)?;
    compare_float(node, LayoutField::OffsetY, rust.y, cpp.y, tolerance)
}

fn compare_size_fields<N: Copy>(
    node: N,
    rust: Size,
    cpp: Size,
    tolerance: LayoutTolerance,
) -> Result<(), ParityError<N>> {
    compare_float(node, LayoutField::Width, rust.width, cpp.width, tolerance)?;
    compare_float(
        node,
        LayoutField::Height,
        rust.height,
        cpp.height,
        tolerance,
    )
}

fn compare_baseline<N: Copy>(
    node: N,
    rust: LayoutResult,
    cpp: LayoutResult,
    tolerance: LayoutTolerance,
) -> Result<(), ParityError<N>> {
    let rust_baseline = rust.baseline.unwrap_or(rust.size.height);
    let cpp_baseline = cpp.baseline.unwrap_or(cpp.size.height);
    compare_float(
        node,
        LayoutField::Baseline,
        rust_baseline,
        cpp_baseline,
        tolerance,
    )
}

fn compare_edges<N: Copy>(
    node: N,
    rust: Edges,
    cpp: Edges,
    fields: [LayoutField; 4],
    tolerance: LayoutTolerance,
) -> Result<(), ParityError<N>> {
    compare_float(node, fields[0], rust.left, cpp.left, tolerance)?;
    compare_float(node, fields[1], rust.right, cpp.right, tolerance)?;
    compare_float(node, fields[2], rust.top, cpp.top, tolerance)?;
    compare_float(node, fields[3], rust.bottom, cpp.bottom, tolerance)
}

fn compare_float<N: Copy>(
    node: N,
    field: LayoutField,
    rust: f32,
    cpp: f32,
    tolerance: LayoutTolerance,
) -> Result<(), ParityError<N>> {
    if (rust - cpp).abs() <= tolerance.epsilon {
        Ok(())
    } else {
        Err(ParityError::LayoutMismatch {
            node,
            field,
            rust,
            cpp,
        })
    }
}

fn size_close(rust: Size, cpp: Size, tolerance: LayoutTolerance) -> bool {
    (rust.width - cpp.width).abs() <= tolerance.epsilon
        && (rust.height - cpp.height).abs() <= tolerance.epsilon
}
