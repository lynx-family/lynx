// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

//! C ABI for running the safe Rust Starlight engine over external trees.
//!
//! The ABI mirrors the Rust [`starlight_layout::LayoutTree`] trait as callback
//! slots. C and C++ embedders keep owning their trees; this crate snapshots the
//! child/style graph into Rust values for one layout pass, calls the safe Rust
//! engine, and reports each resulting layout through `set_layout`.

#![deny(unsafe_op_in_unsafe_fn)]

use std::cell::Cell;
use std::collections::{HashMap, HashSet};
use std::ffi::c_void;
use std::os::raw::c_char;
use std::panic::{catch_unwind, AssertUnwindSafe};
use std::ptr;

use starlight_layout::{
    AlignContent, AlignItems, BaseLength, BoxSizing, Constraints, Direction, Display,
    FlexDirection, FlexWrap, GridAutoFlow, JustifyContent, JustifyItems, LayoutEngine,
    LayoutResult, LayoutTree, Length, LinearCrossGravity, LinearGravity, LinearLayoutGravity,
    LinearOrientation, ListComponentType, MeasureMode, Point, PositionType, Rect, RelativeCenter,
    SideConstraint, Size, Style, Visibility,
};

pub type SLRustNodeId = u64;

const ABI_VERSION_MAJOR: u32 = 1;
const ABI_VERSION_MINOR: u32 = 15;
const ABI_VERSION_PATCH: u32 = 0;

#[repr(C)]
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum SLRustStatus {
    Ok = 0,
    NullPointer = 1,
    MissingCallback = 2,
    InvalidStyle = 3,
    InvalidTree = 4,
    Panic = 5,
    AbiMismatch = 6,
    Disabled = 7,
    UnsupportedTree = 8,
    FixedNodeSetMismatch = 9,
}

