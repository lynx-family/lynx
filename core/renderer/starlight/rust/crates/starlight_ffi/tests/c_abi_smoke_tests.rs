// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use std::collections::BTreeSet;
use std::env;
use std::ffi::{OsStr, OsString};
use std::fs;
use std::path::{Path, PathBuf};
use std::process::{Command, Output};

const LAYOUT_STYLE_SOURCE: &str = include_str!("../../starlight_layout/src/style.rs");
const RUST_FFI_SOURCE: &str = include_str!("../src/lib.rs");
const PUBLIC_HEADER: &str = include_str!("../include/starlight_rust_ffi.h");

#[test]
fn public_header_struct_fields_match_rust_repr_c_declarations() {
    for struct_name in [
        "SLRustAbiInfo",
        "SLRustLength",
        "SLRustRectLength",
        "SLRustRectF32",
        "SLRustSize",
        "SLRustPoint",
        "SLRustSideConstraint",
        "SLRustConstraints",
        "SLRustLayoutResult",
        "SLRustStyle",
        "SLRustTreeCallbacks",
    ] {
        assert_eq!(
            header_struct_fields(PUBLIC_HEADER, struct_name),
            rust_struct_fields(RUST_FFI_SOURCE, struct_name),
            "{struct_name} public header fields must match Rust repr(C) order"
        );
    }
}

#[test]
fn ffi_external_layout_checks_callback_errors_before_flush() {
    let body = braced_body_after(RUST_FFI_SOURCE, "unsafe fn layout_external_inner")
        .expect("layout_external_inner body exists");
    let compact_body = without_ascii_whitespace(body);
    let callback_error_index = compact_body
        .find("tree.callback_error()?;")
        .expect("callback error check exists");
    let size_validation_index = compact_body
        .find("validate_layout_size(size)?;")
        .expect("layout size validation exists");
    let flush_index = compact_body
        .find("tree.flush_layouts();")
        .expect("external layout flush exists");

    assert!(
        callback_error_index < flush_index,
        "FFI must reject failed external callbacks before flushing layouts to the external tree"
    );
    assert!(
        callback_error_index < size_validation_index && size_validation_index < flush_index,
        "FFI must reject non-finite layout outputs before flushing layouts to the external tree"
    );

    let measure_body = braced_body_after(RUST_FFI_SOURCE, "fn measure(&mut self")
        .expect("ExternalTreeSnapshot::measure body exists");
    let compact_measure = without_ascii_whitespace(measure_body);
    for required_fragment in [
        "self.record_callback_error(SLRustStatus::InvalidTree);",
        "letmeasured=size_from_ffi(size);",
        "ifmeasured.is_none(){self.record_callback_error(SLRustStatus::InvalidTree);}",
    ] {
        assert!(
            compact_measure.contains(&without_ascii_whitespace(required_fragment)),
            "measure callback failures must be recorded before returning to the layout engine: {required_fragment}"
        );
    }

    let baseline_body = braced_body_after(RUST_FFI_SOURCE, "fn baseline(&self")
        .expect("ExternalTreeSnapshot::baseline body exists");
    let compact_baseline = without_ascii_whitespace(baseline_body);
    let content_size_validation_index = compact_baseline
        .find("validate_layout_size(content_size).is_err()")
        .expect("baseline content size validation exists");
    let content_size_export_index = compact_baseline
        .find("size_to_ffi(content_size)")
        .expect("baseline content size export exists");
    assert!(
        content_size_validation_index < content_size_export_index,
        "baseline callback content size must be finite before export to external callbacks"
    );
    for required_fragment in [
        "ifvalidate_layout_size(content_size).is_err(){",
        "self.record_callback_error(SLRustStatus::InvalidTree);",
        "returnNone;",
        "if!value.is_finite(){",
        "self.record_callback_error(SLRustStatus::InvalidTree);",
        "returnNone;",
    ] {
        assert!(
            compact_baseline.contains(&without_ascii_whitespace(required_fragment)),
            "non-finite baseline callback results must be recorded before flushing layouts: {required_fragment}"
        );
    }
}

#[test]
fn ffi_external_layout_validates_pending_writebacks_before_flush() {
    for method in ["fn set_layout(&mut self", "fn set_layout_with_constraints("] {
        let body =
            braced_body_after(RUST_FFI_SOURCE, method).expect("layout writeback method exists");
        let compact_body = without_ascii_whitespace(body);
        for required_fragment in [
            "validate_layout_result(layout).is_err()",
            "self.record_callback_error(SLRustStatus::InvalidTree);",
            "return;",
        ] {
            assert!(
                compact_body.contains(&without_ascii_whitespace(required_fragment)),
                "{method} must reject non-finite layout writebacks before queueing them: {required_fragment}"
            );
        }
    }

    let validator_body = braced_body_after(RUST_FFI_SOURCE, "fn validate_layout_result")
        .expect("layout result validator exists");
    let compact_validator = without_ascii_whitespace(validator_body);
    for required_fragment in [
        "validate_layout_size(layout.size)?;",
        "layout.offset.x.is_finite()",
        "layout.offset.y.is_finite()",
        "layout.baseline.is_none_or(f32::is_finite)",
        "rect_f32_is_finite(layout.margin)",
        "rect_f32_is_finite(layout.padding)",
        "rect_f32_is_finite(layout.border)",
        "rect_f32_is_finite(layout.sticky_pos)",
    ] {
        assert!(
            compact_validator.contains(&without_ascii_whitespace(required_fragment)),
            "layout result validator must cover every exported layout field: {required_fragment}"
        );
    }
}

#[test]
fn ffi_external_layout_validates_constraint_writebacks_before_flush() {
    let body = braced_body_after(RUST_FFI_SOURCE, "fn set_layout_with_constraints(")
        .expect("constraint-aware layout writeback method exists");
    let compact_body = without_ascii_whitespace(body);
    for required_fragment in [
        "validate_layout_result(layout).is_err()||validate_constraints(constraints).is_err()",
        "self.record_callback_error(SLRustStatus::InvalidTree);",
        "return;",
    ] {
        assert!(
            compact_body.contains(&without_ascii_whitespace(required_fragment)),
            "constraint-aware writebacks must reject non-finite constraints before queueing them: {required_fragment}"
        );
    }

    let validator_body = braced_body_after(RUST_FFI_SOURCE, "fn validate_constraints")
        .expect("constraints validator exists");
    let compact_validator = without_ascii_whitespace(validator_body);
    for required_fragment in [
        "constraints.width.size.is_finite()",
        "constraints.height.size.is_finite()",
        "Err(SLRustStatus::InvalidTree)",
    ] {
        assert!(
            compact_validator.contains(&without_ascii_whitespace(required_fragment)),
            "constraints validator must reject non-finite constraint sizes: {required_fragment}"
        );
    }
}

#[test]
fn ffi_external_layout_reports_unsupported_tree_for_oversized_snapshots() {
    assert!(
        RUST_FFI_SOURCE.contains("const MAX_EXTERNAL_TREE_NODES: usize"),
        "FFI snapshot builder must define an explicit external tree fast-path size limit"
    );
    let body =
        braced_body_after(RUST_FFI_SOURCE, "fn push_subtree").expect("push_subtree body exists");
    let compact_body = without_ascii_whitespace(body);
    for required_fragment in [
        "self.nodes.len()>=MAX_EXTERNAL_TREE_NODES",
        "child_count>MAX_EXTERNAL_TREE_NODES",
        "returnErr(SLRustStatus::UnsupportedTree);",
    ] {
        assert!(
            compact_body.contains(&without_ascii_whitespace(required_fragment)),
            "oversized external trees must use UnsupportedTree instead of unbounded snapshot growth: {required_fragment}"
        );
    }
}

#[test]
fn public_style_fields_cover_layout_style_fields() {
    let ffi_style_fields = rust_struct_fields(RUST_FFI_SOURCE, "SLRustStyle")
        .into_iter()
        .collect::<BTreeSet<_>>();
    let missing_fields = rust_struct_fields(LAYOUT_STYLE_SOURCE, "Style")
        .into_iter()
        .flat_map(|field| {
            ffi_style_field_requirements(&field)
                .into_iter()
                .filter(|ffi_field| !ffi_style_fields.contains(ffi_field))
                .map(move |ffi_field| format!("{field} -> {ffi_field}"))
        })
        .collect::<Vec<_>>();

    assert!(
        missing_fields.is_empty(),
        "SLRustStyle must expose every layout Style field; missing: {}",
        missing_fields.join(", ")
    );
}

#[test]
fn public_header_abi_version_macros_are_stable() {
    assert_eq!(header_u32_define(PUBLIC_HEADER, "SLRustAbiVersionMajor"), 1);
    assert_eq!(
        header_u32_define(PUBLIC_HEADER, "SLRustAbiVersionMinor"),
        15
    );
    assert_eq!(header_u32_define(PUBLIC_HEADER, "SLRustAbiVersionPatch"), 0);
}

#[test]
fn public_header_enum_values_cover_the_full_ffi_surface() {
    for (enum_name, expected_values) in [
        (
            "SLRustStatus",
            &[
                ("SLRustStatusOk", 0),
                ("SLRustStatusNullPointer", 1),
                ("SLRustStatusMissingCallback", 2),
                ("SLRustStatusInvalidStyle", 3),
                ("SLRustStatusInvalidTree", 4),
                ("SLRustStatusPanic", 5),
                ("SLRustStatusAbiMismatch", 6),
                ("SLRustStatusDisabled", 7),
                ("SLRustStatusUnsupportedTree", 8),
                ("SLRustStatusFixedNodeSetMismatch", 9),
            ][..],
        ),
        (
            "SLRustMeasureMode",
            &[
                ("SLRustMeasureModeIndefinite", 0),
                ("SLRustMeasureModeDefinite", 1),
                ("SLRustMeasureModeAtMost", 2),
            ],
        ),
        (
            "SLRustLengthKind",
            &[
                ("SLRustLengthAuto", 0),
                ("SLRustLengthPoints", 1),
                ("SLRustLengthPercent", 2),
                ("SLRustLengthCalc", 3),
                ("SLRustLengthFr", 4),
                ("SLRustLengthMaxContent", 5),
                ("SLRustLengthFitContent", 6),
                ("SLRustLengthMinContent", 7),
            ],
        ),
        (
            "SLRustDisplay",
            &[
                ("SLRustDisplayNone", 0),
                ("SLRustDisplayBlock", 1),
                ("SLRustDisplayFlex", 2),
                ("SLRustDisplayLinear", 3),
                ("SLRustDisplayRelative", 4),
                ("SLRustDisplayGrid", 5),
            ],
        ),
        (
            "SLRustPositionType",
            &[
                ("SLRustPositionStatic", 0),
                ("SLRustPositionRelative", 1),
                ("SLRustPositionAbsolute", 2),
                ("SLRustPositionFixed", 3),
                ("SLRustPositionSticky", 4),
            ],
        ),
        (
            "SLRustBoxSizing",
            &[
                ("SLRustBoxSizingContentBox", 0),
                ("SLRustBoxSizingBorderBox", 1),
            ],
        ),
        (
            "SLRustDirection",
            &[("SLRustDirectionLtr", 0), ("SLRustDirectionRtl", 1)],
        ),
        (
            "SLRustVisibility",
            &[
                ("SLRustVisibilityVisible", 0),
                ("SLRustVisibilityHidden", 1),
                ("SLRustVisibilityCollapse", 2),
            ],
        ),
        (
            "SLRustFlexDirection",
            &[
                ("SLRustFlexDirectionRow", 0),
                ("SLRustFlexDirectionRowReverse", 1),
                ("SLRustFlexDirectionColumn", 2),
                ("SLRustFlexDirectionColumnReverse", 3),
            ],
        ),
        (
            "SLRustLinearOrientation",
            &[
                ("SLRustLinearOrientationHorizontal", 0),
                ("SLRustLinearOrientationHorizontalReverse", 1),
                ("SLRustLinearOrientationVertical", 2),
                ("SLRustLinearOrientationVerticalReverse", 3),
                ("SLRustLinearOrientationRow", 4),
                ("SLRustLinearOrientationRowReverse", 5),
                ("SLRustLinearOrientationColumn", 6),
                ("SLRustLinearOrientationColumnReverse", 7),
            ],
        ),
        (
            "SLRustFlexWrap",
            &[
                ("SLRustFlexWrapNoWrap", 0),
                ("SLRustFlexWrapWrap", 1),
                ("SLRustFlexWrapWrapReverse", 2),
            ],
        ),
        (
            "SLRustJustifyContent",
            &[
                ("SLRustJustifyContentStretch", 0),
                ("SLRustJustifyContentFlexStart", 1),
                ("SLRustJustifyContentStart", 2),
                ("SLRustJustifyContentCenter", 3),
                ("SLRustJustifyContentFlexEnd", 4),
                ("SLRustJustifyContentEnd", 5),
                ("SLRustJustifyContentSpaceBetween", 6),
                ("SLRustJustifyContentSpaceAround", 7),
                ("SLRustJustifyContentSpaceEvenly", 8),
            ],
        ),
        (
            "SLRustAlignItems",
            &[
                ("SLRustAlignItemsStretch", 0),
                ("SLRustAlignItemsFlexStart", 1),
                ("SLRustAlignItemsStart", 2),
                ("SLRustAlignItemsCenter", 3),
                ("SLRustAlignItemsFlexEnd", 4),
                ("SLRustAlignItemsEnd", 5),
                ("SLRustAlignItemsBaseline", 6),
            ],
        ),
        (
            "SLRustAlignContent",
            &[
                ("SLRustAlignContentFlexStart", 0),
                ("SLRustAlignContentCenter", 1),
                ("SLRustAlignContentFlexEnd", 2),
                ("SLRustAlignContentSpaceBetween", 3),
                ("SLRustAlignContentSpaceAround", 4),
                ("SLRustAlignContentSpaceEvenly", 5),
                ("SLRustAlignContentStretch", 6),
                ("SLRustAlignContentStart", 7),
                ("SLRustAlignContentEnd", 8),
            ],
        ),
        (
            "SLRustJustifyItems",
            &[
                ("SLRustJustifyItemsAuto", 0),
                ("SLRustJustifyItemsStretch", 1),
                ("SLRustJustifyItemsStart", 2),
                ("SLRustJustifyItemsCenter", 3),
                ("SLRustJustifyItemsEnd", 4),
            ],
        ),
        (
            "SLRustGridAutoFlow",
            &[
                ("SLRustGridAutoFlowRow", 0),
                ("SLRustGridAutoFlowColumn", 1),
                ("SLRustGridAutoFlowDense", 2),
                ("SLRustGridAutoFlowRowDense", 3),
                ("SLRustGridAutoFlowColumnDense", 4),
            ],
        ),
        (
            "SLRustRelativeCenter",
            &[
                ("SLRustRelativeCenterNone", 0),
                ("SLRustRelativeCenterHorizontal", 1),
                ("SLRustRelativeCenterVertical", 2),
                ("SLRustRelativeCenterBoth", 3),
            ],
        ),
        (
            "SLRustLinearGravity",
            &[
                ("SLRustLinearGravityNone", 0),
                ("SLRustLinearGravityTop", 1),
                ("SLRustLinearGravityBottom", 2),
                ("SLRustLinearGravityLeft", 3),
                ("SLRustLinearGravityRight", 4),
                ("SLRustLinearGravityCenterVertical", 5),
                ("SLRustLinearGravityCenterHorizontal", 6),
                ("SLRustLinearGravitySpaceBetween", 7),
                ("SLRustLinearGravityStart", 8),
                ("SLRustLinearGravityEnd", 9),
                ("SLRustLinearGravityCenter", 10),
            ],
        ),
        (
            "SLRustLinearLayoutGravity",
            &[
                ("SLRustLinearLayoutGravityNone", 0),
                ("SLRustLinearLayoutGravityTop", 1),
                ("SLRustLinearLayoutGravityBottom", 2),
                ("SLRustLinearLayoutGravityLeft", 3),
                ("SLRustLinearLayoutGravityRight", 4),
                ("SLRustLinearLayoutGravityCenterVertical", 5),
                ("SLRustLinearLayoutGravityCenterHorizontal", 6),
                ("SLRustLinearLayoutGravityFillVertical", 7),
                ("SLRustLinearLayoutGravityFillHorizontal", 8),
                ("SLRustLinearLayoutGravityCenter", 9),
                ("SLRustLinearLayoutGravityStretch", 10),
                ("SLRustLinearLayoutGravityStart", 11),
                ("SLRustLinearLayoutGravityEnd", 12),
            ],
        ),
        (
            "SLRustLinearCrossGravity",
            &[
                ("SLRustLinearCrossGravityNone", 0),
                ("SLRustLinearCrossGravityStart", 1),
                ("SLRustLinearCrossGravityEnd", 2),
                ("SLRustLinearCrossGravityCenter", 3),
                ("SLRustLinearCrossGravityStretch", 4),
            ],
        ),
        (
            "SLRustListComponentType",
            &[
                ("SLRustListComponentTypeNone", -1),
                ("SLRustListComponentTypeHeader", 0),
                ("SLRustListComponentTypeFooter", 1),
                ("SLRustListComponentTypeListRow", 2),
                ("SLRustListComponentTypeDefault", 3),
            ],
        ),
    ] {
        let expected_values = expected_values
            .iter()
            .map(|&(name, value)| (name.to_owned(), value))
            .collect::<Vec<_>>();
        assert_eq!(
            header_enum_values(PUBLIC_HEADER, enum_name),
            expected_values,
            "{enum_name} public header values must stay stable"
        );
    }

    assert_eq!(
        header_anonymous_optional_values(PUBLIC_HEADER),
        [
            ("SLRustOptionalUnset".to_owned(), -1),
            ("SLRustGridLineAuto".to_owned(), 0),
        ],
        "anonymous public sentinel enum values must stay stable"
    );
}

#[test]
fn public_header_compiles_as_c_and_cpp_with_abi_smoke_checks() {
    compile_smoke("clang", "starlight_ffi_c_abi_smoke.c", C_ABI_SMOKE);
    compile_smoke("clang++", "starlight_ffi_cpp_abi_smoke.cc", CPP_ABI_SMOKE);
}

#[test]
fn cxx_tree_adapter_rejects_partial_measure_contracts_at_compile_time() {
    compile_smoke_expect_failure(
        "clang++",
        "starlight_ffi_cpp_partial_measure_smoke.cc",
        CPP_PARTIAL_MEASURE_ADAPTER_SMOKE,
        "Rust Starlight external tree measure support requires both",
    );
}

#[test]
fn cxx_tree_adapter_rejects_missing_required_callbacks_at_compile_time() {
    compile_smoke_expect_failure(
        "clang++",
        "starlight_ffi_cpp_missing_required_smoke.cc",
        CPP_MISSING_REQUIRED_ADAPTER_SMOKE,
        "Rust Starlight external tree adapters require",
    );
}

#[test]
fn public_ffi_header_does_not_expose_runtime_layout_switch() {
    for forbidden_fragment in [
        "SLRustLayoutRuntimeSetEnabled",
        "SLRustLayoutRuntimeIsEnabled",
        "RUNTIME_LAYOUT_ENABLED",
        "layout_runtime_enabled",
    ] {
        assert!(
            !PUBLIC_HEADER.contains(forbidden_fragment) && !RUST_FFI_SOURCE.contains(forbidden_fragment),
            "standalone FFI must not expose a production runtime layout switch: {forbidden_fragment}"
        );
    }
}

#[test]
fn ffi_library_env_path_accepts_workspace_relative_paths() {
    let path = resolve_ffi_library_env_path(PathBuf::from("target/release/libstarlight_ffi.a"));

    assert_eq!(
        path,
        rust_workspace_root().join("target/release/libstarlight_ffi.a")
    );
}

#[test]
fn public_header_links_and_runs_c_smoke_when_ffi_library_is_available() {
    let Some(library_path) = find_ffi_library() else {
        eprintln!("skipped C ABI link/run smoke: libstarlight_ffi was not built");
        return;
    };
    let Some(executable_path) = compile_linked_smoke(
        "clang",
        "starlight_ffi_c_link_smoke.c",
        C_LINK_SMOKE,
        &library_path,
    ) else {
        eprintln!("skipped C ABI link/run smoke: clang is unavailable");
        return;
    };

    let mut command = Command::new(&executable_path);
    if let Some(library_dir) = library_path.parent() {
        command.env(dynamic_library_path_variable(), library_dir);
    }
    let output = command.output().expect("run linked C ABI smoke executable");
    assert!(
        output.status.success(),
        "linked C ABI smoke failed with status {}\nstdout:\n{}\nstderr:\n{}",
        output.status,
        String::from_utf8_lossy(&output.stdout),
        String::from_utf8_lossy(&output.stderr)
    );
}

