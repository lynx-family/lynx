#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import argparse
import json
from pathlib import Path


def _enum_names(names: str, prefix: str = "k") -> dict:
    return {
        name: prefix + "".join(part.capitalize() for part in name.split("-"))
        for name in names.split()
    }


BACKENDS = {"ios": ("kIOS", "IOS")}
KINDS = _enum_names("keyframe transition")
EVENTS = _enum_names("start end cancel iteration", "kAnimationEvent")

PROPERTIES = {
    **_enum_names("""
        opacity scale-x scale-y transform background-color color visibility
        left top right bottom width height max-width min-width max-height min-height
        padding-left padding-right padding-top padding-bottom
        margin-left margin-right margin-top margin-bottom
        border-left-width border-right-width border-top-width border-bottom-width
        border-top-color border-left-color border-right-color border-bottom-color
        flex-basis flex-grow border-width border-color margin padding filter
        box-shadow offset-distance background-position transform-origin
    """),
    "scale-xy": "kScaleXY",
}

VALUE_TYPES = _enum_names("float color length vec2 filter transform box-shadow enum")

TIMING_FUNCTIONS = {
    **dict.fromkeys("""
        ease ease-in ease-out ease-in-out ease-in-ease-out square-bezier cubic-bezier
    """.split(), "kTimingFunctionCubicBezier"),
    "linear": "kTimingFunctionLinear",
    "steps": "kTimingFunctionSteps",
}

AXES_XY = _enum_names("x y", "kTransformAxis")
AXES_XYZ = _enum_names("x y z", "kTransformAxis")
UNITS = _enum_names("number percent", "kTransformUnit")
MATRIX_DIMENSIONS = {"2d": "kTransformMatrix2D", "3d": "kTransformMatrix3D"}

TRANSFORM_OPERATION_FIELDS = {
    "translate": {axis: (f"translate_{axis}_units", UNITS) for axis in "xyz"},
    "rotate": {"axes": ("rotate_axes", AXES_XYZ)},
    "scale": {"axes": ("scale_axes", AXES_XY)},
    "skew": {"axes": ("skew_axes", AXES_XY)},
    "matrix": {"dimensions": ("matrix_dimensions", MATRIX_DIMENSIONS)},
}


def _require_mapping(value: object, description: str, fields=None) -> dict:
    if not isinstance(value, dict):
        raise ValueError(f"{description} must be an object")
    if fields is not None:
        unknown = set(value) - set(fields)
        if unknown:
            raise ValueError(f"unsupported {description} fields: {sorted(unknown)}")
    return value


def _require_list(value: object, description: str) -> list:
    if not isinstance(value, list) or not value:
        raise ValueError(f"{description} must be a non-empty array")
    return value


def _enum_value(value: object, mapping: dict, description: str):
    if not isinstance(value, str) or value not in mapping:
        raise ValueError(f"unsupported {description}: {value}")
    return mapping[value]


def _mask(values: object, mapping: dict, description: str) -> str:
    values = list(dict.fromkeys(
        _enum_value(value, mapping, description)
        for value in _require_list(values, description)))
    aliases = {
        frozenset(("kTransformAxisX", "kTransformAxisY")):
            "kTransformAxesXY",
        frozenset(("kTransformAxisX", "kTransformAxisY",
                   "kTransformAxisZ")): "kTransformAxesXYZ",
        frozenset(("kTransformUnitNumber", "kTransformUnitPercent")):
            "kAllTransformUnits",
        frozenset(("kTransformMatrix2D", "kTransformMatrix3D")):
            "kAllTransformMatrices",
    }
    return aliases.get(frozenset(values), " | ".join(values))


def _transform_assignments(capability: dict, index: int) -> list:
    features = _require_mapping(capability.get("valueFeatures"),
                                f"capabilities[{index}].valueFeatures")
    operations = _require_mapping(features.get("operations"),
                                  f"capabilities[{index}].valueFeatures.operations",
                                  TRANSFORM_OPERATION_FIELDS)

    assignments = []
    for operation_name, fields in TRANSFORM_OPERATION_FIELDS.items():
        if operation_name not in operations:
            continue
        operation = _require_mapping(
            operations[operation_name],
            f"capabilities[{index}].valueFeatures.operations.{operation_name}",
            fields)
        for field_name, (target_field, mapping) in fields.items():
            # An omitted translate axis is unsupported (its mask stays zero).
            if operation_name == "translate" and field_name not in operation:
                continue
            mask = _mask(
                operation.get(field_name), mapping,
                f"{operation_name}.{field_name}")
            assignments.append(
                f"    capability.transform.{target_field} = {mask};")
    return assignments


