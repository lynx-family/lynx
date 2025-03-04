# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from core.env.env import RTFEnv
from core.utils.log import Log


class RTFContext:
    def __init__(self, workspace=None):
        self.parent = None
        self.children = []
        self.variables = {}
        self.options = None
        if workspace is None:
            workspace = RTFEnv.get_project_root_path()
        self.insert_variable("workspace", workspace)

    def export(self):
        def builder(value):
            self.__builder(value)

        def coverage(value):
            self.__coverage(value)

        def targets(value):
            self.__targets(value)

        def include(template_path):
            self.options.include(template_path)

        return {
            "options": self.options,
            "builder": builder,
            "coverage": coverage,
            "targets": targets,
            "include": include,
        }

    def find_variable(self, name):
        if name in self.variables:
            return self.variables[name]
        parent = self.parent
        while parent is not None:
            value = parent.find_variable(name)
            if value is not None:
                return value
            parent = self.parent.parent
        return None

    def insert_variable(self, name, value):
        if name in self.variables:
            Log.fatal(f"Variable redefined! {name}")
        self.variables[name] = value

    def update_variable(self, name, value):
        self.variables[name] = value

    def __builder(self, builder: dict):
        self.insert_variable("builder", builder)

    def __coverage(self, coverage: dict):
        self.insert_variable("coverage", coverage)

    def __targets(self, targets: dict):
        self.insert_variable("targets", targets)