#[test]
fn public_header_links_and_runs_cpp_smoke_when_ffi_library_is_available() {
    let Some(library_path) = find_ffi_library() else {
        eprintln!("skipped C++ ABI link/run smoke: libstarlight_ffi was not built");
        return;
    };
    let Some(executable_path) = compile_linked_smoke(
        "clang++",
        "starlight_ffi_cpp_link_smoke.cc",
        CPP_LINK_SMOKE,
        &library_path,
    ) else {
        eprintln!("skipped C++ ABI link/run smoke: clang++ is unavailable");
        return;
    };

    let mut command = Command::new(&executable_path);
    if let Some(library_dir) = library_path.parent() {
        command.env(dynamic_library_path_variable(), library_dir);
    }
    let output = command
        .output()
        .expect("run linked C++ ABI smoke executable");
    assert!(
        output.status.success(),
        "linked C++ ABI smoke failed with status {}\nstdout:\n{}\nstderr:\n{}",
        output.status,
        String::from_utf8_lossy(&output.stdout),
        String::from_utf8_lossy(&output.stderr)
    );
}

fn rust_struct_fields(source: &str, struct_name: &str) -> Vec<String> {
    let marker = format!("pub struct {struct_name}");
    let body = braced_body_after(source, &marker).expect("Rust struct body exists");
    body.lines()
        .map(str::trim)
        .filter_map(|line| line.strip_prefix("pub "))
        .filter_map(|line| line.split(':').next())
        .map(str::trim)
        .map(str::to_owned)
        .collect()
}

fn ffi_style_field_requirements(layout_field: &str) -> Vec<String> {
    let mut fields = match layout_field {
        "aspect_ratio" => vec!["aspect_ratio".to_owned(), "has_aspect_ratio".to_owned()],
        "align_self" => vec!["align_self".to_owned(), "has_align_self".to_owned()],
        field => vec![field.to_owned()],
    };

    if is_grid_track_vector_field(layout_field) {
        fields.push(format!("{layout_field}_len"));
    }

    fields
}

fn is_grid_track_vector_field(field: &str) -> bool {
    matches!(
        field,
        "grid_template_columns"
            | "grid_template_rows"
            | "grid_template_columns_max"
            | "grid_template_rows_max"
            | "grid_auto_columns"
            | "grid_auto_rows"
            | "grid_auto_columns_max"
            | "grid_auto_rows_max"
    )
}

fn header_struct_fields(header: &str, struct_name: &str) -> Vec<String> {
    let marker = format!("typedef struct {struct_name}");
    let body = braced_body_after(header, &marker).expect("header struct body exists");
    body.lines()
        .map(str::trim)
        .filter_map(|line| line.strip_suffix(';'))
        .filter_map(last_identifier)
        .map(str::to_owned)
        .collect()
}

fn header_enum_values(header: &str, enum_name: &str) -> Vec<(String, i32)> {
    let marker = format!("typedef enum {enum_name}");
    let body = braced_body_after(header, &marker).expect("header enum body exists");
    parse_c_enum_values(body)
}

fn header_anonymous_optional_values(header: &str) -> Vec<(String, i32)> {
    let body = braced_body_after(header, "enum {").expect("anonymous header enum body exists");
    parse_c_enum_values(body)
}

fn header_u32_define(header: &str, name: &str) -> u32 {
    let marker = format!("#define {name} ");
    let value = header
        .lines()
        .find_map(|line| line.trim().strip_prefix(&marker))
        .unwrap_or_else(|| panic!("{name} macro exists in public header"));
    value
        .trim_end_matches(['u', 'U'])
        .parse::<u32>()
        .unwrap_or_else(|error| panic!("{name} macro must be a u32 literal: {error}"))
}

fn parse_c_enum_values(body: &str) -> Vec<(String, i32)> {
    body.lines()
        .map(str::trim)
        .filter(|line| !line.is_empty())
        .map(|line| line.trim_end_matches(','))
        .map(|line| {
            let (name, value) = line
                .split_once('=')
                .expect("public header enum values must be explicit");
            (
                name.trim().to_owned(),
                value
                    .trim()
                    .parse::<i32>()
                    .expect("public header enum value must fit in i32"),
            )
        })
        .collect()
}

fn braced_body_after<'a>(source: &'a str, marker: &str) -> Option<&'a str> {
    let marker_start = source.find(marker)?;
    let after_marker = &source[marker_start..];
    let body_start = after_marker.find('{')? + marker_start + 1;
    let mut depth = 1usize;
    for (offset, byte) in source[body_start..].bytes().enumerate() {
        match byte {
            b'{' => depth += 1,
            b'}' => {
                depth -= 1;
                if depth == 0 {
                    return Some(&source[body_start..body_start + offset]);
                }
            }
            _ => {}
        }
    }
    None
}

fn last_identifier(line_without_semicolon: &str) -> Option<&str> {
    let mut end = line_without_semicolon.len();
    while end > 0 && !is_identifier_byte(line_without_semicolon.as_bytes()[end - 1]) {
        end -= 1;
    }
    let mut start = end;
    while start > 0 && is_identifier_byte(line_without_semicolon.as_bytes()[start - 1]) {
        start -= 1;
    }
    (start < end).then_some(&line_without_semicolon[start..end])
}

const fn is_identifier_byte(byte: u8) -> bool {
    byte.is_ascii_alphanumeric() || byte == b'_'
}

fn without_ascii_whitespace(source: &str) -> String {
    source
        .chars()
        .filter(|character| !character.is_ascii_whitespace())
        .collect()
}

fn compile_smoke(compiler: &str, file_name: &str, source: &str) {
    compile_smoke_with_include_dirs(compiler, file_name, source, &[ffi_include_dir()]);
}

fn compile_smoke_expect_failure(
    compiler: &str,
    file_name: &str,
    source: &str,
    expected_stderr: &str,
) {
    let source_path = write_smoke_source(file_name, source);
    let standard = if file_name.ends_with(".cc") {
        "-std=c++17"
    } else {
        "-std=c11"
    };
    let output = run_compiler(
        compiler,
        [
            OsStr::new(standard),
            OsStr::new("-fsyntax-only"),
            OsStr::new("-I"),
            ffi_include_dir().as_os_str(),
            source_path.as_os_str(),
        ],
    );

    let Some(output) = output else {
        eprintln!("skipped {file_name}: {compiler} is unavailable");
        return;
    };

    let stderr = String::from_utf8_lossy(&output.stderr);
    assert!(
        !output.status.success(),
        "{compiler} unexpectedly accepted {file_name}\nstdout:\n{}\nstderr:\n{}",
        String::from_utf8_lossy(&output.stdout),
        stderr
    );
    assert!(
        stderr.contains(expected_stderr),
        "{compiler} failed for {file_name}, but did not report the expected contract error\nexpected: {expected_stderr}\nstderr:\n{stderr}"
    );
}

fn compile_smoke_with_include_dirs(
    compiler: &str,
    file_name: &str,
    source: &str,
    include_dirs: &[PathBuf],
) {
    let source_path = write_smoke_source(file_name, source);
    let standard = if file_name.ends_with(".cc") {
        "-std=c++17"
    } else {
        "-std=c11"
    };
    let mut args = vec![OsString::from(standard), OsString::from("-fsyntax-only")];
    for include_dir in include_dirs {
        args.push(OsString::from("-I"));
        args.push(include_dir.as_os_str().to_owned());
    }
    args.push(source_path.as_os_str().to_owned());
    let output = run_compiler(compiler, args);

    let Some(output) = output else {
        eprintln!("skipped {file_name}: {compiler} is unavailable");
        return;
    };

    assert!(
        output.status.success(),
        "{compiler} failed for {file_name}\nstdout:\n{}\nstderr:\n{}",
        String::from_utf8_lossy(&output.stdout),
        String::from_utf8_lossy(&output.stderr)
    );
}

fn ffi_include_dir() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("include")
}

fn rust_workspace_root() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("../..")
        .canonicalize()
        .expect("resolve Rust Starlight workspace root")
}

fn compile_linked_smoke(
    compiler: &str,
    file_name: &str,
    source: &str,
    library_path: &Path,
) -> Option<PathBuf> {
    let source_path = write_smoke_source(file_name, source);
    let executable_path = source_path.with_extension(env::consts::EXE_EXTENSION);
    let include_dir = Path::new(env!("CARGO_MANIFEST_DIR")).join("include");
    let standard = if file_name.ends_with(".cc") {
        "-std=c++17"
    } else {
        "-std=c11"
    };
    let output = run_compiler(
        compiler,
        [
            OsStr::new(standard),
            OsStr::new("-I"),
            include_dir.as_os_str(),
            source_path.as_os_str(),
            library_path.as_os_str(),
            OsStr::new("-o"),
            executable_path.as_os_str(),
        ],
    )?;

    assert!(
        output.status.success(),
        "{compiler} failed to link C ABI smoke with {}\nstdout:\n{}\nstderr:\n{}",
        library_path.display(),
        String::from_utf8_lossy(&output.stdout),
        String::from_utf8_lossy(&output.stderr)
    );
    Some(executable_path)
}

fn find_ffi_library() -> Option<PathBuf> {
    if let Some(path) = env::var_os("STARLIGHT_RUST_FFI_LIBRARY").map(PathBuf::from) {
        let resolved_path = resolve_ffi_library_env_path(path);
        assert!(
            resolved_path.exists(),
            "STARLIGHT_RUST_FFI_LIBRARY points to a missing file: {}",
            resolved_path.display()
        );
        return Some(resolved_path);
    }

    let deps_dir = env::current_exe().ok()?.parent()?.to_path_buf();
    [
        "libstarlight_ffi.dylib",
        "libstarlight_ffi.so",
        "starlight_ffi.dll",
        "libstarlight_ffi.a",
    ]
    .into_iter()
    .map(|file_name| deps_dir.join(file_name))
    .find(|path| path.exists())
}

fn resolve_ffi_library_env_path(path: PathBuf) -> PathBuf {
    if path.is_absolute() || path.exists() {
        path
    } else {
        rust_workspace_root().join(path)
    }
}

const fn dynamic_library_path_variable() -> &'static str {
    if cfg!(target_os = "macos") {
        "DYLD_LIBRARY_PATH"
    } else if cfg!(target_os = "windows") {
        "PATH"
    } else {
        "LD_LIBRARY_PATH"
    }
}

fn write_smoke_source(file_name: &str, source: &str) -> PathBuf {
    let base_dir = env::var_os("CARGO_TARGET_TMPDIR")
        .map(PathBuf::from)
        .unwrap_or_else(env::temp_dir);
    let smoke_dir = base_dir.join(format!("starlight_ffi_abi_smoke_{}", std::process::id()));
    fs::create_dir_all(&smoke_dir).expect("create ABI smoke temp directory");
    let source_path = smoke_dir.join(file_name);
    fs::write(&source_path, source).expect("write ABI smoke source");
    source_path
}

fn run_compiler<I, S>(compiler: &str, args: I) -> Option<Output>
where
    I: IntoIterator<Item = S>,
    S: AsRef<OsStr>,
{
    match Command::new(compiler).args(args).output() {
        Ok(output) => Some(output),
        Err(error) if error.kind() == std::io::ErrorKind::NotFound => None,
        Err(error) => panic!("failed to run {compiler}: {error}"),
    }
}

const C_LINK_SMOKE: &str = r#"
#include "starlight_rust_ffi.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

typedef struct TestNode {
  SLRustStyle style;
  SLRustSize measured_size;
  SLRustConstraints last_measure_constraints;
  SLRustLayoutResult layout;
  bool has_layout;
  bool has_measure;
  bool measure_called;
} TestNode;

typedef struct TestContext {
  TestNode root;
  TestNode child;
  TestNode second_child;
  size_t root_child_count;
} TestContext;

static size_t child_count(void* context, SLRustNodeId node) {
  TestContext* test_context = (TestContext*)context;
  if (node == 1) {
    return test_context->root_child_count;
  }
  return 0;
}

static SLRustNodeId child_at(void* context, SLRustNodeId node, size_t index) {
  TestContext* test_context = (TestContext*)context;
  if (node != 1 || index >= test_context->root_child_count) {
    return 0;
  }
  return index == 0 ? 2 : 3;
}

static bool style_callback(void* context, SLRustNodeId node,
                           SLRustStyle* out_style) {
  TestContext* test_context = (TestContext*)context;
  if (out_style == NULL) {
    return false;
  }
  if (node == 1) {
    *out_style = test_context->root.style;
    return true;
  }
  if (node == 2) {
    *out_style = test_context->child.style;
    return true;
  }
  if (node == 3) {
    *out_style = test_context->second_child.style;
    return true;
  }
  return false;
}

static void set_layout(void* context, SLRustNodeId node,
                       SLRustLayoutResult layout) {
  TestContext* test_context = (TestContext*)context;
  if (node == 1) {
    test_context->root.layout = layout;
    test_context->root.has_layout = true;
  } else if (node == 2) {
    test_context->child.layout = layout;
    test_context->child.has_layout = true;
  } else if (node == 3) {
    test_context->second_child.layout = layout;
    test_context->second_child.has_layout = true;
  }
}

static bool has_measure(void* context, SLRustNodeId node) {
  TestContext* test_context = (TestContext*)context;
  return node == 1 && test_context->root.has_measure;
}

static bool measure(void* context, SLRustNodeId node,
                    SLRustConstraints constraints, SLRustSize* out_size) {
  TestContext* test_context = (TestContext*)context;
  if (node != 1 || out_size == NULL || !test_context->root.has_measure) {
    return false;
  }
  test_context->root.measure_called = true;
  test_context->root.last_measure_constraints = constraints;
  *out_size = test_context->root.measured_size;
  return true;
}

int main(void) {
  TestContext context = {0};
  SLRustAbiInfo caller_abi = SLRustMakeCallerAbiInfo();
  SLRustAbiInfo library_abi = {0};
  if (SLRustGetAbiInfo(&library_abi) != SLRustStatusOk) {
    return 5;
  }
  if (caller_abi.version_major != SLRustAbiVersionMajor ||
      caller_abi.version_minor != SLRustAbiVersionMinor ||
      caller_abi.version_patch != SLRustAbiVersionPatch) {
    return 6;
  }
  if (caller_abi.size_of_abi_info != sizeof(SLRustAbiInfo) ||
      caller_abi.align_of_abi_info != _Alignof(SLRustAbiInfo) ||
      caller_abi.size_of_style != sizeof(SLRustStyle) ||
      caller_abi.align_of_style != _Alignof(SLRustStyle) ||
      caller_abi.size_of_tree_callbacks != sizeof(SLRustTreeCallbacks) ||
      caller_abi.align_of_tree_callbacks != _Alignof(SLRustTreeCallbacks)) {
    return 7;
  }
  if (library_abi.size_of_abi_info != caller_abi.size_of_abi_info ||
      library_abi.align_of_abi_info != caller_abi.align_of_abi_info ||
      library_abi.size_of_style != caller_abi.size_of_style ||
      library_abi.align_of_style != caller_abi.align_of_style ||
      library_abi.size_of_tree_callbacks != caller_abi.size_of_tree_callbacks ||
      library_abi.align_of_tree_callbacks != caller_abi.align_of_tree_callbacks) {
    return 7;
  }
  if (SLRustGetAbiInfo(NULL) != SLRustStatusNullPointer) {
    return 8;
  }
  if (strcmp(SLRustStatusName(SLRustStatusOk), "Ok") != 0 ||
      strcmp(SLRustStatusName(SLRustStatusInvalidStyle), "InvalidStyle") != 0 ||
      strcmp(SLRustStatusName(SLRustStatusFixedNodeSetMismatch),
             "FixedNodeSetMismatch") != 0 ||
      strcmp(SLRustStatusName((SLRustStatus)999), "Unknown") != 0) {
    return 605;
  }

  SLRustStyleDefault(&context.root.style);
  context.root.style.display = SLRustDisplayFlex;
  context.root.style.width = SLRustMakePointsLength(120.0f);
  context.root.style.height = SLRustMakePointsLength(20.0f);

  SLRustLength helper_calc = SLRustMakeCalcLength(4.0f, 10.0f);
  SLRustLength helper_fit_content =
      SLRustMakeFitContentLengthWithBase(8.0f, 25.0f, true);
  SLRustLength helper_fr = SLRustMakeFrLength(2.0f);
  SLRustLength helper_auto = SLRustMakeAutoLength();
  SLRustPoint helper_point = SLRustMakePoint(3.0f, 4.0f);
  SLRustRectF32 helper_rect = SLRustMakeRectF32(1.0f, 2.0f, 3.0f, 4.0f);
  if (helper_calc.kind != SLRustLengthCalc || helper_calc.value != 4.0f ||
      helper_calc.percent != 10.0f || !helper_calc.has_base ||
      !helper_calc.has_percentage ||
      helper_fit_content.kind != SLRustLengthFitContent ||
      helper_fit_content.value != 8.0f ||
      helper_fit_content.percent != 25.0f ||
      !helper_fit_content.has_base ||
      !helper_fit_content.has_percentage ||
      helper_fr.kind != SLRustLengthFr || helper_fr.value != 2.0f ||
      helper_auto.kind != SLRustLengthAuto ||
      helper_point.x != 3.0f || helper_point.y != 4.0f ||
      helper_rect.left != 1.0f || helper_rect.right != 2.0f ||
      helper_rect.top != 3.0f || helper_rect.bottom != 4.0f) {
    return 13;
  }

  SLRustTreeCallbacks callbacks = {0};
  callbacks.context = &context;
  callbacks.child_count = child_count;
  callbacks.child_at = child_at;
  callbacks.style = style_callback;
  callbacks.set_layout = set_layout;

  SLRustConstraints constraints = SLRustMakeConstraints(
      SLRustMakeDefiniteConstraint(120.0f),
      SLRustMakeDefiniteConstraint(20.0f));
  SLRustSize out_size = SLRustMakeSize(0.0f, 0.0f);
  SLRustStatus status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(&caller_abi, &callbacks, 1,
                                                      constraints, &out_size);

  if (status != SLRustStatusOk) {
    return 1;
  }
  if (out_size.width != 120.0f || out_size.height != 20.0f) {
    return 2;
  }
  if (!context.root.has_layout) {
    return 3;
  }
  if (context.root.layout.size.width != 120.0f ||
      context.root.layout.size.height != 20.0f) {
    return 4;
  }

  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  context.root_child_count = 2;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  context.root.style.display = SLRustDisplayFlex;
  context.root.style.width = SLRustMakePointsLength(120.0f);
  context.root.style.height = SLRustMakePointsLength(30.0f);
  context.root.style.align_items = SLRustAlignItemsFlexStart;
  context.root.style.justify_content = SLRustJustifyContentFlexStart;
  context.root.style.column_gap =
      SLRustMakeFitContentLengthWithBase(12.0f, 0.0f, false);
  context.child.style.flex_basis = SLRustMakePointsLength(20.0f);
  context.child.style.height = SLRustMakePointsLength(10.0f);
  context.second_child.style.flex_basis = SLRustMakePointsLength(18.0f);
  context.second_child.style.height = SLRustMakePointsLength(12.0f);
  SLRustConstraints full_value_column_gap_constraints =
      SLRustMakeConstraints(SLRustMakeDefiniteConstraint(120.0f),
                            SLRustMakeDefiniteConstraint(30.0f));
  SLRustStatus full_value_column_gap_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, full_value_column_gap_constraints,
          &out_size);
  if (full_value_column_gap_status != SLRustStatusOk) {
    return 601;
  }
  if (context.second_child.layout.offset.x != 32.0f ||
      context.second_child.layout.offset.y != 0.0f) {
    return 602;
  }

  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  context.root.style.display = SLRustDisplayFlex;
  context.root.style.width = SLRustMakePointsLength(30.0f);
  context.root.style.height = SLRustMakePointsLength(80.0f);
  context.root.style.flex_wrap = SLRustFlexWrapWrap;
  context.root.style.align_items = SLRustAlignItemsFlexStart;
  context.root.style.align_content = SLRustAlignContentFlexStart;
  context.root.style.row_gap = SLRustMakeFrLength(1.0f);
  context.child.style.flex_basis = SLRustMakePointsLength(20.0f);
  context.child.style.height = SLRustMakePointsLength(10.0f);
  context.second_child.style.flex_basis = SLRustMakePointsLength(20.0f);
  context.second_child.style.height = SLRustMakePointsLength(10.0f);
  SLRustConstraints full_value_row_gap_constraints =
      SLRustMakeConstraints(SLRustMakeDefiniteConstraint(30.0f),
                            SLRustMakeDefiniteConstraint(80.0f));
  SLRustStatus full_value_row_gap_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, full_value_row_gap_constraints,
          &out_size);
  if (full_value_row_gap_status != SLRustStatusOk) {
    return 603;
  }
  if (context.second_child.layout.offset.x != 0.0f ||
      context.second_child.layout.offset.y != 11.0f) {
    return 604;
  }

  SLRustLength full_value_edge_lengths[3] = {
      SLRustMakeMaxContentLength(),
      SLRustMakeFitContentLengthWithBase(4.0f, 0.0f, false),
      SLRustMakeFrLength(1.0f),
  };
  float full_value_edge_expected[3] = {0.0f, 4.0f, 1.0f};
  for (size_t edge_index = 0; edge_index < 3; edge_index++) {
    context.root.has_layout = false;
    context.child.has_layout = false;
    context.second_child.has_layout = false;
    context.root_child_count = 1;
    SLRustStyleDefault(&context.root.style);
    SLRustStyleDefault(&context.child.style);
    SLRustStyleDefault(&context.second_child.style);
    context.root.style.display = SLRustDisplayFlex;
    context.root.style.width = SLRustMakePointsLength(80.0f);
    context.root.style.height = SLRustMakePointsLength(20.0f);
    context.root.style.align_items = SLRustAlignItemsFlexStart;
    context.child.style.position = SLRustPositionRelative;
    context.child.style.left = full_value_edge_lengths[edge_index];
    context.child.style.margin.left = full_value_edge_lengths[edge_index];
    context.child.style.padding.left = full_value_edge_lengths[edge_index];
    context.child.style.flex_basis = SLRustMakePointsLength(10.0f);
    context.child.style.height = SLRustMakePointsLength(6.0f);
    context.child.style.box_sizing = SLRustBoxSizingContentBox;
    SLRustConstraints full_value_edge_constraints =
        SLRustMakeConstraints(SLRustMakeDefiniteConstraint(80.0f),
                              SLRustMakeDefiniteConstraint(20.0f));
    SLRustStatus full_value_edge_status =
        SLRustLayoutExternalWithOwnerConstraintsChecked(
            &caller_abi, &callbacks, 1, full_value_edge_constraints,
            &out_size);
    if (full_value_edge_status != SLRustStatusOk) {
      return 606;
    }
    float expected_edge = full_value_edge_expected[edge_index];
    if (context.child.layout.padding.left != expected_edge ||
        context.child.layout.margin.left != expected_edge ||
        context.child.layout.offset.x != expected_edge * 2.0f) {
      return 607;
    }
  }

  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  context.root_child_count = 0;
  SLRustStyleDefault(&context.root.style);
  if (SLRustLayoutExternalChecked(NULL, &callbacks, 1, constraints, &out_size) !=
      SLRustStatusNullPointer) {
    return 9;
  }
  SLRustAbiInfo mismatched_abi_info = caller_abi;
  mismatched_abi_info.size_of_style += 1;
  if (SLRustLayoutExternalChecked(&mismatched_abi_info, &callbacks, 1,
                                  constraints, &out_size) !=
      SLRustStatusAbiMismatch) {
    return 10;
  }

  SLRustStyleDefault(&context.root.style);
  context.root.has_layout = false;
  context.root.has_measure = true;
  context.root.measure_called = false;
  context.root.measured_size.width = 120.0f;
  context.root.measured_size.height = 5.0f;
  callbacks.has_measure = has_measure;
  callbacks.measure = measure;
  SLRustConstraints root_constraints = SLRustMakeConstraints(
      SLRustMakeDefiniteConstraint(40.0f), SLRustMakeIndefiniteConstraint());

  status = SLRustLayoutExternalWithOwnerConstraints(&callbacks, 1,
                                                    root_constraints, &out_size);
  if (status != SLRustStatusOk || out_size.width != 120.0f ||
      !context.root.measure_called ||
      context.root.last_measure_constraints.width.mode !=
          SLRustMeasureModeAtMost) {
    return 11;
  }

  context.root.has_layout = false;
  context.root.measure_called = false;
  status = SLRustLayoutExternalWithNodeConstraintsChecked(
      &caller_abi, &callbacks, 1, root_constraints, &out_size);
  if (status != SLRustStatusOk || out_size.width != 40.0f ||
      !context.root.measure_called ||
      context.root.last_measure_constraints.width.mode !=
          SLRustMeasureModeDefinite) {
    return 12;
  }
  return 0;
}
"#;

