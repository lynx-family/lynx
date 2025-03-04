# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from typing import Callable

from core.builder.gn_builder import GnBuilder
from core.builder.gradle_builder import GradleBuilder
from core.target.target import Target
from core.utils.log import Log


def BuilderFactory(builder_meta_data: dict):
    builder_type = builder_meta_data["type"]
    if builder_type == "gn":
        args = builder_meta_data["args"]
        output = builder_meta_data["output"]
        return GnBuilder(args, output)
    elif builder_type == "gradle":
        args = builder_meta_data["args"]
        workspace = builder_meta_data["workspace"]
        return GradleBuilder(args, workspace)
    else:
        Log.fatal(f"build type {builder_type} is unsupported!")


class BuilderManager:
    def __init__(self, builder_params: dict):
        self.builders = {}
        for builder_name in builder_params.keys():
            self.builders[builder_name] = BuilderFactory(builder_params[builder_name])

    def pre_action(self, skip: Callable[[], bool] = None):
        for builder in self.builders.values():
            builder.pre_action(skip)

    def build(self, target: Target):
        builder_name = (
            target.params["builder"] if "builder" in target.params else "default"
        )
        self.builders[builder_name].build(target)
