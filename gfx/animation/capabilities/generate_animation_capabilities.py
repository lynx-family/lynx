#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import argparse
import json
from pathlib import Path
from typing import Dict, Iterable, List, Tuple


BACKENDS = {
    "ios": ("kIOS", "IOS"),
}

KINDS = {
    "keyframe": "kKeyframe",
    "transition": "kTransition",
}

PROPERTIES = {
    "opacity": "kOpacity",
    "scale-x": "kScaleX",
    "scale-y": "kScaleY",
    "scale-xy": "kScaleXY",
    "transform": "kTransform",
    "background-color": "kBackgroundColor",
    "color": "kColor",
    "visibility": "kVisibility",
    "left": "kLeft",
    "top": "kTop",
    "right": "kRight",
    "bottom": "kBottom",
    "width": "kWidth",
    "height": "kHeight",
    "max-width": "kMaxWidth",
    "min-width": "kMinWidth",
    "max-height": "kMaxHeight",
    "min-height": "kMinHeight",
    "padding-left": "kPaddingLeft",
    "padding-right": "kPaddingRight",
    "padding-top": "kPaddingTop",
    "padding-bottom": "kPaddingBottom",
    "margin-left": "kMarginLeft",
    "margin-right": "kMarginRight",
    "margin-top": "kMarginTop",
    "margin-bottom": "kMarginBottom",
    "border-left-width": "kBorderLeftWidth",
    "border-right-width": "kBorderRightWidth",
    "border-top-width": "kBorderTopWidth",
    "border-bottom-width": "kBorderBottomWidth",
    "border-top-color": "kBorderTopColor",
    "border-left-color": "kBorderLeftColor",
    "border-right-color": "kBorderRightColor",
    "border-bottom-color": "kBorderBottomColor",
    "flex-basis": "kFlexBasis",
    "flex-grow": "kFlexGrow",
    "border-width": "kBorderWidth",
    "border-color": "kBorderColor",
    "margin": "kMargin",
    "padding": "kPadding",
    "filter": "kFilter",
    "box-shadow": "kBoxShadow",
    "offset-distance": "kOffsetDistance",
    "background-position": "kBackgroundPosition",
    "transform-origin": "kTransformOrigin",
}

VALUE_TYPES = {
    "float": "kFloat",
    "color": "kColor",
    "length": "kLength",
    "vec2": "kVec2",
    "filter": "kFilter",
    "transform": "kTransform",
    "box-shadow": "kBoxShadow",
    "enum": "kEnum",
}

TIMING_FUNCTIONS = {
    "linear": "kTimingFunctionLinear",
    "ease": "kTimingFunctionCubicBezier",
    "ease-in": "kTimingFunctionCubicBezier",
    "ease-out": "kTimingFunctionCubicBezier",
    "ease-in-out": "kTimingFunctionCubicBezier",
    "ease-in-ease-out": "kTimingFunctionCubicBezier",
    "square-bezier": "kTimingFunctionCubicBezier",
    "cubic-bezier": "kTimingFunctionCubicBezier",
    "steps": "kTimingFunctionSteps",
}

AXES_XY = {
    "x": "kTransformAxisX",
    "y": "kTransformAxisY",
}

AXES_XYZ = {
    **AXES_XY,
    "z": "kTransformAxisZ",
}

UNITS = {
    "number": "kTransformUnitNumber",
    "percent": "kTransformUnitPercent",
}

MATRIX_DIMENSIONS = {
    "2d": "kTransformMatrix2D",
    "3d": "kTransformMatrix3D",
}

TRANSFORM_OPERATION_FIELDS = {
    "translate": (("axes", AXES_XYZ), ("units", UNITS)),
    "rotate": (("axes", AXES_XYZ),),
    "scale": (("axes", AXES_XY),),
    "skew": (("axes", AXES_XY),),
    "matrix": (("dimensions", MATRIX_DIMENSIONS),),
}


def _require_mapping(value: object, description: str) -> Dict[str, object]:
    if not isinstance(value, dict):
        raise ValueError(f"{description} must be an object")
    return value


def _require_list(value: object, description: str) -> List[object]:
    if not isinstance(value, list) or not value:
        raise ValueError(f"{description} must be a non-empty array")
    return value


def _mapped_values(values: object, mapping: Dict[str, str],
                   description: str) -> List[str]:
    result: List[str] = []
    for value in _require_list(values, description):
        if not isinstance(value, str) or value not in mapping:
            raise ValueError(f"unsupported {description} value: {value}")
        mapped = mapping[value]
        if mapped not in result:
            result.append(mapped)
    return result


def _mask(values: Iterable[str]) -> str:
    values = list(values)
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