const CPP_LINK_SMOKE: &str = r#"
#include "starlight_rust_ffi.h"
#include "starlight_rust_ffi_cpp.h"

#include <cstddef>
#include <cstring>

struct TestNode {
  SLRustStyle style{};
  SLRustLayoutResult layout{};
  bool has_layout = false;
};

struct TestContext {
  TestNode root;
  TestNode child;
  TestNode second_child;
  size_t root_child_count = 1;
  bool measure_called = false;
  bool baseline_called = false;
  bool child_has_measure = true;
  bool second_child_has_measure = false;
  bool root_leaf_mode = false;
  bool root_has_measure = false;
  bool root_measure_called = false;
  bool second_measure_called = false;
  bool child_has_second_child = false;
  SLRustSize child_measured_size{24.0f, 8.0f};
  SLRustSize second_child_measured_size{};
  SLRustSize root_measured_size{};
  SLRustConstraints root_last_measure_constraints{};
  SLRustConstraints last_measure_constraints{};
  SLRustConstraints second_last_measure_constraints{};
  SLRustSize last_baseline_content_size{};
};

struct AdapterSmokeTree {
  SLRustStyle style{};
  SLRustLayoutResult layout{};
  SLRustConstraints layout_constraints{};
  bool has_layout = false;
  bool has_layout_constraints = false;

  size_t ChildCount(SLRustNodeId node) {
    (void)node;
    return 0;
  }

  SLRustNodeId ChildAt(SLRustNodeId node, size_t index) {
    (void)node;
    (void)index;
    return 0;
  }

  bool Style(SLRustNodeId node, SLRustStyle* out_style) {
    if (node != 9 || out_style == nullptr) {
      return false;
    }
    *out_style = style;
    return true;
  }

  void SetLayout(SLRustNodeId node, SLRustLayoutResult result) {
    if (node == 9) {
      layout = result;
      has_layout = true;
    }
  }

  void SetLayoutWithConstraints(SLRustNodeId node,
                                SLRustConstraints constraints,
                                SLRustLayoutResult result) {
    if (node == 9) {
      layout = result;
      layout_constraints = constraints;
      has_layout = true;
      has_layout_constraints = true;
    }
  }
};

struct PixelRatioAdapterSmokeTree {
  SLRustLayoutResult layout{};
  bool has_layout = false;

  size_t ChildCount(SLRustNodeId node) {
    (void)node;
    return 0;
  }

  SLRustNodeId ChildAt(SLRustNodeId node, size_t index) {
    (void)node;
    (void)index;
    return 0;
  }

  bool Style(SLRustNodeId node, SLRustStyle* out_style) {
    if (node != 7 || out_style == nullptr) {
      return false;
    }
    SLRustStyleDefault(out_style);
    return true;
  }

  void SetLayout(SLRustNodeId node, SLRustLayoutResult result) {
    if (node == 7) {
      layout = result;
      has_layout = true;
    }
  }

  bool HasMeasure(SLRustNodeId node) {
    return node == 7;
  }

  bool Measure(SLRustNodeId node, SLRustConstraints constraints,
               SLRustSize* out_size) {
    (void)constraints;
    if (node != 7 || out_size == nullptr) {
      return false;
    }
    *out_size = SLRustMakeSize(10.2f, 4.2f);
    return true;
  }

  bool PhysicalPixelsPerLayoutUnit(SLRustNodeId node, float* out_value) {
    if (node != 7 || out_value == nullptr) {
      return false;
    }
    *out_value = 2.0f;
    return true;
  }
};

struct GridAdapterSmokeTree {
  SLRustLength auto_columns[2] = {
      SLRustMakePointsLength(20.0f),
      SLRustMakePointsLength(10.0f),
  };
  SLRustLength auto_columns_max[2] = {
      SLRustMakeFitContentLengthWithBase(40.0f, 0.0f, false),
      SLRustMakePointsLength(10.0f),
  };
  SLRustLength auto_rows[2] = {
      SLRustMakePointsLength(20.0f),
      SLRustMakePointsLength(10.0f),
  };
  SLRustLength auto_rows_max[2] = {
      SLRustMakeFitContentLengthWithBase(40.0f, 0.0f, false),
      SLRustMakePointsLength(10.0f),
  };
  SLRustLayoutResult root_layout{};
  SLRustLayoutResult child_layout{};
  SLRustLayoutResult second_child_layout{};
  SLRustConstraints root_layout_constraints{};
  bool root_has_layout = false;
  bool root_has_layout_constraints = false;
  bool child_has_layout = false;
  bool second_child_has_layout = false;
  bool measure_called = false;

  size_t ChildCount(SLRustNodeId node) {
    return node == 1 ? 2 : 0;
  }

  SLRustNodeId ChildAt(SLRustNodeId node, size_t index) {
    if (node != 1 || index >= 2) {
      return 0;
    }
    return index == 0 ? 2 : 3;
  }

  bool Style(SLRustNodeId node, SLRustStyle* out_style) {
    if (out_style == nullptr) {
      return false;
    }
    SLRustStyleDefault(out_style);
    if (node == 1) {
      out_style->display = SLRustDisplayGrid;
      out_style->grid_auto_columns = auto_columns;
      out_style->grid_auto_columns_len = 2;
      out_style->grid_auto_columns_max = auto_columns_max;
      out_style->grid_auto_columns_max_len = 2;
      out_style->grid_auto_rows = auto_rows;
      out_style->grid_auto_rows_len = 2;
      out_style->grid_auto_rows_max = auto_rows_max;
      out_style->grid_auto_rows_max_len = 2;
      return true;
    }
    if (node == 2) {
      out_style->grid_column_start = 1;
      out_style->grid_row_start = 1;
      return true;
    }
    if (node == 3) {
      out_style->grid_column_start = 2;
      out_style->grid_row_start = 2;
      return true;
    }
    return false;
  }

  void SetLayout(SLRustNodeId node, SLRustLayoutResult result) {
    if (node == 1) {
      root_layout = result;
      root_has_layout = true;
    } else if (node == 2) {
      child_layout = result;
      child_has_layout = true;
    } else if (node == 3) {
      second_child_layout = result;
      second_child_has_layout = true;
    }
  }

  void SetLayoutWithConstraints(SLRustNodeId node,
                                SLRustConstraints constraints,
                                SLRustLayoutResult result) {
    SetLayout(node, result);
    if (node == 1) {
      root_layout_constraints = constraints;
      root_has_layout_constraints = true;
    }
  }

  bool HasMeasure(SLRustNodeId node) {
    return node == 2;
  }

  bool Measure(SLRustNodeId node, SLRustConstraints constraints,
               SLRustSize* out_size) {
    (void)constraints;
    if (node != 2 || out_size == nullptr) {
      return false;
    }
    measure_called = true;
    *out_size = SLRustMakeSize(70.0f, 70.0f);
    return true;
  }
};

struct FullValueEdgeAdapterSmokeTree {
  SLRustLength edge_lengths[3] = {
      SLRustMakeMaxContentLength(),
      SLRustMakeFitContentLengthWithBase(4.0f, 0.0f, false),
      SLRustMakeFrLength(1.0f),
  };
  float expected_edges[3] = {0.0f, 4.0f, 1.0f};
  size_t active_edge_index = 0;
  SLRustLayoutResult root_layout{};
  SLRustLayoutResult child_layout{};
  SLRustConstraints root_layout_constraints{};
  bool root_has_layout = false;
  bool root_has_layout_constraints = false;
  bool child_has_layout = false;

  void Reset(size_t edge_index) {
    active_edge_index = edge_index;
    root_layout = {};
    child_layout = {};
    root_layout_constraints = {};
    root_has_layout = false;
    root_has_layout_constraints = false;
    child_has_layout = false;
  }

  float ExpectedEdge() const {
    return expected_edges[active_edge_index];
  }

  SLRustLength ActiveEdgeLength() const {
    return edge_lengths[active_edge_index];
  }

  size_t ChildCount(SLRustNodeId node) {
    return node == 1 ? 1 : 0;
  }

  SLRustNodeId ChildAt(SLRustNodeId node, size_t index) {
    if (node != 1 || index != 0) {
      return 0;
    }
    return 2;
  }

  bool Style(SLRustNodeId node, SLRustStyle* out_style) {
    if (out_style == nullptr) {
      return false;
    }
    SLRustStyleDefault(out_style);
    if (node == 1) {
      out_style->display = SLRustDisplayFlex;
      out_style->width = SLRustMakePointsLength(80.0f);
      out_style->height = SLRustMakePointsLength(20.0f);
      out_style->align_items = SLRustAlignItemsFlexStart;
      return true;
    }
    if (node == 2) {
      const SLRustLength edge = ActiveEdgeLength();
      out_style->position = SLRustPositionRelative;
      out_style->left = edge;
      out_style->margin.left = edge;
      out_style->padding.left = edge;
      out_style->flex_basis = SLRustMakePointsLength(10.0f);
      out_style->height = SLRustMakePointsLength(6.0f);
      out_style->box_sizing = SLRustBoxSizingContentBox;
      return true;
    }
    return false;
  }

  void SetLayout(SLRustNodeId node, SLRustLayoutResult result) {
    if (node == 1) {
      root_layout = result;
      root_has_layout = true;
    } else if (node == 2) {
      child_layout = result;
      child_has_layout = true;
    }
  }

  void SetLayoutWithConstraints(SLRustNodeId node,
                                SLRustConstraints constraints,
                                SLRustLayoutResult result) {
    SetLayout(node, result);
    if (node == 1) {
      root_layout_constraints = constraints;
      root_has_layout_constraints = true;
    }
  }
};

extern "C" size_t cpp_link_child_count(void* context, SLRustNodeId node) {
  auto* test_context = static_cast<TestContext*>(context);
  if (test_context->root_leaf_mode) {
    return 0;
  }
  if (node == 1) {
    return test_context->root_child_count;
  }
  return node == 2 && test_context->child_has_second_child ? 1 : 0;
}

extern "C" SLRustNodeId cpp_link_child_at(void* context, SLRustNodeId node,
                                          size_t index) {
  auto* test_context = static_cast<TestContext*>(context);
  if (node == 2 && test_context->child_has_second_child && index == 0) {
    return 3;
  }
  if (node != 1 || index >= test_context->root_child_count) {
    return 0;
  }
  return index == 0 ? 2 : 3;
}

extern "C" bool cpp_link_style_callback(void* context, SLRustNodeId node,
                                         SLRustStyle* out_style) {
  auto* test_context = static_cast<TestContext*>(context);
  if (out_style == nullptr) {
    return false;
  }
  if (node == 1) {
    *out_style = test_context->root.style;
    return true;
  }
  if (node == 2) {
    *out_style = test_context->child.style;
    return true;
  }
  if (node == 3) {
    *out_style = test_context->second_child.style;
    return true;
  }
  return false;
}

extern "C" bool cpp_link_invalid_style_callback(void* context, SLRustNodeId node,
                                                SLRustStyle* out_style) {
  (void)context;
  if (node != 1 || out_style == nullptr) {
    return false;
  }
  SLRustStyleDefault(out_style);
  out_style->display = 999;
  return true;
}

extern "C" bool cpp_link_has_measure(void* context, SLRustNodeId node) {
  auto* test_context = static_cast<TestContext*>(context);
  if (node == 1) {
    return test_context->root_has_measure;
  }
  if (node == 3) {
    return test_context->second_child_has_measure;
  }
  return node == 2 && test_context->child_has_measure;
}

extern "C" bool cpp_link_measure(void* context, SLRustNodeId node,
                                 SLRustConstraints constraints,
                                 SLRustSize* out_size) {
  auto* test_context = static_cast<TestContext*>(context);
  if (node == 1 && test_context->root_has_measure && out_size != nullptr) {
    test_context->root_measure_called = true;
    test_context->root_last_measure_constraints = constraints;
    *out_size = test_context->root_measured_size;
    return true;
  }
  if (node == 3 && test_context->second_child_has_measure &&
      out_size != nullptr) {
    test_context->second_measure_called = true;
    test_context->second_last_measure_constraints = constraints;
    *out_size = test_context->second_child_measured_size;
    return true;
  }
  if (node != 2 || out_size == nullptr) {
    return false;
  }
  test_context->measure_called = true;
  test_context->last_measure_constraints = constraints;
  *out_size = test_context->child_measured_size;
  return true;
}

extern "C" bool cpp_link_baseline(void* context, SLRustNodeId node,
                                  SLRustSize content_size,
                                  float* out_baseline) {
  auto* test_context = static_cast<TestContext*>(context);
  if (node != 2 || out_baseline == nullptr) {
    return false;
  }
  test_context->baseline_called = true;
  test_context->last_baseline_content_size = content_size;
  *out_baseline = 6.0f;
  return true;
}

extern "C" void cpp_link_set_layout(void* context, SLRustNodeId node,
                                    SLRustLayoutResult layout) {
  auto* test_context = static_cast<TestContext*>(context);
  if (node == 1) {
    test_context->root.layout = layout;
    test_context->root.has_layout = true;
  } else if (node == 2) {
    test_context->child.layout = layout;
    test_context->child.has_layout = true;
  } else if (node == 3) {
    test_context->second_child.layout = layout;
    test_context->second_child.has_layout = true;
  }
}

