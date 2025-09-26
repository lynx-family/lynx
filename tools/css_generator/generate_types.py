# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import json
import os
import re
from mako.template import Template

# --- Configuration ---
DEFINES_PATH = os.path.join(
    os.path.dirname(__file__), "value_defines/value_defines.json"
)
CSS_DEFINES_PATH = os.path.join(os.path.dirname(__file__), "css_defines")
TEMPLATE_PATH = os.path.join(os.path.dirname(__file__), "index.d.ts.mako")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "index.d.ts")
# ---------------------


def to_camel_case(kebab_case_str):
    """
    Converts a kebab-case string to camelCase.
    e.g. 'border-top-color' -> 'borderTopColor'
    """
    return re.sub(r"-([a-zA-Z])", lambda m: m.group(1).upper(), kebab_case_str)


def resolve_type(type_str, values_map, resolved_cache):
    """
    Recursively resolves a value type to its primitive TypeScript types.
    """
    if type_str in resolved_cache:
        return resolved_cache[type_str]

    # Handle direct primitives in the syntax string like '<length> | 0'
    if type_str not in values_map:
        # It's a complex syntax like `url(<string>)` or `linear-gradient(...)`
        if "(" in type_str:
            return "(string & {})"

        if type_str == "string":
            return "(string & {})"

        # It's a keyword if it's not a custom type (like <length>)
        if not type_str.startswith("<"):
            if type_str == "number":
                return "(number & {})"
            elif type_str == "string":
                return "(string & {})"
            else:
                try:
                    # Check if it's a numeric literal
                    float(type_str)
                    return type_str
                except ValueError:
                    # It's a keyword, wrap in single quotes
                    return f"'{type_str}'"
        # It's a custom type like <angle> that is not in value_defines.json, return as is
        return type_str

    definition = values_map[type_str]

    # If the definition itself is a complex function, treat as string
    if "(" in definition:
        resolved_cache[type_str] = "(string & {})"
        return "(string & {})"

    parts = [part.strip() for part in definition.split("|")]

    resolved_parts = [resolve_type(part, values_map, resolved_cache) for part in parts]

    # Flatten, unique, and join types
    final_types = set()
    for p in resolved_parts:
        for t in p.split("|"):
            final_types.add(t.strip())

    result = " | ".join(sorted(list(final_types)))
    resolved_cache[type_str] = result
    return result


def main():
    # TODO: Auto gen comment for documentation according to the css_defines file.
    """
    Main function to generate the d.ts file.
    """
    print(f"Loading value definitions from: {DEFINES_PATH}")
    with open(DEFINES_PATH, "r", encoding="utf-8") as f:
        defines = json.load(f)

    values_map = defines.get("values", {})
    resolved_cache = {}

    processed_properties = {}
    print(f"Processing property definitions from: {CSS_DEFINES_PATH}")
    properties_with_empty_desc = []

    for filename in sorted(os.listdir(CSS_DEFINES_PATH)):
        if not filename.endswith(".json"):
            continue

        file_path = os.path.join(CSS_DEFINES_PATH, filename)
        with open(file_path, "r", encoding="utf-8") as f:
            try:
                prop_define = json.load(f)
            except json.JSONDecodeError:
                print(f"Warning: Could not decode JSON from {filename}")
                continue

        prop_name = prop_define.get("name")
        if not prop_name:
            print(f"Warning: 'name' not found in {filename}, skipping.")
            continue

        prop_desc = prop_define.get("desc", "").strip()
        if not prop_desc:
            properties_with_empty_desc.append(prop_name)

        print(f"- Processing {prop_name} from {filename}")
        prop_syntax = prop_define.get("formal_syntax")
        original_syntax = prop_syntax

        final_types = set()
        if not prop_syntax:
            final_types.add("(string & {})")
            final_types.add("(number & {})")
        else:
            # Strip range specifiers like [0,∞] from inside type definitions
            prop_syntax = re.sub(r"(<[\w-]+)\s*\[[^\]]+\](>)", r"\1\2", prop_syntax)

            # Handle syntax multipliers like {1,2}
            has_multiplier = re.search(r"\{[0-9,]+\}", prop_syntax)
            if has_multiplier:
                prop_syntax = re.sub(r"\{[0-9,]+\}", "", prop_syntax).strip()

            # Handle hash multiplier for comma-separated lists
            has_hash_multiplier = prop_syntax.endswith("#")
            if has_hash_multiplier:
                prop_syntax = prop_syntax[:-1].strip()

            # More aggressive cleanup of the syntax string to isolate types and keywords.
            # This will find all <type> definitions and also standalone keywords.
            # We replace grammar symbols with spaces to help with splitting.
            cleaned_syntax = re.sub(r"[[\]?|/]", " ", prop_syntax)
            syntax_parts = re.findall(r"<[^>]+>|[\w\d.-]+", cleaned_syntax)

            resolved_syntax_parts = [
                resolve_type(part, values_map, resolved_cache) for part in syntax_parts
            ]

            # Flatten, unique, and join types
            for p in resolved_syntax_parts:
                for t in p.split("|"):
                    final_types.add(t.strip())

            if has_multiplier or has_hash_multiplier or '[' in original_syntax:
                # For properties that can have multiple values (e.g., `overflow: hidden visible`),
                # we also add a general string type to cover all combinations.
                final_types.add("(string & {})")

        # Allow properties to be explicitly set to undefined.
        final_types.add("undefined")

        ts_type = " | ".join(sorted(list(final_types)))

        camel_prop_name = to_camel_case(prop_name)
        processed_properties[camel_prop_name] = {
            "type": ts_type,
            "desc": prop_desc,
            "syntax": original_syntax,
            "name": prop_name,
        }

    print(f"Loading Mako template from: {TEMPLATE_PATH}")
    template = Template(filename=TEMPLATE_PATH)

    print("Generating index.d.ts content...")
    # The template expects a dictionary of properties
    output_content = template.render(properties=processed_properties)

    with open(OUTPUT_PATH, "w", encoding="utf-8") as f:
        f.write(output_content)

    if properties_with_empty_desc:
        print("\nProperties with empty 'desc':")
        for prop_name in properties_with_empty_desc:
            print(f"- {prop_name}")

    print(f"\n✅ Successfully generated {OUTPUT_PATH}")


if __name__ == "__main__":
    main()