def generate_header(document: dict) -> str:
    document = _require_mapping(
        document, "root", ("backend", "capabilities", "events"))
    backend = document.get("backend")
    backend_enum, backend_name = _enum_value(backend, BACKENDS, "backend")
    capabilities = _require_list(document.get("capabilities"), "capabilities")

    blocks = []
    events = _require_mapping(document.get("events", {}), "events", KINDS)
    for kind, names in events.items():
        if names == []:
            continue
        mask = _mask(names, EVENTS, f"events.{kind}")
        blocks.append(
            "  capabilities.event_capabilities.push_back(\n"
            f"      {{AnimationKind::{KINDS[kind]}, {mask}}});")
    seen = set()
    for index, raw_capability in enumerate(capabilities):
        capability = _require_mapping(raw_capability, f"capabilities[{index}]", (
            "kind", "property", "valueType", "supportsPerKeyframeTiming",
            "timingFunctions", "valueFeatures"))
        kind = _enum_value(capability.get("kind"), KINDS, "animation kind")
        property_name = _enum_value(capability.get("property"), PROPERTIES,
                                    "animation property")
        value_type = _enum_value(capability.get("valueType"), VALUE_TYPES,
                                "keyframe value type")
        key = (capability["kind"], capability["property"], capability["valueType"])
        if key in seen:
            raise ValueError(f"duplicate capability: {key}")
        seen.add(key)

        timing_functions = _mask(
            capability.get("timingFunctions"), TIMING_FUNCTIONS,
            f"capabilities[{index}].timingFunctions")
        supports_per_keyframe_timing = capability.get(
            "supportsPerKeyframeTiming", False)
        if not isinstance(supports_per_keyframe_timing, bool):
            raise ValueError(
                f"capabilities[{index}].supportsPerKeyframeTiming must be a boolean")

        lines = [
            "  {",
            "    AnimationPropertyCapability capability;",
            f"    capability.kind = AnimationKind::{kind};",
            f"    capability.property = AnimationPropertyType::{property_name};",
            f"    capability.value_type = KeyframeValueType::{value_type};",
            "    capability.supports_per_keyframe_timing = "
            f"{'true' if supports_per_keyframe_timing else 'false'};",
            "    capability.timing_functions =",
            f"        {timing_functions};",
        ]
        if capability["valueType"] == "transform":
            lines.extend(_transform_assignments(capability, index))
        elif "valueFeatures" in capability:
            raise ValueError(
                f"capabilities[{index}].valueFeatures is only supported for transform")
        lines.extend([
            "    capabilities.properties.push_back(capability);",
            "  }",
        ])
        blocks.append("\n".join(lines))

    body = "\n".join("  " + line for block in blocks for line in block.splitlines())
    guard = (
        f"GFX_ANIMATION_CAPABILITIES_{backend.upper()}_ANIMATION_"
        "CAPABILITIES_GENERATED_H_")
    return f"""// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// Generated by generate_animation_capabilities.py. Do not edit.

#ifndef {guard}
#define {guard}

#include \"gfx/animation/platform_animation.h\"

namespace lynx {{
namespace gfx {{

inline const AnimationBackendCapabilities& Get{backend_name}AnimationBackendCapabilities() {{
  static const AnimationBackendCapabilities instance = [] {{
    AnimationBackendCapabilities capabilities;
    capabilities.backend = AnimationBackendType::{backend_enum};
    capabilities.properties.reserve({len(capabilities)});
{body}
    return capabilities;
  }}();
  return instance;
}}

}}  // namespace gfx
}}  // namespace lynx

#endif  // {guard}
"""


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    input_path = Path(args.input)
    output_path = Path(args.output)
    document = json.loads(input_path.read_text(encoding="utf-8"))
    generated = generate_header(document)

    if (not output_path.exists() or
            output_path.read_text(encoding="utf-8") != generated):
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(generated, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