int main() {
  TestContext context;
  SLRustAbiInfo caller_abi = SLRustMakeCallerAbiInfo();
  SLRustAbiInfo library_abi{};
  if (SLRustGetAbiInfo(&library_abi) != SLRustStatusOk) {
    return 17;
  }
  if (caller_abi.version_major != SLRustAbiVersionMajor ||
      caller_abi.version_minor != SLRustAbiVersionMinor ||
      caller_abi.version_patch != SLRustAbiVersionPatch) {
    return 18;
  }
  if (caller_abi.size_of_abi_info != sizeof(SLRustAbiInfo) ||
      caller_abi.align_of_abi_info != alignof(SLRustAbiInfo) ||
      caller_abi.size_of_length != sizeof(SLRustLength) ||
      caller_abi.align_of_length != alignof(SLRustLength) ||
      caller_abi.size_of_constraints != sizeof(SLRustConstraints) ||
      caller_abi.align_of_constraints != alignof(SLRustConstraints) ||
      caller_abi.size_of_layout_result != sizeof(SLRustLayoutResult) ||
      caller_abi.align_of_layout_result != alignof(SLRustLayoutResult) ||
      caller_abi.size_of_style != sizeof(SLRustStyle) ||
      caller_abi.align_of_style != alignof(SLRustStyle) ||
      caller_abi.size_of_tree_callbacks != sizeof(SLRustTreeCallbacks) ||
      caller_abi.align_of_tree_callbacks != alignof(SLRustTreeCallbacks)) {
    return 19;
  }
  if (library_abi.size_of_abi_info != caller_abi.size_of_abi_info ||
      library_abi.align_of_abi_info != caller_abi.align_of_abi_info ||
      library_abi.size_of_length != caller_abi.size_of_length ||
      library_abi.align_of_length != caller_abi.align_of_length ||
      library_abi.size_of_constraints != caller_abi.size_of_constraints ||
      library_abi.align_of_constraints != caller_abi.align_of_constraints ||
      library_abi.size_of_layout_result != caller_abi.size_of_layout_result ||
      library_abi.align_of_layout_result != caller_abi.align_of_layout_result ||
      library_abi.size_of_style != caller_abi.size_of_style ||
      library_abi.align_of_style != caller_abi.align_of_style ||
      library_abi.size_of_tree_callbacks != caller_abi.size_of_tree_callbacks ||
      library_abi.align_of_tree_callbacks != caller_abi.align_of_tree_callbacks) {
    return 19;
  }
  if (SLRustGetAbiInfo(nullptr) != SLRustStatusNullPointer) {
    return 20;
  }
  if (std::strcmp(SLRustStatusName(SLRustStatusOk), "Ok") != 0 ||
      std::strcmp(SLRustStatusName(SLRustStatusInvalidStyle),
                  "InvalidStyle") != 0 ||
      std::strcmp(SLRustStatusName(SLRustStatusFixedNodeSetMismatch),
                  "FixedNodeSetMismatch") != 0 ||
      std::strcmp(SLRustStatusName(static_cast<SLRustStatus>(999)),
                  "Unknown") != 0) {
    return 505;
  }

  AdapterSmokeTree adapter_tree;
  SLRustStyleDefault(&adapter_tree.style);
  adapter_tree.style.display = SLRustDisplayFlex;
  adapter_tree.style.width = SLRustMakePointsLength(33.0f);
  adapter_tree.style.height = SLRustMakePointsLength(17.0f);
  SLRustTreeCallbacks adapter_callbacks =
      lynx::starlight::rust_ffi::MakeTreeCallbacks(&adapter_tree);
  if (adapter_callbacks.has_measure != nullptr ||
      adapter_callbacks.measure != nullptr ||
      adapter_callbacks.baseline != nullptr ||
      adapter_callbacks.physical_pixels_per_layout_unit != nullptr ||
      adapter_callbacks.set_layout_with_constraints == nullptr) {
    return 110;
  }
  SLRustSize adapter_out = SLRustMakeSize(0.0f, 0.0f);
  SLRustStatus adapter_status =
      lynx::starlight::rust_ffi::LayoutExternalWithOwnerConstraintsChecked(
          &adapter_tree, 9,
          SLRustMakeConstraints(SLRustMakeDefiniteConstraint(33.0f),
                                SLRustMakeDefiniteConstraint(17.0f)),
          &adapter_out);
  if (adapter_status != SLRustStatusOk || !adapter_tree.has_layout ||
      !adapter_tree.has_layout_constraints ||
      adapter_tree.layout_constraints.width.mode != SLRustMeasureModeDefinite ||
      adapter_tree.layout_constraints.width.size != 33.0f ||
      adapter_out.width != 33.0f || adapter_out.height != 17.0f) {
    return 108;
  }
  if (lynx::starlight::rust_ffi::LayoutExternalChecked<AdapterSmokeTree>(
          nullptr, 9,
          SLRustMakeConstraints(SLRustMakeDefiniteConstraint(1.0f),
                                SLRustMakeDefiniteConstraint(1.0f)),
          &adapter_out) != SLRustStatusNullPointer) {
    return 109;
  }

  PixelRatioAdapterSmokeTree pixel_adapter_tree;
  SLRustTreeCallbacks pixel_adapter_callbacks =
      lynx::starlight::rust_ffi::MakeTreeCallbacks(&pixel_adapter_tree);
  if (pixel_adapter_callbacks.has_measure == nullptr ||
      pixel_adapter_callbacks.measure == nullptr ||
      pixel_adapter_callbacks.physical_pixels_per_layout_unit == nullptr ||
      pixel_adapter_callbacks.baseline != nullptr ||
      pixel_adapter_callbacks.set_layout_with_constraints != nullptr) {
    return 135;
  }
  SLRustSize pixel_adapter_out = SLRustMakeSize(0.0f, 0.0f);
  SLRustStatus pixel_adapter_status =
      lynx::starlight::rust_ffi::LayoutExternalChecked(
          &pixel_adapter_tree, 7,
          SLRustMakeConstraints(SLRustMakeIndefiniteConstraint(),
                                SLRustMakeIndefiniteConstraint()),
          &pixel_adapter_out);
  if (pixel_adapter_status != SLRustStatusOk || !pixel_adapter_tree.has_layout ||
      pixel_adapter_out.width != 10.5f ||
      pixel_adapter_out.height != 4.5f ||
      pixel_adapter_tree.layout.size.width != 10.5f ||
      pixel_adapter_tree.layout.size.height != 4.5f) {
    return 136;
  }

  GridAdapterSmokeTree grid_adapter_tree;
  SLRustTreeCallbacks grid_adapter_callbacks =
      lynx::starlight::rust_ffi::MakeTreeCallbacks(&grid_adapter_tree);
  if (grid_adapter_callbacks.has_measure == nullptr ||
      grid_adapter_callbacks.measure == nullptr ||
      grid_adapter_callbacks.baseline != nullptr ||
      grid_adapter_callbacks.set_layout_with_constraints == nullptr) {
    return 113;
  }
  SLRustSize grid_adapter_out = SLRustMakeSize(0.0f, 0.0f);
  SLRustStatus grid_adapter_status =
      lynx::starlight::rust_ffi::LayoutExternalWithOwnerConstraintsChecked(
          &grid_adapter_tree, 1,
          SLRustMakeConstraints(SLRustMakeIndefiniteConstraint(),
                                SLRustMakeIndefiniteConstraint()),
          &grid_adapter_out);
  if (grid_adapter_status != SLRustStatusOk) {
    return 114;
  }
  if (!grid_adapter_tree.measure_called ||
      !grid_adapter_tree.root_has_layout ||
      !grid_adapter_tree.root_has_layout_constraints ||
      !grid_adapter_tree.child_has_layout ||
      !grid_adapter_tree.second_child_has_layout ||
      grid_adapter_out.width != 50.0f ||
      grid_adapter_out.height != 50.0f ||
      grid_adapter_tree.root_layout.size.width != 50.0f ||
      grid_adapter_tree.root_layout.size.height != 50.0f) {
    return 115;
  }
  if (grid_adapter_tree.root_layout_constraints.width.mode !=
          SLRustMeasureModeIndefinite ||
      grid_adapter_tree.root_layout_constraints.height.mode !=
          SLRustMeasureModeIndefinite) {
    return 116;
  }
  if (grid_adapter_tree.child_layout.size.width != 40.0f ||
      grid_adapter_tree.child_layout.size.height != 40.0f ||
      grid_adapter_tree.second_child_layout.offset.x != 40.0f ||
      grid_adapter_tree.second_child_layout.offset.y != 40.0f ||
      grid_adapter_tree.second_child_layout.size.width != 10.0f ||
      grid_adapter_tree.second_child_layout.size.height != 10.0f) {
    return 117;
  }

  FullValueEdgeAdapterSmokeTree edge_adapter_tree;
  SLRustTreeCallbacks edge_adapter_callbacks =
      lynx::starlight::rust_ffi::MakeTreeCallbacks(&edge_adapter_tree);
  if (edge_adapter_callbacks.has_measure != nullptr ||
      edge_adapter_callbacks.measure != nullptr ||
      edge_adapter_callbacks.baseline != nullptr ||
      edge_adapter_callbacks.set_layout_with_constraints == nullptr) {
    return 132;
  }
  for (size_t edge_index = 0; edge_index < 3; edge_index++) {
    edge_adapter_tree.Reset(edge_index);
    SLRustSize edge_adapter_out = SLRustMakeSize(0.0f, 0.0f);
    SLRustStatus edge_adapter_status =
        lynx::starlight::rust_ffi::LayoutExternalWithOwnerConstraintsChecked(
            &edge_adapter_tree, 1,
            SLRustMakeConstraints(SLRustMakeDefiniteConstraint(80.0f),
                                  SLRustMakeDefiniteConstraint(20.0f)),
            &edge_adapter_out);
    if (edge_adapter_status != SLRustStatusOk) {
      return 133;
    }
    const float expected_edge = edge_adapter_tree.ExpectedEdge();
    if (!edge_adapter_tree.root_has_layout ||
        !edge_adapter_tree.root_has_layout_constraints ||
        !edge_adapter_tree.child_has_layout ||
        edge_adapter_out.width != 80.0f ||
        edge_adapter_out.height != 20.0f ||
        edge_adapter_tree.root_layout.size.width != 80.0f ||
        edge_adapter_tree.root_layout.size.height != 20.0f ||
        edge_adapter_tree.root_layout_constraints.width.mode !=
            SLRustMeasureModeDefinite ||
        edge_adapter_tree.root_layout_constraints.width.size != 80.0f ||
        edge_adapter_tree.child_layout.padding.left != expected_edge ||
        edge_adapter_tree.child_layout.margin.left != expected_edge ||
        edge_adapter_tree.child_layout.offset.x != expected_edge * 2.0f) {
      return 134;
    }
  }

  const SLRustLength helper_fit_content =
      SLRustMakeFitContentLengthWithBase(8.0f, 25.0f, true);
  const SLRustLength helper_percent = SLRustMakePercentLength(50.0f);
  const SLRustLength helper_max_content = SLRustMakeMaxContentLength();
  const SLRustPoint helper_point = SLRustMakePoint(3.0f, 4.0f);
  const SLRustRectF32 helper_rect = SLRustMakeRectF32(1.0f, 2.0f, 3.0f, 4.0f);
  if (helper_fit_content.kind != SLRustLengthFitContent ||
      helper_fit_content.value != 8.0f ||
      helper_fit_content.percent != 25.0f ||
      !helper_fit_content.has_base ||
      !helper_fit_content.has_percentage ||
      helper_percent.kind != SLRustLengthPercent ||
      helper_percent.value != 50.0f ||
      helper_max_content.kind != SLRustLengthMaxContent ||
      helper_point.x != 3.0f || helper_point.y != 4.0f ||
      helper_rect.left != 1.0f || helper_rect.right != 2.0f ||
      helper_rect.top != 3.0f || helper_rect.bottom != 4.0f) {
    return 107;
  }

  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  context.root.style.display = SLRustDisplayFlex;
  context.root.style.width = SLRustMakePointsLength(96.0f);
  context.root.style.height = SLRustMakePointsLength(32.0f);
  context.root.style.align_items = SLRustAlignItemsFlexStart;

  SLRustTreeCallbacks callbacks{};
  callbacks.context = &context;
  callbacks.child_count = cpp_link_child_count;
  callbacks.child_at = cpp_link_child_at;
  callbacks.style = cpp_link_style_callback;
  callbacks.set_layout = cpp_link_set_layout;
  callbacks.has_measure = cpp_link_has_measure;
  callbacks.measure = cpp_link_measure;
  callbacks.baseline = cpp_link_baseline;

  SLRustConstraints constraints = SLRustMakeConstraints(
      SLRustMakeDefiniteConstraint(96.0f),
      SLRustMakeDefiniteConstraint(32.0f));
  SLRustSize out_size = SLRustMakeSize(0.0f, 0.0f);
  const SLRustStatus status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(&caller_abi, &callbacks, 1,
                                                      constraints, &out_size);

  if (status != SLRustStatusOk) {
    return 1;
  }
  if (out_size.width != 96.0f || out_size.height != 32.0f) {
    return 2;
  }
  if (!context.root.has_layout) {
    return 3;
  }
  if (context.root.layout.size.width != 96.0f ||
      context.root.layout.size.height != 32.0f) {
    return 4;
  }
  if (!context.child.has_layout) {
    return 5;
  }
  if (context.child.layout.size.width != 24.0f ||
      context.child.layout.size.height != 8.0f) {
    return 6;
  }
  if (context.child.layout.offset.x != 0.0f ||
      context.child.layout.offset.y != 0.0f) {
    return 7;
  }
  if (!context.measure_called) {
    return 8;
  }
  if (context.last_measure_constraints.width.mode != SLRustMeasureModeDefinite) {
    return 12;
  }
  if (context.last_measure_constraints.width.size != 24.0f) {
    return 13;
  }
  if (context.last_measure_constraints.height.mode != SLRustMeasureModeAtMost ||
      context.last_measure_constraints.height.size != 32.0f) {
    return 14;
  }
  if (!context.baseline_called) {
    return 9;
  }
  if (context.last_baseline_content_size.width != 24.0f ||
      context.last_baseline_content_size.height != 8.0f) {
    return 10;
  }
  if (!context.child.layout.has_baseline ||
      context.child.layout.baseline != 6.0f) {
    return 11;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = false;
  context.second_child_has_measure = false;
  context.root_child_count = 2;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  context.root.style.display = SLRustDisplayFlex;
  context.root.style.width = SLRustMakePointsLength(120.0f);
  context.root.style.height = SLRustMakePointsLength(30.0f);
  context.root.style.align_items = SLRustAlignItemsFlexStart;
  context.root.style.justify_content = SLRustJustifyContentFlexStart;
  context.root.style.column_gap =
      SLRustMakeFitContentLengthWithBase(12.0f, 0.0f, false);
  context.child.style.flex_basis = SLRustMakePointsLength(20.0f);
  context.child.style.height = SLRustMakePointsLength(10.0f);
  context.second_child.style.flex_basis = SLRustMakePointsLength(18.0f);
  context.second_child.style.height = SLRustMakePointsLength(12.0f);
  SLRustConstraints full_value_column_gap_constraints =
      SLRustMakeConstraints(SLRustMakeDefiniteConstraint(120.0f),
                            SLRustMakeDefiniteConstraint(30.0f));
  SLRustStatus full_value_column_gap_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, full_value_column_gap_constraints,
          &out_size);
  if (full_value_column_gap_status != SLRustStatusOk) {
    return 501;
  }
  if (context.second_child.layout.offset.x != 32.0f ||
      context.second_child.layout.offset.y != 0.0f) {
    return 502;
  }

  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  context.root.style.display = SLRustDisplayFlex;
  context.root.style.width = SLRustMakePointsLength(30.0f);
  context.root.style.height = SLRustMakePointsLength(80.0f);
  context.root.style.flex_wrap = SLRustFlexWrapWrap;
  context.root.style.align_items = SLRustAlignItemsFlexStart;
  context.root.style.align_content = SLRustAlignContentFlexStart;
  context.root.style.row_gap = SLRustMakeFrLength(1.0f);
  context.child.style.flex_basis = SLRustMakePointsLength(20.0f);
  context.child.style.height = SLRustMakePointsLength(10.0f);
  context.second_child.style.flex_basis = SLRustMakePointsLength(20.0f);
  context.second_child.style.height = SLRustMakePointsLength(10.0f);
  SLRustConstraints full_value_row_gap_constraints =
      SLRustMakeConstraints(SLRustMakeDefiniteConstraint(30.0f),
                            SLRustMakeDefiniteConstraint(80.0f));
  SLRustStatus full_value_row_gap_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, full_value_row_gap_constraints,
          &out_size);
  if (full_value_row_gap_status != SLRustStatusOk) {
    return 503;
  }
  if (context.second_child.layout.offset.x != 0.0f ||
      context.second_child.layout.offset.y != 11.0f) {
    return 504;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  context.child_has_measure = true;
  context.root_child_count = 1;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  context.root.style.display = SLRustDisplayLinear;
  context.root.style.linear_orientation = SLRustLinearOrientationHorizontal;
  context.root.style.align_items = SLRustAlignItemsFlexStart;
  context.root.style.width.kind = SLRustLengthPoints;
  context.root.style.width.value = 96.0f;
  context.root.style.height.kind = SLRustLengthPoints;
  context.root.style.height.value = 32.0f;
  context.child.style.margin.top.kind = SLRustLengthAuto;
  SLRustStatus linear_start_auto_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(&caller_abi, &callbacks, 1,
                                                      constraints, &out_size);
  if (linear_start_auto_status != SLRustStatusOk) {
    return 25;
  }
  if (!context.root.layout.has_baseline ||
      context.root.layout.baseline != 6.0f) {
    return 26;
  }
  if (context.child.layout.offset.y != 24.0f ||
      context.child.layout.margin.top != 24.0f ||
      context.child.layout.margin.bottom != 0.0f) {
    return 27;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.root.has_layout = false;
  context.child.has_layout = false;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  context.root.style.display = SLRustDisplayLinear;
  context.root.style.linear_orientation = SLRustLinearOrientationHorizontal;
  context.root.style.linear_cross_gravity = SLRustLinearCrossGravityEnd;
  context.root.style.width.kind = SLRustLengthPoints;
  context.root.style.width.value = 96.0f;
  context.root.style.height.kind = SLRustLengthPoints;
  context.root.style.height.value = 32.0f;
  context.child.style.margin.top.kind = SLRustLengthAuto;
  context.child.style.margin.bottom.kind = SLRustLengthAuto;
  SLRustStatus linear_paired_auto_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(&caller_abi, &callbacks, 1,
                                                      constraints, &out_size);
  if (linear_paired_auto_status != SLRustStatusOk) {
    return 28;
  }
  if (!context.root.layout.has_baseline ||
      context.root.layout.baseline != 30.0f) {
    return 29;
  }
  if (context.child.layout.offset.y != 12.0f ||
      context.child.layout.margin.top != 12.0f ||
      context.child.layout.margin.bottom != 12.0f) {
    return 30;
  }

  context.child_measured_size = SLRustSize{24.0f, 8.0f};
  context.second_child_measured_size = SLRustSize{};
  context.second_child_has_measure = false;

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = false;
  context.root_child_count = 2;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  SLRustLength grid_columns[2] = {
      {SLRustLengthPoints, 30.0f, 0.0f, false, false},
      {SLRustLengthPoints, 40.0f, 0.0f, false, false},
  };
  SLRustLength grid_rows[1] = {
      {SLRustLengthPoints, 10.0f, 0.0f, false, false},
  };
  context.root.style.display = SLRustDisplayGrid;
  context.root.style.width.kind = SLRustLengthPoints;
  context.root.style.width.value = 100.0f;
  context.root.style.height.kind = SLRustLengthPoints;
  context.root.style.height.value = 20.0f;
  context.root.style.grid_template_columns = grid_columns;
  context.root.style.grid_template_columns_len = 2;
  context.root.style.grid_template_rows = grid_rows;
  context.root.style.grid_template_rows_len = 1;
  SLRustConstraints grid_constraints{
      {100.0f, SLRustMeasureModeDefinite},
      {20.0f, SLRustMeasureModeDefinite},
  };
  SLRustStatus grid_track_vector_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, grid_constraints, &out_size);
  if (grid_track_vector_status != SLRustStatusOk) {
    return 33;
  }
  if (out_size.width != 100.0f || out_size.height != 20.0f) {
    return 34;
  }
  if (context.child.layout.offset.x != 0.0f ||
      context.child.layout.size.width != 30.0f ||
      context.child.layout.size.height != 10.0f ||
      context.second_child.layout.offset.x != 30.0f ||
      context.second_child.layout.size.width != 40.0f ||
      context.second_child.layout.size.height != 10.0f) {
    return 35;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = false;
  context.root_child_count = 2;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  SLRustLength grid_calc_columns[2] = {
      {SLRustLengthCalc, 10.0f, 25.0f, true, true},
      {SLRustLengthPercent, 50.0f, 0.0f, false, false},
  };
  SLRustLength grid_calc_rows[1] = {
      {SLRustLengthPoints, 10.0f, 0.0f, false, false},
  };
  context.root.style.display = SLRustDisplayGrid;
  context.root.style.width.kind = SLRustLengthPoints;
  context.root.style.width.value = 100.0f;
  context.root.style.height.kind = SLRustLengthPoints;
  context.root.style.height.value = 10.0f;
  context.root.style.grid_template_columns = grid_calc_columns;
  context.root.style.grid_template_columns_len = 2;
  context.root.style.grid_template_rows = grid_calc_rows;
  context.root.style.grid_template_rows_len = 1;
  SLRustConstraints grid_calc_constraints{
      {100.0f, SLRustMeasureModeDefinite},
      {10.0f, SLRustMeasureModeDefinite},
  };
  SLRustStatus grid_calc_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, grid_calc_constraints, &out_size);
  if (grid_calc_status != SLRustStatusOk) {
    return 80;
  }
  if (out_size.width != 100.0f || out_size.height != 10.0f) {
    return 81;
  }
  if (context.child.layout.offset.x != 0.0f ||
      context.child.layout.size.width != 35.0f ||
      context.child.layout.size.height != 10.0f ||
      context.second_child.layout.offset.x != 35.0f ||
      context.second_child.layout.size.width != 50.0f ||
      context.second_child.layout.size.height != 10.0f) {
    return 82;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = false;
  context.root_child_count = 2;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  SLRustLength grid_fr_columns[2] = {
      {SLRustLengthFr, 1.0f, 0.0f, false, false},
      {SLRustLengthFr, 2.0f, 0.0f, false, false},
  };
  SLRustLength grid_fr_rows[1] = {
      {SLRustLengthPoints, 10.0f, 0.0f, false, false},
  };
  context.root.style.display = SLRustDisplayGrid;
  context.root.style.width.kind = SLRustLengthPoints;
  context.root.style.width.value = 90.0f;
  context.root.style.height.kind = SLRustLengthPoints;
  context.root.style.height.value = 10.0f;
  context.root.style.grid_template_columns = grid_fr_columns;
  context.root.style.grid_template_columns_len = 2;
  context.root.style.grid_template_rows = grid_fr_rows;
  context.root.style.grid_template_rows_len = 1;
  SLRustConstraints grid_fr_constraints{
      {90.0f, SLRustMeasureModeDefinite},
      {10.0f, SLRustMeasureModeDefinite},
  };
  SLRustStatus grid_fr_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, grid_fr_constraints, &out_size);
  if (grid_fr_status != SLRustStatusOk) {
    return 74;
  }
  if (out_size.width != 90.0f || out_size.height != 10.0f) {
    return 75;
  }
  if (context.child.layout.offset.x != 0.0f ||
      context.child.layout.size.width != 30.0f ||
      context.child.layout.size.height != 10.0f ||
      context.second_child.layout.offset.x != 30.0f ||
      context.second_child.layout.size.width != 60.0f ||
      context.second_child.layout.size.height != 10.0f) {
    return 76;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = true;
  context.root_child_count = 1;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  SLRustLength grid_max_content_columns[1] = {
      {SLRustLengthMaxContent, 0.0f, 0.0f, false, false},
  };
  SLRustLength grid_max_content_rows[1] = {
      {SLRustLengthMaxContent, 0.0f, 0.0f, false, false},
  };
  context.root.style.display = SLRustDisplayGrid;
  context.root.style.grid_template_columns = grid_max_content_columns;
  context.root.style.grid_template_columns_len = 1;
  context.root.style.grid_template_rows = grid_max_content_rows;
  context.root.style.grid_template_rows_len = 1;
  SLRustConstraints grid_max_content_constraints{
      {0.0f, SLRustMeasureModeIndefinite},
      {0.0f, SLRustMeasureModeIndefinite},
  };
  SLRustStatus grid_max_content_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, grid_max_content_constraints, &out_size);
  if (grid_max_content_status != SLRustStatusOk) {
    return 77;
  }
  if (!context.measure_called || out_size.width != 24.0f ||
      out_size.height != 8.0f) {
    return 78;
  }
  if (context.child.layout.offset.x != 0.0f ||
      context.child.layout.size.width != 24.0f ||
      context.child.layout.size.height != 8.0f) {
    return 79;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = false;
  context.root_child_count = 1;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  SLRustLength grid_fit_content_columns[1] = {
      {SLRustLengthFitContent, 40.0f, 0.0f, true, false},
  };
  SLRustLength grid_fit_content_rows[1] = {
      {SLRustLengthPoints, 10.0f, 0.0f, false, false},
  };
  context.root.style.display = SLRustDisplayGrid;
  context.root.style.grid_template_columns = grid_fit_content_columns;
  context.root.style.grid_template_columns_len = 1;
  context.root.style.grid_template_rows = grid_fit_content_rows;
  context.root.style.grid_template_rows_len = 1;
  context.child.style.width.kind = SLRustLengthPoints;
  context.child.style.width.value = 70.0f;
  context.child.style.height.kind = SLRustLengthPoints;
  context.child.style.height.value = 10.0f;
  SLRustConstraints grid_fit_content_constraints{
      {0.0f, SLRustMeasureModeIndefinite},
      {0.0f, SLRustMeasureModeIndefinite},
  };
  SLRustStatus grid_fit_content_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, grid_fit_content_constraints, &out_size);
  if (grid_fit_content_status != SLRustStatusOk) {
    return 83;
  }
  if (out_size.width != 40.0f || out_size.height != 10.0f) {
    return 84;
  }
  if (context.child.layout.offset.x != 0.0f ||
      context.child.layout.size.width != 70.0f ||
      context.child.layout.size.height != 10.0f) {
    return 85;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = true;
  context.root_child_count = 1;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  context.child_measured_size.width = 90.0f;
  context.child_measured_size.height = 10.0f;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  SLRustLength grid_fit_content_percent_columns[1] = {
      {SLRustLengthFitContent, 0.0f, 50.0f, true, true},
  };
  SLRustLength grid_fit_content_percent_rows[1] = {
      {SLRustLengthPoints, 10.0f, 0.0f, false, false},
  };
  context.root.style.display = SLRustDisplayGrid;
  context.root.style.width.kind = SLRustLengthPoints;
  context.root.style.width.value = 120.0f;
  context.root.style.height.kind = SLRustLengthPoints;
  context.root.style.height.value = 10.0f;
  context.root.style.grid_template_columns = grid_fit_content_percent_columns;
  context.root.style.grid_template_columns_len = 1;
  context.root.style.grid_template_rows = grid_fit_content_percent_rows;
  context.root.style.grid_template_rows_len = 1;
  SLRustConstraints grid_fit_content_percent_constraints{
      {120.0f, SLRustMeasureModeDefinite},
      {10.0f, SLRustMeasureModeDefinite},
  };
  SLRustStatus grid_fit_content_percent_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, grid_fit_content_percent_constraints,
          &out_size);
  if (grid_fit_content_percent_status != SLRustStatusOk) {
    return 86;
  }
  if (!context.measure_called || out_size.width != 120.0f ||
      out_size.height != 10.0f) {
    return 87;
  }
  if (context.child.layout.offset.x != 0.0f ||
      context.child.layout.size.width != 60.0f ||
      context.child.layout.size.height != 10.0f) {
    return 88;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = true;
  context.root_child_count = 1;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  SLRustLength grid_fit_content_calc_columns[1] = {
      {SLRustLengthFitContent, 10.0f, 50.0f, true, true},
  };
  SLRustLength grid_fit_content_calc_rows[1] = {
      {SLRustLengthPoints, 10.0f, 0.0f, false, false},
  };
  context.root.style.display = SLRustDisplayGrid;
  context.root.style.width.kind = SLRustLengthPoints;
  context.root.style.width.value = 120.0f;
  context.root.style.height.kind = SLRustLengthPoints;
  context.root.style.height.value = 10.0f;
  context.root.style.grid_template_columns = grid_fit_content_calc_columns;
  context.root.style.grid_template_columns_len = 1;
  context.root.style.grid_template_rows = grid_fit_content_calc_rows;
  context.root.style.grid_template_rows_len = 1;
  SLRustStatus grid_fit_content_calc_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, grid_fit_content_percent_constraints,
          &out_size);
  if (grid_fit_content_calc_status != SLRustStatusOk) {
    return 89;
  }
  if (!context.measure_called || out_size.width != 120.0f ||
      out_size.height != 10.0f) {
    return 90;
  }
  if (context.child.layout.offset.x != 0.0f ||
      context.child.layout.size.width != 70.0f ||
      context.child.layout.size.height != 10.0f) {
    return 91;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = true;
  context.root_child_count = 1;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  context.child_measured_size.width = 10.0f;
  context.child_measured_size.height = 90.0f;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  SLRustLength grid_fit_content_percent_row_columns[1] = {
      {SLRustLengthPoints, 10.0f, 0.0f, false, false},
  };
  SLRustLength grid_fit_content_percent_row_rows[1] = {
      {SLRustLengthFitContent, 0.0f, 50.0f, true, true},
  };
  context.root.style.display = SLRustDisplayGrid;
  context.root.style.width.kind = SLRustLengthPoints;
  context.root.style.width.value = 10.0f;
  context.root.style.height.kind = SLRustLengthPoints;
  context.root.style.height.value = 120.0f;
  context.root.style.grid_template_columns = grid_fit_content_percent_row_columns;
  context.root.style.grid_template_columns_len = 1;
  context.root.style.grid_template_rows = grid_fit_content_percent_row_rows;
  context.root.style.grid_template_rows_len = 1;
  SLRustConstraints grid_fit_content_percent_row_constraints{
      {10.0f, SLRustMeasureModeDefinite},
      {120.0f, SLRustMeasureModeDefinite},
  };
  SLRustStatus grid_fit_content_percent_row_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, grid_fit_content_percent_row_constraints,
          &out_size);
  if (grid_fit_content_percent_row_status != SLRustStatusOk) {
    return 116;
  }
  if (!context.measure_called || out_size.width != 10.0f ||
      out_size.height != 120.0f) {
    return 117;
  }
  if (context.child.layout.offset.y != 0.0f ||
      context.child.layout.size.width != 10.0f ||
      context.child.layout.size.height != 60.0f) {
    return 118;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = true;
  context.root_child_count = 1;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  SLRustLength grid_fit_content_calc_row_columns[1] = {
      {SLRustLengthPoints, 10.0f, 0.0f, false, false},
  };
  SLRustLength grid_fit_content_calc_row_rows[1] = {
      {SLRustLengthFitContent, 10.0f, 50.0f, true, true},
  };
  context.root.style.display = SLRustDisplayGrid;
  context.root.style.width.kind = SLRustLengthPoints;
  context.root.style.width.value = 10.0f;
  context.root.style.height.kind = SLRustLengthPoints;
  context.root.style.height.value = 120.0f;
  context.root.style.grid_template_columns = grid_fit_content_calc_row_columns;
  context.root.style.grid_template_columns_len = 1;
  context.root.style.grid_template_rows = grid_fit_content_calc_row_rows;
  context.root.style.grid_template_rows_len = 1;
  SLRustStatus grid_fit_content_calc_row_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, grid_fit_content_percent_row_constraints,
          &out_size);
  if (grid_fit_content_calc_row_status != SLRustStatusOk) {
    return 119;
  }
  if (!context.measure_called || out_size.width != 10.0f ||
      out_size.height != 120.0f) {
    return 120;
  }
  if (context.child.layout.offset.y != 0.0f ||
      context.child.layout.size.width != 10.0f ||
      context.child.layout.size.height != 70.0f) {
    return 121;
  }
  context.child_measured_size.width = 24.0f;
  context.child_measured_size.height = 8.0f;

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = false;
  context.root_child_count = 2;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  SLRustLength grid_minmax_columns[2] = {
      {SLRustLengthPoints, 20.0f, 0.0f, false, false},
      {SLRustLengthPoints, 20.0f, 0.0f, false, false},
  };
  SLRustLength grid_minmax_columns_max[2] = {
      {SLRustLengthPoints, 50.0f, 0.0f, false, false},
      {SLRustLengthPoints, 60.0f, 0.0f, false, false},
  };
  SLRustLength grid_minmax_rows[1] = {
      {SLRustLengthPoints, 10.0f, 0.0f, false, false},
  };
  context.root.style.display = SLRustDisplayGrid;
  context.root.style.width.kind = SLRustLengthPoints;
  context.root.style.width.value = 100.0f;
  context.root.style.height.kind = SLRustLengthPoints;
  context.root.style.height.value = 10.0f;
  context.root.style.grid_template_columns = grid_minmax_columns;
  context.root.style.grid_template_columns_len = 2;
  context.root.style.grid_template_columns_max = grid_minmax_columns_max;
  context.root.style.grid_template_columns_max_len = 2;
  context.root.style.grid_template_rows = grid_minmax_rows;
  context.root.style.grid_template_rows_len = 1;
  SLRustConstraints grid_minmax_constraints{
      {100.0f, SLRustMeasureModeDefinite},
      {10.0f, SLRustMeasureModeDefinite},
  };
  SLRustStatus grid_minmax_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, grid_minmax_constraints, &out_size);
  if (grid_minmax_status != SLRustStatusOk) {
    return 59;
  }
  if (out_size.width != 100.0f || out_size.height != 10.0f) {
    return 60;
  }
  if (context.child.layout.offset.x != 0.0f ||
      context.child.layout.size.width != 50.0f ||
      context.child.layout.size.height != 10.0f ||
      context.second_child.layout.offset.x != 50.0f ||
      context.second_child.layout.size.width != 50.0f ||
      context.second_child.layout.size.height != 10.0f) {
    return 61;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = true;
  context.root_child_count = 2;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  context.child_measured_size.width = 90.0f;
  context.child_measured_size.height = 10.0f;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  SLRustLength grid_minmax_fit_percent_columns[2] = {
      {SLRustLengthPoints, 20.0f, 0.0f, false, false},
      {SLRustLengthPoints, 10.0f, 0.0f, false, false},
  };
  SLRustLength grid_minmax_fit_percent_columns_max[1] = {
      {SLRustLengthFitContent, 0.0f, 50.0f, true, true},
  };
  SLRustLength grid_minmax_fit_percent_rows[1] = {
      {SLRustLengthPoints, 10.0f, 0.0f, false, false},
  };
  context.root.style.display = SLRustDisplayGrid;
  context.root.style.width.kind = SLRustLengthPoints;
  context.root.style.width.value = 120.0f;
  context.root.style.height.kind = SLRustLengthPoints;
  context.root.style.height.value = 10.0f;
  context.root.style.grid_template_columns = grid_minmax_fit_percent_columns;
  context.root.style.grid_template_columns_len = 2;
  context.root.style.grid_template_columns_max =
      grid_minmax_fit_percent_columns_max;
  context.root.style.grid_template_columns_max_len = 1;
  context.root.style.grid_template_rows = grid_minmax_fit_percent_rows;
  context.root.style.grid_template_rows_len = 1;
  context.child.style.grid_column_start = 1;
  context.child.style.grid_row_start = 1;
  context.second_child.style.grid_column_start = 2;
  context.second_child.style.grid_row_start = 1;
  SLRustConstraints grid_minmax_fit_constraints{
      {120.0f, SLRustMeasureModeDefinite},
      {10.0f, SLRustMeasureModeDefinite},
  };
  SLRustStatus grid_minmax_fit_percent_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, grid_minmax_fit_constraints, &out_size);
  if (grid_minmax_fit_percent_status != SLRustStatusOk) {
    return 92;
  }
  if (!context.measure_called || out_size.width != 120.0f ||
      out_size.height != 10.0f) {
    return 93;
  }
  if (context.second_child.layout.offset.x != 60.0f ||
      context.second_child.layout.size.width != 10.0f ||
      context.second_child.layout.size.height != 10.0f) {
    return 94;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = true;
  context.root_child_count = 2;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  SLRustLength grid_minmax_fit_calc_columns_max[1] = {
      {SLRustLengthFitContent, 10.0f, 50.0f, true, true},
  };
  context.root.style.display = SLRustDisplayGrid;
  context.root.style.width.kind = SLRustLengthPoints;
  context.root.style.width.value = 120.0f;
  context.root.style.height.kind = SLRustLengthPoints;
  context.root.style.height.value = 10.0f;
  context.root.style.grid_template_columns = grid_minmax_fit_percent_columns;
  context.root.style.grid_template_columns_len = 2;
  context.root.style.grid_template_columns_max =
      grid_minmax_fit_calc_columns_max;
  context.root.style.grid_template_columns_max_len = 1;
  context.root.style.grid_template_rows = grid_minmax_fit_percent_rows;
  context.root.style.grid_template_rows_len = 1;
  context.child.style.grid_column_start = 1;
  context.child.style.grid_row_start = 1;
  context.second_child.style.grid_column_start = 2;
  context.second_child.style.grid_row_start = 1;
  SLRustStatus grid_minmax_fit_calc_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, grid_minmax_fit_constraints, &out_size);
  if (grid_minmax_fit_calc_status != SLRustStatusOk) {
    return 95;
  }
  if (!context.measure_called || out_size.width != 120.0f ||
      out_size.height != 10.0f) {
    return 96;
  }
  if (context.second_child.layout.offset.x != 70.0f ||
      context.second_child.layout.size.width != 10.0f ||
      context.second_child.layout.size.height != 10.0f) {
    return 97;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = true;
  context.root_child_count = 2;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  context.child_measured_size.width = 10.0f;
  context.child_measured_size.height = 90.0f;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  SLRustLength grid_minmax_fit_row_columns[1] = {
      {SLRustLengthPoints, 10.0f, 0.0f, false, false},
  };
  SLRustLength grid_minmax_fit_row_percent_rows[2] = {
      {SLRustLengthPoints, 20.0f, 0.0f, false, false},
      {SLRustLengthPoints, 10.0f, 0.0f, false, false},
  };
  SLRustLength grid_minmax_fit_percent_rows_max[1] = {
      {SLRustLengthFitContent, 0.0f, 50.0f, true, true},
  };
  context.root.style.display = SLRustDisplayGrid;
  context.root.style.width.kind = SLRustLengthPoints;
  context.root.style.width.value = 10.0f;
  context.root.style.height.kind = SLRustLengthPoints;
  context.root.style.height.value = 120.0f;
  context.root.style.grid_template_columns = grid_minmax_fit_row_columns;
  context.root.style.grid_template_columns_len = 1;
  context.root.style.grid_template_rows = grid_minmax_fit_row_percent_rows;
  context.root.style.grid_template_rows_len = 2;
  context.root.style.grid_template_rows_max = grid_minmax_fit_percent_rows_max;
  context.root.style.grid_template_rows_max_len = 1;
  context.child.style.grid_column_start = 1;
  context.child.style.grid_row_start = 1;
  context.second_child.style.grid_column_start = 1;
  context.second_child.style.grid_row_start = 2;
  SLRustConstraints grid_minmax_fit_row_constraints{
      {10.0f, SLRustMeasureModeDefinite},
      {120.0f, SLRustMeasureModeDefinite},
  };
  SLRustStatus grid_minmax_fit_row_percent_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, grid_minmax_fit_row_constraints,
          &out_size);
  if (grid_minmax_fit_row_percent_status != SLRustStatusOk) {
    return 110;
  }
  if (!context.measure_called || out_size.width != 10.0f ||
      out_size.height != 120.0f) {
    return 111;
  }
  if (context.second_child.layout.offset.y != 60.0f ||
      context.second_child.layout.size.width != 10.0f ||
      context.second_child.layout.size.height != 10.0f) {
    return 112;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = true;
  context.root_child_count = 2;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  SLRustLength grid_minmax_fit_calc_rows_max[1] = {
      {SLRustLengthFitContent, 10.0f, 50.0f, true, true},
  };
  context.root.style.display = SLRustDisplayGrid;
  context.root.style.width.kind = SLRustLengthPoints;
  context.root.style.width.value = 10.0f;
  context.root.style.height.kind = SLRustLengthPoints;
  context.root.style.height.value = 120.0f;
  context.root.style.grid_template_columns = grid_minmax_fit_row_columns;
  context.root.style.grid_template_columns_len = 1;
  context.root.style.grid_template_rows = grid_minmax_fit_row_percent_rows;
  context.root.style.grid_template_rows_len = 2;
  context.root.style.grid_template_rows_max = grid_minmax_fit_calc_rows_max;
  context.root.style.grid_template_rows_max_len = 1;
  context.child.style.grid_column_start = 1;
  context.child.style.grid_row_start = 1;
  context.second_child.style.grid_column_start = 1;
  context.second_child.style.grid_row_start = 2;
  SLRustStatus grid_minmax_fit_row_calc_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, grid_minmax_fit_row_constraints,
          &out_size);
  if (grid_minmax_fit_row_calc_status != SLRustStatusOk) {
    return 113;
  }
  if (!context.measure_called || out_size.width != 10.0f ||
      out_size.height != 120.0f) {
    return 114;
  }
  if (context.second_child.layout.offset.y != 70.0f ||
      context.second_child.layout.size.width != 10.0f ||
      context.second_child.layout.size.height != 10.0f) {
    return 115;
  }
  context.child_measured_size.width = 24.0f;
  context.child_measured_size.height = 8.0f;

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = false;
  context.root_child_count = 2;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  SLRustLength grid_auto_minmax_columns[2] = {
      {SLRustLengthPoints, 20.0f, 0.0f, false, false},
      {SLRustLengthPoints, 20.0f, 0.0f, false, false},
  };
  SLRustLength grid_auto_minmax_columns_max[2] = {
      {SLRustLengthPoints, 50.0f, 0.0f, false, false},
      {SLRustLengthPoints, 60.0f, 0.0f, false, false},
  };
  SLRustLength grid_auto_minmax_rows[1] = {
      {SLRustLengthPoints, 10.0f, 0.0f, false, false},
  };
  SLRustLength grid_auto_minmax_rows_max[1] = {
      {SLRustLengthPoints, 30.0f, 0.0f, false, false},
  };
  context.root.style.display = SLRustDisplayGrid;
  context.root.style.width.kind = SLRustLengthPoints;
  context.root.style.width.value = 110.0f;
  context.root.style.height.kind = SLRustLengthPoints;
  context.root.style.height.value = 30.0f;
  context.root.style.grid_auto_columns = grid_auto_minmax_columns;
  context.root.style.grid_auto_columns_len = 2;
  context.root.style.grid_auto_columns_max = grid_auto_minmax_columns_max;
  context.root.style.grid_auto_columns_max_len = 2;
  context.root.style.grid_auto_rows = grid_auto_minmax_rows;
  context.root.style.grid_auto_rows_len = 1;
  context.root.style.grid_auto_rows_max = grid_auto_minmax_rows_max;
  context.root.style.grid_auto_rows_max_len = 1;
  context.second_child.style.grid_column_start = 2;
  context.second_child.style.grid_row_start = 1;
  SLRustConstraints grid_auto_minmax_constraints{
      {110.0f, SLRustMeasureModeDefinite},
      {30.0f, SLRustMeasureModeDefinite},
  };
  SLRustStatus grid_auto_minmax_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, grid_auto_minmax_constraints, &out_size);
  if (grid_auto_minmax_status != SLRustStatusOk) {
    return 62;
  }
  if (out_size.width != 110.0f || out_size.height != 30.0f) {
    return 63;
  }
  if (context.child.layout.offset.x != 0.0f ||
      context.child.layout.size.width != 50.0f ||
      context.child.layout.size.height != 30.0f ||
      context.second_child.layout.offset.x != 50.0f ||
      context.second_child.layout.size.width != 60.0f ||
      context.second_child.layout.size.height != 30.0f) {
    return 64;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = true;
  context.root_child_count = 2;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  context.child_measured_size.width = 90.0f;
  context.child_measured_size.height = 10.0f;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  SLRustLength grid_auto_fit_percent_columns[2] = {
      {SLRustLengthPoints, 20.0f, 0.0f, false, false},
      {SLRustLengthPoints, 10.0f, 0.0f, false, false},
  };
  SLRustLength grid_auto_fit_percent_columns_max[2] = {
      {SLRustLengthFitContent, 0.0f, 50.0f, true, true},
      {SLRustLengthPoints, 60.0f, 0.0f, false, false},
  };
  SLRustLength grid_auto_fit_rows[1] = {
      {SLRustLengthPoints, 10.0f, 0.0f, false, false},
  };
  context.root.style.display = SLRustDisplayGrid;
  context.root.style.width.kind = SLRustLengthPoints;
  context.root.style.width.value = 120.0f;
  context.root.style.height.kind = SLRustLengthPoints;
  context.root.style.height.value = 10.0f;
  context.root.style.grid_auto_columns = grid_auto_fit_percent_columns;
  context.root.style.grid_auto_columns_len = 2;
  context.root.style.grid_auto_columns_max = grid_auto_fit_percent_columns_max;
  context.root.style.grid_auto_columns_max_len = 2;
  context.root.style.grid_auto_rows = grid_auto_fit_rows;
  context.root.style.grid_auto_rows_len = 1;
  context.child.style.grid_column_start = 1;
  context.child.style.grid_row_start = 1;
  context.second_child.style.grid_column_start = 2;
  context.second_child.style.grid_row_start = 1;
  SLRustConstraints grid_auto_fit_constraints{
      {120.0f, SLRustMeasureModeDefinite},
      {10.0f, SLRustMeasureModeDefinite},
  };
  SLRustStatus grid_auto_fit_percent_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, grid_auto_fit_constraints, &out_size);
  if (grid_auto_fit_percent_status != SLRustStatusOk) {
    return 98;
  }
  if (!context.measure_called || out_size.width != 120.0f ||
      out_size.height != 10.0f) {
    return 99;
  }
  if (context.second_child.layout.offset.x != 60.0f ||
      context.second_child.layout.size.width != 60.0f ||
      context.second_child.layout.size.height != 10.0f) {
    return 100;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = true;
  context.root_child_count = 2;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  SLRustLength grid_auto_fit_calc_columns_max[2] = {
      {SLRustLengthFitContent, 10.0f, 50.0f, true, true},
      {SLRustLengthPoints, 50.0f, 0.0f, false, false},
  };
  context.root.style.display = SLRustDisplayGrid;
  context.root.style.width.kind = SLRustLengthPoints;
  context.root.style.width.value = 120.0f;
  context.root.style.height.kind = SLRustLengthPoints;
  context.root.style.height.value = 10.0f;
  context.root.style.grid_auto_columns = grid_auto_fit_percent_columns;
  context.root.style.grid_auto_columns_len = 2;
  context.root.style.grid_auto_columns_max = grid_auto_fit_calc_columns_max;
  context.root.style.grid_auto_columns_max_len = 2;
  context.root.style.grid_auto_rows = grid_auto_fit_rows;
  context.root.style.grid_auto_rows_len = 1;
  context.child.style.grid_column_start = 1;
  context.child.style.grid_row_start = 1;
  context.second_child.style.grid_column_start = 2;
  context.second_child.style.grid_row_start = 1;
  SLRustStatus grid_auto_fit_calc_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, grid_auto_fit_constraints, &out_size);
  if (grid_auto_fit_calc_status != SLRustStatusOk) {
    return 101;
  }
  if (!context.measure_called || out_size.width != 120.0f ||
      out_size.height != 10.0f) {
    return 102;
  }
  if (context.second_child.layout.offset.x != 70.0f ||
      context.second_child.layout.size.width != 50.0f ||
      context.second_child.layout.size.height != 10.0f) {
    return 103;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = true;
  context.root_child_count = 2;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  context.child_measured_size.width = 10.0f;
  context.child_measured_size.height = 90.0f;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  SLRustLength grid_auto_row_fit_columns[1] = {
      {SLRustLengthPoints, 10.0f, 0.0f, false, false},
  };
  SLRustLength grid_auto_row_fit_percent_rows[2] = {
      {SLRustLengthPoints, 20.0f, 0.0f, false, false},
      {SLRustLengthPoints, 10.0f, 0.0f, false, false},
  };
  SLRustLength grid_auto_row_fit_percent_rows_max[2] = {
      {SLRustLengthFitContent, 0.0f, 50.0f, true, true},
      {SLRustLengthPoints, 60.0f, 0.0f, false, false},
  };
  context.root.style.display = SLRustDisplayGrid;
  context.root.style.width.kind = SLRustLengthPoints;
  context.root.style.width.value = 10.0f;
  context.root.style.height.kind = SLRustLengthPoints;
  context.root.style.height.value = 120.0f;
  context.root.style.grid_auto_columns = grid_auto_row_fit_columns;
  context.root.style.grid_auto_columns_len = 1;
  context.root.style.grid_auto_rows = grid_auto_row_fit_percent_rows;
  context.root.style.grid_auto_rows_len = 2;
  context.root.style.grid_auto_rows_max = grid_auto_row_fit_percent_rows_max;
  context.root.style.grid_auto_rows_max_len = 2;
  context.child.style.grid_column_start = 1;
  context.child.style.grid_row_start = 1;
  context.second_child.style.grid_column_start = 1;
  context.second_child.style.grid_row_start = 2;
  SLRustConstraints grid_auto_row_fit_constraints{
      {10.0f, SLRustMeasureModeDefinite},
      {120.0f, SLRustMeasureModeDefinite},
  };
  SLRustStatus grid_auto_row_fit_percent_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, grid_auto_row_fit_constraints,
          &out_size);
  if (grid_auto_row_fit_percent_status != SLRustStatusOk) {
    return 104;
  }
  if (!context.measure_called || out_size.width != 10.0f ||
      out_size.height != 120.0f) {
    return 105;
  }
  if (context.second_child.layout.offset.y != 60.0f ||
      context.second_child.layout.size.width != 10.0f ||
      context.second_child.layout.size.height != 60.0f) {
    return 106;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = true;
  context.root_child_count = 2;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  SLRustLength grid_auto_row_fit_calc_rows_max[2] = {
      {SLRustLengthFitContent, 10.0f, 50.0f, true, true},
      {SLRustLengthPoints, 50.0f, 0.0f, false, false},
  };
  context.root.style.display = SLRustDisplayGrid;
  context.root.style.width.kind = SLRustLengthPoints;
  context.root.style.width.value = 10.0f;
  context.root.style.height.kind = SLRustLengthPoints;
  context.root.style.height.value = 120.0f;
  context.root.style.grid_auto_columns = grid_auto_row_fit_columns;
  context.root.style.grid_auto_columns_len = 1;
  context.root.style.grid_auto_rows = grid_auto_row_fit_percent_rows;
  context.root.style.grid_auto_rows_len = 2;
  context.root.style.grid_auto_rows_max = grid_auto_row_fit_calc_rows_max;
  context.root.style.grid_auto_rows_max_len = 2;
  context.child.style.grid_column_start = 1;
  context.child.style.grid_row_start = 1;
  context.second_child.style.grid_column_start = 1;
  context.second_child.style.grid_row_start = 2;
  SLRustStatus grid_auto_row_fit_calc_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, grid_auto_row_fit_constraints,
          &out_size);
  if (grid_auto_row_fit_calc_status != SLRustStatusOk) {
    return 107;
  }
  if (!context.measure_called || out_size.width != 10.0f ||
      out_size.height != 120.0f) {
    return 108;
  }
  if (context.second_child.layout.offset.y != 70.0f ||
      context.second_child.layout.size.width != 10.0f ||
      context.second_child.layout.size.height != 50.0f) {
    return 109;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = true;
  context.root_child_count = 2;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  context.child_measured_size.width = 70.0f;
  context.child_measured_size.height = 70.0f;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  SLRustLength grid_auto_fit_fixed_columns[2] = {
      {SLRustLengthPoints, 20.0f, 0.0f, false, false},
      {SLRustLengthPoints, 10.0f, 0.0f, false, false},
  };
  SLRustLength grid_auto_fit_fixed_columns_max[2] = {
      {SLRustLengthFitContent, 40.0f, 0.0f, true, false},
      {SLRustLengthPoints, 10.0f, 0.0f, false, false},
  };
  SLRustLength grid_auto_fit_fixed_rows[2] = {
      {SLRustLengthPoints, 20.0f, 0.0f, false, false},
      {SLRustLengthPoints, 10.0f, 0.0f, false, false},
  };
  SLRustLength grid_auto_fit_fixed_rows_max[2] = {
      {SLRustLengthFitContent, 40.0f, 0.0f, true, false},
      {SLRustLengthPoints, 10.0f, 0.0f, false, false},
  };
  context.root.style.display = SLRustDisplayGrid;
  context.root.style.grid_auto_columns = grid_auto_fit_fixed_columns;
  context.root.style.grid_auto_columns_len = 2;
  context.root.style.grid_auto_columns_max = grid_auto_fit_fixed_columns_max;
  context.root.style.grid_auto_columns_max_len = 2;
  context.root.style.grid_auto_rows = grid_auto_fit_fixed_rows;
  context.root.style.grid_auto_rows_len = 2;
  context.root.style.grid_auto_rows_max = grid_auto_fit_fixed_rows_max;
  context.root.style.grid_auto_rows_max_len = 2;
  context.child.style.grid_column_start = 1;
  context.child.style.grid_row_start = 1;
  context.second_child.style.grid_column_start = 2;
  context.second_child.style.grid_row_start = 2;
  SLRustConstraints grid_auto_fit_fixed_indefinite_constraints{
      {0.0f, SLRustMeasureModeIndefinite},
      {0.0f, SLRustMeasureModeIndefinite},
  };
  SLRustStatus grid_auto_fit_fixed_indefinite_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1,
          grid_auto_fit_fixed_indefinite_constraints, &out_size);
  if (grid_auto_fit_fixed_indefinite_status != SLRustStatusOk) {
    return 122;
  }
  if (!context.measure_called || out_size.width != 50.0f ||
      out_size.height != 50.0f) {
    return 123;
  }
  if (context.child.layout.size.width != 40.0f ||
      context.child.layout.size.height != 40.0f ||
      context.second_child.layout.offset.x != 40.0f ||
      context.second_child.layout.offset.y != 40.0f ||
      context.second_child.layout.size.width != 10.0f ||
      context.second_child.layout.size.height != 10.0f) {
    return 124;
  }
  context.child_measured_size.width = 24.0f;
  context.child_measured_size.height = 8.0f;

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = false;
  context.root_child_count = 2;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  SLRustLength grid_rtl_columns[2] = {
      {SLRustLengthPoints, 20.0f, 0.0f, false, false},
      {SLRustLengthPoints, 30.0f, 0.0f, false, false},
  };
  SLRustLength grid_rtl_rows[1] = {
      {SLRustLengthPoints, 10.0f, 0.0f, false, false},
  };
  context.root.style.display = SLRustDisplayGrid;
  context.root.style.direction = SLRustDirectionRtl;
  context.root.style.width.kind = SLRustLengthPoints;
  context.root.style.width.value = 60.0f;
  context.root.style.height.kind = SLRustLengthPoints;
  context.root.style.height.value = 10.0f;
  context.root.style.column_gap.kind = SLRustLengthPoints;
  context.root.style.column_gap.value = 10.0f;
  context.root.style.grid_template_columns = grid_rtl_columns;
  context.root.style.grid_template_columns_len = 2;
  context.root.style.grid_template_rows = grid_rtl_rows;
  context.root.style.grid_template_rows_len = 1;
  context.child.style.grid_column_start = 1;
  context.child.style.grid_row_start = 1;
  context.second_child.style.grid_column_start = 2;
  context.second_child.style.grid_row_start = 1;
  SLRustConstraints grid_rtl_constraints{
      {60.0f, SLRustMeasureModeDefinite},
      {10.0f, SLRustMeasureModeDefinite},
  };
  SLRustStatus grid_rtl_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, grid_rtl_constraints, &out_size);
  if (grid_rtl_status != SLRustStatusOk) {
    return 65;
  }
  if (out_size.width != 60.0f || out_size.height != 10.0f) {
    return 66;
  }
  if (context.child.layout.offset.x != 40.0f ||
      context.child.layout.size.width != 20.0f ||
      context.child.layout.size.height != 10.0f ||
      context.second_child.layout.offset.x != 0.0f ||
      context.second_child.layout.size.width != 30.0f ||
      context.second_child.layout.size.height != 10.0f) {
    return 67;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = false;
  context.root_child_count = 1;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  context.root.style.width.kind = SLRustLengthPoints;
  context.root.style.width.value = 200.0f;
  context.root.style.height.kind = SLRustLengthPoints;
  context.root.style.height.value = 80.0f;
  context.child.style.position = SLRustPositionSticky;
  context.child.style.width.kind = SLRustLengthPoints;
  context.child.style.width.value = 20.0f;
  context.child.style.height.kind = SLRustLengthPoints;
  context.child.style.height.value = 10.0f;
  context.child.style.left.kind = SLRustLengthCalc;
  context.child.style.left.value = 3.0f;
  context.child.style.left.percent = 10.0f;
  context.child.style.left.has_base = true;
  context.child.style.right.kind = SLRustLengthCalc;
  context.child.style.right.value = 4.0f;
  context.child.style.right.percent = 5.0f;
  context.child.style.right.has_base = true;
  context.child.style.top.kind = SLRustLengthCalc;
  context.child.style.top.value = 2.0f;
  context.child.style.top.percent = 25.0f;
  context.child.style.top.has_base = true;
  context.child.style.bottom.kind = SLRustLengthCalc;
  context.child.style.bottom.value = 1.0f;
  context.child.style.bottom.percent = 50.0f;
  context.child.style.bottom.has_base = true;
  SLRustConstraints sticky_constraints{
      {200.0f, SLRustMeasureModeDefinite},
      {80.0f, SLRustMeasureModeDefinite},
  };
  SLRustStatus sticky_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, sticky_constraints, &out_size);
  if (sticky_status != SLRustStatusOk) {
    return 36;
  }
  if (context.child.layout.sticky_pos.left != 23.0f ||
      context.child.layout.sticky_pos.right != 14.0f ||
      context.child.layout.sticky_pos.top != 22.0f ||
      context.child.layout.sticky_pos.bottom != 41.0f) {
    return 37;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = false;
  context.root_child_count = 2;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  context.root.style.display = SLRustDisplayRelative;
  context.child.style.width.kind = SLRustLengthPoints;
  context.child.style.width.value = 5.0f;
  context.child.style.height.kind = SLRustLengthPoints;
  context.child.style.height.value = 7.0f;
  context.child.style.relative_right_of = 10;
  context.child.style.relative_bottom_of = 10;
  context.second_child.style.width.kind = SLRustLengthPoints;
  context.second_child.style.width.value = 20.0f;
  context.second_child.style.height.kind = SLRustLengthPoints;
  context.second_child.style.height.value = 10.0f;
  context.second_child.style.relative_id = 10;
  SLRustConstraints relative_constraints{
      {0.0f, SLRustMeasureModeIndefinite},
      {0.0f, SLRustMeasureModeIndefinite},
  };
  SLRustStatus relative_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, relative_constraints, &out_size);
  if (relative_status != SLRustStatusOk) {
    return 38;
  }
  if (out_size.width != 25.0f || out_size.height != 17.0f) {
    return 39;
  }
  if (context.child.layout.offset.x != 20.0f ||
      context.child.layout.offset.y != 10.0f ||
      context.child.layout.size.width != 5.0f ||
      context.child.layout.size.height != 7.0f ||
      context.second_child.layout.offset.x != 0.0f ||
      context.second_child.layout.offset.y != 0.0f ||
      context.second_child.layout.size.width != 20.0f ||
      context.second_child.layout.size.height != 10.0f) {
    return 40;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = false;
  context.root_child_count = 1;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  SLRustLength grid_place_columns[3] = {
      {SLRustLengthPoints, 40.0f, 0.0f, false, false},
      {SLRustLengthPoints, 40.0f, 0.0f, false, false},
      {SLRustLengthPoints, 40.0f, 0.0f, false, false},
  };
  SLRustLength grid_place_rows[2] = {
      {SLRustLengthPoints, 20.0f, 0.0f, false, false},
      {SLRustLengthPoints, 20.0f, 0.0f, false, false},
  };
  context.root.style.display = SLRustDisplayGrid;
  context.root.style.width.kind = SLRustLengthPoints;
  context.root.style.width.value = 130.0f;
  context.root.style.height.kind = SLRustLengthPoints;
  context.root.style.height.value = 45.0f;
  context.root.style.grid_template_columns = grid_place_columns;
  context.root.style.grid_template_columns_len = 3;
  context.root.style.grid_template_rows = grid_place_rows;
  context.root.style.grid_template_rows_len = 2;
  context.root.style.column_gap.kind = SLRustLengthPoints;
  context.root.style.column_gap.value = 5.0f;
  context.root.style.row_gap.kind = SLRustLengthPoints;
  context.root.style.row_gap.value = 5.0f;
  context.child.style.width.kind = SLRustLengthPoints;
  context.child.style.width.value = 10.0f;
  context.child.style.height.kind = SLRustLengthPoints;
  context.child.style.height.value = 8.0f;
  context.child.style.grid_column_start = 2;
  context.child.style.grid_row_start = 2;
  context.child.style.grid_column_span = 2;
  context.child.style.justify_self = SLRustJustifyItemsEnd;
  context.child.style.align_self = SLRustAlignItemsFlexEnd;
  context.child.style.has_align_self = true;
  SLRustConstraints grid_place_constraints{
      {130.0f, SLRustMeasureModeDefinite},
      {45.0f, SLRustMeasureModeDefinite},
  };
  SLRustStatus grid_place_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, grid_place_constraints, &out_size);
  if (grid_place_status != SLRustStatusOk) {
    return 41;
  }
  if (out_size.width != 130.0f || out_size.height != 45.0f) {
    return 42;
  }
  if (context.child.layout.offset.x != 120.0f ||
      context.child.layout.offset.y != 37.0f ||
      context.child.layout.size.width != 10.0f ||
      context.child.layout.size.height != 8.0f) {
    return 43;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = false;
  context.root_child_count = 1;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  SLRustLength grid_absolute_columns[2] = {
      {SLRustLengthPoints, 20.0f, 0.0f, false, false},
      {SLRustLengthPoints, 30.0f, 0.0f, false, false},
  };
  SLRustLength grid_absolute_rows[2] = {
      {SLRustLengthPoints, 10.0f, 0.0f, false, false},
      {SLRustLengthPoints, 20.0f, 0.0f, false, false},
  };
  context.root.style.display = SLRustDisplayGrid;
  context.root.style.width.kind = SLRustLengthPoints;
  context.root.style.width.value = 55.0f;
  context.root.style.height.kind = SLRustLengthPoints;
  context.root.style.height.value = 35.0f;
  context.root.style.grid_template_columns = grid_absolute_columns;
  context.root.style.grid_template_columns_len = 2;
  context.root.style.grid_template_rows = grid_absolute_rows;
  context.root.style.grid_template_rows_len = 2;
  context.root.style.column_gap.kind = SLRustLengthPoints;
  context.root.style.column_gap.value = 5.0f;
  context.root.style.row_gap.kind = SLRustLengthPoints;
  context.root.style.row_gap.value = 5.0f;
  context.child.style.position = SLRustPositionAbsolute;
  context.child.style.grid_column_start = 2;
  context.child.style.grid_column_end = 3;
  context.child.style.grid_row_start = 1;
  context.child.style.grid_row_end = 3;
  context.child.style.left.kind = SLRustLengthPoints;
  context.child.style.left.value = 2.0f;
  context.child.style.right.kind = SLRustLengthPoints;
  context.child.style.right.value = 3.0f;
  context.child.style.top.kind = SLRustLengthPoints;
  context.child.style.top.value = 1.0f;
  context.child.style.bottom.kind = SLRustLengthPoints;
  context.child.style.bottom.value = 4.0f;
  SLRustConstraints grid_absolute_constraints{
      {55.0f, SLRustMeasureModeDefinite},
      {35.0f, SLRustMeasureModeDefinite},
  };
  SLRustStatus grid_absolute_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, grid_absolute_constraints, &out_size);
  if (grid_absolute_status != SLRustStatusOk) {
    return 68;
  }
  if (out_size.width != 55.0f || out_size.height != 35.0f) {
    return 69;
  }
  if (context.child.layout.offset.x != 27.0f ||
      context.child.layout.offset.y != 1.0f ||
      context.child.layout.size.width != 25.0f ||
      context.child.layout.size.height != 30.0f) {
    return 70;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = false;
  context.root_child_count = 1;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  SLRustLength grid_fixed_columns[2] = {
      {SLRustLengthPoints, 20.0f, 0.0f, false, false},
      {SLRustLengthPoints, 30.0f, 0.0f, false, false},
  };
  SLRustLength grid_fixed_rows[1] = {
      {SLRustLengthPoints, 10.0f, 0.0f, false, false},
  };
  context.root.style.display = SLRustDisplayGrid;
  context.root.style.width.kind = SLRustLengthPoints;
  context.root.style.width.value = 55.0f;
  context.root.style.height.kind = SLRustLengthPoints;
  context.root.style.height.value = 10.0f;
  context.root.style.grid_template_columns = grid_fixed_columns;
  context.root.style.grid_template_columns_len = 2;
  context.root.style.grid_template_rows = grid_fixed_rows;
  context.root.style.grid_template_rows_len = 1;
  context.root.style.column_gap.kind = SLRustLengthPoints;
  context.root.style.column_gap.value = 5.0f;
  context.child.style.position = SLRustPositionFixed;
  context.child.style.grid_column_start = 2;
  context.child.style.grid_column_end = 3;
  context.child.style.grid_row_start = 1;
  context.child.style.grid_row_end = 2;
  context.child.style.left.kind = SLRustLengthPoints;
  context.child.style.left.value = 2.0f;
  context.child.style.right.kind = SLRustLengthPoints;
  context.child.style.right.value = 3.0f;
  context.child.style.top.kind = SLRustLengthPoints;
  context.child.style.top.value = 1.0f;
  context.child.style.bottom.kind = SLRustLengthPoints;
  context.child.style.bottom.value = 4.0f;
  SLRustConstraints grid_fixed_constraints{
      {55.0f, SLRustMeasureModeDefinite},
      {10.0f, SLRustMeasureModeDefinite},
  };
  SLRustStatus grid_fixed_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, grid_fixed_constraints, &out_size);
  if (grid_fixed_status != SLRustStatusOk) {
    return 71;
  }
  if (out_size.width != 55.0f || out_size.height != 10.0f) {
    return 72;
  }
  if (context.child.layout.offset.x != 27.0f ||
      context.child.layout.offset.y != 1.0f ||
      context.child.layout.size.width != 25.0f ||
      context.child.layout.size.height != 5.0f) {
    return 73;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = false;
  context.root_child_count = 2;
  context.child_has_second_child = false;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  context.root.style.padding.left.kind = SLRustLengthPoints;
  context.root.style.padding.left.value = 2.0f;
  context.root.style.padding.right.kind = SLRustLengthPoints;
  context.root.style.padding.right.value = 2.0f;
  context.root.style.padding.top.kind = SLRustLengthPoints;
  context.root.style.padding.top.value = 2.0f;
  context.root.style.padding.bottom.kind = SLRustLengthPoints;
  context.root.style.padding.bottom.value = 2.0f;
  context.child.style.position = SLRustPositionAbsolute;
  context.child.style.width.kind = SLRustLengthPoints;
  context.child.style.width.value = 20.0f;
  context.child.style.height.kind = SLRustLengthPoints;
  context.child.style.height.value = 30.0f;
  context.child.style.left.kind = SLRustLengthPoints;
  context.child.style.left.value = 7.0f;
  context.child.style.top.kind = SLRustLengthPoints;
  context.child.style.top.value = 9.0f;
  context.second_child.style.width.kind = SLRustLengthPoints;
  context.second_child.style.width.value = 10.0f;
  context.second_child.style.height.kind = SLRustLengthPoints;
  context.second_child.style.height.value = 5.0f;
  SLRustConstraints absolute_constraints{
      {100.0f, SLRustMeasureModeDefinite},
      {0.0f, SLRustMeasureModeIndefinite},
  };
  SLRustStatus absolute_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, absolute_constraints, &out_size);
  if (absolute_status != SLRustStatusOk) {
    return 44;
  }
  if (out_size.height != 9.0f) {
    return 45;
  }
  if (context.child.layout.offset.x != 7.0f ||
      context.child.layout.offset.y != 9.0f ||
      context.child.layout.size.width != 20.0f ||
      context.child.layout.size.height != 30.0f ||
      context.second_child.layout.offset.y != 2.0f) {
    return 46;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = false;
  context.root_child_count = 1;
  context.child_has_second_child = true;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  context.root.style.width.kind = SLRustLengthPoints;
  context.root.style.width.value = 100.0f;
  context.root.style.height.kind = SLRustLengthPoints;
  context.root.style.height.value = 80.0f;
  context.child.style.width.kind = SLRustLengthPoints;
  context.child.style.width.value = 40.0f;
  context.child.style.height.kind = SLRustLengthPoints;
  context.child.style.height.value = 30.0f;
  context.child.style.padding.left.kind = SLRustLengthPoints;
  context.child.style.padding.left.value = 2.0f;
  context.child.style.padding.right.kind = SLRustLengthPoints;
  context.child.style.padding.right.value = 2.0f;
  context.child.style.padding.top.kind = SLRustLengthPoints;
  context.child.style.padding.top.value = 2.0f;
  context.child.style.padding.bottom.kind = SLRustLengthPoints;
  context.child.style.padding.bottom.value = 2.0f;
  context.second_child.style.position = SLRustPositionFixed;
  context.second_child.style.width.kind = SLRustLengthPoints;
  context.second_child.style.width.value = 20.0f;
  context.second_child.style.height.kind = SLRustLengthPoints;
  context.second_child.style.height.value = 10.0f;
  context.second_child.style.right.kind = SLRustLengthPoints;
  context.second_child.style.right.value = 5.0f;
  context.second_child.style.bottom.kind = SLRustLengthPoints;
  context.second_child.style.bottom.value = 7.0f;
  SLRustConstraints fixed_constraints{
      {100.0f, SLRustMeasureModeDefinite},
      {80.0f, SLRustMeasureModeDefinite},
  };
  SLRustStatus fixed_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, fixed_constraints, &out_size);
  if (fixed_status != SLRustStatusOk) {
    return 47;
  }
  if (out_size.width != 100.0f || out_size.height != 80.0f) {
    return 48;
  }
  if (context.child.layout.size.width != 44.0f ||
      context.child.layout.size.height != 34.0f ||
      context.second_child.layout.offset.x != 75.0f ||
      context.second_child.layout.offset.y != 63.0f ||
      context.second_child.layout.size.width != 20.0f ||
      context.second_child.layout.size.height != 10.0f) {
    return 49;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = false;
  context.root_child_count = 1;
  context.child_has_second_child = false;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  SLRustLength grid_margin_columns[1] = {
      {SLRustLengthPoints, 50.0f, 0.0f, false, false},
  };
  SLRustLength grid_margin_rows[1] = {
      {SLRustLengthPoints, 40.0f, 0.0f, false, false},
  };
  context.root.style.display = SLRustDisplayGrid;
  context.root.style.width.kind = SLRustLengthPoints;
  context.root.style.width.value = 60.0f;
  context.root.style.height.kind = SLRustLengthPoints;
  context.root.style.height.value = 50.0f;
  context.root.style.grid_template_columns = grid_margin_columns;
  context.root.style.grid_template_columns_len = 1;
  context.root.style.grid_template_rows = grid_margin_rows;
  context.root.style.grid_template_rows_len = 1;
  context.root.style.justify_items = SLRustJustifyItemsStart;
  context.root.style.align_items = SLRustAlignItemsStart;
  context.child.style.width.kind = SLRustLengthPoints;
  context.child.style.width.value = 10.0f;
  context.child.style.height.kind = SLRustLengthPoints;
  context.child.style.height.value = 10.0f;
  context.child.style.margin.left.kind = SLRustLengthAuto;
  context.child.style.margin.right.kind = SLRustLengthAuto;
  context.child.style.margin.top.kind = SLRustLengthAuto;
  context.child.style.margin.bottom.kind = SLRustLengthPoints;
  context.child.style.margin.bottom.value = 0.0f;
  context.child.style.justify_self = SLRustJustifyItemsEnd;
  context.child.style.align_self = SLRustAlignItemsStart;
  context.child.style.has_align_self = true;
  SLRustConstraints grid_margin_constraints{
      {60.0f, SLRustMeasureModeDefinite},
      {50.0f, SLRustMeasureModeDefinite},
  };
  SLRustStatus grid_margin_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, grid_margin_constraints, &out_size);
  if (grid_margin_status != SLRustStatusOk) {
    return 50;
  }
  if (out_size.width != 60.0f || out_size.height != 50.0f) {
    return 51;
  }
  if (context.child.layout.offset.x != 20.0f ||
      context.child.layout.offset.y != 30.0f ||
      context.child.layout.size.width != 10.0f ||
      context.child.layout.size.height != 10.0f ||
      context.child.layout.margin.left != 20.0f ||
      context.child.layout.margin.right != 20.0f ||
      context.child.layout.margin.top != 30.0f ||
      context.child.layout.margin.bottom != 0.0f) {
    return 52;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = true;
  context.root_child_count = 1;
  context.child_has_second_child = false;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  context.root.style.width.kind = SLRustLengthPoints;
  context.root.style.width.value = 100.0f;
  context.root.style.height.kind = SLRustLengthPoints;
  context.root.style.height.value = 50.0f;
  context.root.style.padding.left.kind = SLRustLengthPoints;
  context.root.style.padding.left.value = 10.0f;
  context.root.style.padding.right.kind = SLRustLengthPoints;
  context.root.style.padding.right.value = 10.0f;
  context.root.style.padding.top.kind = SLRustLengthPoints;
  context.root.style.padding.top.value = 10.0f;
  context.root.style.padding.bottom.kind = SLRustLengthPoints;
  context.root.style.padding.bottom.value = 10.0f;
  context.child.style.position = SLRustPositionAbsolute;
  context.child.style.left.kind = SLRustLengthPoints;
  context.child.style.left.value = 10.0f;
  context.child.style.right.kind = SLRustLengthPoints;
  context.child.style.right.value = 15.0f;
  context.child.style.top.kind = SLRustLengthPoints;
  context.child.style.top.value = 4.0f;
  context.child.style.bottom.kind = SLRustLengthPoints;
  context.child.style.bottom.value = 6.0f;
  context.child.style.margin.left.kind = SLRustLengthPoints;
  context.child.style.margin.left.value = 2.0f;
  context.child.style.margin.right.kind = SLRustLengthPoints;
  context.child.style.margin.right.value = 3.0f;
  context.child.style.margin.top.kind = SLRustLengthPoints;
  context.child.style.margin.top.value = 1.0f;
  context.child.style.margin.bottom.kind = SLRustLengthPoints;
  context.child.style.margin.bottom.value = 2.0f;
  SLRustConstraints absolute_fill_constraints{
      {120.0f, SLRustMeasureModeDefinite},
      {70.0f, SLRustMeasureModeDefinite},
  };
  SLRustStatus absolute_fill_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, absolute_fill_constraints, &out_size);
  if (absolute_fill_status != SLRustStatusOk) {
    return 53;
  }
  if (!context.measure_called ||
      context.last_measure_constraints.width.mode !=
          SLRustMeasureModeDefinite ||
      context.last_measure_constraints.height.mode !=
          SLRustMeasureModeDefinite ||
      context.last_measure_constraints.width.size != 90.0f ||
      context.last_measure_constraints.height.size != 57.0f) {
    return 54;
  }
  if (context.child.layout.offset.x != 12.0f ||
      context.child.layout.offset.y != 5.0f ||
      context.child.layout.size.width != 90.0f ||
      context.child.layout.size.height != 57.0f) {
    return 55;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = true;
  context.root_child_count = 1;
  context.child_has_second_child = false;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  context.root.style.width.kind = SLRustLengthPoints;
  context.root.style.width.value = 200.0f;
  context.root.style.height.kind = SLRustLengthPoints;
  context.root.style.height.value = 100.0f;
  context.child.style.position = SLRustPositionAbsolute;
  context.child.style.left.kind = SLRustLengthPercent;
  context.child.style.left.value = 10.0f;
  context.child.style.right.kind = SLRustLengthCalc;
  context.child.style.right.value = 5.0f;
  context.child.style.right.percent = 20.0f;
  context.child.style.right.has_base = true;
  context.child.style.right.has_percentage = true;
  context.child.style.top.kind = SLRustLengthCalc;
  context.child.style.top.value = 2.0f;
  context.child.style.top.percent = 10.0f;
  context.child.style.top.has_base = true;
  context.child.style.top.has_percentage = true;
  context.child.style.bottom.kind = SLRustLengthPercent;
  context.child.style.bottom.value = 25.0f;
  context.child.style.margin.left.kind = SLRustLengthPoints;
  context.child.style.margin.left.value = 3.0f;
  context.child.style.margin.right.kind = SLRustLengthPoints;
  context.child.style.margin.right.value = 7.0f;
  context.child.style.margin.top.kind = SLRustLengthPoints;
  context.child.style.margin.top.value = 4.0f;
  context.child.style.margin.bottom.kind = SLRustLengthPoints;
  context.child.style.margin.bottom.value = 6.0f;
  SLRustConstraints percent_calc_fill_constraints{
      {200.0f, SLRustMeasureModeDefinite},
      {100.0f, SLRustMeasureModeDefinite},
  };
  SLRustStatus percent_calc_absolute_fill_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, percent_calc_fill_constraints, &out_size);
  if (percent_calc_absolute_fill_status != SLRustStatusOk) {
    return 101;
  }
  if (!context.measure_called ||
      context.last_measure_constraints.width.mode !=
          SLRustMeasureModeDefinite ||
      context.last_measure_constraints.height.mode !=
          SLRustMeasureModeDefinite ||
      context.last_measure_constraints.width.size != 125.0f ||
      context.last_measure_constraints.height.size != 53.0f) {
    return 102;
  }
  if (context.child.layout.offset.x != 23.0f ||
      context.child.layout.offset.y != 16.0f ||
      context.child.layout.size.width != 125.0f ||
      context.child.layout.size.height != 53.0f) {
    return 103;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = true;
  context.root_child_count = 1;
  context.child_has_second_child = false;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  context.root.style.width.kind = SLRustLengthPoints;
  context.root.style.width.value = 200.0f;
  context.root.style.height.kind = SLRustLengthPoints;
  context.root.style.height.value = 100.0f;
  context.child.style.position = SLRustPositionFixed;
  context.child.style.left.kind = SLRustLengthPercent;
  context.child.style.left.value = 10.0f;
  context.child.style.right.kind = SLRustLengthCalc;
  context.child.style.right.value = 5.0f;
  context.child.style.right.percent = 20.0f;
  context.child.style.right.has_base = true;
  context.child.style.right.has_percentage = true;
  context.child.style.top.kind = SLRustLengthCalc;
  context.child.style.top.value = 2.0f;
  context.child.style.top.percent = 10.0f;
  context.child.style.top.has_base = true;
  context.child.style.top.has_percentage = true;
  context.child.style.bottom.kind = SLRustLengthPercent;
  context.child.style.bottom.value = 25.0f;
  context.child.style.margin.left.kind = SLRustLengthPoints;
  context.child.style.margin.left.value = 3.0f;
  context.child.style.margin.right.kind = SLRustLengthPoints;
  context.child.style.margin.right.value = 7.0f;
  context.child.style.margin.top.kind = SLRustLengthPoints;
  context.child.style.margin.top.value = 4.0f;
  context.child.style.margin.bottom.kind = SLRustLengthPoints;
  context.child.style.margin.bottom.value = 6.0f;
  SLRustStatus percent_calc_fixed_fill_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, percent_calc_fill_constraints, &out_size);
  if (percent_calc_fixed_fill_status != SLRustStatusOk) {
    return 104;
  }
  if (!context.measure_called ||
      context.last_measure_constraints.width.mode !=
          SLRustMeasureModeDefinite ||
      context.last_measure_constraints.height.mode !=
          SLRustMeasureModeDefinite ||
      context.last_measure_constraints.width.size != 125.0f ||
      context.last_measure_constraints.height.size != 53.0f) {
    return 105;
  }
  if (context.child.layout.offset.x != 23.0f ||
      context.child.layout.offset.y != 16.0f ||
      context.child.layout.size.width != 125.0f ||
      context.child.layout.size.height != 53.0f) {
    return 106;
  }

  context.measure_called = false;
  context.baseline_called = false;
  context.child_has_measure = false;
  context.root_child_count = 2;
  context.child_has_second_child = false;
  context.root.has_layout = false;
  context.child.has_layout = false;
  context.second_child.has_layout = false;
  SLRustStyleDefault(&context.root.style);
  SLRustStyleDefault(&context.child.style);
  SLRustStyleDefault(&context.second_child.style);
  SLRustLength grid_hidden_columns[2] = {
      {SLRustLengthPoints, 50.0f, 0.0f, false, false},
      {SLRustLengthPoints, 50.0f, 0.0f, false, false},
  };
  SLRustLength grid_hidden_rows[1] = {
      {SLRustLengthPoints, 20.0f, 0.0f, false, false},
  };
  context.root.style.display = SLRustDisplayGrid;
  context.root.style.width.kind = SLRustLengthPoints;
  context.root.style.width.value = 100.0f;
  context.root.style.height.kind = SLRustLengthPoints;
  context.root.style.height.value = 20.0f;
  context.root.style.grid_template_columns = grid_hidden_columns;
  context.root.style.grid_template_columns_len = 2;
  context.root.style.grid_template_rows = grid_hidden_rows;
  context.root.style.grid_template_rows_len = 1;
  context.child.style.display = SLRustDisplayNone;
  context.child.style.width.kind = SLRustLengthPoints;
  context.child.style.width.value = 50.0f;
  context.child.style.height.kind = SLRustLengthPoints;
  context.child.style.height.value = 20.0f;
  SLRustConstraints grid_hidden_constraints{
      {100.0f, SLRustMeasureModeDefinite},
      {20.0f, SLRustMeasureModeDefinite},
  };
  SLRustStatus grid_hidden_status =
      SLRustLayoutExternalWithOwnerConstraintsChecked(
          &caller_abi, &callbacks, 1, grid_hidden_constraints, &out_size);
  if (grid_hidden_status != SLRustStatusOk) {
    return 56;
  }
  if (out_size.width != 100.0f || out_size.height != 20.0f) {
    return 57;
  }
  if (context.child.layout.offset.x != 0.0f ||
      context.child.layout.offset.y != 0.0f ||
      context.child.layout.size.width != 0.0f ||
      context.child.layout.size.height != 0.0f ||
      context.second_child.layout.offset.x != 0.0f ||
      context.second_child.layout.offset.y != 0.0f ||
      context.second_child.layout.size.width != 50.0f ||
      context.second_child.layout.size.height != 20.0f) {
    return 58;
  }
  context.child_has_second_child = false;
  context.child_has_measure = true;
  context.root_child_count = 1;

  SLRustStatus null_callbacks_status =
      SLRustLayoutExternal(nullptr, 1, constraints, &out_size);
  if (null_callbacks_status != SLRustStatusNullPointer) {
    return 15;
  }
  SLRustStatus null_abi_status =
      SLRustLayoutExternalChecked(nullptr, &callbacks, 1, constraints, &out_size);
  if (null_abi_status != SLRustStatusNullPointer) {
    return 21;
  }
  SLRustAbiInfo mismatched_abi_info = caller_abi;
  mismatched_abi_info.version_major += 1;
  SLRustStatus mismatched_abi_status =
      SLRustLayoutExternalChecked(&mismatched_abi_info, &callbacks, 1,
                                  constraints, &out_size);
  if (mismatched_abi_status != SLRustStatusAbiMismatch) {
    return 22;
  }

  context.root_leaf_mode = true;
  context.root_has_measure = true;
  context.root_measure_called = false;
  context.root_measured_size.width = 120.0f;
  context.root_measured_size.height = 5.0f;
  SLRustStyleDefault(&context.root.style);
  SLRustConstraints root_constraints{
      {40.0f, SLRustMeasureModeDefinite},
      {0.0f, SLRustMeasureModeIndefinite},
  };
  SLRustStatus owner_constraints_status =
      SLRustLayoutExternalWithOwnerConstraints(&callbacks, 1, root_constraints,
                                               &out_size);
  if (owner_constraints_status != SLRustStatusOk ||
      out_size.width != 120.0f ||
      !context.root_measure_called ||
      context.root_last_measure_constraints.width.mode !=
          SLRustMeasureModeAtMost) {
    return 23;
  }

  context.root_measure_called = false;
  SLRustStatus node_constraints_status =
      SLRustLayoutExternalWithNodeConstraintsChecked(
          &caller_abi, &callbacks, 1, root_constraints, &out_size);
  if (node_constraints_status != SLRustStatusOk ||
      out_size.width != 40.0f ||
      !context.root_measure_called ||
      context.root_last_measure_constraints.width.mode !=
          SLRustMeasureModeDefinite) {
    return 24;
  }

  context.root_leaf_mode = false;
  context.root_has_measure = false;
  SLRustTreeCallbacks invalid_style_callbacks{};
  invalid_style_callbacks.context = &context;
  invalid_style_callbacks.child_count = cpp_link_child_count;
  invalid_style_callbacks.child_at = cpp_link_child_at;
  invalid_style_callbacks.style = cpp_link_invalid_style_callback;
  invalid_style_callbacks.set_layout = cpp_link_set_layout;
  SLRustStatus invalid_style_status =
      SLRustLayoutExternal(&invalid_style_callbacks, 1, constraints, &out_size);
  if (invalid_style_status != SLRustStatusInvalidStyle) {
    return 16;
  }
  return 0;
}
"#;

