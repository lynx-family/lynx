// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use std::collections::{BTreeMap, BTreeSet};
use std::fs;
use std::path::{Path, PathBuf};

const REQUIRED_SECTIONS: [&str; 20] = [
    "Linear Items",
    "Linear 1 Initial Setup",
    "Linear 2 Item Measurement",
    "Linear 3 Weighted Main-Size Resolution",
    "Linear 4 Container Size Determination",
    "Linear 5 Main-Axis Alignment",
    "Linear 6 Cross-Axis Alignment",
    "Linear 7 Baseline",
    "Linear 8 Out-of-Flow Children",
    "Relative Items",
    "Relative Reference Resolution",
    "Relative Dependency Ordering",
    "Relative 1 Initial Setup",
    "Relative 2 Initial Child Constraints",
    "Relative 3 Position Equation",
    "Relative 4 One-Pass Relative Layout",
    "Relative 5 Two-Pass Relative Layout",
    "Relative 6 Container Size Determination",
    "Relative 7 Final Item Placement",
    "Relative 8 Out-of-Flow Children",
];

const REQUIRED_LINEAR_HELPERS: [&str; 31] = [
    "crates/starlight_layout/src/engine/linear.rs::layout_linear",
    "crates/starlight_layout/src/engine/linear.rs::linear_child_cross_constraint",
    "crates/starlight_layout/src/engine/linear.rs::resolve_linear_weights",
    "crates/starlight_layout/src/engine/linear.rs::remaining_linear_weight_space",
    "crates/starlight_layout/src/engine/linear.rs::linear_total_main",
    "crates/starlight_layout/src/engine/linear.rs::linear_max_cross",
    "crates/starlight_layout/src/engine/linear.rs::linear_out_of_flow_alignment",
    "crates/starlight_layout/src/engine/linear.rs::child_linear_constraints",
    "crates/starlight_layout/src/engine/linear.rs::linear_fit_content_axis_constraint",
    "crates/starlight_layout/src/engine/linear.rs::default_linear_child_cross_constraint",
    "crates/starlight_layout/src/engine/linear.rs::linear_main_alignment",
    "crates/starlight_layout/src/engine/linear.rs::logic_linear_gravity",
    "crates/starlight_layout/src/engine/linear.rs::linear_gravity_from_justify_content",
    "crates/starlight_layout/src/engine/linear.rs::linear_gravity_is_physical",
    "crates/starlight_layout/src/engine/linear.rs::physical_linear_gravity_to_logic",
    "crates/starlight_layout/src/engine/linear.rs::linear_main_front_is_reversed",
    "crates/starlight_layout/src/engine/linear.rs::linear_cross_front_is_reversed",
    "crates/starlight_layout/src/engine/linear.rs::main_start_margin_with_auto",
    "crates/starlight_layout/src/engine/linear.rs::main_end_margin_with_auto",
    "crates/starlight_layout/src/engine/linear.rs::auto_cross_offset",
    "crates/starlight_layout/src/engine/linear.rs::linear_used_margin",
    "crates/starlight_layout/src/engine/linear.rs::resolved_auto_cross_margins",
    "crates/starlight_layout/src/engine/linear.rs::linear_child_style_override",
    "crates/starlight_layout/src/engine/linear.rs::linear_cross_margin_bound_offset",
    "crates/starlight_layout/src/engine/linear.rs::linear_physical_cross_offset_from_margin_bound",
    "crates/starlight_layout/src/engine/linear.rs::linear_layout_gravity_is_after",
    "crates/starlight_layout/src/engine/linear.rs::linear_layout_gravity_is_center",
    "crates/starlight_layout/src/engine.rs::computed_linear_layout_gravity",
    "crates/starlight_layout/src/engine.rs::layout_display_none_children",
    "crates/starlight_layout/src/engine.rs::ordered_in_flow_children",
    "crates/starlight_layout/src/engine.rs::layout_out_of_flow_children",
];

