// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#![forbid(unsafe_code)]

use std::fs;
use std::path::Path;

const STYLE_ENUMS_REQUIRING_NATIVE_COVERAGE: &[&str] = &[
    "JustifyContent",
    "AlignItems",
    "AlignContent",
    "JustifyItems",
    "FlexDirection",
    "FlexWrap",
    "LinearOrientation",
    "GridAutoFlow",
    "RelativeCenter",
    "PositionType",
    "Display",
    "Direction",
    "BoxSizing",
];

const STYLE_DATA_ENUMS_REQUIRING_NATIVE_COVERAGE: &[&str] =
    &["LinearGravity", "LinearLayoutGravity", "LinearCrossGravity"];

#[test]
fn native_head_to_head_explicitly_mentions_each_layout_facing_enum_variant() {
    let manifest_dir = Path::new(env!("CARGO_MANIFEST_DIR"));
    let style_source = fs::read_to_string(manifest_dir.join("../starlight_layout/src/style.rs"))
        .expect("style.rs should be readable");
    let style_data_source =
        fs::read_to_string(manifest_dir.join("../starlight_layout/src/style_data.rs"))
            .expect("style_data.rs should be readable");
    let coverage_source = native_coverage_source(manifest_dir);

    let mut missing_variants = Vec::new();
    collect_missing_enum_variants(
        &style_source,
        STYLE_ENUMS_REQUIRING_NATIVE_COVERAGE,
        &coverage_source,
        &mut missing_variants,
    );
    collect_missing_enum_variants(
        &style_data_source,
        STYLE_DATA_ENUMS_REQUIRING_NATIVE_COVERAGE,
        &coverage_source,
        &mut missing_variants,
    );

    assert!(
        missing_variants.is_empty(),
        "missing explicit native head-to-head coverage for:\n{}",
        missing_variants.join("\n")
    );
}

fn native_coverage_source(manifest_dir: &Path) -> String {
    [
        "native_head_to_head_tests.rs",
        "native_generated_head_to_head_tests.rs",
    ]
    .into_iter()
    .map(|file_name| {
        fs::read_to_string(manifest_dir.join("tests").join(file_name))
            .unwrap_or_else(|error| panic!("{file_name} should be readable: {error}"))
    })
    .collect::<Vec<_>>()
    .join("\n")
}

fn collect_missing_enum_variants(
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