const C_ABI_SMOKE: &str = r#"
#include "starlight_rust_ffi.h"

#include <stddef.h>

_Static_assert(sizeof(SLRustNodeId) == 8, "node ids stay 64-bit");
_Static_assert(SLRustAbiVersionMajor == 1u, "ABI major version");
_Static_assert(SLRustAbiVersionMinor == 15u, "ABI minor version");
_Static_assert(SLRustAbiVersionPatch == 0u, "ABI patch version");
_Static_assert(SLRustStatusOk == 0, "status ok ABI value");
_Static_assert(SLRustStatusPanic == 5, "status panic ABI value");
_Static_assert(SLRustStatusAbiMismatch == 6, "status ABI mismatch value");
_Static_assert(SLRustStatusDisabled == 7, "status disabled ABI value");
_Static_assert(SLRustStatusUnsupportedTree == 8,
               "status unsupported tree ABI value");
_Static_assert(SLRustStatusFixedNodeSetMismatch == 9,
               "status fixed node set mismatch ABI value");
_Static_assert(SLRustMeasureModeAtMost == 2, "measure mode ABI value");
_Static_assert(SLRustLengthFitContent == 6, "length kind ABI value");
_Static_assert(SLRustLengthMinContent == 7, "length kind ABI value");
_Static_assert(SLRustOptionalUnset == -1, "optional sentinel ABI value");
_Static_assert(SLRustGridLineAuto == 0, "grid auto line ABI value");
_Static_assert(SLRustDisplayGrid == 5, "display ABI value");
_Static_assert(SLRustPositionSticky == 4, "position ABI value");
_Static_assert(SLRustBoxSizingBorderBox == 1, "box sizing ABI value");
_Static_assert(SLRustDirectionRtl == 1, "direction ABI value");
_Static_assert(SLRustFlexDirectionColumnReverse == 3,
               "flex direction ABI value");
