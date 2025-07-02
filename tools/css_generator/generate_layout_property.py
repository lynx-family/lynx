# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import json
import sys
import hashlib

# Mapping of consumption status to enum values
consumption_status_mapping = {
    "layout-only": "LAYOUT_ONLY",
    "layout-wanted": "LAYOUT_WANTED",
    "skip": "SKIP",
}


def get_consumption_status(file_path):
    with open(file_path, "r") as f:
        data = json.load(f)
        consumption_status = data.get("consumption_status")
        return consumption_status_mapping.get(consumption_status, "SKIP")


def load_previous_hashes(output_path):
    hash_file = output_path + ".hashes"
    if not os.path.exists(hash_file):
        return ""
    try:
        with open(hash_file, "r") as f:
            return f.read().strip()
    except:
        return ""


def save_current_hashes(output_path, hash_value):
    hash_file = output_path + ".hashes"
    with open(hash_file, "w") as f:
        f.write(hash_value)


def get_file_hash(file_path):
    sha256 = hashlib.sha256()
    with open(file_path, "rb") as f:
        while chunk := f.read(4096):
            sha256.update(chunk)
    return sha256.hexdigest()


def load_sha256_file(sha256_path):
    with open(sha256_path, "r") as f:
        return f.readline().strip()


def generate_layout_property(css_defines_dir, sha256_path):
    properties = []
    json_files = []
    for root, dirs, files in os.walk(css_defines_dir):
        for file in files:
            if file.endswith(".json"):
                file_path = os.path.join(root, file)
                json_files.append(file_path)
                try:
                    name = json.load(open(file_path))["name"]
                    status = get_consumption_status(file_path)
                    # Convert name to camel case
                    property_name = "".join(x.capitalize() for x in name.split("-"))
                    if status != "SKIP":
                        properties.append((property_name, status))
                except Exception as e:
                    print(f"Error processing {file_path}: {e}")

    # Sort properties alphabetically
    properties.sort()

    # Generate the layout_property.h content
    header_content = """// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_STYLE_LAYOUT_PROPERTY_H_
#define CORE_STYLE_LAYOUT_PROPERTY_H_

#include "core/renderer/css/css_property.h"

namespace lynx {
namespace tasm {

#define FOREACH_LAYOUT_PROPERTY(V)    \
"""
    for name, status in properties:
        header_content += f"  V({name}, {status})                 \\\n"
    header_content = header_content.rstrip("\\\n") + "\n"

    header_content += """
enum ConsumptionStatus { LAYOUT_ONLY = 0, LAYOUT_WANTED = 1, SKIP = 2 };

class LayoutProperty {
 public:
  LayoutProperty() = delete;
  ~LayoutProperty() = delete;

  static ConsumptionStatus ConsumptionTest(CSSPropertyID id);
  inline static bool IsLayoutOnly(CSSPropertyID id) {
    return ConsumptionTest(id) == LAYOUT_ONLY;
  }
  inline static bool IsLayoutWanted(CSSPropertyID id) {
    return ConsumptionTest(id) == LAYOUT_WANTED;
  }
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_STYLE_LAYOUT_PROPERTY_H_
"""
    return header_content, json_files


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print(
            "Usage: python generate_layout_property.py <css_defines_dir> <output_path> <sha256_path>"
        )
        sys.exit(1)

    css_defines_dir = sys.argv[1]
    output_path = sys.argv[2]
    sha256_path = sys.argv[3]

    # Load previous hashes
    previous_hashes = load_previous_hashes(output_path)
    # Get current hashes
    current_hash = load_sha256_file(sha256_path)

    # Check if hash changed
    if previous_hashes == current_hash and os.path.exists(output_path):
        print("No changes detected. Skipping generation.")
        sys.exit(0)

    content, json_files = generate_layout_property(css_defines_dir, sha256_path)
    with open(output_path, "w") as f:
        f.write(content)

    # Save current hashes for next comparison
    save_current_hashes(output_path, current_hash)
