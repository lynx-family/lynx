#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

"""Local audit tool comparing CSS property defines against Lynx C++ engine sources."""

import argparse
import glob
import json
import os
import re
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
LYNX_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, os.pardir, os.pardir))

DEFAULT_CSS_DEFINES_DIR = os.path.join(SCRIPT_DIR, "css_defines")
DEFAULT_CSS_PROPERTY_CC = os.path.join(
    LYNX_ROOT, "core", "renderer", "css", "css_property.cc"
)
DEFAULT_COMPUTED_STYLE_H = os.path.join(
    LYNX_ROOT, "core", "renderer", "css", "computed_css_style.h"
)
DEFAULT_DEFAULT_COMPUTED_H = os.path.join(
    LYNX_ROOT, "core", "style", "default_computed_style.h"
)
DEFAULT_DEFAULT_LAYOUT_H = os.path.join(
    LYNX_ROOT, "core", "renderer", "starlight", "style", "default_layout_style.h"
)
DEFAULT_CONFIG_H = os.path.join(
    LYNX_ROOT, "core", "renderer", "tasm", "config.h"
)
DEFAULT_PARSER_DIR = os.path.join(LYNX_ROOT, "core", "renderer", "css", "parser")

GENERATED_TYPES = frozenset(
    {
        "color",
        "length",
        "time",
        "number",
        "non-negative-number",
        "enum",
        "string",
        "bool",
        "border-width",
        "border-style",
        "timing-function",
        "animation-property",
    }
)

# Mapping from CSS property name to (C++ default constant name, expected string value).
# Only includes unambiguous defaults where a direct constant exists in C++ headers.
CPP_DEFAULT_OVERRIDES = {
    "opacity": ("DEFAULT_OPACITY", {"1", "1.0"}),
    "overflow": ("DEFAULT_OVERFLOW", {"hidden"}),
    "overflow-x": ("DEFAULT_OVERFLOW", {"hidden"}),
    "overflow-y": ("DEFAULT_OVERFLOW", {"hidden"}),
    "text-align": ("DEFAULT_TEXT_ALIGN", {"start"}),
    "text-overflow": ("DEFAULT_TEXT_OVERFLOW", {"clip"}),
    "font-weight": ("DEFAULT_FONT_WEIGHT", {"normal"}),
    "font-style": ("DEFAULT_FONT_STYLE", {"normal"}),
    "white-space": ("DEFAULT_WHITE_SPACE", {"normal"}),
    "word-break": ("DEFAULT_WORD_BREAK", {"normal"}),
    "outline-style": ("DEFAULT_OUTLINE_STYLE", {"none", "solid"}),
    "visibility": ("DEFAULT_VISIBILITY", {"visible"}),
    "vertical-align": ("DEFAULT_VERTICAL_ALIGN", {"default"}),
    "hyphens": ("DEFAULT_HYPHENS", {"manual"}),
    "text-spacing-trim": ("DEFAULT_TEXT_SPACING_TRIM", {"normal"}),
    "font-optical-sizing": ("DEFAULT_FONT_OPTICAL_SIZING", {"none"}),
    "flex-grow": ("SL_DEFAULT_FLEX_GROW", {"0", "0.0"}),
    "flex-shrink": ("SL_DEFAULT_FLEX_SHRINK", {"1", "1.0"}),
    "order": ("SL_DEFAULT_ORDER", {"0", "0.0"}),
    "flex-direction": ("SL_DEFAULT_FLEX_DIRECTION", {"row"}),
    "flex-wrap": ("SL_DEFAULT_FLEX_WRAP", {"nowrap"}),
    "justify-content": ("SL_DEFAULT_JUSTIFY_CONTENT", {"stretch"}),
    "align-items": ("SL_DEFAULT_ALIGN_ITEMS", {"stretch"}),
    "align-self": ("SL_DEFAULT_ALIGN_SELF", {"auto"}),
    "align-content": ("SL_DEFAULT_ALIGN_CONTENT", {"stretch"}),
    "display": ("SL_DEFAULT_DISPLAY", {"auto"}),
    "position": ("SL_DEFAULT_POSITION", {"relative"}),
    "direction": ("SL_DEFAULT_DIRECTION", {"normal"}),
    "linear-orientation": ("SL_DEFAULT_LINEAR_ORIENTATION", {"vertical"}),
    "linear-layout-gravity": ("SL_DEFAULT_LINEAR_LAYOUT_GRAVITY", {"none"}),
    "linear-gravity": ("SL_DEFAULT_LINEAR_GRAVITY", {"none"}),
    "linear-cross-gravity": ("SL_DEFAULT_LINEAR_CROSS_GRAVITY", {"none"}),
    "relative-center": ("SL_DEFAULT_RELATIVE_CENTER", {"none"}),
    "box-sizing": ("SL_DEFAULT_BOX_SIZING", {"auto"}),
    "justify-self": ("SL_DEFAULT_JUSTIFY_SELF", {"auto"}),
    "justify-items": ("SL_DEFAULT_JUSTIFY_ITEMS", {"stretch"}),
    "grid-auto-flow": ("SL_DEFAULT_GRID_AUTO_FLOW", {"row"}),
    "relative-layout-once": ("SL_DEFAULT_RELATIVE_LAYOUT_ONCE", {"true"}),
    "-x-box-propagate-min-constraints": (
        "SL_DEFAULT_X_BOX_PROPAGATE_MIN_CONSTRAINTS",
        {"false"},
    ),
    "-x-box-match-parent-size": ("SL_DEFAULT_X_BOX_MATCH_PARENT_SIZE", {"false"}),
}