_Static_assert(SLRustLinearOrientationColumnReverse == 7,
               "linear orientation ABI value");
_Static_assert(SLRustFlexWrapWrapReverse == 2, "flex wrap ABI value");
_Static_assert(SLRustJustifyContentSpaceEvenly == 8,
               "justify content ABI value");
_Static_assert(SLRustAlignItemsBaseline == 6, "align items ABI value");
_Static_assert(SLRustAlignContentStretch == 6, "align content ABI value");
_Static_assert(SLRustAlignContentEnd == 8, "align content end ABI value");
_Static_assert(SLRustJustifyItemsEnd == 4, "justify items ABI value");
_Static_assert(SLRustGridAutoFlowColumnDense == 4, "grid auto flow ABI value");
_Static_assert(SLRustRelativeCenterBoth == 3, "relative center ABI value");
_Static_assert(SLRustLinearGravityCenter == 10, "linear gravity ABI value");
_Static_assert(SLRustLinearLayoutGravityEnd == 12,
               "linear layout gravity ABI value");
_Static_assert(SLRustLinearCrossGravityStretch == 4,
               "linear cross gravity ABI value");
_Static_assert(SLRustListComponentTypeNone == -1,
               "list component none ABI value");
_Static_assert(SLRustListComponentTypeDefault == 3,
               "list component default ABI value");

