// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use std::collections::{BTreeMap, BTreeSet};
use std::fs;
use std::path::{Path, PathBuf};

const REQUIRED_SECTIONS: [&str; 9] = [
    "9.1 Initial Setup",
    "9.2 Line Length Determination",
    "9.3 Main Size Determination",
    "9.4 Cross Size Determination",
    "9.5 Main-Axis Alignment",
    "9.6 Cross-Axis Alignment",
    "9.7 Resolving Flexible Lengths",
    "9.8 Definite and Indefinite Sizes",
    "9.9 Intrinsic Sizes",
];

const REQUIRED_ALIGNMENT_CLAUSES: [&str; 8] = [
    "CSS Flexbox §8.1 auto margins",
    "CSS Flexbox §8.2 / §9.5 main-axis distribution",
    "CSS Flexbox §8.3 / §9.6 cross-axis self-alignment",
    "CSS Flexbox §8.3 baseline self-alignment",
    "CSS Flexbox §8.4 align-content and multi-line packing",
    "CSS Flexbox §8.5 flex container baselines",
    "Text layout boundary",
    "LayoutTree::measure",
];

const REQUIRED_INITIAL_SETUP_CLAUSES: [&str; 4] = [
    "CSS Flexbox §4 flex items",
    "CSS Flexbox §4.1 absolutely-positioned flex children",
    "CSS Flexbox §9.1 initial setup",
    "anonymous text item construction",
];

const REQUIRED_LINE_LENGTH_CLAUSES: [&str; 7] = [
    "CSS Flexbox §9.2 available main and cross space",
    "CSS Flexbox §9.2 definite flex basis",
    "CSS Flexbox §9.2 aspect-ratio flex base",
    "CSS Flexbox §9.2 content flex basis into available space",
    "CSS Flexbox §9.2 min/max ignored for flex base size",
    "CSS Flexbox §9.2 hypothetical main size clamp",
    "CSS Flexbox §9.2 flex container main size",
];

const REQUIRED_MAIN_SIZE_CLAUSES: [&str; 7] = [
    "CSS Flexbox §9.3 single-line collection",
    "CSS Flexbox §9.3 multi-line collection until overflow",
    "CSS Flexbox §9.3 oversized first item alone",
    "CSS Flexbox §9.3 outer hypothetical main size",
    "CSS Flexbox §9.3 exact-fit zero-sized flex item",
    "CSS Flexbox §9.3 repeat line collection",
    "CSS Flexbox §9.3 resolve flexible lengths",
];

const REQUIRED_CROSS_SIZE_CLAUSES: [&str; 12] = [
    "CSS Flexbox §9.4 hypothetical cross size",
    "CSS Flexbox §9.4 single-line definite cross size",
    "CSS Flexbox §9.4 baseline cross-size contribution",
    "CSS Flexbox §9.4 largest outer hypothetical cross size",
    "CSS Flexbox §9.4 single-line min/max cross clamp",
    "CSS Flexbox §9.4 align-content stretch line expansion",
    "CSS Flexbox §9.4 visibility collapse struts",
    "Style::visibility",
    "collapse-strut storage",
    "CSS Flexbox §9.4 stretched flex item used cross size",
    "CSS Flexbox §9.4 stretch relayout with definite cross size",
    "Starlight stretch + aspect-ratio investigation",
];

const REQUIRED_9_4_STRETCH_ASPECT_RATIO_DECISION: [&str; 3] = [
    "For this §9.4 stretch + preferred-aspect-ratio case",
    "Rust follows the W3C cross-size step",
    "not a reason to preserve the previous Rust mismatch",
];

#[test]
fn flexbox_layout_algorithm_inventory_tracks_every_w3c_step() {
    let inventory = read_inventory();
    let entries = inventory_entries(&inventory);
    let actual_sections = entries.keys().cloned().collect::<BTreeSet<_>>();
    let expected_sections = REQUIRED_SECTIONS
        .into_iter()
        .map(str::to_owned)
        .collect::<BTreeSet<_>>();

    assert_eq!(
        expected_sections, actual_sections,
        "W3C flexbox layout algorithm inventory must list exactly sections 9.1 through 9.9"
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
            !entry.tests.is_empty(),
            "{section} must list Rust test evidence"
        );
        if entry.status.as_deref() == Some("partial") {
            assert!(
                !entry.gaps.is_empty(),
                "{section} must describe remaining W3C audit or implementation gaps while partial"
            );
        }
    }
}