def to_hump(name):
    return "".join(part[:1].upper() + part[1:] for part in re.split(r"[-_]", name) if part)


def _read_text(path):
    with open(path, "r", encoding="utf-8") as handle:
        return handle.read()


def _load_defines(css_defines_dir):
    defines = []
    for file_path in sorted(glob.glob(os.path.join(css_defines_dir, "*.json"))):
        with open(file_path, "r", encoding="utf-8") as handle:
            defines.append((os.path.basename(file_path), json.load(handle)))
    return defines


def _parse_shorthand_set(css_property_cc_path):
    content = _read_text(css_property_cc_path)
    match = re.search(r"shorthandCSSProperties\{([^}]+)\}", content, re.S)
    if not match:
        return set()
    return set(re.findall(r"kPropertyID([A-Za-z0-9_]+)", match.group(1)))


def _parse_platform_properties(computed_style_h_path):
    content = _read_text(computed_style_h_path)
    match = re.search(r"#define FOREACH_PLATFORM_PROPERTY\(V\)(.*?)\n\n", content, re.S)
    if not match:
        return set()
    return set(re.findall(r"V\(([A-Za-z0-9_]+)\)", match.group(1)))


def _parse_handler_mentions(parser_dir):
    mentions = set()
    for file_path in glob.glob(os.path.join(parser_dir, "*_handler.*")):
        if "_unittest" in file_path:
            continue
        content = _read_text(file_path)
        mentions.update(re.findall(r"kPropertyID([A-Za-z0-9_]+)", content))
    return mentions


def _parse_default_font_size_dp(config_h_path):
    if not os.path.exists(config_h_path):
        return None
    content = _read_text(config_h_path)
    match = re.search(r"#define\s+DEFAULT_FONT_SIZE_DP\s+(\d+)", content)
    return match.group(1) if match else None