_Static_assert(offsetof(SLRustAbiInfo, version_major) == 0,
               "ABI info version remains the first field");
_Static_assert(offsetof(SLRustAbiInfo, size_of_abi_info) >
                   offsetof(SLRustAbiInfo, version_patch),
               "ABI info reports its own size after version fields");
_Static_assert(offsetof(SLRustAbiInfo, size_of_length) >
                   offsetof(SLRustAbiInfo, align_of_abi_info),
               "ABI info reports struct sizes after version fields");
_Static_assert(offsetof(SLRustAbiInfo, size_of_tree_callbacks) >
                   offsetof(SLRustAbiInfo, size_of_style),
               "ABI info reports callback size after style size");
_Static_assert(offsetof(SLRustLength, kind) == 0,
               "length kind remains the first field");
_Static_assert(offsetof(SLRustLength, has_percentage) >
                   offsetof(SLRustLength, has_base),
               "length percentage-presence bit follows base-presence bit");
_Static_assert(offsetof(SLRustStyle, display) == 0,
               "style display remains the first field");
_Static_assert(offsetof(SLRustStyle, has_explicit_direction) >
                   offsetof(SLRustStyle, visibility),
               "style explicit-direction flag follows visibility");
_Static_assert(offsetof(SLRustStyle, visibility) >
                   offsetof(SLRustStyle, direction),
               "style visibility follows direction");