const REQUIRED_RELATIVE_HELPERS: [&str; 34] = [
    "crates/starlight_layout/src/engine/relative.rs::layout_relative",
    "crates/starlight_layout/src/engine/relative.rs::relative_child_constraints",
    "crates/starlight_layout/src/engine/relative.rs::relative_fit_content_axis_constraint",
    "crates/starlight_layout/src/engine/relative.rs::relative_child_style_override",
    "crates/starlight_layout/src/engine/relative.rs::relative_percent_constraints",
    "crates/starlight_layout/src/engine/relative.rs::remeasure_relative_constrained_items",
    "crates/starlight_layout/src/engine/relative.rs::remeasure_relative_horizontal_proposed_items",
    "crates/starlight_layout/src/engine/relative.rs::relative_width_constraint_from_proposed_size",
    "crates/starlight_layout/src/engine/relative.rs::measure_and_position_relative_once",
    "crates/starlight_layout/src/engine/relative.rs::recompute_relative_final_positions",
    "crates/starlight_layout/src/engine/relative.rs::recompute_relative_axis_positions",
    "crates/starlight_layout/src/engine/relative.rs::relative_axis_constraint_from_positions",
    "crates/starlight_layout/src/engine/relative.rs::position_relative_items",
    "crates/starlight_layout/src/engine/relative.rs::relative_axis_position",
    "crates/starlight_layout/src/engine/relative.rs::relative_start_constraint",
    "crates/starlight_layout/src/engine/relative.rs::relative_end_constraint",
    "crates/starlight_layout/src/engine/relative.rs::relative_reference_position",
    "crates/starlight_layout/src/engine/relative.rs::relative_item_side",
    "crates/starlight_layout/src/engine/relative.rs::relative_order",
    "crates/starlight_layout/src/engine/relative.rs::relative_order_for_scope",
    "crates/starlight_layout/src/engine/relative.rs::relative_dependency_ids",
    "crates/starlight_layout/src/engine/relative.rs::relative_id_index",
    "crates/starlight_layout/src/engine/relative.rs::relative_axis_stretches_to_parent",
    "crates/starlight_layout/src/engine/relative.rs::relative_align_start",
    "crates/starlight_layout/src/engine/relative.rs::relative_align_end",
    "crates/starlight_layout/src/engine/relative.rs::relative_content_extent",
    "crates/starlight_layout/src/engine/relative.rs::relative_content_height_after_recompute",
    "crates/starlight_layout/src/engine/relative.rs::relative_vertical_bounds",
    "crates/starlight_layout/src/engine/relative.rs::RelativeAxisBounds",
    "crates/starlight_layout/src/engine/relative.rs::fn new",
    "crates/starlight_layout/src/engine/relative.rs::fn include",
    "crates/starlight_layout/src/engine.rs::layout_display_none_children",
    "crates/starlight_layout/src/engine.rs::ordered_in_flow_children",
    "crates/starlight_layout/src/engine.rs::layout_out_of_flow_children",
];

#[test]
fn linear_relative_algorithm_inventory_tracks_starlight_spec_sections() {
    let entries = inventory_entries(&read_inventory());
    let actual_sections = entries.keys().cloned().collect::<BTreeSet<_>>();
    let expected_sections = REQUIRED_SECTIONS
        .into_iter()
        .map(str::to_owned)
        .collect::<BTreeSet<_>>();

    assert_eq!(
        expected_sections, actual_sections,
        "linear/relative inventory must list every Starlight spec section"
    );

    for section in REQUIRED_SECTIONS {
        let entry = entries.get(section).expect("required section exists");
        assert!(
            matches!(entry.status.as_deref(), Some("partial" | "complete")),
            "{section} must have a `status` of partial or complete"
        );
        assert!(
            !entry.implementations.is_empty(),
            "{section} must list Rust implementation evidence"
        );
        assert!(
            !entry.cpp_implementations.is_empty(),
            "{section} must list C++ implementation evidence"
        );
        assert!(
            !entry.tests.is_empty(),
            "{section} must list Rust or C++ parity test evidence"
        );
        if entry.status.as_deref() == Some("partial") {
            assert!(
                !entry.gaps.is_empty(),
                "{section} must describe remaining audit or parity gaps while partial"
            );
        }
    }
}

#[test]
fn linear_relative_inventory_targets_existing_symbols() {
    let manifest_dir = Path::new(env!("CARGO_MANIFEST_DIR"));
    let workspace_dir = workspace_dir();
    let entries = inventory_entries(&read_inventory());

    let mut missing = Vec::new();
    for entry in entries.values() {
        for target in &entry.implementations {
            let (file, symbol) = split_target(target);
            let path = workspace_dir.join(file);
            let Ok(source) = fs::read_to_string(&path) else {
                missing.push(format!(
                    "{}: missing implementation file `{file}`",
                    entry.section
                ));
                continue;
            };
            if !source.contains(symbol) {
                missing.push(format!(
                    "{}: implementation `{target}` does not contain symbol `{symbol}`",
                    entry.section
                ));
            }
        }

        for target in &entry.cpp_implementations {
            let (file, symbol) = split_target(target);
            let path = workspace_dir.join(file);
            let Ok(source) = fs::read_to_string(&path) else {
                missing.push(format!(
                    "{}: missing C++ implementation file `{file}`",
                    entry.section
                ));
                continue;
            };
            if !source.contains(symbol) {
                missing.push(format!(
                    "{}: C++ implementation `{target}` does not contain symbol `{symbol}`",
                    entry.section
                ));
            }
        }

        for target in &entry.tests {
            let (file, test_name) = split_target(target);
            let path = manifest_dir.join(file);
            let Ok(source) = fs::read_to_string(&path) else {
                missing.push(format!("{}: missing test file `{file}`", entry.section));
                continue;
            };
            if !rust_test_functions(&source).contains(test_name) {
                missing.push(format!(
                    "{}: test `{target}` does not point to a #[test] function",
                    entry.section
                ));
            }
        }
    }

    assert!(
        missing.is_empty(),
        "linear/relative algorithm inventory targets must resolve:\n{}",
        missing.join("\n")
    );
}