def audit_css_defines(
    css_defines_dir=DEFAULT_CSS_DEFINES_DIR,
    css_property_cc=DEFAULT_CSS_PROPERTY_CC,
    computed_style_h=DEFAULT_COMPUTED_STYLE_H,
    default_computed_h=DEFAULT_DEFAULT_COMPUTED_H,
    default_layout_h=DEFAULT_DEFAULT_LAYOUT_H,
    config_h=DEFAULT_CONFIG_H,
    parser_dir=DEFAULT_PARSER_DIR,
):
    defines = _load_defines(css_defines_dir)
    cc_shorthands = _parse_shorthand_set(css_property_cc)
    platform_props = _parse_platform_properties(computed_style_h)
    handler_mentions = _parse_handler_mentions(parser_dir)
    default_cpp_text = _read_text(default_computed_h) + "\n" + _read_text(default_layout_h)
    default_font_size_dp = _parse_default_font_size_dp(config_h)

    report = {
        "total_properties": len(defines),
        "shorthand_mismatches": [],
        "platform_layout_conflicts": [],
        "default_value_mismatches": [],
        "enum_syntax_mismatches": [],
        "non_enum_with_values": [],
        "missing_handler_references": [],
        "missing_metadata": {
            "desc": [],
            "formal_syntax": [],
            "compat_data": [],
        },
    }

    for filename, data in defines:
        name = data.get("name", "")
        hump = to_hump(name)
        prop_type = data.get("type", "")
        is_shorthand = bool(data.get("is_shorthand", False))

        if is_shorthand and hump not in cc_shorthands:
            report["shorthand_mismatches"].append(
                f"{filename}: is_shorthand=true in JSON, not in css_property.cc shorthandCSSProperties"
            )
        elif not is_shorthand and hump in cc_shorthands:
            report["shorthand_mismatches"].append(
                f"{filename}: is_shorthand=false in JSON, but present in css_property.cc shorthandCSSProperties"
            )

        if hump in platform_props and data.get("consumption_status") == "layout-only":
            report["platform_layout_conflicts"].append(
                f"{filename}: consumption_status='layout-only' but listed in FOREACH_PLATFORM_PROPERTY"
            )

        if name in CPP_DEFAULT_OVERRIDES:
            const_name, expected_values = CPP_DEFAULT_OVERRIDES[name]
            if const_name in default_cpp_text:
                actual_default = str(data.get("default_value", ""))
                if actual_default not in expected_values:
                    report["default_value_mismatches"].append(
                        f"{filename}: default_value={actual_default!r}, C++ {const_name} implies {sorted(expected_values)}"
                    )

        if name == "font-size" and default_font_size_dp is not None:
            actual_default = str(data.get("default_value", ""))
            expected_px = f"{default_font_size_dp}px"
            if actual_default != expected_px:
                report["default_value_mismatches"].append(
                    f"{filename}: default_value={actual_default!r}, C++ DEFAULT_FONT_SIZE_DP={default_font_size_dp} ({expected_px!r})"
                )

        values = data.get("values")
        formal_syntax = str(data.get("formal_syntax", "")).strip()
        if prop_type == "enum" and values and formal_syntax:
            val_set = {item["value"] for item in values if isinstance(item, dict) and "value" in item}
            syntax_set = {part.strip() for part in formal_syntax.split("|") if part.strip()}
            if val_set != syntax_set:
                report["enum_syntax_mismatches"].append(
                    f"{filename}: values={sorted(val_set)} vs formal_syntax={sorted(syntax_set)}"
                )

        if prop_type != "enum" and values:
            report["non_enum_with_values"].append(
                f"{filename}: type={prop_type!r} also defines values={[v.get('value') for v in values]}"
            )

        if prop_type not in GENERATED_TYPES and hump not in handler_mentions:
            report["missing_handler_references"].append(
                f"{filename}: type={prop_type!r} has no kPropertyID{hump} reference in parser/*_handler.*"
            )

        if not str(data.get("desc", "")).strip():
            report["missing_metadata"]["desc"].append(filename)
        if not formal_syntax:
            report["missing_metadata"]["formal_syntax"].append(filename)
        if "compat_data" not in data:
            report["missing_metadata"]["compat_data"].append(filename)

    return report


def format_report(report):
    lines = [f"Audited {report['total_properties']} CSS property definitions:"]
    sections = [
        ("Shorthand mismatches vs css_property.cc", report["shorthand_mismatches"]),
        ("Layout-only vs FOREACH_PLATFORM_PROPERTY conflicts", report["platform_layout_conflicts"]),
        ("Default value mismatches vs C++ headers", report["default_value_mismatches"]),
        ("Enum values vs formal_syntax mismatches", report["enum_syntax_mismatches"]),
        ("Non-enum properties with values array", report["non_enum_with_values"]),
        ("Complex properties missing parser handler", report["missing_handler_references"]),
    ]
    for title, items in sections:
        lines.append(f"\n[{title}] ({len(items)})")
        for item in items:
            lines.append(f"  - {item}")

    meta = report["missing_metadata"]
    lines.append(
        "\n[Missing metadata counts] "
        f"desc={len(meta['desc'])}, formal_syntax={len(meta['formal_syntax'])}, compat_data={len(meta['compat_data'])}"
    )
    return "\n".join(lines)


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Audit Lynx CSS define JSON files against C++ engine sources."
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="Output the full audit report as JSON.",
    )
    args = parser.parse_args(argv)

    report = audit_css_defines()
    if args.json:
        print(json.dumps(report, indent=2, ensure_ascii=False))
    else:
        print(format_report(report))
    return 0


if __name__ == "__main__":
    sys.exit(main())
