# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.


def LCovReader(file_path):
    content = {}
    with open(file_path, "r") as f:
        current_file = None
        for line in f:
            line = line.strip()
            if line.startswith("SF"):
                source_file = line.split(":")[1]
                content[source_file] = {}
                current_file = source_file
            elif line == "end_of_record":
                current_file = None
            else:
                if current_file is not None:
                    key = line.split(":")[0]
                    value = line.split(":")[1]
                    if key not in content[current_file]:
                        content[current_file][line.split(":")[0]] = [value]
                    else:
                        content[current_file][line.split(":")[0]].append(
                            line.split(":")[1]
                        )
    return content