def _transform_assignments(capability: Dict[str, object], index: int) -> List[str]:
    features = _require_mapping(capability.get("valueFeatures"),
                                f"capabilities[{index}].valueFeatures")
    operations = _require_mapping(features.get("operations"),
                                  f"capabilities[{index}].valueFeatures.operations")
    unknown_operations = set(operations) - set(TRANSFORM_OPERATION_FIELDS)
    if unknown_operations:
        raise ValueError(
            f"unsupported transform operations: {sorted(unknown_operations)}")

    assignments: List[str] = []
    for operation_name, fields in TRANSFORM_OPERATION_FIELDS.items():
        if operation_name not in operations:
            continue
        operation = _require_mapping(
            operations[operation_name],
            f"capabilities[{index}].valueFeatures.operations.{operation_name}")
        expected_fields = {field_name for field_name, _ in fields}
        unknown_fields = set(operation) - expected_fields
        if unknown_fields:
            raise ValueError(
                f"unsupported {operation_name} fields: {sorted(unknown_fields)}")
        for field_name, mapping in fields:
            values = _mapped_values(
                operation.get(field_name), mapping,
                f"{operation_name}.{field_name}")
            target_field = {
                ("translate", "axes"): "translate_axes",
                ("translate", "units"): "translate_units",
                ("rotate", "axes"): "rotate_axes",
                ("scale", "axes"): "scale_axes",
                ("skew", "axes"): "skew_axes",
                ("matrix", "dimensions"): "matrix_dimensions",
            }[(operation_name, field_name)]
            assignments.append(
                f"    capability.transform.{target_field} = {_mask(values)};")
    return assignments


def generate_header(document: Dict[str, object]) -> str:
    unknown_root_fields = set(document) - {"backend", "capabilities"}
    if unknown_root_fields:
        raise ValueError(
            f"unsupported root fields: {sorted(unknown_root_fields)}")
    backend = document.get("backend")
    if not isinstance(backend, str) or backend not in BACKENDS:
        raise ValueError(f"unsupported backend: {backend}")
    backend_enum, backend_name = BACKENDS[backend]
    capabilities = _require_list(document.get("capabilities"), "capabilities")

    blocks: List[str] = []
    seen: set[Tuple[str, str, str]] = set()
    for index, raw_capability in enumerate(capabilities):
        capability = _require_mapping(raw_capability, f"capabilities[{index}]")
        unknown_capability_fields = set(capability) - {
            "kind", "property", "valueType", "supportsPerKeyframeTiming",
            "timingFunctions", "valueFeatures"
        }
        if unknown_capability_fields:
            raise ValueError(
                "unsupported capability fields: "
                f"{sorted(unknown_capability_fields)}")
        kind = capability.get("kind")
        property_name = capability.get("property")
        value_type = capability.get("valueType")
        if kind not in KINDS:
            raise ValueError(f"unsupported animation kind: {kind}")
        if property_name not in PROPERTIES:
            raise ValueError(f"unsupported animation property: {property_name}")
        if value_type not in VALUE_TYPES:
            raise ValueError(f"unsupported keyframe value type: {value_type}")
        key = (kind, property_name, value_type)
        if key in seen:
            raise ValueError(f"duplicate capability: {key}")
        seen.add(key)

        timing_functions = _mapped_values(
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
            f"    capability.kind = AnimationKind::{KINDS[kind]};",
            f"    capability.property = AnimationPropertyType::{PROPERTIES[property_name]};",
            f"    capability.value_type = KeyframeValueType::{VALUE_TYPES[value_type]};",
            "    capability.supports_per_keyframe_timing = "
            f"{'true' if supports_per_keyframe_timing else 'false'};",
            "    capability.timing_functions =",
            f"        {_mask(timing_functions)};",
        ]
        if value_type == "transform":
            lines.extend(_transform_assignments(capability, index))
        elif "valueFeatures" in capability:
            raise ValueError(
                f"capabilities[{index}].valueFeatures is only supported for transform")
        lines.extend([
            "    capabilities.properties.push_back(capability);",
            "  }",
        ])
        blocks.append("\n".join(lines))

    body = "\n".join(blocks)
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

inline AnimationBackendCapabilities Get{backend_name}AnimationBackendCapabilities() {{
  AnimationBackendCapabilities capabilities;
  capabilities.backend = AnimationBackendType::{backend_enum};
  capabilities.properties.reserve({len(capabilities)});
{body}
  return capabilities;
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
    generated = generate_header(_require_mapping(document, "root"))

    if (not output_path.exists() or
            output_path.read_text(encoding="utf-8") != generated):
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(generated, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