_Static_assert(offsetof(SLRustStyle, width) >
                   offsetof(SLRustStyle, has_explicit_direction),
               "style dimensions follow base enum fields and direction metadata");
_Static_assert(offsetof(SLRustStyle, grid_template_columns_len) >
                   offsetof(SLRustStyle, grid_template_columns),
               "grid template pointer is followed by its length");
_Static_assert(offsetof(SLRustTreeCallbacks, child_count) >
                   offsetof(SLRustTreeCallbacks, context),
               "callbacks keep context first");
_Static_assert(offsetof(SLRustTreeCallbacks, set_layout_with_constraints) >
                   offsetof(SLRustTreeCallbacks, set_layout),
               "constraint-aware layout callback follows base layout callback");

static size_t child_count(void* context, SLRustNodeId node) {
  (void)context;
  (void)node;
  return 0;
}

static SLRustNodeId child_at(void* context, SLRustNodeId node, size_t index) {
  (void)context;
  (void)node;
  (void)index;
  return 0;
}

static bool style_callback(void* context, SLRustNodeId node,
                           SLRustStyle* out_style) {
  (void)context;
  (void)node;
  if (!out_style) {
    return false;
  }
  SLRustStyleDefault(out_style);
  out_style->display = SLRustDisplayFlex;
  out_style->position = SLRustPositionRelative;
  out_style->width = SLRustMakePointsLength(100.0f);
  return true;
}

static void set_layout(void* context, SLRustNodeId node,
                       SLRustLayoutResult layout) {
  (void)context;
  (void)node;
  (void)layout;
}

void starlight_rust_ffi_c_smoke(void) {
  SLRustAbiInfo caller_abi = SLRustMakeCallerAbiInfo();
  SLRustTreeCallbacks callbacks = {0};
  callbacks.child_count = child_count;
  callbacks.child_at = child_at;
  callbacks.style = style_callback;
  callbacks.set_layout = set_layout;

  SLRustLength fit_content_base =
      SLRustMakeFitContentLengthWithBase(8.0f, 25.0f, true);
  SLRustLength calc = SLRustMakeCalcLength(2.0f, 50.0f);
  SLRustLength fr = SLRustMakeFrLength(1.0f);
  SLRustLength max_content = SLRustMakeMaxContentLength();
  SLRustLength min_content = SLRustMakeMinContentLength();
  SLRustLength fit_content = SLRustMakeFitContentLength();
  SLRustLength auto_length = SLRustMakeAutoLength();
  SLRustPoint point = SLRustMakePoint(1.0f, 2.0f);
  SLRustRectF32 rect = SLRustMakeRectF32(1.0f, 2.0f, 3.0f, 4.0f);
  (void)fit_content_base;
  (void)calc;
  (void)fr;
  (void)max_content;
  (void)min_content;
  (void)fit_content;
  (void)auto_length;
  (void)point;
  (void)rect;

  SLRustConstraints constraints = SLRustMakeConstraints(
      SLRustMakeDefiniteConstraint(100.0f),
      SLRustMakeAtMostConstraint(20.0f));
  SLRustSize out_size = SLRustMakeSize(0.0f, 0.0f);
  SLRustStatus status =
      SLRustLayoutExternalChecked(&caller_abi, &callbacks, 1, constraints,
                                  &out_size);
  const char* status_name = SLRustStatusName(SLRustStatusOk);
  status = SLRustLayoutExternalWithOwnerConstraintsChecked(
      &caller_abi, &callbacks, 1, constraints, &out_size);
  status = SLRustLayoutExternalWithNodeConstraintsChecked(
      &caller_abi, &callbacks, 1, constraints, &out_size);
  (void)status_name;
  (void)status;
}
"#;

const CPP_ABI_SMOKE: &str = r#"
#include "starlight_rust_ffi.h"
#include "starlight_rust_ffi_cpp.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

static_assert(sizeof(SLRustNodeId) == 8, "node ids stay 64-bit");
static_assert(SLRustAbiVersionMajor == 1u, "ABI major version");
static_assert(SLRustAbiVersionMinor == 15u, "ABI minor version");
static_assert(SLRustAbiVersionPatch == 0u, "ABI patch version");
static_assert(SLRustStatusNullPointer == 1, "status ABI value");
static_assert(SLRustStatusAbiMismatch == 6, "status ABI mismatch value");
static_assert(SLRustStatusDisabled == 7, "status disabled ABI value");
static_assert(SLRustStatusUnsupportedTree == 8,
              "status unsupported tree ABI value");
static_assert(SLRustStatusFixedNodeSetMismatch == 9,
              "status fixed node set mismatch ABI value");
static_assert(SLRustMeasureModeDefinite == 1, "measure mode ABI value");
static_assert(SLRustLengthCalc == 3, "length kind ABI value");
static_assert(SLRustLengthMinContent == 7, "length kind ABI value");
static_assert(SLRustDisplayLinear == 3, "display ABI value");
static_assert(SLRustPositionAbsolute == 2, "position ABI value");
static_assert(SLRustDirectionRtl == 1, "direction ABI value");
static_assert(SLRustFlexDirectionRowReverse == 1,
              "flex direction ABI value");
static_assert(SLRustLinearLayoutGravityStretch == 10,
              "linear layout gravity ABI value");
static_assert(SLRustListComponentTypeFooter == 1,
              "list component ABI value");

static_assert(offsetof(SLRustAbiInfo, version_major) == 0,
              "ABI info version remains first");
static_assert(offsetof(SLRustAbiInfo, size_of_abi_info) >
                  offsetof(SLRustAbiInfo, version_patch),
              "ABI info own size follows version fields");
static_assert(offsetof(SLRustAbiInfo, size_of_style) >
                  offsetof(SLRustAbiInfo, size_of_layout_result),
              "ABI info style size follows layout size");
static_assert(offsetof(SLRustLayoutResult, offset) == 0,
              "layout result offset remains first");
static_assert(offsetof(SLRustStyle, flex_basis) > offsetof(SLRustStyle, flex_shrink),
              "flex basis follows grow/shrink");
static_assert(offsetof(SLRustStyle, visibility) > offsetof(SLRustStyle, direction),
              "style visibility follows direction");
static_assert(offsetof(SLRustStyle, relative_center) >
                  offsetof(SLRustStyle, relative_layout_once),
              "relative center remains the final relative field");

static_assert(std::is_same<decltype(&SLRustStyleDefault),
                           void (*)(SLRustStyle*)>::value,
              "default-style exported function signature");
static_assert(std::is_same<decltype(&SLRustGetAbiInfo),
                           SLRustStatus (*)(SLRustAbiInfo*)>::value,
              "ABI-info exported function signature");
static_assert(std::is_same<decltype(&SLRustStatusName),
                           const char* (*)(SLRustStatus)>::value,
              "status-name exported function signature");
static_assert(std::is_same<decltype(&SLRustMakeCallerAbiInfo),
                           SLRustAbiInfo (*)()>::value,
              "caller ABI helper signature");
static_assert(std::is_same<decltype(&SLRustMakePointsLength),
                           SLRustLength (*)(float)>::value,
              "points length helper signature");
static_assert(std::is_same<decltype(&SLRustMakeFitContentLengthWithBase),
                           SLRustLength (*)(float, float, bool)>::value,
              "fit-content base length helper signature");
static_assert(std::is_same<decltype(&SLRustMakeMinContentLength),
                           SLRustLength (*)()>::value,
              "min-content length helper signature");
static_assert(std::is_same<decltype(&SLRustMakeConstraints),
                           SLRustConstraints (*)(SLRustSideConstraint,
                                                 SLRustSideConstraint)>::value,
              "constraints helper signature");
static_assert(std::is_same<decltype(&SLRustLayoutExternal),
                           SLRustStatus (*)(const SLRustTreeCallbacks*,
                                             SLRustNodeId,
                                             SLRustConstraints,
                                             SLRustSize*)>::value,
              "external-layout exported function signature");
static_assert(std::is_same<decltype(&SLRustLayoutExternalWithOwnerConstraints),
                           SLRustStatus (*)(const SLRustTreeCallbacks*,
                                             SLRustNodeId,
                                             SLRustConstraints,
                                             SLRustSize*)>::value,
              "owner-constraints external-layout exported function signature");
static_assert(std::is_same<decltype(&SLRustLayoutExternalWithOwnerConstraintsAndDirection),
                           SLRustStatus (*)(const SLRustTreeCallbacks*,
                                             SLRustNodeId,
                                             SLRustConstraints,
                                             int32_t,
                                             SLRustSize*)>::value,
              "owner-direction external-layout exported function signature");
static_assert(std::is_same<decltype(&SLRustLayoutExternalWithNodeConstraints),
                           SLRustStatus (*)(const SLRustTreeCallbacks*,
                                             SLRustNodeId,
                                             SLRustConstraints,
                                             SLRustSize*)>::value,
              "node-constraints external-layout exported function signature");
static_assert(std::is_same<decltype(&SLRustLayoutExternalChecked),
                           SLRustStatus (*)(const SLRustAbiInfo*,
                                             const SLRustTreeCallbacks*,
                                             SLRustNodeId,
                                             SLRustConstraints,
                                             SLRustSize*)>::value,
              "checked external-layout exported function signature");
static_assert(std::is_same<decltype(&SLRustLayoutExternalWithOwnerConstraintsChecked),
                           SLRustStatus (*)(const SLRustAbiInfo*,
                                             const SLRustTreeCallbacks*,
                                             SLRustNodeId,
                                             SLRustConstraints,
                                             SLRustSize*)>::value,
              "checked owner-constraints external-layout exported function signature");
static_assert(std::is_same<decltype(&SLRustLayoutExternalWithOwnerConstraintsAndDirectionChecked),
                           SLRustStatus (*)(const SLRustAbiInfo*,
                                             const SLRustTreeCallbacks*,
                                             SLRustNodeId,
                                             SLRustConstraints,
                                             int32_t,
                                             SLRustSize*)>::value,
              "checked owner-direction external-layout exported function signature");
static_assert(std::is_same<decltype(&SLRustLayoutExternalWithNodeConstraintsChecked),
                           SLRustStatus (*)(const SLRustAbiInfo*,
                                             const SLRustTreeCallbacks*,
                                             SLRustNodeId,
                                             SLRustConstraints,
                                             SLRustSize*)>::value,
              "checked node-constraints external-layout exported function signature");

struct CppAdapterSmokeTree {
  size_t ChildCount(SLRustNodeId node) {
    (void)node;
    return 0;
  }

  SLRustNodeId ChildAt(SLRustNodeId node, size_t index) {
    (void)node;
    (void)index;
    return 0;
  }

  bool Style(SLRustNodeId node, SLRustStyle* out_style) {
    if (node != 1 || out_style == nullptr) {
      return false;
    }
    SLRustStyleDefault(out_style);
    out_style->display = SLRustDisplayFlex;
    out_style->width = SLRustMakePointsLength(100.0f);
    return true;
  }

  void SetLayout(SLRustNodeId node, SLRustLayoutResult layout) {
    (void)node;
    (void)layout;
  }

  bool HasMeasure(SLRustNodeId node) {
    (void)node;
    return false;
  }

  bool Measure(SLRustNodeId node, SLRustConstraints constraints,
               SLRustSize* out_size) {
    (void)node;
    (void)constraints;
    (void)out_size;
    return false;
  }

  bool PhysicalPixelsPerLayoutUnit(SLRustNodeId node, float* out_value) {
    if (node != 1 || out_value == nullptr) {
      return false;
    }
    *out_value = 2.0f;
    return true;
  }
};

static_assert(std::is_same<decltype(&lynx::starlight::rust_ffi::MakeTreeCallbacks<CppAdapterSmokeTree>),
                           SLRustTreeCallbacks (*)(CppAdapterSmokeTree*)>::value,
              "C++ tree callback adapter signature");
static_assert(
    std::is_same<decltype(&lynx::starlight::rust_ffi::
                              LayoutExternalWithOwnerConstraintsAndDirectionChecked<
                                  CppAdapterSmokeTree>),
                 SLRustStatus (*)(CppAdapterSmokeTree*, SLRustNodeId,
                                   SLRustConstraints, int32_t,
                                   SLRustSize*)>::value,
    "C++ owner-direction external layout helper signature");

extern "C" size_t cpp_child_count(void* context, SLRustNodeId node) {
  (void)context;
  (void)node;
  return 0;
}

extern "C" SLRustNodeId cpp_child_at(void* context, SLRustNodeId node,
                                     size_t index) {
  (void)context;
  (void)node;
  (void)index;
  return 0;
}

extern "C" bool cpp_style_callback(void* context, SLRustNodeId node,
                                   SLRustStyle* out_style) {
  (void)context;
  (void)node;
  if (out_style == nullptr) {
    return false;
  }
  SLRustStyleDefault(out_style);
  out_style->display = SLRustDisplayGrid;
  out_style->grid_auto_flow = SLRustGridAutoFlowRowDense;
  out_style->list_component_type = SLRustListComponentTypeDefault;
  out_style->width = SLRustMakePointsLength(100.0f);
  return true;
}

extern "C" void cpp_set_layout(void* context, SLRustNodeId node,
                               SLRustLayoutResult layout) {
  (void)context;
  (void)node;
  (void)layout;
}

void starlight_rust_ffi_cpp_smoke() {
  CppAdapterSmokeTree tree;
  SLRustSize adapter_size{};
  const SLRustStatus adapter_status =
      lynx::starlight::rust_ffi::LayoutExternalChecked(
          &tree, 1,
          SLRustMakeConstraints(SLRustMakeDefiniteConstraint(100.0f),
                                SLRustMakeIndefiniteConstraint()),
          &adapter_size);
  (void)adapter_status;

  SLRustAbiInfo caller_abi = SLRustMakeCallerAbiInfo();
  SLRustTreeCallbacks callbacks{};
  callbacks.child_count = cpp_child_count;
  callbacks.child_at = cpp_child_at;
  callbacks.style = cpp_style_callback;
  callbacks.set_layout = cpp_set_layout;

  SLRustConstraints constraints = SLRustMakeConstraints(
      SLRustMakeDefiniteConstraint(100.0f),
      SLRustMakeIndefiniteConstraint());
  SLRustSize out_size = SLRustMakeSize(0.0f, 0.0f);
  const SLRustStatus status =
      SLRustLayoutExternalChecked(&caller_abi, &callbacks, 1, constraints,
                                  &out_size);
  (void)status;
}
"#;

const CPP_PARTIAL_MEASURE_ADAPTER_SMOKE: &str = r#"
#include "starlight_rust_ffi_cpp.h"

#include <cstddef>

struct PartialMeasureTree {
  size_t ChildCount(SLRustNodeId node) {
    (void)node;
    return 0;
  }

  SLRustNodeId ChildAt(SLRustNodeId node, size_t index) {
    (void)node;
    (void)index;
    return 0;
  }

  bool Style(SLRustNodeId node, SLRustStyle* out_style) {
    (void)node;
    (void)out_style;
    return false;
  }

  void SetLayout(SLRustNodeId node, SLRustLayoutResult layout) {
    (void)node;
    (void)layout;
  }

  bool HasMeasure(SLRustNodeId node) {
    (void)node;
    return false;
  }
};

void starlight_rust_ffi_cpp_partial_measure_smoke() {
  PartialMeasureTree tree;
  (void)lynx::starlight::rust_ffi::MakeTreeCallbacks(&tree);
}
"#;

const CPP_MISSING_REQUIRED_ADAPTER_SMOKE: &str = r#"
#include "starlight_rust_ffi_cpp.h"

#include <cstddef>

struct MissingRequiredTree {
  size_t ChildCount(SLRustNodeId node) {
    (void)node;
    return 0;
  }

  SLRustNodeId ChildAt(SLRustNodeId node, size_t index) {
    (void)node;
    (void)index;
    return 0;
  }

  void SetLayout(SLRustNodeId node, SLRustLayoutResult layout) {
    (void)node;
    (void)layout;
  }
};

void starlight_rust_ffi_cpp_missing_required_smoke() {
  MissingRequiredTree tree;
  (void)lynx::starlight::rust_ffi::MakeTreeCallbacks(&tree);
}
"#;
