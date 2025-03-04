# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import importlib.resources

from core.utils.log import Log


class Template:
    def __init__(self, template_type):
        self.template_type = template_type

    def _template_file_name(self, name):
        return f"{self.template_type}-{name}.template"

    def _content(self):
        if self.template_type == "native-ut":
            file_content = importlib.resources.read_text(
                "core.template", "native_ut_template.py"
            )
            return file_content
        elif self.template_type == "android-ut":
            file_content = importlib.resources.read_text(
                "core.template", "android_ut_template.py"
            )
            return file_content
        else:
            Log.fatal(f"The template type `{self.template_type}` is not supported!")

    def save_to(self, path, name):
        template_file_name = self._template_file_name(name)
        if os.path.exists(os.path.join(path, template_file_name)):
            Log.warning(
                f"The template with the same name `{template_file_name}` has already been created"
            )
            return
        content = self._content()
        with open(os.path.join(path, template_file_name), "w") as f:
            f.write(content)
