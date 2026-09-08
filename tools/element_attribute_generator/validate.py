#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

"""Validate the stable Element attribute ID registry."""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any


LYNX_ROOT = Path(__file__).resolve().parents[2]
GENERATOR_DIR = Path(__file__).resolve().parent
REGISTRY_PATH = GENERATOR_DIR / "element_attribute_index.json"
EXTRACTOR_PATH = GENERATOR_DIR / "extract_element_attributes.js"

MIN_ATTRIBUTE_ID = 1024
MAX_ATTRIBUTE_ID = (1 << 16) - 1
EXPECTED_RESERVED_RANGES = (
    (0, 999, "css_property"),
    (1000, 1023, "element_construction_metadata"),
)


class ValidationError(Exception):
    pass


def read_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as error:
        raise ValidationError(f"invalid JSON in {path}: {error}") from error
    if not isinstance(value, dict):
        raise ValidationError(f"{path} must contain a JSON object")
    return value


def extract_typings(node_binary: str | None = None) -> dict[str, Any]:
    node = node_binary or shutil.which("node")
    if not node:
        raise ValidationError(
            "node is required to parse Element typings; run through tools/env.sh"
        )
    result = subprocess.run(
        [node, str(EXTRACTOR_PATH)],
        cwd=LYNX_ROOT,
        check=False,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        raise ValidationError(
            "Element typing extraction failed:\n"
            + (result.stderr or result.stdout).strip()
        )
    try:
        value = json.loads(result.stdout)
    except json.JSONDecodeError as error:
        raise ValidationError(
            f"Element typing extractor returned invalid JSON: {error}"
        ) from error
    if not isinstance(value, dict):
        raise ValidationError("Element typing extractor returned an invalid model")
    return value


def validate_exact_keys(
    value: dict[str, Any], expected: set[str], location: str
) -> None:
    actual = set(value)
    if actual != expected:
        raise ValidationError(
            f"{location} has invalid keys; "
            f"missing={sorted(expected - actual)}, extra={sorted(actual - expected)}"
        )


def typing_names(source_model: dict[str, Any]) -> set[str]:
    validate_exact_keys(
        source_model, {"schema_version", "source", "attributes"}, "typing model"
    )
    if source_model["schema_version"] != 1:
        raise ValidationError("typing model schema_version must be 1")
    attributes = source_model["attributes"]
    if not isinstance(attributes, list) or not all(
        isinstance(name, str) and name for name in attributes
    ):
        raise ValidationError("typing model attributes must be non-empty strings")
    names = set(attributes)
    if len(names) != len(attributes):
        raise ValidationError("typing model contains duplicate names")
    return names


def validate_registry(
    registry: dict[str, Any],
    source_model: dict[str, Any],
    baseline: dict[str, Any] | None = None,
) -> None:
    validate_exact_keys(
        registry,
        {"schema_version", "id_type", "reserved_ranges", "attributes"},
        "registry",
    )
    if registry["schema_version"] != 1:
        raise ValidationError("schema_version must be 1")
    if registry["id_type"] != "uint16":
        raise ValidationError("id_type must be uint16")

    ranges = registry["reserved_ranges"]
    if not isinstance(ranges, list):
        raise ValidationError("reserved_ranges must be an array")
    normalized_ranges: list[tuple[int, int, str]] = []
    for index, value in enumerate(ranges):
        if not isinstance(value, dict):
            raise ValidationError(f"reserved_ranges[{index}] must be an object")
        validate_exact_keys(value, {"start", "end", "owner"}, f"range {index}")
        start, end, owner = value["start"], value["end"], value["owner"]
        if (
            not isinstance(start, int)
            or isinstance(start, bool)
            or not isinstance(end, int)
            or isinstance(end, bool)
            or not isinstance(owner, str)
            or not owner
            or start < 0
            or end > MAX_ATTRIBUTE_ID
            or start > end
        ):
            raise ValidationError(f"reserved_ranges[{index}] is invalid")
        normalized_ranges.append((start, end, owner))
    if tuple(normalized_ranges) != EXPECTED_RESERVED_RANGES:
        raise ValidationError(
            "reserved_ranges must preserve CSS 0..999 and construction "
            "metadata 1000..1023"
        )

    source_names = typing_names(source_model)
    attributes = registry["attributes"]
    if not isinstance(attributes, list):
        raise ValidationError("attributes must be an array")
    ids: set[int] = set()
    names: set[str] = set()
    active_names: set[str] = set()
    previous_id = MIN_ATTRIBUTE_ID - 1
    for index, value in enumerate(attributes):
        location = f"attributes[{index}]"
        if not isinstance(value, dict):
            raise ValidationError(f"{location} must be an object")
        validate_exact_keys(value, {"id", "name", "state"}, location)
        attribute_id, name, state = value["id"], value["name"], value["state"]
        if (
            not isinstance(attribute_id, int)
            or isinstance(attribute_id, bool)
            or not MIN_ATTRIBUTE_ID <= attribute_id <= MAX_ATTRIBUTE_ID
        ):
            raise ValidationError(
                f"{location}.id must be in {MIN_ATTRIBUTE_ID}..{MAX_ATTRIBUTE_ID}"
            )
        if attribute_id <= previous_id:
            raise ValidationError("attributes must be sorted by strictly increasing ID")
        previous_id = attribute_id
        if not isinstance(name, str) or not name:
            raise ValidationError(f"{location}.name must be a non-empty string")
        if state not in ("active", "tombstone"):
            raise ValidationError(f"{location}.state must be active or tombstone")
        if attribute_id in ids:
            raise ValidationError(f"duplicate attribute ID {attribute_id}")
        if name in names:
            raise ValidationError(f"duplicate attribute name {name!r}")
        ids.add(attribute_id)
        names.add(name)
        if state == "active":
            active_names.add(name)

    missing = sorted(source_names - active_names)
    extra = sorted(active_names - source_names)
    if missing or extra:
        raise ValidationError(
            "registry and public ordinary Element attributes differ; "
            f"missing={missing}, extra={extra}"
        )
    if baseline is not None:
        validate_history(registry, baseline)


def validate_history(registry: dict[str, Any], baseline: dict[str, Any]) -> None:
    current_by_id = {value["id"]: value for value in registry["attributes"]}
    previous_attributes = baseline.get("attributes")
    if not isinstance(previous_attributes, list):
        raise ValidationError("baseline attributes must be an array")
    previous_ids = {
        value.get("id")
        for value in previous_attributes
        if isinstance(value, dict) and isinstance(value.get("id"), int)
    }
    if len(previous_ids) != len(previous_attributes):
        raise ValidationError("baseline contains invalid or duplicate IDs")
    previous_max_id = max(previous_ids, default=MIN_ATTRIBUTE_ID - 1)
    for current in registry["attributes"]:
        if current["id"] not in previous_ids and current["id"] <= previous_max_id:
            raise ValidationError(
                f"new ID {current['id']} must be greater than previous max ID "
                f"{previous_max_id}"
            )
    for previous in previous_attributes:
        attribute_id = previous["id"]
        current = current_by_id.get(attribute_id)
        if current is None:
            raise ValidationError(f"previous ID {attribute_id} was removed")
        if current["name"] != previous.get("name"):
            raise ValidationError(
                f"previous ID {attribute_id} changed name from "
                f"{previous.get('name')!r} to {current['name']!r}"
            )
        if previous.get("state") == "tombstone" and current["state"] != "tombstone":
            raise ValidationError(f"tombstoned ID {attribute_id} became active")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", type=Path)
    parser.add_argument("--node", help="path to the Node.js executable")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        source_model = extract_typings(args.node)
        registry = read_json(REGISTRY_PATH)
        baseline = read_json(args.baseline) if args.baseline else None
        validate_registry(registry, source_model, baseline)
    except (OSError, subprocess.SubprocessError, ValidationError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    print(f"validated {len(source_model['attributes'])} public Element attributes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