#[test]
fn flexbox_inventory_records_9_4_stretch_aspect_ratio_decision() {
    let inventory = normalize_whitespace(&read_inventory());
    let missing = REQUIRED_9_4_STRETCH_ASPECT_RATIO_DECISION
        .into_iter()
        .filter(|clause| !inventory.contains(clause))
        .collect::<Vec<_>>();

    assert!(
        missing.is_empty(),
        "§9.4 stretch + aspect-ratio decision is missing clauses:\n{}",
        missing.join("\n")
    );
}

#[test]
fn flexbox_alignment_inventory_traces_every_w3c_alignment_clause() {
    let inventory = read_inventory();
    let missing = REQUIRED_ALIGNMENT_CLAUSES
        .into_iter()
        .filter(|clause| !inventory.contains(clause))
        .collect::<Vec<_>>();

    assert!(
        missing.is_empty(),
        "W3C flexbox alignment trace is missing clauses:\n{}",
        missing.join("\n")
    );
}

#[test]
fn flexbox_initial_setup_inventory_traces_every_w3c_clause() {
    let inventory = read_inventory();
    let missing = REQUIRED_INITIAL_SETUP_CLAUSES
        .into_iter()
        .filter(|clause| !inventory.contains(clause))
        .collect::<Vec<_>>();

    assert!(
        missing.is_empty(),
        "W3C flexbox initial setup trace is missing clauses:\n{}",
        missing.join("\n")
    );
}

#[test]
fn flexbox_line_length_inventory_traces_every_w3c_clause() {
    let inventory = read_inventory();
    let missing = REQUIRED_LINE_LENGTH_CLAUSES
        .into_iter()
        .filter(|clause| !inventory.contains(clause))
        .collect::<Vec<_>>();

    assert!(
        missing.is_empty(),
        "W3C flexbox line length trace is missing clauses:\n{}",
        missing.join("\n")
    );
}

#[test]
fn flexbox_main_size_inventory_traces_every_w3c_clause() {
    let inventory = read_inventory();
    let missing = REQUIRED_MAIN_SIZE_CLAUSES
        .into_iter()
        .filter(|clause| !inventory.contains(clause))
        .collect::<Vec<_>>();

    assert!(
        missing.is_empty(),
        "W3C flexbox main size trace is missing clauses:\n{}",
        missing.join("\n")
    );
}

#[test]
fn flexbox_cross_size_inventory_traces_every_w3c_clause() {
    let inventory = read_inventory();
    let missing = REQUIRED_CROSS_SIZE_CLAUSES
        .into_iter()
        .filter(|clause| !inventory.contains(clause))
        .collect::<Vec<_>>();

    assert!(
        missing.is_empty(),
        "W3C flexbox cross size trace is missing clauses:\n{}",
        missing.join("\n")
    );
}

#[test]
fn flexbox_layout_algorithm_inventory_targets_existing_symbols() {
    let manifest_dir = Path::new(env!("CARGO_MANIFEST_DIR"));
    let workspace_dir = manifest_dir
        .join("../..")
        .canonicalize()
        .expect("resolve Rust workspace");
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
        "flexbox algorithm inventory targets must resolve:\n{}",
        missing.join("\n")
    );
}

#[test]
fn flexbox_algorithm_debt_prevents_complete_inventory_status() {
    let entries = inventory_entries(&read_inventory());
    let engine = read_layout_engine();

    if has_known_flexbox_algorithm_debt(&engine) {
        let complete_sections = entries
            .values()
            .filter(|entry| entry.status.as_deref() == Some("complete"))
            .map(|entry| entry.section.as_str())
            .collect::<Vec<_>>();
        assert!(
            complete_sections.is_empty(),
            "sections cannot be marked complete while known flexbox algorithm debt remains: {}",
            complete_sections.join(", ")
        );
    }
}

