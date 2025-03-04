# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from plugin_manager.plugin_manager import PluginManager
from args_parser.args_parser import ArgsParser
from core.env.env import RTFEnv


def main():
    RTFEnv.init_project_env()
    plugin_manager = PluginManager(RTFEnv.config.plugins)
    args_parser = ArgsParser()
    args_parser.init_subparsers(plugin_manager.plugins)
    args = args_parser.parse_args()

    if args.plugin is not None:
        plugin_manager.dispatch_args(args)
    else:
        args_parser.print_help()


if __name__ == "__main__":
    main()
