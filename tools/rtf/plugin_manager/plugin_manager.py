# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from plugins.plugin import Plugin
from plugins.plugin_factory import PluginFactory


class PluginManager:
    def __init__(self, plugins):
        self.plugins = {}
        for plugin_name in plugins:
            plugin = PluginFactory(plugin_name)
            self.register_plugin(plugin)

    def register_plugin(self, plugin: Plugin):
        self.plugins[plugin.name] = plugin

    def dispatch_args(self, args):
        plugin = self.plugins[args.plugin]
        plugin.accept(args)