#[test]
fn known_flexbox_algorithm_debt_markers_are_tracked() {
    let inventory = read_inventory();
    let engine = read_layout_engine();

    assert!(
        !inventory.contains("apply_post_flex_resolve_target_adjustments"),
        "inventory must not point at the removed post-resolve target adjustment hook"
    );
    assert!(
        !engine.contains("fn apply_post_flex_resolve_target_adjustments"),
        "post-resolve flex target adjustments must not be reintroduced"
    );
    assert!(
        !inventory.contains("crates/starlight_layout/src/engine.rs::FlexBasisFallbacks"),
        "inventory must not point at the removed flex-basis fallback flag struct"
    );
    assert!(
        !engine.contains("struct FlexBasisFallbacks"),
        "flex-basis fallback flags must not be reintroduced"
    );

    let documented_percent_basis_adjustments = documented_debt_count(
        &inventory,
        "post-resolve percent-basis adjustment functions",
    )
    .expect("documented percent-basis adjustment count");
    assert_eq!(
        documented_percent_basis_adjustments,
        post_resolve_percent_basis_adjustment_count(&engine),
        "inventory must track the current count of post-resolve percent-basis adjustment functions"
    );

    let documented_fallback_flags = documented_debt_count(&inventory, "FlexBasisFallbacks flags")
        .expect("documented FlexBasisFallbacks flag count");
    assert_eq!(
        documented_fallback_flags,
        flex_basis_fallback_flag_count(&engine),
        "inventory must track the current count of FlexBasisFallbacks flags"
    );
}

#[derive(Debug, Default)]
struct InventoryEntry {
    section: String,
    status: Option<String>,
    implementations: Vec<String>,
    tests: Vec<String>,
    gaps: Vec<String>,
}

fn read_inventory() -> String {
    fs::read_to_string(Path::new(env!("CARGO_MANIFEST_DIR")).join("FLEXBOX_ALGORITHM_COVERAGE.md"))
        .expect("read flexbox algorithm coverage inventory")
}

fn normalize_whitespace(source: &str) -> String {
    source.split_whitespace().collect::<Vec<_>>().join(" ")
}

fn read_layout_engine() -> String {
    let workspace = workspace_dir();
    [
        "crates/starlight_layout/src/engine.rs",
        "crates/starlight_layout/src/engine/flex.rs",
    ]
    .into_iter()
    .map(|file| {
        fs::read_to_string(workspace.join(file))
            .unwrap_or_else(|error| panic!("read Rust layout engine source `{file}`: {error}"))
    })
    .collect::<Vec<_>>()
    .join("\n")
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

fn has_known_flexbox_algorithm_debt(engine: &str) -> bool {
    engine.contains("fn apply_post_flex_resolve_target_adjustments")
        || engine.contains("struct FlexBasisFallbacks")
        || post_resolve_percent_basis_adjustment_count(engine) > 0
}

fn post_resolve_percent_basis_adjustment_count(engine: &str) -> usize {
    engine
        .lines()
        .filter(|line| {
            let trimmed = line.trim_start();
            (trimmed.starts_with("fn restore_") || trimmed.starts_with("fn redistribute_"))
                && trimmed.contains("percent_basis")
        })
        .count()
}

fn flex_basis_fallback_flag_count(engine: &str) -> usize {
    let Some(struct_start) = engine.find("struct FlexBasisFallbacks {") else {
        return 0;
    };
    let fields = &engine[struct_start..];
    let struct_end = fields.find("\n}").expect("FlexBasisFallbacks ends");
    fields[..struct_end]
        .lines()
        .filter(|line| {
            let trimmed = line.trim();
            trimmed.ends_with(": bool,") || trimmed == "bool,"
        })
        .count()
}

fn documented_debt_count(inventory: &str, label: &str) -> Option<usize> {
    inventory.lines().find_map(|line| {
        let label_start = line.find(label)?;
        let count_text = line[..label_start]
            .split(|ch: char| !ch.is_ascii_digit())
            .rfind(|part| !part.is_empty())?;
        count_text.parse().ok()
    })
}
