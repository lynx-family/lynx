#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os
import sys


def main():
    missing_outputs = [path for path in sys.argv[1:] if not os.path.isfile(path)]
    if missing_outputs:
        raise FileNotFoundError(
            'Keyword generation outputs are missing: ' +
            ', '.join(missing_outputs)
        )


if __name__ == '__main__':
    main()
