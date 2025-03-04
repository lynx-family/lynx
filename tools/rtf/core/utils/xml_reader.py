# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import xml.etree.ElementTree as ElementTree


def XmlReader(file_path):
    with open(file_path, "r") as f:
        content = f.read()
    return ElementTree.fromstring(content)