#[test]
fn linear_relative_inventory_traces_current_module_helpers() {
    let inventory = read_inventory();
    let missing = REQUIRED_LINEAR_HELPERS
        .into_iter()
        .chain(REQUIRED_RELATIVE_HELPERS)
        .filter(|target| !inventory.contains(target))
        .collect::<Vec<_>>();

    assert!(
        missing.is_empty(),
        "linear/relative inventory must trace every current module helper:\n{}",
        missing.join("\n")
    );
}

#[test]
fn linear_relative_inventory_records_layout_resolver_and_visibility_boundaries() {
    let inventory = read_inventory();

    for required in [
        "Starlight module specifications, not W3C Recommendations",
        "The layout engine consumes style resolver output",
        "Invalid CSS syntax is not a layout repair case",
        "syntactically valid style value is ignored, mapped, falls back",
        "participates in linear/relative layout",
        "`display: none` direct children are laid out as hidden zero-sized subtrees",
        "`visibility: hidden` and non-flex/table `visibility: collapse` children",
        "remain linear or relative items",
        "standalone C++ public C API still rejects non-visible",
        "visibility_hidden_and_collapse_linear_children_participate_in_layout",
        "visibility_hidden_and_collapse_relative_children_participate_in_dependency_layout",
        "head_to_head_linear_visibility_hidden_and_collapse_participate_in_layout",
        "head_to_head_relative_visibility_hidden_and_collapse_participate_in_dependency_layout",
    ] {
        assert!(
            inventory.contains(required),
            "linear/relative inventory is missing boundary marker `{required}`"
        );
    }
}

#[derive(Debug, Default)]
struct InventoryEntry {
    section: String,
    status: Option<String>,
    implementations: Vec<String>,
    cpp_implementations: Vec<String>,
    tests: Vec<String>,
    gaps: Vec<String>,
}

fn read_inventory() -> String {
    fs::read_to_string(
        Path::new(env!("CARGO_MANIFEST_DIR")).join("LINEAR_RELATIVE_ALGORITHM_COVERAGE.md"),
    )
    .expect("read linear/relative algorithm coverage inventory")
}

fn workspace_dir() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("../..")
        .canonicalize()
        .expect("resolve Rust workspace")
}

fn inventory_entries(inventory: &str) -> BTreeMap<String, InventoryEntry> {
    let mut entries = BTreeMap::new();
    let mut current: Option<InventoryEntry> = None;

    for line in inventory.lines() {
        if let Some(section) = section_header(line) {
            if let Some(entry) = current.take() {
                entries.insert(entry.section.clone(), entry);
            }
            current = Some(InventoryEntry {
                section,
                ..InventoryEntry::default()
            });
            continue;
        }

        let Some(entry) = current.as_mut() else {
            continue;
        };
        if let Some(status) = inventory_value(line, "status") {
            entry.status = Some(status);
        } else if let Some(implementation) = inventory_value(line, "implementation") {
            entry.implementations.push(implementation);
        } else if let Some(cpp_implementation) = inventory_value(line, "cpp-implementation") {
            entry.cpp_implementations.push(cpp_implementation);
        } else if let Some(test) = inventory_value(line, "tests") {
            entry.tests.push(test);
        } else if let Some(gap) = inventory_value(line, "gap") {
            entry.gaps.push(gap);
        }
    }

    if let Some(entry) = current {
        entries.insert(entry.section.clone(), entry);
    }

    entries
}

fn section_header(line: &str) -> Option<String> {
    let line = line.strip_prefix("## `")?;
    let (section, _) = line.split_once('`')?;
    Some(section.to_owned())
}

fn inventory_value(line: &str, key: &str) -> Option<String> {
    let prefix = format!("- `{key}`: ");
    line.strip_prefix(&prefix)
        .map(|value| value.trim().trim_matches('`').to_owned())
}

fn split_target(target: &str) -> (&str, &str) {
    target
        .split_once("::")
        .unwrap_or_else(|| panic!("inventory target `{target}` must contain `::`"))
}

fn rust_test_functions(source: &str) -> BTreeSet<&str> {
    let mut tests = BTreeSet::new();
    let mut previous_line_was_test_attr = false;
    for line in source.lines() {
        let trimmed = line.trim_start();
        if trimmed == "#[test]" {
            previous_line_was_test_attr = true;
            continue;
        }
        if previous_line_was_test_attr && trimmed.starts_with("#[") {
            continue;
        }
        if previous_line_was_test_attr {
            if let Some(name) = trimmed.strip_prefix("fn ") {
                let Some((name, _)) = name.split_once('(') else {
                    previous_line_was_test_attr = false;
                    continue;
                };
                tests.insert(name);
            }
            previous_line_was_test_attr = false;
        }
    }
    tests
}
