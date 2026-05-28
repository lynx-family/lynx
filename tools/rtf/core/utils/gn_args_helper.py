# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.


def inject_flutter_cxx(builders, enable_flutter_cxx: bool):
    """Inject or update use_flutter_cxx in GN builder args."""
    flutter_cxx_value = "true" if enable_flutter_cxx else "false"
    for builder in builders.values():
        args_list = builder.setdefault("args", [])
        for i, arg in enumerate(args_list):
            if arg.startswith("use_flutter_cxx="):
                args_list[i] = f"use_flutter_cxx={flutter_cxx_value}"
                break
        else:
            args_list.append(f"use_flutter_cxx={flutter_cxx_value}")


def inject_use_xcode(builders, enable_use_xcode: bool):
    """Inject or update use_xcode in GN builder args."""
    use_xcode_value = "true" if enable_use_xcode else "false"
    for builder in builders.values():
        args_list = builder.setdefault("args", [])
        for i, arg in enumerate(args_list):
            if arg.startswith("use_xcode="):
                args_list[i] = f"use_xcode={use_xcode_value}"
                break
        else:
            args_list.append(f"use_xcode={use_xcode_value}")