const STATUS_NAME_OK: &[u8] = b"Ok\0";
const STATUS_NAME_NULL_POINTER: &[u8] = b"NullPointer\0";
const STATUS_NAME_MISSING_CALLBACK: &[u8] = b"MissingCallback\0";
const STATUS_NAME_INVALID_STYLE: &[u8] = b"InvalidStyle\0";
const STATUS_NAME_INVALID_TREE: &[u8] = b"InvalidTree\0";
const STATUS_NAME_PANIC: &[u8] = b"Panic\0";
const STATUS_NAME_ABI_MISMATCH: &[u8] = b"AbiMismatch\0";
const STATUS_NAME_DISABLED: &[u8] = b"Disabled\0";
const STATUS_NAME_UNSUPPORTED_TREE: &[u8] = b"UnsupportedTree\0";
const STATUS_NAME_FIXED_NODE_SET_MISMATCH: &[u8] = b"FixedNodeSetMismatch\0";
const STATUS_NAME_UNKNOWN: &[u8] = b"Unknown\0";

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct SLRustAbiInfo {
    pub version_major: u32,
    pub version_minor: u32,
    pub version_patch: u32,
    pub size_of_abi_info: usize,
    pub align_of_abi_info: usize,
    pub size_of_length: usize,
    pub align_of_length: usize,
    pub size_of_constraints: usize,
    pub align_of_constraints: usize,
    pub size_of_layout_result: usize,
    pub align_of_layout_result: usize,
    pub size_of_style: usize,
    pub align_of_style: usize,
    pub size_of_tree_callbacks: usize,
    pub align_of_tree_callbacks: usize,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum SLRustMeasureMode {
    Indefinite = 0,
    Definite = 1,
    AtMost = 2,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum SLRustLengthKind {
    Auto = 0,
    Points = 1,
    Percent = 2,
    Calc = 3,
    Fr = 4,
    MaxContent = 5,
    FitContent = 6,
    MinContent = 7,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct SLRustLength {
    pub kind: i32,
    pub value: f32,
    pub percent: f32,
    pub has_base: bool,
    pub has_percentage: bool,
}

impl Default for SLRustLength {
    fn default() -> Self {
        length_to_ffi(Length::Auto)
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct SLRustRectLength {
    pub left: SLRustLength,
    pub right: SLRustLength,
    pub top: SLRustLength,
    pub bottom: SLRustLength,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct SLRustRectF32 {
    pub left: f32,
    pub right: f32,
    pub top: f32,
    pub bottom: f32,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct SLRustSize {
    pub width: f32,
    pub height: f32,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct SLRustPoint {
    pub x: f32,
    pub y: f32,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct SLRustSideConstraint {
    pub size: f32,
    pub mode: i32,
}

impl Default for SLRustSideConstraint {
    fn default() -> Self {
        side_constraint_to_ffi(SideConstraint::indefinite())
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct SLRustConstraints {
    pub width: SLRustSideConstraint,
    pub height: SLRustSideConstraint,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct SLRustLayoutResult {
    pub offset: SLRustPoint,
    pub size: SLRustSize,
    pub baseline: f32,
    pub has_baseline: bool,
    pub margin: SLRustRectF32,
    pub padding: SLRustRectF32,
    pub border: SLRustRectF32,
    pub sticky_pos: SLRustRectF32,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct SLRustStyle {
    pub display: i32,
    pub position: i32,
    pub box_sizing: i32,
    pub direction: i32,
    pub visibility: i32,
    pub has_explicit_direction: bool,
    pub width: SLRustLength,
    pub height: SLRustLength,
    pub min_width: SLRustLength,
    pub min_height: SLRustLength,
    pub max_width: SLRustLength,
    pub max_height: SLRustLength,
    pub aspect_ratio: f32,
    pub has_aspect_ratio: bool,
    pub left: SLRustLength,
    pub right: SLRustLength,
    pub top: SLRustLength,
    pub bottom: SLRustLength,
    pub margin: SLRustRectLength,
    pub padding: SLRustRectLength,
    pub border: SLRustRectF32,
    pub flex_direction: i32,
    pub flex_wrap: i32,
    pub justify_content: i32,
    pub align_items: i32,
    pub align_self: i32,
    pub has_align_self: bool,
    pub align_content: i32,
    pub justify_items: i32,
    pub justify_self: i32,
    pub flex_grow: f32,
    pub flex_shrink: f32,
    pub flex_basis: SLRustLength,
    pub order: i32,
    pub row_gap: SLRustLength,
    pub column_gap: SLRustLength,
    pub linear_orientation: i32,
    pub linear_gravity: i32,
    pub linear_layout_gravity: i32,
    pub linear_cross_gravity: i32,
    pub linear_weight: f32,
    pub linear_weight_sum: f32,
    pub linear_column_count: i32,
    pub list_main_axis_gap: SLRustLength,
    pub list_cross_axis_gap: SLRustLength,
    pub list_component_type: i32,
    pub grid_template_columns: *const SLRustLength,
    pub grid_template_columns_len: usize,
    pub grid_template_rows: *const SLRustLength,
    pub grid_template_rows_len: usize,
    pub grid_template_columns_max: *const SLRustLength,
    pub grid_template_columns_max_len: usize,
    pub grid_template_rows_max: *const SLRustLength,
    pub grid_template_rows_max_len: usize,
    pub grid_auto_columns: *const SLRustLength,
    pub grid_auto_columns_len: usize,
    pub grid_auto_rows: *const SLRustLength,
    pub grid_auto_rows_len: usize,
    pub grid_auto_columns_max: *const SLRustLength,
    pub grid_auto_columns_max_len: usize,
    pub grid_auto_rows_max: *const SLRustLength,
    pub grid_auto_rows_max_len: usize,
    pub grid_auto_flow: i32,
    pub grid_column_start: i32,
    pub grid_column_end: i32,
    pub grid_row_start: i32,
    pub grid_row_end: i32,
    pub grid_column_span: usize,
    pub grid_row_span: usize,
    pub relative_id: i32,
    pub relative_align_top: i32,
    pub relative_align_right: i32,
    pub relative_align_bottom: i32,
    pub relative_align_left: i32,
    pub relative_top_of: i32,
    pub relative_right_of: i32,
    pub relative_bottom_of: i32,
    pub relative_left_of: i32,
    pub relative_layout_once: bool,
    pub relative_center: i32,
}

impl Default for SLRustStyle {
    fn default() -> Self {
        style_to_ffi_defaults(&Style::default())
    }
}

pub type SLRustChildCountFunc = extern "C" fn(*mut c_void, SLRustNodeId) -> usize;
pub type SLRustChildAtFunc = extern "C" fn(*mut c_void, SLRustNodeId, usize) -> SLRustNodeId;
pub type SLRustStyleFunc = extern "C" fn(*mut c_void, SLRustNodeId, *mut SLRustStyle) -> bool;
pub type SLRustSetLayoutFunc = extern "C" fn(*mut c_void, SLRustNodeId, SLRustLayoutResult);
pub type SLRustSetLayoutWithConstraintsFunc =
    extern "C" fn(*mut c_void, SLRustNodeId, SLRustConstraints, SLRustLayoutResult);
pub type SLRustHasMeasureFunc = extern "C" fn(*mut c_void, SLRustNodeId) -> bool;
pub type SLRustMeasureFunc =
    extern "C" fn(*mut c_void, SLRustNodeId, SLRustConstraints, *mut SLRustSize) -> bool;
pub type SLRustBaselineFunc =
    extern "C" fn(*mut c_void, SLRustNodeId, SLRustSize, *mut f32) -> bool;
pub type SLRustPhysicalPixelsPerLayoutUnitFunc =
    extern "C" fn(*mut c_void, SLRustNodeId, *mut f32) -> bool;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct SLRustTreeCallbacks {
    pub context: *mut c_void,
    pub child_count: Option<SLRustChildCountFunc>,
    pub child_at: Option<SLRustChildAtFunc>,
    pub style: Option<SLRustStyleFunc>,
    pub set_layout: Option<SLRustSetLayoutFunc>,
    pub set_layout_with_constraints: Option<SLRustSetLayoutWithConstraintsFunc>,
    /// `has_measure` and `measure` are a pair: provide both for custom
    /// measured content, or neither for pure style-driven layout.
    pub has_measure: Option<SLRustHasMeasureFunc>,
    pub measure: Option<SLRustMeasureFunc>,
    pub baseline: Option<SLRustBaselineFunc>,
    pub physical_pixels_per_layout_unit: Option<SLRustPhysicalPixelsPerLayoutUnitFunc>,
}

/// Writes ABI version and C-layout metadata into `out_info`.
///
/// # Safety
///
/// `out_info` must be null or valid for writing one [`SLRustAbiInfo`].
#[no_mangle]
pub unsafe extern "C" fn SLRustGetAbiInfo(out_info: *mut SLRustAbiInfo) -> SLRustStatus {
    let Some(out_info) = (unsafe { out_info.as_mut() }) else {
        return SLRustStatus::NullPointer;
    };
    *out_info = current_abi_info();
    SLRustStatus::Ok
}

#[no_mangle]
pub extern "C" fn SLRustStatusName(status: i32) -> *const c_char {
    let name = match status {
        0 => STATUS_NAME_OK,
        1 => STATUS_NAME_NULL_POINTER,
        2 => STATUS_NAME_MISSING_CALLBACK,
        3 => STATUS_NAME_INVALID_STYLE,
        4 => STATUS_NAME_INVALID_TREE,
        5 => STATUS_NAME_PANIC,
        6 => STATUS_NAME_ABI_MISMATCH,
        7 => STATUS_NAME_DISABLED,
        8 => STATUS_NAME_UNSUPPORTED_TREE,
        9 => STATUS_NAME_FIXED_NODE_SET_MISMATCH,
        _ => STATUS_NAME_UNKNOWN,
    };
    name.as_ptr().cast()
}

fn current_abi_info() -> SLRustAbiInfo {
    SLRustAbiInfo {
        version_major: ABI_VERSION_MAJOR,
        version_minor: ABI_VERSION_MINOR,
        version_patch: ABI_VERSION_PATCH,
        size_of_abi_info: std::mem::size_of::<SLRustAbiInfo>(),
        align_of_abi_info: std::mem::align_of::<SLRustAbiInfo>(),
        size_of_length: std::mem::size_of::<SLRustLength>(),
        align_of_length: std::mem::align_of::<SLRustLength>(),
        size_of_constraints: std::mem::size_of::<SLRustConstraints>(),
        align_of_constraints: std::mem::align_of::<SLRustConstraints>(),
        size_of_layout_result: std::mem::size_of::<SLRustLayoutResult>(),
        align_of_layout_result: std::mem::align_of::<SLRustLayoutResult>(),
        size_of_style: std::mem::size_of::<SLRustStyle>(),
        align_of_style: std::mem::align_of::<SLRustStyle>(),
        size_of_tree_callbacks: std::mem::size_of::<SLRustTreeCallbacks>(),
        align_of_tree_callbacks: std::mem::align_of::<SLRustTreeCallbacks>(),
    }
}

fn validate_abi_info(caller_abi: &SLRustAbiInfo) -> Result<(), SLRustStatus> {
    let current = current_abi_info();
    if caller_abi.version_major != current.version_major
        || caller_abi.version_minor > current.version_minor
        || caller_abi.size_of_abi_info != current.size_of_abi_info
        || caller_abi.align_of_abi_info != current.align_of_abi_info
        || caller_abi.size_of_length != current.size_of_length
        || caller_abi.align_of_length != current.align_of_length
        || caller_abi.size_of_constraints != current.size_of_constraints
        || caller_abi.align_of_constraints != current.align_of_constraints
        || caller_abi.size_of_layout_result != current.size_of_layout_result
        || caller_abi.align_of_layout_result != current.align_of_layout_result
        || caller_abi.size_of_style != current.size_of_style
        || caller_abi.align_of_style != current.align_of_style
        || caller_abi.size_of_tree_callbacks != current.size_of_tree_callbacks
        || caller_abi.align_of_tree_callbacks != current.align_of_tree_callbacks
    {
        return Err(SLRustStatus::AbiMismatch);
    }
    Ok(())
}

/// Writes a default latest-mode Rust Starlight style into `out_style`.
///
/// # Safety
///
/// `out_style` must be null or valid for writing one [`SLRustStyle`].
#[no_mangle]
pub unsafe extern "C" fn SLRustStyleDefault(out_style: *mut SLRustStyle) {
    if let Some(out_style) = unsafe { out_style.as_mut() } {
        *out_style = SLRustStyle::default();
    }
}

/// Runs the safe Rust Starlight engine over an external callback-backed tree.
///
/// # Safety
///
/// `callbacks` and `out_size` must be valid pointers for the duration of this
/// call. Any pointers returned inside [`SLRustStyle`] track arrays must remain
/// valid until the style callback returns; the Rust glue copies them
/// immediately. Callback functions must not unwind across this C ABI boundary.
#[no_mangle]
pub unsafe extern "C" fn SLRustLayoutExternal(
    callbacks: *const SLRustTreeCallbacks,
    root: SLRustNodeId,
    constraints: SLRustConstraints,
    out_size: *mut SLRustSize,
) -> SLRustStatus {
    let result = catch_unwind(AssertUnwindSafe(|| unsafe {
        layout_external_inner(
            callbacks,
            root,
            constraints,
            out_size,
            ConstraintEntryMode::Owner,
            None,
        )
    }));
    match result {
        Ok(Ok(())) => SLRustStatus::Ok,
        Ok(Err(status)) => status,
        Err(_) => SLRustStatus::Panic,
    }
}

/// Runs layout from C++ Starlight-style owner constraints.
///
/// # Safety
///
/// `callbacks` and `out_size` must be valid pointers for the duration of this
/// call. Any pointers returned inside [`SLRustStyle`] track arrays must remain
/// valid until the style callback returns; the Rust glue copies them
/// immediately. Callback functions must not unwind across this C ABI boundary.
#[no_mangle]
pub unsafe extern "C" fn SLRustLayoutExternalWithOwnerConstraints(
    callbacks: *const SLRustTreeCallbacks,
    root: SLRustNodeId,
    constraints: SLRustConstraints,
    out_size: *mut SLRustSize,
) -> SLRustStatus {
    let result = catch_unwind(AssertUnwindSafe(|| unsafe {
        layout_external_inner(
            callbacks,
            root,
            constraints,
            out_size,
            ConstraintEntryMode::Owner,
            None,
        )
    }));
    match result {
        Ok(Ok(())) => SLRustStatus::Ok,
        Ok(Err(status)) => status,
        Err(_) => SLRustStatus::Panic,
    }
}

/// Runs owner-constraints layout with standalone-style owner direction inheritance.
///
/// Nodes whose [`SLRustStyle::has_explicit_direction`] is false use
/// `owner_direction` for this layout pass. Explicit node directions are left
/// unchanged.
///
/// # Safety
///
/// `callbacks` and `out_size` must be valid pointers for the duration of this
/// call. Any pointers returned inside [`SLRustStyle`] track arrays must remain
/// valid until the style callback returns; the Rust glue copies them
/// immediately. Callback functions must not unwind across this C ABI boundary.
#[no_mangle]
pub unsafe extern "C" fn SLRustLayoutExternalWithOwnerConstraintsAndDirection(
    callbacks: *const SLRustTreeCallbacks,
    root: SLRustNodeId,
    constraints: SLRustConstraints,
    owner_direction: i32,
    out_size: *mut SLRustSize,
) -> SLRustStatus {
    let result = catch_unwind(AssertUnwindSafe(|| unsafe {
        layout_external_inner(
            callbacks,
            root,
            constraints,
            out_size,
            ConstraintEntryMode::Owner,
            Some(owner_direction),
        )
    }));
    match result {
        Ok(Ok(())) => SLRustStatus::Ok,
        Ok(Err(status)) => status,
        Err(_) => SLRustStatus::Panic,
    }
}

/// Runs layout with `constraints` already resolved for the root node.
///
/// # Safety
///
/// `callbacks` and `out_size` must be valid pointers for the duration of this
/// call. Any pointers returned inside [`SLRustStyle`] track arrays must remain
/// valid until the style callback returns; the Rust glue copies them
/// immediately. Callback functions must not unwind across this C ABI boundary.
#[no_mangle]
pub unsafe extern "C" fn SLRustLayoutExternalWithNodeConstraints(
    callbacks: *const SLRustTreeCallbacks,
    root: SLRustNodeId,
    constraints: SLRustConstraints,
    out_size: *mut SLRustSize,
) -> SLRustStatus {
    let result = catch_unwind(AssertUnwindSafe(|| unsafe {
        layout_external_inner(
            callbacks,
            root,
            constraints,
            out_size,
            ConstraintEntryMode::Node,
            None,
        )
    }));
    match result {
        Ok(Ok(())) => SLRustStatus::Ok,
        Ok(Err(status)) => status,
        Err(_) => SLRustStatus::Panic,
    }
}

/// Runs owner-constraints layout only after validating the caller's ABI metadata.
///
/// # Safety
///
/// `caller_abi`, `callbacks`, and `out_size` must be valid pointers for the
/// duration of this call. `caller_abi` should be initialized from
/// [`SLRustGetAbiInfo`] using the same public header version used to compile
/// the C/C++ caller.
#[no_mangle]
pub unsafe extern "C" fn SLRustLayoutExternalChecked(
    caller_abi: *const SLRustAbiInfo,
    callbacks: *const SLRustTreeCallbacks,
    root: SLRustNodeId,
    constraints: SLRustConstraints,
    out_size: *mut SLRustSize,
) -> SLRustStatus {
    let result = catch_unwind(AssertUnwindSafe(|| unsafe {
        layout_external_checked_inner(
            caller_abi,
            callbacks,
            root,
            constraints,
            out_size,
            ConstraintEntryMode::Owner,
            None,
        )
    }));
    match result {
        Ok(Ok(())) => SLRustStatus::Ok,
        Ok(Err(status)) => status,
        Err(_) => SLRustStatus::Panic,
    }
}

/// Runs owner-constraints layout after validating the caller's ABI metadata.
///
/// # Safety
///
/// `caller_abi`, `callbacks`, and `out_size` must be valid pointers for the
/// duration of this call. `caller_abi` should be initialized from
/// [`SLRustGetAbiInfo`] using the same public header version used to compile
/// the C/C++ caller.
#[no_mangle]
pub unsafe extern "C" fn SLRustLayoutExternalWithOwnerConstraintsChecked(
    caller_abi: *const SLRustAbiInfo,
    callbacks: *const SLRustTreeCallbacks,
    root: SLRustNodeId,
    constraints: SLRustConstraints,
    out_size: *mut SLRustSize,
) -> SLRustStatus {
    let result = catch_unwind(AssertUnwindSafe(|| unsafe {
        layout_external_checked_inner(
            caller_abi,
            callbacks,
            root,
            constraints,
            out_size,
            ConstraintEntryMode::Owner,
            None,
        )
    }));
    match result {
        Ok(Ok(())) => SLRustStatus::Ok,
        Ok(Err(status)) => status,
        Err(_) => SLRustStatus::Panic,
    }
}

/// Runs owner-constraints layout with owner direction after validating ABI metadata.
///
/// # Safety
///
/// `caller_abi`, `callbacks`, and `out_size` must be valid pointers for the
/// duration of this call. `caller_abi` should be initialized from
/// [`SLRustGetAbiInfo`] using the same public header version used to compile
/// the C/C++ caller.
#[no_mangle]
pub unsafe extern "C" fn SLRustLayoutExternalWithOwnerConstraintsAndDirectionChecked(
    caller_abi: *const SLRustAbiInfo,
    callbacks: *const SLRustTreeCallbacks,
    root: SLRustNodeId,
    constraints: SLRustConstraints,
    owner_direction: i32,
    out_size: *mut SLRustSize,
) -> SLRustStatus {
    let result = catch_unwind(AssertUnwindSafe(|| unsafe {
        layout_external_checked_inner(
            caller_abi,
            callbacks,
            root,
            constraints,
            out_size,
            ConstraintEntryMode::Owner,
            Some(owner_direction),
        )
    }));
    match result {
        Ok(Ok(())) => SLRustStatus::Ok,
        Ok(Err(status)) => status,
        Err(_) => SLRustStatus::Panic,
    }
}

/// Runs node-constraints layout after validating the caller's ABI metadata.
///
/// # Safety
///
/// `caller_abi`, `callbacks`, and `out_size` must be valid pointers for the
/// duration of this call. `caller_abi` should be initialized from
/// [`SLRustGetAbiInfo`] using the same public header version used to compile
/// the C/C++ caller.
#[no_mangle]
pub unsafe extern "C" fn SLRustLayoutExternalWithNodeConstraintsChecked(
    caller_abi: *const SLRustAbiInfo,
    callbacks: *const SLRustTreeCallbacks,
    root: SLRustNodeId,
    constraints: SLRustConstraints,
    out_size: *mut SLRustSize,
) -> SLRustStatus {
    let result = catch_unwind(AssertUnwindSafe(|| unsafe {
        layout_external_checked_inner(
            caller_abi,
            callbacks,
            root,
            constraints,
            out_size,
            ConstraintEntryMode::Node,
            None,
        )
    }));
    match result {
        Ok(Ok(())) => SLRustStatus::Ok,
        Ok(Err(status)) => status,
        Err(_) => SLRustStatus::Panic,
    }
}

#[derive(Clone, Copy)]
enum ConstraintEntryMode {
    Owner,
    Node,
}

unsafe fn layout_external_checked_inner(
    caller_abi: *const SLRustAbiInfo,
    callbacks: *const SLRustTreeCallbacks,
    root: SLRustNodeId,
    constraints: SLRustConstraints,
    out_size: *mut SLRustSize,
    mode: ConstraintEntryMode,
    owner_direction: Option<i32>,
) -> Result<(), SLRustStatus> {
    let caller_abi = unsafe { caller_abi.as_ref() }.ok_or(SLRustStatus::NullPointer)?;
    validate_abi_info(caller_abi)?;
    unsafe {
        layout_external_inner(
            callbacks,
            root,
            constraints,
            out_size,
            mode,
            owner_direction,
        )
    }
}

unsafe fn layout_external_inner(
    callbacks: *const SLRustTreeCallbacks,
    root: SLRustNodeId,
    constraints: SLRustConstraints,
    out_size: *mut SLRustSize,
    mode: ConstraintEntryMode,
    owner_direction: Option<i32>,
) -> Result<(), SLRustStatus> {
    let callbacks = unsafe { callbacks.as_ref() }.ok_or(SLRustStatus::NullPointer)?;
    let out_size = unsafe { out_size.as_mut() }.ok_or(SLRustStatus::NullPointer)?;
    RequiredCallbacks::new(callbacks)?;

    let constraints = constraints_from_ffi(constraints)?;
    let owner_direction = owner_direction.map(direction_from_ffi).transpose()?;
    let mut tree = ExternalTreeSnapshot::build(*callbacks, root)?;
    let root = tree.root;
    if let Some(owner_direction) = owner_direction {
        tree.apply_owner_direction_to_unset_subtree(root, owner_direction);
    }
    let size = match mode {
        ConstraintEntryMode::Owner => {
            LayoutEngine::new().layout_with_owner_constraints(&mut tree, root, constraints)
        }
        ConstraintEntryMode::Node => LayoutEngine::new().layout(&mut tree, root, constraints),
    };
    tree.callback_error()?;
    validate_layout_size(size)?;
    tree.flush_layouts();
    *out_size = size_to_ffi(size);
    Ok(())
}

#[derive(Clone, Copy)]
struct RequiredCallbacks {
    child_count: SLRustChildCountFunc,
    child_at: SLRustChildAtFunc,
    style: SLRustStyleFunc,
    set_layout: SLRustSetLayoutFunc,
}

impl RequiredCallbacks {
    fn new(callbacks: &SLRustTreeCallbacks) -> Result<Self, SLRustStatus> {
        if callbacks.has_measure.is_some() != callbacks.measure.is_some() {
            return Err(SLRustStatus::MissingCallback);
        }

        Ok(Self {
            child_count: callbacks.child_count.ok_or(SLRustStatus::MissingCallback)?,
            child_at: callbacks.child_at.ok_or(SLRustStatus::MissingCallback)?,
            style: callbacks.style.ok_or(SLRustStatus::MissingCallback)?,
            set_layout: callbacks.set_layout.ok_or(SLRustStatus::MissingCallback)?,
        })
    }
}

#[derive(Clone, Debug)]
struct SnapshotNode {
    external_id: SLRustNodeId,
    style: Style,
    has_explicit_direction_style: bool,
    physical_pixels_per_layout_unit: f32,
    children: Vec<usize>,
    layout: LayoutResult,
}

#[derive(Clone, Copy, Debug)]
struct PendingLayoutWrite {
    node: usize,
    layout: LayoutResult,
    constraints: Option<Constraints>,
}

const MAX_EXTERNAL_TREE_NODES: usize = 1_000_000;

struct ExternalTreeSnapshot {
    callbacks: SLRustTreeCallbacks,
    required: RequiredCallbacks,
    root: usize,
    nodes: Vec<SnapshotNode>,
    index_by_external_id: HashMap<SLRustNodeId, usize>,
    pending_layout_writes: Vec<PendingLayoutWrite>,
    callback_error: Cell<Option<SLRustStatus>>,
}

impl ExternalTreeSnapshot {
    fn build(callbacks: SLRustTreeCallbacks, root: SLRustNodeId) -> Result<Self, SLRustStatus> {
        let required = RequiredCallbacks::new(&callbacks)?;
        let mut tree = Self {
            callbacks,
            required,
            root: 0,
            nodes: Vec::new(),
            index_by_external_id: HashMap::new(),
            pending_layout_writes: Vec::new(),
            callback_error: Cell::new(None),
        };
        let mut visiting = HashSet::new();
        tree.root = tree.push_subtree(root, &mut visiting)?;
        Ok(tree)
    }

    fn push_subtree(
        &mut self,
        external_id: SLRustNodeId,
        visiting: &mut HashSet<SLRustNodeId>,
    ) -> Result<usize, SLRustStatus> {
        if self.index_by_external_id.contains_key(&external_id) {
            return Err(SLRustStatus::InvalidTree);
        }
        if !visiting.insert(external_id) {
            return Err(SLRustStatus::InvalidTree);
        }
        if self.nodes.len() >= MAX_EXTERNAL_TREE_NODES {
            visiting.remove(&external_id);
            return Err(SLRustStatus::UnsupportedTree);
        }

        let mut raw_style = SLRustStyle::default();
        if !(self.required.style)(
            self.callbacks.context,
            external_id,
            &mut raw_style as *mut SLRustStyle,
        ) {
            visiting.remove(&external_id);
            return Err(SLRustStatus::InvalidStyle);
        }
        let ffi_style = style_from_ffi(&raw_style)?;
        let physical_pixels_per_layout_unit =
            self.physical_pixels_per_layout_unit_from_callback(external_id)?;
        let index = self.nodes.len();
        self.index_by_external_id.insert(external_id, index);
        self.nodes.push(SnapshotNode {
            external_id,
            style: ffi_style.style,
            has_explicit_direction_style: ffi_style.has_explicit_direction_style,
            physical_pixels_per_layout_unit,
            children: Vec::new(),
            layout: LayoutResult::default(),
        });

        let child_count = (self.required.child_count)(self.callbacks.context, external_id);
        if child_count > MAX_EXTERNAL_TREE_NODES {
            visiting.remove(&external_id);
            return Err(SLRustStatus::UnsupportedTree);
        }
        for child_index in 0..child_count {
            let child_external_id =
                (self.required.child_at)(self.callbacks.context, external_id, child_index);
            if visiting.contains(&child_external_id) {
                visiting.remove(&external_id);
                return Err(SLRustStatus::InvalidTree);
            }
            let child_snapshot_index = self.push_subtree(child_external_id, visiting)?;
            self.nodes[index].children.push(child_snapshot_index);
        }

        visiting.remove(&external_id);
        Ok(index)
    }

    fn physical_pixels_per_layout_unit_from_callback(
        &self,
        external_id: SLRustNodeId,
    ) -> Result<f32, SLRustStatus> {
        let Some(callback) = self.callbacks.physical_pixels_per_layout_unit else {
            return Ok(1.0);
        };
        let mut value: f32 = 1.0;
        if !callback(self.callbacks.context, external_id, &mut value as *mut f32) {
            return Err(SLRustStatus::InvalidTree);
        }
        if value.is_finite() && value > 0.0 {
            Ok(value)
        } else {
            Err(SLRustStatus::InvalidTree)
        }
    }

    fn flush_layouts(&self) {
        for pending in &self.pending_layout_writes {
            let node = &self.nodes[pending.node];
            let layout = layout_result_to_ffi(pending.layout);
            if let (Some(set_layout_with_constraints), Some(constraints)) = (
                self.callbacks.set_layout_with_constraints,
                pending.constraints,
            ) {
                set_layout_with_constraints(
                    self.callbacks.context,
                    node.external_id,
                    constraints_to_ffi(constraints),
                    layout,
                );
            } else {
                (self.required.set_layout)(self.callbacks.context, node.external_id, layout);
            }
        }
    }

    fn callback_error(&self) -> Result<(), SLRustStatus> {
        self.callback_error.get().map_or(Ok(()), Err)
    }

    fn record_callback_error(&self, status: SLRustStatus) {
        if self.callback_error.get().is_none() {
            self.callback_error.set(Some(status));
        }
    }

    fn apply_owner_direction_to_unset_subtree(&mut self, node: usize, owner_direction: Direction) {
        self.apply_owner_direction_to_unset_subtree_inner(
            node,
            owner_layout_direction(owner_direction),
        );
    }

    fn apply_owner_direction_to_unset_subtree_inner(
        &mut self,
        node: usize,
        layout_direction: Direction,
    ) {
        if !self.nodes[node].has_explicit_direction_style {
            self.nodes[node].style.direction = layout_direction;
        }

        let children = self.nodes[node].children.clone();
        for child in children {
            self.apply_owner_direction_to_unset_subtree_inner(child, layout_direction);
        }
    }
}

fn owner_layout_direction(owner_direction: Direction) -> Direction {
    if owner_direction.is_any_rtl() {
        Direction::Rtl
    } else {
        Direction::Ltr
    }
}

impl LayoutTree for ExternalTreeSnapshot {
    type NodeId = usize;
    type Children<'a> = std::iter::Copied<std::slice::Iter<'a, usize>>;

    fn children(&self, node: Self::NodeId) -> Self::Children<'_> {
        self.nodes[node].children.iter().copied()
    }

    fn style(&self, node: Self::NodeId) -> &Style {
        &self.nodes[node].style
    }

    fn has_explicit_direction_style(&self, node: Self::NodeId) -> bool {
        self.nodes[node].has_explicit_direction_style
    }

    fn set_layout(&mut self, node: Self::NodeId, layout: LayoutResult) {
        self.nodes[node].layout = layout;
        if validate_layout_result(layout).is_err() {
            self.record_callback_error(SLRustStatus::InvalidTree);
            return;
        }
        self.pending_layout_writes.push(PendingLayoutWrite {
            node,
            layout,
            constraints: None,
        });
    }

    fn set_layout_with_constraints(
        &mut self,
        node: Self::NodeId,
        constraints: Constraints,
        layout: LayoutResult,
    ) {
        self.nodes[node].layout = layout;
        if validate_layout_result(layout).is_err() || validate_constraints(constraints).is_err() {
            self.record_callback_error(SLRustStatus::InvalidTree);
            return;
        }
        self.pending_layout_writes.push(PendingLayoutWrite {
            node,
            layout,
            constraints: Some(constraints),
        });
    }

    fn layout(&self, node: Self::NodeId) -> Option<LayoutResult> {
        Some(self.nodes[node].layout)
    }

    fn measure(&mut self, node: Self::NodeId, constraints: Constraints) -> Option<Size> {
        let measure = self.callbacks.measure?;
        let mut size = SLRustSize::default();
        if !measure(
            self.callbacks.context,
            self.nodes[node].external_id,
            constraints_to_ffi(constraints),
            &mut size as *mut SLRustSize,
        ) {
            self.record_callback_error(SLRustStatus::InvalidTree);
            return None;
        }
        let measured = size_from_ffi(size);
        if measured.is_none() {
            self.record_callback_error(SLRustStatus::InvalidTree);
        }
        measured
    }

    fn has_measure(&self, node: Self::NodeId) -> bool {
        self.callbacks.has_measure.is_some_and(|has_measure| {
            has_measure(self.callbacks.context, self.nodes[node].external_id)
        })
    }

    fn physical_pixels_per_layout_unit(&self, node: Self::NodeId) -> f32 {
        self.nodes[node].physical_pixels_per_layout_unit
    }

    fn baseline(&self, node: Self::NodeId, content_size: Size) -> Option<f32> {
        let baseline = self.callbacks.baseline?;
        if validate_layout_size(content_size).is_err() {
            self.record_callback_error(SLRustStatus::InvalidTree);
            return None;
        }
        let mut value: f32 = 0.0;
        if !baseline(
            self.callbacks.context,
            self.nodes[node].external_id,
            size_to_ffi(content_size),
            &mut value as *mut f32,
        ) {
            return None;
        }
        if !value.is_finite() {
            self.record_callback_error(SLRustStatus::InvalidTree);
            return None;
        }
        Some(value)
    }
}

#[derive(Clone, Debug, PartialEq)]
struct FfiStyle {
    style: Style,
    has_explicit_direction_style: bool,
}

fn style_from_ffi(style: &SLRustStyle) -> Result<FfiStyle, SLRustStatus> {
    Ok(FfiStyle {
        has_explicit_direction_style: style.has_explicit_direction,
        style: Style {
            display: display_from_ffi(style.display)?,
            position: position_from_ffi(style.position)?,
            box_sizing: box_sizing_from_ffi(style.box_sizing)?,
            direction: direction_from_ffi(style.direction)?,
            visibility: visibility_from_ffi(style.visibility)?,
            width: length_from_ffi(style.width)?,
            height: length_from_ffi(style.height)?,
            min_width: length_from_ffi(style.min_width)?,
            min_height: length_from_ffi(style.min_height)?,
            max_width: length_from_ffi(style.max_width)?,
            max_height: length_from_ffi(style.max_height)?,
            aspect_ratio: style
                .has_aspect_ratio
                .then(|| finite_f32(style.aspect_ratio))
                .transpose()?,
            left: length_from_ffi(style.left)?,
            right: length_from_ffi(style.right)?,
            top: length_from_ffi(style.top)?,
            bottom: length_from_ffi(style.bottom)?,
            margin: rect_length_from_ffi(style.margin)?,
            padding: rect_length_from_ffi(style.padding)?,
            border: rect_f32_from_ffi(style.border)?,
            flex_direction: flex_direction_from_ffi(style.flex_direction)?,
            flex_wrap: flex_wrap_from_ffi(style.flex_wrap)?,
            justify_content: justify_content_from_ffi(style.justify_content)?,
            align_items: align_items_from_ffi(style.align_items)?,
            align_self: style
                .has_align_self
                .then(|| align_items_from_ffi(style.align_self))
                .transpose()?,
            align_content: align_content_from_ffi(style.align_content)?,
            justify_items: justify_items_from_ffi(style.justify_items)?,
            justify_self: justify_items_from_ffi(style.justify_self)?,
            flex_grow: finite_f32(style.flex_grow)?,
            flex_shrink: finite_f32(style.flex_shrink)?,
            flex_basis: length_from_ffi(style.flex_basis)?,
            order: style.order,
            row_gap: length_from_ffi(style.row_gap)?,
            column_gap: length_from_ffi(style.column_gap)?,
            linear_orientation: linear_orientation_from_ffi(style.linear_orientation)?,
            linear_gravity: linear_gravity_from_ffi(style.linear_gravity)?,
            linear_layout_gravity: linear_layout_gravity_from_ffi(style.linear_layout_gravity)?,
            linear_cross_gravity: linear_cross_gravity_from_ffi(style.linear_cross_gravity)?,
            linear_weight: finite_f32(style.linear_weight)?,
            linear_weight_sum: finite_f32(style.linear_weight_sum)?,
            linear_column_count: optional_usize_from_i32(style.linear_column_count)?,
            list_main_axis_gap: length_from_ffi(style.list_main_axis_gap)?,
            list_cross_axis_gap: length_from_ffi(style.list_cross_axis_gap)?,
            list_component_type: list_component_type_from_ffi(style.list_component_type)?,
            grid_template_columns: lengths_from_ffi(
                style.grid_template_columns,
                style.grid_template_columns_len,
            )?,
            grid_template_rows: lengths_from_ffi(
                style.grid_template_rows,
                style.grid_template_rows_len,
            )?,
            grid_template_columns_max: lengths_from_ffi(
                style.grid_template_columns_max,
                style.grid_template_columns_max_len,
            )?,
            grid_template_rows_max: lengths_from_ffi(
                style.grid_template_rows_max,
                style.grid_template_rows_max_len,
            )?,
            grid_auto_columns: lengths_from_ffi(
                style.grid_auto_columns,
                style.grid_auto_columns_len,
            )?,
            grid_auto_rows: lengths_from_ffi(style.grid_auto_rows, style.grid_auto_rows_len)?,
            grid_auto_columns_max: lengths_from_ffi(
                style.grid_auto_columns_max,
                style.grid_auto_columns_max_len,
            )?,
            grid_auto_rows_max: lengths_from_ffi(
                style.grid_auto_rows_max,
                style.grid_auto_rows_max_len,
            )?,
            grid_auto_flow: grid_auto_flow_from_ffi(style.grid_auto_flow)?,
            grid_column_start: grid_line_from_ffi(style.grid_column_start),
            grid_column_end: grid_line_from_ffi(style.grid_column_end),
            grid_row_start: grid_line_from_ffi(style.grid_row_start),
            grid_row_end: grid_line_from_ffi(style.grid_row_end),
            grid_column_span: grid_span_from_ffi(style.grid_column_span)?,
            grid_row_span: grid_span_from_ffi(style.grid_row_span)?,
            relative_id: style.relative_id,
            relative_align_top: style.relative_align_top,
            relative_align_right: style.relative_align_right,
            relative_align_bottom: style.relative_align_bottom,
            relative_align_left: style.relative_align_left,
            relative_top_of: style.relative_top_of,
            relative_right_of: style.relative_right_of,
            relative_bottom_of: style.relative_bottom_of,
            relative_left_of: style.relative_left_of,
            relative_layout_once: style.relative_layout_once,
            relative_center: relative_center_from_ffi(style.relative_center)?,
        },
    })
}

fn style_to_ffi_defaults(style: &Style) -> SLRustStyle {
    SLRustStyle {
        display: display_to_ffi(style.display),
        position: position_to_ffi(style.position),
        box_sizing: box_sizing_to_ffi(style.box_sizing),
        direction: direction_to_ffi(style.direction),
        visibility: visibility_to_ffi(style.visibility),
        has_explicit_direction: false,
        width: length_to_ffi(style.width),
        height: length_to_ffi(style.height),
        min_width: length_to_ffi(style.min_width),
        min_height: length_to_ffi(style.min_height),
        max_width: length_to_ffi(style.max_width),
        max_height: length_to_ffi(style.max_height),
        aspect_ratio: style.aspect_ratio.unwrap_or(-1.0),
        has_aspect_ratio: style.aspect_ratio.is_some(),
        left: length_to_ffi(style.left),
        right: length_to_ffi(style.right),
        top: length_to_ffi(style.top),
        bottom: length_to_ffi(style.bottom),
        margin: rect_length_to_ffi(style.margin),
        padding: rect_length_to_ffi(style.padding),
        border: rect_f32_to_ffi(style.border),
        flex_direction: flex_direction_to_ffi(style.flex_direction),
        flex_wrap: flex_wrap_to_ffi(style.flex_wrap),
        justify_content: justify_content_to_ffi(style.justify_content),
        align_items: align_items_to_ffi(style.align_items),
        align_self: style.align_self.map_or(0, align_items_to_ffi),
        has_align_self: style.align_self.is_some(),
        align_content: align_content_to_ffi(style.align_content),
        justify_items: justify_items_to_ffi(style.justify_items),
        justify_self: justify_items_to_ffi(style.justify_self),
        flex_grow: style.flex_grow,
        flex_shrink: style.flex_shrink,
        flex_basis: length_to_ffi(style.flex_basis),
        order: style.order,
        row_gap: length_to_ffi(style.row_gap),
        column_gap: length_to_ffi(style.column_gap),
        linear_orientation: linear_orientation_to_ffi(style.linear_orientation),
        linear_gravity: linear_gravity_to_ffi(style.linear_gravity),
        linear_layout_gravity: linear_layout_gravity_to_ffi(style.linear_layout_gravity),
        linear_cross_gravity: linear_cross_gravity_to_ffi(style.linear_cross_gravity),
        linear_weight: style.linear_weight,
        linear_weight_sum: style.linear_weight_sum,
        linear_column_count: style.linear_column_count.map_or(-1, |value| value as i32),
        list_main_axis_gap: length_to_ffi(style.list_main_axis_gap),
        list_cross_axis_gap: length_to_ffi(style.list_cross_axis_gap),
        list_component_type: style
            .list_component_type
            .map_or(-1, list_component_type_to_ffi),
        grid_template_columns: ptr::null(),
        grid_template_columns_len: 0,
        grid_template_rows: ptr::null(),
        grid_template_rows_len: 0,
        grid_template_columns_max: ptr::null(),
        grid_template_columns_max_len: 0,
        grid_template_rows_max: ptr::null(),
        grid_template_rows_max_len: 0,
        grid_auto_columns: ptr::null(),
        grid_auto_columns_len: 0,
        grid_auto_rows: ptr::null(),
        grid_auto_rows_len: 0,
        grid_auto_columns_max: ptr::null(),
        grid_auto_columns_max_len: 0,
        grid_auto_rows_max: ptr::null(),
        grid_auto_rows_max_len: 0,
        grid_auto_flow: grid_auto_flow_to_ffi(style.grid_auto_flow),
        grid_column_start: style.grid_column_start.unwrap_or(0),
        grid_column_end: style.grid_column_end.unwrap_or(0),
        grid_row_start: style.grid_row_start.unwrap_or(0),
        grid_row_end: style.grid_row_end.unwrap_or(0),
        grid_column_span: style.grid_column_span,
        grid_row_span: style.grid_row_span,
        relative_id: style.relative_id,
        relative_align_top: style.relative_align_top,
        relative_align_right: style.relative_align_right,
        relative_align_bottom: style.relative_align_bottom,
        relative_align_left: style.relative_align_left,
        relative_top_of: style.relative_top_of,
        relative_right_of: style.relative_right_of,
        relative_bottom_of: style.relative_bottom_of,
        relative_left_of: style.relative_left_of,
        relative_layout_once: style.relative_layout_once,
        relative_center: relative_center_to_ffi(style.relative_center),
    }
}

fn length_from_ffi(length: SLRustLength) -> Result<Length, SLRustStatus> {
    match length.kind {
        0 => Ok(Length::Auto),
        1 => Ok(Length::points(finite_f32(length.value)?)),
        2 => Ok(Length::percent(finite_f32(length.value)?)),
        3 => Ok(Length::calc(
            finite_f32(length.value)?,
            finite_f32(length.percent)?,
        )),
        4 => Ok(Length::fr(finite_f32(length.value)?)),
        5 => Ok(Length::MaxContent),
        6 => Ok(Length::FitContent(if length.has_base {
            let fixed = finite_f32(length.value)?;
            let percent = finite_f32(length.percent)?;
            Some(if length.has_percentage {
                BaseLength::fixed_and_percent(fixed, percent)
            } else {
                BaseLength::fixed(fixed)
            })
        } else {
            None
        })),
        7 => Ok(Length::MinContent),
        _ => Err(SLRustStatus::InvalidStyle),
    }
}

fn length_to_ffi(length: Length) -> SLRustLength {
    match length {
        Length::Auto => SLRustLength {
            kind: SLRustLengthKind::Auto as i32,
            value: 0.0,
            percent: 0.0,
            has_base: false,
            has_percentage: false,
        },
        Length::Points(value) => SLRustLength {
            kind: SLRustLengthKind::Points as i32,
            value,
            percent: 0.0,
            has_base: false,
            has_percentage: false,
        },
        Length::Percent(value) => SLRustLength {
            kind: SLRustLengthKind::Percent as i32,
            value,
            percent: 0.0,
            has_base: false,
            has_percentage: false,
        },
        Length::Calc { fixed, percent } => SLRustLength {
            kind: SLRustLengthKind::Calc as i32,
            value: fixed,
            percent,
            has_base: true,
            has_percentage: true,
        },
        Length::Fr(value) => SLRustLength {
            kind: SLRustLengthKind::Fr as i32,
            value,
            percent: 0.0,
            has_base: false,
            has_percentage: false,
        },
        Length::MaxContent => SLRustLength {
            kind: SLRustLengthKind::MaxContent as i32,
            value: 0.0,
            percent: 0.0,
            has_base: false,
            has_percentage: false,
        },
        Length::MinContent => SLRustLength {
            kind: SLRustLengthKind::MinContent as i32,
            value: 0.0,
            percent: 0.0,
            has_base: false,
            has_percentage: false,
        },
        Length::FitContent(base) => {
            let (value, percent, has_base, has_percentage) =
                base.map_or((0.0, 0.0, false, false), |base| {
                    (
                        base.fixed_part(),
                        base.percentage_part(),
                        true,
                        base.contains_percentage(),
                    )
                });
            SLRustLength {
                kind: SLRustLengthKind::FitContent as i32,
                value,
                percent,
                has_base,
                has_percentage,
            }
        }
    }
}

fn lengths_from_ffi(values: *const SLRustLength, len: usize) -> Result<Vec<Length>, SLRustStatus> {
    if len == 0 {
        return Ok(Vec::new());
    }
    if values.is_null() {
        return Err(SLRustStatus::NullPointer);
    }
    let values = unsafe { std::slice::from_raw_parts(values, len) };
    values.iter().copied().map(length_from_ffi).collect()
}

fn rect_length_from_ffi(rect: SLRustRectLength) -> Result<Rect<Length>, SLRustStatus> {
    Ok(Rect::new(
        length_from_ffi(rect.left)?,
        length_from_ffi(rect.right)?,
        length_from_ffi(rect.top)?,
        length_from_ffi(rect.bottom)?,
    ))
}

fn rect_length_to_ffi(rect: Rect<Length>) -> SLRustRectLength {
    SLRustRectLength {
        left: length_to_ffi(rect.left),
        right: length_to_ffi(rect.right),
        top: length_to_ffi(rect.top),
        bottom: length_to_ffi(rect.bottom),
    }
}

fn rect_f32_from_ffi(rect: SLRustRectF32) -> Result<Rect<f32>, SLRustStatus> {
    Ok(Rect::new(
        finite_f32(rect.left)?,
        finite_f32(rect.right)?,
        finite_f32(rect.top)?,
        finite_f32(rect.bottom)?,
    ))
}

fn rect_f32_to_ffi(rect: Rect<f32>) -> SLRustRectF32 {
    SLRustRectF32 {
        left: rect.left,
        right: rect.right,
        top: rect.top,
        bottom: rect.bottom,
    }
}

fn size_to_ffi(size: Size) -> SLRustSize {
    SLRustSize {
        width: size.width,
        height: size.height,
    }
}

fn validate_layout_size(size: Size) -> Result<(), SLRustStatus> {
    if size.width.is_finite() && size.height.is_finite() {
        Ok(())
    } else {
        Err(SLRustStatus::InvalidTree)
    }
}

fn validate_constraints(constraints: Constraints) -> Result<(), SLRustStatus> {
    if constraints.width.size.is_finite() && constraints.height.size.is_finite() {
        Ok(())
    } else {
        Err(SLRustStatus::InvalidTree)
    }
}

fn validate_layout_result(layout: LayoutResult) -> Result<(), SLRustStatus> {
    validate_layout_size(layout.size)?;
    if layout.offset.x.is_finite()
        && layout.offset.y.is_finite()
        && layout.baseline.is_none_or(f32::is_finite)
        && rect_f32_is_finite(layout.margin)
        && rect_f32_is_finite(layout.padding)
        && rect_f32_is_finite(layout.border)
        && rect_f32_is_finite(layout.sticky_pos)
    {
        Ok(())
    } else {
        Err(SLRustStatus::InvalidTree)
    }
}

fn rect_f32_is_finite(rect: Rect<f32>) -> bool {
    rect.left.is_finite()
        && rect.right.is_finite()
        && rect.top.is_finite()
        && rect.bottom.is_finite()
}

fn size_from_ffi(size: SLRustSize) -> Option<Size> {
    (size.width.is_finite() && size.height.is_finite()).then(|| Size::new(size.width, size.height))
}

fn point_to_ffi(point: Point) -> SLRustPoint {
    SLRustPoint {
        x: point.x,
        y: point.y,
    }
}

fn side_constraint_from_ffi(
    constraint: SLRustSideConstraint,
) -> Result<SideConstraint, SLRustStatus> {
    match constraint.mode {
        0 => Ok(SideConstraint::indefinite()),
        1 => Ok(SideConstraint::definite(finite_f32(constraint.size)?)),
        2 => Ok(SideConstraint::at_most(finite_f32(constraint.size)?)),
        _ => Err(SLRustStatus::InvalidStyle),
    }
}

fn side_constraint_to_ffi(constraint: SideConstraint) -> SLRustSideConstraint {
    let mode = match constraint.mode {
        MeasureMode::Indefinite => SLRustMeasureMode::Indefinite,
        MeasureMode::Definite => SLRustMeasureMode::Definite,
        MeasureMode::AtMost => SLRustMeasureMode::AtMost,
    };
    SLRustSideConstraint {
        size: constraint.size,
        mode: mode as i32,
    }
}

fn constraints_from_ffi(constraints: SLRustConstraints) -> Result<Constraints, SLRustStatus> {
    Ok(Constraints::new(
        side_constraint_from_ffi(constraints.width)?,
        side_constraint_from_ffi(constraints.height)?,
    ))
}

fn constraints_to_ffi(constraints: Constraints) -> SLRustConstraints {
    SLRustConstraints {
        width: side_constraint_to_ffi(constraints.width),
        height: side_constraint_to_ffi(constraints.height),
    }
}

fn layout_result_to_ffi(layout: LayoutResult) -> SLRustLayoutResult {
    SLRustLayoutResult {
        offset: point_to_ffi(layout.offset),
        size: size_to_ffi(layout.size),
        baseline: layout.baseline.unwrap_or(0.0),
        has_baseline: layout.baseline.is_some(),
        margin: rect_f32_to_ffi(layout.margin),
        padding: rect_f32_to_ffi(layout.padding),
        border: rect_f32_to_ffi(layout.border),
        sticky_pos: rect_f32_to_ffi(layout.sticky_pos),
    }
}

fn finite_f32(value: f32) -> Result<f32, SLRustStatus> {
    value
        .is_finite()
        .then_some(value)
        .ok_or(SLRustStatus::InvalidStyle)
}

fn optional_usize_from_i32(value: i32) -> Result<Option<usize>, SLRustStatus> {
    if value < 0 {
        Ok(None)
    } else {
        usize::try_from(value)
            .map(Some)
            .map_err(|_| SLRustStatus::InvalidStyle)
    }
}

fn grid_line_from_ffi(value: i32) -> Option<i32> {
    (value != 0).then_some(value)
}

fn grid_span_from_ffi(value: usize) -> Result<usize, SLRustStatus> {
    (value > 0)
        .then_some(value)
        .ok_or(SLRustStatus::InvalidStyle)
}

fn display_from_ffi(value: i32) -> Result<Display, SLRustStatus> {
    match value {
        0 => Ok(Display::None),
        1 => Ok(Display::Block),
        2 => Ok(Display::Flex),
        3 => Ok(Display::Linear),
        4 => Ok(Display::Relative),
        5 => Ok(Display::Grid),
        _ => Err(SLRustStatus::InvalidStyle),
    }
}

fn display_to_ffi(value: Display) -> i32 {
    match value {
        Display::None => 0,
        Display::Block => 1,
        Display::Flex => 2,
        Display::Linear => 3,
        Display::Relative => 4,
        Display::Grid => 5,
    }
}

fn position_from_ffi(value: i32) -> Result<PositionType, SLRustStatus> {
    match value {
        0 => Ok(PositionType::Static),
        1 => Ok(PositionType::Relative),
        2 => Ok(PositionType::Absolute),
        3 => Ok(PositionType::Fixed),
        4 => Ok(PositionType::Sticky),
        _ => Err(SLRustStatus::InvalidStyle),
    }
}

fn position_to_ffi(value: PositionType) -> i32 {
    match value {
        PositionType::Static => 0,
        PositionType::Relative => 1,
        PositionType::Absolute => 2,
        PositionType::Fixed => 3,
        PositionType::Sticky => 4,
    }
}

fn box_sizing_from_ffi(value: i32) -> Result<BoxSizing, SLRustStatus> {
    match value {
        0 => Ok(BoxSizing::ContentBox),
        1 => Ok(BoxSizing::BorderBox),
        _ => Err(SLRustStatus::InvalidStyle),
    }
}

fn box_sizing_to_ffi(value: BoxSizing) -> i32 {
    match value {
        BoxSizing::ContentBox => 0,
        BoxSizing::BorderBox => 1,
    }
}

fn direction_from_ffi(value: i32) -> Result<Direction, SLRustStatus> {
    match value {
        0 => Ok(Direction::Ltr),
        1 => Ok(Direction::Rtl),
        _ => Err(SLRustStatus::InvalidStyle),
    }
}

fn direction_to_ffi(value: Direction) -> i32 {
    match value {
        Direction::Ltr => 0,
        Direction::Rtl => 1,
    }
}

fn visibility_from_ffi(value: i32) -> Result<Visibility, SLRustStatus> {
    match value {
        0 => Ok(Visibility::Visible),
        1 => Ok(Visibility::Hidden),
        2 => Ok(Visibility::Collapse),
        _ => Err(SLRustStatus::InvalidStyle),
    }
}

fn visibility_to_ffi(value: Visibility) -> i32 {
    match value {
        Visibility::Visible => 0,
        Visibility::Hidden => 1,
        Visibility::Collapse => 2,
    }
}

fn flex_direction_from_ffi(value: i32) -> Result<FlexDirection, SLRustStatus> {
    match value {
        0 => Ok(FlexDirection::Row),
        1 => Ok(FlexDirection::RowReverse),
        2 => Ok(FlexDirection::Column),
        3 => Ok(FlexDirection::ColumnReverse),
        _ => Err(SLRustStatus::InvalidStyle),
    }
}

fn flex_direction_to_ffi(value: FlexDirection) -> i32 {
    match value {
        FlexDirection::Row => 0,
        FlexDirection::RowReverse => 1,
        FlexDirection::Column => 2,
        FlexDirection::ColumnReverse => 3,
    }
}

fn linear_orientation_from_ffi(value: i32) -> Result<LinearOrientation, SLRustStatus> {
    match value {
        0 => Ok(LinearOrientation::Horizontal),
        1 => Ok(LinearOrientation::HorizontalReverse),
        2 => Ok(LinearOrientation::Vertical),
        3 => Ok(LinearOrientation::VerticalReverse),
        4 => Ok(LinearOrientation::Row),
        5 => Ok(LinearOrientation::RowReverse),
        6 => Ok(LinearOrientation::Column),
        7 => Ok(LinearOrientation::ColumnReverse),
        _ => Err(SLRustStatus::InvalidStyle),
    }
}

fn linear_orientation_to_ffi(value: LinearOrientation) -> i32 {
    match value {
        LinearOrientation::Horizontal => 0,
        LinearOrientation::HorizontalReverse => 1,
        LinearOrientation::Vertical => 2,
        LinearOrientation::VerticalReverse => 3,
        LinearOrientation::Row => 4,
        LinearOrientation::RowReverse => 5,
        LinearOrientation::Column => 6,
        LinearOrientation::ColumnReverse => 7,
    }
}

fn flex_wrap_from_ffi(value: i32) -> Result<FlexWrap, SLRustStatus> {
    match value {
        0 => Ok(FlexWrap::NoWrap),
        1 => Ok(FlexWrap::Wrap),
        2 => Ok(FlexWrap::WrapReverse),
        _ => Err(SLRustStatus::InvalidStyle),
    }
}

fn flex_wrap_to_ffi(value: FlexWrap) -> i32 {
    match value {
        FlexWrap::NoWrap => 0,
        FlexWrap::Wrap => 1,
        FlexWrap::WrapReverse => 2,
    }
}

fn justify_content_from_ffi(value: i32) -> Result<JustifyContent, SLRustStatus> {
    match value {
        0 => Ok(JustifyContent::Stretch),
        1 => Ok(JustifyContent::FlexStart),
        2 => Ok(JustifyContent::Start),
        3 => Ok(JustifyContent::Center),
        4 => Ok(JustifyContent::FlexEnd),
        5 => Ok(JustifyContent::End),
        6 => Ok(JustifyContent::SpaceBetween),
        7 => Ok(JustifyContent::SpaceAround),
        8 => Ok(JustifyContent::SpaceEvenly),
        _ => Err(SLRustStatus::InvalidStyle),
    }
}

fn justify_content_to_ffi(value: JustifyContent) -> i32 {
    match value {
        JustifyContent::Stretch => 0,
        JustifyContent::FlexStart => 1,
        JustifyContent::Start => 2,
        JustifyContent::Center => 3,
        JustifyContent::FlexEnd => 4,
        JustifyContent::End => 5,
        JustifyContent::SpaceBetween => 6,
        JustifyContent::SpaceAround => 7,
        JustifyContent::SpaceEvenly => 8,
    }
}

fn align_items_from_ffi(value: i32) -> Result<AlignItems, SLRustStatus> {
    match value {
        0 => Ok(AlignItems::Stretch),
        1 => Ok(AlignItems::FlexStart),
        2 => Ok(AlignItems::Start),
        3 => Ok(AlignItems::Center),
        4 => Ok(AlignItems::FlexEnd),
        5 => Ok(AlignItems::End),
        6 => Ok(AlignItems::Baseline),
        _ => Err(SLRustStatus::InvalidStyle),
    }
}

fn align_items_to_ffi(value: AlignItems) -> i32 {
    match value {
        AlignItems::Stretch => 0,
        AlignItems::FlexStart => 1,
        AlignItems::Start => 2,
        AlignItems::Center => 3,
        AlignItems::FlexEnd => 4,
        AlignItems::End => 5,
        AlignItems::Baseline => 6,
    }
}

fn align_content_from_ffi(value: i32) -> Result<AlignContent, SLRustStatus> {
    match value {
        0 => Ok(AlignContent::FlexStart),
        1 => Ok(AlignContent::Center),
        2 => Ok(AlignContent::FlexEnd),
        3 => Ok(AlignContent::SpaceBetween),
        4 => Ok(AlignContent::SpaceAround),
        5 => Ok(AlignContent::SpaceEvenly),
        6 => Ok(AlignContent::Stretch),
        7 => Ok(AlignContent::Start),
        8 => Ok(AlignContent::End),
        _ => Err(SLRustStatus::InvalidStyle),
    }
}

fn align_content_to_ffi(value: AlignContent) -> i32 {
    match value {
        AlignContent::FlexStart => 0,
        AlignContent::Center => 1,
        AlignContent::FlexEnd => 2,
        AlignContent::SpaceBetween => 3,
        AlignContent::SpaceAround => 4,
        AlignContent::SpaceEvenly => 5,
        AlignContent::Stretch => 6,
        AlignContent::Start => 7,
        AlignContent::End => 8,
    }
}

fn justify_items_from_ffi(value: i32) -> Result<JustifyItems, SLRustStatus> {
    match value {
        0 => Ok(JustifyItems::Auto),
        1 => Ok(JustifyItems::Stretch),
        2 => Ok(JustifyItems::Start),
        3 => Ok(JustifyItems::Center),
        4 => Ok(JustifyItems::End),
        _ => Err(SLRustStatus::InvalidStyle),
    }
}

fn justify_items_to_ffi(value: JustifyItems) -> i32 {
    match value {
        JustifyItems::Auto => 0,
        JustifyItems::Stretch => 1,
        JustifyItems::Start => 2,
        JustifyItems::Center => 3,
        JustifyItems::End => 4,
    }
}

fn grid_auto_flow_from_ffi(value: i32) -> Result<GridAutoFlow, SLRustStatus> {
    match value {
        0 => Ok(GridAutoFlow::Row),
        1 => Ok(GridAutoFlow::Column),
        2 => Ok(GridAutoFlow::Dense),
        3 => Ok(GridAutoFlow::RowDense),
        4 => Ok(GridAutoFlow::ColumnDense),
        _ => Err(SLRustStatus::InvalidStyle),
    }
}

fn grid_auto_flow_to_ffi(value: GridAutoFlow) -> i32 {
    match value {
        GridAutoFlow::Row => 0,
        GridAutoFlow::Column => 1,
        GridAutoFlow::Dense => 2,
        GridAutoFlow::RowDense => 3,
        GridAutoFlow::ColumnDense => 4,
    }
}

fn relative_center_from_ffi(value: i32) -> Result<RelativeCenter, SLRustStatus> {
    match value {
        0 => Ok(RelativeCenter::None),
        1 => Ok(RelativeCenter::Horizontal),
        2 => Ok(RelativeCenter::Vertical),
        3 => Ok(RelativeCenter::Both),
        _ => Err(SLRustStatus::InvalidStyle),
    }
}

fn relative_center_to_ffi(value: RelativeCenter) -> i32 {
    match value {
        RelativeCenter::None => 0,
        RelativeCenter::Horizontal => 1,
        RelativeCenter::Vertical => 2,
        RelativeCenter::Both => 3,
    }
}

fn linear_gravity_from_ffi(value: i32) -> Result<LinearGravity, SLRustStatus> {
    match value {
        0 => Ok(LinearGravity::None),
        1 => Ok(LinearGravity::Top),
        2 => Ok(LinearGravity::Bottom),
        3 => Ok(LinearGravity::Left),
        4 => Ok(LinearGravity::Right),
        5 => Ok(LinearGravity::CenterVertical),
        6 => Ok(LinearGravity::CenterHorizontal),
        7 => Ok(LinearGravity::SpaceBetween),
        8 => Ok(LinearGravity::Start),
        9 => Ok(LinearGravity::End),
        10 => Ok(LinearGravity::Center),
        _ => Err(SLRustStatus::InvalidStyle),
    }
}

fn linear_gravity_to_ffi(value: LinearGravity) -> i32 {
    match value {
        LinearGravity::None => 0,
        LinearGravity::Top => 1,
        LinearGravity::Bottom => 2,
        LinearGravity::Left => 3,
        LinearGravity::Right => 4,
        LinearGravity::CenterVertical => 5,
        LinearGravity::CenterHorizontal => 6,
        LinearGravity::SpaceBetween => 7,
        LinearGravity::Start => 8,
        LinearGravity::End => 9,
        LinearGravity::Center => 10,
    }
}

fn linear_layout_gravity_from_ffi(value: i32) -> Result<LinearLayoutGravity, SLRustStatus> {
    match value {
        0 => Ok(LinearLayoutGravity::None),
        1 => Ok(LinearLayoutGravity::Top),
        2 => Ok(LinearLayoutGravity::Bottom),
        3 => Ok(LinearLayoutGravity::Left),
        4 => Ok(LinearLayoutGravity::Right),
        5 => Ok(LinearLayoutGravity::CenterVertical),
        6 => Ok(LinearLayoutGravity::CenterHorizontal),
        7 => Ok(LinearLayoutGravity::FillVertical),
        8 => Ok(LinearLayoutGravity::FillHorizontal),
        9 => Ok(LinearLayoutGravity::Center),
        10 => Ok(LinearLayoutGravity::Stretch),
        11 => Ok(LinearLayoutGravity::Start),
        12 => Ok(LinearLayoutGravity::End),
        _ => Err(SLRustStatus::InvalidStyle),
    }
}

fn linear_layout_gravity_to_ffi(value: LinearLayoutGravity) -> i32 {
    match value {
        LinearLayoutGravity::None => 0,
        LinearLayoutGravity::Top => 1,
        LinearLayoutGravity::Bottom => 2,
        LinearLayoutGravity::Left => 3,
        LinearLayoutGravity::Right => 4,
        LinearLayoutGravity::CenterVertical => 5,
        LinearLayoutGravity::CenterHorizontal => 6,
        LinearLayoutGravity::FillVertical => 7,
        LinearLayoutGravity::FillHorizontal => 8,
        LinearLayoutGravity::Center => 9,
        LinearLayoutGravity::Stretch => 10,
        LinearLayoutGravity::Start => 11,
        LinearLayoutGravity::End => 12,
    }
}

fn linear_cross_gravity_from_ffi(value: i32) -> Result<LinearCrossGravity, SLRustStatus> {
    match value {
        0 => Ok(LinearCrossGravity::None),
        1 => Ok(LinearCrossGravity::Start),
        2 => Ok(LinearCrossGravity::End),
        3 => Ok(LinearCrossGravity::Center),
        4 => Ok(LinearCrossGravity::Stretch),
        _ => Err(SLRustStatus::InvalidStyle),
    }
}

fn linear_cross_gravity_to_ffi(value: LinearCrossGravity) -> i32 {
    match value {
        LinearCrossGravity::None => 0,
        LinearCrossGravity::Start => 1,
        LinearCrossGravity::End => 2,
        LinearCrossGravity::Center => 3,
        LinearCrossGravity::Stretch => 4,
    }
}

fn list_component_type_from_ffi(value: i32) -> Result<Option<ListComponentType>, SLRustStatus> {
    match value {
        -1 => Ok(None),
        0 => Ok(Some(ListComponentType::Header)),
        1 => Ok(Some(ListComponentType::Footer)),
        2 => Ok(Some(ListComponentType::ListRow)),
        3 => Ok(Some(ListComponentType::Default)),
        _ => Err(SLRustStatus::InvalidStyle),
    }
}

fn list_component_type_to_ffi(value: ListComponentType) -> i32 {
    match value {
        ListComponentType::Header => 0,
        ListComponentType::Footer => 1,
        ListComponentType::ListRow => 2,
        ListComponentType::Default => 3,
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::ffi::CStr;
    use std::fs;
    use std::path::Path;

    const STYLE_ENUMS_REQUIRING_FFI_TEST_CASES: &[&str] = &[
        "Display",
        "PositionType",
        "BoxSizing",
        "Direction",
        "FlexDirection",
        "LinearOrientation",
        "FlexWrap",
        "JustifyContent",
        "AlignItems",
        "AlignContent",
        "JustifyItems",
        "GridAutoFlow",
        "RelativeCenter",
    ];

    const STYLE_DATA_ENUMS_REQUIRING_FFI_TEST_CASES: &[&str] = &[
        "LinearGravity",
        "LinearLayoutGravity",
        "LinearCrossGravity",
        "ListComponentType",
    ];

    const CURRENT_SOURCE: &str = include_str!("lib.rs");

    fn assert_close(actual: f32, expected: f32) {
        let delta = (actual - expected).abs();
        assert!(
            delta < 0.0001,
            "expected {actual} to be within tolerance of {expected}"
        );
    }

    #[test]
    fn status_name_returns_stable_c_strings_for_diagnostics() {
        for (status, expected) in [
            (SLRustStatus::Ok as i32, "Ok"),
            (SLRustStatus::NullPointer as i32, "NullPointer"),
            (SLRustStatus::MissingCallback as i32, "MissingCallback"),
            (SLRustStatus::InvalidStyle as i32, "InvalidStyle"),
            (SLRustStatus::InvalidTree as i32, "InvalidTree"),
            (SLRustStatus::Panic as i32, "Panic"),
            (SLRustStatus::AbiMismatch as i32, "AbiMismatch"),
            (SLRustStatus::Disabled as i32, "Disabled"),
            (SLRustStatus::UnsupportedTree as i32, "UnsupportedTree"),
            (
                SLRustStatus::FixedNodeSetMismatch as i32,
                "FixedNodeSetMismatch",
            ),
            (i32::MAX, "Unknown"),
        ] {
            let name = unsafe { CStr::from_ptr(SLRustStatusName(status)) };
            assert_eq!(name.to_str().unwrap(), expected);
        }
    }

    #[test]
    fn abi_validation_accepts_older_minor_and_rejects_newer_minor() {
        let current = current_abi_info();
        assert_eq!(validate_abi_info(&current), Ok(()));

        let mut older_caller = current;
        older_caller.version_minor = current.version_minor.saturating_sub(1);
        assert_eq!(validate_abi_info(&older_caller), Ok(()));

        let mut newer_caller = current;
        newer_caller.version_minor = current.version_minor + 1;
        assert_eq!(
            validate_abi_info(&newer_caller),
            Err(SLRustStatus::AbiMismatch)
        );
    }

    fn assert_enum_mapping<T>(
        cases: &[(i32, T)],
        from_ffi: fn(i32) -> Result<T, SLRustStatus>,
        to_ffi: fn(T) -> i32,
    ) where
        T: Copy + std::fmt::Debug + PartialEq,
    {
        for &(raw, value) in cases {
            assert_eq!(from_ffi(raw), Ok(value));
            assert_eq!(to_ffi(value), raw);
        }
        assert_eq!(from_ffi(i32::MAX), Err(SLRustStatus::InvalidStyle));
    }

    fn assert_invalid_style(style: SLRustStyle) {
        assert!(matches!(
            style_from_ffi(&style),
            Err(SLRustStatus::InvalidStyle)
        ));
    }

    fn function_body<'a>(source: &'a str, function_name: &str) -> Option<&'a str> {
        let function_marker = format!("fn {function_name}");
        let function_start = source.find(&function_marker)?;
        let source_after_marker = &source[function_start..];
        let body_start = source_after_marker.find('{')?;
        let body = &source_after_marker[body_start + 1..];
        let mut depth = 1usize;
        for (idx, character) in body.char_indices() {
            match character {
                '{' => depth += 1,
                '}' => {
                    depth -= 1;
                    if depth == 0 {
                        return Some(&body[..idx]);
                    }
                }
                _ => {}
            }
        }
        None
    }

    fn rust_struct_fields(source: &str, struct_name: &str) -> Vec<String> {
        let struct_marker = format!("pub struct {struct_name}");
        let struct_start = source
            .find(&struct_marker)
            .unwrap_or_else(|| panic!("{struct_name} struct exists"));
        let source_after_marker = &source[struct_start..];
        let body_start = source_after_marker
            .find('{')
            .map(|offset| struct_start + offset + 1)
            .unwrap_or_else(|| panic!("{struct_name} struct body starts"));
        let body_end = source[body_start..]
            .find('}')
            .map(|offset| body_start + offset)
            .unwrap_or_else(|| panic!("{struct_name} struct body ends"));

        source[body_start..body_end]
            .lines()
            .map(str::trim)
            .filter_map(|line| line.strip_prefix("pub "))
            .filter_map(|line| line.split_once(':').map(|(field, _)| field.trim()))
            .filter(|field| !field.is_empty())
            .map(ToOwned::to_owned)
            .collect()
    }

    fn without_ascii_whitespace(source: &str) -> String {
        source
            .chars()
            .filter(|character| !character.is_ascii_whitespace())
            .collect()
    }

    fn enum_variants(source: &str, enum_name: &str) -> Vec<String> {
        let enum_marker = format!("pub enum {enum_name}");
        let Some(enum_start) = source.find(&enum_marker) else {
            return Vec::new();
        };
        let source_after_marker = &source[enum_start..];
        let Some(body_start) = source_after_marker.find('{') else {
            return Vec::new();
        };
        let source_after_body_start = &source_after_marker[body_start + 1..];
        let Some(body_end) = source_after_body_start.find('}') else {
            return Vec::new();
        };

        source_after_body_start[..body_end]
            .lines()
            .map(str::trim)
            .filter(|line| !line.is_empty() && !line.starts_with("#["))
            .filter_map(|line| {
                let name = line
                    .split(',')
                    .next()
                    .unwrap_or_default()
                    .split('=')
                    .next()
                    .unwrap_or_default()
                    .trim();
                name.chars()
                    .next()
                    .is_some_and(char::is_uppercase)
                    .then(|| name.to_owned())
            })
            .collect()
    }

    #[derive(Clone, Debug)]
    struct TestNode {
        id: SLRustNodeId,
        style: SLRustStyle,
        children: Vec<SLRustNodeId>,
        measured_size: Option<SLRustSize>,
        baseline: Option<f32>,
        layout: Option<SLRustLayoutResult>,
        last_constraints: Option<SLRustConstraints>,
    }

    #[derive(Default)]
    struct TestContext {
        nodes: Vec<TestNode>,
    }

    impl TestContext {
        fn node(&self, id: SLRustNodeId) -> &TestNode {
            self.nodes
                .iter()
                .find(|node| node.id == id)
                .expect("test node exists")
        }

        fn node_mut(&mut self, id: SLRustNodeId) -> &mut TestNode {
            self.nodes
                .iter_mut()
                .find(|node| node.id == id)
                .expect("test node exists")
        }
    }

    extern "C" fn child_count(context: *mut c_void, node: SLRustNodeId) -> usize {
        let context = unsafe { &*(context as *const TestContext) };
        context.node(node).children.len()
    }

    extern "C" fn child_count_too_large(_context: *mut c_void, _node: SLRustNodeId) -> usize {
        MAX_EXTERNAL_TREE_NODES + 1
    }

    extern "C" fn child_at(context: *mut c_void, node: SLRustNodeId, index: usize) -> SLRustNodeId {
        let context = unsafe { &*(context as *const TestContext) };
        context.node(node).children[index]
    }

    extern "C" fn style(
        context: *mut c_void,
        node: SLRustNodeId,
        out_style: *mut SLRustStyle,
    ) -> bool {
        let context = unsafe { &*(context as *const TestContext) };
        let Some(out_style) = (unsafe { out_style.as_mut() }) else {
            return false;
        };
        *out_style = context.node(node).style;
        true
    }

    extern "C" fn failing_style(
        _context: *mut c_void,
        _node: SLRustNodeId,
        _out_style: *mut SLRustStyle,
    ) -> bool {
        false
    }

    extern "C" fn always_has_measure(_context: *mut c_void, _node: SLRustNodeId) -> bool {
        true
    }

    extern "C" fn failing_measure(
        _context: *mut c_void,
        _node: SLRustNodeId,
        _constraints: SLRustConstraints,
        _out_size: *mut SLRustSize,
    ) -> bool {
        false
    }

    extern "C" fn non_finite_measure(
        _context: *mut c_void,
        _node: SLRustNodeId,
        _constraints: SLRustConstraints,
        out_size: *mut SLRustSize,
    ) -> bool {
        let Some(out_size) = (unsafe { out_size.as_mut() }) else {
            return false;
        };
        *out_size = SLRustSize {
            width: f32::NAN,
            height: 1.0,
        };
        true
    }

    extern "C" fn set_layout(context: *mut c_void, node: SLRustNodeId, layout: SLRustLayoutResult) {
        let context = unsafe { &mut *(context as *mut TestContext) };
        context.node_mut(node).layout = Some(layout);
    }

    extern "C" fn set_layout_with_constraints(
        context: *mut c_void,
        node: SLRustNodeId,
        constraints: SLRustConstraints,
        layout: SLRustLayoutResult,
    ) {
        let context = unsafe { &mut *(context as *mut TestContext) };
        let node = context.node_mut(node);
        node.layout = Some(layout);
        node.last_constraints = Some(constraints);
    }

    extern "C" fn has_measure(context: *mut c_void, node: SLRustNodeId) -> bool {
        let context = unsafe { &*(context as *const TestContext) };
        context.node(node).measured_size.is_some()
    }

    extern "C" fn measure(
        context: *mut c_void,
        node: SLRustNodeId,
        constraints: SLRustConstraints,
        out_size: *mut SLRustSize,
    ) -> bool {
        let context = unsafe { &mut *(context as *mut TestContext) };
        let test_node = context.node_mut(node);
        test_node.last_constraints = Some(constraints);
        let Some(size) = test_node.measured_size else {
            return false;
        };
        let Some(out_size) = (unsafe { out_size.as_mut() }) else {
            return false;
        };
        *out_size = size;
        true
    }

    extern "C" fn baseline(
        context: *mut c_void,
        node: SLRustNodeId,
        _content_size: SLRustSize,
        out_baseline: *mut f32,
    ) -> bool {
        let context = unsafe { &*(context as *const TestContext) };
        let Some(baseline) = context.node(node).baseline else {
            return false;
        };
        let Some(out_baseline) = (unsafe { out_baseline.as_mut() }) else {
            return false;
        };
        *out_baseline = baseline;
        true
    }

    extern "C" fn baseline_from_content_height(
        _context: *mut c_void,
        _node: SLRustNodeId,
        content_size: SLRustSize,
        out_baseline: *mut f32,
    ) -> bool {
        let Some(out_baseline) = (unsafe { out_baseline.as_mut() }) else {
            return false;
        };
        *out_baseline = content_size.height - 5.0;
        true
    }

    extern "C" fn two_physical_pixels_per_layout_unit(
        _context: *mut c_void,
        _node: SLRustNodeId,
        out_value: *mut f32,
    ) -> bool {
        let Some(out_value) = (unsafe { out_value.as_mut() }) else {
            return false;
        };
        *out_value = 2.0;
        true
    }

    extern "C" fn non_finite_physical_pixels_per_layout_unit(
        _context: *mut c_void,
        _node: SLRustNodeId,
        out_value: *mut f32,
    ) -> bool {
        let Some(out_value) = (unsafe { out_value.as_mut() }) else {
            return false;
        };
        *out_value = f32::NAN;
        true
    }

    extern "C" fn non_finite_baseline(
        _context: *mut c_void,
        _node: SLRustNodeId,
        _content_size: SLRustSize,
        out_baseline: *mut f32,
    ) -> bool {
        let Some(out_baseline) = (unsafe { out_baseline.as_mut() }) else {
            return false;
        };
        *out_baseline = f32::NAN;
        true
    }

    fn callbacks(context: &mut TestContext) -> SLRustTreeCallbacks {
        SLRustTreeCallbacks {
            context: context as *mut TestContext as *mut c_void,
            child_count: Some(child_count),
            child_at: Some(child_at),
            style: Some(style),
            set_layout: Some(set_layout),
            set_layout_with_constraints: Some(set_layout_with_constraints),
            has_measure: Some(has_measure),
            measure: Some(measure),
            baseline: Some(baseline),
            physical_pixels_per_layout_unit: None,
        }
    }

    #[test]
    fn external_tree_snapshot_buffers_layout_writebacks_until_flush() {
        let mut context = TestContext {
            nodes: vec![TestNode {
                id: 1,
                style: SLRustStyle::default(),
                children: Vec::new(),
                measured_size: None,
                baseline: None,
                layout: None,
                last_constraints: None,
            }],
        };
        let callbacks = callbacks(&mut context);
        let mut tree = ExternalTreeSnapshot::build(callbacks, 1).expect("snapshot builds");
        let root = tree.root;
        let constraints = Constraints {
            width: SideConstraint::definite(37.0),
            height: SideConstraint::at_most(19.0),
        };
        let layout = LayoutResult {
            size: Size::new(37.0, 11.0),
            ..LayoutResult::default()
        };
        let final_layout = LayoutResult {
            size: Size::new(42.0, 13.0),
            ..LayoutResult::default()
        };

        tree.set_layout_with_constraints(root, constraints, layout);
        tree.set_layout(root, final_layout);

        assert!(context.node(1).layout.is_none());
        assert!(context.node(1).last_constraints.is_none());
        assert_eq!(tree.pending_layout_writes.len(), 2);
        assert!(tree.pending_layout_writes[0].constraints.is_some());
        assert!(tree.pending_layout_writes[1].constraints.is_none());
        tree.flush_layouts();
        assert_eq!(
            context.node(1).layout.expect("layout flushed").size.width,
            42.0
        );
        let flushed_constraints = context
            .node(1)
            .last_constraints
            .expect("constraints flushed");
        assert_eq!(
            flushed_constraints.width.mode,
            SLRustMeasureMode::Definite as i32
        );
        assert_eq!(flushed_constraints.width.size, 37.0);
        assert_eq!(
            flushed_constraints.height.mode,
            SLRustMeasureMode::AtMost as i32
        );
        assert_eq!(flushed_constraints.height.size, 19.0);
    }

    #[test]
    fn external_tree_snapshot_flex_stretch_reuse_reexports_cached_descendant_layout() {
        let root_style = SLRustStyle {
            display: display_to_ffi(Display::Flex),
            flex_direction: flex_direction_to_ffi(FlexDirection::Row),
            align_items: align_items_to_ffi(AlignItems::Stretch),
            ..SLRustStyle::default()
        };
        let child_style = SLRustStyle {
            display: display_to_ffi(Display::Block),
            ..SLRustStyle::default()
        };
        let grandchild_style = SLRustStyle {
            width: length_to_ffi(Length::points(10.0)),
            height: length_to_ffi(Length::points(20.0)),
            ..SLRustStyle::default()
        };
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: root_style,
                    children: vec![2],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: child_style,
                    children: vec![3],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 3,
                    style: grandchild_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let mut tree = ExternalTreeSnapshot::build(callbacks, 1).expect("snapshot builds");
        let root = tree.root;
        let size = LayoutEngine::new().layout(&mut tree, root, Constraints::definite(30.0, 20.0));

        assert_close(size.width, 30.0);
        assert_close(size.height, 20.0);
        let grandchild = *tree
            .index_by_external_id
            .get(&3)
            .expect("grandchild snapshot index");
        let grandchild_writes = tree
            .pending_layout_writes
            .iter()
            .filter(|write| write.node == grandchild)
            .collect::<Vec<_>>();
        assert!(
            grandchild_writes.len() >= 2,
            "grandchild should be written once by normal layout and again by cache re-export"
        );
        assert!(
            grandchild_writes
                .iter()
                .any(|write| write.constraints.is_some()),
            "normal grandchild layout should carry constraints"
        );
        assert!(
            grandchild_writes
                .iter()
                .any(|write| write.constraints.is_none()),
            "cache re-export should write the cached grandchild layout without constraints"
        );

        tree.flush_layouts();
        let grandchild_layout = context.node(3).layout.expect("grandchild layout flushed");
        assert_close(grandchild_layout.offset.x, 0.0);
        assert_close(grandchild_layout.offset.y, 0.0);
        assert_close(grandchild_layout.size.width, 10.0);
        assert_close(grandchild_layout.size.height, 20.0);
    }

    #[test]
    fn external_tree_snapshot_preserves_explicit_direction_style_flag() {
        let mut explicit_style = SLRustStyle {
            has_explicit_direction: true,
            ..SLRustStyle::default()
        };
        explicit_style.direction = direction_to_ffi(Direction::Rtl);
        let inherited_style = SLRustStyle {
            has_explicit_direction: false,
            direction: direction_to_ffi(Direction::Rtl),
            ..SLRustStyle::default()
        };
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: inherited_style,
                    children: vec![2],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: explicit_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let tree = ExternalTreeSnapshot::build(callbacks, 1).expect("snapshot builds");
        let child = tree.children(tree.root).next().expect("child exists");

        assert!(!tree.has_explicit_direction_style(tree.root));
        assert!(tree.has_explicit_direction_style(child));
        assert_eq!(tree.style(tree.root).direction, Direction::Rtl);
        assert_eq!(tree.style(child).direction, Direction::Rtl);
    }

    #[test]
    fn external_layout_with_owner_direction_reaches_unset_descendants() {
        let root_style = SLRustStyle {
            display: display_to_ffi(Display::Flex),
            flex_direction: flex_direction_to_ffi(FlexDirection::Column),
            align_items: align_items_to_ffi(AlignItems::FlexStart),
            width: length_to_ffi(Length::points(30.0)),
            height: length_to_ffi(Length::points(20.0)),
            ..SLRustStyle::default()
        };
        let inherited_container_style = SLRustStyle {
            display: display_to_ffi(Display::Flex),
            flex_direction: flex_direction_to_ffi(FlexDirection::Row),
            align_items: align_items_to_ffi(AlignItems::FlexStart),
            width: length_to_ffi(Length::points(30.0)),
            height: length_to_ffi(Length::points(10.0)),
            ..SLRustStyle::default()
        };
        let explicit_ltr_container_style = SLRustStyle {
            has_explicit_direction: true,
            direction: direction_to_ffi(Direction::Ltr),
            ..inherited_container_style
        };
        let leaf_style = SLRustStyle {
            width: length_to_ffi(Length::points(10.0)),
            height: length_to_ffi(Length::points(5.0)),
            ..SLRustStyle::default()
        };
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: root_style,
                    children: vec![2, 4],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: inherited_container_style,
                    children: vec![3],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 3,
                    style: leaf_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 4,
                    style: explicit_ltr_container_style,
                    children: vec![5],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 5,
                    style: leaf_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let constraints = SLRustConstraints {
            width: side_constraint_to_ffi(SideConstraint::definite(30.0)),
            height: side_constraint_to_ffi(SideConstraint::definite(20.0)),
        };
        let abi = current_abi_info();
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternalWithOwnerConstraintsAndDirectionChecked(
                &abi as *const SLRustAbiInfo,
                &callbacks as *const SLRustTreeCallbacks,
                1,
                constraints,
                direction_to_ffi(Direction::Rtl),
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);
        assert_close(
            context
                .node(3)
                .layout
                .expect("inherited leaf layout")
                .offset
                .x,
            20.0,
        );
        assert_close(
            context
                .node(5)
                .layout
                .expect("explicit ltr leaf layout")
                .offset
                .x,
            0.0,
        );
    }

    #[test]
    fn external_tree_snapshot_rejects_non_finite_layout_writebacks_before_flush() {
        let mut context = TestContext {
            nodes: vec![TestNode {
                id: 1,
                style: SLRustStyle::default(),
                children: Vec::new(),
                measured_size: None,
                baseline: None,
                layout: None,
                last_constraints: None,
            }],
        };
        let callbacks = callbacks(&mut context);
        let mut tree = ExternalTreeSnapshot::build(callbacks, 1).expect("snapshot builds");
        let root = tree.root;
        let layout = LayoutResult {
            offset: Point::new(f32::NAN, 0.0),
            size: Size::new(10.0, 10.0),
            ..LayoutResult::default()
        };

        tree.set_layout(root, layout);

        assert_eq!(tree.callback_error(), Err(SLRustStatus::InvalidTree));
        assert!(tree.pending_layout_writes.is_empty());
        tree.flush_layouts();
        assert!(context.node(1).layout.is_none());
    }

    #[test]
    fn external_tree_snapshot_rejects_non_finite_constraint_writebacks_before_flush() {
        let mut context = TestContext {
            nodes: vec![TestNode {
                id: 1,
                style: SLRustStyle::default(),
                children: Vec::new(),
                measured_size: None,
                baseline: None,
                layout: None,
                last_constraints: None,
            }],
        };
        let callbacks = callbacks(&mut context);
        let mut tree = ExternalTreeSnapshot::build(callbacks, 1).expect("snapshot builds");
        let root = tree.root;
        let constraints = Constraints {
            width: SideConstraint::definite(f32::NAN),
            height: SideConstraint::at_most(10.0),
        };
        let layout = LayoutResult {
            size: Size::new(10.0, 10.0),
            ..LayoutResult::default()
        };

        tree.set_layout_with_constraints(root, constraints, layout);

        assert_eq!(tree.callback_error(), Err(SLRustStatus::InvalidTree));
        assert!(tree.pending_layout_writes.is_empty());
        tree.flush_layouts();
        assert!(context.node(1).layout.is_none());
        assert!(context.node(1).last_constraints.is_none());
    }

    #[test]
    fn external_layout_reports_invalid_tree_when_measure_callback_fails() {
        let mut context = TestContext {
            nodes: vec![TestNode {
                id: 1,
                style: SLRustStyle::default(),
                children: Vec::new(),
                measured_size: None,
                baseline: None,
                layout: None,
                last_constraints: None,
            }],
        };
        let mut callbacks = callbacks(&mut context);
        callbacks.has_measure = Some(always_has_measure);
        callbacks.measure = Some(failing_measure);
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                SLRustConstraints::default(),
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::InvalidTree);
        assert!(context.node(1).layout.is_none());
    }

    #[test]
    fn external_layout_reports_invalid_tree_for_non_finite_measure_output() {
        let mut context = TestContext {
            nodes: vec![TestNode {
                id: 1,
                style: SLRustStyle::default(),
                children: Vec::new(),
                measured_size: None,
                baseline: None,
                layout: None,
                last_constraints: None,
            }],
        };
        let mut callbacks = callbacks(&mut context);
        callbacks.has_measure = Some(always_has_measure);
        callbacks.measure = Some(non_finite_measure);
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                SLRustConstraints::default(),
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::InvalidTree);
        assert!(context.node(1).layout.is_none());
    }

    #[test]
    fn external_layout_reports_invalid_tree_for_non_finite_baseline_output() {
        let mut context = TestContext {
            nodes: vec![TestNode {
                id: 1,
                style: SLRustStyle::default(),
                children: Vec::new(),
                measured_size: Some(SLRustSize {
                    width: 12.0,
                    height: 8.0,
                }),
                baseline: None,
                layout: None,
                last_constraints: None,
            }],
        };
        let mut callbacks = callbacks(&mut context);
        callbacks.baseline = Some(non_finite_baseline);
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                SLRustConstraints::default(),
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::InvalidTree);
        assert!(context.node(1).layout.is_none());
    }

    #[test]
    fn external_tree_snapshot_rejects_non_finite_baseline_content_size_before_callback() {
        let mut context = TestContext {
            nodes: vec![TestNode {
                id: 1,
                style: SLRustStyle::default(),
                children: Vec::new(),
                measured_size: None,
                baseline: Some(7.0),
                layout: None,
                last_constraints: None,
            }],
        };
        let callbacks = callbacks(&mut context);
        let tree = ExternalTreeSnapshot::build(callbacks, 1).expect("snapshot builds");
        let root = tree.root;

        assert_eq!(tree.baseline(root, Size::new(f32::NAN, 1.0)), None);
        assert_eq!(tree.callback_error(), Err(SLRustStatus::InvalidTree));
    }

    #[test]
    fn external_layout_reports_invalid_tree_for_non_finite_layout_output() {
        let mut style = SLRustStyle::default();
        let max_points = SLRustLength {
            kind: SLRustLengthKind::Points as i32,
            value: f32::MAX,
            percent: 0.0,
            has_base: false,
            has_percentage: false,
        };
        style.width = max_points;
        style.padding.left = max_points;

        let mut context = TestContext {
            nodes: vec![TestNode {
                id: 1,
                style,
                children: Vec::new(),
                measured_size: None,
                baseline: None,
                layout: None,
                last_constraints: None,
            }],
        };
        let callbacks = callbacks(&mut context);
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                SLRustConstraints::default(),
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::InvalidTree);
        assert!(context.node(1).layout.is_none());
    }

    #[test]
    fn ffi_style_from_ffi_consumes_every_public_style_field() {
        let style_from_ffi_body =
            function_body(CURRENT_SOURCE, "style_from_ffi").expect("style_from_ffi exists");
        let compact_body = without_ascii_whitespace(style_from_ffi_body);
        let missing_fields = rust_struct_fields(CURRENT_SOURCE, "SLRustStyle")
            .into_iter()
            .filter(|field| !compact_body.contains(&format!("style.{field}")))
            .collect::<Vec<_>>();

        assert!(
            missing_fields.is_empty(),
            "style_from_ffi must consume every public SLRustStyle field; missing: {}",
            missing_fields.join(", ")
        );
    }

    #[test]
    fn ffi_style_defaults_initialize_every_public_style_field() {
        let defaults_body = function_body(CURRENT_SOURCE, "style_to_ffi_defaults")
            .expect("style_to_ffi_defaults exists");
        let compact_body = without_ascii_whitespace(defaults_body);
        let missing_fields = rust_struct_fields(CURRENT_SOURCE, "SLRustStyle")
            .into_iter()
            .filter(|field| !compact_body.contains(&format!("{field}:")))
            .collect::<Vec<_>>();

        assert!(
            missing_fields.is_empty(),
            "style_to_ffi_defaults must initialize every public SLRustStyle field; missing: {}",
            missing_fields.join(", ")
        );
    }

    #[test]
    fn ffi_style_enum_values_match_rust_mapping() {
        assert_enum_mapping(
            &[
                (0, Display::None),
                (1, Display::Block),
                (2, Display::Flex),
                (3, Display::Linear),
                (4, Display::Relative),
                (5, Display::Grid),
            ],
            display_from_ffi,
            display_to_ffi,
        );
        assert_enum_mapping(
            &[
                (0, PositionType::Static),
                (1, PositionType::Relative),
                (2, PositionType::Absolute),
                (3, PositionType::Fixed),
                (4, PositionType::Sticky),
            ],
            position_from_ffi,
            position_to_ffi,
        );
        assert_enum_mapping(
            &[(0, BoxSizing::ContentBox), (1, BoxSizing::BorderBox)],
            box_sizing_from_ffi,
            box_sizing_to_ffi,
        );
        assert_enum_mapping(
            &[(0, Direction::Ltr), (1, Direction::Rtl)],
            direction_from_ffi,
            direction_to_ffi,
        );
        assert_eq!(direction_from_ffi(2), Err(SLRustStatus::InvalidStyle));
        assert_enum_mapping(
            &[
                (0, FlexDirection::Row),
                (1, FlexDirection::RowReverse),
                (2, FlexDirection::Column),
                (3, FlexDirection::ColumnReverse),
            ],
            flex_direction_from_ffi,
            flex_direction_to_ffi,
        );
        assert_enum_mapping(
            &[
                (0, LinearOrientation::Horizontal),
                (1, LinearOrientation::HorizontalReverse),
                (2, LinearOrientation::Vertical),
                (3, LinearOrientation::VerticalReverse),
                (4, LinearOrientation::Row),
                (5, LinearOrientation::RowReverse),
                (6, LinearOrientation::Column),
                (7, LinearOrientation::ColumnReverse),
            ],
            linear_orientation_from_ffi,
            linear_orientation_to_ffi,
        );
        assert_enum_mapping(
            &[
                (0, FlexWrap::NoWrap),
                (1, FlexWrap::Wrap),
                (2, FlexWrap::WrapReverse),
            ],
            flex_wrap_from_ffi,
            flex_wrap_to_ffi,
        );
        assert_enum_mapping(
            &[
                (0, JustifyContent::Stretch),
                (1, JustifyContent::FlexStart),
                (2, JustifyContent::Start),
                (3, JustifyContent::Center),
                (4, JustifyContent::FlexEnd),
                (5, JustifyContent::End),
                (6, JustifyContent::SpaceBetween),
                (7, JustifyContent::SpaceAround),
                (8, JustifyContent::SpaceEvenly),
            ],
            justify_content_from_ffi,
            justify_content_to_ffi,
        );
        assert_enum_mapping(
            &[
                (0, AlignItems::Stretch),
                (1, AlignItems::FlexStart),
                (2, AlignItems::Start),
                (3, AlignItems::Center),
                (4, AlignItems::FlexEnd),
                (5, AlignItems::End),
                (6, AlignItems::Baseline),
            ],
            align_items_from_ffi,
            align_items_to_ffi,
        );
        assert_enum_mapping(
            &[
                (0, AlignContent::FlexStart),
                (1, AlignContent::Center),
                (2, AlignContent::FlexEnd),
                (3, AlignContent::SpaceBetween),
                (4, AlignContent::SpaceAround),
                (5, AlignContent::SpaceEvenly),
                (6, AlignContent::Stretch),
                (7, AlignContent::Start),
                (8, AlignContent::End),
            ],
            align_content_from_ffi,
            align_content_to_ffi,
        );
        assert_enum_mapping(
            &[
                (0, JustifyItems::Auto),
                (1, JustifyItems::Stretch),
                (2, JustifyItems::Start),
                (3, JustifyItems::Center),
                (4, JustifyItems::End),
            ],
            justify_items_from_ffi,
            justify_items_to_ffi,
        );
        assert_enum_mapping(
            &[
                (0, GridAutoFlow::Row),
                (1, GridAutoFlow::Column),
                (2, GridAutoFlow::Dense),
                (3, GridAutoFlow::RowDense),
                (4, GridAutoFlow::ColumnDense),
            ],
            grid_auto_flow_from_ffi,
            grid_auto_flow_to_ffi,
        );
        assert_enum_mapping(
            &[
                (0, RelativeCenter::None),
                (1, RelativeCenter::Horizontal),
                (2, RelativeCenter::Vertical),
                (3, RelativeCenter::Both),
            ],
            relative_center_from_ffi,
            relative_center_to_ffi,
        );
        assert_enum_mapping(
            &[
                (0, LinearGravity::None),
                (1, LinearGravity::Top),
                (2, LinearGravity::Bottom),
                (3, LinearGravity::Left),
                (4, LinearGravity::Right),
                (5, LinearGravity::CenterVertical),
                (6, LinearGravity::CenterHorizontal),
                (7, LinearGravity::SpaceBetween),
                (8, LinearGravity::Start),
                (9, LinearGravity::End),
                (10, LinearGravity::Center),
            ],
            linear_gravity_from_ffi,
            linear_gravity_to_ffi,
        );
        assert_enum_mapping(
            &[
                (0, LinearLayoutGravity::None),
                (1, LinearLayoutGravity::Top),
                (2, LinearLayoutGravity::Bottom),
                (3, LinearLayoutGravity::Left),
                (4, LinearLayoutGravity::Right),
                (5, LinearLayoutGravity::CenterVertical),
                (6, LinearLayoutGravity::CenterHorizontal),
                (7, LinearLayoutGravity::FillVertical),
                (8, LinearLayoutGravity::FillHorizontal),
                (9, LinearLayoutGravity::Center),
                (10, LinearLayoutGravity::Stretch),
                (11, LinearLayoutGravity::Start),
                (12, LinearLayoutGravity::End),
            ],
            linear_layout_gravity_from_ffi,
            linear_layout_gravity_to_ffi,
        );
        assert_enum_mapping(
            &[
                (0, LinearCrossGravity::None),
                (1, LinearCrossGravity::Start),
                (2, LinearCrossGravity::End),
                (3, LinearCrossGravity::Center),
                (4, LinearCrossGravity::Stretch),
            ],
            linear_cross_gravity_from_ffi,
            linear_cross_gravity_to_ffi,
        );

        assert_eq!(list_component_type_from_ffi(-1), Ok(None));
        for &(raw, value) in &[
            (0, ListComponentType::Header),
            (1, ListComponentType::Footer),
            (2, ListComponentType::ListRow),
            (3, ListComponentType::Default),
        ] {
            assert_eq!(list_component_type_from_ffi(raw), Ok(Some(value)));
            assert_eq!(list_component_type_to_ffi(value), raw);
        }
        assert_eq!(
            list_component_type_from_ffi(i32::MAX),
            Err(SLRustStatus::InvalidStyle)
        );
    }

    #[test]
    fn ffi_rejects_non_finite_style_numeric_values() {
        assert!(matches!(
            length_from_ffi(SLRustLength {
                kind: SLRustLengthKind::Points as i32,
                value: f32::NAN,
                percent: 0.0,
                has_base: false,
                has_percentage: false,
            }),
            Err(SLRustStatus::InvalidStyle)
        ));
        assert!(matches!(
            length_from_ffi(SLRustLength {
                kind: SLRustLengthKind::Calc as i32,
                value: 1.0,
                percent: f32::INFINITY,
                has_base: true,
                has_percentage: true,
            }),
            Err(SLRustStatus::InvalidStyle)
        ));
        assert!(matches!(
            length_from_ffi(SLRustLength {
                kind: SLRustLengthKind::FitContent as i32,
                value: f32::NEG_INFINITY,
                percent: 0.0,
                has_base: true,
                has_percentage: false,
            }),
            Err(SLRustStatus::InvalidStyle)
        ));
        assert_invalid_style(SLRustStyle {
            width: SLRustLength {
                kind: SLRustLengthKind::Points as i32,
                value: f32::NAN,
                percent: 0.0,
                has_base: false,
                has_percentage: false,
            },
            ..SLRustStyle::default()
        });
        assert_invalid_style(SLRustStyle {
            border: SLRustRectF32 {
                left: f32::INFINITY,
                ..SLRustRectF32::default()
            },
            ..SLRustStyle::default()
        });
        assert_invalid_style(SLRustStyle {
            has_aspect_ratio: true,
            aspect_ratio: f32::NAN,
            ..SLRustStyle::default()
        });
        assert_invalid_style(SLRustStyle {
            flex_grow: f32::INFINITY,
            ..SLRustStyle::default()
        });
        assert_invalid_style(SLRustStyle {
            linear_weight: f32::NEG_INFINITY,
            ..SLRustStyle::default()
        });
    }

    #[test]
    fn ffi_rejects_invalid_grid_spans() {
        assert_invalid_style(SLRustStyle {
            grid_column_span: 0,
            ..SLRustStyle::default()
        });
        assert_invalid_style(SLRustStyle {
            grid_row_span: 0,
            ..SLRustStyle::default()
        });
    }

    #[test]
    fn ffi_length_round_trip_preserves_fit_content_base_kind() {
        let fixed = Length::fit_content(Some(BaseLength::fixed(40.0)));
        let fixed_ffi = length_to_ffi(fixed);
        assert!(fixed_ffi.has_base);
        assert!(!fixed_ffi.has_percentage);
        assert_eq!(length_from_ffi(fixed_ffi), Ok(fixed));

        let fixed_and_percent =
            Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0)));
        let fixed_and_percent_ffi = length_to_ffi(fixed_and_percent);
        assert!(fixed_and_percent_ffi.has_base);
        assert!(fixed_and_percent_ffi.has_percentage);
        assert_eq!(
            length_from_ffi(fixed_and_percent_ffi),
            Ok(fixed_and_percent)
        );

        let argumentless = Length::fit_content(None);
        let argumentless_ffi = length_to_ffi(argumentless);
        assert!(!argumentless_ffi.has_base);
        assert!(!argumentless_ffi.has_percentage);
        assert_eq!(length_from_ffi(argumentless_ffi), Ok(argumentless));

        let min_content_ffi = length_to_ffi(Length::MinContent);
        assert_eq!(min_content_ffi.kind, SLRustLengthKind::MinContent as i32);
        assert_eq!(length_from_ffi(min_content_ffi), Ok(Length::MinContent));
    }

    #[test]
    fn ffi_rejects_non_finite_constraints_and_callback_scalar_results() {
        assert!(matches!(
            side_constraint_from_ffi(SLRustSideConstraint {
                size: f32::NAN,
                mode: SLRustMeasureMode::Definite as i32,
            }),
            Err(SLRustStatus::InvalidStyle)
        ));
        assert!(matches!(
            constraints_from_ffi(SLRustConstraints {
                width: SLRustSideConstraint {
                    size: 10.0,
                    mode: SLRustMeasureMode::Definite as i32,
                },
                height: SLRustSideConstraint {
                    size: f32::INFINITY,
                    mode: SLRustMeasureMode::AtMost as i32,
                },
            }),
            Err(SLRustStatus::InvalidStyle)
        ));
        assert!(side_constraint_from_ffi(SLRustSideConstraint {
            size: f32::NAN,
            mode: SLRustMeasureMode::Indefinite as i32,
        })
        .is_ok());

        assert_eq!(
            size_from_ffi(SLRustSize {
                width: 10.0,
                height: f32::NAN,
            }),
            None
        );
        assert_eq!(
            size_from_ffi(SLRustSize {
                width: 10.0,
                height: 20.0,
            }),
            Some(Size::new(10.0, 20.0))
        );
    }

    #[test]
    fn ffi_style_enum_value_test_cases_cover_every_layout_facing_enum_variant() {
        let manifest_dir = Path::new(env!("CARGO_MANIFEST_DIR"));
        let style_source =
            fs::read_to_string(manifest_dir.join("../starlight_layout/src/style.rs"))
                .expect("style.rs should be readable");
        let style_data_source =
            fs::read_to_string(manifest_dir.join("../starlight_layout/src/style_data.rs"))
                .expect("style_data.rs should be readable");
        let ffi_source =
            fs::read_to_string(manifest_dir.join("src/lib.rs")).expect("lib.rs should be readable");
        let enum_mapping_test_body =
            function_body(&ffi_source, "ffi_style_enum_values_match_rust_mapping")
                .expect("enum mapping test should exist");

        let mut missing_variants = Vec::new();
        collect_missing_enum_mapping_cases(
            &style_source,
            STYLE_ENUMS_REQUIRING_FFI_TEST_CASES,
            enum_mapping_test_body,
            &mut missing_variants,
        );
        collect_missing_enum_mapping_cases(
            &style_data_source,
            STYLE_DATA_ENUMS_REQUIRING_FFI_TEST_CASES,
            enum_mapping_test_body,
            &mut missing_variants,
        );

        assert!(
            missing_variants.is_empty(),
            "missing explicit FFI enum mapping test cases for:\n{}",
            missing_variants.join("\n")
        );
    }

    fn collect_missing_enum_mapping_cases(
        enum_source: &str,
        enum_names: &[&str],
        coverage_source: &str,
        missing_variants: &mut Vec<String>,
    ) {
        for enum_name in enum_names {
            let variants = enum_variants(enum_source, enum_name);
            assert!(
                !variants.is_empty(),
                "expected to find variants for {enum_name}"
            );
            for variant in variants {
                let needle = format!("{enum_name}::{variant}");
                if !coverage_source.contains(&needle) {
                    missing_variants.push(needle);
                }
            }
        }
    }

    #[test]
    fn style_default_exports_layout_defaults() {
        let mut style = SLRustStyle {
            display: -99,
            ..SLRustStyle::default()
        };

        unsafe { SLRustStyleDefault(&mut style as *mut SLRustStyle) };

        assert_eq!(style.display, display_to_ffi(Display::Block));
        assert_eq!(style.position, position_to_ffi(PositionType::Static));
        assert!(!style.has_explicit_direction);
        assert_eq!(style.flex_shrink, 1.0);
        assert_eq!(style.grid_column_span, 1);
    }

    #[test]
    fn external_callback_tree_layouts_with_safe_rust_engine() {
        let root_style = SLRustStyle {
            width: length_to_ffi(Length::points(100.0)),
            ..SLRustStyle::default()
        };
        let child_style = SLRustStyle::default();
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: root_style,
                    children: vec![2],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: child_style,
                    children: Vec::new(),
                    measured_size: Some(SLRustSize {
                        width: 20.0,
                        height: 7.0,
                    }),
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let constraints = SLRustConstraints {
            width: side_constraint_to_ffi(SideConstraint::definite(100.0)),
            height: side_constraint_to_ffi(SideConstraint::indefinite()),
        };
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                constraints,
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);
        assert_eq!(out_size.width, 100.0);
        assert_eq!(out_size.height, 7.0);
        let root_layout = context.node(1).layout.expect("root layout exported");
        let child_layout = context.node(2).layout.expect("child layout exported");
        assert_eq!(root_layout.size.width, 100.0);
        assert_eq!(root_layout.size.height, 7.0);
        assert_eq!(child_layout.size.width, 100.0);
        assert_eq!(child_layout.size.height, 7.0);
        let child_constraints = context
            .node(2)
            .last_constraints
            .expect("child measure constraints recorded");
        assert_eq!(
            child_constraints.width.mode,
            SLRustMeasureMode::Definite as i32
        );
        assert_eq!(child_constraints.width.size, 100.0);
    }

    #[test]
    fn external_callback_measured_leaf_at_most_does_not_clamp_callback_result() {
        let mut context = TestContext {
            nodes: vec![TestNode {
                id: 1,
                style: SLRustStyle::default(),
                children: Vec::new(),
                measured_size: Some(SLRustSize {
                    width: 120.0,
                    height: 5.0,
                }),
                baseline: None,
                layout: None,
                last_constraints: None,
            }],
        };
        let callbacks = callbacks(&mut context);
        let constraints = SLRustConstraints {
            width: side_constraint_to_ffi(SideConstraint::at_most(40.0)),
            height: side_constraint_to_ffi(SideConstraint::indefinite()),
        };
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                constraints,
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);
        assert_close(out_size.width, 120.0);
        assert_close(out_size.height, 5.0);

        let measure_constraints = context
            .node(1)
            .last_constraints
            .expect("measure constraints recorded");
        assert_eq!(
            measure_constraints.width.mode,
            SLRustMeasureMode::AtMost as i32
        );
        assert_close(measure_constraints.width.size, 40.0);
    }

    #[test]
    fn external_callback_physical_pixels_per_layout_unit_drives_measured_rounding() {
        let mut context = TestContext {
            nodes: vec![TestNode {
                id: 1,
                style: SLRustStyle::default(),
                children: Vec::new(),
                measured_size: Some(SLRustSize {
                    width: 10.2,
                    height: 4.2,
                }),
                baseline: None,
                layout: None,
                last_constraints: None,
            }],
        };
        let mut callbacks = callbacks(&mut context);
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                SLRustConstraints::default(),
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);
        assert_close(out_size.width, 11.0);
        assert_close(out_size.height, 5.0);

        context.node_mut(1).layout = None;
        callbacks.physical_pixels_per_layout_unit = Some(two_physical_pixels_per_layout_unit);
        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                SLRustConstraints::default(),
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);
        assert_close(out_size.width, 10.5);
        assert_close(out_size.height, 4.5);
        let root_layout = context.node(1).layout.expect("root layout exported");
        assert_close(root_layout.size.width, 10.5);
        assert_close(root_layout.size.height, 4.5);
    }

    #[test]
    fn external_callback_rejects_invalid_physical_pixels_per_layout_unit() {
        let mut context = TestContext {
            nodes: vec![TestNode {
                id: 1,
                style: SLRustStyle::default(),
                children: Vec::new(),
                measured_size: None,
                baseline: None,
                layout: None,
                last_constraints: None,
            }],
        };
        let mut callbacks = callbacks(&mut context);
        callbacks.physical_pixels_per_layout_unit =
            Some(non_finite_physical_pixels_per_layout_unit);
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                SLRustConstraints::default(),
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::InvalidTree);
        assert!(context.node(1).layout.is_none());
    }

    #[test]
    fn explicit_owner_and_node_constraints_entrypoints_select_root_constraint_semantics() {
        fn measured_root_context() -> TestContext {
            TestContext {
                nodes: vec![TestNode {
                    id: 1,
                    style: SLRustStyle::default(),
                    children: Vec::new(),
                    measured_size: Some(SLRustSize {
                        width: 120.0,
                        height: 5.0,
                    }),
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                }],
            }
        }

        let constraints = SLRustConstraints {
            width: side_constraint_to_ffi(SideConstraint::definite(40.0)),
            height: side_constraint_to_ffi(SideConstraint::indefinite()),
        };
        let abi = current_abi_info();

        let mut owner_context = measured_root_context();
        let owner_callbacks = callbacks(&mut owner_context);
        let mut owner_size = SLRustSize::default();
        let owner_status = unsafe {
            SLRustLayoutExternalWithOwnerConstraintsChecked(
                &abi as *const SLRustAbiInfo,
                &owner_callbacks as *const SLRustTreeCallbacks,
                1,
                constraints,
                &mut owner_size as *mut SLRustSize,
            )
        };

        assert_eq!(owner_status, SLRustStatus::Ok);
        assert_close(owner_size.width, 120.0);
        let owner_measure_constraints = owner_context
            .node(1)
            .last_constraints
            .expect("owner constraints measure constraints recorded");
        assert_eq!(
            owner_measure_constraints.width.mode,
            SLRustMeasureMode::AtMost as i32
        );
        assert_close(owner_measure_constraints.width.size, 40.0);

        let mut node_context = measured_root_context();
        let node_callbacks = callbacks(&mut node_context);
        let mut node_size = SLRustSize::default();
        let node_status = unsafe {
            SLRustLayoutExternalWithNodeConstraintsChecked(
                &abi as *const SLRustAbiInfo,
                &node_callbacks as *const SLRustTreeCallbacks,
                1,
                constraints,
                &mut node_size as *mut SLRustSize,
            )
        };

        assert_eq!(node_status, SLRustStatus::Ok);
        assert_close(node_size.width, 40.0);
        let node_measure_constraints = node_context
            .node(1)
            .last_constraints
            .expect("node constraints measure constraints recorded");
        assert_eq!(
            node_measure_constraints.width.mode,
            SLRustMeasureMode::Definite as i32
        );
        assert_close(node_measure_constraints.width.size, 40.0);

        let mut base_context = measured_root_context();
        let base_callbacks = callbacks(&mut base_context);
        let mut base_size = SLRustSize::default();
        let base_status = unsafe {
            SLRustLayoutExternal(
                &base_callbacks as *const SLRustTreeCallbacks,
                1,
                constraints,
                &mut base_size as *mut SLRustSize,
            )
        };

        assert_eq!(base_status, SLRustStatus::Ok);
        assert_close(base_size.width, owner_size.width);
    }

    #[test]
    fn external_callback_measured_leaf_applies_max_width_before_measure() {
        let mut context = TestContext {
            nodes: vec![TestNode {
                id: 1,
                style: SLRustStyle {
                    max_width: length_to_ffi(Length::points(40.0)),
                    ..SLRustStyle::default()
                },
                children: Vec::new(),
                measured_size: Some(SLRustSize {
                    width: 100.0,
                    height: 5.0,
                }),
                baseline: None,
                layout: None,
                last_constraints: None,
            }],
        };
        let callbacks = callbacks(&mut context);
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                SLRustConstraints::default(),
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);
        assert_close(out_size.width, 40.0);
        assert_close(out_size.height, 5.0);

        let layout = context.node(1).layout.expect("root layout exported");
        let measure_constraints = context
            .node(1)
            .last_constraints
            .expect("measure constraints recorded");
        assert_close(layout.size.width, 40.0);
        assert_eq!(
            measure_constraints.width.mode,
            SLRustMeasureMode::AtMost as i32
        );
        assert_close(measure_constraints.width.size, 40.0);
    }

    #[test]
    fn external_callback_measured_leaf_applies_min_width_before_measure() {
        let mut context = TestContext {
            nodes: vec![TestNode {
                id: 1,
                style: SLRustStyle {
                    width: length_to_ffi(Length::points(20.0)),
                    min_width: length_to_ffi(Length::points(40.0)),
                    ..SLRustStyle::default()
                },
                children: Vec::new(),
                measured_size: Some(SLRustSize {
                    width: 10.0,
                    height: 5.0,
                }),
                baseline: None,
                layout: None,
                last_constraints: None,
            }],
        };
        let callbacks = callbacks(&mut context);
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                SLRustConstraints::default(),
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);
        assert_close(out_size.width, 40.0);
        assert_close(out_size.height, 5.0);

        let layout = context.node(1).layout.expect("root layout exported");
        let measure_constraints = context
            .node(1)
            .last_constraints
            .expect("measure constraints recorded");
        assert_close(layout.size.width, 40.0);
        assert_eq!(
            measure_constraints.width.mode,
            SLRustMeasureMode::Definite as i32
        );
        assert_close(measure_constraints.width.size, 40.0);
    }

    #[test]
    fn external_callback_exports_margin_padding_and_border_edges() {
        let root_style = SLRustStyle {
            width: length_to_ffi(Length::points(100.0)),
            ..SLRustStyle::default()
        };
        let child_style = SLRustStyle {
            width: length_to_ffi(Length::points(20.0)),
            height: length_to_ffi(Length::points(10.0)),
            margin: SLRustRectLength {
                left: length_to_ffi(Length::points(3.0)),
                right: length_to_ffi(Length::points(4.0)),
                top: length_to_ffi(Length::points(5.0)),
                bottom: length_to_ffi(Length::points(6.0)),
            },
            padding: SLRustRectLength {
                left: length_to_ffi(Length::points(7.0)),
                right: length_to_ffi(Length::points(8.0)),
                top: length_to_ffi(Length::points(9.0)),
                bottom: length_to_ffi(Length::points(10.0)),
            },
            border: SLRustRectF32 {
                left: 1.0,
                right: 2.0,
                top: 3.0,
                bottom: 4.0,
            },
            ..SLRustStyle::default()
        };
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: root_style,
                    children: vec![2],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: child_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let constraints = SLRustConstraints {
            width: side_constraint_to_ffi(SideConstraint::definite(100.0)),
            height: side_constraint_to_ffi(SideConstraint::indefinite()),
        };
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                constraints,
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);

        let child_layout = context.node(2).layout.expect("child layout exported");
        assert_close(child_layout.size.width, 38.0);
        assert_close(child_layout.size.height, 36.0);
        assert_close(child_layout.margin.left, 3.0);
        assert_close(child_layout.margin.right, 4.0);
        assert_close(child_layout.margin.top, 5.0);
        assert_close(child_layout.margin.bottom, 6.0);
        assert_close(child_layout.padding.left, 7.0);
        assert_close(child_layout.padding.right, 8.0);
        assert_close(child_layout.padding.top, 9.0);
        assert_close(child_layout.padding.bottom, 10.0);
        assert_close(child_layout.border.left, 1.0);
        assert_close(child_layout.border.right, 2.0);
        assert_close(child_layout.border.top, 3.0);
        assert_close(child_layout.border.bottom, 4.0);
    }

    #[test]
    fn external_callback_maps_calc_padding_and_position_edges() {
        let root_style = SLRustStyle {
            width: length_to_ffi(Length::points(120.0)),
            height: length_to_ffi(Length::points(60.0)),
            padding: SLRustRectLength {
                left: length_to_ffi(Length::calc(2.0, 10.0)),
                right: length_to_ffi(Length::calc(3.0, 5.0)),
                top: length_to_ffi(Length::calc(1.0, 5.0)),
                bottom: length_to_ffi(Length::points(0.0)),
            },
            ..SLRustStyle::default()
        };
        let flow_style = SLRustStyle {
            width: length_to_ffi(Length::points(20.0)),
            height: length_to_ffi(Length::points(10.0)),
            ..SLRustStyle::default()
        };
        let absolute_style = SLRustStyle {
            position: position_to_ffi(PositionType::Absolute),
            width: length_to_ffi(Length::points(10.0)),
            height: length_to_ffi(Length::points(8.0)),
            left: length_to_ffi(Length::calc(2.0, 10.0)),
            top: length_to_ffi(Length::calc(3.0, 5.0)),
            ..SLRustStyle::default()
        };
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: root_style,
                    children: vec![2, 3],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: flow_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 3,
                    style: absolute_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let constraints = SLRustConstraints {
            width: side_constraint_to_ffi(SideConstraint::definite(120.0)),
            height: side_constraint_to_ffi(SideConstraint::definite(60.0)),
        };
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                constraints,
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);

        let root_layout = context.node(1).layout.expect("root layout exported");
        let flow_layout = context.node(2).layout.expect("flow layout exported");
        let absolute_layout = context.node(3).layout.expect("absolute layout exported");
        assert_close(root_layout.padding.left, 14.0);
        assert_close(root_layout.padding.right, 9.0);
        assert_close(root_layout.padding.top, 7.0);
        assert_close(root_layout.padding.bottom, 0.0);
        assert_close(flow_layout.size.width, 20.0);
        assert_close(flow_layout.size.height, 10.0);
        assert_close(absolute_layout.offset.x, 16.0);
        assert_close(absolute_layout.offset.y, 6.0);
    }

    #[test]
    fn external_callback_absolute_child_is_removed_from_block_flow_and_uses_insets() {
        let root_style = SLRustStyle {
            padding: SLRustRectLength {
                left: length_to_ffi(Length::points(2.0)),
                right: length_to_ffi(Length::points(2.0)),
                top: length_to_ffi(Length::points(2.0)),
                bottom: length_to_ffi(Length::points(2.0)),
            },
            ..SLRustStyle::default()
        };
        let absolute_style = SLRustStyle {
            position: position_to_ffi(PositionType::Absolute),
            width: length_to_ffi(Length::points(20.0)),
            height: length_to_ffi(Length::points(30.0)),
            left: length_to_ffi(Length::points(7.0)),
            top: length_to_ffi(Length::points(9.0)),
            ..SLRustStyle::default()
        };
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: root_style,
                    children: vec![2, 3],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: absolute_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 3,
                    style: SLRustStyle::default(),
                    children: Vec::new(),
                    measured_size: Some(SLRustSize {
                        width: 10.0,
                        height: 5.0,
                    }),
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let constraints = SLRustConstraints {
            width: side_constraint_to_ffi(SideConstraint::definite(100.0)),
            height: side_constraint_to_ffi(SideConstraint::indefinite()),
        };
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                constraints,
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);
        assert_close(out_size.height, 9.0);

        let absolute_layout = context.node(2).layout.expect("absolute layout exported");
        let normal_layout = context.node(3).layout.expect("normal layout exported");
        assert_close(absolute_layout.size.width, 20.0);
        assert_close(absolute_layout.size.height, 30.0);
        assert_close(absolute_layout.offset.x, 7.0);
        assert_close(absolute_layout.offset.y, 9.0);
        assert_close(normal_layout.offset.y, 2.0);
    }

    #[test]
    fn external_callback_absolute_auto_width_strips_single_inset_from_measure_constraint() {
        let root_style = SLRustStyle {
            width: length_to_ffi(Length::points(100.0)),
            height: length_to_ffi(Length::points(40.0)),
            ..SLRustStyle::default()
        };
        let absolute_style = SLRustStyle {
            position: position_to_ffi(PositionType::Absolute),
            left: length_to_ffi(Length::points(10.0)),
            ..SLRustStyle::default()
        };
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: root_style,
                    children: vec![2],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: absolute_style,
                    children: Vec::new(),
                    measured_size: Some(SLRustSize {
                        width: 200.0,
                        height: 10.0,
                    }),
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let constraints = SLRustConstraints {
            width: side_constraint_to_ffi(SideConstraint::definite(100.0)),
            height: side_constraint_to_ffi(SideConstraint::definite(40.0)),
        };
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                constraints,
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);

        let absolute_layout = context.node(2).layout.expect("absolute layout exported");
        let measure_constraints = context
            .node(2)
            .last_constraints
            .expect("absolute child should have been measured");
        assert_eq!(
            measure_constraints.width.mode,
            SLRustMeasureMode::AtMost as i32
        );
        assert_close(measure_constraints.width.size, 90.0);
        assert_close(absolute_layout.size.width, 200.0);
        assert_close(absolute_layout.size.height, 10.0);
        assert_close(absolute_layout.offset.x, 10.0);
    }

    #[test]
    fn external_callback_absolute_auto_size_with_both_insets_fills_padding_box_minus_margins() {
        let root_style = SLRustStyle {
            width: length_to_ffi(Length::points(100.0)),
            height: length_to_ffi(Length::points(50.0)),
            padding: SLRustRectLength {
                left: length_to_ffi(Length::points(10.0)),
                right: length_to_ffi(Length::points(10.0)),
                top: length_to_ffi(Length::points(10.0)),
                bottom: length_to_ffi(Length::points(10.0)),
            },
            ..SLRustStyle::default()
        };
        let absolute_style = SLRustStyle {
            position: position_to_ffi(PositionType::Absolute),
            left: length_to_ffi(Length::points(10.0)),
            right: length_to_ffi(Length::points(15.0)),
            top: length_to_ffi(Length::points(4.0)),
            bottom: length_to_ffi(Length::points(6.0)),
            margin: SLRustRectLength {
                left: length_to_ffi(Length::points(2.0)),
                right: length_to_ffi(Length::points(3.0)),
                top: length_to_ffi(Length::points(1.0)),
                bottom: length_to_ffi(Length::points(2.0)),
            },
            ..SLRustStyle::default()
        };
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: root_style,
                    children: vec![2],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: absolute_style,
                    children: Vec::new(),
                    measured_size: Some(SLRustSize {
                        width: 200.0,
                        height: 200.0,
                    }),
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let constraints = SLRustConstraints {
            width: side_constraint_to_ffi(SideConstraint::definite(120.0)),
            height: side_constraint_to_ffi(SideConstraint::definite(70.0)),
        };
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                constraints,
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);

        let absolute_layout = context.node(2).layout.expect("absolute layout exported");
        let measure_constraints = context
            .node(2)
            .last_constraints
            .expect("absolute child should have been measured");
        assert_eq!(
            measure_constraints.width.mode,
            SLRustMeasureMode::Definite as i32
        );
        assert_eq!(
            measure_constraints.height.mode,
            SLRustMeasureMode::Definite as i32
        );
        assert_close(measure_constraints.width.size, 90.0);
        assert_close(measure_constraints.height.size, 57.0);
        assert_close(absolute_layout.size.width, 90.0);
        assert_close(absolute_layout.size.height, 57.0);
        assert_close(absolute_layout.offset.x, 12.0);
        assert_close(absolute_layout.offset.y, 5.0);
    }

    #[test]
    fn external_callback_absolute_auto_size_with_percent_and_calc_insets_fills_containing_block() {
        let root_style = SLRustStyle {
            width: length_to_ffi(Length::points(200.0)),
            height: length_to_ffi(Length::points(100.0)),
            ..SLRustStyle::default()
        };
        let absolute_style = SLRustStyle {
            position: position_to_ffi(PositionType::Absolute),
            left: length_to_ffi(Length::percent(10.0)),
            right: length_to_ffi(Length::calc(5.0, 20.0)),
            top: length_to_ffi(Length::calc(2.0, 10.0)),
            bottom: length_to_ffi(Length::percent(25.0)),
            margin: SLRustRectLength {
                left: length_to_ffi(Length::points(3.0)),
                right: length_to_ffi(Length::points(7.0)),
                top: length_to_ffi(Length::points(4.0)),
                bottom: length_to_ffi(Length::points(6.0)),
            },
            ..SLRustStyle::default()
        };
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: root_style,
                    children: vec![2],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: absolute_style,
                    children: Vec::new(),
                    measured_size: Some(SLRustSize {
                        width: 300.0,
                        height: 200.0,
                    }),
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let constraints = SLRustConstraints {
            width: side_constraint_to_ffi(SideConstraint::definite(200.0)),
            height: side_constraint_to_ffi(SideConstraint::definite(100.0)),
        };
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                constraints,
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);

        let absolute_layout = context.node(2).layout.expect("absolute layout exported");
        let measure_constraints = context
            .node(2)
            .last_constraints
            .expect("absolute child should have been measured");
        assert_eq!(
            measure_constraints.width.mode,
            SLRustMeasureMode::Definite as i32
        );
        assert_eq!(
            measure_constraints.height.mode,
            SLRustMeasureMode::Definite as i32
        );
        assert_close(measure_constraints.width.size, 125.0);
        assert_close(measure_constraints.height.size, 53.0);
        assert_close(absolute_layout.size.width, 125.0);
        assert_close(absolute_layout.size.height, 53.0);
        assert_close(absolute_layout.offset.x, 23.0);
        assert_close(absolute_layout.offset.y, 16.0);
    }

    #[test]
    fn external_callback_fixed_descendant_uses_root_containing_block() {
        let root_style = SLRustStyle {
            width: length_to_ffi(Length::points(100.0)),
            height: length_to_ffi(Length::points(80.0)),
            ..SLRustStyle::default()
        };
        let nested_style = SLRustStyle {
            width: length_to_ffi(Length::points(40.0)),
            height: length_to_ffi(Length::points(30.0)),
            padding: SLRustRectLength {
                left: length_to_ffi(Length::points(2.0)),
                right: length_to_ffi(Length::points(2.0)),
                top: length_to_ffi(Length::points(2.0)),
                bottom: length_to_ffi(Length::points(2.0)),
            },
            ..SLRustStyle::default()
        };
        let fixed_style = SLRustStyle {
            position: position_to_ffi(PositionType::Fixed),
            width: length_to_ffi(Length::points(20.0)),
            height: length_to_ffi(Length::points(10.0)),
            right: length_to_ffi(Length::points(5.0)),
            bottom: length_to_ffi(Length::points(7.0)),
            ..SLRustStyle::default()
        };
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: root_style,
                    children: vec![2],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: nested_style,
                    children: vec![3],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 3,
                    style: fixed_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let constraints = SLRustConstraints {
            width: side_constraint_to_ffi(SideConstraint::definite(100.0)),
            height: side_constraint_to_ffi(SideConstraint::definite(80.0)),
        };
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                constraints,
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);
        assert_close(out_size.width, 100.0);
        assert_close(out_size.height, 80.0);

        let nested_layout = context.node(2).layout.expect("nested layout exported");
        let fixed_layout = context.node(3).layout.expect("fixed layout exported");
        assert_close(nested_layout.size.width, 44.0);
        assert_close(nested_layout.size.height, 34.0);
        assert_close(fixed_layout.offset.x, 75.0);
        assert_close(fixed_layout.offset.y, 63.0);
    }

    #[test]
    fn external_callback_fixed_auto_size_with_percent_and_calc_insets_uses_root_containing_block() {
        let root_style = SLRustStyle {
            width: length_to_ffi(Length::points(200.0)),
            height: length_to_ffi(Length::points(100.0)),
            ..SLRustStyle::default()
        };
        let nested_style = SLRustStyle {
            width: length_to_ffi(Length::points(40.0)),
            height: length_to_ffi(Length::points(30.0)),
            ..SLRustStyle::default()
        };
        let fixed_style = SLRustStyle {
            position: position_to_ffi(PositionType::Fixed),
            left: length_to_ffi(Length::percent(10.0)),
            right: length_to_ffi(Length::calc(5.0, 20.0)),
            top: length_to_ffi(Length::calc(2.0, 10.0)),
            bottom: length_to_ffi(Length::percent(25.0)),
            margin: SLRustRectLength {
                left: length_to_ffi(Length::points(3.0)),
                right: length_to_ffi(Length::points(7.0)),
                top: length_to_ffi(Length::points(4.0)),
                bottom: length_to_ffi(Length::points(6.0)),
            },
            ..SLRustStyle::default()
        };
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: root_style,
                    children: vec![2],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: nested_style,
                    children: vec![3],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 3,
                    style: fixed_style,
                    children: Vec::new(),
                    measured_size: Some(SLRustSize {
                        width: 300.0,
                        height: 200.0,
                    }),
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let constraints = SLRustConstraints {
            width: side_constraint_to_ffi(SideConstraint::definite(200.0)),
            height: side_constraint_to_ffi(SideConstraint::definite(100.0)),
        };
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                constraints,
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);

        let fixed_layout = context.node(3).layout.expect("fixed layout exported");
        let measure_constraints = context
            .node(3)
            .last_constraints
            .expect("fixed child should have been measured");
        assert_eq!(
            measure_constraints.width.mode,
            SLRustMeasureMode::Definite as i32
        );
        assert_eq!(
            measure_constraints.height.mode,
            SLRustMeasureMode::Definite as i32
        );
        assert_close(measure_constraints.width.size, 125.0);
        assert_close(measure_constraints.height.size, 53.0);
        assert_close(fixed_layout.size.width, 125.0);
        assert_close(fixed_layout.size.height, 53.0);
        assert_close(fixed_layout.offset.x, 23.0);
        assert_close(fixed_layout.offset.y, 16.0);
    }

    #[test]
    fn external_callback_flex_tree_maps_style_callbacks() {
        let root_style = SLRustStyle {
            display: display_to_ffi(Display::Flex),
            width: length_to_ffi(Length::points(100.0)),
            height: length_to_ffi(Length::points(20.0)),
            align_items: align_items_to_ffi(AlignItems::FlexStart),
            ..SLRustStyle::default()
        };
        let first_child_style = SLRustStyle {
            flex_grow: 1.0,
            flex_basis: length_to_ffi(Length::points(20.0)),
            height: length_to_ffi(Length::points(10.0)),
            ..SLRustStyle::default()
        };
        let second_child_style = SLRustStyle {
            flex_grow: 3.0,
            flex_basis: length_to_ffi(Length::points(20.0)),
            height: length_to_ffi(Length::points(10.0)),
            ..SLRustStyle::default()
        };
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: root_style,
                    children: vec![2, 3],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: first_child_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 3,
                    style: second_child_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let constraints = SLRustConstraints {
            width: side_constraint_to_ffi(SideConstraint::definite(100.0)),
            height: side_constraint_to_ffi(SideConstraint::definite(20.0)),
        };
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                constraints,
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);
        assert_close(out_size.width, 100.0);
        assert_close(out_size.height, 20.0);

        let root_layout = context.node(1).layout.expect("root layout exported");
        let first_layout = context.node(2).layout.expect("first layout exported");
        let second_layout = context.node(3).layout.expect("second layout exported");
        assert_close(root_layout.size.width, 100.0);
        assert_close(root_layout.size.height, 20.0);
        assert_close(first_layout.offset.x, 0.0);
        assert_close(first_layout.offset.y, 0.0);
        assert_close(first_layout.size.width, 35.0);
        assert_close(first_layout.size.height, 10.0);
        assert_close(second_layout.offset.x, 35.0);
        assert_close(second_layout.offset.y, 0.0);
        assert_close(second_layout.size.width, 65.0);
        assert_close(second_layout.size.height, 10.0);
    }

    #[test]
    fn external_callback_flex_column_gap_accepts_full_value_lengths() {
        for (column_gap, expected_gap) in [
            (Length::MaxContent, 0.0),
            (Length::fit_content(Some(BaseLength::fixed(12.0))), 12.0),
            (Length::fr(1.0), 1.0),
        ] {
            let root_style = SLRustStyle {
                display: display_to_ffi(Display::Flex),
                width: length_to_ffi(Length::points(120.0)),
                height: length_to_ffi(Length::points(30.0)),
                column_gap: length_to_ffi(column_gap),
                align_items: align_items_to_ffi(AlignItems::FlexStart),
                justify_content: justify_content_to_ffi(JustifyContent::FlexStart),
                ..SLRustStyle::default()
            };
            let mut context = TestContext {
                nodes: vec![
                    TestNode {
                        id: 1,
                        style: root_style,
                        children: vec![2, 3],
                        measured_size: None,
                        baseline: None,
                        layout: None,
                        last_constraints: None,
                    },
                    TestNode {
                        id: 2,
                        style: SLRustStyle {
                            flex_basis: length_to_ffi(Length::points(20.0)),
                            height: length_to_ffi(Length::points(10.0)),
                            ..SLRustStyle::default()
                        },
                        children: Vec::new(),
                        measured_size: None,
                        baseline: None,
                        layout: None,
                        last_constraints: None,
                    },
                    TestNode {
                        id: 3,
                        style: SLRustStyle {
                            flex_basis: length_to_ffi(Length::points(18.0)),
                            height: length_to_ffi(Length::points(12.0)),
                            ..SLRustStyle::default()
                        },
                        children: Vec::new(),
                        measured_size: None,
                        baseline: None,
                        layout: None,
                        last_constraints: None,
                    },
                ],
            };
            let callbacks = callbacks(&mut context);
            let constraints = SLRustConstraints {
                width: side_constraint_to_ffi(SideConstraint::definite(120.0)),
                height: side_constraint_to_ffi(SideConstraint::definite(30.0)),
            };
            let mut out_size = SLRustSize::default();

            let status = unsafe {
                SLRustLayoutExternal(
                    &callbacks as *const SLRustTreeCallbacks,
                    1,
                    constraints,
                    &mut out_size as *mut SLRustSize,
                )
            };

            assert_eq!(status, SLRustStatus::Ok);
            let second_layout = context.node(3).layout.expect("second layout exported");
            assert_close(second_layout.offset.x, 20.0 + expected_gap);
        }
    }

    #[test]
    fn external_callback_flex_row_gap_accepts_full_value_lengths() {
        for (row_gap, expected_gap) in [
            (Length::MaxContent, 0.0),
            (Length::fit_content(Some(BaseLength::fixed(12.0))), 12.0),
            (Length::fr(1.0), 1.0),
        ] {
            let root_style = SLRustStyle {
                display: display_to_ffi(Display::Flex),
                width: length_to_ffi(Length::points(30.0)),
                height: length_to_ffi(Length::points(80.0)),
                flex_wrap: flex_wrap_to_ffi(FlexWrap::Wrap),
                row_gap: length_to_ffi(row_gap),
                align_items: align_items_to_ffi(AlignItems::FlexStart),
                align_content: align_content_to_ffi(AlignContent::FlexStart),
                ..SLRustStyle::default()
            };
            let child_style = SLRustStyle {
                flex_basis: length_to_ffi(Length::points(20.0)),
                height: length_to_ffi(Length::points(10.0)),
                ..SLRustStyle::default()
            };
            let mut context = TestContext {
                nodes: vec![
                    TestNode {
                        id: 1,
                        style: root_style,
                        children: vec![2, 3],
                        measured_size: None,
                        baseline: None,
                        layout: None,
                        last_constraints: None,
                    },
                    TestNode {
                        id: 2,
                        style: child_style,
                        children: Vec::new(),
                        measured_size: None,
                        baseline: None,
                        layout: None,
                        last_constraints: None,
                    },
                    TestNode {
                        id: 3,
                        style: child_style,
                        children: Vec::new(),
                        measured_size: None,
                        baseline: None,
                        layout: None,
                        last_constraints: None,
                    },
                ],
            };
            let callbacks = callbacks(&mut context);
            let constraints = SLRustConstraints {
                width: side_constraint_to_ffi(SideConstraint::definite(30.0)),
                height: side_constraint_to_ffi(SideConstraint::definite(80.0)),
            };
            let mut out_size = SLRustSize::default();

            let status = unsafe {
                SLRustLayoutExternal(
                    &callbacks as *const SLRustTreeCallbacks,
                    1,
                    constraints,
                    &mut out_size as *mut SLRustSize,
                )
            };

            assert_eq!(status, SLRustStatus::Ok);
            let second_layout = context.node(3).layout.expect("second layout exported");
            assert_close(second_layout.offset.y, 10.0 + expected_gap);
        }
    }

    #[test]
    fn external_callback_flex_edge_lengths_accept_full_value_lengths() {
        for (edge_length, expected) in [
            (Length::MaxContent, 0.0),
            (Length::fit_content(Some(BaseLength::fixed(4.0))), 4.0),
            (Length::fr(1.0), 1.0),
        ] {
            let root_style = SLRustStyle {
                display: display_to_ffi(Display::Flex),
                width: length_to_ffi(Length::points(80.0)),
                height: length_to_ffi(Length::points(20.0)),
                align_items: align_items_to_ffi(AlignItems::FlexStart),
                ..SLRustStyle::default()
            };
            let child_style = SLRustStyle {
                position: position_to_ffi(PositionType::Relative),
                left: length_to_ffi(edge_length),
                margin: SLRustRectLength {
                    left: length_to_ffi(edge_length),
                    right: length_to_ffi(Length::ZERO),
                    top: length_to_ffi(Length::ZERO),
                    bottom: length_to_ffi(Length::ZERO),
                },
                padding: SLRustRectLength {
                    left: length_to_ffi(edge_length),
                    right: length_to_ffi(Length::ZERO),
                    top: length_to_ffi(Length::ZERO),
                    bottom: length_to_ffi(Length::ZERO),
                },
                flex_basis: length_to_ffi(Length::points(10.0)),
                height: length_to_ffi(Length::points(6.0)),
                box_sizing: box_sizing_to_ffi(BoxSizing::ContentBox),
                ..SLRustStyle::default()
            };
            let mut context = TestContext {
                nodes: vec![
                    TestNode {
                        id: 1,
                        style: root_style,
                        children: vec![2],
                        measured_size: None,
                        baseline: None,
                        layout: None,
                        last_constraints: None,
                    },
                    TestNode {
                        id: 2,
                        style: child_style,
                        children: Vec::new(),
                        measured_size: None,
                        baseline: None,
                        layout: None,
                        last_constraints: None,
                    },
                ],
            };
            let callbacks = callbacks(&mut context);
            let constraints = SLRustConstraints {
                width: side_constraint_to_ffi(SideConstraint::definite(80.0)),
                height: side_constraint_to_ffi(SideConstraint::definite(20.0)),
            };
            let mut out_size = SLRustSize::default();

            let status = unsafe {
                SLRustLayoutExternal(
                    &callbacks as *const SLRustTreeCallbacks,
                    1,
                    constraints,
                    &mut out_size as *mut SLRustSize,
                )
            };

            assert_eq!(status, SLRustStatus::Ok);
            let child_layout = context.node(2).layout.expect("child layout exported");
            assert_close(child_layout.padding.left, expected);
            assert_close(child_layout.margin.left, expected);
            assert_close(child_layout.offset.x, expected * 2.0);
        }
    }

    #[test]
    fn external_callback_display_none_child_is_zero_and_skipped_by_flex() {
        let root_style = SLRustStyle {
            display: display_to_ffi(Display::Flex),
            width: length_to_ffi(Length::points(100.0)),
            height: length_to_ffi(Length::points(10.0)),
            align_items: align_items_to_ffi(AlignItems::FlexStart),
            ..SLRustStyle::default()
        };
        let first_child_style = SLRustStyle {
            flex_basis: length_to_ffi(Length::points(20.0)),
            height: length_to_ffi(Length::points(10.0)),
            ..SLRustStyle::default()
        };
        let hidden_child_style = SLRustStyle {
            display: display_to_ffi(Display::None),
            flex_basis: length_to_ffi(Length::points(50.0)),
            height: length_to_ffi(Length::points(10.0)),
            ..SLRustStyle::default()
        };
        let second_child_style = SLRustStyle {
            flex_basis: length_to_ffi(Length::points(20.0)),
            height: length_to_ffi(Length::points(10.0)),
            ..SLRustStyle::default()
        };
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: root_style,
                    children: vec![2, 3, 4],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: first_child_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 3,
                    style: hidden_child_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 4,
                    style: second_child_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let constraints = SLRustConstraints {
            width: side_constraint_to_ffi(SideConstraint::definite(100.0)),
            height: side_constraint_to_ffi(SideConstraint::definite(10.0)),
        };
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                constraints,
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);
        assert_close(out_size.width, 100.0);
        assert_close(out_size.height, 10.0);

        let first_layout = context.node(2).layout.expect("first layout exported");
        let hidden_layout = context.node(3).layout.expect("hidden layout exported");
        let second_layout = context.node(4).layout.expect("second layout exported");
        assert_close(first_layout.offset.x, 0.0);
        assert_close(first_layout.size.width, 20.0);
        assert_close(hidden_layout.offset.x, 0.0);
        assert_close(hidden_layout.offset.y, 0.0);
        assert_close(hidden_layout.size.width, 0.0);
        assert_close(hidden_layout.size.height, 0.0);
        assert_close(second_layout.offset.x, 20.0);
        assert_close(second_layout.size.width, 20.0);
    }

    #[test]
    fn external_callback_display_none_child_is_zero_and_skipped_by_linear_stack() {
        let root_style = SLRustStyle {
            display: display_to_ffi(Display::Linear),
            width: length_to_ffi(Length::points(100.0)),
            ..SLRustStyle::default()
        };
        let first_child_style = SLRustStyle {
            height: length_to_ffi(Length::points(10.0)),
            ..SLRustStyle::default()
        };
        let hidden_child_style = SLRustStyle {
            display: display_to_ffi(Display::None),
            width: length_to_ffi(Length::points(100.0)),
            height: length_to_ffi(Length::points(50.0)),
            ..SLRustStyle::default()
        };
        let second_child_style = SLRustStyle {
            height: length_to_ffi(Length::points(20.0)),
            ..SLRustStyle::default()
        };
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: root_style,
                    children: vec![2, 3, 4],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: first_child_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 3,
                    style: hidden_child_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 4,
                    style: second_child_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let constraints = SLRustConstraints {
            width: side_constraint_to_ffi(SideConstraint::definite(100.0)),
            height: side_constraint_to_ffi(SideConstraint::indefinite()),
        };
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                constraints,
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);
        assert_close(out_size.width, 100.0);
        assert_close(out_size.height, 30.0);

        let first_layout = context.node(2).layout.expect("first layout exported");
        let hidden_layout = context.node(3).layout.expect("hidden layout exported");
        let second_layout = context.node(4).layout.expect("second layout exported");
        assert_close(first_layout.offset.y, 0.0);
        assert_close(first_layout.size.width, 100.0);
        assert_close(first_layout.size.height, 10.0);
        assert_close(hidden_layout.offset.x, 0.0);
        assert_close(hidden_layout.offset.y, 0.0);
        assert_close(hidden_layout.size.width, 0.0);
        assert_close(hidden_layout.size.height, 0.0);
        assert_close(second_layout.offset.y, 10.0);
        assert_close(second_layout.size.width, 100.0);
        assert_close(second_layout.size.height, 20.0);
    }

    #[test]
    fn external_callback_relative_skips_display_none_duplicate_dependency_anchor() {
        let root_style = SLRustStyle {
            display: display_to_ffi(Display::Relative),
            ..SLRustStyle::default()
        };
        let visible_anchor_style = SLRustStyle {
            width: length_to_ffi(Length::points(20.0)),
            height: length_to_ffi(Length::points(10.0)),
            relative_id: 10,
            ..SLRustStyle::default()
        };
        let follower_style = SLRustStyle {
            width: length_to_ffi(Length::points(5.0)),
            height: length_to_ffi(Length::points(7.0)),
            relative_right_of: 10,
            relative_bottom_of: 10,
            ..SLRustStyle::default()
        };
        let hidden_anchor_style = SLRustStyle {
            display: display_to_ffi(Display::None),
            width: length_to_ffi(Length::points(80.0)),
            height: length_to_ffi(Length::points(40.0)),
            relative_id: 10,
            ..SLRustStyle::default()
        };
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: root_style,
                    children: vec![2, 3, 4],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: visible_anchor_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 3,
                    style: follower_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 4,
                    style: hidden_anchor_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                SLRustConstraints::default(),
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);
        assert_close(out_size.width, 25.0);
        assert_close(out_size.height, 17.0);

        let visible_layout = context.node(2).layout.expect("visible layout exported");
        let follower_layout = context.node(3).layout.expect("follower layout exported");
        let hidden_layout = context.node(4).layout.expect("hidden layout exported");
        assert_close(visible_layout.offset.x, 0.0);
        assert_close(visible_layout.offset.y, 0.0);
        assert_close(follower_layout.offset.x, 20.0);
        assert_close(follower_layout.offset.y, 10.0);
        assert_close(hidden_layout.offset.x, 0.0);
        assert_close(hidden_layout.offset.y, 0.0);
        assert_close(hidden_layout.size.width, 0.0);
        assert_close(hidden_layout.size.height, 0.0);
    }

    #[test]
    fn external_callback_sticky_percent_insets_are_exported_for_container_children() {
        let grid_columns = [length_to_ffi(Length::points(100.0))];
        let grid_rows = [length_to_ffi(Length::points(40.0))];
        let root_style = SLRustStyle {
            width: length_to_ffi(Length::points(100.0)),
            ..SLRustStyle::default()
        };
        let flex_container_style = sticky_container_style(Display::Flex);
        let linear_container_style = SLRustStyle {
            linear_orientation: linear_orientation_to_ffi(LinearOrientation::Horizontal),
            ..sticky_container_style(Display::Linear)
        };
        let grid_container_style = SLRustStyle {
            grid_template_columns: grid_columns.as_ptr(),
            grid_template_columns_len: grid_columns.len(),
            grid_template_rows: grid_rows.as_ptr(),
            grid_template_rows_len: grid_rows.len(),
            ..sticky_container_style(Display::Grid)
        };
        let relative_container_style = sticky_container_style(Display::Relative);
        let sticky_child_style = SLRustStyle {
            position: position_to_ffi(PositionType::Sticky),
            width: length_to_ffi(Length::points(20.0)),
            height: length_to_ffi(Length::points(10.0)),
            left: length_to_ffi(Length::percent(10.0)),
            top: length_to_ffi(Length::percent(25.0)),
            ..SLRustStyle::default()
        };
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: root_style,
                    children: vec![2, 4, 6, 8],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: flex_container_style,
                    children: vec![3],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 3,
                    style: sticky_child_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 4,
                    style: linear_container_style,
                    children: vec![5],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 5,
                    style: sticky_child_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 6,
                    style: grid_container_style,
                    children: vec![7],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 7,
                    style: sticky_child_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 8,
                    style: relative_container_style,
                    children: vec![9],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 9,
                    style: sticky_child_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let constraints = SLRustConstraints {
            width: side_constraint_to_ffi(SideConstraint::definite(100.0)),
            height: side_constraint_to_ffi(SideConstraint::indefinite()),
        };
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                constraints,
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);
        assert_close(out_size.width, 100.0);
        assert_close(out_size.height, 160.0);

        for node in [3, 5, 7, 9] {
            let layout = context.node(node).layout.expect("sticky layout exported");
            assert_close(layout.sticky_pos.left, 10.0);
            assert_close(layout.sticky_pos.top, 10.0);
            assert_close(layout.sticky_pos.right, -1e10);
            assert_close(layout.sticky_pos.bottom, -1e10);
        }
    }

    fn sticky_container_style(display: Display) -> SLRustStyle {
        SLRustStyle {
            display: display_to_ffi(display),
            width: length_to_ffi(Length::points(100.0)),
            height: length_to_ffi(Length::points(40.0)),
            align_items: align_items_to_ffi(AlignItems::FlexStart),
            ..SLRustStyle::default()
        }
    }

    #[test]
    fn external_callback_flex_baseline_uses_content_baseline_callback() {
        let root_style = SLRustStyle {
            display: display_to_ffi(Display::Flex),
            align_items: align_items_to_ffi(AlignItems::Baseline),
            ..SLRustStyle::default()
        };
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: root_style,
                    children: vec![2, 3],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: SLRustStyle::default(),
                    children: Vec::new(),
                    measured_size: Some(SLRustSize {
                        width: 10.0,
                        height: 20.0,
                    }),
                    baseline: Some(5.0),
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 3,
                    style: SLRustStyle::default(),
                    children: Vec::new(),
                    measured_size: Some(SLRustSize {
                        width: 10.0,
                        height: 30.0,
                    }),
                    baseline: Some(25.0),
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                SLRustConstraints::default(),
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);
        assert_close(out_size.width, 20.0);
        assert_close(out_size.height, 40.0);

        let root_layout = context.node(1).layout.expect("root layout exported");
        let first_layout = context.node(2).layout.expect("first layout exported");
        let second_layout = context.node(3).layout.expect("second layout exported");
        assert!(root_layout.has_baseline);
        assert_close(root_layout.baseline, 25.0);
        assert_close(first_layout.offset.y, 20.0);
        assert_close(second_layout.offset.y, 0.0);
    }

    #[test]
    fn external_callback_flex_uses_nested_flex_container_baseline() {
        let root_style = SLRustStyle {
            display: display_to_ffi(Display::Flex),
            align_items: align_items_to_ffi(AlignItems::Baseline),
            ..SLRustStyle::default()
        };
        let nested_style = SLRustStyle {
            display: display_to_ffi(Display::Flex),
            align_items: align_items_to_ffi(AlignItems::Baseline),
            ..SLRustStyle::default()
        };
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: root_style,
                    children: vec![2, 3],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: SLRustStyle::default(),
                    children: Vec::new(),
                    measured_size: Some(SLRustSize {
                        width: 8.0,
                        height: 30.0,
                    }),
                    baseline: Some(26.0),
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 3,
                    style: nested_style,
                    children: vec![4, 5],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 4,
                    style: SLRustStyle::default(),
                    children: Vec::new(),
                    measured_size: Some(SLRustSize {
                        width: 10.0,
                        height: 12.0,
                    }),
                    baseline: Some(4.0),
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 5,
                    style: SLRustStyle::default(),
                    children: Vec::new(),
                    measured_size: Some(SLRustSize {
                        width: 10.0,
                        height: 20.0,
                    }),
                    baseline: Some(16.0),
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                SLRustConstraints::default(),
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);
        assert_close(out_size.width, 28.0);
        assert_close(out_size.height, 34.0);

        let root_layout = context.node(1).layout.expect("root layout exported");
        let reference_layout = context.node(2).layout.expect("reference layout exported");
        let nested_layout = context.node(3).layout.expect("nested layout exported");
        let first_nested_layout = context
            .node(4)
            .layout
            .expect("first nested layout exported");
        let second_nested_layout = context
            .node(5)
            .layout
            .expect("second nested layout exported");
        assert!(root_layout.has_baseline);
        assert_close(root_layout.baseline, 26.0);
        assert!(nested_layout.has_baseline);
        assert_close(nested_layout.baseline, 16.0);
        assert_close(reference_layout.offset.y, 0.0);
        assert_close(nested_layout.offset.y, 10.0);
        assert_close(first_nested_layout.offset.y, 12.0);
        assert_close(second_nested_layout.offset.y, 0.0);
    }

    #[test]
    fn external_callback_flex_uses_nested_linear_container_baseline() {
        let root_style = SLRustStyle {
            display: display_to_ffi(Display::Flex),
            align_items: align_items_to_ffi(AlignItems::Baseline),
            ..SLRustStyle::default()
        };
        let nested_style = SLRustStyle {
            display: display_to_ffi(Display::Linear),
            linear_orientation: linear_orientation_to_ffi(LinearOrientation::Horizontal),
            ..SLRustStyle::default()
        };
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: root_style,
                    children: vec![2, 3],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: SLRustStyle::default(),
                    children: Vec::new(),
                    measured_size: Some(SLRustSize {
                        width: 10.0,
                        height: 40.0,
                    }),
                    baseline: Some(35.0),
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 3,
                    style: nested_style,
                    children: vec![4, 5],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 4,
                    style: SLRustStyle::default(),
                    children: Vec::new(),
                    measured_size: Some(SLRustSize {
                        width: 10.0,
                        height: 20.0,
                    }),
                    baseline: Some(5.0),
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 5,
                    style: SLRustStyle::default(),
                    children: Vec::new(),
                    measured_size: Some(SLRustSize {
                        width: 10.0,
                        height: 30.0,
                    }),
                    baseline: Some(25.0),
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                SLRustConstraints::default(),
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);
        assert_close(out_size.width, 30.0);
        assert_close(out_size.height, 40.0);

        let root_layout = context.node(1).layout.expect("root layout exported");
        let reference_layout = context.node(2).layout.expect("reference layout exported");
        let nested_layout = context.node(3).layout.expect("nested layout exported");
        assert!(root_layout.has_baseline);
        assert_close(root_layout.baseline, 35.0);
        assert!(nested_layout.has_baseline);
        assert_close(nested_layout.baseline, 25.0);
        assert_close(reference_layout.offset.y, 0.0);
        assert_close(nested_layout.offset.y, 10.0);
    }

    #[test]
    fn external_callback_flex_uses_nested_grid_container_baseline() {
        let columns = [length_to_ffi(Length::points(20.0))];
        let rows = [length_to_ffi(Length::points(10.0))];
        let root_style = SLRustStyle {
            display: display_to_ffi(Display::Flex),
            align_items: align_items_to_ffi(AlignItems::Baseline),
            ..SLRustStyle::default()
        };
        let nested_style = SLRustStyle {
            display: display_to_ffi(Display::Grid),
            width: length_to_ffi(Length::points(20.0)),
            height: length_to_ffi(Length::points(10.0)),
            grid_template_columns: columns.as_ptr(),
            grid_template_columns_len: columns.len(),
            grid_template_rows: rows.as_ptr(),
            grid_template_rows_len: rows.len(),
            align_items: align_items_to_ffi(AlignItems::Baseline),
            ..SLRustStyle::default()
        };
        let nested_child_style = SLRustStyle {
            width: length_to_ffi(Length::MaxContent),
            height: length_to_ffi(Length::MaxContent),
            ..SLRustStyle::default()
        };
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: root_style,
                    children: vec![2, 3],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: SLRustStyle::default(),
                    children: Vec::new(),
                    measured_size: Some(SLRustSize {
                        width: 10.0,
                        height: 30.0,
                    }),
                    baseline: Some(25.0),
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 3,
                    style: nested_style,
                    children: vec![4],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 4,
                    style: nested_child_style,
                    children: Vec::new(),
                    measured_size: Some(SLRustSize {
                        width: 8.0,
                        height: 6.0,
                    }),
                    baseline: Some(4.0),
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                SLRustConstraints::default(),
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);
        assert_close(out_size.width, 30.0);
        assert_close(out_size.height, 31.0);

        let root_layout = context.node(1).layout.expect("root layout exported");
        let reference_layout = context.node(2).layout.expect("reference layout exported");
        let nested_layout = context.node(3).layout.expect("nested layout exported");
        let nested_child_layout = context
            .node(4)
            .layout
            .expect("nested child layout exported");
        assert!(root_layout.has_baseline);
        assert_close(root_layout.baseline, 25.0);
        assert!(nested_layout.has_baseline);
        assert_close(nested_layout.baseline, 4.0);
        assert_close(reference_layout.offset.y, 0.0);
        assert_close(nested_layout.offset.y, 21.0);
        assert_close(nested_child_layout.offset.y, 0.0);
    }

    #[test]
    fn external_callback_baseline_receives_measured_content_size() {
        let root_style = SLRustStyle {
            display: display_to_ffi(Display::Flex),
            align_items: align_items_to_ffi(AlignItems::Baseline),
            ..SLRustStyle::default()
        };
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: root_style,
                    children: vec![2, 3],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: SLRustStyle::default(),
                    children: Vec::new(),
                    measured_size: Some(SLRustSize {
                        width: 10.0,
                        height: 20.0,
                    }),
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 3,
                    style: SLRustStyle::default(),
                    children: Vec::new(),
                    measured_size: Some(SLRustSize {
                        width: 10.0,
                        height: 30.0,
                    }),
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let mut callbacks = callbacks(&mut context);
        callbacks.baseline = Some(baseline_from_content_height);
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                SLRustConstraints::default(),
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);
        assert_close(out_size.width, 20.0);
        assert_close(out_size.height, 30.0);

        let root_layout = context.node(1).layout.expect("root layout exported");
        let first_layout = context.node(2).layout.expect("first layout exported");
        let second_layout = context.node(3).layout.expect("second layout exported");
        assert!(root_layout.has_baseline);
        assert_close(root_layout.baseline, 25.0);
        assert_close(first_layout.offset.y, 10.0);
        assert_close(second_layout.offset.y, 0.0);
    }

    #[test]
    fn external_callback_linear_baseline_keeps_unresolved_start_auto_margin() {
        let root_style = SLRustStyle {
            display: display_to_ffi(Display::Linear),
            linear_orientation: linear_orientation_to_ffi(LinearOrientation::Horizontal),
            align_items: align_items_to_ffi(AlignItems::FlexStart),
            width: length_to_ffi(Length::points(100.0)),
            height: length_to_ffi(Length::points(100.0)),
            ..SLRustStyle::default()
        };
        let child_style = SLRustStyle {
            margin: SLRustRectLength {
                left: length_to_ffi(Length::ZERO),
                right: length_to_ffi(Length::ZERO),
                top: length_to_ffi(Length::Auto),
                bottom: length_to_ffi(Length::ZERO),
            },
            ..SLRustStyle::default()
        };
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: root_style,
                    children: vec![2],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: child_style,
                    children: Vec::new(),
                    measured_size: Some(SLRustSize {
                        width: 20.0,
                        height: 10.0,
                    }),
                    baseline: Some(4.0),
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let constraints = SLRustConstraints {
            width: side_constraint_to_ffi(SideConstraint::definite(100.0)),
            height: side_constraint_to_ffi(SideConstraint::definite(100.0)),
        };
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                constraints,
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);
        let root_layout = context.node(1).layout.expect("root layout exported");
        let child_layout = context.node(2).layout.expect("child layout exported");
        assert!(root_layout.has_baseline);
        assert_close(root_layout.baseline, 4.0);
        assert_close(child_layout.offset.y, 90.0);
        assert_close(child_layout.margin.top, 90.0);
        assert_close(child_layout.margin.bottom, 0.0);
    }

    #[test]
    fn external_callback_linear_baseline_uses_gravity_before_paired_auto_margins_resolve() {
        let root_style = SLRustStyle {
            display: display_to_ffi(Display::Linear),
            linear_orientation: linear_orientation_to_ffi(LinearOrientation::Horizontal),
            linear_cross_gravity: linear_cross_gravity_to_ffi(LinearCrossGravity::End),
            width: length_to_ffi(Length::points(100.0)),
            height: length_to_ffi(Length::points(100.0)),
            ..SLRustStyle::default()
        };
        let child_style = SLRustStyle {
            margin: SLRustRectLength {
                left: length_to_ffi(Length::ZERO),
                right: length_to_ffi(Length::ZERO),
                top: length_to_ffi(Length::Auto),
                bottom: length_to_ffi(Length::Auto),
            },
            ..SLRustStyle::default()
        };
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: root_style,
                    children: vec![2],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: child_style,
                    children: Vec::new(),
                    measured_size: Some(SLRustSize {
                        width: 20.0,
                        height: 10.0,
                    }),
                    baseline: Some(4.0),
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let constraints = SLRustConstraints {
            width: side_constraint_to_ffi(SideConstraint::definite(100.0)),
            height: side_constraint_to_ffi(SideConstraint::definite(100.0)),
        };
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                constraints,
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);
        let root_layout = context.node(1).layout.expect("root layout exported");
        let child_layout = context.node(2).layout.expect("child layout exported");
        assert!(root_layout.has_baseline);
        assert_close(root_layout.baseline, 94.0);
        assert_close(child_layout.offset.y, 45.0);
        assert_close(child_layout.margin.top, 45.0);
        assert_close(child_layout.margin.bottom, 45.0);
    }

    #[test]
    fn external_callback_grid_maps_track_vectors() {
        let columns = [
            length_to_ffi(Length::points(30.0)),
            length_to_ffi(Length::points(40.0)),
        ];
        let rows = [length_to_ffi(Length::points(10.0))];
        let root_style = SLRustStyle {
            display: display_to_ffi(Display::Grid),
            width: length_to_ffi(Length::points(100.0)),
            height: length_to_ffi(Length::points(20.0)),
            grid_template_columns: columns.as_ptr(),
            grid_template_columns_len: columns.len(),
            grid_template_rows: rows.as_ptr(),
            grid_template_rows_len: rows.len(),
            ..SLRustStyle::default()
        };
        let child_style = SLRustStyle::default();
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: root_style,
                    children: vec![2, 3],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: child_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 3,
                    style: child_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let constraints = SLRustConstraints {
            width: side_constraint_to_ffi(SideConstraint::definite(100.0)),
            height: side_constraint_to_ffi(SideConstraint::definite(20.0)),
        };
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                constraints,
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);
        assert_close(out_size.width, 100.0);
        assert_close(out_size.height, 20.0);

        let first_layout = context.node(2).layout.expect("first layout exported");
        let second_layout = context.node(3).layout.expect("second layout exported");
        assert_close(first_layout.offset.x, 0.0);
        assert_close(first_layout.offset.y, 0.0);
        assert_close(first_layout.size.width, 30.0);
        assert_close(first_layout.size.height, 10.0);
        assert_close(second_layout.offset.x, 30.0);
        assert_close(second_layout.offset.y, 0.0);
        assert_close(second_layout.size.width, 40.0);
        assert_close(second_layout.size.height, 10.0);
    }

    #[test]
    fn external_callback_grid_fit_content_track_caps_fixed_item_growth() {
        let columns = [length_to_ffi(Length::fit_content(Some(BaseLength::fixed(
            40.0,
        ))))];
        let rows = [length_to_ffi(Length::points(10.0))];
        let root_style = SLRustStyle {
            display: display_to_ffi(Display::Grid),
            grid_template_columns: columns.as_ptr(),
            grid_template_columns_len: columns.len(),
            grid_template_rows: rows.as_ptr(),
            grid_template_rows_len: rows.len(),
            ..SLRustStyle::default()
        };
        let child_style = SLRustStyle {
            width: length_to_ffi(Length::points(70.0)),
            height: length_to_ffi(Length::points(10.0)),
            ..SLRustStyle::default()
        };
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: root_style,
                    children: vec![2],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: child_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let constraints = SLRustConstraints {
            width: side_constraint_to_ffi(SideConstraint::indefinite()),
            height: side_constraint_to_ffi(SideConstraint::indefinite()),
        };
        let abi = current_abi_info();
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternalWithOwnerConstraintsChecked(
                &abi as *const SLRustAbiInfo,
                &callbacks as *const SLRustTreeCallbacks,
                1,
                constraints,
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);
        assert_close(out_size.width, 40.0);
        assert_close(out_size.height, 10.0);

        let child_layout = context.node(2).layout.expect("child layout exported");
        assert_close(child_layout.offset.x, 0.0);
        assert_close(child_layout.offset.y, 0.0);
        assert_close(child_layout.size.width, 70.0);
        assert_close(child_layout.size.height, 10.0);
    }

    #[test]
    fn external_callback_grid_fit_content_percent_and_calc_tracks_keep_percentage_base() {
        fn measured_track_width(base: BaseLength) -> f32 {
            let column = length_to_ffi(Length::fit_content(Some(base)));
            assert!(column.has_base);
            assert!(column.has_percentage);
            let columns = [column];
            let rows = [length_to_ffi(Length::points(10.0))];
            let root_style = SLRustStyle {
                display: display_to_ffi(Display::Grid),
                width: length_to_ffi(Length::points(120.0)),
                height: length_to_ffi(Length::points(10.0)),
                grid_template_columns: columns.as_ptr(),
                grid_template_columns_len: columns.len(),
                grid_template_rows: rows.as_ptr(),
                grid_template_rows_len: rows.len(),
                ..SLRustStyle::default()
            };
            let mut context = TestContext {
                nodes: vec![
                    TestNode {
                        id: 1,
                        style: root_style,
                        children: vec![2],
                        measured_size: None,
                        baseline: None,
                        layout: None,
                        last_constraints: None,
                    },
                    TestNode {
                        id: 2,
                        style: SLRustStyle::default(),
                        children: Vec::new(),
                        measured_size: Some(SLRustSize {
                            width: 90.0,
                            height: 10.0,
                        }),
                        baseline: None,
                        layout: None,
                        last_constraints: None,
                    },
                ],
            };
            let callbacks = callbacks(&mut context);
            let constraints = SLRustConstraints {
                width: side_constraint_to_ffi(SideConstraint::definite(120.0)),
                height: side_constraint_to_ffi(SideConstraint::definite(10.0)),
            };
            let abi = current_abi_info();
            let mut out_size = SLRustSize::default();

            let status = unsafe {
                SLRustLayoutExternalWithOwnerConstraintsChecked(
                    &abi as *const SLRustAbiInfo,
                    &callbacks as *const SLRustTreeCallbacks,
                    1,
                    constraints,
                    &mut out_size as *mut SLRustSize,
                )
            };

            assert_eq!(status, SLRustStatus::Ok);
            assert_close(out_size.width, 120.0);
            assert_close(out_size.height, 10.0);

            let child_layout = context.node(2).layout.expect("child layout");
            assert_close(child_layout.offset.x, 0.0);
            assert_close(child_layout.size.height, 10.0);
            child_layout.size.width
        }

        assert_close(
            measured_track_width(BaseLength::fixed_and_percent(0.0, 50.0)),
            60.0,
        );
        assert_close(
            measured_track_width(BaseLength::fixed_and_percent(10.0, 50.0)),
            70.0,
        );
    }

    #[test]
    fn external_callback_grid_fit_content_percent_and_calc_row_tracks_keep_percentage_base() {
        fn measured_track_height(base: BaseLength) -> f32 {
            let row = length_to_ffi(Length::fit_content(Some(base)));
            assert!(row.has_base);
            assert!(row.has_percentage);
            let columns = [length_to_ffi(Length::points(10.0))];
            let rows = [row];
            let root_style = SLRustStyle {
                display: display_to_ffi(Display::Grid),
                width: length_to_ffi(Length::points(10.0)),
                height: length_to_ffi(Length::points(120.0)),
                grid_template_columns: columns.as_ptr(),
                grid_template_columns_len: columns.len(),
                grid_template_rows: rows.as_ptr(),
                grid_template_rows_len: rows.len(),
                ..SLRustStyle::default()
            };
            let mut context = TestContext {
                nodes: vec![
                    TestNode {
                        id: 1,
                        style: root_style,
                        children: vec![2],
                        measured_size: None,
                        baseline: None,
                        layout: None,
                        last_constraints: None,
                    },
                    TestNode {
                        id: 2,
                        style: SLRustStyle::default(),
                        children: Vec::new(),
                        measured_size: Some(SLRustSize {
                            width: 10.0,
                            height: 90.0,
                        }),
                        baseline: None,
                        layout: None,
                        last_constraints: None,
                    },
                ],
            };
            let callbacks = callbacks(&mut context);
            let constraints = SLRustConstraints {
                width: side_constraint_to_ffi(SideConstraint::definite(10.0)),
                height: side_constraint_to_ffi(SideConstraint::definite(120.0)),
            };
            let abi = current_abi_info();
            let mut out_size = SLRustSize::default();

            let status = unsafe {
                SLRustLayoutExternalWithOwnerConstraintsChecked(
                    &abi as *const SLRustAbiInfo,
                    &callbacks as *const SLRustTreeCallbacks,
                    1,
                    constraints,
                    &mut out_size as *mut SLRustSize,
                )
            };

            assert_eq!(status, SLRustStatus::Ok);
            assert_close(out_size.width, 10.0);
            assert_close(out_size.height, 120.0);

            let child_layout = context.node(2).layout.expect("child layout");
            assert_close(child_layout.offset.y, 0.0);
            assert_close(child_layout.size.width, 10.0);
            child_layout.size.height
        }

        assert_close(
            measured_track_height(BaseLength::fixed_and_percent(0.0, 50.0)),
            60.0,
        );
        assert_close(
            measured_track_height(BaseLength::fixed_and_percent(10.0, 50.0)),
            70.0,
        );
    }

    #[test]
    fn external_callback_grid_minmax_fit_content_percent_and_calc_max_tracks_keep_percentage_base()
    {
        fn following_offset_with_max(base: BaseLength) -> f32 {
            let columns = [
                length_to_ffi(Length::points(20.0)),
                length_to_ffi(Length::points(10.0)),
            ];
            let column_max = length_to_ffi(Length::fit_content(Some(base)));
            assert!(column_max.has_base);
            assert!(column_max.has_percentage);
            let columns_max = [column_max];
            let rows = [length_to_ffi(Length::points(10.0))];
            let root_style = SLRustStyle {
                display: display_to_ffi(Display::Grid),
                width: length_to_ffi(Length::points(120.0)),
                height: length_to_ffi(Length::points(10.0)),
                grid_template_columns: columns.as_ptr(),
                grid_template_columns_len: columns.len(),
                grid_template_columns_max: columns_max.as_ptr(),
                grid_template_columns_max_len: columns_max.len(),
                grid_template_rows: rows.as_ptr(),
                grid_template_rows_len: rows.len(),
                ..SLRustStyle::default()
            };
            let intrinsic_style = SLRustStyle {
                grid_column_start: 1,
                ..SLRustStyle::default()
            };
            let following_style = SLRustStyle {
                grid_column_start: 2,
                ..SLRustStyle::default()
            };
            let mut context = TestContext {
                nodes: vec![
                    TestNode {
                        id: 1,
                        style: root_style,
                        children: vec![2, 3],
                        measured_size: None,
                        baseline: None,
                        layout: None,
                        last_constraints: None,
                    },
                    TestNode {
                        id: 2,
                        style: intrinsic_style,
                        children: Vec::new(),
                        measured_size: Some(SLRustSize {
                            width: 90.0,
                            height: 10.0,
                        }),
                        baseline: None,
                        layout: None,
                        last_constraints: None,
                    },
                    TestNode {
                        id: 3,
                        style: following_style,
                        children: Vec::new(),
                        measured_size: None,
                        baseline: None,
                        layout: None,
                        last_constraints: None,
                    },
                ],
            };
            let callbacks = callbacks(&mut context);
            let constraints = SLRustConstraints {
                width: side_constraint_to_ffi(SideConstraint::definite(120.0)),
                height: side_constraint_to_ffi(SideConstraint::definite(10.0)),
            };
            let abi = current_abi_info();
            let mut out_size = SLRustSize::default();

            let status = unsafe {
                SLRustLayoutExternalWithOwnerConstraintsChecked(
                    &abi as *const SLRustAbiInfo,
                    &callbacks as *const SLRustTreeCallbacks,
                    1,
                    constraints,
                    &mut out_size as *mut SLRustSize,
                )
            };

            assert_eq!(status, SLRustStatus::Ok);
            assert_close(out_size.width, 120.0);
            assert_close(out_size.height, 10.0);

            let intrinsic = context.node(2).layout.expect("intrinsic layout");
            let following = context.node(3).layout.expect("following layout");
            assert_close(intrinsic.offset.x, 0.0);
            assert_close(intrinsic.size.height, 10.0);
            assert_close(following.size.width, 10.0);
            following.offset.x
        }

        assert_close(
            following_offset_with_max(BaseLength::fixed_and_percent(0.0, 50.0)),
            60.0,
        );
        assert_close(
            following_offset_with_max(BaseLength::fixed_and_percent(10.0, 50.0)),
            70.0,
        );
    }

    #[test]
    fn external_callback_grid_minmax_row_fit_content_percent_and_calc_max_tracks_keep_percentage_base(
    ) {
        fn following_offset_with_row_max(base: BaseLength) -> f32 {
            let columns = [length_to_ffi(Length::points(10.0))];
            let rows = [
                length_to_ffi(Length::points(20.0)),
                length_to_ffi(Length::points(10.0)),
            ];
            let row_max = length_to_ffi(Length::fit_content(Some(base)));
            assert!(row_max.has_base);
            assert!(row_max.has_percentage);
            let rows_max = [row_max];
            let root_style = SLRustStyle {
                display: display_to_ffi(Display::Grid),
                width: length_to_ffi(Length::points(10.0)),
                height: length_to_ffi(Length::points(120.0)),
                grid_template_columns: columns.as_ptr(),
                grid_template_columns_len: columns.len(),
                grid_template_rows: rows.as_ptr(),
                grid_template_rows_len: rows.len(),
                grid_template_rows_max: rows_max.as_ptr(),
                grid_template_rows_max_len: rows_max.len(),
                ..SLRustStyle::default()
            };
            let intrinsic_style = SLRustStyle {
                grid_column_start: 1,
                grid_row_start: 1,
                ..SLRustStyle::default()
            };
            let following_style = SLRustStyle {
                grid_column_start: 1,
                grid_row_start: 2,
                ..SLRustStyle::default()
            };
            let mut context = TestContext {
                nodes: vec![
                    TestNode {
                        id: 1,
                        style: root_style,
                        children: vec![2, 3],
                        measured_size: None,
                        baseline: None,
                        layout: None,
                        last_constraints: None,
                    },
                    TestNode {
                        id: 2,
                        style: intrinsic_style,
                        children: Vec::new(),
                        measured_size: Some(SLRustSize {
                            width: 10.0,
                            height: 90.0,
                        }),
                        baseline: None,
                        layout: None,
                        last_constraints: None,
                    },
                    TestNode {
                        id: 3,
                        style: following_style,
                        children: Vec::new(),
                        measured_size: None,
                        baseline: None,
                        layout: None,
                        last_constraints: None,
                    },
                ],
            };
            let callbacks = callbacks(&mut context);
            let constraints = SLRustConstraints {
                width: side_constraint_to_ffi(SideConstraint::definite(10.0)),
                height: side_constraint_to_ffi(SideConstraint::definite(120.0)),
            };
            let abi = current_abi_info();
            let mut out_size = SLRustSize::default();

            let status = unsafe {
                SLRustLayoutExternalWithOwnerConstraintsChecked(
                    &abi as *const SLRustAbiInfo,
                    &callbacks as *const SLRustTreeCallbacks,
                    1,
                    constraints,
                    &mut out_size as *mut SLRustSize,
                )
            };

            assert_eq!(status, SLRustStatus::Ok);
            assert_close(out_size.width, 10.0);
            assert_close(out_size.height, 120.0);

            let intrinsic = context.node(2).layout.expect("intrinsic layout");
            let following = context.node(3).layout.expect("following layout");
            assert_close(intrinsic.offset.y, 0.0);
            assert_close(intrinsic.size.width, 10.0);
            assert_close(following.size.width, 10.0);
            following.offset.y
        }

        assert_close(
            following_offset_with_row_max(BaseLength::fixed_and_percent(0.0, 50.0)),
            60.0,
        );
        assert_close(
            following_offset_with_row_max(BaseLength::fixed_and_percent(10.0, 50.0)),
            70.0,
        );
    }

    #[test]
    fn external_callback_grid_auto_fit_content_percent_and_calc_max_tracks_keep_percentage_base() {
        fn following_offset_with_auto_max(base: BaseLength, following_max: f32) -> f32 {
            let auto_columns = [
                length_to_ffi(Length::points(20.0)),
                length_to_ffi(Length::points(10.0)),
            ];
            let auto_column_max = length_to_ffi(Length::fit_content(Some(base)));
            assert!(auto_column_max.has_base);
            assert!(auto_column_max.has_percentage);
            let auto_columns_max = [
                auto_column_max,
                length_to_ffi(Length::points(following_max)),
            ];
            let auto_rows = [length_to_ffi(Length::points(10.0))];
            let root_style = SLRustStyle {
                display: display_to_ffi(Display::Grid),
                width: length_to_ffi(Length::points(120.0)),
                height: length_to_ffi(Length::points(10.0)),
                grid_auto_columns: auto_columns.as_ptr(),
                grid_auto_columns_len: auto_columns.len(),
                grid_auto_columns_max: auto_columns_max.as_ptr(),
                grid_auto_columns_max_len: auto_columns_max.len(),
                grid_auto_rows: auto_rows.as_ptr(),
                grid_auto_rows_len: auto_rows.len(),
                ..SLRustStyle::default()
            };
            let intrinsic_style = SLRustStyle {
                grid_column_start: 1,
                grid_row_start: 1,
                ..SLRustStyle::default()
            };
            let following_style = SLRustStyle {
                grid_column_start: 2,
                grid_row_start: 1,
                ..SLRustStyle::default()
            };
            let mut context = TestContext {
                nodes: vec![
                    TestNode {
                        id: 1,
                        style: root_style,
                        children: vec![2, 3],
                        measured_size: None,
                        baseline: None,
                        layout: None,
                        last_constraints: None,
                    },
                    TestNode {
                        id: 2,
                        style: intrinsic_style,
                        children: Vec::new(),
                        measured_size: Some(SLRustSize {
                            width: 90.0,
                            height: 10.0,
                        }),
                        baseline: None,
                        layout: None,
                        last_constraints: None,
                    },
                    TestNode {
                        id: 3,
                        style: following_style,
                        children: Vec::new(),
                        measured_size: None,
                        baseline: None,
                        layout: None,
                        last_constraints: None,
                    },
                ],
            };
            let callbacks = callbacks(&mut context);
            let constraints = SLRustConstraints {
                width: side_constraint_to_ffi(SideConstraint::definite(120.0)),
                height: side_constraint_to_ffi(SideConstraint::definite(10.0)),
            };
            let abi = current_abi_info();
            let mut out_size = SLRustSize::default();

            let status = unsafe {
                SLRustLayoutExternalWithOwnerConstraintsChecked(
                    &abi as *const SLRustAbiInfo,
                    &callbacks as *const SLRustTreeCallbacks,
                    1,
                    constraints,
                    &mut out_size as *mut SLRustSize,
                )
            };

            assert_eq!(status, SLRustStatus::Ok);
            assert_close(out_size.width, 120.0);
            assert_close(out_size.height, 10.0);

            let intrinsic = context.node(2).layout.expect("intrinsic layout");
            let following = context.node(3).layout.expect("following layout");
            assert_close(intrinsic.offset.x, 0.0);
            assert_close(intrinsic.size.height, 10.0);
            assert_close(following.size.height, 10.0);
            following.offset.x
        }

        assert_close(
            following_offset_with_auto_max(BaseLength::fixed_and_percent(0.0, 50.0), 60.0),
            60.0,
        );
        assert_close(
            following_offset_with_auto_max(BaseLength::fixed_and_percent(10.0, 50.0), 50.0),
            70.0,
        );
    }

    #[test]
    fn external_callback_grid_auto_row_fit_content_percent_and_calc_max_tracks_keep_percentage_base(
    ) {
        fn following_offset_with_auto_row_max(base: BaseLength, following_max: f32) -> f32 {
            let auto_columns = [length_to_ffi(Length::points(10.0))];
            let auto_rows = [
                length_to_ffi(Length::points(20.0)),
                length_to_ffi(Length::points(10.0)),
            ];
            let auto_row_max = length_to_ffi(Length::fit_content(Some(base)));
            assert!(auto_row_max.has_base);
            assert!(auto_row_max.has_percentage);
            let auto_rows_max = [auto_row_max, length_to_ffi(Length::points(following_max))];
            let root_style = SLRustStyle {
                display: display_to_ffi(Display::Grid),
                width: length_to_ffi(Length::points(10.0)),
                height: length_to_ffi(Length::points(120.0)),
                grid_auto_columns: auto_columns.as_ptr(),
                grid_auto_columns_len: auto_columns.len(),
                grid_auto_rows: auto_rows.as_ptr(),
                grid_auto_rows_len: auto_rows.len(),
                grid_auto_rows_max: auto_rows_max.as_ptr(),
                grid_auto_rows_max_len: auto_rows_max.len(),
                ..SLRustStyle::default()
            };
            let intrinsic_style = SLRustStyle {
                grid_column_start: 1,
                grid_row_start: 1,
                ..SLRustStyle::default()
            };
            let following_style = SLRustStyle {
                grid_column_start: 1,
                grid_row_start: 2,
                ..SLRustStyle::default()
            };
            let mut context = TestContext {
                nodes: vec![
                    TestNode {
                        id: 1,
                        style: root_style,
                        children: vec![2, 3],
                        measured_size: None,
                        baseline: None,
                        layout: None,
                        last_constraints: None,
                    },
                    TestNode {
                        id: 2,
                        style: intrinsic_style,
                        children: Vec::new(),
                        measured_size: Some(SLRustSize {
                            width: 10.0,
                            height: 90.0,
                        }),
                        baseline: None,
                        layout: None,
                        last_constraints: None,
                    },
                    TestNode {
                        id: 3,
                        style: following_style,
                        children: Vec::new(),
                        measured_size: None,
                        baseline: None,
                        layout: None,
                        last_constraints: None,
                    },
                ],
            };
            let callbacks = callbacks(&mut context);
            let constraints = SLRustConstraints {
                width: side_constraint_to_ffi(SideConstraint::definite(10.0)),
                height: side_constraint_to_ffi(SideConstraint::definite(120.0)),
            };
            let abi = current_abi_info();
            let mut out_size = SLRustSize::default();

            let status = unsafe {
                SLRustLayoutExternalWithOwnerConstraintsChecked(
                    &abi as *const SLRustAbiInfo,
                    &callbacks as *const SLRustTreeCallbacks,
                    1,
                    constraints,
                    &mut out_size as *mut SLRustSize,
                )
            };

            assert_eq!(status, SLRustStatus::Ok);
            assert_close(out_size.width, 10.0);
            assert_close(out_size.height, 120.0);

            let intrinsic = context.node(2).layout.expect("intrinsic layout");
            let following = context.node(3).layout.expect("following layout");
            assert_close(intrinsic.offset.y, 0.0);
            assert_close(intrinsic.size.width, 10.0);
            assert_close(following.size.width, 10.0);
            following.offset.y
        }

        assert_close(
            following_offset_with_auto_row_max(BaseLength::fixed_and_percent(0.0, 50.0), 60.0),
            60.0,
        );
        assert_close(
            following_offset_with_auto_row_max(BaseLength::fixed_and_percent(10.0, 50.0), 50.0),
            70.0,
        );
    }

    #[test]
    fn external_callback_grid_auto_fit_content_fixed_max_tracks_cap_indefinite_intrinsic_growth() {
        let auto_columns = [
            length_to_ffi(Length::points(20.0)),
            length_to_ffi(Length::points(10.0)),
        ];
        let auto_columns_max = [
            length_to_ffi(Length::fit_content(Some(BaseLength::fixed(40.0)))),
            length_to_ffi(Length::points(10.0)),
        ];
        let auto_rows = [
            length_to_ffi(Length::points(20.0)),
            length_to_ffi(Length::points(10.0)),
        ];
        let auto_rows_max = [
            length_to_ffi(Length::fit_content(Some(BaseLength::fixed(40.0)))),
            length_to_ffi(Length::points(10.0)),
        ];
        let root_style = SLRustStyle {
            display: display_to_ffi(Display::Grid),
            grid_auto_columns: auto_columns.as_ptr(),
            grid_auto_columns_len: auto_columns.len(),
            grid_auto_columns_max: auto_columns_max.as_ptr(),
            grid_auto_columns_max_len: auto_columns_max.len(),
            grid_auto_rows: auto_rows.as_ptr(),
            grid_auto_rows_len: auto_rows.len(),
            grid_auto_rows_max: auto_rows_max.as_ptr(),
            grid_auto_rows_max_len: auto_rows_max.len(),
            ..SLRustStyle::default()
        };
        let intrinsic_style = SLRustStyle {
            grid_column_start: 1,
            grid_row_start: 1,
            ..SLRustStyle::default()
        };
        let following_style = SLRustStyle {
            grid_column_start: 2,
            grid_row_start: 2,
            ..SLRustStyle::default()
        };
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: root_style,
                    children: vec![2, 3],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: intrinsic_style,
                    children: Vec::new(),
                    measured_size: Some(SLRustSize {
                        width: 70.0,
                        height: 70.0,
                    }),
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 3,
                    style: following_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let constraints = SLRustConstraints {
            width: side_constraint_to_ffi(SideConstraint::indefinite()),
            height: side_constraint_to_ffi(SideConstraint::indefinite()),
        };
        let abi = current_abi_info();
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternalWithOwnerConstraintsChecked(
                &abi as *const SLRustAbiInfo,
                &callbacks as *const SLRustTreeCallbacks,
                1,
                constraints,
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);
        assert_close(out_size.width, 50.0);
        assert_close(out_size.height, 50.0);

        let intrinsic = context.node(2).layout.expect("intrinsic layout");
        let following = context.node(3).layout.expect("following layout");
        assert_close(intrinsic.offset.x, 0.0);
        assert_close(intrinsic.offset.y, 0.0);
        assert_close(intrinsic.size.width, 40.0);
        assert_close(intrinsic.size.height, 40.0);
        assert_close(following.offset.x, 40.0);
        assert_close(following.offset.y, 40.0);
        assert_close(following.size.width, 10.0);
        assert_close(following.size.height, 10.0);
    }

    #[test]
    fn external_callback_grid_maps_explicit_lines_spans_and_self_alignment() {
        let columns = [
            length_to_ffi(Length::points(40.0)),
            length_to_ffi(Length::points(40.0)),
            length_to_ffi(Length::points(40.0)),
        ];
        let rows = [
            length_to_ffi(Length::points(20.0)),
            length_to_ffi(Length::points(20.0)),
        ];
        let root_style = SLRustStyle {
            display: display_to_ffi(Display::Grid),
            width: length_to_ffi(Length::points(130.0)),
            height: length_to_ffi(Length::points(45.0)),
            grid_template_columns: columns.as_ptr(),
            grid_template_columns_len: columns.len(),
            grid_template_rows: rows.as_ptr(),
            grid_template_rows_len: rows.len(),
            column_gap: length_to_ffi(Length::points(5.0)),
            row_gap: length_to_ffi(Length::points(5.0)),
            ..SLRustStyle::default()
        };
        let child_style = SLRustStyle {
            width: length_to_ffi(Length::points(10.0)),
            height: length_to_ffi(Length::points(8.0)),
            grid_column_start: 2,
            grid_row_start: 2,
            grid_column_span: 2,
            justify_self: justify_items_to_ffi(JustifyItems::End),
            align_self: align_items_to_ffi(AlignItems::FlexEnd),
            has_align_self: true,
            ..SLRustStyle::default()
        };
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: root_style,
                    children: vec![2],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: child_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let constraints = SLRustConstraints {
            width: side_constraint_to_ffi(SideConstraint::definite(130.0)),
            height: side_constraint_to_ffi(SideConstraint::definite(45.0)),
        };
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                constraints,
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);
        assert_close(out_size.width, 130.0);
        assert_close(out_size.height, 45.0);

        let child_layout = context.node(2).layout.expect("child layout exported");
        assert_close(child_layout.offset.x, 120.0);
        assert_close(child_layout.offset.y, 37.0);
        assert_close(child_layout.size.width, 10.0);
        assert_close(child_layout.size.height, 8.0);
    }

    #[test]
    fn external_callback_grid_align_self_overrides_container_alignment() {
        let columns = [
            length_to_ffi(Length::points(20.0)),
            length_to_ffi(Length::points(20.0)),
        ];
        let rows = [length_to_ffi(Length::points(30.0))];
        let root_style = SLRustStyle {
            display: display_to_ffi(Display::Grid),
            width: length_to_ffi(Length::points(50.0)),
            height: length_to_ffi(Length::points(40.0)),
            grid_template_columns: columns.as_ptr(),
            grid_template_columns_len: columns.len(),
            grid_template_rows: rows.as_ptr(),
            grid_template_rows_len: rows.len(),
            align_items: align_items_to_ffi(AlignItems::End),
            ..SLRustStyle::default()
        };
        let end_aligned_style = SLRustStyle {
            width: length_to_ffi(Length::points(10.0)),
            height: length_to_ffi(Length::points(10.0)),
            ..SLRustStyle::default()
        };
        let start_aligned_style = SLRustStyle {
            width: length_to_ffi(Length::points(10.0)),
            height: length_to_ffi(Length::points(10.0)),
            align_self: align_items_to_ffi(AlignItems::Start),
            has_align_self: true,
            ..SLRustStyle::default()
        };
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: root_style,
                    children: vec![2, 3],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: end_aligned_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 3,
                    style: start_aligned_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let constraints = SLRustConstraints {
            width: side_constraint_to_ffi(SideConstraint::definite(50.0)),
            height: side_constraint_to_ffi(SideConstraint::definite(40.0)),
        };
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                constraints,
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);
        assert_close(out_size.width, 50.0);
        assert_close(out_size.height, 40.0);

        let end_aligned_layout = context.node(2).layout.expect("end layout exported");
        let start_aligned_layout = context.node(3).layout.expect("start layout exported");
        assert_close(end_aligned_layout.offset.x, 0.0);
        assert_close(end_aligned_layout.offset.y, 20.0);
        assert_close(start_aligned_layout.offset.x, 20.0);
        assert_close(start_aligned_layout.offset.y, 0.0);
    }

    #[test]
    fn external_callback_grid_auto_margins_override_alignment_and_export_used_margins() {
        let columns = [length_to_ffi(Length::points(50.0))];
        let rows = [length_to_ffi(Length::points(40.0))];
        let root_style = SLRustStyle {
            display: display_to_ffi(Display::Grid),
            width: length_to_ffi(Length::points(60.0)),
            height: length_to_ffi(Length::points(50.0)),
            grid_template_columns: columns.as_ptr(),
            grid_template_columns_len: columns.len(),
            grid_template_rows: rows.as_ptr(),
            grid_template_rows_len: rows.len(),
            justify_items: justify_items_to_ffi(JustifyItems::Start),
            align_items: align_items_to_ffi(AlignItems::Start),
            ..SLRustStyle::default()
        };
        let child_style = SLRustStyle {
            width: length_to_ffi(Length::points(10.0)),
            height: length_to_ffi(Length::points(10.0)),
            margin: SLRustRectLength {
                left: length_to_ffi(Length::Auto),
                right: length_to_ffi(Length::Auto),
                top: length_to_ffi(Length::Auto),
                bottom: length_to_ffi(Length::ZERO),
            },
            justify_self: justify_items_to_ffi(JustifyItems::End),
            align_self: align_items_to_ffi(AlignItems::Start),
            has_align_self: true,
            ..SLRustStyle::default()
        };
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: root_style,
                    children: vec![2],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: child_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let constraints = SLRustConstraints {
            width: side_constraint_to_ffi(SideConstraint::definite(60.0)),
            height: side_constraint_to_ffi(SideConstraint::definite(50.0)),
        };
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                constraints,
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);
        assert_close(out_size.width, 60.0);
        assert_close(out_size.height, 50.0);

        let child_layout = context.node(2).layout.expect("child layout exported");
        assert_close(child_layout.offset.x, 20.0);
        assert_close(child_layout.offset.y, 30.0);
        assert_close(child_layout.size.width, 10.0);
        assert_close(child_layout.size.height, 10.0);
        assert_close(child_layout.margin.left, 20.0);
        assert_close(child_layout.margin.right, 20.0);
        assert_close(child_layout.margin.top, 30.0);
        assert_close(child_layout.margin.bottom, 0.0);
    }

    #[test]
    fn external_callback_display_none_child_is_zero_and_skipped_by_grid_auto_placement() {
        let columns = [
            length_to_ffi(Length::points(50.0)),
            length_to_ffi(Length::points(50.0)),
        ];
        let rows = [length_to_ffi(Length::points(20.0))];
        let root_style = SLRustStyle {
            display: display_to_ffi(Display::Grid),
            width: length_to_ffi(Length::points(100.0)),
            height: length_to_ffi(Length::points(20.0)),
            grid_template_columns: columns.as_ptr(),
            grid_template_columns_len: columns.len(),
            grid_template_rows: rows.as_ptr(),
            grid_template_rows_len: rows.len(),
            ..SLRustStyle::default()
        };
        let hidden_child_style = SLRustStyle {
            display: display_to_ffi(Display::None),
            width: length_to_ffi(Length::points(50.0)),
            height: length_to_ffi(Length::points(20.0)),
            ..SLRustStyle::default()
        };
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: root_style,
                    children: vec![2, 3, 4],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: SLRustStyle::default(),
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 3,
                    style: hidden_child_style,
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 4,
                    style: SLRustStyle::default(),
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let constraints = SLRustConstraints {
            width: side_constraint_to_ffi(SideConstraint::definite(100.0)),
            height: side_constraint_to_ffi(SideConstraint::definite(20.0)),
        };
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                constraints,
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::Ok);
        assert_close(out_size.width, 100.0);
        assert_close(out_size.height, 20.0);

        let first_layout = context.node(2).layout.expect("first layout exported");
        let hidden_layout = context.node(3).layout.expect("hidden layout exported");
        let second_layout = context.node(4).layout.expect("second layout exported");
        assert_close(first_layout.offset.x, 0.0);
        assert_close(first_layout.offset.y, 0.0);
        assert_close(first_layout.size.width, 50.0);
        assert_close(first_layout.size.height, 20.0);
        assert_close(hidden_layout.offset.x, 0.0);
        assert_close(hidden_layout.offset.y, 0.0);
        assert_close(hidden_layout.size.width, 0.0);
        assert_close(hidden_layout.size.height, 0.0);
        assert_close(second_layout.offset.x, 50.0);
        assert_close(second_layout.offset.y, 0.0);
        assert_close(second_layout.size.width, 50.0);
        assert_close(second_layout.size.height, 20.0);
    }

    #[test]
    fn external_layout_rejects_duplicate_external_node_ids() {
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: SLRustStyle::default(),
                    children: vec![2, 2],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: SLRustStyle::default(),
                    children: Vec::new(),
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                SLRustConstraints::default(),
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::InvalidTree);
    }

    #[test]
    fn external_layout_rejects_external_tree_cycles() {
        let mut context = TestContext {
            nodes: vec![
                TestNode {
                    id: 1,
                    style: SLRustStyle::default(),
                    children: vec![2],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
                TestNode {
                    id: 2,
                    style: SLRustStyle::default(),
                    children: vec![1],
                    measured_size: None,
                    baseline: None,
                    layout: None,
                    last_constraints: None,
                },
            ],
        };
        let callbacks = callbacks(&mut context);
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                SLRustConstraints::default(),
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::InvalidTree);
        assert!(context.node(1).layout.is_none());
        assert!(context.node(2).layout.is_none());
    }

    #[test]
    fn external_layout_reports_unsupported_tree_for_too_many_children() {
        let mut context = TestContext {
            nodes: vec![TestNode {
                id: 1,
                style: SLRustStyle::default(),
                children: Vec::new(),
                measured_size: None,
                baseline: None,
                layout: None,
                last_constraints: None,
            }],
        };
        let mut callbacks = callbacks(&mut context);
        callbacks.child_count = Some(child_count_too_large);
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                SLRustConstraints::default(),
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::UnsupportedTree);
        assert!(context.node(1).layout.is_none());
    }

    #[test]
    fn external_layout_reports_invalid_style_when_style_callback_fails() {
        let mut context = TestContext {
            nodes: vec![TestNode {
                id: 1,
                style: SLRustStyle::default(),
                children: Vec::new(),
                measured_size: None,
                baseline: None,
                layout: None,
                last_constraints: None,
            }],
        };
        let mut callbacks = callbacks(&mut context);
        callbacks.style = Some(failing_style);
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                SLRustConstraints::default(),
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::InvalidStyle);
        assert!(context.node(1).layout.is_none());
    }

    #[test]
    fn external_layout_reports_invalid_style_for_invalid_style_enum() {
        let mut context = TestContext {
            nodes: vec![TestNode {
                id: 1,
                style: SLRustStyle {
                    display: i32::MAX,
                    ..SLRustStyle::default()
                },
                children: Vec::new(),
                measured_size: None,
                baseline: None,
                layout: None,
                last_constraints: None,
            }],
        };
        let callbacks = callbacks(&mut context);
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                SLRustConstraints::default(),
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::InvalidStyle);
        assert!(context.node(1).layout.is_none());
    }

    #[test]
    fn external_layout_reports_null_pointer_for_non_empty_grid_track_vector() {
        let mut context = TestContext {
            nodes: vec![TestNode {
                id: 1,
                style: SLRustStyle {
                    display: display_to_ffi(Display::Grid),
                    grid_template_columns: ptr::null(),
                    grid_template_columns_len: 1,
                    ..SLRustStyle::default()
                },
                children: Vec::new(),
                measured_size: None,
                baseline: None,
                layout: None,
                last_constraints: None,
            }],
        };
        let callbacks = callbacks(&mut context);
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                SLRustConstraints::default(),
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::NullPointer);
        assert!(context.node(1).layout.is_none());
    }

    #[test]
    fn external_layout_reports_null_pointer_for_null_entrypoint_pointers() {
        let mut out_size = SLRustSize::default();
        let status = unsafe {
            SLRustLayoutExternal(
                ptr::null(),
                1,
                SLRustConstraints::default(),
                &mut out_size as *mut SLRustSize,
            )
        };
        assert_eq!(status, SLRustStatus::NullPointer);

        let mut context = TestContext {
            nodes: vec![TestNode {
                id: 1,
                style: SLRustStyle::default(),
                children: Vec::new(),
                measured_size: None,
                baseline: None,
                layout: None,
                last_constraints: None,
            }],
        };
        let callbacks = callbacks(&mut context);
        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                SLRustConstraints::default(),
                ptr::null_mut(),
            )
        };

        assert_eq!(status, SLRustStatus::NullPointer);
        assert!(context.node(1).layout.is_none());
    }

    #[test]
    fn external_layout_requires_measure_callback_when_has_measure_is_provided() {
        let mut context = TestContext {
            nodes: vec![TestNode {
                id: 1,
                style: SLRustStyle::default(),
                children: Vec::new(),
                measured_size: None,
                baseline: None,
                layout: None,
                last_constraints: None,
            }],
        };
        let mut callbacks = callbacks(&mut context);
        callbacks.measure = None;
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                SLRustConstraints::default(),
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::MissingCallback);
    }

    #[test]
    fn external_layout_requires_has_measure_callback_when_measure_is_provided() {
        let mut context = TestContext {
            nodes: vec![TestNode {
                id: 1,
                style: SLRustStyle::default(),
                children: Vec::new(),
                measured_size: None,
                baseline: None,
                layout: None,
                last_constraints: None,
            }],
        };
        let mut callbacks = callbacks(&mut context);
        callbacks.has_measure = None;
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                SLRustConstraints::default(),
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::MissingCallback);
    }

    #[test]
    fn external_layout_reports_missing_required_callback() {
        let callbacks = SLRustTreeCallbacks {
            context: ptr::null_mut(),
            child_count: None,
            child_at: Some(child_at),
            style: Some(style),
            set_layout: Some(set_layout),
            set_layout_with_constraints: None,
            has_measure: None,
            measure: None,
            baseline: None,
            physical_pixels_per_layout_unit: None,
        };
        let mut out_size = SLRustSize::default();

        let status = unsafe {
            SLRustLayoutExternal(
                &callbacks as *const SLRustTreeCallbacks,
                1,
                SLRustConstraints::default(),
                &mut out_size as *mut SLRustSize,
            )
        };

        assert_eq!(status, SLRustStatus::MissingCallback);
    }
}
